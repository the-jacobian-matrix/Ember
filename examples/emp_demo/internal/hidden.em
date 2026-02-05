// Not exported on purpose.
fn secret() -> i32 {
  return 42;
}

// Not exported on purpose; should still be visible via `use @`.
const SECRET_CODE: i32 = 1337;
