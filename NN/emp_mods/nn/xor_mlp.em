use {step01} from nn.activations;

// A tiny, explicit MLP for XOR:
// - input: 2 floats (0.0 or 1.0)
// - hidden: 2 step units approximating OR and AND
// - output: 1 step unit computing XOR from hidden activations
//
// This file is intentionally written using only features that EMP already
// codegens well today: locals, fixed-size arrays, indexing, while, if.

export fn xor_forward(x0: f64, x1: f64) -> f64 {
  // 2->2->1 MLP, expressed with fixed indices only.
  //
  // Note: the current parser has a var-decl lookahead heuristic that can
  // misinterpret statements like `h[j] = ...;` as a type+decl attempt
  // (because types use postfix `T[10]`). Keeping indices constant here
  // avoids that ambiguity.

  f64[2] x;
  x[0] = x0;
  x[1] = x1;

  // Hidden layer weights/biases (step activations):
  // - neuron 0 approximates OR:  step(x0 + x1 - 0.5)
  // - neuron 1 approximates AND: step(x0 + x1 - 1.5)
  f64[2] h;

  f64 s0 = x[0] * 1.0 + x[1] * 1.0 + (-0.5);
  f64 s1 = x[0] * 1.0 + x[1] * 1.0 + (-1.5);
  h[0] = step01(s0);
  h[1] = step01(s1);

  // Output layer: step(h0 - 2*h1 - 0.5)
  f64 out = h[0] * 1.0 + h[1] * (-2.0) + (-0.5);
  return step01(out);
}

export fn xor_predict(x0: f64, x1: f64) -> i32 {
  f64 y = xor_forward(x0, x1);
  if y > 0.5 { return 1; }
  else { return 0; }
}
