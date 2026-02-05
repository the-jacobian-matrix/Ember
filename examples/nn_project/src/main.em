use {println} from std.console;

// Tiny neural net demo: XOR using a 2-layer perceptron network.
//
// Implemented with integer math (scaled thresholds) so we don't need floats yet.

fn step_gt(v: i32, thr: i32) -> i32 {
  if v > thr { return 1; }
  else { return 0; }
}

fn xor_nn(x1: i32, x2: i32) -> i32 {
  // Hidden: h1 = OR, h2 = AND (scaled by 2 to avoid 0.5).
  i32 sum2 = 2 * (x1 + x2);
  i32 h1 = step_gt(sum2, 1);
  i32 h2 = step_gt(sum2, 3);

  // Output: y = h1 - 2*h2 - 0.5  (scaled: 2*h1 - 4*h2 > 1)
  i32 v = 2 * h1 - 4 * h2;
  return step_gt(v, 1);
}

fn check(x1: i32, x2: i32, expected: i32, label: *u8) {
  i32 got = xor_nn(x1, x2);
  if got == expected {
    println(label);
  } else {
    println("FAILED");
  }
}

fn main() -> i32 {
  println(`EMP XOR neural net demo
(no floats yet; using integer thresholds)
`);
  check(0, 0, 0, "case 0,0 OK");
  check(1, 0, 1, "case 1,0 OK");
  check(0, 1, 1, "case 0,1 OK");
  check(1, 1, 0, "case 1,1 OK");
  return 0;
}
