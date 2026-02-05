#pragma once

#include "emp_lexer.h"
#include "emp_arena.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EmpVec {
    void **items;
    size_t len;
    size_t cap;
} EmpVec;

void emp_vec_init(EmpVec *v);
void emp_vec_free(EmpVec *v);
bool emp_vec_push(EmpVec *v, void *item);

typedef struct EmpDiag {
    EmpSpan span;
    const char *message; // points to arena-allocated string
} EmpDiag;

typedef struct EmpDiags {
    EmpDiag *items;
    size_t len;
    size_t cap;
} EmpDiags;

void emp_diags_init(EmpDiags *d);
void emp_diags_free(EmpDiags *d);
bool emp_diags_push(EmpDiags *d, EmpDiag diag);

// ===== AST =====

typedef struct EmpType EmpType;
typedef struct EmpClassMethod EmpClassMethod;

typedef enum EmpTypeKind {
    EMP_TYPE_AUTO,
    EMP_TYPE_NAME,  // e.g. int, int32, MyStruct
    EMP_TYPE_PTR,   // *T (raw pointer)
    EMP_TYPE_ARRAY, // T[N]
    EMP_TYPE_LIST,  // T[]
    EMP_TYPE_TUPLE, // (T a, U b)
    EMP_TYPE_DYN,   // dyn Base (fat pointer: {data, vtbl})
} EmpTypeKind;

typedef struct EmpTupleField {
    EmpType *ty;
    EmpSlice name; // optional; name.len==0 means unnamed
    EmpSpan span;
} EmpTupleField;

struct EmpType {
    EmpTypeKind kind;
    EmpSpan span;
    union {
        EmpSlice name;
        struct {
            EmpSlice base_name;
        } dyn;
        struct {
            EmpType *pointee;
        } ptr;
        struct {
            EmpType *elem;
            EmpSlice size_text; // for arrays; e.g. "10" (from INT token). len==0 for list.
        } array;
        struct {
            EmpVec fields; // EmpTupleField*
        } tuple;
    } as;
};

typedef enum EmpBinOp {
    EMP_BIN_ADD,
    EMP_BIN_SUB,
    EMP_BIN_MUL,
    EMP_BIN_DIV,
    EMP_BIN_REM,
    EMP_BIN_EQ,
    EMP_BIN_NE,
    EMP_BIN_LT,
    EMP_BIN_LE,
    EMP_BIN_GT,
    EMP_BIN_GE,
    EMP_BIN_AND,
    EMP_BIN_OR,
    EMP_BIN_BITAND,
    EMP_BIN_BITOR,
    EMP_BIN_BITXOR,
    EMP_BIN_SHL,
    EMP_BIN_SHR,
    EMP_BIN_ASSIGN,
    EMP_BIN_ADD_ASSIGN,
    EMP_BIN_SUB_ASSIGN,
    EMP_BIN_MUL_ASSIGN,
    EMP_BIN_DIV_ASSIGN,
    EMP_BIN_REM_ASSIGN,
    EMP_BIN_SHL_ASSIGN,
    EMP_BIN_SHR_ASSIGN,
    EMP_BIN_BITAND_ASSIGN,
    EMP_BIN_BITOR_ASSIGN,
    EMP_BIN_BITXOR_ASSIGN,
} EmpBinOp;

typedef enum EmpUnOp {
    EMP_UN_NEG,
    EMP_UN_NOT,
    EMP_UN_BITNOT,
    EMP_UN_BORROW,
    EMP_UN_BORROW_MUT,
} EmpUnOp;

typedef struct EmpExpr EmpExpr;
typedef struct EmpStmt EmpStmt;

typedef struct EmpFStringPart {
    bool is_expr;
    EmpSlice text; // valid when !is_expr
    EmpExpr *expr; // valid when is_expr
    EmpSpan span;
} EmpFStringPart;

typedef enum EmpExprKind {
    EMP_EXPR_INT,
    EMP_EXPR_FLOAT,
    EMP_EXPR_STRING,
    EMP_EXPR_FSTRING,
    EMP_EXPR_CHAR,
    EMP_EXPR_IDENT,
    EMP_EXPR_UNARY,
    EMP_EXPR_BINARY,
    EMP_EXPR_CALL,
    EMP_EXPR_GROUP,
    EMP_EXPR_CAST,
    EMP_EXPR_TUPLE,
    EMP_EXPR_LIST,
    EMP_EXPR_INDEX,
    EMP_EXPR_MEMBER,
    EMP_EXPR_NEW,
    EMP_EXPR_TERNARY,
    EMP_EXPR_RANGE,
} EmpExprKind;

