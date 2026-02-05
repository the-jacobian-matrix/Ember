export struct Vec2 { x: auto; y: auto; }
export enum Color { Red; Green; Rgb(i32, i32, i32) }

fn main() {
  let v = 1;
  let ok = true && !false || false;
  match v {
    0 => { 'z'; }
    else => { 'a'; }
  }
  ok;
}
