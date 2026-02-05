# EMP Language (Draft)

This document captures the current EMP syntax implemented by the lexer/parser, and the intended memory-safety model.

## Ownership and Borrowing (Compile-time)

EMP uses a compile-time ownership and borrowing system to guarantee memory safety without runtime overhead.

- **Single owner:** Every value has exactly one owner.
- **Moves by default:** Ownership transfers through moves; there is no implicit copying.
- **Deterministic drop:** When an owned value goes out of scope, its destructor (if defined) runs **exactly once**.

### Borrowing

References are created by borrowing, in two forms:

- **Shared borrow:** `&expr` provides read-only access. Any number of shared borrows may coexist.
- **Mutable borrow:** `&mut expr` provides exclusive mutable access. Exactly one mutable borrow may exist, and it may not coexist with any shared borrows.

The compiler statically enforces:

- $N$ shared borrows OR $1$ mutable borrow
- never both at the same time

### Lifetimes

Borrowed references are guaranteed not to outlive the value they reference.

- Lifetimes are inferred automatically from lexical scope and dataflow.
- No explicit lifetime annotations are required in normal code.

> Note: the current implementation includes a **lexical-scope** borrow checker pass.
> Lifetimes are currently scope-based (not last-use/data-flow yet).

## Drop Insertion (Deterministic Destruction)

After parsing, EMP runs a drop insertion pass that rewrites the AST to include explicit `drop <name>` statements.

Current behavior:

- Drops are inserted at the end of scopes (including function parameters at function end).
- Drops are inserted before `return` (so early returns still destruct owned values).
- Moved-from bindings are not dropped.
- No drops are inserted inside `@emp off` blocks.

Manual memory mode (`@emp mm off`) is the current opt-out mechanism:

- Drops are not inserted inside `@emp mm off { ... }` blocks.
- If the file has `@emp mm off;`, drop insertion for the entire module is disabled.

See the dedicated section below and `mm.md` for the full rules.

## `@emp off` (Unsafe Escape Hatch)

For low-level programming and interoperability, EMP provides an explicit escape hatch:

```emp
@emp off {
  // ownership/borrow/lifetime checks disabled here
}
```

Inside an `@emp off` block:

- Ownership, borrowing, and lifetime checks are disabled.
- Values behave like raw machine data with unrestricted copying and aliasing.

### Safe/Unsafe boundary

The boundary between safe code and `@emp off` code is strictly enforced:

- Unsafe code may not leak borrowed references back into safe code (e.g. returning `&x` / `&mut x` from inside `@emp off`, or assigning a borrow created in `@emp off` into an outer binding).
- Unsafe code may not leak invalid ownership states back into safe code (e.g. moving an outer binding inside `@emp off` and then continuing in safe code).

> Note: checks are disabled *inside* `@emp off` blocks, but the compiler validates that unsafe actions do not escape across the boundary.

## `@emp mm off` (Manual Memory Management mode)

`@emp mm off` is EMP's "Zig-level" manual memory mode:

- It enables **manual memory management** (manual allocation/free primitives, pointer arithmetic, etc.).
- It disables EMP's Rust-like safety passes inside the MM-off region:
  - no ownership / borrow enforcement
  - no automatic drop insertion

It can be applied in exactly two ways:

```emp
@emp mm off; // applies to the whole file/module
```

```emp
@emp mm off {
  // applies only to this block
}
```

Notes:

- `@emp off {}` is *not* a replacement for manual memory mode. It is an unsafe escape hatch for ownership/borrow checks (FFI, low-level ops), but manual memory primitives are gated by `@emp mm off`.
- Outside `@emp mm off`, safe code should rely on deterministic drops instead of explicit frees.

See `mm.md` for the detailed spec.

## Arrays vs Lists

EMP has both fixed-size arrays and dynamic lists. Both store multiple values, but they differ in rigidity and flexibility.

- Arrays: `T[n]`
  - Fixed-size and contiguous.
  - Length is chosen up front and does not change.
  - Indexing is $O(1)$.

- Lists: `T[]`
  - Dynamic, growable sequence (array-backed: `{ptr,len,cap}`).
  - Indexing is $O(1)$.
  - `push`/`append` is amortized $O(1)$ (may reallocate when capacity grows).
  - `insert`/`remove` in the middle is $O(n)$ due to shifting.

A mental model:

- An array is like a row of numbered lockers bolted to the floor.
- A list is like a set of boxes you can add/remove as needed.

## Extern / FFI (Declarations Only)

EMP can declare external functions using `extern fn` (optionally with an ABI string). These functions have no body and are considered `unsafe`.

```emp
extern "C" fn rt_write(fd: i32, buf: *u8, len: usize) -> isize;
```

Rules:

- `extern fn` declarations end with `;` (no body).
- `extern "ABI" fn` selects an ABI by name; if omitted, it defaults to the target C ABI.
- `extern fn` is implicitly `unsafe`.
- Calling an `extern`/`unsafe` function is only allowed inside `@emp off { ... }`.

See [ABI.md](ABI.md) for the locked-down EMP ↔ OS ABI (sizes/alignments/layouts) and how it maps directly to LLVM IR types.

EMP also supports `unsafe fn` for user-defined wrappers:

```emp
unsafe fn do_thing() {
  @emp off {
    // unsafe work
  }
}
```

## Strings

EMP currently supports two string literal syntaxes:

- **Escaped strings:** `"..."` (single-line)
  - Supports escapes like `\n`, `\r`, `\t`, `\\`, `\"`, `\0`.
  - Newlines are not allowed directly.
- **Raw multiline strings:** `` `...` ``
  - Allows newlines directly (no manual `\n` needed).
  - No escapes are processed (backslashes are literal).

### Interpolated strings (f-strings)

EMP reserves `$` as the f-string prefix:

- `$"Hello {name}"`
- `` $`Hello {name}` ``

Placeholders use `{expr}`.

Current status:

- The lexer/parser/AST/JSON support f-strings.
- Compile-time folding exists for literal placeholders like `{123}` and `{"text"}`.
- Runtime lowering for `{ident}` / general `{expr}` is not implemented yet.

## CLI
- Build a native exe (default when a file is provided): `emp.exe file.em`
- Emit LLVM IR only: `emp.exe --nobin --out out.ll file.em`
- Emit LLVM IR (alias): `emp.exe --ll --out out.ll file.em`
- Parse+print AST: `emp.exe --ast file.em`
- Emit JSON: `emp.exe --json --out out.json file.em`

EMP source files use the `.em` extension. If you pass a path with no extension, `.em` is appended.

For convenience (and for the bundled tests), EMP also accepts `.em` files wrapped in a single Markdown code fence:

````text
```plaintext
fn main() {
  return;
}
```
````
