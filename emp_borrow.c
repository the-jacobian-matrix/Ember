#include "emp_borrow.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct EmpBorrowBind {
    EmpSlice name;
    int shared_count;
    bool mut_active;
    int ref_origin_unsafe_depth; // >0 if this binding currently holds a borrow value created in @emp off
} EmpBorrowBind;

typedef struct EmpBorrowDelta {
    EmpBorrowBind *bind;
    int shared_delta;
    bool mut_delta; // true means "added a mutable borrow" in this scope
} EmpBorrowDelta;

typedef struct EmpBorrowScope {
    size_t binds_mark;
    size_t deltas_mark;
} EmpBorrowScope;

typedef struct EmpBorrowCtx {
    // stack of bindings (supports shadowing)
    EmpBorrowBind **binds;
    size_t binds_len;
    size_t binds_cap;

    // deltas to unwind at scope exit
    EmpBorrowDelta *deltas;
    size_t deltas_len;
    size_t deltas_cap;

    // scope stack
    EmpBorrowScope *scopes;
    size_t scopes_len;
    size_t scopes_cap;
} EmpBorrowCtx;

static bool slice_eq(EmpSlice a, EmpSlice b) {
    if (a.len != b.len) return false;
    if (a.ptr == b.ptr) return true;
    if (!a.ptr || !b.ptr) return false;
    return memcmp(a.ptr, b.ptr, a.len) == 0;
}

