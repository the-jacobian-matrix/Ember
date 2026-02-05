// Smoke test: list allocation + push + for/in + free.

fn main() -> i32 {
  i32[] xs;

  list_push(&mut xs, 10);
  list_push(&mut xs, 20);
  list_reserve(&mut xs, 16);
  list_push(&mut xs, 5);

  i32 sum = 0;
  for _, v in xs {
    sum += v;
  }

  @emp mm off {
    list_free(&mut xs);
  }
  return sum;
}
