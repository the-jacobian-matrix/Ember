#include "emp_ast.h"

#include <stdlib.h>
#include <string.h>

void emp_vec_init(EmpVec *v) {
    v->items = NULL;
    v->len = 0;
    v->cap = 0;
}

void emp_vec_free(EmpVec *v) {
    free(v->items);
    v->items = NULL;
    v->len = 0;
    v->cap = 0;
}

bool emp_vec_push(EmpVec *v, void *item) {
    if (v->len + 1 > v->cap) {
        size_t new_cap = v->cap ? (v->cap * 2) : 8;
        void **new_items = (void **)realloc(v->items, new_cap * sizeof(void *));
        if (!new_items) return false;
        v->items = new_items;
        v->cap = new_cap;
    }
    v->items[v->len++] = item;
    return true;
}

void emp_diags_init(EmpDiags *d) {
    d->items = NULL;
    d->len = 0;
    d->cap = 0;
}

void emp_diags_free(EmpDiags *d) {
    free(d->items);
    d->items = NULL;
    d->len = 0;
    d->cap = 0;
}

bool emp_diags_push(EmpDiags *d, EmpDiag diag) {
    if (d->len + 1 > d->cap) {
        size_t new_cap = d->cap ? (d->cap * 2) : 8;
        EmpDiag *new_items = (EmpDiag *)realloc(d->items, new_cap * sizeof(EmpDiag));
        if (!new_items) return false;
        d->items = new_items;
        d->cap = new_cap;
    }
    d->items[d->len++] = diag;
    return true;
}

const char *emp_binop_name(EmpBinOp op) {
    switch (op) {
        case EMP_BIN_ADD: return "+";
        case EMP_BIN_SUB: return "-";
        case EMP_BIN_MUL: return "*";
        case EMP_BIN_DIV: return "/";
        case EMP_BIN_REM: return "%";
        case EMP_BIN_EQ: return "==";
        case EMP_BIN_NE: return "!=";
        case EMP_BIN_LT: return "<";
        case EMP_BIN_LE: return "<=";
        case EMP_BIN_GT: return ">";
        case EMP_BIN_GE: return ">=";
        case EMP_BIN_AND: return "&&";
        case EMP_BIN_OR: return "||";
        case EMP_BIN_BITAND: return "&";
        case EMP_BIN_BITOR: return "|";
        case EMP_BIN_BITXOR: return "^";
        case EMP_BIN_SHL: return "<<";
        case EMP_BIN_SHR: return ">>";
        case EMP_BIN_ASSIGN: return "=";
        case EMP_BIN_ADD_ASSIGN: return "+=";
        case EMP_BIN_SUB_ASSIGN: return "-=";
        case EMP_BIN_MUL_ASSIGN: return "*=";
        case EMP_BIN_DIV_ASSIGN: return "/=";
        case EMP_BIN_REM_ASSIGN: return "%=";
        case EMP_BIN_SHL_ASSIGN: return "<<=";
        case EMP_BIN_SHR_ASSIGN: return ">>=";
        case EMP_BIN_BITAND_ASSIGN: return "&=";
        case EMP_BIN_BITOR_ASSIGN: return "|=";
        case EMP_BIN_BITXOR_ASSIGN: return "^=";
        default: return "?";
    }
}

const char *emp_unop_name(EmpUnOp op) {
    switch (op) {
        case EMP_UN_NEG: return "-";
        case EMP_UN_NOT: return "!";
        case EMP_UN_BITNOT: return "~";
        case EMP_UN_BORROW: return "&";
        case EMP_UN_BORROW_MUT: return "&mut";
        default: return "?";
    }
}

const char *emp_type_kind_name(EmpTypeKind kind) {
    switch (kind) {
        case EMP_TYPE_AUTO: return "Auto";
        case EMP_TYPE_NAME: return "Name";
        case EMP_TYPE_PTR: return "Ptr";
        case EMP_TYPE_ARRAY: return "Array";
        case EMP_TYPE_LIST: return "List";
        case EMP_TYPE_TUPLE: return "Tuple";
        case EMP_TYPE_DYN: return "Dyn";
        default: return "Unknown";
    }
}

static void emp_expr_free_vectors(EmpExpr *e);
static void emp_stmt_free_vectors(EmpStmt *s);

static void emp_type_free_vectors(EmpType *t) {
    if (!t) return;
    switch (t->kind) {
        case EMP_TYPE_PTR:
            emp_type_free_vectors(t->as.ptr.pointee);
            break;
        case EMP_TYPE_ARRAY:
        case EMP_TYPE_LIST:
            emp_type_free_vectors(t->as.array.elem);
            break;
        case EMP_TYPE_TUPLE:
            for (size_t i = 0; i < t->as.tuple.fields.len; i++) {
                EmpTupleField *f = (EmpTupleField *)t->as.tuple.fields.items[i];
                if (f) emp_type_free_vectors(f->ty);
            }
            emp_vec_free(&t->as.tuple.fields);
            break;
        default:
            break;
    }
}

