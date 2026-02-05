# EMP Inconsistencies (current as of 2026-02-05)

This is a pragmatic list of **places where EMP has two (or more) ways to do the same thing**, or where docs + implementation don’t line up yet.

## 1) Collections: builtin `T[]` vs stdlib `List`

**What exists today**
- Language/runtime has a builtin dynamic list type: `T[]` with compiler builtins (`list_push`, `list_pop`, …) and deterministic drop freeing.
- There is also a stdlib module `emp_mods/collections/list.em` exporting a `List` struct and functions like `list_new`/`list_len`.

**Why it’s inconsistent**
- Two different “list” concepts share the same mental slot but have different behavior/ergonomics:
  - `T[]` is a first-class language type (drop-managed in safe code).
  - `List` is a placeholder explicit-allocation API (currently even has `data = null` placeholder).
- Naming overlaps (`list_len`) but refers to different things:
  - builtin `list_len(&xs)` returns length of `T[]`.
  - `emp_mods/collections/list.em`’s `list_len(xs: List)` returns `List.len`.

**Recommended direction**
- Pick one user-facing story:
  - Either: make `std::collections` be **thin wrappers around builtin `T[]`** (preferred short-term),
  - Or: deprecate/hide builtin `T[]` and make stdlib `List` the real container (bigger change).

## 2) Strings: method sugar exists, but `std::string` module is empty

**What exists today**
- Builtin `string` type + compiler builtins (`string_len`, `string_clone`, `string_cstr`, parse ops, and now starts/ends/contains/replace).
- Method-call sugar exists (e.g. `s.len()`, `s.starts_with(&p)`).
- Folder exists: `stdlib/emp_mods/string/` but it’s empty.

**Why it’s inconsistent**
- Docs/checklists imply a string module surface (`std::string`) but the only “nice” API is currently compiler sugar, not a discoverable stdlib module.

**Recommended direction**
- Add a small `stdlib/emp_mods/string/strings.em` (or `string.em`) that re-exports the ergonomic APIs (even if they just call the builtins).

## 3) Manual memory management: canonical meaning of `@emp mm off`

**Canonical rule**
- `@emp mm off;` applies to the whole file/module.
- `@emp mm off { ... }` applies only to that block.

**What it does**
- Enables Zig-style manual memory management primitives.
- Disables ownership/borrow enforcement and automatic drop insertion inside the MM-off region.

This is now consistently documented in `mm.md` and `LANGUAGE.md`.

## 4) “No hidden allocation” vs builtin operations allocating implicitly

**Stated goal (docs)**
- `STDLIB.md` + examples emphasize: no hidden costs; allocation explicit.

**Reality**
- `T[]` operations like `push/reserve/insert` will allocate/grow capacity behind the scenes.

**Why it’s inconsistent**
- Users can write safe code that allocates without ever calling an alloc API, which conflicts with the strict reading of “explicit allocation”.

**Recommended direction**
- Clarify the rule:
  - Either: “explicit allocation” only applies to raw alloc/free APIs, not to safe containers,
  - Or: require explicit allocator plumbing (bigger design shift).

## 5) Two “unsafe” knobs: `@emp off` vs `@emp mm off`

**What they mean (docs)**
- `@emp off` = disables ownership/borrow checks (unsafe escape hatch).
- `@emp mm off` = stronger; unlocks manual memory primitives + disables drop insertion.

**Where it’s confusing**
- Users may expect `@emp off` to allow “low-level stuff” including free-like operations.
- In practice, some operations are gated specifically by MM depth (e.g. `list_free`).

**Recommended direction**
- Keep the separation, but ensure every “dangerous” primitive is consistently categorized:
  - FFI-only unsafe -> `@emp off`
  - manual free/alloc -> `@emp mm off`

## 6) String builtin argument expectations: “borrowed string idents (for now)”

**What’s implemented**
- Typecheck for several string builtins currently expects `&<string_ident>` (not arbitrary expressions) and the method-sugar layer tries to auto-borrow identifier args.

**Why it’s inconsistent**
- Surface syntax suggests `s.contains(&("abc".replace(...)))` should be fine, but it will fail if the argument isn’t a simple identifier.

**Recommended direction**
- Either:
  - Make builtins accept general `&string` expressions (requires lvalue/temp handling), or
  - Document the restriction and provide stdlib helpers that name temps (e.g. `let tmp = ...; s.contains(&tmp)`), or
  - Expand method sugar to introduce compiler-generated temporaries (bigger compiler feature).

## 7) Naming conventions: raw builtins vs “friendly” API

**Reality**
- Users can still call `list_push(&mut xs, v)` and `string_len(&s)` even though the preferred style is now `xs.push(v)` and `s.len()`.

**Why it’s inconsistent**
- Two styles coexist and both are “valid”, so examples/tests may mix them (harder to teach and document).

**Recommended direction**
- Decide a preferred style for user code + docs (likely method sugar), and treat raw builtins as “compiler intrinsics” (documented but discouraged).

---

If you want, I can follow up by:
- updating `LANGUAGE.md` to match the current region-based `@emp mm off` behavior, and/or
- rewriting `stdlib/emp_mods/collections/list.em` so it stops conflicting with builtin `list_*` naming.
