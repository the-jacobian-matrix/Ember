// Smoke test: list default init + for/in + indexing.

fn main() -> i32 {
  i32 sum = 0;

  i32[] data;
  for _, v in data {
    sum += v;
  }

  return sum;
}
