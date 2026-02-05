use * from net;
use {Point} from geo.point;
use {Drawable} from geo.drawable;
use @ from internal.hidden;
use {BUILD_NUMBER} from internal.public;
use {c_print, c_add} from ffi.c;
use {raw_add as add_fast} from umath.math;

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

  @emp off {
    c_print("hello");
    let c: auto = c_add(10, 20);
    let d: auto = add_fast(c, 1);
    d;
  }
}
