use {Point} from geo.point;
use {c_print, c_add} from ffi.c;
use {raw_add as add_fast} from umath.math;

fn borrow_demo() {
  let x: auto = 1;
  let r: auto = &x;
  r;
}

fn main() {
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