static char *arena_strdup(EmpArena *a, const char *s) {
    size_t n = strlen(s);
    char *p = (char *)emp_arena_alloc(a, n + 1, 1);
    if (!p) return NULL;
    memcpy(p, s, n + 1);
    return p;
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

static void ctx_init(EmpBorrowCtx *c) {
    memset(c, 0, sizeof(*c));
}

static void ctx_free(EmpBorrowCtx *c) {
    if (c->binds) {
        for (size_t i = 0; i < c->binds_len; i++) free(c->binds[i]);
    }
    free(c->binds);
    free(c->deltas);
    free(c->scopes);
    memset(c, 0, sizeof(*c));
}

static bool ensure_ptr_cap(void ***items, size_t *cap, size_t need) {
    if (*cap >= need) return true;
    size_t new_cap = *cap ? *cap * 2 : 32;
    while (new_cap < need) new_cap *= 2;
    void **p = (void **)realloc(*items, new_cap * sizeof(void *));
    if (!p) return false;
    *items = p;
    *cap = new_cap;
    return true;
}

static bool ensure_delta_cap(EmpBorrowCtx *c, size_t need) {
    if (c->deltas_cap >= need) return true;
    size_t new_cap = c->deltas_cap ? c->deltas_cap * 2 : 64;
    while (new_cap < need) new_cap *= 2;
    EmpBorrowDelta *p = (EmpBorrowDelta *)realloc(c->deltas, new_cap * sizeof(EmpBorrowDelta));
    if (!p) return false;
    c->deltas = p;
    c->deltas_cap = new_cap;
    return true;
}

static bool ensure_scope_cap(EmpBorrowCtx *c, size_t need) {
    if (c->scopes_cap >= need) return true;
    size_t new_cap = c->scopes_cap ? c->scopes_cap * 2 : 32;
    while (new_cap < need) new_cap *= 2;
    EmpBorrowScope *p = (EmpBorrowScope *)realloc(c->scopes, new_cap * sizeof(EmpBorrowScope));
    if (!p) return false;
    c->scopes = p;
    c->scopes_cap = new_cap;
    return true;
}

static bool push_scope(EmpBorrowCtx *c) {
    if (!ensure_scope_cap(c, c->scopes_len + 1)) return false;
    EmpBorrowScope s;
    s.binds_mark = c->binds_len;
    s.deltas_mark = c->deltas_len;
    c->scopes[c->scopes_len++] = s;
    return true;
}

static void pop_scope(EmpBorrowCtx *c) {
    if (c->scopes_len == 0) return;
    EmpBorrowScope s = c->scopes[--c->scopes_len];

    // unwind deltas
    while (c->deltas_len > s.deltas_mark) {
        EmpBorrowDelta d = c->deltas[--c->deltas_len];
        if (d.bind) {
            d.bind->shared_count -= d.shared_delta;
            if (d.mut_delta) d.bind->mut_active = false;
        }
    }

    // pop bindings introduced in this scope
    while (c->binds_len > s.binds_mark) {
        free(c->binds[--c->binds_len]);
    }
}

static EmpBorrowBind *lookup_bind(EmpBorrowCtx *c, EmpSlice name) {
    for (size_t i = c->binds_len; i > 0; i--) {
        EmpBorrowBind *b = c->binds[i - 1];
        if (b && slice_eq(b->name, name)) return b;
    }
    return NULL;
}

static bool declare_bind(EmpBorrowCtx *c, EmpSlice name) {
    if (!ensure_ptr_cap((void ***)&c->binds, &c->binds_cap, c->binds_len + 1)) return false;
    EmpBorrowBind *b = (EmpBorrowBind *)calloc(1, sizeof(EmpBorrowBind));
    if (!b) return false;
    b->name = name;
    b->shared_count = 0;
    b->mut_active = false;
    b->ref_origin_unsafe_depth = 0;
    c->binds[c->binds_len++] = b;
    return true;
}

static bool record_delta(EmpBorrowCtx *c, EmpBorrowBind *b, int shared_delta, bool mut_delta) {
    if (!ensure_delta_cap(c, c->deltas_len + 1)) return false;
    EmpBorrowDelta d;
    d.bind = b;
    d.shared_delta = shared_delta;
    d.mut_delta = mut_delta;
    c->deltas[c->deltas_len++] = d;
    return true;
}

typedef enum EmpBorrowUseKind {
    EMP_BOR_USE_READ,
    EMP_BOR_USE_MOVE,
} EmpBorrowUseKind;

static void visit_expr(EmpArena *arena, EmpDiags *diags, EmpBorrowCtx *c, const EmpExpr *e, EmpBorrowUseKind use, int unsafe_depth);
static void visit_stmt(EmpArena *arena, EmpDiags *diags, EmpBorrowCtx *c, const EmpStmt *s, int unsafe_depth);

static bool program_has_emp_mm_off(const EmpProgram *program) {
    if (!program) return false;
    for (size_t i = 0; i < program->items.len; i++) {
        const EmpItem *it = (const EmpItem *)program->items.items[i];
        if (it && it->kind == EMP_ITEM_EMP_MM_OFF) return true;
    }
    return false;
}

static bool expr_is_borrow_value(const EmpExpr *e) {
    return e && e->kind == EMP_EXPR_UNARY && (e->as.unary.op == EMP_UN_BORROW || e->as.unary.op == EMP_UN_BORROW_MUT);
}

static int expr_ref_origin(EmpBorrowCtx *c, const EmpExpr *e, int unsafe_depth) {
    if (!e) return 0;

    if (expr_is_borrow_value(e)) {
        return unsafe_depth > 0 ? unsafe_depth : 0;
    }

    switch (e->kind) {
        case EMP_EXPR_FSTRING: {
            int origin = 0;
            for (size_t i = 0; i < e->as.fstring.parts.len; i++) {
                const EmpFStringPart *pt = (const EmpFStringPart *)e->as.fstring.parts.items[i];
                if (!pt || !pt->is_expr) continue;
                int o = expr_ref_origin(c, pt->expr, unsafe_depth);
                if (o > origin) origin = o;
            }
            return origin;
        }

        case EMP_EXPR_GROUP:
            return expr_ref_origin(c, e->as.group.inner, unsafe_depth);

        case EMP_EXPR_CAST:
            return expr_ref_origin(c, e->as.cast.expr, unsafe_depth);

        case EMP_EXPR_TUPLE: {
            int origin = 0;
            for (size_t i = 0; i < e->as.tuple.items.len; i++) {
                const EmpExpr *it = (const EmpExpr *)e->as.tuple.items.items[i];
                int o = expr_ref_origin(c, it, unsafe_depth);
                if (o > origin) origin = o;
            }
            return origin;
        }

        case EMP_EXPR_BINARY: {
            int lo = expr_ref_origin(c, e->as.binary.lhs, unsafe_depth);
            if (lo > 0) return lo;
            return expr_ref_origin(c, e->as.binary.rhs, unsafe_depth);
        }

        case EMP_EXPR_RANGE: {
            int lo = expr_ref_origin(c, e->as.range.start, unsafe_depth);
            if (lo > 0) return lo;
            return expr_ref_origin(c, e->as.range.end, unsafe_depth);
        }

        case EMP_EXPR_UNARY:
            // Propagate through wrappers like -(&x) conservatively.
            return expr_ref_origin(c, e->as.unary.rhs, unsafe_depth);

        case EMP_EXPR_INDEX:
            return expr_ref_origin(c, e->as.index.base, unsafe_depth);

        case EMP_EXPR_MEMBER:
            return expr_ref_origin(c, e->as.member.base, unsafe_depth);

        case EMP_EXPR_IDENT: {
            EmpBorrowBind *b = lookup_bind(c, e->as.lit);
            return b ? b->ref_origin_unsafe_depth : 0;
        }
        default:
            return 0;
    }
}

static EmpSlice root_binding_name(const EmpExpr *e) {
    if (!e) return (EmpSlice){0};

    switch (e->kind) {
        case EMP_EXPR_IDENT:
            return e->as.lit;
        case EMP_EXPR_GROUP:
            return root_binding_name(e->as.group.inner);
        case EMP_EXPR_INDEX:
            return root_binding_name(e->as.index.base);
        case EMP_EXPR_MEMBER:
            return root_binding_name(e->as.member.base);
        default:
            return (EmpSlice){0};
    }
}

static void try_add_shared_borrow(EmpArena *arena, EmpDiags *diags, EmpBorrowCtx *c, EmpSpan span, EmpSlice name) {
    EmpBorrowBind *b = lookup_bind(c, name);
    if (!b) return;

    if (b->mut_active) {
        diagf(arena, diags, span, "borrow: cannot take shared borrow of '%s' while a mutable borrow is active", name);
        return;
    }

    b->shared_count += 1;
    (void)record_delta(c, b, 1, false);
}

static void try_add_mut_borrow(EmpArena *arena, EmpDiags *diags, EmpBorrowCtx *c, EmpSpan span, EmpSlice name) {
    EmpBorrowBind *b = lookup_bind(c, name);
    if (!b) return;

    if (b->mut_active) {
        diagf(arena, diags, span, "borrow: cannot take mutable borrow of '%s' while another mutable borrow is active", name);
        return;
    }
    if (b->shared_count > 0) {
        diagf(arena, diags, span, "borrow: cannot take mutable borrow of '%s' while shared borrows are active", name);
        return;
    }

    b->mut_active = true;
    (void)record_delta(c, b, 0, true);
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

static void try_move_ident(EmpArena *arena, EmpDiags *diags, EmpBorrowCtx *c, EmpSpan span, EmpSlice name) {
    EmpBorrowBind *b = lookup_bind(c, name);
    if (!b) return;

    if (b->mut_active || b->shared_count > 0) {
        diagf(arena, diags, span, "borrow: cannot move '%s' while it is borrowed", name);
    }
}

static void try_assign_ident(EmpArena *arena, EmpDiags *diags, EmpBorrowCtx *c, EmpSpan span, EmpSlice name) {
    EmpBorrowBind *b = lookup_bind(c, name);
    if (!b) return;

    if (b->mut_active || b->shared_count > 0) {
        diagf(arena, diags, span, "borrow: cannot assign to '%s' while it is borrowed", name);
    }
}

static void visit_expr(EmpArena *arena, EmpDiags *diags, EmpBorrowCtx *c, const EmpExpr *e, EmpBorrowUseKind use, int unsafe_depth) {
    if (!e) return;

    switch (e->kind) {
        case EMP_EXPR_FSTRING:
            for (size_t i = 0; i < e->as.fstring.parts.len; i++) {
                const EmpFStringPart *pt = (const EmpFStringPart *)e->as.fstring.parts.items[i];
                if (!pt || !pt->is_expr) continue;
                visit_expr(arena, diags, c, pt->expr, EMP_BOR_USE_READ, unsafe_depth);
            }
            return;

        case EMP_EXPR_IDENT:
            if (unsafe_depth == 0 && use == EMP_BOR_USE_MOVE) {
                try_move_ident(arena, diags, c, e->span, e->as.lit);
            }
            return;

        case EMP_EXPR_CAST:
            visit_expr(arena, diags, c, e->as.cast.expr, use, unsafe_depth);
            return;

        case EMP_EXPR_UNARY:
            if (e->as.unary.op == EMP_UN_BORROW || e->as.unary.op == EMP_UN_BORROW_MUT) {
                const EmpExpr *rhs = e->as.unary.rhs;
                if (unsafe_depth == 0) {
                    // MVP: allow borrowing identifier-rooted lvalues such as:
                    // - `ident`
                    // - `ident[expr]`
                    // - `ident.field` (and longer member/index chains)
                    // Borrowing a subobject is treated as borrowing the whole root binding.
                    EmpSlice base_name = root_binding_name(rhs);
                    if (base_name.ptr && base_name.len) {
                        if (e->as.unary.op == EMP_UN_BORROW) {
                            try_add_shared_borrow(arena, diags, c, e->span, base_name);
                        } else {
                            try_add_mut_borrow(arena, diags, c, e->span, base_name);
                        }
                    } else {
                        // Keep early pass simple.
                        diagf(arena, diags, e->span, "borrow: can only borrow identifier-rooted lvalues (like `x`, `x[i]`, `x.f`) in this phase", (EmpSlice){"",0});
                    }
                }
                visit_expr(arena, diags, c, rhs, EMP_BOR_USE_READ, unsafe_depth);
                return;
            }
            visit_expr(arena, diags, c, e->as.unary.rhs, use, unsafe_depth);
            return;

        case EMP_EXPR_BINARY: {
            EmpBinOp op = e->as.binary.op;
            if (is_assign_like(op)) {
                EmpBorrowUseKind rhs_use = (op == EMP_BIN_ASSIGN) ? EMP_BOR_USE_MOVE : EMP_BOR_USE_READ;
                visit_expr(arena, diags, c, e->as.binary.rhs, rhs_use, unsafe_depth);

                EmpSlice base_name = root_binding_name(e->as.binary.lhs);
                if (unsafe_depth == 0 && base_name.ptr && base_name.len) {
                    // Writing through a member/index chain is treated as assigning through the root binding.
                    // This is conservative but keeps sub-borrows sound w.r.t. mutations.
                    try_assign_ident(arena, diags, c, e->span, base_name);
                }

                // Evaluate any side-effectful index expressions on the LHS.
                visit_expr(arena, diags, c, e->as.binary.lhs, EMP_BOR_USE_READ, unsafe_depth);
                return;
            }

            visit_expr(arena, diags, c, e->as.binary.lhs, EMP_BOR_USE_READ, unsafe_depth);
            visit_expr(arena, diags, c, e->as.binary.rhs, EMP_BOR_USE_READ, unsafe_depth);
            return;
        }

        case EMP_EXPR_CALL:
            visit_expr(arena, diags, c, e->as.call.callee, EMP_BOR_USE_READ, unsafe_depth);
            // Treat borrows created for call arguments as temporaries that end after the call.
            // This keeps patterns like `f(&mut x); f(&mut x);` usable without requiring an
            // entire block scope to end.
            (void)push_scope(c);

            // Conservative method receiver rule: treat `obj.method(...)` as taking a temporary
            // mutable borrow of the root receiver binding.
            if (unsafe_depth == 0 && e->as.call.callee && e->as.call.callee->kind == EMP_EXPR_MEMBER) {
                EmpSlice recv = root_binding_name(e->as.call.callee->as.member.base);
                if (recv.ptr && recv.len) {
                    try_add_mut_borrow(arena, diags, c, e->span, recv);
                }
            }

            for (size_t i = 0; i < e->as.call.args.len; i++) {
                visit_expr(arena, diags, c, (const EmpExpr *)e->as.call.args.items[i], EMP_BOR_USE_MOVE, unsafe_depth);
            }
            pop_scope(c);
            return;

        case EMP_EXPR_GROUP:
            visit_expr(arena, diags, c, e->as.group.inner, use, unsafe_depth);
            return;

        case EMP_EXPR_TUPLE:
            for (size_t i = 0; i < e->as.tuple.items.len; i++) {
                visit_expr(arena, diags, c, (const EmpExpr *)e->as.tuple.items.items[i], use, unsafe_depth);
            }
            return;

        case EMP_EXPR_INDEX:
            // Indexing reads base/index in this phase.
            visit_expr(arena, diags, c, e->as.index.base, EMP_BOR_USE_READ, unsafe_depth);
            visit_expr(arena, diags, c, e->as.index.index, EMP_BOR_USE_READ, unsafe_depth);
            return;

        case EMP_EXPR_MEMBER:
            visit_expr(arena, diags, c, e->as.member.base, EMP_BOR_USE_READ, unsafe_depth);
            return;

        case EMP_EXPR_NEW:
            // Same as calls: borrows used to construct a value are temporaries.
            (void)push_scope(c);
            for (size_t i = 0; i < e->as.new_expr.args.len; i++) {
                visit_expr(arena, diags, c, (const EmpExpr *)e->as.new_expr.args.items[i], EMP_BOR_USE_MOVE, unsafe_depth);
            }
            pop_scope(c);
            return;

        case EMP_EXPR_RANGE:
            visit_expr(arena, diags, c, e->as.range.start, EMP_BOR_USE_READ, unsafe_depth);
            visit_expr(arena, diags, c, e->as.range.end, EMP_BOR_USE_READ, unsafe_depth);
            return;

        default:
            return;
    }
}

static void visit_block(EmpArena *arena, EmpDiags *diags, EmpBorrowCtx *c, const EmpStmt *block) {
    if (!block || block->kind != EMP_STMT_BLOCK) return;

    (void)push_scope(c);
    for (size_t i = 0; i < block->as.block.stmts.len; i++) {
        const EmpStmt *s = (const EmpStmt *)block->as.block.stmts.items[i];
        visit_stmt(arena, diags, c, s, 0);
    }
    pop_scope(c);
}

static void visit_block_with_unsafe(EmpArena *arena, EmpDiags *diags, EmpBorrowCtx *c, const EmpStmt *block, int unsafe_depth) {
    if (!block || block->kind != EMP_STMT_BLOCK) return;

    (void)push_scope(c);
    for (size_t i = 0; i < block->as.block.stmts.len; i++) {
        const EmpStmt *s = (const EmpStmt *)block->as.block.stmts.items[i];
        visit_stmt(arena, diags, c, s, unsafe_depth);
    }
    pop_scope(c);
}

static void visit_stmt(EmpArena *arena, EmpDiags *diags, EmpBorrowCtx *c, const EmpStmt *s, int unsafe_depth) {
    if (!s) return;

    switch (s->kind) {
        case EMP_STMT_BLOCK:
            visit_block_with_unsafe(arena, diags, c, s, unsafe_depth);
            return;

        case EMP_STMT_VAR:
            if (s->as.let_stmt.is_destructure) {
                for (size_t i = 0; i < s->as.let_stmt.destruct_names.len; i++) {
                    const EmpSlice *nm = (const EmpSlice *)s->as.let_stmt.destruct_names.items[i];
                    if (!nm) continue;
                    (void)declare_bind(c, *nm);
                    EmpBorrowBind *b = lookup_bind(c, *nm);
                    if (b) b->ref_origin_unsafe_depth = expr_ref_origin(c, s->as.let_stmt.init, unsafe_depth);
                }
                visit_expr(arena, diags, c, s->as.let_stmt.init, EMP_BOR_USE_MOVE, unsafe_depth);
                return;
            }

            (void)declare_bind(c, s->as.let_stmt.name);
            {
                EmpBorrowBind *b = lookup_bind(c, s->as.let_stmt.name);
                if (b) b->ref_origin_unsafe_depth = expr_ref_origin(c, s->as.let_stmt.init, unsafe_depth);
            }
            visit_expr(arena, diags, c, s->as.let_stmt.init, EMP_BOR_USE_MOVE, unsafe_depth);
            return;

        case EMP_STMT_EXPR:
            // Track assignments that may store a borrow value.
            if (s->as.expr.expr && s->as.expr.expr->kind == EMP_EXPR_BINARY && is_assign_like(s->as.expr.expr->as.binary.op)) {
                const EmpExpr *lhs = s->as.expr.expr->as.binary.lhs;
                const EmpExpr *rhs = s->as.expr.expr->as.binary.rhs;
                if (lhs && lhs->kind == EMP_EXPR_IDENT) {
                    EmpBorrowBind *b = lookup_bind(c, lhs->as.lit);
                    if (b) b->ref_origin_unsafe_depth = expr_ref_origin(c, rhs, unsafe_depth);
                }
            }
            visit_expr(arena, diags, c, s->as.expr.expr, EMP_BOR_USE_READ, unsafe_depth);
            return;

        case EMP_STMT_RETURN:
            if (unsafe_depth > 0) {
                int origin = expr_ref_origin(c, s->as.ret.value, unsafe_depth);
                if (origin > 0) {
                    diagf(arena, diags, s->span, "emp off: cannot return borrowed reference from @emp off", (EmpSlice){"",0});
                }
            }
            visit_expr(arena, diags, c, s->as.ret.value, EMP_BOR_USE_MOVE, unsafe_depth);
            return;

        case EMP_STMT_IF:
            visit_expr(arena, diags, c, s->as.if_stmt.cond, EMP_BOR_USE_READ, unsafe_depth);
            visit_stmt(arena, diags, c, s->as.if_stmt.then_branch, unsafe_depth);
            visit_stmt(arena, diags, c, s->as.if_stmt.else_branch, unsafe_depth);
            return;

        case EMP_STMT_WHILE:
            visit_expr(arena, diags, c, s->as.while_stmt.cond, EMP_BOR_USE_READ, unsafe_depth);
            visit_stmt(arena, diags, c, s->as.while_stmt.body, unsafe_depth);
            return;

        case EMP_STMT_FOR: {
            visit_expr(arena, diags, c, s->as.for_stmt.iterable, EMP_BOR_USE_READ, unsafe_depth);
            // loop introduces bindings for its names inside the body scope
            if (s->as.for_stmt.body && s->as.for_stmt.body->kind == EMP_STMT_BLOCK) {
                (void)push_scope(c);
                if (s->as.for_stmt.idx_name.ptr && s->as.for_stmt.idx_name.len && !(s->as.for_stmt.idx_name.len == 1 && s->as.for_stmt.idx_name.ptr[0] == '_')) {
                    (void)declare_bind(c, s->as.for_stmt.idx_name);
                }
                if (s->as.for_stmt.val_name.ptr && s->as.for_stmt.val_name.len && !(s->as.for_stmt.val_name.len == 1 && s->as.for_stmt.val_name.ptr[0] == '_')) {
                    (void)declare_bind(c, s->as.for_stmt.val_name);
                }
                for (size_t i = 0; i < s->as.for_stmt.body->as.block.stmts.len; i++) {
                    visit_stmt(arena, diags, c, (const EmpStmt *)s->as.for_stmt.body->as.block.stmts.items[i], unsafe_depth);
                }
                pop_scope(c);
            } else {
                visit_stmt(arena, diags, c, s->as.for_stmt.body, unsafe_depth);
            }
            return;
        }

        case EMP_STMT_BREAK:
        case EMP_STMT_CONTINUE:
            return;

        case EMP_STMT_MATCH:
            visit_expr(arena, diags, c, s->as.match_stmt.scrutinee, EMP_BOR_USE_READ, unsafe_depth);
            for (size_t i = 0; i < s->as.match_stmt.arms.len; i++) {
                const EmpMatchArm *a = (const EmpMatchArm *)s->as.match_stmt.arms.items[i];
                if (!a) continue;
                if (!a->is_default) {
                    visit_expr(arena, diags, c, a->pat, EMP_BOR_USE_READ, unsafe_depth);
                }
                visit_stmt(arena, diags, c, a->body, unsafe_depth);
            }
            return;

        case EMP_STMT_EMP_OFF:
            // Borrow rules are disabled inside @emp off, but we enforce that borrowed references
            // do not escape back into safe code.
            if (unsafe_depth > 0) {
                visit_stmt(arena, diags, c, s->as.emp_off.body, unsafe_depth + 1);
                return;
            }
            {
                size_t entry_len = c->binds_len;
                visit_stmt(arena, diags, c, s->as.emp_off.body, unsafe_depth + 1);

                // If any outer binding now holds a borrow created in unsafe, reject.
                for (size_t i = 0; i < entry_len && i < c->binds_len; i++) {
                    EmpBorrowBind *b = c->binds[i];
                    if (!b) continue;
                    if (b->ref_origin_unsafe_depth > 0) {
                        diagf(arena, diags, s->span, "emp off: borrowed reference escapes unsafe boundary via '%s'", b->name);
                        // minimize cascaded errors
                        b->ref_origin_unsafe_depth = 0;
                    }
                }
            }
            return;

        case EMP_STMT_EMP_MM_OFF:
            // Manual-MM mode disables borrow rules inside, but we still enforce that borrows
            // created inside do not escape back into safe code.
            if (unsafe_depth > 0) {
                visit_stmt(arena, diags, c, s->as.emp_mm_off.body, unsafe_depth + 1);
                return;
            }
            {
                size_t entry_len = c->binds_len;
                visit_stmt(arena, diags, c, s->as.emp_mm_off.body, unsafe_depth + 1);

                for (size_t i = 0; i < entry_len && i < c->binds_len; i++) {
                    EmpBorrowBind *b = c->binds[i];
                    if (!b) continue;
                    if (b->ref_origin_unsafe_depth > 0) {
                        diagf(arena, diags, s->span, "emp mm off: borrowed reference escapes unsafe boundary via '%s'", b->name);
                        b->ref_origin_unsafe_depth = 0;
                    }
                }
            }
            return;

        default:
            return;
    }
}

void emp_sem_check_borrows_lexical(EmpArena *arena, const EmpProgram *program, EmpDiags *diags) {
    if (!arena || !program || !diags) return;

    EmpBorrowCtx c;
    ctx_init(&c);
    (void)push_scope(&c);

    const bool file_mm_off = program_has_emp_mm_off(program);

    for (size_t i = 0; i < program->items.len; i++) {
        const EmpItem *it = (const EmpItem *)program->items.items[i];
        if (!it) continue;

        if (it->kind == EMP_ITEM_FN && it->as.fn.body) {
            // reset function scope
            while (c.scopes_len) pop_scope(&c);
            (void)push_scope(&c);

            for (size_t j = 0; j < it->as.fn.params.len; j++) {
                const EmpParam *p = (const EmpParam *)it->as.fn.params.items[j];
                if (!p) continue;
                (void)declare_bind(&c, p->name);
            }
            visit_stmt(arena, diags, &c, it->as.fn.body, file_mm_off ? 1 : 0);
            continue;
        }

        if (it->kind == EMP_ITEM_CLASS) {
            EmpSlice self_name = {(const char *)"self", 4};
            for (size_t mi = 0; mi < it->as.class_decl.methods.len; mi++) {
                const EmpClassMethod *mth = (const EmpClassMethod *)it->as.class_decl.methods.items[mi];
                if (!mth || !mth->body) continue;

                while (c.scopes_len) pop_scope(&c);
                (void)push_scope(&c);

                (void)declare_bind(&c, self_name);
                for (size_t j = 0; j < mth->params.len; j++) {
                    const EmpParam *p = (const EmpParam *)mth->params.items[j];
                    if (!p) continue;
                    (void)declare_bind(&c, p->name);
                }
                visit_stmt(arena, diags, &c, mth->body, file_mm_off ? 1 : 0);
            }
            continue;
        }

        if (it->kind == EMP_ITEM_IMPL) {
            EmpSlice self_name = {(const char *)"self", 4};
            for (size_t mi = 0; mi < it->as.impl_decl.methods.len; mi++) {
                const EmpImplMethod *mth = (const EmpImplMethod *)it->as.impl_decl.methods.items[mi];
                if (!mth || !mth->body) continue;

                while (c.scopes_len) pop_scope(&c);
                (void)push_scope(&c);

                (void)declare_bind(&c, self_name);
                for (size_t j = 0; j < mth->params.len; j++) {
                    const EmpParam *p = (const EmpParam *)mth->params.items[j];
                    if (!p) continue;
                    (void)declare_bind(&c, p->name);
                }
                visit_stmt(arena, diags, &c, mth->body, file_mm_off ? 1 : 0);
            }
            continue;
        }
    }

    ctx_free(&c);
}
