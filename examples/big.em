// EMP demo program (larger)
// Exercises: typed decls, lists, tuple returns, for/in, indexing, compound assign.

fn pair(auto a, auto b) -> (int c, int d) {
  return (a, b);
}

fn clamp_int(int x, int lo, int hi) -> int {
  if x < lo { return lo; }
  if x > hi { return hi; }
  return x;
}

fn sum_list(int[] data) -> int32 {
  int32 acc = 0;
  for _, v in data {
    acc += v;
  }
  return acc;
}

fn main() {
  // declarations
  auto answer;
  int32 total = 0;
  int[] data;

  // borrow syntax (semantic rules enforced later)
  let r = &total;
  let rm = &mut total;

  // explicit unsafe escape hatch
  @emp off {
    // raw/unchecked region (ownership/borrowing checks disabled)
    total += 1;
  }

  // a little arithmetic
  total += 1;
  total += 2;
  total += 3;

  // indexing + for loop
  for i, v in data {
    total += v;
    total += data[i];
  }

  // tuple return + call
  let t = pair(10, 20);

  // more calls
  let x = clamp_int(200, 0, 255);
  let s = sum_list(data);

  // keep it simple
  return;
}
