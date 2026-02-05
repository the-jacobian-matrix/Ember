use * from net;
use {Point} from geo.point;
use @ from internal.hidden;
use {BUILD_NUMBER} from internal.public;

fn borrow_demo() {
  let x: auto = 1;
  let r: auto = &x;
  r;
}

fn main() {
  let a: i32 = connect();
  let b: i32 = get();
  a;
  b;

  let s: auto = secret();
  s;

  BUILD_NUMBER;
  SECRET_CODE;

  let p: auto = new Point(1, 2);
  p.sum();

  borrow_demo();
}
