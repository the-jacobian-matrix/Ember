export fn relu(x: f64) -> f64 {
  if x < 0.0 { return 0.0; }
  else { return x; }
}

export fn step01(x: f64) -> f64 {
  if x > 0.0 { return 1.0; }
  else { return 0.0; }
}
