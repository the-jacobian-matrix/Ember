# Project Readout

This repository contains **EMP**, a C + LLVM based compiler and language workspace centered around `.em` source files.

## What this repo is

- Core compiler implementation in C source/header files at the repository root (`main.c`, `emp_lexer.c`, `emp_ast.c`, `emp_typecheck.c`, `emp_borrow.c`, `emp_drop.c`, etc.).
- Language and user docs under `docs/` (getting started, syntax, type system, ownership, modules, enums/match, traits/UFCS, stdlib, testing index, grammar/tokens).
- Example EMP programs under `examples/` and related generated artifacts under `LR/`, `NN/out/`, and some `examples/**/out` directories.
- Historical/auxiliary language notes in `Mds/`.

## High-level architecture (from code/docs)

`main.c` orchestrates compilation flow and CLI command handling. It includes lexer/parser/semantic/typecheck/borrow/drop/codegen interfaces and provides utility logic such as file/path handling and project scaffolding (`emp new`).

The docs describe EMP as:

- Safe-by-default ownership/borrowing language.
- Deterministic destruction via inserted drops.
- Escape hatches for unsafe/manual memory regions (`@emp off`, `@emp mm off`).
- LLVM IR generation via LLVM-C and toolchain binaries (`llc`, `lld-link`, `lli`).

## Notable repo observations

- `README.md` references some files/directories that are not currently present in this checkout (for example: `tests/`, `winlib/`, `NO_WINDOWS_SDK.md`, and a few parser/codegen file names).
- `CMakePresets.json` exists and defines Visual Studio presets (`make`, `make-x64`), but there is no `CMakeLists.txt` in the current checkout.
- This suggests the repo may be a reduced snapshot, or docs are ahead/behind current tracked files.

## Quick navigation

- Start with `README.md` for user-oriented workflow.
- Read `docs/00_overview.md` + `docs/README.md` to understand language intent and how the docs are organized.
- Inspect `main.c` next to see command flow and integration points.
- Use `examples/` to understand practical language syntax/features.
