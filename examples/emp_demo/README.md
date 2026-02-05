# EMP Demo Project

This folder is a **small EMP project** that exercises the features currently implemented in this compiler:

- Multi-file modules (`use ... from ...`)
- Package imports (`use * from net;` imports across a folder)
- `class`, `trait`, `new`, member access/calls (`p.sum()`)
- `@emp off { ... }` blocks (disables EMP safety checks inside)
- `extern fn` declarations + calling externs (must be inside `@emp off`)
- `unsafe fn` + calling unsafe functions (must be inside `@emp off`)

## Quick mental model (what’s real right now)

- `let` declares a local variable.
- `auto` is the only *special* type today: it means “infer/unknown”.
- Names like `i32`, `i8`, `Point` are currently just **type names** in the AST (there’s not a full typechecker yet).
  - So `i32` vs `int32` is **not enforced** by the compiler yet.
  - Use `i32` in this demo because it’s short and consistent.

Also: module path segments must be identifiers. Keywords like `unsafe`, `class`, `trait`, `fn`, etc. can’t be used as package/module names.

## Running (PowerShell on Windows)

Because the path contains spaces, always use the call operator + quotes:

- AST printer:
  - `& "C:\Users\[username]\Pictures\emp\out\build\make\Debug\emp.exe" --ast "C:\Users\[username]\Pictures\emp\examples\emp_demo\main.em"`

- JSON output:
  - `& "C:\Users\[username]\Pictures\emp\out\build\make\Debug\emp.exe" --json "C:\Users\[username]\Pictures\emp\examples\emp_demo\main.em"`

You should get `"diags":[]` in JSON mode.

## What each module demonstrates

- `main.em`
  - `use * from net;` (package import)
  - `use {Point} from geo.point;` (explicit import of a class)
  - `use {Drawable} from geo.drawable;` (explicit import of a trait)
  - `use @ from internal.hidden;` (imports non-exported `secret()`)
  - `extern` + `unsafe` calls are wrapped in `@emp off { ... }`

- `net/socket.em`, `net/http.em`
  - exports used by `use * from net;`

- `geo/point.em`, `geo/drawable.em`
  - exported `class` and `trait`

- `internal/hidden.em`
  - non-exported function imported via `use @ from internal.hidden;`

- `ffi/c.em`
  - extern declarations only (no body)

- `unsafe/math.em`
  - `unsafe fn` that must be called from `@emp off`