struct EmpExpr {
    EmpExprKind kind;
    EmpSpan span;
    union {
        EmpSlice lit; // for literals + ident: slice into source
        struct {
            EmpVec parts; // EmpFStringPart*
        } fstring;
        struct {
            EmpUnOp op;
            EmpExpr *rhs;
        } unary;
        struct {
            EmpBinOp op;
            EmpExpr *lhs;
            EmpExpr *rhs;
        } binary;
        struct {
            EmpExpr *callee;
            EmpVec args; // EmpExpr*
            // Optional: set by semantic/typecheck when overload resolution picks a specific
            // symbol name (e.g. `foo__i32_u32`). When empty, codegen falls back to legacy
            // name-based lookup.
            EmpSlice resolved_name;

            // When calling a method on a `dyn Base`, typecheck sets these so codegen can lower
            // an indirect vtable call. When NULL/empty, call lowering is static.
            const EmpClassMethod *dyn_method; // selected virtual method signature (decl in Base)
            EmpSlice dyn_base_name;           // Base name from `dyn Base`
            uint32_t dyn_slot;                // vtable slot index for dyn_method
        } call;
        struct {
            EmpExpr *inner;
        } group;
        struct {
            EmpType *ty;
            EmpExpr *expr;
            // When casting `*Concrete as dyn Base`, typecheck sets this to Concrete.
            // Codegen uses it to pick the right vtable.
            EmpSlice dyn_concrete_name;
        } cast;
        struct {
            EmpVec items; // EmpExpr*
        } tuple;
        struct {
            EmpVec items; // EmpExpr*
        } list;
        struct {
            EmpExpr *base;
            EmpExpr *index;
        } index;
        struct {
            EmpExpr *base;
            EmpSlice member;
        } member;
        struct {
            EmpSlice class_name;
            EmpVec args; // EmpExpr*
        } new_expr;
        struct {
            EmpExpr *cond;
            EmpExpr *then_expr;
            EmpExpr *else_expr;
        } ternary;
        struct {
            EmpExpr *start;
            EmpExpr *end;
            bool inclusive;
        } range;
    } as;
};

typedef enum EmpStmtKind {
    EMP_STMT_VAR,
    EMP_STMT_DROP,
    EMP_STMT_DEFER,
    EMP_STMT_RETURN,
    EMP_STMT_EXPR,
    EMP_STMT_TAG,
    EMP_STMT_BLOCK,
    EMP_STMT_IF,
    EMP_STMT_WHILE,
    EMP_STMT_FOR,
    EMP_STMT_BREAK,
    EMP_STMT_CONTINUE,
    EMP_STMT_MATCH,
    EMP_STMT_EMP_OFF,
    EMP_STMT_EMP_MM_OFF,
} EmpStmtKind;

typedef struct EmpMatchArm {
    bool is_default;
    EmpExpr *pat; // NULL when is_default
    EmpStmt *body; // block
    EmpSpan span;
} EmpMatchArm;

struct EmpStmt {
    EmpStmtKind kind;
    EmpSpan span;
    union {
        struct {
            EmpType *ty;
            EmpSlice name;
            bool is_destructure;
            EmpVec destruct_names; // EmpSlice*; only used when is_destructure
            EmpExpr *init; // optional
        } let_stmt;
        struct {
            EmpSlice name;
        } drop_stmt;
        struct {
            EmpStmt *body; // block
        } defer_stmt;
        struct {
            EmpSlice name; // e.g. `tag` for `#tag`
        } tag_stmt;
        struct {
            EmpExpr *value; // optional
        } ret;
        struct {
            EmpExpr *expr;
        } expr;
        struct {
            EmpVec stmts; // EmpStmt*
        } block;
        struct {
            EmpExpr *cond;
            EmpStmt *then_branch; // block
            EmpStmt *else_branch; // optional: block or if
        } if_stmt;
        struct {
            EmpExpr *cond;
            EmpStmt *body; // block
        } while_stmt;
        struct {
            EmpSlice idx_name;   // e.g. '_' or 'i'
            EmpSlice val_name;   // optional; val_name.len==0 means none
            EmpExpr *iterable;
            EmpStmt *body; // block
        } for_stmt;
        struct {
            EmpExpr *scrutinee;
            EmpVec arms; // EmpMatchArm*
        } match_stmt;
        struct {
            EmpStmt *body; // block
        } emp_off;
        struct {
            EmpStmt *body; // block
        } emp_mm_off;
    } as;
};

typedef struct EmpParam {
    EmpType *ty;
    EmpSlice name;
    EmpSpan span;
} EmpParam;

typedef struct EmpItemFn {
    EmpSlice name;
    EmpSpan span;
    bool is_exported;
    bool is_extern;
    bool is_unsafe;
    bool is_mm_only; // only callable inside `@emp mm off` regions/files
    EmpSlice abi; // optional; empty means target default C ABI
    EmpVec params; // EmpParam*
    EmpType *ret_ty; // optional
    EmpStmt *body; // block; NULL for extern declarations
} EmpItemFn;

typedef struct EmpClassField {
    EmpSlice name;
    EmpType *ty;
    EmpSpan span;
} EmpClassField;

