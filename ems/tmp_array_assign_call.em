// Smoke test: direct array assignment + passing arrays to functions.

fn first(i32[4] x) -> i32 {
  return x[0];
}

fn main() -> i32 {
  i32[4] a;
  i32[4] b;

  a[0] = 7;
  a[1] = 8;
  a[2] = 9;
  a[3] = 10;

  b = a;
  return first(b);
}
