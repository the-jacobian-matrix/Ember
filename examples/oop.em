export class Point {
  x: i32;
  y: i32;

  init(x: i32, y: i32) {
  }

  fn len() -> i32 {
    return 0;
  }
}

export trait Drawable {
  fn draw();
}

fn main() {
  let p: auto = new Point(1, 2);
  p.len();
}
