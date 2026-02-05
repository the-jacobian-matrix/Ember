#include "emp_drop.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum EmpDropState {
    EMP_DROP_UNINIT = 0,
    EMP_DROP_LIVE = 1,
    EMP_DROP_MOVED = 2,
    EMP_DROP_MAYBE_MOVED = 3,
} EmpDropState;

typedef struct EmpDropBind {
    EmpSlice name;
    EmpSpan decl_span;
    bool owned;
    int state; // EmpDropState
} EmpDropBind;

typedef struct EmpDropStack {
    EmpDropBind *items;
    size_t len;
    size_t cap;

    size_t *scopes;
    size_t scopes_len;
    size_t scopes_cap;
} EmpDropStack;

typedef enum EmpUseKind {
    EMP_USE_READ,
    EMP_USE_MOVE,
} EmpUseKind;

typedef struct EmpDropCtx {
    EmpArena *arena;
    EmpDiags *diags;

    EmpDropStack ds;
    unsigned tmp_counter;

    // Loop stack: each entry is a ds.len mark at loop entry.
    // Used to insert drops on `break`/`continue` for loop-local bindings.
    size_t loop_marks[64];
    size_t loop_depth;
} EmpDropCtx;

static void loop_push(EmpDropCtx *c) {
    if (!c) return;
    if (c->loop_depth < (sizeof(c->loop_marks) / sizeof(c->loop_marks[0]))) {
        c->loop_marks[c->loop_depth++] = c->ds.len;
    }
}

static void loop_pop(EmpDropCtx *c) {
    if (!c) return;
    if (c->loop_depth) c->loop_depth--;
}

static bool loop_active(const EmpDropCtx *c) {
    return c && c->loop_depth > 0;
}

static size_t loop_mark(const EmpDropCtx *c) {
    if (!c || c->loop_depth == 0) return 0;
    return c->loop_marks[c->loop_depth - 1];
}

static void *arena_alloc(EmpArena *a, size_t size, size_t align) {
    return emp_arena_alloc(a, size, align);
}

