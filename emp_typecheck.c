#include "emp_typecheck.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *arena_strdup(EmpArena *arena, const char *s) {
    if (!arena || !s) return NULL;
    size_t n = strlen(s);
    char *p = (char *)emp_arena_alloc(arena, n + 1, 1);
    if (!p) return NULL;
    memcpy(p, s, n + 1);
    return p;
}

static void diagf(EmpArena *arena, EmpDiags *diags, EmpSpan span, const char *prefix, const char *msg) {
    if (!diags) return;

    char buf[512];
    if (!prefix) prefix = "";
    if (!msg) msg = "";
    snprintf(buf, sizeof(buf), "%s%s", prefix, msg);

    EmpDiag d;
    d.span = span;
    d.message = arena_strdup(arena, buf);
    (void)emp_diags_push(diags, d);
}

static bool slice_eq(EmpSlice a, EmpSlice b) {
    if (a.len != b.len) return false;
    if (a.ptr == b.ptr) return true;
    if (!a.ptr || !b.ptr) return false;
    return memcmp(a.ptr, b.ptr, a.len) == 0;
}

static const char *slice_to_cstr(EmpArena *arena, EmpSlice s) {
    if (!arena || !s.ptr || s.len == 0) return "";
    char *p = (char *)emp_arena_alloc(arena, s.len + 1, 1);
    if (!p) return "";
    memcpy(p, s.ptr, s.len);
    p[s.len] = '\0';
    return p;
}

static bool slice_is(EmpSlice s, const char *lit) {
    if (!lit) return false;
    size_t n = strlen(lit);
    return s.ptr && s.len == n && memcmp(s.ptr, lit, n) == 0;
}

static EmpSlice slice_from_cstr(const char *s) {
    EmpSlice out;
    out.ptr = s;
    out.len = s ? (int)strlen(s) : 0;
    return out;
}

typedef struct StrBuf {
    char *data;
    size_t len;
    size_t cap;
} StrBuf;

static void sb_free(StrBuf *sb) {
    if (!sb) return;
    free(sb->data);
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
}

static bool sb_reserve(StrBuf *sb, size_t add) {
    if (!sb) return false;
    size_t need = sb->len + add + 1;
    if (need <= sb->cap) return true;
    size_t nc = sb->cap ? sb->cap * 2 : 128;
    while (nc < need) nc *= 2;
    char *p = (char *)realloc(sb->data, nc);
    if (!p) return false;
    sb->data = p;
    sb->cap = nc;
    return true;
}

