#include "emp_json.h"

#include <stdbool.h>
#include <string.h>

typedef struct EmpJsonW {
    FILE *out;
    int indent;
} EmpJsonW;

static void jw_indent(EmpJsonW *w) {
    for (int i = 0; i < w->indent; i++) fputs("  ", w->out);
}

static void jw_str(EmpJsonW *w, const char *s, size_t n) {
    fputc('"', w->out);
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        switch (c) {
            case '"': fputs("\\\"", w->out); break;
            case '\\': fputs("\\\\", w->out); break;
            case '\b': fputs("\\b", w->out); break;
            case '\f': fputs("\\f", w->out); break;
            case '\n': fputs("\\n", w->out); break;
            case '\r': fputs("\\r", w->out); break;
            case '\t': fputs("\\t", w->out); break;
            default:
                if (c < 0x20) {
                    fprintf(w->out, "\\u%04X", (unsigned)c);
                } else {
                    fputc((int)c, w->out);
                }
        }
    }
    fputc('"', w->out);
}

static void jw_slice(EmpJsonW *w, EmpSlice s) { jw_str(w, s.ptr, s.len); }

static void jw_span(EmpJsonW *w, EmpSpan s) {
    fputs("{", w->out);
    fputs("\"start\":", w->out);
    fprintf(w->out, "%zu", s.start);
    fputs(",\"end\":", w->out);
    fprintf(w->out, "%zu", s.end);
    fputs(",\"line\":", w->out);
    fprintf(w->out, "%u", (unsigned)s.line);
    fputs(",\"col\":", w->out);
    fprintf(w->out, "%u", (unsigned)s.col);
    fputs("}", w->out);
}

static void jw_nl(EmpJsonW *w) { fputc('\n', w->out); }

static void emit_type(EmpJsonW *w, const EmpType *t);
static void emit_expr(EmpJsonW *w, const EmpExpr *e);
static void emit_stmt(EmpJsonW *w, const EmpStmt *s);

static void emit_type(EmpJsonW *w, const EmpType *t) {
    if (!t) {
        fputs("null", w->out);
        return;
    }

    fputs("{\"kind\":", w->out);
    jw_str(w, emp_type_kind_name(t->kind), strlen(emp_type_kind_name(t->kind)));
    fputs(",\"span\":", w->out);
    jw_span(w, t->span);

    if (t->kind == EMP_TYPE_NAME) {
        fputs(",\"name\":", w->out);
        jw_slice(w, t->as.name);
    } else if (t->kind == EMP_TYPE_DYN) {
        fputs(",\"baseName\":", w->out);
        jw_slice(w, t->as.dyn.base_name);
    } else if (t->kind == EMP_TYPE_PTR) {
        fputs(",\"pointee\":", w->out);
        emit_type(w, t->as.ptr.pointee);
    } else if (t->kind == EMP_TYPE_ARRAY || t->kind == EMP_TYPE_LIST) {
        fputs(",\"elem\":", w->out);
        emit_type(w, t->as.array.elem);
        if (t->kind == EMP_TYPE_ARRAY) {
            fputs(",\"sizeText\":", w->out);
            if (t->as.array.size_text.ptr && t->as.array.size_text.len) {
                jw_slice(w, t->as.array.size_text);
            } else {
                fputs("null", w->out);
            }
        }
    } else if (t->kind == EMP_TYPE_TUPLE) {
        fputs(",\"fields\":[", w->out);
        for (size_t i = 0; i < t->as.tuple.fields.len; i++) {
            if (i) fputc(',', w->out);
            const EmpTupleField *f = (const EmpTupleField *)t->as.tuple.fields.items[i];
            if (!f) {
                fputs("null", w->out);
                continue;
            }
            fputs("{\"type\":", w->out);
            emit_type(w, f->ty);
            fputs(",\"name\":", w->out);
            if (f->name.ptr && f->name.len) jw_slice(w, f->name); else fputs("null", w->out);
            fputs(",\"span\":", w->out);
            jw_span(w, f->span);
            fputs("}", w->out);
        }
        fputc(']', w->out);
    }

    fputs("}", w->out);
}

