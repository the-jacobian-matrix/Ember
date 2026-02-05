// mem_test.em
// Minimal EMP program to test compilation

// stub print function
fn print(s: *u8) {
    // do nothing for now
    @emp off { }
}

// stub println function
fn println(s: *u8) {
    print(s);
    @emp off { }  // newline placeholder
}

fn main() {
    println("Hello, EMP!");
    return;
}