static bool sb_append_n(StrBuf *sb, const char *s, size_t n) {
    if (!sb) return false;
    if (!s || n == 0) return true;
    if (!sb_reserve(sb, n)) return false;
    memcpy(sb->data + sb->len, s, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
    return true;
}

static bool sb_append_c(StrBuf *sb, char c) {
    return sb_append_n(sb, &c, 1);
}

static bool sb_append_slice(StrBuf *sb, EmpSlice s) {
    if (!s.ptr || s.len == 0) return true;
    return sb_append_n(sb, s.ptr, (size_t)s.len);
}

static EmpSlice sb_to_arena_slice(EmpArena *arena, const StrBuf *sb) {
    EmpSlice out = {0};
    if (!arena || !sb || !sb->data || sb->len == 0) return out;
    char *p = (char *)emp_arena_alloc(arena, sb->len + 1, 1);
    if (!p) return out;
    memcpy(p, sb->data, sb->len);
    p[sb->len] = '\0';
    out.ptr = p;
    out.len = (int)sb->len;
    return out;
}

static bool mangle_type_sb(StrBuf *sb, const EmpType *t) {
    if (!sb) return false;
    if (!t) return sb_append_n(sb, "V", 1); // void/unknown

    switch (t->kind) {
        case EMP_TYPE_AUTO:
            return sb_append_n(sb, "U", 1);
        case EMP_TYPE_NAME:
            if (!sb_append_n(sb, "N", 1)) return false;
            return sb_append_slice(sb, t->as.name);
        case EMP_TYPE_DYN:
            if (!sb_append_n(sb, "D", 1)) return false;
            return sb_append_slice(sb, t->as.dyn.base_name);
        case EMP_TYPE_PTR:
            if (!sb_append_n(sb, "P", 1)) return false;
            return mangle_type_sb(sb, t->as.ptr.pointee);
        case EMP_TYPE_ARRAY:
            if (!sb_append_n(sb, "A", 1)) return false;
            if (t->as.array.size_text.ptr && t->as.array.size_text.len) {
                if (!sb_append_slice(sb, t->as.array.size_text)) return false;
            } else {
                if (!sb_append_n(sb, "0", 1)) return false;
            }
            if (!sb_append_c(sb, '_')) return false;
            return mangle_type_sb(sb, t->as.array.elem);
        case EMP_TYPE_LIST:
            if (!sb_append_n(sb, "L", 1)) return false;
            return mangle_type_sb(sb, t->as.array.elem);
        case EMP_TYPE_TUPLE: {
            if (!sb_append_n(sb, "T", 1)) return false;
            char numbuf[32];
            snprintf(numbuf, sizeof(numbuf), "%zu", t->as.tuple.fields.len);
            if (!sb_append_n(sb, numbuf, strlen(numbuf))) return false;
            for (size_t i = 0; i < t->as.tuple.fields.len; i++) {
                const EmpTupleField *f = (const EmpTupleField *)t->as.tuple.fields.items[i];
                if (!sb_append_c(sb, '_')) return false;
                if (!mangle_type_sb(sb, f ? f->ty : NULL)) return false;
            }
            return true;
        }
        default:
            return sb_append_n(sb, "X", 1);
    }
}

static EmpSlice mangle_overload_name(EmpArena *arena, EmpSlice base, EmpType **params, size_t params_len) {
    StrBuf sb;
    memset(&sb, 0, sizeof(sb));

    (void)sb_append_slice(&sb, base);
    (void)sb_append_n(&sb, "__", 2);

    if (params_len == 0) {
        (void)sb_append_n(&sb, "v", 1);
    } else {
        for (size_t i = 0; i < params_len; i++) {
            if (i) (void)sb_append_c(&sb, '_');
            (void)mangle_type_sb(&sb, params ? params[i] : NULL);
        }
    }

    EmpSlice out = sb_to_arena_slice(arena, &sb);
    sb_free(&sb);
    return out;
}

static EmpSlice mangle2(EmpArena *arena, EmpSlice a, const char *mid, EmpSlice b) {
    StrBuf sb;
    memset(&sb, 0, sizeof(sb));
    (void)sb_append_slice(&sb, a);
    if (mid) (void)sb_append_n(&sb, mid, strlen(mid));
    (void)sb_append_slice(&sb, b);
    EmpSlice out = sb_to_arena_slice(arena, &sb);
    sb_free(&sb);
    return out;
}

static EmpType *make_named(EmpArena *arena, EmpSpan span, const char *name) {
    EmpType *t = (EmpType *)emp_arena_alloc(arena, sizeof(EmpType), _Alignof(EmpType));
    if (!t) return NULL;
    memset(t, 0, sizeof(*t));
    t->kind = EMP_TYPE_NAME;
    t->span = span;
    t->as.name = slice_from_cstr(name);
    return t;
}

static EmpType *make_ptr(EmpArena *arena, EmpSpan span, EmpType *pointee) {
    EmpType *t = (EmpType *)emp_arena_alloc(arena, sizeof(EmpType), _Alignof(EmpType));
    if (!t) return NULL;
    memset(t, 0, sizeof(*t));
    t->kind = EMP_TYPE_PTR;
    t->span = span;
    t->as.ptr.pointee = pointee;
    return t;
}

static EmpType *make_list(EmpArena *arena, EmpSpan span, EmpType *elem) {
    EmpType *t = (EmpType *)emp_arena_alloc(arena, sizeof(EmpType), _Alignof(EmpType));
    if (!t) return NULL;
    memset(t, 0, sizeof(*t));
    t->kind = EMP_TYPE_LIST;
    t->span = span;
    t->as.array.elem = elem;
    t->as.array.size_text.ptr = NULL;
    t->as.array.size_text.len = 0;
    return t;
}

static EmpType *make_tuple(EmpArena *arena, EmpSpan span) {
    EmpType *t = (EmpType *)emp_arena_alloc(arena, sizeof(EmpType), _Alignof(EmpType));
    if (!t) return NULL;
    memset(t, 0, sizeof(*t));
    t->kind = EMP_TYPE_TUPLE;
    t->span = span;
    emp_vec_init(&t->as.tuple.fields);
    return t;
}

static EmpType *make_auto(EmpArena *arena, EmpSpan span) {
    EmpType *t = (EmpType *)emp_arena_alloc(arena, sizeof(EmpType), _Alignof(EmpType));
    if (!t) return NULL;
    memset(t, 0, sizeof(*t));
    t->kind = EMP_TYPE_AUTO;
    t->span = span;
    return t;
}

static bool type_is_void(const EmpType *t) {
    return t == NULL; // In EMP today, NULL return type means void.
}

static bool type_is_bool(const EmpType *t) {
    return t && t->kind == EMP_TYPE_NAME && slice_is(t->as.name, "bool");
}

static bool type_is_char(const EmpType *t) {
    return t && t->kind == EMP_TYPE_NAME && slice_is(t->as.name, "char");
}

static bool type_is_int_name(EmpSlice name) {
    static const char *const ints[] = {
        "int",
        "int8",
        "int16",
        "int32",
        "int64",
        "i8",
        "i16",
        "i32",
        "i64",
        "isize",
        "uint8",
        "uint16",
        "uint32",
        "uint64",
        "u8",
        "u16",
        "u32",
        "u64",
        "usize",
    };
    for (size_t i = 0; i < sizeof(ints) / sizeof(ints[0]); i++) {
        if (slice_is(name, ints[i])) return true;
    }
    return false;
}

static bool type_is_int(const EmpType *t) {
    return t && t->kind == EMP_TYPE_NAME && type_is_int_name(t->as.name);
}

static bool type_is_float(const EmpType *t) {
    return t && t->kind == EMP_TYPE_NAME &&
           (slice_is(t->as.name, "f32") || slice_is(t->as.name, "f64") || slice_is(t->as.name, "float") || slice_is(t->as.name, "double"));
}

static bool type_is_auto(const EmpType *t) {
    return t && t->kind == EMP_TYPE_AUTO;
}

static bool type_is_ptr(const EmpType *t) {
    return t && t->kind == EMP_TYPE_PTR;
}

static bool type_is_string(const EmpType *t) {
    return t && t->kind == EMP_TYPE_NAME && slice_is(t->as.name, "string");
}

static bool type_is_str(const EmpType *t) {
    // For now string literals are modeled as *u8 (C string).
    return t && t->kind == EMP_TYPE_PTR && t->as.ptr.pointee && t->as.ptr.pointee->kind == EMP_TYPE_NAME && slice_is(t->as.ptr.pointee->as.name, "u8");
}

static bool type_is_ptr_u8(const EmpType *t) {
    return type_is_str(t);
}

static bool type_is_stringlike(const EmpType *t) {
    return type_is_str(t) || type_is_string(t);
}

static bool type_is_name(const EmpType *t, const char *lit) {
    return t && t->kind == EMP_TYPE_NAME && slice_is(t->as.name, lit);
}

// Forward declarations for helpers defined later in this file.
static const EmpItem *find_named_decl(EmpProgram *p, EmpSlice name);
static bool type_eq_shallow(const EmpType *a, const EmpType *b);

static EmpProgram *g_tc_program = NULL;
static int g_tc_mm_depth = 0;

static bool program_has_emp_mm_off(const EmpProgram *program) {
    if (!program) return false;
    for (size_t i = 0; i < program->items.len; i++) {
        const EmpItem *it = (const EmpItem *)program->items.items[i];
        if (it && it->kind == EMP_ITEM_EMP_MM_OFF) return true;
    }
    return false;
}

static const EmpItem *find_class_decl(EmpProgram *p, EmpSlice name) {
    const EmpItem *it = find_named_decl(p, name);
    if (!it || it->kind != EMP_ITEM_CLASS) return NULL;
    return it;
}

static const EmpItem *find_enum_decl(EmpProgram *p, EmpSlice name) {
    const EmpItem *it = find_named_decl(p, name);
    if (!it || it->kind != EMP_ITEM_ENUM) return NULL;
    return it;
}

static const EmpItem *find_trait_decl(EmpProgram *p, EmpSlice name) {
    const EmpItem *it = find_named_decl(p, name);
    if (!it || it->kind != EMP_ITEM_TRAIT) return NULL;
    return it;
}

static const EmpTraitMethod *find_trait_method(const EmpItem *tr, EmpSlice method_name) {
    if (!tr || tr->kind != EMP_ITEM_TRAIT) return NULL;
    for (size_t i = 0; i < tr->as.trait_decl.methods.len; i++) {
        const EmpTraitMethod *m = (const EmpTraitMethod *)tr->as.trait_decl.methods.items[i];
        if (!m) continue;
        if (slice_eq(m->name, method_name)) return m;
    }
    return NULL;
}

static const EmpEnumVariant *find_enum_variant(const EmpItem *en, EmpSlice variant_name, size_t *out_index) {
    if (out_index) *out_index = 0;
    if (!en || en->kind != EMP_ITEM_ENUM) return NULL;
    for (size_t i = 0; i < en->as.enum_decl.variants.len; i++) {
        const EmpEnumVariant *v = (const EmpEnumVariant *)en->as.enum_decl.variants.items[i];
        if (!v) continue;
        if (slice_eq(v->name, variant_name)) {
            if (out_index) *out_index = i;
            return v;
        }
    }
    return NULL;
}

static bool method_sig_exact_matches(const EmpVec *params, const EmpType *ret_ty, const EmpVec *want_params, const EmpType *want_ret_ty) {
    if (!params || !want_params) return false;
    if (params->len != want_params->len) return false;

    if (type_is_void(ret_ty) != type_is_void(want_ret_ty)) return false;
    if (!type_is_void(ret_ty) && !type_eq_shallow(ret_ty, want_ret_ty)) return false;

    for (size_t i = 0; i < params->len; i++) {
        const EmpParam *a = (const EmpParam *)params->items[i];
        const EmpParam *b = (const EmpParam *)want_params->items[i];
        if (!a || !b) return false;
        if (!type_eq_shallow(a->ty, b->ty)) return false;
    }
    return true;
}

static bool type_matches_trait_self(const EmpType *trait_ty, const EmpType *impl_ty, EmpSlice self_name) {
    if (!trait_ty || !impl_ty) return false;

    // In trait method signatures, `auto` stands for `Self`.
    if (trait_ty->kind == EMP_TYPE_AUTO) {
        return impl_ty->kind == EMP_TYPE_NAME && slice_eq(impl_ty->as.name, self_name);
    }

    if (trait_ty->kind != impl_ty->kind) return false;
    switch (trait_ty->kind) {
        case EMP_TYPE_NAME:
            return slice_eq(trait_ty->as.name, impl_ty->as.name);
        case EMP_TYPE_PTR:
            return type_matches_trait_self(trait_ty->as.ptr.pointee, impl_ty->as.ptr.pointee, self_name);
        case EMP_TYPE_LIST:
            return type_matches_trait_self(trait_ty->as.array.elem, impl_ty->as.array.elem, self_name);
        case EMP_TYPE_ARRAY:
            if (!slice_eq(trait_ty->as.array.size_text, impl_ty->as.array.size_text)) return false;
            return type_matches_trait_self(trait_ty->as.array.elem, impl_ty->as.array.elem, self_name);
        case EMP_TYPE_TUPLE:
            if (trait_ty->as.tuple.fields.len != impl_ty->as.tuple.fields.len) return false;
            for (size_t i = 0; i < trait_ty->as.tuple.fields.len; i++) {
                const EmpTupleField *a = (const EmpTupleField *)trait_ty->as.tuple.fields.items[i];
                const EmpTupleField *b = (const EmpTupleField *)impl_ty->as.tuple.fields.items[i];
                if (!a || !b) return false;
                if (!type_matches_trait_self(a->ty, b->ty, self_name)) return false;
            }
            return true;
        case EMP_TYPE_DYN:
            return slice_eq(trait_ty->as.dyn.base_name, impl_ty->as.dyn.base_name);
        default:
            return false;
    }
}

static const EmpImplMethod *find_impl_method_exact(EmpProgram *p, EmpSlice recv_name, const EmpClassMethod *base_m) {
    if (!p || !base_m) return NULL;
    for (size_t i = 0; i < p->items.len; i++) {
        const EmpItem *it = (const EmpItem *)p->items.items[i];
        if (!it || it->kind != EMP_ITEM_IMPL) continue;
        if (it->as.impl_decl.trait_name.ptr && it->as.impl_decl.trait_name.len) continue;
        if (!slice_eq(it->as.impl_decl.target_name, recv_name)) continue;
        for (size_t j = 0; j < it->as.impl_decl.methods.len; j++) {
            const EmpImplMethod *m = (const EmpImplMethod *)it->as.impl_decl.methods.items[j];
            if (!m) continue;
            if (!slice_eq(m->name, base_m->name)) continue;
            if (m->params.len != base_m->params.len) continue;
            if (!method_sig_exact_matches(&m->params, m->ret_ty, &base_m->params, base_m->ret_ty)) continue;
            return m;
        }
    }
    return NULL;
}

static const EmpClassMethod *find_class_method_exact(EmpProgram *p, EmpSlice recv_name, const EmpClassMethod *base_m) {
    if (!p || !base_m) return NULL;
    const EmpItem *decl = find_class_decl(p, recv_name);
    if (!decl) return NULL;
    for (size_t i = 0; i < decl->as.class_decl.methods.len; i++) {
        const EmpClassMethod *m = (const EmpClassMethod *)decl->as.class_decl.methods.items[i];
        if (!m) continue;
        if (!slice_eq(m->name, base_m->name)) continue;
        if (m->params.len != base_m->params.len) continue;
        if (!method_sig_exact_matches(&m->params, m->ret_ty, &base_m->params, base_m->ret_ty)) continue;
        return m;
    }
    return NULL;
}

static uint32_t dyn_slot_for_base_method(const EmpItem *base_class_decl, const EmpClassMethod *want) {
    if (!base_class_decl || base_class_decl->kind != EMP_ITEM_CLASS || !want) return 0;
    uint32_t slot = 0;
    for (size_t i = 0; i < base_class_decl->as.class_decl.methods.len; i++) {
        const EmpClassMethod *m = (const EmpClassMethod *)base_class_decl->as.class_decl.methods.items[i];
        if (!m || m->is_init) continue;
        if (!m->is_virtual) continue;
        if (m == want) return slot;
        slot++;
    }
    return 0;
}

static const EmpItem *find_named_decl(EmpProgram *p, EmpSlice name) {
    if (!p || !name.ptr || !name.len) return NULL;
    for (size_t i = 0; i < p->items.len; i++) {
        const EmpItem *it = (const EmpItem *)p->items.items[i];
        if (!it) continue;
        if (it->kind == EMP_ITEM_STRUCT && slice_eq(it->as.struct_decl.name, name)) return it;
        if (it->kind == EMP_ITEM_CLASS && slice_eq(it->as.class_decl.name, name)) return it;
        if (it->kind == EMP_ITEM_TRAIT && slice_eq(it->as.trait_decl.name, name)) return it;
        if (it->kind == EMP_ITEM_ENUM && slice_eq(it->as.enum_decl.name, name)) return it;
    }
    return NULL;
}

static EmpItem *find_fn_item(EmpProgram *p, EmpSlice name) {
    if (!p || !name.ptr || !name.len) return NULL;
    for (size_t i = 0; i < p->items.len; i++) {
        EmpItem *it = (EmpItem *)p->items.items[i];
        if (!it) continue;
        if (it->kind == EMP_ITEM_FN && slice_eq(it->as.fn.name, name)) return it;
    }
    return NULL;
}

static const EmpType *lookup_field_type(EmpProgram *p, const EmpType *base_ty, EmpSlice field, EmpSpan err_span, EmpArena *arena, EmpDiags *diags) {
    if (!base_ty) return NULL;

    // Treat `*Named` similarly to `Named` for member access (`self.x` where `self: *Point`).
    if (base_ty->kind == EMP_TYPE_PTR && base_ty->as.ptr.pointee) {
        base_ty = base_ty->as.ptr.pointee;
        if (!base_ty) return NULL;
    }

    // Built-in list layout supports `.ptr` and `.len`.
    if (base_ty->kind == EMP_TYPE_LIST) {
        if (slice_is(field, "len")) return make_named(arena, err_span, "i32");
        if (slice_is(field, "cap")) return make_named(arena, err_span, "i32");
        if (slice_is(field, "ptr")) {
            EmpType *pt = (EmpType *)emp_arena_alloc(arena, sizeof(EmpType), sizeof(void *));
            if (!pt) return NULL;
            memset(pt, 0, sizeof(*pt));
            pt->kind = EMP_TYPE_PTR;
            pt->span = err_span;
            pt->as.ptr.pointee = base_ty->as.array.elem;
            return pt;
        }
        diagf(arena, diags, err_span, "type: ", "unknown list member (supported: ptr,len,cap)");
        return NULL;
    }

    // Tuple named fields: `(T a, U b)` supports `.a` and `.b`.
    if (base_ty->kind == EMP_TYPE_TUPLE) {
        for (size_t i = 0; i < base_ty->as.tuple.fields.len; i++) {
            const EmpTupleField *f = (const EmpTupleField *)base_ty->as.tuple.fields.items[i];
            if (!f) continue;
            if (f->name.len && slice_eq(f->name, field)) return f->ty;
        }
        diagf(arena, diags, err_span, "type: ", "unknown tuple field");
        return NULL;
    }

    if (base_ty->kind != EMP_TYPE_NAME) {
        diagf(arena, diags, err_span, "type: ", "member access base must be a named type or list");
        return NULL;
    }

    const EmpItem *decl = find_named_decl(p, base_ty->as.name);
    if (!decl) {
        diagf(arena, diags, err_span, "type: ", "unknown type in member access");
        return NULL;
    }

    if (decl->kind == EMP_ITEM_STRUCT) {
        for (size_t i = 0; i < decl->as.struct_decl.fields.len; i++) {
            const EmpStructField *f = (const EmpStructField *)decl->as.struct_decl.fields.items[i];
            if (!f) continue;
            if (slice_eq(f->name, field)) return f->ty;
        }
        diagf(arena, diags, err_span, "type: ", "unknown struct field");
        return NULL;
    }

    if (decl->kind == EMP_ITEM_CLASS) {
        for (size_t i = 0; i < decl->as.class_decl.fields.len; i++) {
            const EmpClassField *f = (const EmpClassField *)decl->as.class_decl.fields.items[i];
            if (!f) continue;
            if (slice_eq(f->name, field)) return f->ty;
        }
        diagf(arena, diags, err_span, "type: ", "unknown class field");
        return NULL;
    }

    diagf(arena, diags, err_span, "type: ", "member access not supported on this type yet");
    return NULL;
}

static bool type_eq_shallow(const EmpType *a, const EmpType *b) {
    if (a == b) return true;
    if (!a || !b) return false;
    if (a->kind == EMP_TYPE_AUTO || b->kind == EMP_TYPE_AUTO) return true;
    if (a->kind != b->kind) return false;
    switch (a->kind) {
        case EMP_TYPE_NAME:
            return slice_eq(a->as.name, b->as.name);
        case EMP_TYPE_DYN:
            return slice_eq(a->as.dyn.base_name, b->as.dyn.base_name);
        case EMP_TYPE_PTR:
            // Pointer type checking is currently shallow; pointee types are not enforced strictly yet.
            return true;
        case EMP_TYPE_ARRAY:
            if (!type_eq_shallow(a->as.array.elem, b->as.array.elem)) return false;
            return slice_eq(a->as.array.size_text, b->as.array.size_text);
        case EMP_TYPE_LIST:
            return type_eq_shallow(a->as.array.elem, b->as.array.elem);
        case EMP_TYPE_TUPLE:
            if (a->as.tuple.fields.len != b->as.tuple.fields.len) return false;
            for (size_t i = 0; i < a->as.tuple.fields.len; i++) {
                const EmpTupleField *fa = (const EmpTupleField *)a->as.tuple.fields.items[i];
                const EmpTupleField *fb = (const EmpTupleField *)b->as.tuple.fields.items[i];
                if (!fa || !fb) return false;
                if (!type_eq_shallow(fa->ty, fb->ty)) return false;
            }
            return true;
        default:
            return false;
    }
}

static EmpSlice tc_mangle_trait_method_base(EmpArena *arena, EmpSlice trait_name, EmpSlice recv_name, EmpSlice method_name) {
    // Trait methods live in a separate namespace from inherent methods.
    EmpSlice tr = mangle2(arena, trait_name, "__", recv_name);
    if (!tr.ptr || !tr.len) return (EmpSlice){0};
    return mangle2(arena, tr, "__", method_name);
}

static size_t tc_count_trait_method_overloads(const EmpProgram *p, EmpSlice trait_name, EmpSlice recv_name, EmpSlice method_name) {
    if (!p) return 0;
    size_t n = 0;
    for (size_t i = 0; i < p->items.len; i++) {
        const EmpItem *it = (const EmpItem *)p->items.items[i];
        if (!it || it->kind != EMP_ITEM_IMPL) continue;
        if (!it->as.impl_decl.trait_name.ptr || !it->as.impl_decl.trait_name.len) continue;
        if (!slice_eq(it->as.impl_decl.trait_name, trait_name)) continue;
        if (!slice_eq(it->as.impl_decl.target_name, recv_name)) continue;
        for (size_t j = 0; j < it->as.impl_decl.methods.len; j++) {
            const EmpImplMethod *m = (const EmpImplMethod *)it->as.impl_decl.methods.items[j];
            if (!m) continue;
            if (slice_eq(m->name, method_name)) n++;
        }
    }
    return n;
}

static EmpSlice tc_trait_method_symbol_name(EmpArena *arena, const EmpProgram *p, EmpSlice trait_name, EmpSlice recv_name, EmpSlice method_name, const EmpVec *params) {
    EmpSlice base = tc_mangle_trait_method_base(arena, trait_name, recv_name, method_name);
    if (!base.ptr || !base.len) return (EmpSlice){0};
    size_t n = tc_count_trait_method_overloads(p, trait_name, recv_name, method_name);
    if (n <= 1) return base;
    // params in signature are the user params (not including implicit self).
    EmpType **tmp_params = NULL;
    size_t plen = params ? params->len : 0;
    if (plen) {
        tmp_params = (EmpType **)calloc(plen, sizeof(EmpType *));
        if (tmp_params) {
            for (size_t i = 0; i < plen; i++) {
                const EmpParam *pa = (const EmpParam *)params->items[i];
                tmp_params[i] = pa ? pa->ty : NULL;
            }
        }
    }
    EmpSlice out = mangle_overload_name(arena, base, tmp_params, plen);
    free(tmp_params);
    return out;
}

static void validate_trait_impls(EmpArena *arena, EmpProgram *program, EmpDiags *diags) {
    if (!arena || !program || !diags) return;

    // Trait decl checks.
    for (size_t i = 0; i < program->items.len; i++) {
        const EmpItem *it = (const EmpItem *)program->items.items[i];
        if (!it || it->kind != EMP_ITEM_TRAIT) continue;

        for (size_t mi = 0; mi < it->as.trait_decl.methods.len; mi++) {
            const EmpTraitMethod *m = (const EmpTraitMethod *)it->as.trait_decl.methods.items[mi];
            if (!m) continue;
            if (m->body) {
                diagf(arena, diags, m->span, "type: ", "trait default method bodies are not supported yet");
            }
            // Duplicate names.
            for (size_t mj = mi + 1; mj < it->as.trait_decl.methods.len; mj++) {
                const EmpTraitMethod *n = (const EmpTraitMethod *)it->as.trait_decl.methods.items[mj];
                if (n && slice_eq(n->name, m->name)) {
                    diagf(arena, diags, n->span, "type: ", "duplicate trait method name");
                }
            }
        }
    }

    // Trait impl checks.
    for (size_t i = 0; i < program->items.len; i++) {
        const EmpItem *it = (const EmpItem *)program->items.items[i];
        if (!it || it->kind != EMP_ITEM_IMPL) continue;
        if (!it->as.impl_decl.trait_name.ptr || !it->as.impl_decl.trait_name.len) continue;

        const EmpItem *tr = find_trait_decl(program, it->as.impl_decl.trait_name);
        if (!tr) continue;

        // Each impl method must exist in trait and match signature.
        for (size_t mi = 0; mi < it->as.impl_decl.methods.len; mi++) {
            const EmpImplMethod *im = (const EmpImplMethod *)it->as.impl_decl.methods.items[mi];
            if (!im) continue;
            const EmpTraitMethod *tm = find_trait_method(tr, im->name);
            if (!tm) {
                diagf(arena, diags, im->span, "type: ", "method not declared in trait");
                continue;
            }

            if (tm->params.len != im->params.len) {
                diagf(arena, diags, im->span, "type: ", "trait method arity mismatch");
                continue;
            }

            // Return type.
            if (type_is_void(tm->ret_ty) != type_is_void(im->ret_ty)) {
                diagf(arena, diags, im->span, "type: ", "trait method return type mismatch");
            } else if (!type_is_void(tm->ret_ty) && !type_matches_trait_self(tm->ret_ty, im->ret_ty, it->as.impl_decl.target_name)) {
                diagf(arena, diags, im->span, "type: ", "trait method return type mismatch");
            }

            // Params.
            for (size_t pi = 0; pi < tm->params.len && pi < im->params.len; pi++) {
                const EmpParam *tp = (const EmpParam *)tm->params.items[pi];
                const EmpParam *ip = (const EmpParam *)im->params.items[pi];
                if (!tp || !ip) continue;
                if (!type_matches_trait_self(tp->ty, ip->ty, it->as.impl_decl.target_name)) {
                    diagf(arena, diags, ip->span, "type: ", "trait method parameter type mismatch");
                }
            }
        }

        // Each trait method must be implemented.
        for (size_t mi = 0; mi < tr->as.trait_decl.methods.len; mi++) {
            const EmpTraitMethod *tm = (const EmpTraitMethod *)tr->as.trait_decl.methods.items[mi];
            if (!tm) continue;
            bool found = false;
            for (size_t mj = 0; mj < it->as.impl_decl.methods.len; mj++) {
                const EmpImplMethod *im = (const EmpImplMethod *)it->as.impl_decl.methods.items[mj];
                if (im && slice_eq(im->name, tm->name)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                diagf(arena, diags, it->as.impl_decl.span, "type: ", "trait impl is missing a required method");
            }
        }
    }
}

typedef enum TcLitKind {
    TC_LIT_NONE,
    TC_LIT_INT,
    TC_LIT_INT_ZERO,
    TC_LIT_FLOAT,
    TC_LIT_NULL,
    TC_LIT_BOOL,
    TC_LIT_STRING,
} TcLitKind;

typedef struct TcType {
    const EmpType *ty; // can be NULL for void
    TcLitKind lit;     // when expr is a literal-like (for coercions)
} TcType;

static bool tc_is_numeric_tc(TcType t) {
    if (t.lit == TC_LIT_INT || t.lit == TC_LIT_INT_ZERO || t.lit == TC_LIT_FLOAT) return true;
    return t.ty && (type_is_int(t.ty) || type_is_float(t.ty));
}

static bool tc_is_bool_tc(TcType t) {
    if (t.lit == TC_LIT_BOOL) return true;
    return t.ty && type_is_bool(t.ty);
}

static bool tc_is_char_tc(TcType t) {
    return t.ty && type_is_char(t.ty);
}

static bool tc_is_ptr_tc(TcType t) {
    return t.ty && type_is_ptr(t.ty);
}

static bool tc_is_nullish_tc(TcType t) {
    return t.lit == TC_LIT_NULL || t.lit == TC_LIT_INT_ZERO;
}

static bool tc_is_intish(TcType t) {
    return t.lit == TC_LIT_INT || t.lit == TC_LIT_INT_ZERO || (t.ty && type_is_int(t.ty));
}

static TcType tctype(const EmpType *t) {
    TcType o;
    o.ty = t;
    o.lit = TC_LIT_NONE;
    return o;
}

static bool can_coerce(const TcType src, const EmpType *dst) {
    if (type_is_void(dst)) return false;
    if (dst && dst->kind == EMP_TYPE_AUTO) return true;
    if (src.ty && src.ty->kind == EMP_TYPE_AUTO) return true;
    if (type_eq_shallow(src.ty, dst)) return true;

    // null -> any pointer
    if (src.lit == TC_LIT_NULL && type_is_ptr(dst)) return true;
    if (src.lit == TC_LIT_INT_ZERO && type_is_ptr(dst)) return true;

    // string literal -> *u8
    if (src.lit == TC_LIT_STRING && type_is_str(dst)) return true;

    // string literal -> string (owned)
    if (src.lit == TC_LIT_STRING && type_is_string(dst)) return true;

    // int literal -> any integer or float
    if (src.lit == TC_LIT_INT || src.lit == TC_LIT_INT_ZERO) {
        if (type_is_int(dst) || type_is_float(dst)) return true;
    }

    // float literal -> any float
    if (src.lit == TC_LIT_FLOAT) {
        if (type_is_float(dst)) return true;
    }

    // int -> int (implicit for now)
    if (src.ty && dst && type_is_int(src.ty) && type_is_int(dst)) return true;

    // float -> float
    if (src.ty && dst && type_is_float(src.ty) && type_is_float(dst)) return true;

    // int -> float (implicit for now)
    if (src.ty && dst && type_is_int(src.ty) && type_is_float(dst)) return true;

    // ptr -> ptr (opaque)
    if (src.ty && dst && type_is_ptr(src.ty) && type_is_ptr(dst)) return true;

    return false;
}

typedef struct TcBind {
    EmpSlice name;
    EmpType *ty; // owned by arena
} TcBind;

typedef struct TcEnv {
    TcBind *items;
    size_t len;
    size_t cap;
} TcEnv;

static void env_free(TcEnv *e) {
    free(e->items);
    memset(e, 0, sizeof(*e));
}

static EmpType *env_lookup(TcEnv *e, EmpSlice name) {
    for (size_t i = e->len; i > 0; i--) {
        TcBind *b = &e->items[i - 1];
        if (slice_eq(b->name, name)) return b->ty;
    }
    return NULL;
}

static bool env_push(TcEnv *e, EmpSlice name, EmpType *ty) {
    if (e->len + 1 > e->cap) {
        size_t nc = e->cap ? e->cap * 2 : 64;
        TcBind *p = (TcBind *)realloc(e->items, nc * sizeof(TcBind));
        if (!p) return false;
        e->items = p;
        e->cap = nc;
    }
    e->items[e->len].name = name;
    e->items[e->len].ty = ty;
    e->len++;
    return true;
}

static const EmpExpr *unwrap_group_expr(const EmpExpr *e) {
    while (e && e->kind == EMP_EXPR_GROUP) e = e->as.group.inner;
    return e;
}

static EmpExpr *make_ident_expr(EmpArena *arena, EmpSpan span, const char *name) {
    if (!arena || !name) return NULL;
    EmpExpr *e = (EmpExpr *)emp_arena_alloc(arena, sizeof(EmpExpr), _Alignof(EmpExpr));
    if (!e) return NULL;
    memset(e, 0, sizeof(*e));
    e->kind = EMP_EXPR_IDENT;
    e->span = span;
    e->as.lit = slice_from_cstr(name);
    return e;
}

static EmpExpr *make_borrow_mut_ident_expr(EmpArena *arena, EmpSpan span, EmpSlice ident) {
    if (!arena || !ident.ptr || !ident.len) return NULL;

    EmpExpr *id = (EmpExpr *)emp_arena_alloc(arena, sizeof(EmpExpr), _Alignof(EmpExpr));
    if (!id) return NULL;
    memset(id, 0, sizeof(*id));
    id->kind = EMP_EXPR_IDENT;
    id->span = span;
    id->as.lit = ident;

    EmpExpr *u = (EmpExpr *)emp_arena_alloc(arena, sizeof(EmpExpr), _Alignof(EmpExpr));
    if (!u) return NULL;
    memset(u, 0, sizeof(*u));
    u->kind = EMP_EXPR_UNARY;
    u->span = span;
    u->as.unary.op = EMP_UN_BORROW_MUT;
    u->as.unary.rhs = id;
    return u;
}

static EmpExpr *make_borrow_ident_expr(EmpArena *arena, EmpSpan span, EmpSlice ident) {
    if (!arena || !ident.ptr || !ident.len) return NULL;

    EmpExpr *id = (EmpExpr *)emp_arena_alloc(arena, sizeof(EmpExpr), _Alignof(EmpExpr));
    if (!id) return NULL;
    memset(id, 0, sizeof(*id));
    id->kind = EMP_EXPR_IDENT;
    id->span = span;
    id->as.lit = ident;

    EmpExpr *u = (EmpExpr *)emp_arena_alloc(arena, sizeof(EmpExpr), _Alignof(EmpExpr));
    if (!u) return NULL;
    memset(u, 0, sizeof(*u));
    u->kind = EMP_EXPR_UNARY;
    u->span = span;
    u->as.unary.op = EMP_UN_BORROW;
    u->as.unary.rhs = id;
    return u;
}

static EmpExpr *make_int_expr(EmpArena *arena, EmpSpan span, const char *lit) {
    if (!arena || !lit) return NULL;
    EmpExpr *e = (EmpExpr *)emp_arena_alloc(arena, sizeof(EmpExpr), _Alignof(EmpExpr));
    if (!e) return NULL;
    memset(e, 0, sizeof(*e));
    e->kind = EMP_EXPR_INT;
    e->span = span;
    e->as.lit = slice_from_cstr(lit);
    return e;
}

typedef struct TcFnSig {
    EmpSlice name;
    EmpType **params; // heap-owned
    size_t params_len;
    EmpType *ret; // NULL means void
    EmpSpan span;
    EmpItem *decl; // owning declaration for this overload
} TcFnSig;

typedef struct TcFns {
    TcFnSig *items;
    size_t len;
    size_t cap;
} TcFns;

static void fns_free(TcFns *f) {
    if (!f) return;
    for (size_t i = 0; i < f->len; i++) {
        free(f->items[i].params);
    }
    free(f->items);
    memset(f, 0, sizeof(*f));
}

static TcFnSig *fns_find(TcFns *f, EmpSlice name) {
    for (size_t i = 0; i < f->len; i++) {
        if (slice_eq(f->items[i].name, name)) return &f->items[i];
    }
    return NULL;
}

static size_t fns_count_name(const TcFns *f, EmpSlice name) {
    if (!f) return 0;
    size_t n = 0;
    for (size_t i = 0; i < f->len; i++) {
        if (slice_eq(f->items[i].name, name)) n++;
    }
    return n;
}

static bool fns_has_exact_overload(const TcFns *f, EmpSlice name, EmpType **params, size_t params_len, const EmpType *ret) {
    if (!f) return false;
    for (size_t i = 0; i < f->len; i++) {
        const TcFnSig *s = &f->items[i];
        if (!slice_eq(s->name, name)) continue;
        if (s->params_len != params_len) continue;
        bool ok = true;
        for (size_t j = 0; j < params_len; j++) {
            if (!type_eq_shallow(s->params[j], params[j])) {
                ok = false;
                break;
            }
        }
        if (!ok) continue;
        // Return type must also match for a true duplicate overload.
        if (!type_eq_shallow(s->ret, (EmpType *)ret)) continue;
        return true;
    }
    return false;
}

static TcFnSig *fns_find_by_decl(TcFns *f, const EmpItem *decl) {
    if (!f || !decl) return NULL;
    for (size_t i = 0; i < f->len; i++) {
        if (f->items[i].decl == decl) return &f->items[i];
    }
    return NULL;
}

// Forward decl: used by overload resolution helpers.
static bool tc_check_tuple_literal_against_type(EmpArena *arena, EmpDiags *diags, TcFns *fns, TcEnv *env, const EmpExpr *tuple_lit, const EmpType *expected, bool lenient, bool *io_changed);

static bool tc_arg_compatible(EmpArena *arena, EmpDiags *diags, TcFns *fns, TcEnv *env, TcType actual, const EmpExpr *arg_expr, EmpType *expected, bool lenient, bool *io_changed) {
    if (!expected) return false;

    bool ok = can_coerce(actual, expected);
    if (!ok && expected->kind == EMP_TYPE_TUPLE && arg_expr && arg_expr->kind == EMP_EXPR_TUPLE) {
        ok = tc_check_tuple_literal_against_type(arena, diags, fns, env, arg_expr, expected, lenient, io_changed);
    }
    return ok;
}

static TcFnSig *tc_resolve_free_overload(
    EmpArena *arena,
    EmpDiags *diags,
    TcFns *fns,
    TcEnv *env,
    EmpExpr *call_expr,
    EmpSlice base_name,
    TcType *arg_types,
    EmpExpr **arg_exprs,
    size_t argc,
    bool lenient,
    bool *io_changed
) {
    if (!fns || !call_expr) return NULL;

    TcFnSig *best = NULL;
    int best_cost = 0;
    bool ambiguous = false;
    bool any_named = false;
    bool any_mm_only = false;

    for (size_t si = 0; si < fns->len; si++) {
        TcFnSig *sig = &fns->items[si];
        if (!slice_eq(sig->name, base_name)) continue;
        any_named = true;

        // Manual-MM-only functions are only callable inside `@emp mm off`.
        if (g_tc_mm_depth == 0 && sig->decl && sig->decl->kind == EMP_ITEM_FN && sig->decl->as.fn.is_mm_only) {
            any_mm_only = true;
            continue;
        }

        if (sig->params_len != argc) continue;

        int cost = 0;
        bool ok = true;
        for (size_t i = 0; i < argc; i++) {
            EmpType *pt = sig->params[i];
            if (!pt) {
                ok = false;
                break;
            }

            if (type_is_auto(pt)) {
                // `auto` can match anything, but should lose to concrete overloads.
                cost += 10;
                continue;
            }

            // Prefer exact matches.
            if (arg_types[i].ty && type_eq_shallow(arg_types[i].ty, pt)) {
                continue;
            }

            if (!tc_arg_compatible(arena, /*diags*/ NULL, fns, env, arg_types[i], arg_exprs ? arg_exprs[i] : NULL, pt, lenient, io_changed)) {
                ok = false;
                break;
            }
            cost += 1;
        }

        if (!ok) continue;

        if (!best || cost < best_cost) {
            best = sig;
            best_cost = cost;
            ambiguous = false;
        } else if (cost == best_cost) {
            ambiguous = true;
        }
    }

    if (!any_named) {
        diagf(arena, diags, call_expr->span, "type: ", "call to unknown function");
        return NULL;
    }
    if (!best) {
        if (any_mm_only && g_tc_mm_depth == 0) {
            diagf(arena, diags, call_expr->span, "type: ", "manual memory function requires @emp mm off");
            return NULL;
        }
        diagf(arena, diags, call_expr->span, "type: ", "no matching overload");
        return NULL;
    }
    if (ambiguous) {
        diagf(arena, diags, call_expr->span, "type: ", "ambiguous overload");
        return NULL;
    }

    return best;
}

static bool method_sig_eq(const EmpVec *a_params, const EmpVec *b_params) {
    if (!a_params || !b_params) return false;
    if (a_params->len != b_params->len) return false;
    for (size_t i = 0; i < a_params->len; i++) {
        const EmpParam *ap = (const EmpParam *)a_params->items[i];
        const EmpParam *bp = (const EmpParam *)b_params->items[i];
        if (!ap || !bp) return false;
        if (!type_eq_shallow(ap->ty, bp->ty)) return false;
    }
    return true;
}

static bool class_method_overridden_by_impl(const EmpProgram *p, EmpSlice recv_name, const EmpClassMethod *cm) {
    if (!p || !cm) return false;
    for (size_t i = 0; i < p->items.len; i++) {
        const EmpItem *it = (const EmpItem *)p->items.items[i];
        if (!it || it->kind != EMP_ITEM_IMPL) continue;
        if (it->as.impl_decl.trait_name.ptr && it->as.impl_decl.trait_name.len) continue;
        if (!slice_eq(it->as.impl_decl.target_name, recv_name)) continue;
        for (size_t j = 0; j < it->as.impl_decl.methods.len; j++) {
            const EmpImplMethod *im = (const EmpImplMethod *)it->as.impl_decl.methods.items[j];
            if (!im) continue;
            if (!slice_eq(im->name, cm->name)) continue;
            if (method_sig_eq(&cm->params, &im->params)) return true;
        }
    }
    return false;
}

static size_t tc_count_method_overloads(const EmpProgram *p, EmpSlice recv_name, EmpSlice method_name) {
    if (!p) return 0;
    size_t n = 0;

    // impl methods count
    for (size_t i = 0; i < p->items.len; i++) {
        const EmpItem *it = (const EmpItem *)p->items.items[i];
        if (!it || it->kind != EMP_ITEM_IMPL) continue;
        if (it->as.impl_decl.trait_name.ptr && it->as.impl_decl.trait_name.len) continue;
        if (!slice_eq(it->as.impl_decl.target_name, recv_name)) continue;
        for (size_t j = 0; j < it->as.impl_decl.methods.len; j++) {
            const EmpImplMethod *m = (const EmpImplMethod *)it->as.impl_decl.methods.items[j];
            if (!m) continue;
            if (slice_eq(m->name, method_name)) n++;
        }
    }

    // class methods that are not overridden by impl
    const EmpItem *decl = find_named_decl((EmpProgram *)p, recv_name);
    if (decl && decl->kind == EMP_ITEM_CLASS) {
        for (size_t i = 0; i < decl->as.class_decl.methods.len; i++) {
            const EmpClassMethod *m = (const EmpClassMethod *)decl->as.class_decl.methods.items[i];
            if (!m || m->is_init) continue;
            if (!slice_eq(m->name, method_name)) continue;
            if (class_method_overridden_by_impl(p, recv_name, m)) continue;
            n++;
        }
    }

    return n;
}

static bool fns_push(TcFns *f, TcFnSig sig) {
    if (f->len + 1 > f->cap) {
        size_t nc = f->cap ? f->cap * 2 : 64;
        TcFnSig *p = (TcFnSig *)realloc(f->items, nc * sizeof(TcFnSig));
        if (!p) return false;
        f->items = p;
        f->cap = nc;
    }
    f->items[f->len++] = sig;
    return true;
}

static bool type_name_is_builtin(EmpSlice s) {
    if (slice_is(s, "bool") || slice_is(s, "char")) return true;
    if (slice_is(s, "string")) return true;
    if (slice_is(s, "f32") || slice_is(s, "f64") || slice_is(s, "float") || slice_is(s, "double")) return true;
    if (type_is_int_name(s)) return true;
    return false;
}

typedef struct TcTypeNames {
    EmpSlice *items;
    size_t len;
    size_t cap;
} TcTypeNames;

static void tnames_free(TcTypeNames *t) {
    free(t->items);
    memset(t, 0, sizeof(*t));
}

static bool tnames_push(TcTypeNames *t, EmpSlice name) {
    if (!name.ptr || !name.len) return true;
    for (size_t i = 0; i < t->len; i++) {
        if (slice_eq(t->items[i], name)) return true;
    }
    if (t->len + 1 > t->cap) {
        size_t nc = t->cap ? t->cap * 2 : 64;
        EmpSlice *p = (EmpSlice *)realloc(t->items, nc * sizeof(EmpSlice));
        if (!p) return false;
        t->items = p;
        t->cap = nc;
    }
    t->items[t->len++] = name;
    return true;
}

static bool tnames_has(const TcTypeNames *t, EmpSlice name) {
    for (size_t i = 0; i < t->len; i++) {
        if (slice_eq(t->items[i], name)) return true;
    }
    return false;
}

static void collect_declared_type_names(const EmpProgram *p, TcTypeNames *out) {
    for (size_t i = 0; i < p->items.len; i++) {
        const EmpItem *it = (const EmpItem *)p->items.items[i];
        if (!it) continue;
        if (it->kind == EMP_ITEM_STRUCT) (void)tnames_push(out, it->as.struct_decl.name);
        if (it->kind == EMP_ITEM_ENUM) (void)tnames_push(out, it->as.enum_decl.name);
        if (it->kind == EMP_ITEM_CLASS) (void)tnames_push(out, it->as.class_decl.name);
        if (it->kind == EMP_ITEM_TRAIT) (void)tnames_push(out, it->as.trait_decl.name);
    }
}

static void validate_type_names_in_type(EmpArena *arena, EmpDiags *diags, const TcTypeNames *decls, const EmpType *t) {
    if (!t) return;

    switch (t->kind) {
        case EMP_TYPE_AUTO:
            return;
        case EMP_TYPE_NAME:
            if (type_name_is_builtin(t->as.name)) return;
            if (!tnames_has(decls, t->as.name)) {
                diagf(arena, diags, t->span, "type: ", "unknown type name");
            }
            return;
        case EMP_TYPE_PTR:
            validate_type_names_in_type(arena, diags, decls, t->as.ptr.pointee);
            return;
        case EMP_TYPE_ARRAY:
        case EMP_TYPE_LIST:
            validate_type_names_in_type(arena, diags, decls, t->as.array.elem);
            return;
        case EMP_TYPE_TUPLE:
            for (size_t i = 0; i < t->as.tuple.fields.len; i++) {
                const EmpTupleField *f = (const EmpTupleField *)t->as.tuple.fields.items[i];
                if (f) validate_type_names_in_type(arena, diags, decls, f->ty);
            }
            return;
        default:
            return;
    }
}

static void validate_type_names_in_stmt(EmpArena *arena, EmpDiags *diags, const TcTypeNames *decls, const EmpStmt *s);

static void validate_type_names_in_expr(EmpArena *arena, EmpDiags *diags, const TcTypeNames *decls, const EmpExpr *e) {
    if (!e) return;
    switch (e->kind) {
        case EMP_EXPR_CAST:
            validate_type_names_in_type(arena, diags, decls, e->as.cast.ty);
            validate_type_names_in_expr(arena, diags, decls, e->as.cast.expr);
            return;
        case EMP_EXPR_UNARY:
            validate_type_names_in_expr(arena, diags, decls, e->as.unary.rhs);
            return;
        case EMP_EXPR_BINARY:
            validate_type_names_in_expr(arena, diags, decls, e->as.binary.lhs);
            validate_type_names_in_expr(arena, diags, decls, e->as.binary.rhs);
            return;
        case EMP_EXPR_CALL:
            validate_type_names_in_expr(arena, diags, decls, e->as.call.callee);
            for (size_t i = 0; i < e->as.call.args.len; i++) {
                validate_type_names_in_expr(arena, diags, decls, (const EmpExpr *)e->as.call.args.items[i]);
            }
            return;
        case EMP_EXPR_GROUP:
            validate_type_names_in_expr(arena, diags, decls, e->as.group.inner);
            return;
        case EMP_EXPR_TUPLE:
            for (size_t i = 0; i < e->as.tuple.items.len; i++) {
                validate_type_names_in_expr(arena, diags, decls, (const EmpExpr *)e->as.tuple.items.items[i]);
            }
            return;
        case EMP_EXPR_LIST:
            for (size_t i = 0; i < e->as.list.items.len; i++) {
                validate_type_names_in_expr(arena, diags, decls, (const EmpExpr *)e->as.list.items.items[i]);
            }
            return;
        case EMP_EXPR_TERNARY:
            validate_type_names_in_expr(arena, diags, decls, e->as.ternary.cond);
            validate_type_names_in_expr(arena, diags, decls, e->as.ternary.then_expr);
            validate_type_names_in_expr(arena, diags, decls, e->as.ternary.else_expr);
            return;
        case EMP_EXPR_INDEX:
            validate_type_names_in_expr(arena, diags, decls, e->as.index.base);
            validate_type_names_in_expr(arena, diags, decls, e->as.index.index);
            return;
        case EMP_EXPR_MEMBER:
            validate_type_names_in_expr(arena, diags, decls, e->as.member.base);
            return;
        case EMP_EXPR_NEW:
            // New class name is a slice; validate against declared types.
            if (!tnames_has(decls, e->as.new_expr.class_name)) {
                diagf(arena, diags, e->span, "type: ", "unknown class name");
            }
            for (size_t i = 0; i < e->as.new_expr.args.len; i++) {
                validate_type_names_in_expr(arena, diags, decls, (const EmpExpr *)e->as.new_expr.args.items[i]);
            }
            return;
        case EMP_EXPR_FSTRING:
            for (size_t i = 0; i < e->as.fstring.parts.len; i++) {
                const EmpFStringPart *p = (const EmpFStringPart *)e->as.fstring.parts.items[i];
                if (!p) continue;
                if (p->is_expr) validate_type_names_in_expr(arena, diags, decls, p->expr);
            }
            return;
        default:
            return;
    }
}

static void validate_type_names_in_stmt(EmpArena *arena, EmpDiags *diags, const TcTypeNames *decls, const EmpStmt *s) {
    if (!s) return;
    switch (s->kind) {
        case EMP_STMT_TAG:
            return;
        case EMP_STMT_VAR:
            validate_type_names_in_type(arena, diags, decls, s->as.let_stmt.ty);
            validate_type_names_in_expr(arena, diags, decls, s->as.let_stmt.init);
            return;
        case EMP_STMT_RETURN:
            validate_type_names_in_expr(arena, diags, decls, s->as.ret.value);
            return;
        case EMP_STMT_EXPR:
            validate_type_names_in_expr(arena, diags, decls, s->as.expr.expr);
            return;
        case EMP_STMT_BLOCK:
            for (size_t i = 0; i < s->as.block.stmts.len; i++) {
                validate_type_names_in_stmt(arena, diags, decls, (const EmpStmt *)s->as.block.stmts.items[i]);
            }
            return;
        case EMP_STMT_IF:
            validate_type_names_in_expr(arena, diags, decls, s->as.if_stmt.cond);
            validate_type_names_in_stmt(arena, diags, decls, s->as.if_stmt.then_branch);
            validate_type_names_in_stmt(arena, diags, decls, s->as.if_stmt.else_branch);
            return;
        case EMP_STMT_WHILE:
            validate_type_names_in_expr(arena, diags, decls, s->as.while_stmt.cond);
            validate_type_names_in_stmt(arena, diags, decls, s->as.while_stmt.body);
            return;
        case EMP_STMT_FOR:
            validate_type_names_in_expr(arena, diags, decls, s->as.for_stmt.iterable);
            validate_type_names_in_stmt(arena, diags, decls, s->as.for_stmt.body);
            return;
        case EMP_STMT_BREAK:
        case EMP_STMT_CONTINUE:
            return;
        case EMP_STMT_MATCH:
            validate_type_names_in_expr(arena, diags, decls, s->as.match_stmt.scrutinee);
            for (size_t i = 0; i < s->as.match_stmt.arms.len; i++) {
                const EmpMatchArm *a = (const EmpMatchArm *)s->as.match_stmt.arms.items[i];
                if (!a) continue;
                validate_type_names_in_expr(arena, diags, decls, a->pat);
                validate_type_names_in_stmt(arena, diags, decls, a->body);
            }
            return;
        case EMP_STMT_EMP_OFF:
            validate_type_names_in_stmt(arena, diags, decls, s->as.emp_off.body);
            return;
        case EMP_STMT_EMP_MM_OFF:
            validate_type_names_in_stmt(arena, diags, decls, s->as.emp_mm_off.body);
            return;
        default:
            return;
    }
}

static void validate_type_names_in_program(EmpArena *arena, EmpDiags *diags, EmpProgram *p) {
    TcTypeNames decls;
    memset(&decls, 0, sizeof(decls));
    collect_declared_type_names(p, &decls);

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
                    EmpParam *pa = (EmpParam *)it->as.fn.params.items[j];
                    if (pa) validate_type_names_in_type(arena, diags, &decls, pa->ty);
                }
                validate_type_names_in_type(arena, diags, &decls, it->as.fn.ret_ty);
                validate_type_names_in_stmt(arena, diags, &decls, it->as.fn.body);
                break;
            case EMP_ITEM_CONST:
                validate_type_names_in_type(arena, diags, &decls, it->as.const_decl.ty);
                validate_type_names_in_expr(arena, diags, &decls, it->as.const_decl.init);
                break;
            case EMP_ITEM_STRUCT:
                for (size_t j = 0; j < it->as.struct_decl.fields.len; j++) {
                    const EmpStructField *f = (const EmpStructField *)it->as.struct_decl.fields.items[j];
                    if (f) validate_type_names_in_type(arena, diags, &decls, f->ty);
                }
                break;
            case EMP_ITEM_ENUM:
                for (size_t vi = 0; vi < it->as.enum_decl.variants.len; vi++) {
                    const EmpEnumVariant *v = (const EmpEnumVariant *)it->as.enum_decl.variants.items[vi];
                    if (!v) continue;
                    for (size_t fi = 0; fi < v->fields.len; fi++) {
                        validate_type_names_in_type(arena, diags, &decls, (const EmpType *)v->fields.items[fi]);
                    }
                }
                break;
            case EMP_ITEM_CLASS:
                if (it->as.class_decl.base_name.len && !tnames_has(&decls, it->as.class_decl.base_name)) {
                    diagf(arena, diags, it->as.class_decl.span, "type: ", "unknown base class name");
                }
                for (size_t j = 0; j < it->as.class_decl.fields.len; j++) {
                    const EmpClassField *f = (const EmpClassField *)it->as.class_decl.fields.items[j];
                    if (f) validate_type_names_in_type(arena, diags, &decls, f->ty);
                }
                for (size_t mi = 0; mi < it->as.class_decl.methods.len; mi++) {
                    const EmpClassMethod *m = (const EmpClassMethod *)it->as.class_decl.methods.items[mi];
                    if (!m) continue;
                    for (size_t j = 0; j < m->params.len; j++) {
                        EmpParam *pa = (EmpParam *)m->params.items[j];
                        if (pa) validate_type_names_in_type(arena, diags, &decls, pa->ty);
                    }
                    validate_type_names_in_type(arena, diags, &decls, m->ret_ty);
                    validate_type_names_in_stmt(arena, diags, &decls, m->body);
                }
                break;
            case EMP_ITEM_TRAIT:
                for (size_t mi = 0; mi < it->as.trait_decl.methods.len; mi++) {
                    const EmpTraitMethod *m = (const EmpTraitMethod *)it->as.trait_decl.methods.items[mi];
                    if (!m) continue;
                    for (size_t j = 0; j < m->params.len; j++) {
                        EmpParam *pa = (EmpParam *)m->params.items[j];
                        if (pa) validate_type_names_in_type(arena, diags, &decls, pa->ty);
                    }
                    validate_type_names_in_type(arena, diags, &decls, m->ret_ty);
                    validate_type_names_in_stmt(arena, diags, &decls, m->body);
                }
                break;
            case EMP_ITEM_IMPL:
                // Best-effort: ensure target exists.
                if (it->as.impl_decl.target_name.len && !tnames_has(&decls, it->as.impl_decl.target_name)) {
                    diagf(arena, diags, it->as.impl_decl.span, "type: ", "unknown impl target type");
                }
                if (it->as.impl_decl.trait_name.ptr && it->as.impl_decl.trait_name.len) {
                    const EmpItem *tr = find_trait_decl(p, it->as.impl_decl.trait_name);
                    if (!tr) {
                        diagf(arena, diags, it->as.impl_decl.span, "type: ", "unknown impl trait");
                    }
                }
                for (size_t mi = 0; mi < it->as.impl_decl.methods.len; mi++) {
                    const EmpImplMethod *m = (const EmpImplMethod *)it->as.impl_decl.methods.items[mi];
                    if (!m) continue;
                    for (size_t j = 0; j < m->params.len; j++) {
                        EmpParam *pa = (EmpParam *)m->params.items[j];
                        if (pa) validate_type_names_in_type(arena, diags, &decls, pa->ty);
                    }
                    validate_type_names_in_type(arena, diags, &decls, m->ret_ty);
                    validate_type_names_in_stmt(arena, diags, &decls, m->body);
                }
                break;
            default:
                break;
        }
    }

    tnames_free(&decls);
}