typedef struct EmpClassMethod {
    EmpSlice name; // "init" allowed
    bool is_init;
    bool is_exported;
    bool is_unsafe;
    bool is_virtual;
    EmpVec params; // EmpParam* (types may be auto)
    EmpType *ret_ty; // optional
    EmpStmt *body;   // block
    EmpSpan span;
} EmpClassMethod;

typedef struct EmpItemClass {
    EmpSlice name;
    bool is_exported;
    EmpSlice base_name; // optional; len==0 means none
    EmpVec fields;      // EmpClassField*
    EmpVec methods;     // EmpClassMethod*
    EmpSpan span;
} EmpItemClass;

typedef struct EmpTraitMethod {
    EmpSlice name;
    EmpVec params; // EmpParam* (types may be auto)
    EmpType *ret_ty; // optional
    EmpStmt *body;   // optional (can be NULL)
    EmpSpan span;
} EmpTraitMethod;

typedef struct EmpItemTrait {
    EmpSlice name;
    bool is_exported;
    EmpVec methods; // EmpTraitMethod*
    EmpSpan span;
} EmpItemTrait;

typedef struct EmpItemConst {
    EmpSlice name;
    bool is_exported;
    EmpType *ty;     // optional (auto if omitted)
    EmpExpr *init;   // required
    EmpSpan span;
} EmpItemConst;

typedef struct EmpStructField {
    EmpSlice name;
    EmpType *ty;
    EmpSpan span;
} EmpStructField;

typedef struct EmpItemStruct {
    EmpSlice name;
    bool is_exported;
    EmpVec fields; // EmpStructField*
    EmpSpan span;
} EmpItemStruct;

typedef struct EmpEnumVariant {
    EmpSlice name;
    EmpVec fields; // EmpType* (tuple-style payload types)
    EmpSpan span;
} EmpEnumVariant;

typedef struct EmpItemEnum {
    EmpSlice name;
    bool is_exported;
    EmpVec variants; // EmpEnumVariant*
    EmpSpan span;
} EmpItemEnum;

typedef struct EmpImplMethod {
    EmpSlice name;
    bool is_exported;
    bool is_unsafe;
    EmpVec params; // EmpParam* (types may be auto)
    EmpType *ret_ty; // optional
    EmpStmt *body;   // block
    EmpSpan span;
} EmpImplMethod;

typedef struct EmpItemImpl {
    // Optional: when non-empty, this is a trait impl: `impl Trait for Type { ... }`.
    // When empty, this is an inherent impl: `impl Type { ... }`.
    EmpSlice trait_name;
    EmpSlice target_name;
    EmpVec methods; // EmpImplMethod*
    EmpSpan span;
} EmpItemImpl;

typedef struct EmpUseName {
    EmpSlice name;
    EmpSlice alias; // optional; alias.len==0 means no alias
    EmpSpan span;
} EmpUseName;

typedef struct EmpItemUse {
    EmpSpan span;
    bool allow_private; // `use @...` allows importing private items
    bool wildcard;      // `use foo::*;`
    EmpSlice from_path; // text like `std.io` or `network.socket`
    EmpVec names;       // EmpUseName*; when !wildcard. Supports `use {a, b as c} from x.y;`
} EmpItemUse;

typedef struct EmpItemTag {
    EmpSlice name; // e.g. `tag` for `#tag`
    EmpSpan span;
} EmpItemTag;

typedef struct EmpItemEmpMmOff {
    // File-level directive: `@emp mm off;`
    EmpSpan span;
} EmpItemEmpMmOff;

typedef enum EmpItemKind {
    EMP_ITEM_TAG,
    EMP_ITEM_EMP_MM_OFF,
    EMP_ITEM_FN,
    EMP_ITEM_USE,
    EMP_ITEM_CLASS,
    EMP_ITEM_TRAIT,
    EMP_ITEM_CONST,
    EMP_ITEM_STRUCT,
    EMP_ITEM_ENUM,
    EMP_ITEM_IMPL,
} EmpItemKind;

typedef struct EmpItem {
    EmpItemKind kind;
    EmpSpan span;
    union {
        EmpItemTag tag;
        EmpItemEmpMmOff emp_mm_off;
        EmpItemFn fn;
        EmpItemUse use;
        EmpItemClass class_decl;
        EmpItemTrait trait_decl;
        EmpItemConst const_decl;
        EmpItemStruct struct_decl;
        EmpItemEnum enum_decl;
        EmpItemImpl impl_decl;
    } as;
} EmpItem;

typedef struct EmpProgram {
    EmpVec items; // EmpItem*
} EmpProgram;

// Frees heap-backed vectors within the AST (does NOT free nodes themselves).
// Call this before freeing the arena that owns the AST nodes.
void emp_program_free_vectors(EmpProgram *p);

// Helpers
const char *emp_binop_name(EmpBinOp op);
const char *emp_unop_name(EmpUnOp op);

const char *emp_type_kind_name(EmpTypeKind kind);

#ifdef __cplusplus
}
#endif
