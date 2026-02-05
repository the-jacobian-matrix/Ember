use * from net;
use {c_print, c_add} from ffi.c;
use {raw_add as add_fast} from umath.math;

fn main() {
  let a: i32 = connect();
  let b: i32 = get();
  a;
  b;

  @emp off {
    c_print("hello");
    let c: auto = c_add(10, 20);
    let d: auto = add_fast(c, 1);
    d;
  }
}
