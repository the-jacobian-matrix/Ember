# nn_project (EMP)

This project mirrors what `emp new nn_project` generates.

## Build + run (native exe)

From this folder:

- Build: `emp.exe src/main.em`
- Run: `./out/bin/main.exe`

## Emit LLVM IR only

- `emp.exe --nobin --out out/main.ll src/main.em`