static void emp_expr_free_vectors(EmpExpr *e) {
    if (!e) return;

    switch (e->kind) {
        case EMP_EXPR_FSTRING:
            for (size_t i = 0; i < e->as.fstring.parts.len; i++) {
                EmpFStringPart *p = (EmpFStringPart *)e->as.fstring.parts.items[i];
                if (!p) continue;
                if (p->is_expr) emp_expr_free_vectors(p->expr);
            }
            emp_vec_free(&e->as.fstring.parts);
            break;
        case EMP_EXPR_UNARY:
            emp_expr_free_vectors(e->as.unary.rhs);
            break;
        case EMP_EXPR_BINARY:
            emp_expr_free_vectors(e->as.binary.lhs);
            emp_expr_free_vectors(e->as.binary.rhs);
            break;
        case EMP_EXPR_CALL:
            emp_expr_free_vectors(e->as.call.callee);
            for (size_t i = 0; i < e->as.call.args.len; i++) {
                emp_expr_free_vectors((EmpExpr *)e->as.call.args.items[i]);
            }
            emp_vec_free(&e->as.call.args);
            break;
        case EMP_EXPR_GROUP:
            emp_expr_free_vectors(e->as.group.inner);
            break;
        case EMP_EXPR_CAST:
            emp_type_free_vectors(e->as.cast.ty);
            emp_expr_free_vectors(e->as.cast.expr);
            break;
        case EMP_EXPR_TUPLE:
            for (size_t i = 0; i < e->as.tuple.items.len; i++) {
                emp_expr_free_vectors((EmpExpr *)e->as.tuple.items.items[i]);
            }
            emp_vec_free(&e->as.tuple.items);
            break;
        case EMP_EXPR_LIST:
            for (size_t i = 0; i < e->as.list.items.len; i++) {
                emp_expr_free_vectors((EmpExpr *)e->as.list.items.items[i]);
            }
            emp_vec_free(&e->as.list.items);
            break;
        case EMP_EXPR_INDEX:
            emp_expr_free_vectors(e->as.index.base);
            emp_expr_free_vectors(e->as.index.index);
            break;
        case EMP_EXPR_MEMBER:
            emp_expr_free_vectors(e->as.member.base);
            break;
        case EMP_EXPR_NEW:
            for (size_t i = 0; i < e->as.new_expr.args.len; i++) {
                EmpExpr *arg = (EmpExpr *)e->as.new_expr.args.items[i];
                emp_expr_free_vectors(arg);
            }
            emp_vec_free(&e->as.new_expr.args);
            break;
        case EMP_EXPR_TERNARY:
            emp_expr_free_vectors(e->as.ternary.cond);
            emp_expr_free_vectors(e->as.ternary.then_expr);
            emp_expr_free_vectors(e->as.ternary.else_expr);
            break;
        case EMP_EXPR_RANGE:
            emp_expr_free_vectors(e->as.range.start);
            emp_expr_free_vectors(e->as.range.end);
            break;
        default:
            break;
    }
}

static void emp_stmt_free_vectors(EmpStmt *s) {
    if (!s) return;

    switch (s->kind) {
        case EMP_STMT_VAR:
            emp_type_free_vectors(s->as.let_stmt.ty);
            emp_expr_free_vectors(s->as.let_stmt.init);
            emp_vec_free(&s->as.let_stmt.destruct_names);
            break;
        case EMP_STMT_DROP:
            break;
        case EMP_STMT_RETURN:
            emp_expr_free_vectors(s->as.ret.value);
            break;
        case EMP_STMT_EXPR:
            emp_expr_free_vectors(s->as.expr.expr);
            break;
        case EMP_STMT_TAG:
            break;
        case EMP_STMT_BLOCK:
            for (size_t i = 0; i < s->as.block.stmts.len; i++) {
                emp_stmt_free_vectors((EmpStmt *)s->as.block.stmts.items[i]);
            }
            emp_vec_free(&s->as.block.stmts);
            break;
        case EMP_STMT_IF:
            emp_expr_free_vectors(s->as.if_stmt.cond);
            emp_stmt_free_vectors(s->as.if_stmt.then_branch);
            emp_stmt_free_vectors(s->as.if_stmt.else_branch);
            break;
        case EMP_STMT_WHILE:
            emp_expr_free_vectors(s->as.while_stmt.cond);
            emp_stmt_free_vectors(s->as.while_stmt.body);
            break;
        case EMP_STMT_FOR:
            emp_expr_free_vectors(s->as.for_stmt.iterable);
            emp_stmt_free_vectors(s->as.for_stmt.body);
            break;
        case EMP_STMT_BREAK:
        case EMP_STMT_CONTINUE:
            break;
        case EMP_STMT_MATCH:
            emp_expr_free_vectors(s->as.match_stmt.scrutinee);
            for (size_t i = 0; i < s->as.match_stmt.arms.len; i++) {
                EmpMatchArm *a = (EmpMatchArm *)s->as.match_stmt.arms.items[i];
                if (!a) continue;
                emp_expr_free_vectors(a->pat);
                emp_stmt_free_vectors(a->body);
            }
            emp_vec_free(&s->as.match_stmt.arms);
            break;
            case EMP_STMT_DEFER:
                emp_stmt_free_vectors(s->as.defer_stmt.body);
                break;
        case EMP_STMT_EMP_OFF:
            emp_stmt_free_vectors(s->as.emp_off.body);
            break;
        case EMP_STMT_EMP_MM_OFF:
            emp_stmt_free_vectors(s->as.emp_mm_off.body);
            break;
        default:
            break;
    }
}

