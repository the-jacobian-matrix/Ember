// Value-backed list shape.
// No hidden allocation: allocation happens only via explicit functions.

use {c_add} from emp_mods.platform.c;

export struct List {
  data: *auto;
  len: i32;
  cap: i32;
  elem_size: i32;
}

// This is intentionally a tiny, explicit API until generics land.
export fn list_new(elem_size: i32) -> List {
  let out: List;
  // Placeholder: real allocation will be added via platform module.
  out.data = null;
  out.len = 0;
  out.cap = 0;
  out.elem_size = elem_size;
  return out;
}

export fn list_len(xs: List) -> i32{
  return xs.len;
}

// A silly function proving cross-module calls + returns.
export fn list_demo_add(a: i32, b: i32) -> i32 {
  // Safe wrapper around C: internal `@emp off` capability block.
  @emp off {
    return c_add(a, b);
  }
}
