fn main() -> i32 {
    let x: i32 = 1;
    let y: i32 = 0;

    y = y | (x << 3);
    y = y ^ 2;

    let r: i32 = 10 % 3;
    let z: i32 = ~r;

    if (y != 0 && z != 0) {
        return y + r;
    } else {
        return 0;
    }
}
