# EMP Manual Memory Management Mode (`@emp mm off`)

This document specifies EMP’s **Manual Memory Management mode** ("MM mode").

EMP’s default mode is **safe, Rust-like memory management**:
- ownership + borrowing rules are enforced
- deterministic drops are inserted automatically
- low-level allocation/memory primitives are *not* available by default

MM mode is an explicit opt-in that defines *where* those low-level primitives are allowed and where the Rust-like memory model is disabled.

---

## 1) Syntax

MM mode has two forms:

### 1.1 File-wide directive

```emp
@emp mm off;
```

- Applies to the **entire file/module**.
- Must appear at **top-level** (i.e., as a module item).

### 1.2 Block-scoped region

```emp
@emp mm off {
    // ...
}
```

- Applies only within the braces.
- May appear anywhere a statement may appear.

---

## 2) Meaning (High-level)

Inside an MM-off region/file:

- EMP’s **Rust-like memory management model is disabled**.
  - No ownership/borrow enforcement.
  - No automatic drop insertion.
- EMP unlocks **manual memory primitives** ("Zig-style" APIs): raw allocation, raw free, pointer arithmetic, etc.
- The programmer is responsible for:
  - initialization and deinitialization
  - correct freeing
  - avoiding leaks, double-frees, use-after-free, and invalid aliasing

Outside MM-off regions/files:

- Memory primitives are **not allowed**.
- The normal safety model applies.

---

## 3) Relationship to `@emp off` (unsafe blocks)

EMP already supports:

```emp
@emp off {
    // unsafe escape hatch
}
```

`@emp off {}` is an *unsafe* escape hatch primarily for:
- FFI wrappers
- narrowly scoped unsafe operations

`@emp mm off` is stronger:
- It is the **only place** where manual allocation/memory primitives are permitted.
- It disables the Rust-like memory model inside.

Nesting rules:
- `@emp mm off { ... }` behaves like an unsafe region for the purpose of "unsafe depth".
- `@emp off { ... }` *does not* automatically grant access to manual memory primitives; only `@emp mm off` does.
  - (Implementation note: this gating is enforced by the typechecker / call rules, not by codegen.)

---

## 4) Compiler Semantics (what changes)

### 4.1 Ownership / move checking

- **Disabled** inside MM-off.
- Outside MM-off, normal rules apply.

### 4.2 Borrow checking

- **Disabled** inside MM-off.
- Outside MM-off, normal rules apply.

Boundary safety rule (still enforced):
- Values/borrows created in an unsafe/MM-off context must not "smuggle" invalid borrows back into safe code.
  - The exact boundary rule mirrors `@emp off`’s existing escape checks.

### 4.3 Drop insertion

- Automatic drop insertion is **skipped** inside MM-off blocks.
- If the file has `@emp mm off;`, drop insertion for the module is **disabled**.

### 4.4 Typechecking

- Typechecking still runs inside MM-off.
- MM-off changes *what is permitted to be called/used* (manual memory primitives), not whether expressions have types.

---

## 5) Manual memory primitives (“Zig-style functions”)

This section defines the policy; the exact API surface lives in the stdlib.

Rule:
- Any API categorized as a **manual memory primitive** is only legal to call inside an MM-off region or in a file with `@emp mm off;`.

How a function becomes a “manual memory primitive”:
- In EMP source, these functions are declared with the `mm` modifier, typically alongside `unsafe`/`extern`:

```emp
mm extern fn malloc(size: u64) -> *u8;
mm extern fn free(p: *u8);
```

Examples of what qualifies:
- raw allocation / free (`alloc`, `free`)
- reallocation (`realloc`)
- pointer arithmetic, raw pointer reads/writes
- converting between raw pointers and integers

Non-goals:
- Making safe containers unusable. Safe abstractions may exist outside MM-off.

Implementation note (stdlib):
- Manual primitives are implemented in `std.rawmem` (and also available via `std.mm`).
- Convenience wrappers live in `std.mem`.
- Small wrappers are provided in `std.alloc`.

---

## 5.1) Scoped cleanup: `defer { ... }`

EMP supports a scoped cleanup statement:

```emp
defer {
  // runs when the current scope exits
}
```

Semantics:
- Runs on **normal fallthrough** of the scope.
- Runs on **early exits** (`return`, `break`, `continue`) as the scope is unwound.
- Execution order is **LIFO** within a scope (last defer runs first).

This is the intended efficient cleanup mechanism for MM-off code, where automatic drops are disabled.

---

## 6) Step-by-step rollout checklist

1. Parse + AST support for `@emp mm off;` and `@emp mm off {}`.
2. Treat MM-off as an unsafe region in ownership/borrow/drop passes.
3. Add typechecker gating for manual memory primitives.
4. Add tests:
   - parsing: directive + block forms
   - semantics: borrow/move/drop disabled inside MM-off
   - gating: manual primitives rejected outside, accepted inside

---

## 7) Open questions (to decide next)

- The exact naming/location of manual primitives in the stdlib (e.g. `std::alloc::*` vs `std::mm::*`).
- Whether `@emp mm off` should implicitly allow *all* `extern`/`unsafe` calls, or only the manual-memory subset.
- Boundary rules for values produced inside MM-off and used outside (beyond existing borrow-escape constraints).