static TcType tc_expr_expected(EmpArena *arena, EmpDiags *diags, TcFns *fns, TcEnv *env, EmpExpr *e, const EmpType *expected, bool lenient, bool *io_changed);
static TcType tc_expr(EmpArena *arena, EmpDiags *diags, TcFns *fns, TcEnv *env, EmpExpr *e, bool lenient, bool *io_changed);

static bool tc_check_tuple_literal_against_type(EmpArena *arena, EmpDiags *diags, TcFns *fns, TcEnv *env, const EmpExpr *tuple_lit, const EmpType *expected, bool lenient, bool *io_changed) {
    if (!tuple_lit || tuple_lit->kind != EMP_EXPR_TUPLE) return false;
    if (!expected || expected->kind != EMP_TYPE_TUPLE) return false;

    const size_t n_items = tuple_lit->as.tuple.items.len;
    const size_t n_fields = expected->as.tuple.fields.len;
    if (n_items != n_fields) {
        diagf(arena, diags, tuple_lit->span, "type: ", "tuple literal arity mismatch");
        return false;
    }

    bool ok = true;
    for (size_t i = 0; i < n_items; i++) {
        const EmpExpr *it = (const EmpExpr *)tuple_lit->as.tuple.items.items[i];
        const EmpTupleField *f = (const EmpTupleField *)expected->as.tuple.fields.items[i];
        if (!it || !f || !f->ty) continue;

        if (it->kind == EMP_EXPR_TUPLE && f->ty->kind == EMP_TYPE_TUPLE) {
            if (!tc_check_tuple_literal_against_type(arena, diags, fns, env, it, f->ty, lenient, io_changed)) ok = false;
            continue;
        }

        TcType tt = tc_expr(arena, diags, fns, env, (EmpExpr *)it, lenient, io_changed);
        if (!tt.ty) {
            ok = false;
            continue;
        }

        if (!can_coerce(tt, f->ty)) {
            diagf(arena, diags, it->span, "type: ", "tuple element initializer type mismatch");
            ok = false;
        }
    }
    return ok;
}

static TcType tc_binary_numeric(EmpArena *arena, EmpDiags *diags, EmpSpan span, EmpBinOp op, TcType a, TcType b, bool lenient) {
    (void)op;

    // Handle literal -> numeric coercions and pick a result type.
    const EmpType *ta = a.ty;
    const EmpType *tb = b.ty;

    bool a_int = (a.lit == TC_LIT_INT) || (ta && type_is_int(ta));
    bool b_int = (b.lit == TC_LIT_INT) || (tb && type_is_int(tb));

    bool a_float = (a.lit == TC_LIT_FLOAT) || (ta && type_is_float(ta));
    bool b_float = (b.lit == TC_LIT_FLOAT) || (tb && type_is_float(tb));

    if ((a_int || a_float) && (b_int || b_float)) {
        // If any float, promote to f64 for now.
        if (a_float || b_float) {
            return (TcType){ .ty = make_named(arena, span, "f64"), .lit = TC_LIT_NONE };
        }
        // integers: default to i32 for now.
        return (TcType){ .ty = make_named(arena, span, "i32"), .lit = TC_LIT_NONE };
    }

    if (lenient && (type_is_auto(ta) || type_is_auto(tb))) {
        return (TcType){ .ty = make_auto(arena, span), .lit = TC_LIT_NONE };
    }

    diagf(arena, diags, span, "type: ", "binary operator expects numeric operands");
    return (TcType){0};
}

static TcType tc_expr(EmpArena *arena, EmpDiags *diags, TcFns *fns, TcEnv *env, EmpExpr *e, bool lenient, bool *io_changed) {
    return tc_expr_expected(arena, diags, fns, env, e, NULL, lenient, io_changed);
}

