# EMP Compiler (Windows x64)

EMP is a small C + LLVM-based compiler for the `.em` language in this repo.

This README covers:

- How to run EMP on Windows
- How to build from source
- How LLVM is handled 
- How native `.exe` output works **without requiring the Windows SDK**

## Status (what works)

- The compiler runs on **Windows x64**.
- The repo test suite currently passes on Windows x64 (see `tests/run_tests.ps1`).
- EMP can:
  - tokenize (`--lex`)
  - parse and print AST (`--ast`)
  - emit JSON AST + diagnostics (`--json`)
  - emit LLVM IR (`--ll` / `--nobin`)
  - produce a native Windows `.exe` (default mode when you pass a file)
  - run via LLVM `lli` (`--run`)

EMP is **not** a finished, cross-platform, stable language yet, but it is usable on Windows.

## Quick start (using the portable zip)

If you have a prebuilt zip (form the releases), extract it and run:

- Run a program via JIT (no link step):
  - `emp.exe --run path\to\program.em`

- Emit LLVM IR:
  - `emp.exe --nobin --out out\program.ll path\to\program.em`

- Build a native Windows executable:
  - `emp.exe path\to\program.em`
  - Output goes to `out\bin\<name>.exe` by default

### Runtime prerequisites

On some machines, Windows may require the **Microsoft Visual C++ Redistributable (x64)** for `emp.exe` to start.

## First program (recommended)

This is the easiest way to get a working project layout.

### 1) Create a project

From the folder that contains `emp.exe`:

- `emp.exe new hello_emp`
- `cd hello_emp`

This creates:

- `emp.toml`
- `src\main.em`
- `emp_mods\` (for vendored modules)
- `.gitignore`

### 2) Paste a tiny “hello world” (uses the stdlib)

Open `src\main.em` and replace it with:

```emp
use {println} from std.console;

fn main() -> i32 {
  println("Hello from EMP!");
  return 0;
}
```

### 3) Build + run

- Build a native Windows executable:
  - `..\emp.exe src\main.em`

- Run it:
  - `./out/bin/main.exe`

If you don’t want to link a `.exe` (or you’re using Wine), you can run via LLVM instead:

- `..\emp.exe --run src\main.em`

Notes:

- `std.console` works because the portable zip bundles `stdlib\` next to `emp.exe`.
- Output exe naming is currently based on the input filename (`main.em` → `main.exe`).

## Building from source (Windows)

### Requirements

- Windows 10/11
- Visual Studio Build Tools 2022 (MSVC)
- CMake

### Configure + build

From the repo root:

- Configure:
  - `cmake --preset make-x64`

- Build (Release recommended for sharing):
  - `cmake --build --preset make-x64-release`

The resulting compiler is:

- `out\build\make-x64\Release\emp.exe`

## LLVM dependency (important)

EMP uses LLVM via the **LLVM C API** and also shells out to LLVM tools (`llc`, `lld-link`, `lli`).

To build without the bundled LLVM folder:

1. Download a Windows x64 LLVM build that includes:
   - headers: `include/llvm-c/Core.h`
   - import lib: `lib/LLVM-C.lib`
   - runtime DLL: `bin/LLVM-C.dll`
   - tools: `bin/llc.exe`, `bin/lld-link.exe`, `bin/lli.exe`, `bin/llvm-dlltool.exe`

2. Extract LLVM somewhere (example):
   - `C:\deps\llvm-21.1.8-windows-amd64-msvc17-msvcrt\...`

3. Point CMake at it by setting the cache variable:
   - `cmake --preset make-x64 -DEMP_LLVM_ROOT=C:\deps\llvm-21.1.8-windows-amd64-msvc17-msvcrt`

That’s it—then run the normal build preset.

## Native `.exe` output WITHOUT the Windows SDK

We **do not** require the Windows SDK to link programs.

Instead, EMP generates and ships a tiny `kernel32.lib` import library, built from a `.def` file:

- `winlib/kernel32.def`

CMake uses `llvm-dlltool.exe` (from LLVM) to generate:

- `kernel32.lib`

…and copies it next to `emp.exe`. When `emp.exe` links your program, it prefers that local `kernel32.lib`.

Design note:

- See `NO_WINDOWS_SDK.md`

## Packaging a zip to share

After building Release, you can create a shareable zip like this:

- `powershell -ExecutionPolicy Bypass -File tools\package.ps1 -BuildDir out/build/make-x64 -Config Release`

This produces:

- `dist\emp-win64\` folder
- `dist\emp-win64.zip`

The zip includes everything needed to run EMP and build `.exe` files **without** the Windows SDK:

- `emp.exe`
- `LLVM-C.dll`
- `llc.exe`, `lld-link.exe`, `lli.exe`
- `kernel32.lib`
- bundled `stdlib/`

## Running tests

- `powershell -ExecutionPolicy Bypass -File tests\run_tests.ps1`

## Documentation

- Markdown docs: `docs/`
- Viewer (serve the repo root):
  - `python -m http.server 8000`
  - open: `http://localhost:8000/docs/viewer/`

## Wine (Linux/macOS)

- Running `emp.exe` under Wine may work.
- If `emp.exe` doesn’t start, your Wine prefix may need the MSVC runtime (often via `winetricks vcrun2022`).
- Building native `.exe` should work under Wine **as long as** the zip includes `kernel32.lib` next to `emp.exe`.

## Troubleshooting

- **"LLVM backend not enabled"**: you built without LLVM-C available.
  - Fix: provide LLVM and set `EMP_LLVM_ROOT` at configure time.

- **"LLVM tools missing"** (llc/lld-link/lli):
  - Fix: keep `llc.exe`, `lld-link.exe`, `lli.exe` next to `emp.exe` (the packager does this).

- **Link errors mentioning missing Win32 symbols**:
  - Fix: add the missing symbol to `winlib/kernel32.def`, rebuild so `kernel32.lib` is regenerated.
  - If the API is not in `kernel32.dll`, we’ll add another import lib (e.g. `user32`).




