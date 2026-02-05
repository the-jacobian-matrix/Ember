// Demo app: lots of f-strings.
// Build+run (native exe): emp.exe examples/fstrings_demo.em ; .\out\bin\fstrings_demo.exe
// Or emit IR only: emp.exe --ll --nobin --out out/fstrings_demo.ll examples/fstrings_demo.em

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
  let who: *u8 = "emp";
  let name: *u8 = "world";
  let a: i32 = 7;
  let b: i32 = 3;
  let ok: bool = true;
  let bad: bool = false;
  let c1:char = 'A';
  println($"=== fstrings demo ({1}) ===");
  println($"hello {who}, {name}!");
  println($"Hi hello is this A? {c1}");
  // math + bools + chars
  println($"a={a} b={b} a+b={a+b} a*b={a*b} ok={ok} bad={bad} ch1={c1} ch2={'Z'}");
  println($"comparisons: (a>0)={(a>0)} (a>10)={(a>10)} (a<b)={(a<b)} (a==b)={(a==b)}");
  println("hi");
  #tag
  // brace escaping + mixed interpolations
  println($"literal braces: {{ and }} and {{ok}} end");
  println($"mix: {{ {a} }} :: {{ {who} }} :: {name} :: done");

  // raw f-string (backticks)
  println($`raw: who={who} a={a}
path: C:\temp\file.txt
literal braces: {{ }}
`);

  // a big one
  println($"BIG: who={who} name={name} a={a} b={b} ok={ok} ch={'A'} a+b={a+b} a*b={a*b} (a<b)={(a<b)}");

  return 0;
}