static TcType tc_expr_expected(EmpArena *arena, EmpDiags *diags, TcFns *fns, TcEnv *env, EmpExpr *e, const EmpType *expected, bool lenient, bool *io_changed) {
    if (!e) return (TcType){0};

    switch (e->kind) {
        case EMP_EXPR_INT: {
            // Detect zero-valued int literals so they can also coerce to pointers (C-style null).
            bool is_zero = false;
            if (e->as.lit.ptr && e->as.lit.len) {
                const char *p = e->as.lit.ptr;
                size_t n = e->as.lit.len;
                size_t i = 0;
                int base = 10;
                if (n >= 2 && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
                    base = 16;
                    i = 2;
                } else if (n >= 2 && p[0] == '0' && (p[1] == 'b' || p[1] == 'B')) {
                    base = 2;
                    i = 2;
                } else if (n >= 2 && p[0] == '0' && (p[1] == 'o' || p[1] == 'O')) {
                    base = 8;
                    i = 2;
                }

                bool saw_digit = false;
                bool any_nonzero = false;
                for (; i < n; i++) {
                    char ch = p[i];
                    if (ch == '_') continue;
                    int v = -1;
                    if (ch >= '0' && ch <= '9') v = (int)(ch - '0');
                    else if (base == 16 && ch >= 'a' && ch <= 'f') v = 10 + (int)(ch - 'a');
                    else if (base == 16 && ch >= 'A' && ch <= 'F') v = 10 + (int)(ch - 'A');
                    else v = -1;
                    if (v < 0 || v >= base) continue;
                    saw_digit = true;
                    if (v != 0) {
                        any_nonzero = true;
                        break;
                    }
                }
                is_zero = saw_digit && !any_nonzero;
            }

            TcType t;
            t.ty = make_named(arena, e->span, "i32");
            t.lit = is_zero ? TC_LIT_INT_ZERO : TC_LIT_INT;
            return t;
        }

        case EMP_EXPR_FLOAT: {
            TcType t;
            t.ty = make_named(arena, e->span, "f64");
            t.lit = TC_LIT_FLOAT;
            return t;
        }

        case EMP_EXPR_CHAR:
            return (TcType){ .ty = make_named(arena, e->span, "char"), .lit = TC_LIT_NONE };

        case EMP_EXPR_STRING: {
            // C string literal type: *u8
            EmpType *u8t = make_named(arena, e->span, "u8");
            EmpType *pt = make_ptr(arena, e->span, u8t);
            return (TcType){ .ty = pt, .lit = TC_LIT_STRING };
        }

        case EMP_EXPR_FSTRING:
            // f-string type: string (owned heap string). Interpolations are formatted at runtime.
            for (size_t i = 0; i < e->as.fstring.parts.len; i++) {
                const EmpFStringPart *pt = (const EmpFStringPart *)e->as.fstring.parts.items[i];
                if (!pt || !pt->is_expr) continue;

                TcType tt = tc_expr(arena, diags, fns, env, pt->expr, lenient, io_changed);
                if (!tt.ty) {
                    if (!lenient) {
                        diagf(arena, diags, pt->span, "type: ", "f-string interpolation expression must not be void");
                    }
                    continue;
                }

                // MVP formatting support: string/*u8, integers, bool, char.
                if (!(type_is_stringlike(tt.ty) || type_is_int(tt.ty) || type_is_bool(tt.ty) || type_is_char(tt.ty))) {
                    if (!(lenient && type_is_auto(tt.ty))) {
                        diagf(arena, diags, pt->span, "type: ", "unsupported f-string interpolation type (supported: string, *u8, int, bool, char)");
                    }
                }
            }

            return (TcType){ .ty = make_named(arena, e->span, "string"), .lit = TC_LIT_NONE };

        case EMP_EXPR_LIST: {
            // Infer list element type from items, or from an expected list type when provided.
            EmpType *expected_elem = NULL;
            if (expected && expected->kind == EMP_TYPE_LIST) {
                expected_elem = expected->as.array.elem;
            }

            EmpType *elem_ty = NULL;
            if (e->as.list.items.len == 0) {
                if (expected_elem) {
                    // If we have an expected list type (including `auto[]`), accept `[]` and
                    // defer any element-type inference until later (e.g. first push/append).
                    elem_ty = expected_elem;
                } else {
                    if (!lenient) {
                        diagf(arena, diags, e->span, "type: ", "cannot infer type for empty list literal; add an explicit type annotation");
                    }
                    elem_ty = make_auto(arena, e->span);
                }
            } else {
                if (expected_elem && !type_is_auto(expected_elem)) {
                    elem_ty = expected_elem;
                } else {
                    const EmpExpr *first = (const EmpExpr *)e->as.list.items.items[0];
                    TcType ft = tc_expr_expected(arena, diags, fns, env, (EmpExpr *)first, NULL, lenient, io_changed);
                    if (!ft.ty) return (TcType){0};
                    elem_ty = (EmpType *)ft.ty;
                }

                for (size_t i = 1; i < e->as.list.items.len; i++) {
                    const EmpExpr *it = (const EmpExpr *)e->as.list.items.items[i];
                    TcType tt = tc_expr_expected(arena, diags, fns, env, (EmpExpr *)it, elem_ty, lenient, io_changed);
                    if (!tt.ty) continue;
                    if (!can_coerce(tt, elem_ty)) {
                        diagf(arena, diags, it ? it->span : e->span, "type: ", "list literal element type mismatch");
                    }
                }
            }

            if (expected && expected->kind == EMP_TYPE_LIST && elem_ty && !type_is_auto(elem_ty)) {
                return (TcType){ .ty = expected, .lit = TC_LIT_NONE };
            }

            EmpType *lt = make_list(arena, e->span, elem_ty);
            return (TcType){ .ty = lt, .lit = TC_LIT_NONE };
        }

        case EMP_EXPR_TUPLE: {
            EmpType *tt = make_tuple(arena, e->span);
            if (!tt) return (TcType){0};

            for (size_t i = 0; i < e->as.tuple.items.len; i++) {
                const EmpExpr *it = (const EmpExpr *)e->as.tuple.items.items[i];
                TcType et = tc_expr(arena, diags, fns, env, (EmpExpr *)it, lenient, io_changed);
                if (!et.ty) {
                    if (!lenient) {
                        diagf(arena, diags, it ? it->span : e->span, "type: ", "tuple element must not be void");
                    }
                    et.ty = make_auto(arena, it ? it->span : e->span);
                }

                EmpTupleField *f = (EmpTupleField *)emp_arena_alloc(arena, sizeof(EmpTupleField), _Alignof(EmpTupleField));
                if (!f) continue;
                memset(f, 0, sizeof(*f));
                f->ty = (EmpType *)et.ty;
                f->name.ptr = NULL;
                f->name.len = 0;
                f->span = it ? it->span : e->span;
                (void)emp_vec_push(&tt->as.tuple.fields, f);
            }

            return (TcType){ .ty = tt, .lit = TC_LIT_NONE };
        }

        case EMP_EXPR_TERNARY: {
            TcType cnd = tc_expr(arena, diags, fns, env, e->as.ternary.cond, lenient, io_changed);
            if (cnd.ty && !type_is_bool(cnd.ty) && !type_is_int(cnd.ty) && cnd.lit != TC_LIT_BOOL && cnd.lit != TC_LIT_INT && cnd.lit != TC_LIT_INT_ZERO) {
                if (!(lenient && type_is_auto(cnd.ty))) {
                    diagf(arena, diags, e->as.ternary.cond ? e->as.ternary.cond->span : e->span, "type: ", "ternary condition must be bool");
                }
            }

            TcType tt = tc_expr(arena, diags, fns, env, e->as.ternary.then_expr, lenient, io_changed);
            TcType ft = tc_expr(arena, diags, fns, env, e->as.ternary.else_expr, lenient, io_changed);
            if (!tt.ty || !ft.ty) return (TcType){0};

            if (lenient && (type_is_auto(tt.ty) || type_is_auto(ft.ty))) {
                return (TcType){ .ty = make_auto(arena, e->span), .lit = TC_LIT_NONE };
            }

            bool t_int = (tt.lit == TC_LIT_INT || tt.lit == TC_LIT_INT_ZERO) || (tt.ty && type_is_int(tt.ty));
            bool f_int = (ft.lit == TC_LIT_INT || ft.lit == TC_LIT_INT_ZERO) || (ft.ty && type_is_int(ft.ty));
            bool t_float = (tt.lit == TC_LIT_FLOAT) || (tt.ty && type_is_float(tt.ty));
            bool f_float = (ft.lit == TC_LIT_FLOAT) || (ft.ty && type_is_float(ft.ty));

            if ((t_int || t_float) && (f_int || f_float)) {
                // Promote numeric ternary branches.
                if (t_float || f_float) return (TcType){ .ty = make_named(arena, e->span, "f64"), .lit = TC_LIT_NONE };
                return (TcType){ .ty = make_named(arena, e->span, "i32"), .lit = TC_LIT_NONE };
            }

            if (can_coerce(tt, (EmpType *)ft.ty)) return (TcType){ .ty = ft.ty, .lit = TC_LIT_NONE };
            if (can_coerce(ft, (EmpType *)tt.ty)) return (TcType){ .ty = tt.ty, .lit = TC_LIT_NONE };

            diagf(arena, diags, e->span, "type: ", "ternary branches must have compatible types");
            return (TcType){ .ty = tt.ty, .lit = TC_LIT_NONE };
        }

        case EMP_EXPR_RANGE: {
            // Range endpoints are still typechecked so errors point at them.
            TcType st = tc_expr(arena, diags, fns, env, e->as.range.start, lenient, io_changed);
            TcType en = tc_expr(arena, diags, fns, env, e->as.range.end, lenient, io_changed);

            bool st_int = (st.lit == TC_LIT_INT) || (st.lit == TC_LIT_INT_ZERO) || (st.ty && type_is_int(st.ty));
            bool en_int = (en.lit == TC_LIT_INT) || (en.lit == TC_LIT_INT_ZERO) || (en.ty && type_is_int(en.ty));

            if (st.ty && !st_int && !(lenient && type_is_auto(st.ty))) {
                diagf(arena, diags, e->as.range.start ? e->as.range.start->span : e->span, "type: ", "range start must be an int");
            }
            if (en.ty && !en_int && !(lenient && type_is_auto(en.ty))) {
                diagf(arena, diags, e->as.range.end ? e->as.range.end->span : e->span, "type: ", "range end must be an int");
            }

            diagf(arena, diags, e->span, "type: ", "range expressions are only supported in for/in for now");
            return (TcType){0};
        }

        case EMP_EXPR_IDENT: {
            // builtin ident-like literals
            if (slice_is(e->as.lit, "true") || slice_is(e->as.lit, "false")) {
                return (TcType){ .ty = make_named(arena, e->span, "bool"), .lit = TC_LIT_BOOL };
            }
            if (slice_is(e->as.lit, "null")) {
                // null literal: can coerce to any pointer
                return (TcType){ .ty = make_ptr(arena, e->span, make_named(arena, e->span, "u8")), .lit = TC_LIT_NULL };
            }

            EmpType *t = env_lookup(env, e->as.lit);
            if (!t) {
                // allow function name as callee only; if referenced as value, error
                if (fns_find(fns, e->as.lit)) {
                    diagf(arena, diags, e->span, "type: ", "function used as a value is not supported yet");
                    return (TcType){0};
                }
                diagf(arena, diags, e->span, "type: ", "unknown identifier");
                return (TcType){0};
            }
            return (TcType){ .ty = t, .lit = TC_LIT_NONE };
        }

        case EMP_EXPR_GROUP:
            return tc_expr(arena, diags, fns, env, e->as.group.inner, lenient, io_changed);

        case EMP_EXPR_CAST: {
            TcType src = tc_expr(arena, diags, fns, env, e->as.cast.expr, lenient, io_changed);
            const EmpType *dst = e->as.cast.ty;
            if (!dst) return (TcType){0};

            if (lenient && (type_is_auto(src.ty) || type_is_auto(dst))) {
                return (TcType){ .ty = make_auto(arena, e->span), .lit = TC_LIT_NONE };
            }

            // dyn cast: `*Concrete as dyn Base`
            if (dst->kind == EMP_TYPE_DYN) {
                if (!g_tc_program) {
                    diagf(arena, diags, e->span, "type: ", "internal error: program not set for typechecking");
                    return (TcType){0};
                }

                if (!src.ty || src.ty->kind != EMP_TYPE_PTR || !src.ty->as.ptr.pointee || src.ty->as.ptr.pointee->kind != EMP_TYPE_NAME) {
                    diagf(arena, diags, e->span, "type: ", "dyn cast expects a pointer to a class (use &obj or &mut obj)");
                    return (TcType){0};
                }

                EmpSlice concrete_name = src.ty->as.ptr.pointee->as.name;
                const EmpItem *concrete_decl = find_class_decl(g_tc_program, concrete_name);
                const EmpItem *base_decl = find_class_decl(g_tc_program, dst->as.dyn.base_name);
                if (!concrete_decl) {
                    diagf(arena, diags, e->span, "type: ", "dyn cast source must be a class type");
                    return (TcType){0};
                }
                if (!base_decl) {
                    diagf(arena, diags, e->span, "type: ", "dyn base must be a class type");
                    return (TcType){0};
                }

                // MVP: allow `*Base as dyn Base`, or a direct `class Concrete : Base` relationship.
                if (!slice_eq(concrete_name, dst->as.dyn.base_name)) {
                    if (!concrete_decl->as.class_decl.base_name.len || !slice_eq(concrete_decl->as.class_decl.base_name, dst->as.dyn.base_name)) {
                        diagf(arena, diags, e->span, "type: ", "dyn cast requires `*Base` or `class Concrete : Base` (direct base for MVP)");
                        return (TcType){0};
                    }
                }

                // Layout safety MVP: when casting a derived type to `dyn Base`, require exact overrides
                // for all Base virtual methods so we never dispatch to Base's implementation on a
                // Concrete instance (EMP does not embed Base fields in Concrete yet).
                if (!slice_eq(concrete_name, dst->as.dyn.base_name)) {
                    for (size_t i = 0; i < base_decl->as.class_decl.methods.len; i++) {
                        const EmpClassMethod *vm = (const EmpClassMethod *)base_decl->as.class_decl.methods.items[i];
                        if (!vm || vm->is_init) continue;
                        if (!vm->is_virtual) continue;

                        const EmpImplMethod *im = find_impl_method_exact(g_tc_program, concrete_name, vm);
                        const EmpClassMethod *cm = find_class_method_exact(g_tc_program, concrete_name, vm);
                        if (!im && !cm) {
                            diagf(arena, diags, e->span, "type: ", "dyn cast requires concrete type to override all virtual methods (MVP)");
                            return (TcType){0};
                        }
                    }
                }

                e->as.cast.dyn_concrete_name = concrete_name;

                return (TcType){ .ty = dst, .lit = TC_LIT_NONE };
            }

            // Only numeric casts for now (aside from dyn).
            bool dst_int = type_is_int(dst);
            bool dst_float = type_is_float(dst);
            bool src_int = (src.lit == TC_LIT_INT) || (src.ty && type_is_int(src.ty));
            bool src_float = (src.lit == TC_LIT_FLOAT) || (src.ty && type_is_float(src.ty));

            if ((dst_int || dst_float) && (src_int || src_float)) {
                return (TcType){ .ty = (const EmpType *)dst, .lit = TC_LIT_NONE };
            }

            diagf(arena, diags, e->span, "type: ", "unsupported cast (only int/float and dyn casts supported yet)");
            return (TcType){0};
        }

        case EMP_EXPR_UNARY: {
            TcType rhs = tc_expr(arena, diags, fns, env, e->as.unary.rhs, lenient, io_changed);
            if (!rhs.ty) return (TcType){0};

            if (lenient && type_is_auto(rhs.ty)) {
                return rhs;
            }

            switch (e->as.unary.op) {
                case EMP_UN_NEG:
                    if (!type_is_int(rhs.ty) && !type_is_float(rhs.ty) && rhs.lit != TC_LIT_INT && rhs.lit != TC_LIT_FLOAT) {
                        diagf(arena, diags, e->span, "type: ", "unary '-' expects numeric operand");
                        return (TcType){0};
                    }
                    return (TcType){ .ty = rhs.ty, .lit = TC_LIT_NONE };

                case EMP_UN_BITNOT:
                    if (!type_is_int(rhs.ty) && rhs.lit != TC_LIT_INT) {
                        diagf(arena, diags, e->span, "type: ", "unary '~' expects integer operand");
                        return (TcType){0};
                    }
                    return (TcType){ .ty = rhs.ty, .lit = TC_LIT_NONE };

                case EMP_UN_NOT:
                    if (!type_is_bool(rhs.ty) && rhs.lit != TC_LIT_BOOL) {
                        diagf(arena, diags, e->span, "type: ", "unary '!' expects bool");
                        return (TcType){0};
                    }
                    return (TcType){ .ty = make_named(arena, e->span, "bool"), .lit = TC_LIT_NONE };

                case EMP_UN_BORROW:
                case EMP_UN_BORROW_MUT:
                    return (TcType){ .ty = make_ptr(arena, e->span, (EmpType *)rhs.ty), .lit = TC_LIT_NONE };

                default:
                    return rhs;
            }
        }

        case EMP_EXPR_BINARY: {
            EmpBinOp op = e->as.binary.op;

            // For assignment-like ops, typecheck RHS after determining LHS target type.
            bool assign_like = (op == EMP_BIN_ASSIGN || op == EMP_BIN_ADD_ASSIGN || op == EMP_BIN_SUB_ASSIGN || op == EMP_BIN_MUL_ASSIGN || op == EMP_BIN_DIV_ASSIGN ||
                                op == EMP_BIN_REM_ASSIGN || op == EMP_BIN_SHL_ASSIGN || op == EMP_BIN_SHR_ASSIGN || op == EMP_BIN_BITAND_ASSIGN || op == EMP_BIN_BITOR_ASSIGN || op == EMP_BIN_BITXOR_ASSIGN);

            TcType lhs = tc_expr(arena, diags, fns, env, e->as.binary.lhs, lenient, io_changed);
            if (!lhs.ty) return (TcType){0};

            TcType rhs = {0};
            if (!assign_like) {
                rhs = tc_expr(arena, diags, fns, env, e->as.binary.rhs, lenient, io_changed);
                if (!rhs.ty) return (TcType){0};
            }

            switch (op) {
                case EMP_BIN_ASSIGN:
                case EMP_BIN_ADD_ASSIGN:
                case EMP_BIN_SUB_ASSIGN:
                case EMP_BIN_MUL_ASSIGN:
                case EMP_BIN_DIV_ASSIGN: {
                    // LHS must be an lvalue: ident or array index.
                    EmpType *lhs_ty = NULL;
                    if (e->as.binary.lhs && e->as.binary.lhs->kind == EMP_EXPR_IDENT) {
                        lhs_ty = env_lookup(env, e->as.binary.lhs->as.lit);
                        if (!lhs_ty) {
                            diagf(arena, diags, e->span, "type: ", "assignment to unknown identifier");
                            return (TcType){0};
                        }
                    } else if (e->as.binary.lhs && e->as.binary.lhs->kind == EMP_EXPR_INDEX) {
                        // Support indexing into arrays/lists.
                        TcType bt = tc_expr(arena, diags, fns, env, e->as.binary.lhs->as.index.base, lenient, io_changed);
                        (void)tc_expr(arena, diags, fns, env, e->as.binary.lhs->as.index.index, lenient, io_changed);
                        if (!bt.ty) return (TcType){0};
                        if (bt.ty->kind == EMP_TYPE_ARRAY || bt.ty->kind == EMP_TYPE_LIST) {
                            lhs_ty = bt.ty->as.array.elem;
                        } else if (bt.ty->kind == EMP_TYPE_TUPLE) {
                            const EmpExpr *idxe = e->as.binary.lhs->as.index.index;
                            if (!idxe || idxe->kind != EMP_EXPR_INT) {
                                diagf(arena, diags, e->span, "type: ", "tuple lvalue index must be an integer literal");
                                return (TcType){0};
                            }

                            const char *txt = slice_to_cstr(arena, idxe->as.lit);
                            long long idx = txt ? strtoll(txt, NULL, 10) : -1;
                            if (idx < 0 || (size_t)idx >= bt.ty->as.tuple.fields.len) {
                                diagf(arena, diags, e->span, "type: ", "tuple lvalue index out of bounds");
                                return (TcType){0};
                            }

                            const EmpTupleField *f = (const EmpTupleField *)bt.ty->as.tuple.fields.items[(size_t)idx];
                            lhs_ty = f ? f->ty : NULL;
                            if (!lhs_ty) return (TcType){0};
                        } else {
                            diagf(arena, diags, e->span, "type: ", "index assignment requires array/list base (for now)");
                            return (TcType){0};
                        }
                    } else if (e->as.binary.lhs && e->as.binary.lhs->kind == EMP_EXPR_MEMBER) {
                        // Member assignments: use the already-typechecked member expression type.
                        lhs_ty = (EmpType *)lhs.ty;
                    } else {
                        diagf(arena, diags, e->span, "type: ", "assignment lhs must be an identifier, member, or array/list index (for now)");
                        return (TcType){0};
                    }

                    rhs = tc_expr_expected(arena, diags, fns, env, e->as.binary.rhs, lhs_ty, lenient, io_changed);
                    if (!rhs.ty) return (TcType){0};

                    if (op == EMP_BIN_ASSIGN) {
                        if (!can_coerce(rhs, lhs_ty)) {
                            diagf(arena, diags, e->span, "type: ", "assignment type mismatch");
                        }
                        return (TcType){ .ty = lhs_ty, .lit = TC_LIT_NONE };
                    }

                    // Compound assigns: numeric for now.
                    if (!type_is_int(lhs_ty) && !type_is_float(lhs_ty)) {
                        diagf(arena, diags, e->span, "type: ", "compound assignment requires numeric lhs");
                        return (TcType){0};
                    }
                    if (!can_coerce(rhs, lhs_ty)) {
                        diagf(arena, diags, e->span, "type: ", "compound assignment rhs type mismatch");
                    }
                    return (TcType){ .ty = lhs_ty, .lit = TC_LIT_NONE };
                }

                case EMP_BIN_REM_ASSIGN:
                case EMP_BIN_SHL_ASSIGN:
                case EMP_BIN_SHR_ASSIGN:
                case EMP_BIN_BITAND_ASSIGN:
                case EMP_BIN_BITOR_ASSIGN:
                case EMP_BIN_BITXOR_ASSIGN: {
                    // LHS must be an lvalue: ident or array index.
                    EmpType *lhs_ty = NULL;
                    if (e->as.binary.lhs && e->as.binary.lhs->kind == EMP_EXPR_IDENT) {
                        lhs_ty = env_lookup(env, e->as.binary.lhs->as.lit);
                        if (!lhs_ty) {
                            diagf(arena, diags, e->span, "type: ", "assignment to unknown identifier");
                            return (TcType){0};
                        }
                    } else if (e->as.binary.lhs && e->as.binary.lhs->kind == EMP_EXPR_INDEX) {
                        TcType bt = tc_expr(arena, diags, fns, env, e->as.binary.lhs->as.index.base, lenient, io_changed);
                        (void)tc_expr(arena, diags, fns, env, e->as.binary.lhs->as.index.index, lenient, io_changed);
                        if (!bt.ty) return (TcType){0};
                        if (bt.ty->kind == EMP_TYPE_ARRAY || bt.ty->kind == EMP_TYPE_LIST) {
                            lhs_ty = bt.ty->as.array.elem;
                        } else if (bt.ty->kind == EMP_TYPE_TUPLE) {
                            const EmpExpr *idxe = e->as.binary.lhs->as.index.index;
                            if (!idxe || idxe->kind != EMP_EXPR_INT) {
                                diagf(arena, diags, e->span, "type: ", "tuple lvalue index must be an integer literal");
                                return (TcType){0};
                            }

                            const char *txt = slice_to_cstr(arena, idxe->as.lit);
                            long long idx = txt ? strtoll(txt, NULL, 10) : -1;
                            if (idx < 0 || (size_t)idx >= bt.ty->as.tuple.fields.len) {
                                diagf(arena, diags, e->span, "type: ", "tuple lvalue index out of bounds");
                                return (TcType){0};
                            }

                            const EmpTupleField *f = (const EmpTupleField *)bt.ty->as.tuple.fields.items[(size_t)idx];
                            lhs_ty = f ? f->ty : NULL;
                            if (!lhs_ty) return (TcType){0};
                        } else {
                            diagf(arena, diags, e->span, "type: ", "index assignment requires array/list base (for now)");
                            return (TcType){0};
                        }
                    } else if (e->as.binary.lhs && e->as.binary.lhs->kind == EMP_EXPR_MEMBER) {
                        lhs_ty = (EmpType *)lhs.ty;
                    } else {
                        diagf(arena, diags, e->span, "type: ", "assignment lhs must be an identifier, member, or array/list index (for now)");
                        return (TcType){0};
                    }

                    rhs = tc_expr_expected(arena, diags, fns, env, e->as.binary.rhs, lhs_ty, lenient, io_changed);
                    if (!rhs.ty) return (TcType){0};

                    // Integer-only compound assignments.
                    if (!type_is_int(lhs_ty)) {
                        diagf(arena, diags, e->span, "type: ", "bitwise/shift/remainder compound assignment requires integer lhs");
                        return (TcType){0};
                    }

                    // RHS must be integer-coercible.
                    if (!(rhs.lit == TC_LIT_INT) && !(rhs.ty && type_is_int(rhs.ty))) {
                        diagf(arena, diags, e->span, "type: ", "bitwise/shift/remainder compound assignment requires integer rhs");
                        return (TcType){0};
                    }

                    return (TcType){ .ty = lhs_ty, .lit = TC_LIT_NONE };
                }

                case EMP_BIN_ADD:
                case EMP_BIN_SUB:
                case EMP_BIN_MUL:
                case EMP_BIN_DIV:
                    // Pointer arithmetic (manual-MM only for now): `*u8 + int` and `*u8 - int`.
                    if ((op == EMP_BIN_ADD || op == EMP_BIN_SUB) && g_tc_mm_depth > 0) {
                        if (lhs.ty && type_is_ptr_u8(lhs.ty) && tc_is_intish(rhs)) {
                            return (TcType){ .ty = lhs.ty, .lit = TC_LIT_NONE };
                        }
                        if (op == EMP_BIN_ADD && rhs.ty && type_is_ptr_u8(rhs.ty) && tc_is_intish(lhs)) {
                            return (TcType){ .ty = rhs.ty, .lit = TC_LIT_NONE };
                        }
                    }
                    if ((op == EMP_BIN_ADD || op == EMP_BIN_SUB) && (tc_is_ptr_tc(lhs) || tc_is_ptr_tc(rhs))) {
                        diagf(arena, diags, e->span, "type: ", "pointer arithmetic requires @emp mm off (and currently only supports *u8)");
                        return (TcType){0};
                    }

                    return tc_binary_numeric(arena, diags, e->span, op, lhs, rhs, lenient);

                case EMP_BIN_REM: {
                    // Integer remainder for now.
                    bool li = (lhs.lit == TC_LIT_INT) || (lhs.ty && type_is_int(lhs.ty));
                    bool ri = (rhs.lit == TC_LIT_INT) || (rhs.ty && type_is_int(rhs.ty));
                    if (!li || !ri) {
                        if (lenient && (type_is_auto(lhs.ty) || type_is_auto(rhs.ty))) {
                            return (TcType){ .ty = make_auto(arena, e->span), .lit = TC_LIT_NONE };
                        }
                        diagf(arena, diags, e->span, "type: ", "'%' expects integer operands (for now)");
                        return (TcType){0};
                    }
                    return (TcType){ .ty = make_named(arena, e->span, "i32"), .lit = TC_LIT_NONE };
                }

                case EMP_BIN_EQ:
                case EMP_BIN_NE:
                    if (lenient && (type_is_auto(lhs.ty) || type_is_auto(rhs.ty))) {
                        return (TcType){ .ty = make_named(arena, e->span, "bool"), .lit = TC_LIT_NONE };
                    }

                    // Equality is supported for scalar-ish types.
                    if (tc_is_numeric_tc(lhs) && tc_is_numeric_tc(rhs)) {
                        return (TcType){ .ty = make_named(arena, e->span, "bool"), .lit = TC_LIT_NONE };
                    }
                    if (tc_is_bool_tc(lhs) && tc_is_bool_tc(rhs)) {
                        return (TcType){ .ty = make_named(arena, e->span, "bool"), .lit = TC_LIT_NONE };
                    }
                    if (tc_is_char_tc(lhs) && tc_is_char_tc(rhs)) {
                        return (TcType){ .ty = make_named(arena, e->span, "bool"), .lit = TC_LIT_NONE };
                    }

                    // Pointer equality: ptr==ptr, ptr==null, ptr==0.
                    if ((tc_is_ptr_tc(lhs) && tc_is_ptr_tc(rhs)) ||
                        (tc_is_ptr_tc(lhs) && tc_is_nullish_tc(rhs)) ||
                        (tc_is_ptr_tc(rhs) && tc_is_nullish_tc(lhs))) {
                        return (TcType){ .ty = make_named(arena, e->span, "bool"), .lit = TC_LIT_NONE };
                    }

                    diagf(arena, diags, e->span, "type: ", "equality operator expects comparable operands");
                    return (TcType){0};

                case EMP_BIN_LT:
                case EMP_BIN_LE:
                case EMP_BIN_GT:
                case EMP_BIN_GE:
                    if (lenient && (type_is_auto(lhs.ty) || type_is_auto(rhs.ty))) {
                        return (TcType){ .ty = make_named(arena, e->span, "bool"), .lit = TC_LIT_NONE };
                    }

                    // Ordering comparisons: numeric, plus `char`.
                    if (tc_is_numeric_tc(lhs) && tc_is_numeric_tc(rhs)) {
                        return (TcType){ .ty = make_named(arena, e->span, "bool"), .lit = TC_LIT_NONE };
                    }
                    if (tc_is_char_tc(lhs) && tc_is_char_tc(rhs)) {
                        return (TcType){ .ty = make_named(arena, e->span, "bool"), .lit = TC_LIT_NONE };
                    }

                    diagf(arena, diags, e->span, "type: ", "comparison operator expects numeric (or char) operands");
                    return (TcType){0};

                case EMP_BIN_AND:
                case EMP_BIN_OR:
                    if (lenient && (type_is_auto(lhs.ty) || type_is_auto(rhs.ty))) {
                        return (TcType){ .ty = make_named(arena, e->span, "bool"), .lit = TC_LIT_NONE };
                    }
                    if (!type_is_bool(lhs.ty) && lhs.lit != TC_LIT_BOOL) {
                        diagf(arena, diags, e->as.binary.lhs ? e->as.binary.lhs->span : e->span, "type: ", "logical operator expects bool lhs");
                        return (TcType){0};
                    }
                    if (!type_is_bool(rhs.ty) && rhs.lit != TC_LIT_BOOL) {
                        diagf(arena, diags, e->as.binary.rhs ? e->as.binary.rhs->span : e->span, "type: ", "logical operator expects bool rhs");
                        return (TcType){0};
                    }
                    return (TcType){ .ty = make_named(arena, e->span, "bool"), .lit = TC_LIT_NONE };

                case EMP_BIN_BITAND:
                case EMP_BIN_BITOR:
                case EMP_BIN_BITXOR:
                case EMP_BIN_SHL:
                case EMP_BIN_SHR: {
                    bool li = (lhs.lit == TC_LIT_INT) || (lhs.ty && type_is_int(lhs.ty));
                    bool ri = (rhs.lit == TC_LIT_INT) || (rhs.ty && type_is_int(rhs.ty));
                    if (!li || !ri) {
                        if (lenient && (type_is_auto(lhs.ty) || type_is_auto(rhs.ty))) {
                            return (TcType){ .ty = make_auto(arena, e->span), .lit = TC_LIT_NONE };
                        }
                        diagf(arena, diags, e->span, "type: ", "bitwise/shift operators expect integer operands");
                        return (TcType){0};
                    }
                    return (TcType){ .ty = make_named(arena, e->span, "i32"), .lit = TC_LIT_NONE };
                }

                default:
                    diagf(arena, diags, e->span, "type: ", "binary operator not typechecked yet");
                    return (TcType){0};
            }
        }

        case EMP_EXPR_INDEX: {
            TcType bt = tc_expr(arena, diags, fns, env, e->as.index.base, lenient, io_changed);
            TcType it = tc_expr(arena, diags, fns, env, e->as.index.index, lenient, io_changed);

            // Index must be an int (or int literal).
            if (it.ty && !type_is_int(it.ty) && it.lit != TC_LIT_INT) {
                diagf(arena, diags, e->as.index.index ? e->as.index.index->span : e->span, "type: ", "index must be int");
            }

            if (!bt.ty) return (TcType){0};
            if (bt.ty->kind == EMP_TYPE_ARRAY) {
                return (TcType){ .ty = bt.ty->as.array.elem, .lit = TC_LIT_NONE };
            }

            if (bt.ty->kind == EMP_TYPE_LIST) {
                return (TcType){ .ty = bt.ty->as.array.elem, .lit = TC_LIT_NONE };
            }

            if (type_is_string(bt.ty)) {
                // Indexing into an owned string yields a `char`.
                return (TcType){ .ty = make_named(arena, e->span, "char"), .lit = TC_LIT_NONE };
            }

            if (bt.ty->kind == EMP_TYPE_TUPLE) {
                if (!e->as.index.index || e->as.index.index->kind != EMP_EXPR_INT) {
                    diagf(arena, diags, e->as.index.index ? e->as.index.index->span : e->span, "type: ", "tuple index must be an integer literal");
                    return (TcType){0};
                }

                const char *txt = slice_to_cstr(arena, e->as.index.index->as.lit);
                long long idx = txt ? strtoll(txt, NULL, 10) : -1;
                if (idx < 0 || (size_t)idx >= bt.ty->as.tuple.fields.len) {
                    diagf(arena, diags, e->span, "type: ", "tuple index out of bounds");
                    return (TcType){0};
                }

                const EmpTupleField *f = (const EmpTupleField *)bt.ty->as.tuple.fields.items[(size_t)idx];
                if (!f) return (TcType){0};
                return (TcType){ .ty = f->ty, .lit = TC_LIT_NONE };
            }

            diagf(arena, diags, e->span, "type: ", "indexing is only supported on arrays, lists, tuples, and strings for now");
            return (TcType){0};
        }

        case EMP_EXPR_MEMBER: {
            // Enum variant access: `Enum::Variant`.
            if (g_tc_program && e->as.member.base && e->as.member.base->kind == EMP_EXPR_IDENT) {
                EmpSlice enum_name = e->as.member.base->as.lit;
                const EmpItem *en = find_enum_decl(g_tc_program, enum_name);
                if (en) {
                    size_t vidx = 0;
                    const EmpEnumVariant *v = find_enum_variant(en, e->as.member.member, &vidx);
                    if (!v) {
                        diagf(arena, diags, e->span, "type: ", "unknown enum variant");
                        return (TcType){0};
                    }
                    if (v->fields.len != 0) {
                        diagf(arena, diags, e->span, "type: ", "enum variant requires arguments; use `Enum::Variant(...)`");
                        return (TcType){0};
                    }
                    return (TcType){ .ty = make_named(arena, e->span, slice_to_cstr(arena, enum_name)), .lit = TC_LIT_NONE };
                }
            }

            TcType bt = tc_expr(arena, diags, fns, env, e->as.member.base, lenient, io_changed);
            if (!bt.ty) return (TcType){0};
            const EmpType *ft = lookup_field_type(g_tc_program, bt.ty, e->as.member.member, e->span, arena, diags);
            return ft ? (TcType){ .ty = ft, .lit = TC_LIT_NONE } : (TcType){0};
        }

        case EMP_EXPR_NEW: {
            // Validate the class exists.
            if (!g_tc_program) {
                diagf(arena, diags, e->span, "type: ", "internal error: program not set for typechecking");
                return (TcType){0};
            }

            const EmpItem *decl = find_named_decl(g_tc_program, e->as.new_expr.class_name);
            if (!decl || decl->kind != EMP_ITEM_CLASS) {
                diagf(arena, diags, e->span, "type: ", "new expects a class name");
                return (TcType){0};
            }

            // Find init if present.
            const EmpClassMethod *initm = NULL;
            for (size_t i = 0; i < decl->as.class_decl.methods.len; i++) {
                const EmpClassMethod *m = (const EmpClassMethod *)decl->as.class_decl.methods.items[i];
                if (m && m->is_init) {
                    initm = m;
                    break;
                }
            }

            if (initm) {
                if (initm->params.len != e->as.new_expr.args.len) {
                    diagf(arena, diags, e->span, "type: ", "wrong number of arguments to init");
                } else {
                    for (size_t i = 0; i < initm->params.len; i++) {
                        const EmpParam *p = (const EmpParam *)initm->params.items[i];
                        const EmpExpr *arg = (const EmpExpr *)e->as.new_expr.args.items[i];
                        TcType at = tc_expr(arena, diags, fns, env, (EmpExpr *)arg, lenient, io_changed);
                        if (!p || !p->ty) continue;
                        if (!can_coerce(at, p->ty)) {
                            diagf(arena, diags, arg ? arg->span : e->span, "type: ", "argument type mismatch");
                        }
                    }
                }
            } else {
                if (e->as.new_expr.args.len != 0) {
                    diagf(arena, diags, e->span, "type: ", "class has no init; expected 0 arguments");
                }
            }

            return (TcType){ .ty = make_named(arena, e->span, slice_to_cstr(arena, e->as.new_expr.class_name)), .lit = TC_LIT_NONE };
        }

        case EMP_EXPR_CALL: {
            if (!e->as.call.callee) {
                diagf(arena, diags, e->span, "type: ", "call missing callee");
                return (TcType){0};
            }

            // Method sugar for built-in list type `T[]` and builtin `string`.
            // Lower `xs.push(v)` and friends into `list_push(&mut xs, v)` etc.
            // Lower `s.len()` and friends into `string_len(&s)` etc.
            if (e->as.call.callee->kind == EMP_EXPR_MEMBER) {
                const EmpExpr *base0 = unwrap_group_expr(e->as.call.callee->as.member.base);
                EmpSlice mname = e->as.call.callee->as.member.member;

                if (base0 && base0->kind == EMP_EXPR_IDENT) {
                    EmpType *bt = env_lookup(env, base0->as.lit);

                    // List methods (mutating).
                    if (bt && bt->kind == EMP_TYPE_LIST) {
                        const char *builtin = NULL;
                        size_t want = 0;
                        bool prepend_borrow_mut = true;
                        bool prepend_borrow = false;
                        bool add_zero_index = false;

                        if (slice_is(mname, "append") || slice_is(mname, "push") || slice_is(mname, "enqueue")) {
                            builtin = "list_push";
                            want = 1;
                        } else if (slice_is(mname, "reserve")) {
                            builtin = "list_reserve";
                            want = 1;
                        } else if (slice_is(mname, "pop")) {
                            builtin = "list_pop";
                            want = 0;
                        } else if (slice_is(mname, "insert")) {
                            builtin = "list_insert";
                            want = 2;
                        } else if (slice_is(mname, "remove")) {
                            builtin = "list_remove";
                            want = 1;
                        } else if (slice_is(mname, "dequeue")) {
                            builtin = "list_remove";
                            want = 0;
                            add_zero_index = true;
                        } else if (slice_is(mname, "len")) {
                            builtin = "list_len";
                            want = 0;
                            prepend_borrow_mut = false;
                            prepend_borrow = true;
                        } else if (slice_is(mname, "cap")) {
                            builtin = "list_cap";
                            want = 0;
                            prepend_borrow_mut = false;
                            prepend_borrow = true;
                        }

                        if (builtin) {
                            if (e->as.call.args.len != want) {
                                diagf(arena, diags, e->span, "type: ", "wrong number of arguments");
                                return (TcType){0};
                            }

                            EmpExpr *new_callee = make_ident_expr(arena, e->as.call.callee->span, builtin);
                            if (!new_callee) return (TcType){0};

                            EmpExpr *a0 = NULL;
                            if (prepend_borrow_mut) {
                                a0 = make_borrow_mut_ident_expr(arena, base0->span, base0->as.lit);
                            } else if (prepend_borrow) {
                                a0 = make_borrow_ident_expr(arena, base0->span, base0->as.lit);
                            }
                            if (!a0) return (TcType){0};

                            EmpVec new_args;
                            emp_vec_init(&new_args);
                            (void)emp_vec_push(&new_args, a0);
                            for (size_t i = 0; i < e->as.call.args.len; i++) {
                                (void)emp_vec_push(&new_args, e->as.call.args.items[i]);
                            }
                            if (add_zero_index) {
                                EmpExpr *z = make_int_expr(arena, e->span, "0");
                                if (!z) return (TcType){0};
                                (void)emp_vec_push(&new_args, z);
                            }

                            e->as.call.callee = new_callee;
                            e->as.call.args = new_args;
                        }
                    }

                    // String methods.
                    if (bt && type_is_string(bt)) {
                        const char *builtin = NULL;
                        size_t want = 0;
                        bool borrow_self = true;

                        if (slice_is(mname, "len")) {
                            builtin = "string_len";
                            want = 0;
                        } else if (slice_is(mname, "cstr")) {
                            builtin = "string_cstr";
                            want = 0;
                        } else if (slice_is(mname, "clone")) {
                            builtin = "string_clone";
                            want = 0;
                        } else if (slice_is(mname, "parse_i32")) {
                            builtin = "string_parse_i32";
                            want = 0;
                        } else if (slice_is(mname, "parse_bool")) {
                            builtin = "string_parse_bool";
                            want = 0;
                        } else if (slice_is(mname, "starts_with")) {
                            builtin = "string_starts_with";
                            want = 1;
                        } else if (slice_is(mname, "ends_with")) {
                            builtin = "string_ends_with";
                            want = 1;
                        } else if (slice_is(mname, "contains")) {
                            builtin = "string_contains";
                            want = 1;
                        } else if (slice_is(mname, "replace")) {
                            builtin = "string_replace";
                            want = 2;
                        }

                        if (builtin) {
                            if (e->as.call.args.len != want) {
                                diagf(arena, diags, e->span, "type: ", "wrong number of arguments");
                                return (TcType){0};
                            }

                            EmpExpr *new_callee = make_ident_expr(arena, e->as.call.callee->span, builtin);
                            if (!new_callee) return (TcType){0};

                            EmpExpr *self_arg = borrow_self ? make_borrow_ident_expr(arena, base0->span, base0->as.lit) : NULL;
                            if (!self_arg) return (TcType){0};

                            EmpVec new_args;
                            emp_vec_init(&new_args);
                            (void)emp_vec_push(&new_args, self_arg);

                            // For now, these builtins expect borrowed string idents.
                            for (size_t i = 0; i < e->as.call.args.len; i++) {
                                EmpExpr *arg = (EmpExpr *)e->as.call.args.items[i];
                                const EmpExpr *a0u = unwrap_group_expr((const EmpExpr *)arg);
                                if (a0u && a0u->kind == EMP_EXPR_IDENT) {
                                    EmpExpr *ba = make_borrow_ident_expr(arena, a0u->span, a0u->as.lit);
                                    if (!ba) return (TcType){0};
                                    (void)emp_vec_push(&new_args, ba);
                                } else {
                                    // Keep as-is; typecheck will error with a clear message.
                                    (void)emp_vec_push(&new_args, arg);
                                }
                            }

                            e->as.call.callee = new_callee;
                            e->as.call.args = new_args;
                        }
                    }
                }
            }

            // Free-function calls: `foo(...)`
            if (e->as.call.callee->kind == EMP_EXPR_IDENT) {
                // Compiler builtins for built-in list type `T[]`.
                bool is_reserve = slice_is(e->as.call.callee->as.lit, "list_reserve");
                bool is_push = slice_is(e->as.call.callee->as.lit, "list_push");
                bool is_free = slice_is(e->as.call.callee->as.lit, "list_free");
                bool is_pop = slice_is(e->as.call.callee->as.lit, "list_pop");
                bool is_insert = slice_is(e->as.call.callee->as.lit, "list_insert");
                bool is_remove = slice_is(e->as.call.callee->as.lit, "list_remove");
                bool is_len = slice_is(e->as.call.callee->as.lit, "list_len");
                bool is_cap = slice_is(e->as.call.callee->as.lit, "list_cap");

                // Compiler builtins for builtin `string`.
                bool is_s_from = slice_is(e->as.call.callee->as.lit, "string_from_cstr");
                bool is_s_clone = slice_is(e->as.call.callee->as.lit, "string_clone");
                bool is_s_len = slice_is(e->as.call.callee->as.lit, "string_len");
                bool is_s_cstr = slice_is(e->as.call.callee->as.lit, "string_cstr");
                bool is_s_pi32 = slice_is(e->as.call.callee->as.lit, "string_parse_i32");
                bool is_s_pbool = slice_is(e->as.call.callee->as.lit, "string_parse_bool");

                if (is_reserve || is_push || is_free || is_pop || is_insert || is_remove || is_len || is_cap) {
                    size_t want_args = 0;
                    if (is_free || is_pop || is_len || is_cap) want_args = 1;
                    else if (is_reserve || is_push || is_remove) want_args = 2;
                    else if (is_insert) want_args = 3;

                    if (e->as.call.args.len != want_args) {
                        diagf(arena, diags, e->span, "type: ", "wrong number of arguments");
                        return (TcType){0};
                    }

                    // `list_free` is an explicit manual free; safe code should rely on drop.
                    // Only allow it inside `@emp mm off`.
                    if (is_free && g_tc_mm_depth == 0) {
                        diagf(arena, diags, e->span, "type: ", "manual memory function requires @emp mm off");
                        return (TcType){0};
                    }

                    const EmpExpr *a0 = (const EmpExpr *)e->as.call.args.items[0];
                    TcType t0 = tc_expr(arena, diags, fns, env, (EmpExpr *)a0, lenient, io_changed);
                    if (!t0.ty || t0.ty->kind != EMP_TYPE_PTR || !t0.ty->as.ptr.pointee || t0.ty->as.ptr.pointee->kind != EMP_TYPE_LIST) {
                        diagf(arena, diags, a0 ? a0->span : e->span, "type: ", "first argument must be a pointer to a list (use &mut xs where xs: T[])");
                        return (TcType){0};
                    }
                    const EmpType *list_ty = t0.ty->as.ptr.pointee;

                    if (is_len || is_cap) {
                        return (TcType){ .ty = make_named(arena, e->span, "i32"), .lit = TC_LIT_NONE };
                    }

                    if (is_reserve) {
                        const EmpExpr *a1 = (const EmpExpr *)e->as.call.args.items[1];
                        TcType t1 = tc_expr(arena, diags, fns, env, (EmpExpr *)a1, lenient, io_changed);
                        if (t1.ty && !type_is_int(t1.ty) && t1.lit != TC_LIT_INT) {
                            diagf(arena, diags, a1 ? a1->span : e->span, "type: ", "reserve cap must be int");
                            return (TcType){0};
                        }
                        return (TcType){ .ty = NULL, .lit = TC_LIT_NONE };
                    }

                    if (is_free) {
                        return (TcType){ .ty = NULL, .lit = TC_LIT_NONE };
                    }

                    if (is_pop) {
                        return (TcType){ .ty = list_ty->as.array.elem, .lit = TC_LIT_NONE };
                    }

                    if (is_remove) {
                        const EmpExpr *a1 = (const EmpExpr *)e->as.call.args.items[1];
                        TcType t1 = tc_expr(arena, diags, fns, env, (EmpExpr *)a1, lenient, io_changed);
                        if (t1.ty && !type_is_int(t1.ty) && t1.lit != TC_LIT_INT) {
                            diagf(arena, diags, a1 ? a1->span : e->span, "type: ", "remove index must be int");
                            return (TcType){0};
                        }
                        return (TcType){ .ty = list_ty->as.array.elem, .lit = TC_LIT_NONE };
                    }

                    if (is_insert) {
                        const EmpExpr *a1 = (const EmpExpr *)e->as.call.args.items[1];
                        const EmpExpr *a2 = (const EmpExpr *)e->as.call.args.items[2];
                        TcType t1 = tc_expr(arena, diags, fns, env, (EmpExpr *)a1, lenient, io_changed);
                        if (t1.ty && !type_is_int(t1.ty) && t1.lit != TC_LIT_INT) {
                            diagf(arena, diags, a1 ? a1->span : e->span, "type: ", "insert index must be int");
                            return (TcType){0};
                        }
                        TcType t2 = tc_expr(arena, diags, fns, env, (EmpExpr *)a2, lenient, io_changed);
                        const EmpType *elem_ty = list_ty->as.array.elem;
                        if (!elem_ty) return (TcType){0};
                        if (!can_coerce(t2, (EmpType *)elem_ty)) {
                            diagf(arena, diags, a2 ? a2->span : e->span, "type: ", "insert value type mismatch");
                            return (TcType){0};
                        }
                        return (TcType){ .ty = NULL, .lit = TC_LIT_NONE };
                    }

                    if (is_push) {
                        const EmpExpr *a1 = (const EmpExpr *)e->as.call.args.items[1];
                        TcType t1 = tc_expr(arena, diags, fns, env, (EmpExpr *)a1, lenient, io_changed);
                        const EmpType *elem_ty = list_ty->as.array.elem;
                        if (!elem_ty) return (TcType){0};

                        // If the list element type is `auto`, infer it from the first pushed value.
                        if (type_is_auto(elem_ty)) {
                            EmpType *inferred = NULL;
                            if (t1.ty && !type_is_auto(t1.ty)) {
                                inferred = (EmpType *)t1.ty;
                            } else if (t1.lit == TC_LIT_INT || t1.lit == TC_LIT_INT_ZERO) {
                                inferred = make_named(arena, a1 ? a1->span : e->span, "i32");
                            } else if (t1.lit == TC_LIT_FLOAT) {
                                inferred = make_named(arena, a1 ? a1->span : e->span, "f64");
                            } else if (t1.lit == TC_LIT_BOOL) {
                                inferred = make_named(arena, a1 ? a1->span : e->span, "bool");
                            } else if (t1.lit == TC_LIT_STRING) {
                                // String literals are `*u8`.
                                inferred = make_ptr(arena, a1 ? a1->span : e->span, make_named(arena, a1 ? a1->span : e->span, "u8"));
                            }

                            if (inferred) {
                                // Update the binding type when the first argument is `&mut <ident>`.
                                const EmpExpr *arg0 = unwrap_group_expr((const EmpExpr *)e->as.call.args.items[0]);
                                if (arg0 && arg0->kind == EMP_EXPR_UNARY && arg0->as.unary.op == EMP_UN_BORROW_MUT) {
                                    const EmpExpr *rhs0 = unwrap_group_expr(arg0->as.unary.rhs);
                                    if (rhs0 && rhs0->kind == EMP_EXPR_IDENT) {
                                        EmpType *new_list_ty = make_list(arena, rhs0->span, inferred);
                                        if (new_list_ty) (void)env_push(env, rhs0->as.lit, new_list_ty);
                                        elem_ty = inferred;
                                    }
                                }
                            }
                        }

                        if (!can_coerce(t1, elem_ty)) {
                            diagf(arena, diags, a1 ? a1->span : e->span, "type: ", "push value type mismatch");
                            return (TcType){0};
                        }
                        return (TcType){ .ty = NULL, .lit = TC_LIT_NONE };
                    }
                }

                if (is_s_from) {
                    if (e->as.call.args.len != 1) {
                        diagf(arena, diags, e->span, "type: ", "string_from_cstr expects 1 arg");
                        return (TcType){0};
                    }
                    const EmpExpr *a0 = (const EmpExpr *)e->as.call.args.items[0];
                    TcType t0 = tc_expr(arena, diags, fns, env, (EmpExpr *)a0, lenient, io_changed);
                    if (!(t0.lit == TC_LIT_STRING || type_is_str(t0.ty) || (t0.lit == TC_LIT_NULL && type_is_ptr(t0.ty)))) {
                        diagf(arena, diags, a0 ? a0->span : e->span, "type: ", "string_from_cstr expects a *u8 C string");
                        return (TcType){0};
                    }
                    return (TcType){ .ty = make_named(arena, e->span, "string"), .lit = TC_LIT_NONE };
                }

                bool is_s_sw = slice_is(e->as.call.callee->as.lit, "string_starts_with");
                bool is_s_ew = slice_is(e->as.call.callee->as.lit, "string_ends_with");
                bool is_s_contains = slice_is(e->as.call.callee->as.lit, "string_contains");
                bool is_s_replace = slice_is(e->as.call.callee->as.lit, "string_replace");

                if (is_s_sw || is_s_ew || is_s_contains) {
                    if (e->as.call.args.len != 2) {
                        diagf(arena, diags, e->span, "type: ", "wrong number of arguments");
                        return (TcType){0};
                    }
                    const EmpExpr *a0 = (const EmpExpr *)e->as.call.args.items[0];
                    const EmpExpr *a1 = (const EmpExpr *)e->as.call.args.items[1];
                    TcType t0 = tc_expr(arena, diags, fns, env, (EmpExpr *)a0, lenient, io_changed);
                    TcType t1 = tc_expr(arena, diags, fns, env, (EmpExpr *)a1, lenient, io_changed);
                    if (!t0.ty || t0.ty->kind != EMP_TYPE_PTR || !t0.ty->as.ptr.pointee || !type_is_string(t0.ty->as.ptr.pointee)) {
                        diagf(arena, diags, a0 ? a0->span : e->span, "type: ", "string builtin expects `&<string_ident>` (for now)");
                        return (TcType){0};
                    }
                    if (!t1.ty || t1.ty->kind != EMP_TYPE_PTR || !t1.ty->as.ptr.pointee || !type_is_string(t1.ty->as.ptr.pointee)) {
                        diagf(arena, diags, a1 ? a1->span : e->span, "type: ", "string builtin expects `&<string_ident>` (for now)");
                        return (TcType){0};
                    }
                    return (TcType){ .ty = make_named(arena, e->span, "bool"), .lit = TC_LIT_NONE };
                }

                if (is_s_replace) {
                    if (e->as.call.args.len != 3) {
                        diagf(arena, diags, e->span, "type: ", "wrong number of arguments");
                        return (TcType){0};
                    }
                    for (size_t i = 0; i < 3; i++) {
                        const EmpExpr *ai = (const EmpExpr *)e->as.call.args.items[i];
                        TcType ti = tc_expr(arena, diags, fns, env, (EmpExpr *)ai, lenient, io_changed);
                        if (!ti.ty || ti.ty->kind != EMP_TYPE_PTR || !ti.ty->as.ptr.pointee || !type_is_string(ti.ty->as.ptr.pointee)) {
                            diagf(arena, diags, ai ? ai->span : e->span, "type: ", "string builtin expects `&<string_ident>` (for now)");
                            return (TcType){0};
                        }
                    }
                    return (TcType){ .ty = make_named(arena, e->span, "string"), .lit = TC_LIT_NONE };
                }

                if (is_s_clone || is_s_len || is_s_cstr || is_s_pi32 || is_s_pbool) {
                    if (e->as.call.args.len != 1) {
                        diagf(arena, diags, e->span, "type: ", "wrong number of arguments");
                        return (TcType){0};
                    }
                    const EmpExpr *a0 = (const EmpExpr *)e->as.call.args.items[0];
                    TcType t0 = tc_expr(arena, diags, fns, env, (EmpExpr *)a0, lenient, io_changed);

                    // Expect `&string` for now.
                    if (!t0.ty || t0.ty->kind != EMP_TYPE_PTR || !t0.ty->as.ptr.pointee || !type_is_string(t0.ty->as.ptr.pointee)) {
                        diagf(arena, diags, a0 ? a0->span : e->span, "type: ", "string builtin expects `&<string_ident>` (for now)");
                        return (TcType){0};
                    }

                    if (is_s_clone) {
                        return (TcType){ .ty = make_named(arena, e->span, "string"), .lit = TC_LIT_NONE };
                    }
                    if (is_s_len) {
                        return (TcType){ .ty = make_named(arena, e->span, "i32"), .lit = TC_LIT_NONE };
                    }
                    if (is_s_cstr) {
                        return (TcType){ .ty = make_ptr(arena, e->span, make_named(arena, e->span, "u8")), .lit = TC_LIT_NONE };
                    }
                    if (is_s_pi32) {
                        return (TcType){ .ty = make_named(arena, e->span, "i32"), .lit = TC_LIT_NONE };
                    }
                    if (is_s_pbool) {
                        return (TcType){ .ty = make_named(arena, e->span, "bool"), .lit = TC_LIT_NONE };
                    }
                }

                // Typecheck args once.
                size_t argc = e->as.call.args.len;
                TcType *arg_types = NULL;
                EmpExpr **arg_exprs = NULL;
                if (argc) {
                    arg_types = (TcType *)calloc(argc, sizeof(TcType));
                    arg_exprs = (EmpExpr **)calloc(argc, sizeof(EmpExpr *));
                    if (!arg_types || !arg_exprs) {
                        free(arg_types);
                        free(arg_exprs);
                        diagf(arena, diags, e->span, "type: ", "out of memory");
                        return (TcType){0};
                    }
                    for (size_t i = 0; i < argc; i++) {
                        EmpExpr *arg = (EmpExpr *)e->as.call.args.items[i];
                        arg_exprs[i] = arg;
                        arg_types[i] = tc_expr(arena, diags, fns, env, arg, lenient, io_changed);
                    }
                }

                TcFnSig *sig = tc_resolve_free_overload(arena, diags, fns, env, e, e->as.call.callee->as.lit, arg_types, arg_exprs, argc, lenient, io_changed);
                if (!sig) {
                    free(arg_types);
                    free(arg_exprs);
                    return (TcType){0};
                }

                // Monomorphic `auto` parameter inference from call sites (per-overload).
                if (io_changed) {
                    for (size_t i = 0; i < argc; i++) {
                        EmpType *pt = sig->params[i];
                        if (!pt || !type_is_auto(pt)) continue;
                        if (arg_types[i].ty && !type_is_auto(arg_types[i].ty) && !type_is_void(arg_types[i].ty)) {
                            sig->params[i] = (EmpType *)arg_types[i].ty;
                            if (sig->decl && sig->decl->kind == EMP_ITEM_FN && i < sig->decl->as.fn.params.len) {
                                EmpParam *p = (EmpParam *)sig->decl->as.fn.params.items[i];
                                if (p) p->ty = sig->params[i];
                            }
                            *io_changed = true;
                        }
                    }

                    // List element inference for wrappers: if an overload takes `*auto[]` and `auto`,
                    // infer the list element type from the first pushed value and specialize both
                    // the list binding and the overload parameter types.
                    if (argc >= 2 && sig->params_len >= 2) {
                        EmpType *p0 = sig->params[0];
                        EmpType *p1 = sig->params[1];
                        if (p0 && p0->kind == EMP_TYPE_PTR && p0->as.ptr.pointee && p0->as.ptr.pointee->kind == EMP_TYPE_LIST && type_is_auto(p0->as.ptr.pointee->as.array.elem) && p1 && type_is_auto(p1)) {
                            // Infer from argument 1.
                            TcType t1 = arg_types[1];
                            EmpType *inferred = NULL;
                            if (t1.ty && !type_is_auto(t1.ty)) {
                                inferred = (EmpType *)t1.ty;
                            } else if (t1.lit == TC_LIT_INT || t1.lit == TC_LIT_INT_ZERO) {
                                inferred = make_named(arena, arg_exprs[1] ? arg_exprs[1]->span : e->span, "i32");
                            } else if (t1.lit == TC_LIT_FLOAT) {
                                inferred = make_named(arena, arg_exprs[1] ? arg_exprs[1]->span : e->span, "f64");
                            } else if (t1.lit == TC_LIT_BOOL) {
                                inferred = make_named(arena, arg_exprs[1] ? arg_exprs[1]->span : e->span, "bool");
                            } else if (t1.lit == TC_LIT_STRING) {
                                inferred = make_ptr(arena, arg_exprs[1] ? arg_exprs[1]->span : e->span, make_named(arena, arg_exprs[1] ? arg_exprs[1]->span : e->span, "u8"));
                            }

                            if (inferred) {
                                // Specialize param 1.
                                sig->params[1] = inferred;
                                if (sig->decl && sig->decl->kind == EMP_ITEM_FN && 1 < sig->decl->as.fn.params.len) {
                                    EmpParam *p = (EmpParam *)sig->decl->as.fn.params.items[1];
                                    if (p) p->ty = inferred;
                                }

                                // Specialize param 0: `*auto[]` -> `*<inferred>[]`.
                                EmpType *new_list = make_list(arena, e->span, inferred);
                                EmpType *new_p0 = new_list ? make_ptr(arena, e->span, new_list) : NULL;
                                if (new_p0) {
                                    sig->params[0] = new_p0;
                                    if (sig->decl && sig->decl->kind == EMP_ITEM_FN && 0 < sig->decl->as.fn.params.len) {
                                        EmpParam *p = (EmpParam *)sig->decl->as.fn.params.items[0];
                                        if (p) p->ty = new_p0;
                                    }
                                }

                                // Update the binding type when argument 0 is `&mut <ident>`.
                                const EmpExpr *arg0 = unwrap_group_expr((const EmpExpr *)e->as.call.args.items[0]);
                                if (arg0 && arg0->kind == EMP_EXPR_UNARY && arg0->as.unary.op == EMP_UN_BORROW_MUT) {
                                    const EmpExpr *rhs0 = unwrap_group_expr(arg0->as.unary.rhs);
                                    if (rhs0 && rhs0->kind == EMP_EXPR_IDENT) {
                                        EmpType *new_list_ty = make_list(arena, rhs0->span, inferred);
                                        if (new_list_ty) {
                                            (void)env_push(env, rhs0->as.lit, new_list_ty);
                                        }
                                    }
                                }

                                *io_changed = true;
                            }
                        }
                    }
                }

                // Enforce argument compatibility against the selected overload.
                for (size_t i = 0; i < argc; i++) {
                    EmpType *pt = sig->params[i];
                    if (!pt) {
                        diagf(arena, diags, e->span, "type: ", "internal error: missing parameter type");
                        continue;
                    }

                    // If inference didn't run yet, allow lenient pass to proceed.
                    if (lenient && type_is_auto(pt)) continue;

                    if (!tc_arg_compatible(arena, diags, fns, env, arg_types[i], arg_exprs ? arg_exprs[i] : NULL, pt, lenient, io_changed)) {
                        diagf(arena, diags, arg_exprs[i] ? arg_exprs[i]->span : e->span, "type: ", "argument type mismatch");
                    }
                }

                // Record resolved callee symbol name for codegen when overloaded.
                if (fns_count_name(fns, sig->name) > 1) {
                    e->as.call.resolved_name = mangle_overload_name(arena, sig->name, sig->params, sig->params_len);
                } else {
                    e->as.call.resolved_name = (EmpSlice){0};
                }

                free(arg_types);
                free(arg_exprs);
                return (TcType){ .ty = sig->ret, .lit = TC_LIT_NONE };
            }

            // Method calls: `obj.method(...)`
            if (e->as.call.callee->kind == EMP_EXPR_MEMBER) {
                EmpExpr *mcall = e->as.call.callee;

                // UFCS-style trait method call: `Trait::method(recv, ...)`.
                // Static dispatch only: resolve to `impl Trait for <recv_type>` and call that method.
                if (g_tc_program && mcall->as.member.base && mcall->as.member.base->kind == EMP_EXPR_IDENT) {
                    const EmpItem *tr = find_trait_decl(g_tc_program, mcall->as.member.base->as.lit);
                    if (tr) {
                        const EmpTraitMethod *tm = find_trait_method(tr, mcall->as.member.member);
                        if (!tm) {
                            diagf(arena, diags, e->span, "type: ", "unknown trait method");
                            return (TcType){0};
                        }
                        if (e->as.call.args.len == 0) {
                            diagf(arena, diags, e->span, "type: ", "trait method call expects a receiver argument");
                            return (TcType){0};
                        }

                        EmpExpr *recv_expr = (EmpExpr *)e->as.call.args.items[0];
                        TcType recv_tt = tc_expr(arena, diags, fns, env, recv_expr, lenient, io_changed);
                        if (!recv_tt.ty) return (TcType){0};

                        const EmpType *recv_ty = recv_tt.ty;
                        if (recv_ty->kind == EMP_TYPE_PTR && recv_ty->as.ptr.pointee) recv_ty = recv_ty->as.ptr.pointee;
                        if (!recv_ty || recv_ty->kind != EMP_TYPE_NAME) {
                            diagf(arena, diags, recv_expr ? recv_expr->span : e->span, "type: ", "trait receiver must be a named type (or pointer to one)");
                            return (TcType){0};
                        }

                        EmpSlice recv_name = recv_ty->as.name;

                        // Find an impl block: `impl Trait for Recv`.
                        const EmpItem *impl_it = NULL;
                        for (size_t ii = 0; ii < g_tc_program->items.len; ii++) {
                            const EmpItem *it = (const EmpItem *)g_tc_program->items.items[ii];
                            if (!it || it->kind != EMP_ITEM_IMPL) continue;
                            if (!it->as.impl_decl.trait_name.ptr || !it->as.impl_decl.trait_name.len) continue;
                            if (!slice_eq(it->as.impl_decl.trait_name, tr->as.trait_decl.name)) continue;
                            if (!slice_eq(it->as.impl_decl.target_name, recv_name)) continue;
                            impl_it = it;
                            break;
                        }

                        if (!impl_it) {
                            diagf(arena, diags, e->span, "type: ", "missing trait impl for receiver type");
                            return (TcType){0};
                        }

                        // Typecheck user args (excluding receiver) once.
                        size_t user_argc = (e->as.call.args.len > 0) ? (e->as.call.args.len - 1) : 0;
                        TcType *arg_types = NULL;
                        EmpExpr **arg_exprs = NULL;
                        if (user_argc) {
                            arg_types = (TcType *)calloc(user_argc, sizeof(TcType));
                            arg_exprs = (EmpExpr **)calloc(user_argc, sizeof(EmpExpr *));
                            if (!arg_types || !arg_exprs) {
                                free(arg_types);
                                free(arg_exprs);
                                diagf(arena, diags, e->span, "type: ", "out of memory");
                                return (TcType){0};
                            }
                            for (size_t ai = 0; ai < user_argc; ai++) {
                                EmpExpr *arg = (EmpExpr *)e->as.call.args.items[ai + 1];
                                arg_exprs[ai] = arg;
                                arg_types[ai] = tc_expr(arena, diags, fns, env, arg, lenient, io_changed);
                            }
                        }

                        const EmpImplMethod *best = NULL;
                        int best_cost = 0;
                        bool ambiguous = false;

                        for (size_t mi = 0; mi < impl_it->as.impl_decl.methods.len; mi++) {
                            const EmpImplMethod *m = (const EmpImplMethod *)impl_it->as.impl_decl.methods.items[mi];
                            if (!m) continue;
                            if (!slice_eq(m->name, tm->name)) continue;
                            if (m->params.len != user_argc) continue;

                            // Ensure this overload matches the trait signature (with `auto` = Self).
                            if (tm->params.len != m->params.len) continue;
                            bool sig_ok = true;
                            for (size_t pi = 0; pi < tm->params.len; pi++) {
                                const EmpParam *tp = (const EmpParam *)tm->params.items[pi];
                                const EmpParam *ip = (const EmpParam *)m->params.items[pi];
                                if (!tp || !ip || !type_matches_trait_self(tp->ty, ip->ty, recv_name)) {
                                    sig_ok = false;
                                    break;
                                }
                            }
                            if (type_is_void(tm->ret_ty) != type_is_void(m->ret_ty)) sig_ok = false;
                            if (!type_is_void(tm->ret_ty) && !type_matches_trait_self(tm->ret_ty, m->ret_ty, recv_name)) sig_ok = false;
                            if (!sig_ok) continue;

                            int cost = 0;
                            bool ok = true;
                            for (size_t ai = 0; ai < user_argc; ai++) {
                                const EmpParam *p = (const EmpParam *)m->params.items[ai];
                                if (!p || !p->ty) {
                                    ok = false;
                                    break;
                                }
                                if (arg_types[ai].ty && type_eq_shallow(arg_types[ai].ty, p->ty)) {
                                    continue;
                                }
                                if (!tc_arg_compatible(arena, /*diags*/ NULL, fns, env, arg_types[ai], arg_exprs ? arg_exprs[ai] : NULL, p->ty, lenient, io_changed)) {
                                    ok = false;
                                    break;
                                }
                                cost += 1;
                            }
                            if (!ok) continue;

                            if (!best) {
                                best = m;
                                best_cost = cost;
                                ambiguous = false;
                            } else if (cost < best_cost) {
                                best = m;
                                best_cost = cost;
                                ambiguous = false;
                            } else if (cost == best_cost) {
                                ambiguous = true;
                            }
                        }

                        if (!best || ambiguous) {
                            diagf(arena, diags, e->span, "type: ", (!best) ? "no matching trait method overload" : "ambiguous trait method overload");
                            free(arg_types);
                            free(arg_exprs);
                            return (TcType){0};
                        }

                        // Validate args against chosen impl method.
                        for (size_t i = 0; i < user_argc; i++) {
                            const EmpParam *p = (const EmpParam *)best->params.items[i];
                            if (!p || !p->ty) continue;
                            if (!tc_arg_compatible(arena, diags, fns, env, arg_types ? arg_types[i] : (TcType){0}, arg_exprs ? arg_exprs[i] : NULL, p->ty, lenient, io_changed)) {
                                diagf(arena, diags, arg_exprs && arg_exprs[i] ? arg_exprs[i]->span : e->span, "type: ", "argument type mismatch");
                            }
                        }

                        // Record resolved trait impl symbol name for codegen.
                        e->as.call.resolved_name = tc_trait_method_symbol_name(arena, g_tc_program, tr->as.trait_decl.name, recv_name, tm->name, &best->params);

                        free(arg_types);
                        free(arg_exprs);
                        return (TcType){ .ty = best->ret_ty, .lit = TC_LIT_NONE };
                    }
                }

                // Enum variant constructors: `Enum::Variant(...)`
                if (g_tc_program && mcall->as.member.base && mcall->as.member.base->kind == EMP_EXPR_IDENT) {
                    EmpSlice enum_name = mcall->as.member.base->as.lit;
                    const EmpItem *en = find_enum_decl(g_tc_program, enum_name);
                    if (en) {
                        size_t vidx = 0;
                        const EmpEnumVariant *v = find_enum_variant(en, mcall->as.member.member, &vidx);
                        if (!v) {
                            diagf(arena, diags, e->span, "type: ", "unknown enum variant");
                            return (TcType){0};
                        }

                        if (e->as.call.args.len != v->fields.len) {
                            diagf(arena, diags, e->span, "type: ", "wrong number of arguments to enum variant");
                            return (TcType){0};
                        }

                        for (size_t i = 0; i < v->fields.len; i++) {
                            const EmpType *ft = (const EmpType *)v->fields.items[i];
                            const EmpExpr *arg = (const EmpExpr *)e->as.call.args.items[i];
                            TcType at = tc_expr(arena, diags, fns, env, (EmpExpr *)arg, lenient, io_changed);
                            if (!ft) continue;
                            if (!can_coerce(at, (EmpType *)ft)) {
                                diagf(arena, diags, arg ? arg->span : e->span, "type: ", "argument type mismatch");
                            }
                        }

                        return (TcType){ .ty = make_named(arena, e->span, slice_to_cstr(arena, enum_name)), .lit = TC_LIT_NONE };
                    }
                }

                TcType bt = tc_expr(arena, diags, fns, env, mcall->as.member.base, lenient, io_changed);
                if (!bt.ty) return (TcType){0};

                const EmpType *recv = bt.ty;
                if (recv->kind == EMP_TYPE_PTR && recv->as.ptr.pointee) recv = recv->as.ptr.pointee;
                if (!g_tc_program) {
                    diagf(arena, diags, e->span, "type: ", "internal error: program not set for typechecking");
                    return (TcType){0};
                }

                // Typecheck args once.
                size_t argc = e->as.call.args.len;
                TcType *arg_types = NULL;
                EmpExpr **arg_exprs = NULL;
                if (argc) {
                    arg_types = (TcType *)calloc(argc, sizeof(TcType));
                    arg_exprs = (EmpExpr **)calloc(argc, sizeof(EmpExpr *));
                    if (!arg_types || !arg_exprs) {
                        free(arg_types);
                        free(arg_exprs);
                        diagf(arena, diags, e->span, "type: ", "out of memory");
                        return (TcType){0};
                    }
                    for (size_t i = 0; i < argc; i++) {
                        EmpExpr *arg = (EmpExpr *)e->as.call.args.items[i];
                        arg_exprs[i] = arg;
                        arg_types[i] = tc_expr(arena, diags, fns, env, arg, lenient, io_changed);
                    }
                }

                // Dyn method calls: `d.method(...)` where `d: dyn Base`
                if (recv && recv->kind == EMP_TYPE_DYN) {
                    const EmpItem *base_decl = find_class_decl(g_tc_program, recv->as.dyn.base_name);
                    if (!base_decl) {
                        diagf(arena, diags, e->span, "type: ", "dyn receiver base must be a class");
                        free(arg_types);
                        free(arg_exprs);
                        return (TcType){0};
                    }

                    const EmpClassMethod *best_vm = NULL;
                    int best_cost = 0;
                    bool ambiguous = false;
                    bool saw_named = false;
                    bool saw_virtual = false;

                    for (size_t i = 0; i < base_decl->as.class_decl.methods.len; i++) {
                        const EmpClassMethod *m = (const EmpClassMethod *)base_decl->as.class_decl.methods.items[i];
                        if (!m || m->is_init) continue;
                        if (!slice_eq(m->name, mcall->as.member.member)) continue;
                        saw_named = true;
                        if (!m->is_virtual) continue;
                        saw_virtual = true;
                        if (m->params.len != argc) continue;

                        int cost = 0;
                        bool ok = true;
                        for (size_t ai = 0; ai < argc; ai++) {
                            const EmpParam *p = (const EmpParam *)m->params.items[ai];
                            if (!p || !p->ty) {
                                ok = false;
                                break;
                            }
                            if (arg_types[ai].ty && type_eq_shallow(arg_types[ai].ty, p->ty)) {
                                continue;
                            }
                            if (!tc_arg_compatible(arena, /*diags*/ NULL, fns, env, arg_types[ai], arg_exprs ? arg_exprs[ai] : NULL, p->ty, lenient, io_changed)) {
                                ok = false;
                                break;
                            }
                            cost += 1;
                        }

                        if (!ok) continue;

                        if (!best_vm) {
                            best_vm = m;
                            best_cost = cost;
                            ambiguous = false;
                        } else if (cost < best_cost) {
                            best_vm = m;
                            best_cost = cost;
                            ambiguous = false;
                        } else if (cost == best_cost) {
                            ambiguous = true;
                        }
                    }

                    if (!saw_named) {
                        diagf(arena, diags, e->span, "type: ", "unknown method");
                        free(arg_types);
                        free(arg_exprs);
                        return (TcType){0};
                    }
                    if (saw_named && !saw_virtual) {
                        diagf(arena, diags, e->span, "type: ", "cannot call non-virtual method on dyn receiver");
                        free(arg_types);
                        free(arg_exprs);
                        return (TcType){0};
                    }
                    if (!best_vm || ambiguous) {
                        diagf(arena, diags, e->span, "type: ", (!best_vm) ? "no matching overload" : "ambiguous overload");
                        free(arg_types);
                        free(arg_exprs);
                        return (TcType){0};
                    }

                    // Validate args against chosen virtual method.
                    for (size_t i = 0; i < argc; i++) {
                        const EmpParam *p = (const EmpParam *)best_vm->params.items[i];
                        if (!p || !p->ty) continue;
                        if (!tc_arg_compatible(arena, diags, fns, env, arg_types[i], arg_exprs ? arg_exprs[i] : NULL, p->ty, lenient, io_changed)) {
                            diagf(arena, diags, arg_exprs[i] ? arg_exprs[i]->span : e->span, "type: ", "argument type mismatch");
                        }
                    }

                    e->as.call.resolved_name = (EmpSlice){0};
                    e->as.call.dyn_method = best_vm;
                    e->as.call.dyn_base_name = recv->as.dyn.base_name;
                    e->as.call.dyn_slot = dyn_slot_for_base_method(base_decl, best_vm);

                    free(arg_types);
                    free(arg_exprs);
                    return (TcType){ .ty = best_vm->ret_ty, .lit = TC_LIT_NONE };
                }

                if (!recv || recv->kind != EMP_TYPE_NAME) {
                    diagf(arena, diags, e->span, "type: ", "method call base must be a named type (or pointer to one) or dyn");
                    free(arg_types);
                    free(arg_exprs);
                    return (TcType){0};
                }

                const EmpClassMethod *best_cm = NULL;
                const EmpImplMethod *best_im = NULL;
                int best_cost = 0;
                bool ambiguous = false;
                bool any_named = false;

                // 1) Consider impl methods first (they override class methods on equal rank).
                for (size_t i = 0; i < g_tc_program->items.len; i++) {
                    const EmpItem *it = (const EmpItem *)g_tc_program->items.items[i];
                    if (!it || it->kind != EMP_ITEM_IMPL) continue;
                    if (it->as.impl_decl.trait_name.ptr && it->as.impl_decl.trait_name.len) continue;
                    if (!slice_eq(it->as.impl_decl.target_name, recv->as.name)) continue;
                    for (size_t j = 0; j < it->as.impl_decl.methods.len; j++) {
                        const EmpImplMethod *m = (const EmpImplMethod *)it->as.impl_decl.methods.items[j];
                        if (!m) continue;
                        if (!slice_eq(m->name, mcall->as.member.member)) continue;
                        any_named = true;
                        if (m->params.len != argc) continue;

                        int cost = 0;
                        bool ok = true;
                        for (size_t ai = 0; ai < argc; ai++) {
                            const EmpParam *p = (const EmpParam *)m->params.items[ai];
                            if (!p || !p->ty) {
                                ok = false;
                                break;
                            }
                            if (arg_types[ai].ty && type_eq_shallow(arg_types[ai].ty, p->ty)) {
                                continue;
                            }
                            if (!tc_arg_compatible(arena, /*diags*/ NULL, fns, env, arg_types[ai], arg_exprs ? arg_exprs[ai] : NULL, p->ty, lenient, io_changed)) {
                                ok = false;
                                break;
                            }
                            cost += 1;
                        }

                        if (!ok) continue;

                        if (!best_cm && !best_im) {
                            best_im = m;
                            best_cost = cost;
                            ambiguous = false;
                        } else if (cost < best_cost) {
                            best_cm = NULL;
                            best_im = m;
                            best_cost = cost;
                            ambiguous = false;
                        } else if (cost == best_cost) {
                            // If current best is also impl, ambiguity remains.
                            ambiguous = true;
                        }
                    }
                }

                // 2) Consider class methods (skipping those overridden by impl on same signature).
                const EmpItem *decl = find_named_decl(g_tc_program, recv->as.name);
                if (decl && decl->kind == EMP_ITEM_CLASS) {
                    for (size_t i = 0; i < decl->as.class_decl.methods.len; i++) {
                        const EmpClassMethod *m = (const EmpClassMethod *)decl->as.class_decl.methods.items[i];
                        if (!m || m->is_init) continue;
                        if (!slice_eq(m->name, mcall->as.member.member)) continue;
                        any_named = true;
                        if (m->params.len != argc) continue;
                        if (class_method_overridden_by_impl(g_tc_program, recv->as.name, m)) continue;

                        int cost = 0;
                        bool ok = true;
                        for (size_t ai = 0; ai < argc; ai++) {
                            const EmpParam *p = (const EmpParam *)m->params.items[ai];
                            if (!p || !p->ty) {
                                ok = false;
                                break;
                            }
                            if (arg_types[ai].ty && type_eq_shallow(arg_types[ai].ty, p->ty)) {
                                continue;
                            }
                            if (!tc_arg_compatible(arena, /*diags*/ NULL, fns, env, arg_types[ai], arg_exprs ? arg_exprs[ai] : NULL, p->ty, lenient, io_changed)) {
                                ok = false;
                                break;
                            }
                            cost += 1;
                        }

                        if (!ok) continue;

                        if (!best_cm && !best_im) {
                            best_cm = m;
                            best_cost = cost;
                            ambiguous = false;
                        } else if (cost < best_cost) {
                            best_im = NULL;
                            best_cm = m;
                            best_cost = cost;
                            ambiguous = false;
                        } else if (cost == best_cost) {
                            // Tie: prefer impl over class.
                            if (best_im) {
                                // keep impl
                            } else {
                                ambiguous = true;
                            }
                        }
                    }
                }

                if (!any_named) {
                    diagf(arena, diags, e->span, "type: ", "unknown method");
                    free(arg_types);
                    free(arg_exprs);
                    return (TcType){0};
                }
                if ((!best_cm && !best_im) || ambiguous) {
                    diagf(arena, diags, e->span, "type: ", (!best_cm && !best_im) ? "no matching overload" : "ambiguous overload");
                    free(arg_types);
                    free(arg_exprs);
                    return (TcType){0};
                }

                // Validate args against chosen method and set resolved symbol name if needed.
                const EmpVec *params = best_im ? &best_im->params : &best_cm->params;
                for (size_t i = 0; i < argc; i++) {
                    const EmpParam *p = (const EmpParam *)params->items[i];
                    if (!p || !p->ty) continue;
                    if (!tc_arg_compatible(arena, diags, fns, env, arg_types[i], arg_exprs ? arg_exprs[i] : NULL, p->ty, lenient, io_changed)) {
                        diagf(arena, diags, arg_exprs[i] ? arg_exprs[i]->span : e->span, "type: ", "argument type mismatch");
                    }
                }

                // If this receiver has multiple overloads for this name, record a signature-mangled symbol.
                size_t overloads = tc_count_method_overloads(g_tc_program, recv->as.name, mcall->as.member.member);
                if (overloads > 1) {
                    EmpSlice base = mangle2(arena, recv->as.name, "__", mcall->as.member.member);
                    // params in signature are the user params (not including implicit self).
                    // Build a temporary params array.
                    EmpType **tmp_params = NULL;
                    size_t plen = params ? params->len : 0;
                    if (plen) {
                        tmp_params = (EmpType **)calloc(plen, sizeof(EmpType *));
                        if (tmp_params) {
                            for (size_t i = 0; i < plen; i++) {
                                const EmpParam *p = (const EmpParam *)params->items[i];
                                tmp_params[i] = p ? p->ty : NULL;
                            }
                        }
                    }
                    e->as.call.resolved_name = mangle_overload_name(arena, base, tmp_params, plen);
                    free(tmp_params);
                } else {
                    e->as.call.resolved_name = (EmpSlice){0};
                }

                free(arg_types);
                free(arg_exprs);
                return (TcType){ .ty = best_im ? best_im->ret_ty : best_cm->ret_ty, .lit = TC_LIT_NONE };
            }

            diagf(arena, diags, e->span, "type: ", "only calling ident functions or methods is supported right now");
            return (TcType){0};
        }

        default:
            diagf(arena, diags, e->span, "type: ", "expression kind not typechecked yet");
            return (TcType){0};
    }
}

