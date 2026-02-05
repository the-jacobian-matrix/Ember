// FFI surface: declarations only.
// Rule in this compiler: calling extern/unsafe requires `@emp off { ... }`.
export extern fn c_print(msg: *i8);
export extern fn c_add(a: i32, b: i32) -> i32;