static void emit_expr(EmpJsonW *w, const EmpExpr *e) {
    if (!e) {
        fputs("null", w->out);
        return;
    }

    fputs("{", w->out);
    fputs("\"kind\":", w->out);

    switch (e->kind) {
        case EMP_EXPR_INT: jw_str(w, "Int", 3); break;
        case EMP_EXPR_FLOAT: jw_str(w, "Float", 5); break;
        case EMP_EXPR_STRING: jw_str(w, "String", 6); break;
        case EMP_EXPR_FSTRING: jw_str(w, "FString", 7); break;
        case EMP_EXPR_CHAR: jw_str(w, "Char", 4); break;
        case EMP_EXPR_IDENT: jw_str(w, "Ident", 5); break;
        case EMP_EXPR_UNARY: jw_str(w, "Unary", 5); break;
        case EMP_EXPR_BINARY: jw_str(w, "Binary", 6); break;
        case EMP_EXPR_CALL: jw_str(w, "Call", 4); break;
        case EMP_EXPR_GROUP: jw_str(w, "Group", 5); break;
        case EMP_EXPR_CAST: jw_str(w, "Cast", 4); break;
        case EMP_EXPR_TUPLE: jw_str(w, "Tuple", 5); break;
        case EMP_EXPR_INDEX: jw_str(w, "Index", 5); break;
        case EMP_EXPR_MEMBER: jw_str(w, "Member", 6); break;
        case EMP_EXPR_NEW: jw_str(w, "New", 3); break;
        case EMP_EXPR_RANGE: jw_str(w, "Range", 5); break;
        default: jw_str(w, "Unknown", 7); break;
    }

    fputs(",\"span\":", w->out);
    jw_span(w, e->span);

    if (e->kind == EMP_EXPR_INT || e->kind == EMP_EXPR_FLOAT || e->kind == EMP_EXPR_STRING || e->kind == EMP_EXPR_CHAR || e->kind == EMP_EXPR_IDENT) {
        fputs(",\"text\":", w->out);
        jw_slice(w, e->as.lit);
    } else if (e->kind == EMP_EXPR_FSTRING) {
        fputs(",\"parts\":[", w->out);
        for (size_t i = 0; i < e->as.fstring.parts.len; i++) {
            if (i) fputc(',', w->out);
            const EmpFStringPart *pt = (const EmpFStringPart *)e->as.fstring.parts.items[i];
            if (!pt) {
                fputs("null", w->out);
                continue;
            }
            fputs("{\"kind\":", w->out);
            if (!pt->is_expr) {
                jw_str(w, "Lit", 3);
                fputs(",\"text\":", w->out);
                jw_slice(w, pt->text);
            } else {
                jw_str(w, "Expr", 4);
                fputs(",\"expr\":", w->out);
                emit_expr(w, pt->expr);
            }
            fputs(",\"span\":", w->out);
            jw_span(w, pt->span);
            fputs("}", w->out);
        }
        fputc(']', w->out);
    } else if (e->kind == EMP_EXPR_UNARY) {
        fputs(",\"op\":", w->out);
        jw_str(w, emp_unop_name(e->as.unary.op), strlen(emp_unop_name(e->as.unary.op)));
        fputs(",\"rhs\":", w->out);
        emit_expr(w, e->as.unary.rhs);
    } else if (e->kind == EMP_EXPR_BINARY) {
        fputs(",\"op\":", w->out);
        jw_str(w, emp_binop_name(e->as.binary.op), strlen(emp_binop_name(e->as.binary.op)));
        fputs(",\"lhs\":", w->out);
        emit_expr(w, e->as.binary.lhs);
        fputs(",\"rhs\":", w->out);
        emit_expr(w, e->as.binary.rhs);
    } else if (e->kind == EMP_EXPR_CALL) {
        fputs(",\"callee\":", w->out);
        emit_expr(w, e->as.call.callee);
        fputs(",\"args\":[", w->out);
        for (size_t i = 0; i < e->as.call.args.len; i++) {
            if (i) fputc(',', w->out);
            emit_expr(w, (const EmpExpr *)e->as.call.args.items[i]);
        }
        fputc(']', w->out);
    } else if (e->kind == EMP_EXPR_GROUP) {
        fputs(",\"inner\":", w->out);
        emit_expr(w, e->as.group.inner);
    } else if (e->kind == EMP_EXPR_CAST) {
        fputs(",\"type\":", w->out);
        emit_type(w, e->as.cast.ty);
        fputs(",\"expr\":", w->out);
        emit_expr(w, e->as.cast.expr);
    } else if (e->kind == EMP_EXPR_TUPLE) {
        fputs(",\"items\":[", w->out);
        for (size_t i = 0; i < e->as.tuple.items.len; i++) {
            if (i) fputc(',', w->out);
            emit_expr(w, (const EmpExpr *)e->as.tuple.items.items[i]);
        }
        fputc(']', w->out);
    } else if (e->kind == EMP_EXPR_INDEX) {
        fputs(",\"base\":", w->out);
        emit_expr(w, e->as.index.base);
        fputs(",\"index\":", w->out);
        emit_expr(w, e->as.index.index);
    } else if (e->kind == EMP_EXPR_MEMBER) {
        fputs(",\"base\":", w->out);
        emit_expr(w, e->as.member.base);
        fputs(",\"name\":", w->out);
        jw_slice(w, e->as.member.member);
    } else if (e->kind == EMP_EXPR_NEW) {
        fputs(",\"class\":", w->out);
        jw_slice(w, e->as.new_expr.class_name);
        fputs(",\"args\":[", w->out);
        for (size_t i = 0; i < e->as.new_expr.args.len; i++) {
            if (i) fputc(',', w->out);
            emit_expr(w, (const EmpExpr *)e->as.new_expr.args.items[i]);
        }
        fputc(']', w->out);
    } else if (e->kind == EMP_EXPR_RANGE) {
        fputs(",\"start\":", w->out);
        emit_expr(w, e->as.range.start);
        fputs(",\"end\":", w->out);
        emit_expr(w, e->as.range.end);
        fputs(",\"inclusive\":", w->out);
        fputs(e->as.range.inclusive ? "true" : "false", w->out);
    }

    fputs("}", w->out);
}

