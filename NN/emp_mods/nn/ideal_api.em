// "Ideal" NN module sketch (language review driver)
//
// This file is *not* expected to fully compile to an exe today.
// It exists to make missing EMP features concrete.
//
// Things this wants from EMP/std:
// - heap allocation (std.mem.alloc/free or arena)
// - slices/spans (ptr+len) + safe indexing bounds helpers
// - lvalues beyond identifiers (field assign, *ptr assign) outside @emp off
// - range loops: for i in 0..n
// - basic math: exp/log/tanh
// - generics (Tensor<T>, Slice<T>, Result<T,E>) or at least monomorphized patterns

export struct Shape2 {
  rows: i32;
  cols: i32;
}

export struct TensorF64 {
  data: *f64;
  len: i32;
  shape: Shape2;
}

// Desired API shape (won't work end-to-end without allocator + field lvalues).
export fn tensor_new(shape: Shape2) -> TensorF64 {
  let t: TensorF64;
  // Would like:
  // t.len = shape.rows * shape.cols;
  // t.data = std.mem.alloc_f64(t.len);
  // t.shape = shape;
  return t;
}

export fn dense_forward(/* layer: Dense, */ x: TensorF64) -> TensorF64 {
  // Would like:
  // y = x @ W + b
  // y = relu(y)
  return x;
}
