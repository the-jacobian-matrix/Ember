use {c_print} from emp_mods.platform.c;

// Safe wrapper: users can call this without `@emp off`.
export fn print(msg: auto) {
  @emp off {
    c_print(msg);
  }
}

export fn println(msg: auto) {
  @emp off {
    c_print(msg);
    c_print("\n");
  }
}
