// Smoke test: fixed-size arrays, list default init, indexing, while, for/in.

fn main() -> i32 {
  i32 sum = 0;

  i32[4] a;
  a[0] = 1;
  a[1] = 2;
  a[2] = 3;
  a[3] = 4;

  i32 i = 0;
  while i < 4 {
    sum += a[i];
    i += 1;
  }

  // List default-init should be safe (len=0), so loop does nothing.
  i32[] data;
  for _, v in data {
    sum += v;
  }

  // Array iteration.
  for idx, v in a {
    sum += v;
    sum += a[idx];
  }

  return sum;
}