static char *arena_strdup_n(EmpArena *a, const char *s, size_t n) {
    char *p = (char *)arena_alloc(a, n + 1, 1);
    if (!p) return NULL;
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

static char *arena_strdup(EmpArena *a, const char *s) {
    return arena_strdup_n(a, s, strlen(s));
}

static bool slice_eq(EmpSlice a, EmpSlice b) {
    if (a.len != b.len) return false;
    if (a.ptr == b.ptr) return true;
    if (!a.ptr || !b.ptr) return false;
    return memcmp(a.ptr, b.ptr, a.len) == 0;
}

static const EmpProgram *g_drop_program = NULL;

static const EmpItem *drop_find_enum_decl(const EmpProgram *program, EmpSlice name) {
    if (!program || !name.ptr || !name.len) return NULL;
    for (size_t i = 0; i < program->items.len; i++) {
        const EmpItem *it = (const EmpItem *)program->items.items[i];
        if (!it || it->kind != EMP_ITEM_ENUM) continue;
        if (slice_eq(it->as.enum_decl.name, name)) return it;
    }
    return NULL;
}

static const EmpEnumVariant *drop_find_enum_variant(const EmpItem *en, EmpSlice vname, size_t *out_idx) {
    if (!en || en->kind != EMP_ITEM_ENUM || !vname.ptr || !vname.len) return NULL;
    for (size_t i = 0; i < en->as.enum_decl.variants.len; i++) {
        const EmpEnumVariant *v = (const EmpEnumVariant *)en->as.enum_decl.variants.items[i];
        if (!v) continue;
        if (slice_eq(v->name, vname)) {
            if (out_idx) *out_idx = i;
            return v;
        }
    }
    return NULL;
}

    static bool slice_is_one_of(EmpSlice s, const char *const *names, size_t names_len) {
        for (size_t i = 0; i < names_len; i++) {
            const char *n = names[i];
            size_t nlen = strlen(n);
            if (s.len == nlen && s.ptr && memcmp(s.ptr, n, nlen) == 0) return true;
        }
        return false;
    }

    static bool type_is_copy_like(const EmpType *ty) {
        if (!ty) return false;
        switch (ty->kind) {
            case EMP_TYPE_PTR:
                return true;
            case EMP_TYPE_NAME: {
                static const char *const copy_names[] = {
                    "bool",
                    "char",
                    "int",
                    "i8",
                    "i16",
                    "i32",
                    "i64",
                    "isize",
                    "u8",
                    "u16",
                    "u32",
                    "u64",
                    "usize",
                    "float",
                    "double",
                    "f32",
                    "f64",
                };
                return slice_is_one_of(ty->as.name, copy_names, sizeof(copy_names) / sizeof(copy_names[0]));
            }
            default:
                return false;
        }
    }

static void diagf(EmpArena *arena, EmpDiags *diags, EmpSpan span, const char *fmt, EmpSlice name) {
    char name_buf[128];
    size_t n = 0;
    if (name.ptr && name.len) {
        n = name.len < sizeof(name_buf) - 1 ? name.len : sizeof(name_buf) - 1;
        memcpy(name_buf, name.ptr, n);
    }
    name_buf[n] = '\0';

    char msg[256];
    snprintf(msg, sizeof(msg), fmt, name_buf);

    EmpDiag d;
    d.span = span;
    d.message = arena_strdup(arena, msg);
    (void)emp_diags_push(diags, d);
}

static void ds_init(EmpDropStack *ds) {
    memset(ds, 0, sizeof(*ds));
}

static void ds_free(EmpDropStack *ds) {
    free(ds->items);
    free(ds->scopes);
    memset(ds, 0, sizeof(*ds));
}

static bool ds_push_scope(EmpDropStack *ds) {
    if (ds->scopes_len + 1 > ds->scopes_cap) {
        size_t new_cap = ds->scopes_cap ? ds->scopes_cap * 2 : 16;
        size_t *p = (size_t *)realloc(ds->scopes, new_cap * sizeof(size_t));
        if (!p) return false;
        ds->scopes = p;
        ds->scopes_cap = new_cap;
    }
    ds->scopes[ds->scopes_len++] = ds->len;
    return true;
}

static size_t ds_scope_mark(const EmpDropStack *ds) {
    if (!ds->scopes_len) return 0;
    return ds->scopes[ds->scopes_len - 1];
}

static void ds_pop_scope(EmpDropStack *ds) {
    if (!ds->scopes_len) return;
    size_t mark = ds->scopes[--ds->scopes_len];
    if (mark <= ds->len) ds->len = mark;
}

static bool ds_push_bind(EmpDropStack *ds, EmpSlice name, EmpSpan decl_span, bool owned, EmpDropState init_state) {
    if (ds->len + 1 > ds->cap) {
        size_t new_cap = ds->cap ? ds->cap * 2 : 32;
        EmpDropBind *p = (EmpDropBind *)realloc(ds->items, new_cap * sizeof(EmpDropBind));
        if (!p) return false;
        ds->items = p;
        ds->cap = new_cap;
    }
    EmpDropBind *b = &ds->items[ds->len++];
    b->name = name;
    b->decl_span = decl_span;
    b->owned = owned;
    b->state = (int)init_state;
    return true;
}

static EmpDropBind *ds_lookup(EmpDropStack *ds, EmpSlice name) {
    for (size_t i = ds->len; i > 0; i--) {
        EmpDropBind *b = &ds->items[i - 1];
        if (slice_eq(b->name, name)) return b;
    }
    return NULL;
}

static EmpDropState merge_state(EmpDropState a, EmpDropState b) {
    if (a == b) return a;
    if (a == EMP_DROP_MAYBE_MOVED || b == EMP_DROP_MAYBE_MOVED) return EMP_DROP_MAYBE_MOVED;
    // any disagreement becomes "maybe moved" for now
    return EMP_DROP_MAYBE_MOVED;
}

static bool ds_clone(EmpDropStack *dst, const EmpDropStack *src) {
    memset(dst, 0, sizeof(*dst));

    if (src->cap) {
        dst->items = (EmpDropBind *)malloc(src->cap * sizeof(EmpDropBind));
        if (!dst->items) return false;
        memcpy(dst->items, src->items, src->cap * sizeof(EmpDropBind));
        dst->cap = src->cap;
    }
    dst->len = src->len;

    if (src->scopes_cap) {
        dst->scopes = (size_t *)malloc(src->scopes_cap * sizeof(size_t));
        if (!dst->scopes) {
            free(dst->items);
            memset(dst, 0, sizeof(*dst));
            return false;
        }
        memcpy(dst->scopes, src->scopes, src->scopes_cap * sizeof(size_t));
        dst->scopes_cap = src->scopes_cap;
    }
    dst->scopes_len = src->scopes_len;
    return true;
}

static void ds_merge_prefix(EmpDropStack *dst, const EmpDropStack *a, const EmpDropStack *b, size_t prefix_len) {
    for (size_t i = 0; i < prefix_len; i++) {
        EmpDropState sa = (EmpDropState)a->items[i].state;
        EmpDropState sb = (EmpDropState)b->items[i].state;
        dst->items[i].state = (int)merge_state(sa, sb);
    }
}

static bool is_assign_like(EmpBinOp op) {
    switch (op) {
        case EMP_BIN_ASSIGN:
        case EMP_BIN_ADD_ASSIGN:
        case EMP_BIN_SUB_ASSIGN:
        case EMP_BIN_MUL_ASSIGN:
        case EMP_BIN_DIV_ASSIGN:
        case EMP_BIN_REM_ASSIGN:
        case EMP_BIN_SHL_ASSIGN:
        case EMP_BIN_SHR_ASSIGN:
        case EMP_BIN_BITAND_ASSIGN:
        case EMP_BIN_BITOR_ASSIGN:
        case EMP_BIN_BITXOR_ASSIGN:
            return true;
        default:
            return false;
    }
}

static bool expr_is_borrow(const EmpExpr *e) {
    return e && e->kind == EMP_EXPR_UNARY && (e->as.unary.op == EMP_UN_BORROW || e->as.unary.op == EMP_UN_BORROW_MUT);
}

static EmpStmt *make_stmt(EmpArena *arena, EmpStmtKind kind, EmpSpan span) {
    EmpStmt *s = (EmpStmt *)arena_alloc(arena, sizeof(EmpStmt), (size_t)_Alignof(EmpStmt));
    if (!s) return NULL;
    memset(s, 0, sizeof(*s));
    s->kind = kind;
    s->span = span;
    return s;
}

static EmpExpr *make_expr(EmpArena *arena, EmpExprKind kind, EmpSpan span) {
    EmpExpr *e = (EmpExpr *)arena_alloc(arena, sizeof(EmpExpr), (size_t)_Alignof(EmpExpr));
    if (!e) return NULL;
    memset(e, 0, sizeof(*e));
    e->kind = kind;
    e->span = span;
    return e;
}

static EmpType *make_auto_type(EmpArena *arena, EmpSpan span) {
    EmpType *t = (EmpType *)arena_alloc(arena, sizeof(EmpType), (size_t)_Alignof(EmpType));
    if (!t) return NULL;
    memset(t, 0, sizeof(*t));
    t->kind = EMP_TYPE_AUTO;
    t->span = span;
    return t;
}

static EmpStmt *make_drop_stmt(EmpArena *arena, EmpSpan span, EmpSlice name) {
    EmpStmt *s = make_stmt(arena, EMP_STMT_DROP, span);
    if (!s) return NULL;
    s->as.drop_stmt.name = name;
    return s;
}

static EmpSlice make_tmp_name(EmpDropCtx *c) {
    char buf[64];
    snprintf(buf, sizeof(buf), "__emp_tmp%u", c->tmp_counter++);
    char *owned = arena_strdup(c->arena, buf);
    EmpSlice s;
    s.ptr = owned;
    s.len = owned ? strlen(owned) : 0;
    return s;
}

static void visit_expr(EmpDropCtx *c, const EmpExpr *e, EmpUseKind use);
static EmpStmt *rewrite_stmt(EmpDropCtx *c, EmpStmt *s, bool *out_terminated);

static void use_ident(EmpDropCtx *c, EmpSpan span, EmpSlice name, EmpUseKind use) {
    EmpDropBind *b = ds_lookup(&c->ds, name);
    if (!b) return;
    if (!b->owned) return;

    if (use == EMP_USE_MOVE) {
        if (b->state == EMP_DROP_LIVE) {
            b->state = EMP_DROP_MOVED;
        } else if (b->state == EMP_DROP_UNINIT) {
            // moving an uninitialized value isn't handled yet; ownership pass should diagnose
            b->state = EMP_DROP_MOVED;
        } else if (b->state == EMP_DROP_MOVED) {
            // already moved
        } else {
            // maybe moved stays maybe moved
        }
    } else {
        // reads don't affect state here
        (void)span;
    }
}

static void visit_expr(EmpDropCtx *c, const EmpExpr *e, EmpUseKind use) {
    if (!e) return;

    switch (e->kind) {
        case EMP_EXPR_IDENT:
            use_ident(c, e->span, e->as.lit, use);
            return;

        case EMP_EXPR_INT:
        case EMP_EXPR_FLOAT:
        case EMP_EXPR_STRING:
        case EMP_EXPR_CHAR:
            return;

        case EMP_EXPR_FSTRING:
            for (size_t i = 0; i < e->as.fstring.parts.len; i++) {
                const EmpFStringPart *pt = (const EmpFStringPart *)e->as.fstring.parts.items[i];
                if (!pt || !pt->is_expr) continue;
                visit_expr(c, pt->expr, EMP_USE_READ);
            }
            return;

        case EMP_EXPR_GROUP:
            visit_expr(c, e->as.group.inner, use);
            return;

        case EMP_EXPR_TUPLE:
            for (size_t i = 0; i < e->as.tuple.items.len; i++) {
                visit_expr(c, (const EmpExpr *)e->as.tuple.items.items[i], use);
            }
            return;

        case EMP_EXPR_INDEX:
            visit_expr(c, e->as.index.base, EMP_USE_READ);
            visit_expr(c, e->as.index.index, EMP_USE_READ);
            return;

        case EMP_EXPR_MEMBER:
            visit_expr(c, e->as.member.base, EMP_USE_READ);
            return;

        case EMP_EXPR_NEW:
            for (size_t i = 0; i < e->as.new_expr.args.len; i++) {
                visit_expr(c, (const EmpExpr *)e->as.new_expr.args.items[i], EMP_USE_MOVE);
            }
            return;

        case EMP_EXPR_CALL:
            visit_expr(c, e->as.call.callee, EMP_USE_READ);
            for (size_t i = 0; i < e->as.call.args.len; i++) {
                visit_expr(c, (const EmpExpr *)e->as.call.args.items[i], EMP_USE_MOVE);
            }
            return;

        case EMP_EXPR_UNARY:
            if (e->as.unary.op == EMP_UN_BORROW || e->as.unary.op == EMP_UN_BORROW_MUT) {
                visit_expr(c, e->as.unary.rhs, EMP_USE_READ);
            } else {
                visit_expr(c, e->as.unary.rhs, use);
            }
            return;

        case EMP_EXPR_BINARY: {
            EmpBinOp op = e->as.binary.op;
            if (is_assign_like(op)) {
                EmpUseKind rhs_use = (op == EMP_BIN_ASSIGN) ? EMP_USE_MOVE : EMP_USE_READ;
                visit_expr(c, e->as.binary.rhs, rhs_use);
                visit_expr(c, e->as.binary.lhs, EMP_USE_READ);
                return;
            }

            visit_expr(c, e->as.binary.lhs, EMP_USE_READ);
            visit_expr(c, e->as.binary.rhs, EMP_USE_READ);
            return;
        }

        case EMP_EXPR_RANGE:
            visit_expr(c, e->as.range.start, EMP_USE_READ);
            visit_expr(c, e->as.range.end, EMP_USE_READ);
            return;

        default:
            return;
    }
}

static void append_scope_end_drops(EmpDropCtx *c, EmpVec *out) {
    size_t mark = ds_scope_mark(&c->ds);
    for (size_t i = c->ds.len; i > mark; i--) {
        EmpDropBind *b = &c->ds.items[i - 1];
        if (!b->owned) continue;

        EmpDropState st = (EmpDropState)b->state;
        if (st == EMP_DROP_LIVE) {
            EmpStmt *d = make_drop_stmt(c->arena, b->decl_span, b->name);
            if (d) (void)emp_vec_push(out, d);
        } else if (st == EMP_DROP_MAYBE_MOVED) {
            diagf(c->arena, c->diags, b->decl_span, "drop: cannot insert drop for '%s' because it may be moved on some path", b->name);
        }
    }
}

static void append_drops_since_mark(EmpDropCtx *c, EmpVec *out, size_t mark, EmpSpan at_span) {
    for (size_t i = c->ds.len; i > mark; i--) {
        EmpDropBind *b = &c->ds.items[i - 1];
        if (!b->owned) continue;

        EmpDropState st = (EmpDropState)b->state;
        if (st == EMP_DROP_LIVE) {
            EmpStmt *d = make_drop_stmt(c->arena, at_span, b->name);
            if (d) (void)emp_vec_push(out, d);
        } else if (st == EMP_DROP_MAYBE_MOVED) {
            diagf(c->arena, c->diags, at_span, "drop: value '%s' may be moved at break/continue", b->name);
        }
    }
}

static void append_return_drops(EmpDropCtx *c, EmpVec *out, EmpSpan at_span) {
    // Drop all live owned bindings in reverse order.
    // NOTE: We do not mutate states here, because this is path-specific.
    for (size_t i = c->ds.len; i > 0; i--) {
        EmpDropBind *b = &c->ds.items[i - 1];
        if (!b->owned) continue;
        if ((EmpDropState)b->state == EMP_DROP_LIVE) {
            EmpStmt *d = make_drop_stmt(c->arena, at_span, b->name);
            if (d) (void)emp_vec_push(out, d);
        } else if ((EmpDropState)b->state == EMP_DROP_MAYBE_MOVED) {
            diagf(c->arena, c->diags, at_span, "drop: value '%s' may be moved at return", b->name);
        }
    }
}

static bool rewrite_block_scoped(EmpDropCtx *c, EmpStmt *block, bool push_new_scope) {
    if (!block || block->kind != EMP_STMT_BLOCK) return false;

    if (push_new_scope) {
        (void)ds_push_scope(&c->ds);
    }

    EmpVec new_stmts;
    emp_vec_init(&new_stmts);

    bool terminated = false;

    for (size_t i = 0; i < block->as.block.stmts.len; i++) {
        EmpStmt *s = (EmpStmt *)block->as.block.stmts.items[i];
        if (!s) continue;
        bool term = false;
        EmpStmt *out = rewrite_stmt(c, s, &term);
        (void)emp_vec_push(&new_stmts, out);
        if (term) {
            terminated = true;
            break;
        }
    }

    if (!terminated) {
        append_scope_end_drops(c, &new_stmts);
    }

    // Replace stmt vector
    EmpVec old = block->as.block.stmts;
    block->as.block.stmts = new_stmts;
    emp_vec_free(&old);

    if (push_new_scope) {
        ds_pop_scope(&c->ds);
    }

    return terminated;
}
static EmpStmt *rewrite_assignment_expr_stmt(EmpDropCtx *c, EmpStmt *s) {
    if (!s || s->kind != EMP_STMT_EXPR) return s;

    EmpExpr *e = s->as.expr.expr;
    if (!e || e->kind != EMP_EXPR_BINARY) return s;
    if (e->as.binary.op != EMP_BIN_ASSIGN) return s;

    EmpExpr *lhs = e->as.binary.lhs;
    EmpExpr *rhs = e->as.binary.rhs;
    if (!lhs || lhs->kind != EMP_EXPR_IDENT) return s;

    EmpDropBind *b = ds_lookup(&c->ds, lhs->as.lit);
    bool needs_drop_old = b && b->owned && (EmpDropState)b->state == EMP_DROP_LIVE;

    if (!needs_drop_old) {
        // Evaluate rhs as a move, then mark lhs live.
        visit_expr(c, rhs, EMP_USE_MOVE);
        if (b && b->owned) b->state = EMP_DROP_LIVE;
        return s;
    }

    // Rewrite into:
    // {
    //   auto __emp_tmpN = <rhs>;
    //   drop <lhs>;
    //   <lhs> = __emp_tmpN;
    // }
    EmpSlice tmp_name = make_tmp_name(c);
    EmpExpr *tmp_ident = make_expr(c->arena, EMP_EXPR_IDENT, e->span);
    if (tmp_ident) tmp_ident->as.lit = tmp_name;

    EmpStmt *tmp_var = make_stmt(c->arena, EMP_STMT_VAR, e->span);
    if (!tmp_var) return s;
    tmp_var->as.let_stmt.ty = make_auto_type(c->arena, e->span);
    tmp_var->as.let_stmt.name = tmp_name;
    tmp_var->as.let_stmt.init = rhs;

    EmpStmt *drop_old = make_drop_stmt(c->arena, e->span, lhs->as.lit);

    // Mutate assignment rhs to temp
    e->as.binary.rhs = tmp_ident;

    EmpStmt *wrap = make_stmt(c->arena, EMP_STMT_BLOCK, e->span);
    if (!wrap) return s;
    emp_vec_init(&wrap->as.block.stmts);
    (void)emp_vec_push(&wrap->as.block.stmts, tmp_var);
    if (drop_old) (void)emp_vec_push(&wrap->as.block.stmts, drop_old);
    (void)emp_vec_push(&wrap->as.block.stmts, s);

    return wrap;
}

static EmpStmt *rewrite_stmt(EmpDropCtx *c, EmpStmt *s, bool *out_terminated) {
    if (out_terminated) *out_terminated = false;
    if (!s) return s;

    // Do not insert/modify inside @emp off / @emp mm off.
    if (s->kind == EMP_STMT_EMP_OFF || s->kind == EMP_STMT_EMP_MM_OFF) {
        return s;
    }

    switch (s->kind) {
        case EMP_STMT_TAG:
            return s;
        case EMP_STMT_BLOCK:
            if (out_terminated) *out_terminated = rewrite_block_scoped(c, s, true);
            return s;

        case EMP_STMT_VAR: {
            // Evaluate initializer in the previous environment (supports shadowing semantics).
            visit_expr(c, s->as.let_stmt.init, EMP_USE_MOVE);

            bool owned = true;
            EmpDropState init_state = EMP_DROP_UNINIT;
            if (s->as.let_stmt.init) {
                if (expr_is_borrow(s->as.let_stmt.init)) {
                    owned = false;
                }
                init_state = EMP_DROP_LIVE;
            }

            // Copy-like types do not require drops.
            if (type_is_copy_like(s->as.let_stmt.ty)) {
                owned = false;
            }

            if (s->as.let_stmt.is_destructure && s->as.let_stmt.ty && s->as.let_stmt.ty->kind == EMP_TYPE_TUPLE) {
                size_t n = s->as.let_stmt.destruct_names.len;
                if (s->as.let_stmt.ty->as.tuple.fields.len < n) n = s->as.let_stmt.ty->as.tuple.fields.len;
                for (size_t i = 0; i < n; i++) {
                    const EmpSlice *nm = (const EmpSlice *)s->as.let_stmt.destruct_names.items[i];
                    const EmpTupleField *f = (const EmpTupleField *)s->as.let_stmt.ty->as.tuple.fields.items[i];
                    if (!nm || !f) continue;
                    bool elem_owned = owned;
                    if (type_is_copy_like(f->ty)) elem_owned = false;
                    (void)ds_push_bind(&c->ds, *nm, s->span, elem_owned, init_state);
                }
                return s;
            }

            (void)ds_push_bind(&c->ds, s->as.let_stmt.name, s->span, owned, init_state);
            return s;
        }

        case EMP_STMT_DROP: {
            EmpDropBind *b = ds_lookup(&c->ds, s->as.drop_stmt.name);
            if (!b) return s;
            if (!b->owned) return s;

            EmpDropState st = (EmpDropState)b->state;
            if (st == EMP_DROP_LIVE) {
                b->state = EMP_DROP_UNINIT;
            } else if (st == EMP_DROP_UNINIT) {
                diagf(c->arena, c->diags, s->span, "drop: double drop of '%s'", b->name);
            } else if (st == EMP_DROP_MOVED) {
                diagf(c->arena, c->diags, s->span, "drop: cannot drop moved value '%s'", b->name);
            } else {
                diagf(c->arena, c->diags, s->span, "drop: cannot drop '%s' because it may be moved", b->name);
            }
            return s;
        }

        case EMP_STMT_EXPR: {
            EmpStmt *maybe_wrap = rewrite_assignment_expr_stmt(c, s);
            // If we created a wrapper block, rewrite it immediately so its internal moves update state.
            if (maybe_wrap && maybe_wrap != s && maybe_wrap->kind == EMP_STMT_BLOCK) {
                bool term = false;
                (void)rewrite_stmt(c, maybe_wrap, &term);
            } else {
                visit_expr(c, s->as.expr.expr, EMP_USE_READ);
            }
            return maybe_wrap ? maybe_wrap : s;
        }

        case EMP_STMT_RETURN: {
            EmpStmt *wrap = make_stmt(c->arena, EMP_STMT_BLOCK, s->span);
            if (wrap) {
                emp_vec_init(&wrap->as.block.stmts);
                // Returning a value moves any owned bindings referenced by the return expression.
                // Update states first so we don't insert drops for moved-out values.
                if (s->as.ret.value) {
                    visit_expr(c, s->as.ret.value, EMP_USE_MOVE);
                }
                append_return_drops(c, &wrap->as.block.stmts, s->span);
                (void)emp_vec_push(&wrap->as.block.stmts, s);
            }
            if (out_terminated) *out_terminated = true;
            return wrap ? wrap : s;
        }

        case EMP_STMT_BREAK:
        case EMP_STMT_CONTINUE: {
            if (!loop_active(c)) {
                diagf(c->arena, c->diags, s->span,
                      s->kind == EMP_STMT_BREAK ? "drop: 'break' used outside of a loop" : "drop: 'continue' used outside of a loop",
                      (EmpSlice){"", 0});
                if (out_terminated) *out_terminated = true;
                return s;
            }

            EmpStmt *wrap = make_stmt(c->arena, EMP_STMT_BLOCK, s->span);
            if (wrap) {
                emp_vec_init(&wrap->as.block.stmts);
                append_drops_since_mark(c, &wrap->as.block.stmts, loop_mark(c), s->span);
                (void)emp_vec_push(&wrap->as.block.stmts, s);
            }
            if (out_terminated) *out_terminated = true;
            return wrap ? wrap : s;
        }

        case EMP_STMT_IF: {
            visit_expr(c, s->as.if_stmt.cond, EMP_USE_READ);

            size_t prefix = c->ds.len;
            EmpDropStack then_ds;
            EmpDropStack else_ds;

            if (!ds_clone(&then_ds, &c->ds) || !ds_clone(&else_ds, &c->ds)) {
                ds_free(&then_ds);
                ds_free(&else_ds);
                // best-effort: still visit branches without merging
                bool t1 = false;
                bool t2 = false;
                (void)rewrite_stmt(c, s->as.if_stmt.then_branch, &t1);
                (void)rewrite_stmt(c, s->as.if_stmt.else_branch, &t2);
                if (out_terminated) *out_terminated = t1 && (s->as.if_stmt.else_branch ? t2 : false);
                return s;
            }

            EmpDropCtx then_c = *c;
            EmpDropCtx else_c = *c;
            then_c.ds = then_ds;
            else_c.ds = else_ds;

            bool then_term = false;
            (void)rewrite_stmt(&then_c, s->as.if_stmt.then_branch, &then_term);
            bool else_term = false;
            if (s->as.if_stmt.else_branch) {
                (void)rewrite_stmt(&else_c, s->as.if_stmt.else_branch, &else_term);
            }

            ds_merge_prefix(&c->ds, &then_c.ds, &else_c.ds, prefix);
            ds_free(&then_c.ds);
            ds_free(&else_c.ds);

            if (out_terminated) *out_terminated = then_term && (s->as.if_stmt.else_branch ? else_term : false);
            return s;
        }

        case EMP_STMT_WHILE: {
            visit_expr(c, s->as.while_stmt.cond, EMP_USE_READ);

            size_t prefix = c->ds.len;
            EmpDropStack body_ds;
            if (!ds_clone(&body_ds, &c->ds)) {
                bool t = false;
                (void)rewrite_stmt(c, s->as.while_stmt.body, &t);
                return s;
            }

            EmpDropCtx body_c = *c;
            body_c.ds = body_ds;
            {
                loop_push(&body_c);
                bool t = false;
                (void)rewrite_stmt(&body_c, s->as.while_stmt.body, &t);
                loop_pop(&body_c);
            }
            // loop might execute 0 times => merge entry with body
            ds_merge_prefix(&c->ds, &c->ds, &body_c.ds, prefix);
            ds_free(&body_c.ds);
            return s;
        }

        case EMP_STMT_FOR: {
            // Iterable is read.
            visit_expr(c, s->as.for_stmt.iterable, EMP_USE_READ);

            // Body executes 0+ times.
            size_t prefix = c->ds.len;
            EmpDropStack body_ds;
            if (!ds_clone(&body_ds, &c->ds)) {
                bool t = false;
                (void)rewrite_stmt(c, s->as.for_stmt.body, &t);
                return s;
            }

            EmpDropCtx body_c = *c;
            body_c.ds = body_ds;

            // Track loop-local drops for break/continue.
            loop_push(&body_c);

            // Loop variables live in the body scope. (We model it as entering the block and declaring them.)
            if (s->as.for_stmt.body && s->as.for_stmt.body->kind == EMP_STMT_BLOCK) {
                (void)ds_push_scope(&body_c.ds);
                if (s->as.for_stmt.idx_name.ptr && s->as.for_stmt.idx_name.len && !(s->as.for_stmt.idx_name.len == 1 && s->as.for_stmt.idx_name.ptr[0] == '_')) {
                    (void)ds_push_bind(&body_c.ds, s->as.for_stmt.idx_name, s->span, true, EMP_DROP_LIVE);
                }
                if (s->as.for_stmt.val_name.ptr && s->as.for_stmt.val_name.len && !(s->as.for_stmt.val_name.len == 1 && s->as.for_stmt.val_name.ptr[0] == '_')) {
                    (void)ds_push_bind(&body_c.ds, s->as.for_stmt.val_name, s->span, true, EMP_DROP_LIVE);
                }
                {
                    bool t = false;
                    (void)rewrite_stmt(&body_c, s->as.for_stmt.body, &t);
                }
                ds_pop_scope(&body_c.ds);
            } else {
                bool t = false;
                (void)rewrite_stmt(&body_c, s->as.for_stmt.body, &t);
            }

            loop_pop(&body_c);

            ds_merge_prefix(&c->ds, &c->ds, &body_c.ds, prefix);
            ds_free(&body_c.ds);
            return s;
        }

        case EMP_STMT_MATCH: {
            visit_expr(c, s->as.match_stmt.scrutinee, EMP_USE_READ);

            size_t prefix = c->ds.len;
            EmpDropStack merged_ds;
            memset(&merged_ds, 0, sizeof(merged_ds));
            bool merged_init = false;

            bool all_term = true;
            bool has_default = false;

            for (size_t i = 0; i < s->as.match_stmt.arms.len; i++) {
                const EmpMatchArm *a = (const EmpMatchArm *)s->as.match_stmt.arms.items[i];
                if (!a) continue;
                if (a->is_default) has_default = true;
                if (!a->is_default) visit_expr(c, a->pat, EMP_USE_READ);

                EmpDropStack arm_ds;
                if (!ds_clone(&arm_ds, &c->ds)) {
                    all_term = false;
                    continue;
                }

                EmpDropCtx arm_c = *c;
                arm_c.ds = arm_ds;
                bool term = false;
                (void)rewrite_stmt(&arm_c, (EmpStmt *)a->body, &term);
                if (!term) all_term = false;

                if (!merged_init) {
                    merged_ds = arm_ds;
                    merged_init = true;
                } else {
                    ds_merge_prefix(&merged_ds, &merged_ds, &arm_c.ds, prefix);
                    ds_free(&arm_c.ds);
                }
            }

            if (merged_init) {
                ds_merge_prefix(&c->ds, &merged_ds, &merged_ds, prefix);
                ds_free(&merged_ds);
            }

            bool enum_exhaustive = false;
            if (!has_default && g_drop_program) {
                // If all non-default arms are enum variant patterns for a single enum,
                // treat the match as exhaustive when all variants are covered.
                EmpSlice enum_name = (EmpSlice){0};
                const EmpItem *en = NULL;
                bool all_enum_pats = true;

                bool *covered = NULL;
                size_t covered_len = 0;

                for (size_t i = 0; i < s->as.match_stmt.arms.len; i++) {
                    const EmpMatchArm *a = (const EmpMatchArm *)s->as.match_stmt.arms.items[i];
                    if (!a || a->is_default) continue;

                    EmpSlice pat_enum = (EmpSlice){0};
                    EmpSlice pat_variant = (EmpSlice){0};

                    if (a->pat && a->pat->kind == EMP_EXPR_MEMBER && a->pat->as.member.base && a->pat->as.member.base->kind == EMP_EXPR_IDENT) {
                        pat_enum = a->pat->as.member.base->as.lit;
                        pat_variant = a->pat->as.member.member;
                    } else if (a->pat && a->pat->kind == EMP_EXPR_CALL && a->pat->as.call.callee && a->pat->as.call.callee->kind == EMP_EXPR_MEMBER) {
                        const EmpExpr *mc = a->pat->as.call.callee;
                        if (mc->as.member.base && mc->as.member.base->kind == EMP_EXPR_IDENT) {
                            pat_enum = mc->as.member.base->as.lit;
                            pat_variant = mc->as.member.member;
                        }
                    }

                    if (!pat_enum.ptr || !pat_enum.len || !pat_variant.ptr || !pat_variant.len) {
                        all_enum_pats = false;
                        break;
                    }

                    if (!enum_name.ptr) {
                        enum_name = pat_enum;
                        en = drop_find_enum_decl(g_drop_program, enum_name);
                        if (!en) {
                            all_enum_pats = false;
                            break;
                        }
                        covered_len = en->as.enum_decl.variants.len;
                        covered = (bool *)calloc(covered_len ? covered_len : 1, sizeof(bool));
                        if (!covered) {
                            all_enum_pats = false;
                            break;
                        }
                    } else if (!slice_eq(enum_name, pat_enum)) {
                        all_enum_pats = false;
                        break;
                    }

                    size_t vidx = 0;
                    if (!drop_find_enum_variant(en, pat_variant, &vidx)) {
                        all_enum_pats = false;
                        break;
                    }
                    if (vidx < covered_len) covered[vidx] = true;
                }

                if (all_enum_pats && en && covered) {
                    enum_exhaustive = true;
                    for (size_t vi = 0; vi < covered_len; vi++) {
                        if (!covered[vi]) {
                            enum_exhaustive = false;
                            break;
                        }
                    }
                }
                free(covered);
            }

            if (!has_default && !enum_exhaustive) {
                diagf(c->arena, c->diags, s->span, "drop: non-exhaustive match: missing else arm", (EmpSlice){"", 0});
            }

            if (out_terminated) *out_terminated = (has_default || enum_exhaustive) && all_term;
            return s;
        }

        default:
            return s;
    }
}

void emp_sem_insert_drops(EmpArena *arena, EmpProgram *program, EmpDiags *diags) {
    if (!arena || !program || !diags) return;

    // File-level manual memory management: '@emp mm off;' disables the Rust-like
    // drop insertion pass for the entire module.
    for (size_t i = 0; i < program->items.len; i++) {
        const EmpItem *it = (const EmpItem *)program->items.items[i];
        if (it && it->kind == EMP_ITEM_EMP_MM_OFF) {
            return;
        }
    }

    g_drop_program = program;

    EmpDropCtx c;
    memset(&c, 0, sizeof(c));
    c.arena = arena;
    c.diags = diags;
    ds_init(&c.ds);
    (void)ds_push_scope(&c.ds);

    EmpSlice self_name = {(const char *)"self", 4};

    for (size_t i = 0; i < program->items.len; i++) {
        EmpItem *it = (EmpItem *)program->items.items[i];
        if (!it) continue;

        if (it->kind == EMP_ITEM_FN && it->as.fn.body) {
            while (c.ds.scopes_len) ds_pop_scope(&c.ds);
            (void)ds_push_scope(&c.ds);

            for (size_t j = 0; j < it->as.fn.params.len; j++) {
                EmpParam *p = (EmpParam *)it->as.fn.params.items[j];
                if (!p) continue;
                bool owned = !type_is_copy_like(p->ty);
                (void)ds_push_bind(&c.ds, p->name, p->span, owned, EMP_DROP_LIVE);
            }
            (void)rewrite_block_scoped(&c, it->as.fn.body, false);
            continue;
        }

        if (it->kind == EMP_ITEM_CLASS) {
            for (size_t mi = 0; mi < it->as.class_decl.methods.len; mi++) {
                EmpClassMethod *mth = (EmpClassMethod *)it->as.class_decl.methods.items[mi];
                if (!mth || !mth->body) continue;

                while (c.ds.scopes_len) ds_pop_scope(&c.ds);
                (void)ds_push_scope(&c.ds);

                // `self` is a pointer-like binding; never drop it.
                (void)ds_push_bind(&c.ds, self_name, mth->span, false, EMP_DROP_LIVE);

                for (size_t j = 0; j < mth->params.len; j++) {
                    EmpParam *p = (EmpParam *)mth->params.items[j];
                    if (!p) continue;
                    bool owned = !type_is_copy_like(p->ty);
                    (void)ds_push_bind(&c.ds, p->name, p->span, owned, EMP_DROP_LIVE);
                }
                (void)rewrite_block_scoped(&c, mth->body, false);
            }
            continue;
        }

        if (it->kind == EMP_ITEM_IMPL) {
            for (size_t mi = 0; mi < it->as.impl_decl.methods.len; mi++) {
                EmpImplMethod *mth = (EmpImplMethod *)it->as.impl_decl.methods.items[mi];
                if (!mth || !mth->body) continue;

                while (c.ds.scopes_len) ds_pop_scope(&c.ds);

    g_drop_program = NULL;
                (void)ds_push_scope(&c.ds);

                (void)ds_push_bind(&c.ds, self_name, mth->span, false, EMP_DROP_LIVE);

                for (size_t j = 0; j < mth->params.len; j++) {
                    EmpParam *p = (EmpParam *)mth->params.items[j];
                    if (!p) continue;
                    bool owned = !type_is_copy_like(p->ty);
                    (void)ds_push_bind(&c.ds, p->name, p->span, owned, EMP_DROP_LIVE);
                }
                (void)rewrite_block_scoped(&c, mth->body, false);
            }
            continue;
        }
    }

    ds_free(&c.ds);
}