static void emit_stmt(EmpJsonW *w, const EmpStmt *s) {
    if (!s) {
        fputs("null", w->out);
        return;
    }

    fputs("{", w->out);
    fputs("\"kind\":", w->out);

    switch (s->kind) {
        case EMP_STMT_VAR: jw_str(w, "Var", 3); break;
        case EMP_STMT_DROP: jw_str(w, "Drop", 4); break;
        case EMP_STMT_DEFER: jw_str(w, "Defer", 5); break;
        case EMP_STMT_RETURN: jw_str(w, "Return", 6); break;
        case EMP_STMT_EXPR: jw_str(w, "Expr", 4); break;
        case EMP_STMT_TAG: jw_str(w, "Tag", 3); break;
        case EMP_STMT_BLOCK: jw_str(w, "Block", 5); break;
        case EMP_STMT_IF: jw_str(w, "If", 2); break;
        case EMP_STMT_WHILE: jw_str(w, "While", 5); break;
        case EMP_STMT_FOR: jw_str(w, "For", 3); break;
        case EMP_STMT_BREAK: jw_str(w, "Break", 5); break;
        case EMP_STMT_CONTINUE: jw_str(w, "Continue", 8); break;
        case EMP_STMT_MATCH: jw_str(w, "Match", 5); break;
        case EMP_STMT_EMP_OFF: jw_str(w, "EmpOff", 6); break;
        case EMP_STMT_EMP_MM_OFF: jw_str(w, "EmpMmOff", 8); break;
        default: jw_str(w, "Unknown", 7); break;
    }

    fputs(",\"span\":", w->out);
    jw_span(w, s->span);

    if (s->kind == EMP_STMT_VAR) {
        fputs(",\"type\":", w->out);
        emit_type(w, s->as.let_stmt.ty);
        if (s->as.let_stmt.is_destructure) {
            fputs(",\"destructure\":[", w->out);
            for (size_t i = 0; i < s->as.let_stmt.destruct_names.len; i++) {
                if (i) fputc(',', w->out);
                const EmpSlice *nm = (const EmpSlice *)s->as.let_stmt.destruct_names.items[i];
                if (nm) jw_slice(w, *nm); else fputs("null", w->out);
            }
            fputc(']', w->out);
        } else {
            fputs(",\"name\":", w->out);
            jw_slice(w, s->as.let_stmt.name);
        }
        fputs(",\"init\":", w->out);
        emit_expr(w, s->as.let_stmt.init);
    } else if (s->kind == EMP_STMT_TAG) {
        fputs(",\"name\":", w->out);
        jw_slice(w, s->as.tag_stmt.name);
    } else if (s->kind == EMP_STMT_DROP) {
        fputs(",\"name\":", w->out);
        jw_slice(w, s->as.drop_stmt.name);
    } else if (s->kind == EMP_STMT_DEFER) {
        fputs(",\"body\":", w->out);
        emit_stmt(w, s->as.defer_stmt.body);
    } else if (s->kind == EMP_STMT_RETURN) {
        fputs(",\"value\":", w->out);
        emit_expr(w, s->as.ret.value);
    } else if (s->kind == EMP_STMT_EXPR) {
        fputs(",\"expr\":", w->out);
        emit_expr(w, s->as.expr.expr);
    } else if (s->kind == EMP_STMT_BLOCK) {
        fputs(",\"stmts\":[", w->out);
        for (size_t i = 0; i < s->as.block.stmts.len; i++) {
            if (i) fputc(',', w->out);
            emit_stmt(w, (const EmpStmt *)s->as.block.stmts.items[i]);
        }
        fputc(']', w->out);
    } else if (s->kind == EMP_STMT_IF) {
        fputs(",\"cond\":", w->out);
        emit_expr(w, s->as.if_stmt.cond);
        fputs(",\"then\":", w->out);
        emit_stmt(w, s->as.if_stmt.then_branch);
        fputs(",\"else\":", w->out);
        emit_stmt(w, s->as.if_stmt.else_branch);
    } else if (s->kind == EMP_STMT_WHILE) {
        fputs(",\"cond\":", w->out);
        emit_expr(w, s->as.while_stmt.cond);
        fputs(",\"body\":", w->out);
        emit_stmt(w, s->as.while_stmt.body);
    } else if (s->kind == EMP_STMT_FOR) {
        fputs(",\"idx\":", w->out);
        jw_slice(w, s->as.for_stmt.idx_name);
        fputs(",\"val\":", w->out);
        if (s->as.for_stmt.val_name.ptr && s->as.for_stmt.val_name.len) jw_slice(w, s->as.for_stmt.val_name); else fputs("null", w->out);
        fputs(",\"in\":", w->out);
        emit_expr(w, s->as.for_stmt.iterable);
        fputs(",\"body\":", w->out);
        emit_stmt(w, s->as.for_stmt.body);
    } else if (s->kind == EMP_STMT_MATCH) {
        fputs(",\"scrutinee\":", w->out);
        emit_expr(w, s->as.match_stmt.scrutinee);
        fputs(",\"arms\":[", w->out);
        for (size_t i = 0; i < s->as.match_stmt.arms.len; i++) {
            if (i) fputc(',', w->out);
            const EmpMatchArm *a = (const EmpMatchArm *)s->as.match_stmt.arms.items[i];
            if (!a) {
                fputs("null", w->out);
                continue;
            }
            fputs("{\"default\":", w->out);
            fputs(a->is_default ? "true" : "false", w->out);
            fputs(",\"pat\":", w->out);
            if (a->is_default) fputs("null", w->out);
            else emit_expr(w, a->pat);
            fputs(",\"body\":", w->out);
            emit_stmt(w, a->body);
            fputs("}", w->out);
        }
        fputc(']', w->out);
    } else if (s->kind == EMP_STMT_EMP_OFF) {
        fputs(",\"body\":", w->out);
        emit_stmt(w, s->as.emp_off.body);
    } else if (s->kind == EMP_STMT_EMP_MM_OFF) {
        fputs(",\"body\":", w->out);
        emit_stmt(w, s->as.emp_mm_off.body);
    }

    fputs("}", w->out);
}