void emp_program_free_vectors(EmpProgram *p) {
    if (!p) return;

    for (size_t i = 0; i < p->items.len; i++) {
        EmpItem *it = (EmpItem *)p->items.items[i];
        if (!it) continue;

        switch (it->kind) {
            case EMP_ITEM_TAG:
                break;
            case EMP_ITEM_EMP_MM_OFF:
                break;
            case EMP_ITEM_FN:
                for (size_t j = 0; j < it->as.fn.params.len; j++) {
                    EmpParam *param = (EmpParam *)it->as.fn.params.items[j];
                    if (param) emp_type_free_vectors(param->ty);
                }
                emp_vec_free(&it->as.fn.params);
                emp_type_free_vectors(it->as.fn.ret_ty);
                emp_stmt_free_vectors(it->as.fn.body);
                break;
            case EMP_ITEM_USE:
                emp_vec_free(&it->as.use.names);
                break;
            case EMP_ITEM_CLASS:
                for (size_t j = 0; j < it->as.class_decl.fields.len; j++) {
                    EmpClassField *f = (EmpClassField *)it->as.class_decl.fields.items[j];
                    if (f) emp_type_free_vectors(f->ty);
                }
                for (size_t j = 0; j < it->as.class_decl.methods.len; j++) {
                    EmpClassMethod *m = (EmpClassMethod *)it->as.class_decl.methods.items[j];
                    if (!m) continue;
                    for (size_t k = 0; k < m->params.len; k++) {
                        EmpParam *param = (EmpParam *)m->params.items[k];
                        if (param) emp_type_free_vectors(param->ty);
                    }
                    emp_vec_free(&m->params);
                    emp_type_free_vectors(m->ret_ty);
                    emp_stmt_free_vectors(m->body);
                }
                emp_vec_free(&it->as.class_decl.fields);
                emp_vec_free(&it->as.class_decl.methods);
                break;
            case EMP_ITEM_TRAIT:
                for (size_t j = 0; j < it->as.trait_decl.methods.len; j++) {
                    EmpTraitMethod *m = (EmpTraitMethod *)it->as.trait_decl.methods.items[j];
                    if (!m) continue;
                    for (size_t k = 0; k < m->params.len; k++) {
                        EmpParam *param = (EmpParam *)m->params.items[k];
                        if (param) emp_type_free_vectors(param->ty);
                    }
                    emp_vec_free(&m->params);
                    emp_type_free_vectors(m->ret_ty);
                    emp_stmt_free_vectors(m->body);
                }
                emp_vec_free(&it->as.trait_decl.methods);
                break;
            case EMP_ITEM_CONST:
                emp_type_free_vectors(it->as.const_decl.ty);
                emp_expr_free_vectors(it->as.const_decl.init);
                break;
            case EMP_ITEM_STRUCT:
                for (size_t j = 0; j < it->as.struct_decl.fields.len; j++) {
                    EmpStructField *f = (EmpStructField *)it->as.struct_decl.fields.items[j];
                    if (f) emp_type_free_vectors(f->ty);
                }
                emp_vec_free(&it->as.struct_decl.fields);
                break;
            case EMP_ITEM_ENUM:
                for (size_t j = 0; j < it->as.enum_decl.variants.len; j++) {
                    EmpEnumVariant *v = (EmpEnumVariant *)it->as.enum_decl.variants.items[j];
                    if (!v) continue;
                    for (size_t k = 0; k < v->fields.len; k++) {
                        EmpType *ty = (EmpType *)v->fields.items[k];
                        emp_type_free_vectors(ty);
                    }
                    emp_vec_free(&v->fields);
                }
                emp_vec_free(&it->as.enum_decl.variants);
                break;
            case EMP_ITEM_IMPL:
                for (size_t j = 0; j < it->as.impl_decl.methods.len; j++) {
                    EmpImplMethod *m = (EmpImplMethod *)it->as.impl_decl.methods.items[j];
                    if (!m) continue;
                    for (size_t k = 0; k < m->params.len; k++) {
                        EmpParam *param = (EmpParam *)m->params.items[k];
                        if (param) emp_type_free_vectors(param->ty);
                    }
                    emp_vec_free(&m->params);
                    emp_type_free_vectors(m->ret_ty);
                    emp_stmt_free_vectors(m->body);
                }
                emp_vec_free(&it->as.impl_decl.methods);
                break;
            default:
                break;
        }
    }

    emp_vec_free(&p->items);
}
