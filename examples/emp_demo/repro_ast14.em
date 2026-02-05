use * from net;
use {Point} from geo.point;
use @ from internal.hidden;

fn main() {
  let a: i32 = connect();
  let b: i32 = get();
  a;
  b;

  let s: auto = secret();
  s;

  SECRET_CODE;

  let p: auto = new Point(1, 2);
  p.sum();
}
