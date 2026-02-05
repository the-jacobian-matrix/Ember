fn relu(f64 x) -> f64 {
  if x > 0.0 { return x; } else { return 0.0; }
}

fn main() -> i32 {
  f64 a = relu(-1.0);
  if a > 0.0 { return 1; } else { return 0; }
}