static void tc_stmt(EmpArena *arena, EmpDiags *diags, TcFns *fns, TcEnv *env, TcFnSig *current_fn_sig, EmpType **fn_ret_slot, EmpStmt *s, int loop_depth, bool *out_terminated, bool lenient, bool *io_changed) {
    if (out_terminated) *out_terminated = false;
    if (!s) return;

    switch (s->kind) {
        case EMP_STMT_TAG:
            return;
        case EMP_STMT_BLOCK: {
            // New scope by snapshotting length.
            size_t mark = env->len;
            for (size_t i = 0; i < s->as.block.stmts.len; i++) {
                EmpStmt *st = (EmpStmt *)s->as.block.stmts.items[i];
                bool term = false;
                tc_stmt(arena, diags, fns, env, current_fn_sig, fn_ret_slot, st, loop_depth, &term, lenient, io_changed);
                if (term) {
                    if (out_terminated) *out_terminated = true;
                    break;
                }
            }
            env->len = mark;
            return;
        }

        case EMP_STMT_VAR: {
            EmpType *decl = s->as.let_stmt.ty;
            TcType init = {0};
            if (s->as.let_stmt.init) {
                const EmpType *expected_init = (decl && decl->kind != EMP_TYPE_AUTO) ? (const EmpType *)decl : NULL;
                init = tc_expr_expected(arena, diags, fns, env, s->as.let_stmt.init, expected_init, lenient, io_changed);
            }

            if (s->as.let_stmt.is_destructure) {
                if (!s->as.let_stmt.init || !init.ty) {
                    diagf(arena, diags, s->span, "type: ", "destructuring let requires an initializer");
                    return;
                }

                // If no explicit type was provided, behave like `auto` and infer from initializer.
                if (!decl || (decl && decl->kind == EMP_TYPE_AUTO)) {
                    s->as.let_stmt.ty = (EmpType *)init.ty;
                    decl = s->as.let_stmt.ty;
                }

                if (!decl || decl->kind != EMP_TYPE_TUPLE) {
                    if (!(lenient && decl && decl->kind == EMP_TYPE_AUTO)) {
                        diagf(arena, diags, s->span, "type: ", "destructuring let expects a tuple type");
                    }
                    return;
                }

                if (init.ty && !type_eq_shallow(init.ty, decl)) {
                    if (!(lenient && (type_is_auto(init.ty) || type_is_auto(decl)))) {
                        diagf(arena, diags, s->span, "type: ", "initializer tuple type mismatch");
                    }
                }

                if (decl->as.tuple.fields.len != s->as.let_stmt.destruct_names.len) {
                    diagf(arena, diags, s->span, "type: ", "tuple destructure arity mismatch");
                    return;
                }

                for (size_t i = 0; i < s->as.let_stmt.destruct_names.len; i++) {
                    const EmpSlice *nm = (const EmpSlice *)s->as.let_stmt.destruct_names.items[i];
                    const EmpTupleField *f = (const EmpTupleField *)decl->as.tuple.fields.items[i];
                    if (!nm || !f) continue;
                    (void)env_push(env, *nm, f->ty);
                }
                return;
            }

            // If no explicit type was provided, behave like `auto` and infer from initializer.
            if (!decl) {
                if (!s->as.let_stmt.init || !init.ty) {
                    diagf(arena, diags, s->span, "type: ", "missing type annotation (cannot infer without initializer)");
                } else {
                    s->as.let_stmt.ty = (EmpType *)init.ty;
                    decl = s->as.let_stmt.ty;
                }
            }

            if (decl && decl->kind == EMP_TYPE_AUTO) {
                if (!s->as.let_stmt.init || !init.ty) {
                    diagf(arena, diags, s->span, "type: ", "cannot infer type for auto binding");
                } else {
                    // Rewrite `auto` to inferred type.
                    s->as.let_stmt.ty = (EmpType *)init.ty;
                    decl = s->as.let_stmt.ty;
                }
            }

            if (decl && s->as.let_stmt.init && init.ty) {
                bool ok = can_coerce(init, decl);
                if (!ok && decl->kind == EMP_TYPE_TUPLE && s->as.let_stmt.init && s->as.let_stmt.init->kind == EMP_EXPR_TUPLE) {
                    ok = tc_check_tuple_literal_against_type(arena, diags, fns, env, s->as.let_stmt.init, decl, lenient, io_changed);
                }
                if (!ok) {
                    diagf(arena, diags, s->span, "type: ", "initializer type mismatch");
                }
            }

            (void)env_push(env, s->as.let_stmt.name, decl);
            return;
        }

        case EMP_STMT_EXPR:
            (void)tc_expr(arena, diags, fns, env, s->as.expr.expr, lenient, io_changed);
            return;

        case EMP_STMT_RETURN: {
            const EmpType *fn_ret = fn_ret_slot ? *fn_ret_slot : NULL;

            if (fn_ret && type_is_auto(fn_ret)) {
                if (!s->as.ret.value) {
                    diagf(arena, diags, s->span, "type: ", "cannot infer auto return type without a return value");
                } else {
                    TcType v = tc_expr_expected(arena, diags, fns, env, s->as.ret.value, fn_ret, lenient, io_changed);
                    if (v.ty && !type_is_auto(v.ty)) {
                        if (!fn_ret_slot) {
                            // should be unreachable; keep safe
                        } else if (!*fn_ret_slot || type_is_auto(*fn_ret_slot)) {
                            *fn_ret_slot = (EmpType *)v.ty;
                            if (current_fn_sig) current_fn_sig->ret = *fn_ret_slot;
                            if (io_changed) *io_changed = true;
                        } else {
                            // already inferred; ensure consistency
                            if (!type_eq_shallow(*fn_ret_slot, v.ty)) {
                                diagf(arena, diags, s->span, "type: ", "auto return inferred to conflicting types");
                            }
                        }
                    }
                }
                if (out_terminated) *out_terminated = true;
                return;
            }

            if (type_is_void(fn_ret)) {
                if (s->as.ret.value) {
                    // If no explicit return type was provided (represented as NULL), infer from the first return value.
                    TcType v = tc_expr_expected(arena, diags, fns, env, s->as.ret.value, fn_ret_slot ? *fn_ret_slot : NULL, lenient, io_changed);
                    if (v.ty && fn_ret_slot && *fn_ret_slot == NULL) {
                        *fn_ret_slot = (EmpType *)v.ty;
                        if (current_fn_sig) current_fn_sig->ret = *fn_ret_slot;
                        if (io_changed) *io_changed = true;
                    } else if (fn_ret_slot && *fn_ret_slot != NULL) {
                        // return type was inferred earlier; validate against it
                        bool ok = can_coerce(v, *fn_ret_slot);
                        if (!ok && *fn_ret_slot && (*fn_ret_slot)->kind == EMP_TYPE_TUPLE && s->as.ret.value && s->as.ret.value->kind == EMP_EXPR_TUPLE) {
                            ok = tc_check_tuple_literal_against_type(arena, diags, fns, env, s->as.ret.value, *fn_ret_slot, lenient, io_changed);
                        }
                        if (!ok) {
                            diagf(arena, diags, s->span, "type: ", "return type mismatch");
                        }
                    } else {
                        diagf(arena, diags, s->span, "type: ", "returning a value from void function");
                    }
                }
            } else {
                if (!s->as.ret.value) {
                    diagf(arena, diags, s->span, "type: ", "missing return value");
                } else {
                    TcType v = tc_expr_expected(arena, diags, fns, env, s->as.ret.value, fn_ret, lenient, io_changed);
                    bool ok = can_coerce(v, fn_ret);
                    if (!ok && fn_ret && fn_ret->kind == EMP_TYPE_TUPLE && s->as.ret.value && s->as.ret.value->kind == EMP_EXPR_TUPLE) {
                        ok = tc_check_tuple_literal_against_type(arena, diags, fns, env, s->as.ret.value, fn_ret, lenient, io_changed);
                    }
                    if (!ok) {
                        diagf(arena, diags, s->span, "type: ", "return type mismatch");
                    }
                }
            }
            if (out_terminated) *out_terminated = true;
            return;
        }

        case EMP_STMT_IF: {
            TcType cnd = tc_expr(arena, diags, fns, env, s->as.if_stmt.cond, lenient, io_changed);
            if (cnd.ty && !type_is_bool(cnd.ty) && !type_is_int(cnd.ty) && cnd.lit != TC_LIT_BOOL && cnd.lit != TC_LIT_INT && cnd.lit != TC_LIT_INT_ZERO) {
                if (!(lenient && type_is_auto(cnd.ty))) {
                    diagf(arena, diags, s->as.if_stmt.cond ? s->as.if_stmt.cond->span : s->span, "type: ", "if condition must be bool");
                }
            }

            // then/else branches in separate env snapshots.
            size_t mark = env->len;
            bool then_term = false;
            tc_stmt(arena, diags, fns, env, current_fn_sig, fn_ret_slot, s->as.if_stmt.then_branch, loop_depth, &then_term, lenient, io_changed);
            env->len = mark;

            bool else_term = false;
            if (s->as.if_stmt.else_branch) {
                tc_stmt(arena, diags, fns, env, current_fn_sig, fn_ret_slot, s->as.if_stmt.else_branch, loop_depth, &else_term, lenient, io_changed);
            }
            env->len = mark;

            if (out_terminated) *out_terminated = then_term && (s->as.if_stmt.else_branch ? else_term : false);
            return;
        }

        case EMP_STMT_WHILE: {
            TcType cnd = tc_expr(arena, diags, fns, env, s->as.while_stmt.cond, lenient, io_changed);
            if (cnd.ty && !type_is_bool(cnd.ty) && !type_is_int(cnd.ty) && cnd.lit != TC_LIT_BOOL && cnd.lit != TC_LIT_INT && cnd.lit != TC_LIT_INT_ZERO) {
                if (!(lenient && type_is_auto(cnd.ty))) {
                    diagf(arena, diags, s->as.while_stmt.cond ? s->as.while_stmt.cond->span : s->span, "type: ", "while condition must be bool");
                }
            }
            bool term = false;
            tc_stmt(arena, diags, fns, env, current_fn_sig, fn_ret_slot, s->as.while_stmt.body, loop_depth + 1, &term, lenient, io_changed);
            return;
        }

        case EMP_STMT_FOR: {
            // Minimal support:
            // - `for i in a...b { ... }`
            // - `for i, v in arr { ... }` where `arr` is `T[N]` or `T[]`.

            if (s->as.for_stmt.iterable && s->as.for_stmt.iterable->kind == EMP_EXPR_RANGE) {
                const EmpExpr *re = s->as.for_stmt.iterable;
                TcType st = tc_expr(arena, diags, fns, env, re->as.range.start, lenient, io_changed);
                TcType en = tc_expr(arena, diags, fns, env, re->as.range.end, lenient, io_changed);

                bool st_int = (st.lit == TC_LIT_INT) || (st.lit == TC_LIT_INT_ZERO) || (st.ty && type_is_int(st.ty));
                bool en_int = (en.lit == TC_LIT_INT) || (en.lit == TC_LIT_INT_ZERO) || (en.ty && type_is_int(en.ty));
                if (st.ty && !st_int && !(lenient && type_is_auto(st.ty))) {
                    diagf(arena, diags, re->as.range.start ? re->as.range.start->span : re->span, "type: ", "range start must be an int");
                }
                if (en.ty && !en_int && !(lenient && type_is_auto(en.ty))) {
                    diagf(arena, diags, re->as.range.end ? re->as.range.end->span : re->span, "type: ", "range end must be an int");
                }

                if (s->as.for_stmt.val_name.len && !(s->as.for_stmt.val_name.len == 1 && s->as.for_stmt.val_name.ptr[0] == '_')) {
                    diagf(arena, diags, s->span, "type: ", "range for-loops only support a single index binding for now");
                }

                EmpType *idx_ty = make_named(arena, s->span, "i32");

                size_t mark = env->len;
                if (s->as.for_stmt.idx_name.len && !(s->as.for_stmt.idx_name.len == 1 && s->as.for_stmt.idx_name.ptr[0] == '_')) {
                    (void)env_push(env, s->as.for_stmt.idx_name, idx_ty);
                }
                (void)tc_stmt(arena, diags, fns, env, current_fn_sig, fn_ret_slot, s->as.for_stmt.body, loop_depth + 1, NULL, lenient, io_changed);
                env->len = mark;
                return;
            }

            TcType it = tc_expr(arena, diags, fns, env, s->as.for_stmt.iterable, lenient, io_changed);
            if (!it.ty) return;
            if (it.ty->kind != EMP_TYPE_ARRAY && it.ty->kind != EMP_TYPE_LIST) {
                diagf(arena, diags, s->span, "type: ", "for/in only supported for arrays and lists for now");
                return;
            }

            EmpType *idx_ty = make_named(arena, s->span, "i32");
            EmpType *val_ty = it.ty->as.array.elem;

            // Body in its own scope with idx/val bindings.
            size_t mark = env->len;
            if (s->as.for_stmt.idx_name.len && !(s->as.for_stmt.idx_name.len == 1 && s->as.for_stmt.idx_name.ptr[0] == '_')) {
                (void)env_push(env, s->as.for_stmt.idx_name, idx_ty);
            }
            if (s->as.for_stmt.val_name.len && !(s->as.for_stmt.val_name.len == 1 && s->as.for_stmt.val_name.ptr[0] == '_')) {
                (void)env_push(env, s->as.for_stmt.val_name, val_ty);
            }

            (void)tc_stmt(arena, diags, fns, env, current_fn_sig, fn_ret_slot, s->as.for_stmt.body, loop_depth + 1, NULL, lenient, io_changed);
            env->len = mark;
            return;
        }

        case EMP_STMT_BREAK:
        case EMP_STMT_CONTINUE:
            if (loop_depth <= 0) {
                diagf(arena, diags, s->span, "type: ", s->kind == EMP_STMT_BREAK ? "'break' used outside of a loop" : "'continue' used outside of a loop");
            }
            if (out_terminated) *out_terminated = true;
            return;

        case EMP_STMT_MATCH: {
            TcType scr = tc_expr(arena, diags, fns, env, s->as.match_stmt.scrutinee, lenient, io_changed);
            if (!scr.ty) return;

            // Enum-aware match: patterns are `Enum::Variant` or `Enum::Variant(x, y, ...)`.
            const EmpItem *en = NULL;
            if (g_tc_program && scr.ty->kind == EMP_TYPE_NAME) {
                en = find_enum_decl(g_tc_program, scr.ty->as.name);
            }

            bool has_default = false;
            bool all_term = true;

            bool exhaustive = false;
            bool *covered = NULL;
            if (en) {
                covered = (bool *)calloc(en->as.enum_decl.variants.len ? en->as.enum_decl.variants.len : 1, sizeof(bool));
            }

            size_t mark = env->len;
            for (size_t i = 0; i < s->as.match_stmt.arms.len; i++) {
                const EmpMatchArm *a = (const EmpMatchArm *)s->as.match_stmt.arms.items[i];
                if (!a) continue;

                if (a->is_default) {
                    has_default = true;
                } else {
                    if (en) {
                        // Parse enum variant pattern.
                        EmpSlice pat_enum = (EmpSlice){0};
                        EmpSlice pat_variant = (EmpSlice){0};
                        const EmpEnumVariant *v = NULL;
                        size_t vidx = 0;
                        const EmpVec *pargs = NULL;

                        if (a->pat && a->pat->kind == EMP_EXPR_MEMBER && a->pat->as.member.base && a->pat->as.member.base->kind == EMP_EXPR_IDENT) {
                            pat_enum = a->pat->as.member.base->as.lit;
                            pat_variant = a->pat->as.member.member;
                            pargs = NULL;
                        } else if (a->pat && a->pat->kind == EMP_EXPR_CALL && a->pat->as.call.callee && a->pat->as.call.callee->kind == EMP_EXPR_MEMBER) {
                            const EmpExpr *mc = a->pat->as.call.callee;
                            if (mc->as.member.base && mc->as.member.base->kind == EMP_EXPR_IDENT) {
                                pat_enum = mc->as.member.base->as.lit;
                                pat_variant = mc->as.member.member;
                                pargs = &a->pat->as.call.args;
                            }
                        }

                        if (!pat_enum.ptr || !pat_enum.len || !pat_variant.ptr || !pat_variant.len) {
                            diagf(arena, diags, a->pat ? a->pat->span : a->span, "type: ", "unsupported enum match pattern; expected `Enum::Variant` or `Enum::Variant(x, ...)`");
                        } else if (!slice_eq(pat_enum, scr.ty->as.name)) {
                            diagf(arena, diags, a->pat ? a->pat->span : a->span, "type: ", "enum match pattern uses the wrong enum type");
                        } else {
                            v = find_enum_variant(en, pat_variant, &vidx);
                            if (!v) {
                                diagf(arena, diags, a->pat ? a->pat->span : a->span, "type: ", "unknown enum variant in match pattern");
                            } else {
                                size_t want = v->fields.len;
                                size_t got = pargs ? pargs->len : 0;
                                if (want != got) {
                                    diagf(arena, diags, a->pat ? a->pat->span : a->span, "type: ", "enum match pattern arity mismatch");
                                }

                                if (covered && vidx < en->as.enum_decl.variants.len) {
                                    if (covered[vidx]) {
                                        diagf(arena, diags, a->pat ? a->pat->span : a->span, "type: ", "duplicate enum variant in match arms");
                                    }
                                    covered[vidx] = true;
                                }

                                // Introduce payload bindings into the arm body scope.
                                if (pargs && want == got) {
                                    for (size_t bi = 0; bi < pargs->len; bi++) {
                                        const EmpExpr *pe = (const EmpExpr *)pargs->items[bi];
                                        const EmpType *fty = (const EmpType *)v->fields.items[bi];
                                        if (!pe || pe->kind != EMP_EXPR_IDENT) {
                                            diagf(arena, diags, pe ? pe->span : a->span, "type: ", "enum pattern bindings must be identifiers (or '_')");
                                            continue;
                                        }
                                        EmpSlice bname = pe->as.lit;
                                        if (bname.len == 1 && bname.ptr && bname.ptr[0] == '_') continue;
                                        (void)env_push(env, bname, (EmpType *)fty);
                                    }
                                }
                            }
                        }
                    } else {
                        TcType pt = tc_expr(arena, diags, fns, env, a->pat, lenient, io_changed);
                        if (pt.ty && !can_coerce(pt, (EmpType *)scr.ty)) {
                            diagf(arena, diags, a->pat ? a->pat->span : s->span, "type: ", "match pattern type mismatch");
                        }
                    }
                }

                bool term = false;
                tc_stmt(arena, diags, fns, env, current_fn_sig, fn_ret_slot, (EmpStmt *)a->body, loop_depth, &term, lenient, io_changed);
                env->len = mark;
                if (!term) all_term = false;
            }

            if (en) {
                if (has_default) {
                    exhaustive = true;
                } else {
                    exhaustive = true;
                    for (size_t vi = 0; vi < en->as.enum_decl.variants.len; vi++) {
                        if (!covered || !covered[vi]) {
                            exhaustive = false;
                            diagf(arena, diags, s->span, "type: ", "non-exhaustive match: missing enum variant arm");
                            break;
                        }
                    }
                }
            } else {
                exhaustive = has_default;
                if (!has_default) {
                    diagf(arena, diags, s->span, "type: ", "non-exhaustive match: missing else arm");
                }
            }

            free(covered);
            if (out_terminated) *out_terminated = exhaustive && all_term;
            return;
        }

        case EMP_STMT_EMP_OFF:
            // Typechecking still applies inside; @emp off affects ownership/borrow rules.
            tc_stmt(arena, diags, fns, env, current_fn_sig, fn_ret_slot, s->as.emp_off.body, loop_depth, NULL, lenient, io_changed);
            return;

        case EMP_STMT_EMP_MM_OFF:
            // Typechecking still applies inside; @emp mm off affects ownership/borrow/drop rules.
            {
                int prev = g_tc_mm_depth;
                g_tc_mm_depth = prev + 1;
                tc_stmt(arena, diags, fns, env, current_fn_sig, fn_ret_slot, s->as.emp_mm_off.body, loop_depth, NULL, lenient, io_changed);
                g_tc_mm_depth = prev;
            }
            return;

        default:
            return;
    }
}