void emp_program_to_json(FILE *out, const EmpProgram *p, const EmpDiags *diags) {
    EmpJsonW w;
    w.out = out;
    w.indent = 0;

    fputs("{", out);

    fputs("\"diags\":[", out);
    if (diags) {
        for (size_t i = 0; i < diags->len; i++) {
            if (i) fputc(',', out);
            fputs("{\"message\":", out);
            jw_str(&w, diags->items[i].message, strlen(diags->items[i].message));
            fputs(",\"span\":", out);
            jw_span(&w, diags->items[i].span);
            fputs("}", out);
        }
    }
    fputs("]", out);

    fputs(",\"program\":{\"items\":[", out);

    if (p) {
        for (size_t i = 0; i < p->items.len; i++) {
            if (i) fputc(',', out);
            const EmpItem *it = (const EmpItem *)p->items.items[i];
            if (!it) {
                fputs("null", out);
                continue;
            }
            if (it->kind == EMP_ITEM_TAG) {
                fputs("{\"kind\":\"Tag\",\"name\":", out);
                jw_slice(&w, it->as.tag.name);
                fputs(",\"span\":", out);
                jw_span(&w, it->span);
                fputs("}", out);
            } else if (it->kind == EMP_ITEM_EMP_MM_OFF) {
                fputs("{\"kind\":\"EmpMmOff\",\"span\":", out);
                jw_span(&w, it->span);
                fputs("}", out);
            } else if (it->kind == EMP_ITEM_FN) {
                fputs("{\"kind\":\"Fn\",\"name\":", out);
                jw_slice(&w, it->as.fn.name);
                fputs(",\"isExported\":", out);
                fputs(it->as.fn.is_exported ? "true" : "false", out);
                fputs(",\"isExtern\":", out);
                fputs(it->as.fn.is_extern ? "true" : "false", out);
                fputs(",\"abi\":", out);
                if (it->as.fn.abi.ptr && it->as.fn.abi.len) jw_slice(&w, it->as.fn.abi);
                else fputs("null", out);
                fputs(",\"isUnsafe\":", out);
                fputs(it->as.fn.is_unsafe ? "true" : "false", out);
                fputs(",\"params\":[", out);
                for (size_t j = 0; j < it->as.fn.params.len; j++) {
                    if (j) fputc(',', out);
                    const EmpParam *param = (const EmpParam *)it->as.fn.params.items[j];
                    fputs("{\"name\":", out);
                    jw_slice(&w, param->name);
                    fputs(",\"type\":", out);
                    emit_type(&w, param->ty);
                    fputs(",\"span\":", out);
                    jw_span(&w, param->span);
                    fputs("}", out);
                }
                fputs("]", out);
                fputs(",\"returns\":", out);
                emit_type(&w, it->as.fn.ret_ty);
                fputs(",\"body\":", out);
                emit_stmt(&w, it->as.fn.body);
                fputs("}", out);
            } else if (it->kind == EMP_ITEM_USE) {
                fputs("{\"kind\":\"Use\",\"from\":", out);
                jw_slice(&w, it->as.use.from_path);
                fputs(",\"mode\":", out);
                if (it->as.use.wildcard) {
                    fputs(it->as.use.allow_private ? "\"all\"" : "\"star\"", out);
                } else {
                    fputs("\"list\"", out);
                }
                fputs(",\"names\":", out);
                if (it->as.use.wildcard) {
                    fputs("null", out);
                } else {
                    fputs("[", out);
                    for (size_t j = 0; j < it->as.use.names.len; j++) {
                        if (j) fputc(',', out);
                        const EmpUseName *u = (const EmpUseName *)it->as.use.names.items[j];
                        if (!u) {
                            fputs("null", out);
                            continue;
                        }
                        fputs("{\"name\":", out);
                        jw_slice(&w, u->name);
                        fputs(",\"alias\":", out);
                        if (u->alias.len) jw_slice(&w, u->alias);
                        else fputs("null", out);
                        fputs("}", out);
                    }
                    fputs("]", out);
                }
                fputs(",\"span\":", out);
                jw_span(&w, it->span);
                fputs("}", out);
            } else if (it->kind == EMP_ITEM_CLASS) {
                fputs("{\"kind\":\"Class\",\"name\":", out);
                jw_slice(&w, it->as.class_decl.name);
                fputs(",\"isExported\":", out);
                fputs(it->as.class_decl.is_exported ? "true" : "false", out);
                fputs(",\"base\":", out);
                if (it->as.class_decl.base_name.ptr && it->as.class_decl.base_name.len) jw_slice(&w, it->as.class_decl.base_name);
                else fputs("null", out);

                fputs(",\"fields\":[", out);
                for (size_t j = 0; j < it->as.class_decl.fields.len; j++) {
                    if (j) fputc(',', out);
                    const EmpClassField *f = (const EmpClassField *)it->as.class_decl.fields.items[j];
                    if (!f) {
                        fputs("null", out);
                        continue;
                    }
                    fputs("{\"name\":", out);
                    jw_slice(&w, f->name);
                    fputs(",\"type\":", out);
                    emit_type(&w, f->ty);
                    fputs(",\"span\":", out);
                    jw_span(&w, f->span);
                    fputs("}", out);
                }
                fputs("]", out);

                fputs(",\"methods\":[", out);
                for (size_t j = 0; j < it->as.class_decl.methods.len; j++) {
                    if (j) fputc(',', out);
                    const EmpClassMethod *m = (const EmpClassMethod *)it->as.class_decl.methods.items[j];
                    if (!m) {
                        fputs("null", out);
                        continue;
                    }
                    fputs("{\"name\":", out);
                    jw_slice(&w, m->name);
                    fputs(",\"isInit\":", out);
                    fputs(m->is_init ? "true" : "false", out);
                    fputs(",\"isVirtual\":", out);
                    fputs(m->is_virtual ? "true" : "false", out);
                    fputs(",\"params\":[", out);
                    for (size_t k = 0; k < m->params.len; k++) {
                        if (k) fputc(',', out);
                        const EmpParam *param = (const EmpParam *)m->params.items[k];
                        if (!param) {
                            fputs("null", out);
                            continue;
                        }
                        fputs("{\"name\":", out);
                        jw_slice(&w, param->name);
                        fputs(",\"type\":", out);
                        emit_type(&w, param->ty);
                        fputs(",\"span\":", out);
                        jw_span(&w, param->span);
                        fputs("}", out);
                    }
                    fputs("]", out);
                    fputs(",\"returns\":", out);
                    emit_type(&w, m->ret_ty);
                    fputs(",\"body\":", out);
                    emit_stmt(&w, m->body);
                    fputs(",\"span\":", out);
                    jw_span(&w, m->span);
                    fputs("}", out);
                }
                fputs("]", out);

                fputs(",\"span\":", out);
                jw_span(&w, it->span);
                fputs("}", out);
            } else if (it->kind == EMP_ITEM_TRAIT) {
                fputs("{\"kind\":\"Trait\",\"name\":", out);
                jw_slice(&w, it->as.trait_decl.name);
                fputs(",\"isExported\":", out);
                fputs(it->as.trait_decl.is_exported ? "true" : "false", out);

                fputs(",\"methods\":[", out);
                for (size_t j = 0; j < it->as.trait_decl.methods.len; j++) {
                    if (j) fputc(',', out);
                    const EmpTraitMethod *m = (const EmpTraitMethod *)it->as.trait_decl.methods.items[j];
                    if (!m) {
                        fputs("null", out);
                        continue;
                    }
                    fputs("{\"name\":", out);
                    jw_slice(&w, m->name);
                    fputs(",\"params\":[", out);
                    for (size_t k = 0; k < m->params.len; k++) {
                        if (k) fputc(',', out);
                        const EmpParam *param = (const EmpParam *)m->params.items[k];
                        if (!param) {
                            fputs("null", out);
                            continue;
                        }
                        fputs("{\"name\":", out);
                        jw_slice(&w, param->name);
                        fputs(",\"type\":", out);
                        emit_type(&w, param->ty);
                        fputs(",\"span\":", out);
                        jw_span(&w, param->span);
                        fputs("}", out);
                    }
                    fputs("]", out);
                    fputs(",\"returns\":", out);
                    emit_type(&w, m->ret_ty);
                    fputs(",\"body\":", out);
                    emit_stmt(&w, m->body);
                    fputs(",\"span\":", out);
                    jw_span(&w, m->span);
                    fputs("}", out);
                }
                fputs("]", out);

                fputs(",\"span\":", out);
                jw_span(&w, it->span);
                fputs("}", out);
            } else if (it->kind == EMP_ITEM_CONST) {
                fputs("{\"kind\":\"Const\",\"name\":", out);
                jw_slice(&w, it->as.const_decl.name);
                fputs(",\"isExported\":", out);
                fputs(it->as.const_decl.is_exported ? "true" : "false", out);
                fputs(",\"type\":", out);
                emit_type(&w, it->as.const_decl.ty);
                fputs(",\"init\":", out);
                emit_expr(&w, it->as.const_decl.init);
                fputs(",\"span\":", out);
                jw_span(&w, it->span);
                fputs("}", out);
            } else if (it->kind == EMP_ITEM_STRUCT) {
                fputs("{\"kind\":\"Struct\",\"name\":", out);
                jw_slice(&w, it->as.struct_decl.name);
                fputs(",\"isExported\":", out);
                fputs(it->as.struct_decl.is_exported ? "true" : "false", out);
                fputs(",\"fields\":[", out);
                for (size_t j = 0; j < it->as.struct_decl.fields.len; j++) {
                    if (j) fputc(',', out);
                    const EmpStructField *f = (const EmpStructField *)it->as.struct_decl.fields.items[j];
                    if (!f) {
                        fputs("null", out);
                        continue;
                    }
                    fputs("{\"name\":", out);
                    jw_slice(&w, f->name);
                    fputs(",\"type\":", out);
                    emit_type(&w, f->ty);
                    fputs(",\"span\":", out);
                    jw_span(&w, f->span);
                    fputs("}", out);
                }
                fputs("]", out);
                fputs(",\"span\":", out);
                jw_span(&w, it->span);
                fputs("}", out);
            } else if (it->kind == EMP_ITEM_ENUM) {
                fputs("{\"kind\":\"Enum\",\"name\":", out);
                jw_slice(&w, it->as.enum_decl.name);
                fputs(",\"isExported\":", out);
                fputs(it->as.enum_decl.is_exported ? "true" : "false", out);
                fputs(",\"variants\":[", out);
                for (size_t j = 0; j < it->as.enum_decl.variants.len; j++) {
                    if (j) fputc(',', out);
                    const EmpEnumVariant *v = (const EmpEnumVariant *)it->as.enum_decl.variants.items[j];
                    if (!v) {
                        fputs("null", out);
                        continue;
                    }
                    fputs("{\"name\":", out);
                    jw_slice(&w, v->name);
                    fputs(",\"fields\":[", out);
                    for (size_t k = 0; k < v->fields.len; k++) {
                        if (k) fputc(',', out);
                        emit_type(&w, (const EmpType *)v->fields.items[k]);
                    }
                    fputs("]}", out);
                }
                fputs("]", out);
                fputs(",\"span\":", out);
                jw_span(&w, it->span);
                fputs("}", out);
            } else if (it->kind == EMP_ITEM_IMPL) {
                fputs("{\"kind\":\"Impl\",\"target\":", out);
                jw_slice(&w, it->as.impl_decl.target_name);
                fputs(",\"trait\":", out);
                if (it->as.impl_decl.trait_name.ptr && it->as.impl_decl.trait_name.len) {
                    jw_slice(&w, it->as.impl_decl.trait_name);
                } else {
                    fputs("null", out);
                }
                fputs(",\"methods\":[", out);
                for (size_t j = 0; j < it->as.impl_decl.methods.len; j++) {
                    if (j) fputc(',', out);
                    const EmpImplMethod *m = (const EmpImplMethod *)it->as.impl_decl.methods.items[j];
                    if (!m) {
                        fputs("null", out);
                        continue;
                    }
                    fputs("{\"name\":", out);
                    jw_slice(&w, m->name);
                    fputs(",\"isExported\":", out);
                    fputs(m->is_exported ? "true" : "false", out);
                    fputs(",\"isUnsafe\":", out);
                    fputs(m->is_unsafe ? "true" : "false", out);
                    fputs(",\"params\":[", out);
                    for (size_t k = 0; k < m->params.len; k++) {
                        if (k) fputc(',', out);
                        const EmpParam *param = (const EmpParam *)m->params.items[k];
                        if (!param) {
                            fputs("null", out);
                            continue;
                        }
                        fputs("{\"name\":", out);
                        jw_slice(&w, param->name);
                        fputs(",\"type\":", out);
                        emit_type(&w, param->ty);
                        fputs(",\"span\":", out);
                        jw_span(&w, param->span);
                        fputs("}", out);
                    }
                    fputs("]", out);
                    fputs(",\"returns\":", out);
                    emit_type(&w, m->ret_ty);
                    fputs(",\"body\":", out);
                    emit_stmt(&w, m->body);
                    fputs(",\"span\":", out);
                    jw_span(&w, m->span);
                    fputs("}", out);
                }
                fputs("]", out);
                fputs(",\"span\":", out);
                jw_span(&w, it->span);
                fputs("}", out);
            } else {
                fputs("{\"kind\":\"Unknown\"}", out);
            }
        }
    }

    fputs("]}", out);

    fputs("}", out);
    jw_nl(&w);
}
