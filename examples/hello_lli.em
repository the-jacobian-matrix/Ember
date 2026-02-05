// LLVM-IR + lli demo (no linking).
//
// This uses the C runtime (msvcrt) symbol `puts`, which `lli.exe` can typically resolve.
// For a true native .exe output, prefer examples/hello_llvm.em (kernel32 + lld-link).

extern "C" fn puts(s: *u8) -> i32;

fn main() -> i32 {
  @emp off {
    puts("Hello from EMP -> LLVM IR -> lli (msvcrt puts)");
  }
  return 0;
}
