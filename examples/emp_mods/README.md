# emp_mods (Standard Library Modules)

This folder is the "standard library" for EMP demos.

Design goals:
- Explicit modules (`use ... from ...`) and explicit exports.
- Value-oriented APIs: structs/enums are plain data; no hidden allocation.
- Allocation is explicit and only available inside manual memory management regions (`@emp mm off`).
- `@emp off` remains an internal escape hatch for FFI/OS calls; safe wrappers should not leak unsafe states.
- No global singletons, no background threads.

Structure (initial):
- `emp_mods/platform/*`  low-level OS/FFI declarations and tiny wrappers
- `emp_mods/io/*`        console and file APIs (thin, explicit)
- `emp_mods/math/*`      pure math helpers (no runtime)
- `emp_mods/collections/*` value-backed collections (allocation explicit)

Note: EMP does not have generics/traits fully wired yet; modules use `auto` + explicit element sizes for now.
