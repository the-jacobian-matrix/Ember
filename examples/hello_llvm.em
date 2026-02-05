// Minimal end-to-end native exe demo (Windows).
//
// - Uses a raw multiline backtick string (no manual \n escapes).
// - Uses a real OS DLL boundary: kernel32 (GetStdHandle, lstrlenA, WriteFile).
// - Works with EMP's default behavior: building a .exe via LLVM (use --nobin to emit IR).

extern "C" fn GetStdHandle(nStdHandle: i32) -> *u8;
extern "C" fn lstrlenA(s: *u8) -> i32;
extern "C" fn WriteFile(hFile: *u8, lpBuffer: *u8, nNumberOfBytesToWrite: u32, lpNumberOfBytesWritten: *u32, lpOverlapped: *u8) -> i32;

fn main() -> i32 {
  @emp off {
    *u8 h = GetStdHandle(-11);

    *u8 msg = `Hello from EMP (native .exe)!
This is a raw multiline backtick string.
Printed via kernel32.dll WriteFile.
`;

    u32 n = lstrlenA(msg);
    u32 written = 0;
    WriteFile(h, msg, n, &mut written, null);
  }
  return 0;
}
