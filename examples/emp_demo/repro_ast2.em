use {c_print, c_add} from ffi.c;
use {raw_add as add_fast} from umath.math;

fn main() {
  @emp off {
    c_print("hello");
    let c: auto = c_add(10, 20);
    let d: auto = add_fast(c, 1);
    d;
  }
}