void emp_sem_typecheck(EmpArena *arena, EmpProgram *program, EmpDiags *diags) {
    if (!arena || !program || !diags) return;

    g_tc_program = program;
    const bool file_mm_off = program_has_emp_mm_off(program);

    // 1) Validate type names (builtins + declared user types).
    validate_type_names_in_program(arena, diags, program);

    // 1b) Validate trait impls (signatures + required methods).
    validate_trait_impls(arena, program, diags);

    // 2) Collect function signatures.
    TcFns fns;
    memset(&fns, 0, sizeof(fns));

    for (size_t i = 0; i < program->items.len; i++) {
        EmpItem *it = (EmpItem *)program->items.items[i];
        if (!it || it->kind != EMP_ITEM_FN) continue;

        TcFnSig sig;
        memset(&sig, 0, sizeof(sig));
        sig.name = it->as.fn.name;
        sig.span = it->as.fn.span;
        sig.ret = it->as.fn.ret_ty; // NULL means void
        sig.decl = it;

        sig.params_len = it->as.fn.params.len;
        if (sig.params_len) {
            sig.params = (EmpType **)calloc(sig.params_len, sizeof(EmpType *));
            if (!sig.params) {
                diagf(arena, diags, it->span, "type: ", "out of memory building signature");
                continue;
            }
            for (size_t j = 0; j < sig.params_len; j++) {
                EmpParam *p = (EmpParam *)it->as.fn.params.items[j];
                sig.params[j] = p ? p->ty : NULL;
            }
        }

        // Detect duplicate overloads (same name + same signature).
        if (fns_has_exact_overload(&fns, sig.name, sig.params, sig.params_len, sig.ret)) {
            diagf(arena, diags, it->span, "type: ", "duplicate function overload");
            free(sig.params);
            continue;
        }

        (void)fns_push(&fns, sig);
    }

    // 3) Typecheck function bodies.
    // 3a) Auto inference pass (no diagnostics): infer monomorphic `auto` params from call sites.
    // Iterate to a fixed point because inference can unlock more inference.
    for (int iter = 0; iter < 8; iter++) {
        bool changed = false;

        for (size_t i = 0; i < program->items.len; i++) {
            EmpItem *it = (EmpItem *)program->items.items[i];
            if (!it || it->kind != EMP_ITEM_FN) continue;
            if (!it->as.fn.body) continue; // extern decl

            TcFnSig *self_sig = fns_find_by_decl(&fns, it);

            TcEnv env;
            memset(&env, 0, sizeof(env));

            for (size_t j = 0; j < it->as.fn.params.len; j++) {
                EmpParam *p = (EmpParam *)it->as.fn.params.items[j];
                if (!p) continue;
                (void)env_push(&env, p->name, p->ty);
            }

            bool term = false;
            g_tc_mm_depth = (file_mm_off ? 1 : 0) + (it->as.fn.is_mm_only ? 1 : 0);
            tc_stmt(arena, /*diags*/ NULL, &fns, &env, self_sig, &it->as.fn.ret_ty, it->as.fn.body, 0, &term, /*lenient*/ true, &changed);
            g_tc_mm_depth = 0;
            if (self_sig) self_sig->ret = it->as.fn.ret_ty;

            env_free(&env);
        }

        // Also infer method return types (no call-site param inference for methods yet).
        {
            EmpSlice self_name = {(const char *)"self", 4};
            (void)self_name;
            for (size_t i = 0; i < program->items.len; i++) {
                EmpItem *it = (EmpItem *)program->items.items[i];
                if (!it) continue;

                if (it->kind == EMP_ITEM_CLASS) {
                    EmpItemClass *cls = &it->as.class_decl;
                    for (size_t mi = 0; mi < cls->methods.len; mi++) {
                        EmpClassMethod *mth = (EmpClassMethod *)cls->methods.items[mi];
                        if (!mth || !mth->body) continue;

                        TcEnv env;
                        memset(&env, 0, sizeof(env));

                        EmpType *self_pointee = make_named(arena, mth->span, slice_to_cstr(arena, cls->name));
                        EmpType *self_ty = make_ptr(arena, mth->span, self_pointee);
                        (void)env_push(&env, (EmpSlice){(const char *)"self", 4}, self_ty);

                        for (size_t j = 0; j < mth->params.len; j++) {
                            EmpParam *p = (EmpParam *)mth->params.items[j];
                            if (!p) continue;
                            (void)env_push(&env, p->name, p->ty);
                        }

                        bool term = false;
                        g_tc_mm_depth = file_mm_off ? 1 : 0;
                        tc_stmt(arena, /*diags*/ NULL, &fns, &env, /*current_fn_sig*/ NULL, &mth->ret_ty, mth->body, 0, &term, /*lenient*/ true, &changed);
                        g_tc_mm_depth = 0;
                        env_free(&env);
                    }
                }

                if (it->kind == EMP_ITEM_IMPL) {
                    EmpItemImpl *imp = &it->as.impl_decl;
                    for (size_t mi = 0; mi < imp->methods.len; mi++) {
                        EmpImplMethod *mth = (EmpImplMethod *)imp->methods.items[mi];
                        if (!mth || !mth->body) continue;

                        TcEnv env;
                        memset(&env, 0, sizeof(env));

                        EmpType *self_pointee = make_named(arena, mth->span, slice_to_cstr(arena, imp->target_name));
                        EmpType *self_ty = make_ptr(arena, mth->span, self_pointee);
                        (void)env_push(&env, (EmpSlice){(const char *)"self", 4}, self_ty);

                        for (size_t j = 0; j < mth->params.len; j++) {
                            EmpParam *p = (EmpParam *)mth->params.items[j];
                            if (!p) continue;
                            (void)env_push(&env, p->name, p->ty);
                        }

                        bool term = false;
                        g_tc_mm_depth = file_mm_off ? 1 : 0;
                        tc_stmt(arena, /*diags*/ NULL, &fns, &env, /*current_fn_sig*/ NULL, &mth->ret_ty, mth->body, 0, &term, /*lenient*/ true, &changed);
                        g_tc_mm_depth = 0;
                        env_free(&env);
                    }
                }
            }
        }

        if (!changed) break;
    }

    // 3b) Strict typecheck pass (emits diagnostics).
    for (size_t i = 0; i < program->items.len; i++) {
        EmpItem *it = (EmpItem *)program->items.items[i];
        if (!it || it->kind != EMP_ITEM_FN) continue;
        if (!it->as.fn.body) continue; // extern decl

        TcFnSig *self_sig = fns_find_by_decl(&fns, it);

        TcEnv env;
        memset(&env, 0, sizeof(env));

        for (size_t j = 0; j < it->as.fn.params.len; j++) {
            EmpParam *p = (EmpParam *)it->as.fn.params.items[j];
            if (!p) continue;
            (void)env_push(&env, p->name, p->ty);
        }

        bool term = false;
        g_tc_mm_depth = (file_mm_off ? 1 : 0) + (it->as.fn.is_mm_only ? 1 : 0);
        tc_stmt(arena, diags, &fns, &env, self_sig, &it->as.fn.ret_ty, it->as.fn.body, 0, &term, /*lenient*/ false, /*io_changed*/ NULL);
        g_tc_mm_depth = 0;
        if (self_sig) self_sig->ret = it->as.fn.ret_ty;

        if (it->as.fn.ret_ty && type_is_auto(it->as.fn.ret_ty)) {
            diagf(arena, diags, it->as.fn.span, "type: ", "cannot infer auto return type (add an explicit return type)");
        }

        // If function has a non-void return, require a terminating return on all paths (very conservative for now).
        if (!type_is_void(it->as.fn.ret_ty) && !term) {
            diagf(arena, diags, it->as.fn.span, "type: ", "missing return (not all paths return yet)");
        }

        env_free(&env);
    }

    // 4) Typecheck class + impl method bodies (MVP OOP: implicit `self: *Type`).
    {
        EmpSlice self_name = {(const char *)"self", 4};

        for (size_t i = 0; i < program->items.len; i++) {
            EmpItem *it = (EmpItem *)program->items.items[i];
            if (!it) continue;

            if (it->kind == EMP_ITEM_CLASS) {
                const EmpItemClass *cls = &it->as.class_decl;

                for (size_t mi = 0; mi < cls->methods.len; mi++) {
                    EmpClassMethod *mth = (EmpClassMethod *)cls->methods.items[mi];
                    if (!mth || !mth->body) continue;

                    TcEnv env;
                    memset(&env, 0, sizeof(env));

                    // self: *ClassName
                    EmpType *self_pointee = make_named(arena, mth->span, slice_to_cstr(arena, cls->name));
                    EmpType *self_ty = make_ptr(arena, mth->span, self_pointee);
                    (void)env_push(&env, self_name, self_ty);

                    // user params
                    for (size_t j = 0; j < mth->params.len; j++) {
                        EmpParam *p = (EmpParam *)mth->params.items[j];
                        if (!p) continue;
                        (void)env_push(&env, p->name, p->ty);
                    }

                    bool term = false;
                    g_tc_mm_depth = file_mm_off ? 1 : 0;
                    tc_stmt(arena, diags, &fns, &env, /*current_fn_sig*/ NULL, &mth->ret_ty, mth->body, 0, &term, /*lenient*/ false, /*io_changed*/ NULL);
                    g_tc_mm_depth = 0;

                    if (mth->ret_ty && type_is_auto(mth->ret_ty)) {
                        diagf(arena, diags, mth->span, "type: ", "cannot infer auto return type (add an explicit return type)");
                    }

                    if (!type_is_void(mth->ret_ty) && !term) {
                        diagf(arena, diags, mth->span, "type: ", "missing return (not all paths return yet)");
                    }

                    env_free(&env);
                }
            }

            if (it->kind == EMP_ITEM_IMPL) {
                const EmpItemImpl *imp = &it->as.impl_decl;

                for (size_t mi = 0; mi < imp->methods.len; mi++) {
                    EmpImplMethod *mth = (EmpImplMethod *)imp->methods.items[mi];
                    if (!mth || !mth->body) continue;

                    TcEnv env;
                    memset(&env, 0, sizeof(env));

                    EmpType *self_pointee = make_named(arena, mth->span, slice_to_cstr(arena, imp->target_name));
                    EmpType *self_ty = make_ptr(arena, mth->span, self_pointee);
                    (void)env_push(&env, self_name, self_ty);

                    for (size_t j = 0; j < mth->params.len; j++) {
                        EmpParam *p = (EmpParam *)mth->params.items[j];
                        if (!p) continue;
                        (void)env_push(&env, p->name, p->ty);
                    }

                    bool term = false;
                    g_tc_mm_depth = file_mm_off ? 1 : 0;
                    tc_stmt(arena, diags, &fns, &env, /*current_fn_sig*/ NULL, &mth->ret_ty, mth->body, 0, &term, /*lenient*/ false, /*io_changed*/ NULL);
                    g_tc_mm_depth = 0;

                    if (mth->ret_ty && type_is_auto(mth->ret_ty)) {
                        diagf(arena, diags, mth->span, "type: ", "cannot infer auto return type (add an explicit return type)");
                    }

                    if (!type_is_void(mth->ret_ty) && !term) {
                        diagf(arena, diags, mth->span, "type: ", "missing return (not all paths return yet)");
                    }

                    env_free(&env);
                }
            }
        }
    }

    fns_free(&fns);

    g_tc_program = NULL;
}
