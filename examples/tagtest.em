extern "C" fn GetStdHandle(nStdHandle: i32) -> *u8;
extern "C" fn lstrlenA(s: *u8) -> i32;
extern "C" fn WriteFile(hFile: *u8, lpBuffer: *u8, nNumberOfBytesToWrite: u32, lpNumberOfBytesWritten: *u32, lpOverlapped: *u8) -> i32;

fn print(s: *u8) {
  @emp off {
    *u8 h = GetStdHandle(-11);
    u32 n = lstrlenA(s);
    u32 written = 0;
    WriteFile(h, s, n, &mut written, null);
  }
}

fn println(s: *u8) {
  print(s);
  print("\n");
}

fn main() -> i32 {
    println("Hi");
    #tag
    return 0;
}