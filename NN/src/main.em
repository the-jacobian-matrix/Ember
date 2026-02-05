use {println} from std.console;
use {xor_predict} from nn.xor_mlp;

fn check(x0: f64, x1: f64, expected: i32, label: *u8) {
  i32 got = xor_predict(x0, x1);
  if got == expected {
    println(label);
  } else {
    println("FAILED");
  }
}

fn main() -> i32 {
  println(`EMP XOR neural net demo
(module-based; uses a tiny MLP forward pass)
`);
  check(0.0, 0.0, 0, "case 0,0 OK");
  check(1.0, 0.0, 1, "case 1,0 OK");
  check(0.0, 1.0, 1, "case 0,1 OK");
  check(1.0, 1.0, 0, "case 1,1 OK");
  return 0;
}
