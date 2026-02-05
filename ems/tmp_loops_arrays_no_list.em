// Smoke test: fixed-size arrays, indexing, while, for/in (array).

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

  for idx, v in a {
    sum += v;
    sum += a[idx];
  }

  return sum;
}
