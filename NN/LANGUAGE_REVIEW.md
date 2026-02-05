# EMP NN Language Review (NN project)

This folder was generated via `emp new NN`, then extended with `emp_mods/nn/*` as a realistic “what would a NN library want?” exercise.

## What works today (in this project)

- Modules under `emp_mods/` and imports like `use {xor_predict} from nn.xor_mlp;`
- Fixed-size arrays + indexing (`f64[4] w; w[i] = ...;`)
- `while` loops, `if/else`, locals, numeric ops, float compares
- Pointers inside `@emp off` blocks (used in `nn.xor_mlp` to emulate multi-return)

Runnable demo entrypoint:
- `src/main.em`

Key module:
- `emp_mods/nn/xor_mlp.em`

## “Ideal” NN API sketch

See:
- `emp_mods/nn/ideal_api.em`

This intentionally sketches the shape of a real library (tensors, shapes, dense layers), and acts as a concrete driver for missing features.

## Missing / next features (most impactful for real NN)

### Language

- Range loops (ergonomics + correctness):
  - want: `for i in 0..n { ... }` and `for i in 0..=n { ... }`
- General lvalues in codegen (core blocker):
  - field assignment: `t.len = ...`
  - deref assignment outside `@emp off`: `*ptr = ...`
  - nested lvalues: `a[i].field = ...`
- Struct values end-to-end:
  - struct literals / constructors
  - passing/returning structs, field reads/writes, copying/moving semantics
- Multi-return / tuples:
  - removes the need for pointer out-params in safe code
- Slices/spans as a first-class safe type:
  - `Slice<T> { data: *T, len: usize }`
  - safe indexing helpers + iteration
- Generics (or a usable monomorphization strategy):
  - `Tensor<T>`, `Slice<T>`, `Result<T,E>`

### Stdlib

- `std.mem` allocation layer:
  - `alloc`, `realloc`, `free` (plus typed helpers)
- `std.math`:
  - `exp`, `log`, `tanh`, `sqrt` (needed for sigmoid/softmax, losses, optimizers)
- `std.rand`:
  - deterministic PRNG for initialization

## Suggested next implementation order (to unlock a real NN)

1. Codegen support for non-identifier lvalues (field + deref + nested)
2. Struct codegen + struct literals
3. Range `for` (`0..n`) and `usize`-friendly indexing
4. `Slice<T>` + iteration and bounds utilities
5. Minimal `std.mem` allocator wrappers
6. Minimal `std.math` (just enough for sigmoid/softmax)
