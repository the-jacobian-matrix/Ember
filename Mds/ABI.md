# EMP ABI (OS-Facing Boundary)

This document defines the **canonical EMP ABI** for values crossing the EMP ↔ OS/shared-library boundary.

Goals:
- One canonical, stable ABI for EMP primitives and aggregates.
- ABI lowers directly to LLVM IR types with **no reinterpretation at call sites**.
- `extern` uses the platform C ABI (SysV AMD64 on Linux, Win64 on Windows) so EMP can call OS/library symbols with **zero glue**.

Non-goals (for the first locked ABI version):
- No managed runtime, no GC, no hidden boxing.
- No “fat objects” or implicit vtables across the ABI.

## 1) Canonical Data Layout

Unless otherwise stated, this ABI assumes **little-endian** targets.

### Integers

| EMP type | Size | Align | LLVM IR |
|---|---:|---:|---|
| `i8` / `u8` | 1 | 1 | `i8` |
| `i16` / `u16` | 2 | 2 | `i16` |
| `i32` / `u32` | 4 | 4 | `i32` |
| `i64` / `u64` | 8 | 8 | `i64` |
| `isize` / `usize` | pointer-sized | pointer-sized | `iPTR` (see below) |

`iPTR` is `i64` on 64-bit targets and `i32` on 32-bit targets.

### Floats

| EMP type | Size | Align | LLVM IR |
|---|---:|---:|---|
| `f32` | 4 | 4 | `float` |
| `f64` | 8 | 8 | `double` |

### Booleans

`bool` is a **1-byte** value:
- Size: 1
- Align: 1
- Valid values: `0` (false), `1` (true). Other bit-patterns are allowed to exist in memory but are **not produced** by EMP codegen.
- LLVM IR: `i8`

### Pointers

Raw pointers use the platform pointer size:
- Size: pointer-sized
- Align: pointer-sized

LLVM IR:
- Prefer opaque pointers: `ptr` (addrspace 0)

### Slices

A slice is a non-owning view: pointer + length.

Canonical layout:

```text
struct Slice<T> {
  data: *T,
  len: usize
}
```

LLVM IR (conceptually):
- `{ ptr, iPTR }`

### Strings

A string is UTF-8 bytes (not NUL-terminated by default): pointer + length.

Canonical layout:

```text
struct String {
  data: *u8,
  len: usize
}
```

LLVM IR (conceptually):
- `{ ptr, iPTR }`

Notes:
- Interop with C NUL-terminated strings uses a distinct type (future): `CStr = *u8`.

### Structs

Default EMP struct layout is **C-compatible**:
- Fields appear in declaration order.
- Each field is aligned to its natural ABI alignment.
- Padding is inserted between fields and at the end to satisfy alignment.
- Struct alignment is `max(field_align)`.

LLVM IR:
- Use an LLVM `struct` type with the exact same field sequence and padding implied by the target data layout.

Future (reserved):
- `repr(packed)` and `repr(align(N))`.

### Enums

Canonical enum ABI uses an **explicit tag + payload**.

Default tag:
- `u32` (4-byte tag, 4-byte alignment)

Payload:
- A C-compatible union of the variant payloads, sized/aligned to fit the largest payload.

Canonical layout:

```text
struct Enum {
  tag: u32,
  payload: [u8; PAYLOAD_SIZE]  // aligned to PAYLOAD_ALIGN
}
```

LLVM IR:
- `{ i32, [PAYLOAD_SIZE x i8] }` with explicit alignment on the aggregate per target rules.

Notes:
- “Niche” optimizations (like Rust’s `Option<NonNull<T>>`) are explicitly **not** part of ABI v1.
- If/when niche optimizations exist, they are opt-in via `repr(…)` and never affect the canonical ABI.

## 2) Calling Conventions

EMP `extern` functions use the **platform C ABI**:
- Linux x86_64: SysV AMD64
- Windows x86_64: Win64

This means:
- Argument passing (registers/stack), return-value strategy, and ABI-required shadow space/stack alignment follow the platform.
- EMP does not insert shims/wrappers for ABI purposes.

LLVM lowering rule:
- `extern "C" fn` is emitted as a plain LLVM declaration using the target’s default C calling convention.
- No bitcasts/reinterpretation at the call site; argument LLVM IR types must match the EMP ABI mapping above.

### Error representation

ABI layer does not define exceptions.

Error handling is by value:
- OS/FFI functions can return status codes (C-style).
- Higher-level EMP APIs wrap these into explicit `Result<T, E>`-style structs/enums (library-level), without changing the ABI of the raw extern.

## 3) `extern "abi"` in EMP

Syntax:

```emp
extern "C" fn puts(s: *u8) -> i32;
extern fn clock() -> i64; // defaults to platform C ABI
```

Rules:
- `extern` declarations have no body and end in `;`.
- ABI string is optional; empty means “target default C ABI”.
- Safety and capability gating live **above** this layer (e.g. `@emp off` wrappers in the stdlib).

## 4) Stdlib boundary organization

Recommended layering:
- `emp_mods/platform/*`: raw extern symbols and minimal, target-specific declarations.
- `emp_mods/sys/*`: thin safe wrappers (I/O, time, memory, threads). No hidden allocation; explicit handles.
- Higher-level modules (`io`, `net`, `collections`, etc.) build on `sys`.

## 5) Runtime glue (minimal)

Only unavoidable runtime pieces are allowed:
- Startup (calling `main`), exit status.
- Panic/abort hook.

Everything else is direct EMP → LLVM → OS calls.
