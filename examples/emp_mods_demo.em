use {println} from emp_mods.io.console;
use {abs_i32, clamp_i32} from emp_mods.math.basic;
use {List, list_new, list_len, list_demo_add} from emp_mods.collections.list;

export enum Kind {
  Ok;
  Err(i32);
}

fn main() {
  println("emp_mods demo");

  let x: i32 = -42;
  let y: i32 = abs_i32(x);
  y;

  let z: i32 = clamp_i32(500, 0, 255);
  z;

  let xs: List = list_new(4);
  list_len(xs);

  let sum: i32 = list_demo_add(10, 20);
  sum;

  let k = 0;
  match k {
    0 => { println("Ok"); }
    else => { println("Other"); }
  }
}
