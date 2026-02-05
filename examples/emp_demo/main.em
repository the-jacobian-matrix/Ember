// Package import: pulls exports from every *.em in the folder.
use * from net;

// Explicit imports:
use {Point} from geo.point;
use {Drawable} from geo.drawable;

// "all" import: pulls even non-exported symbols.
use @ from internal.hidden;

// Const import (export required for list import):
use {BUILD_NUMBER} from internal.public;

// Extern + unsafe:
use {c_print, c_add} from ffi.c;
use {raw_add as add_fast} from umath.math;

fn borrow_demo() {
  let x: auto = 1;
  let r: auto = &x;
  r;
}

fn main() {
  // Package import proved here.
  let a: i32 = connect();
  let b: i32 = get();
  a;
  b;

  // `use @ from internal.hidden;` proved here.
  let s: auto = secret();
  s;

  // Const imports proved here.
  BUILD_NUMBER;
  SECRET_CODE;

  let p: auto = new Point(1, 2);
  p.sum();

  borrow_demo();

  @emp off {
    c_print("hello from emp_demo");
    // Avoid crossing the @emp off boundary with ownership-sensitive locals.
    let c: auto = c_add(10, 20);
    let d: auto = add_fast(c, 1);
    d;
  }
}
