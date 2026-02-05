fn id(p: auto) -> auto {
    return p;
}

fn main() -> i32 {
    let s: *u8 = "hi";
    let t: auto = id(s);
    t;
    return 0;
}
