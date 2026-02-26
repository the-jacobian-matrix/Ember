use {println} from std.console;

// Thread-ready NN batch inference demo.
//
// Why this file exists:
// - EMP currently has no stable std.thread module in this checkout.
// - So we structure work in 4 shard workers that can be executed sequentially now,
//   then mapped to real threads later with minimal code movement.

fn step01(x: f64) -> f64 {
  if x > 0.0 { return 1.0; }
  else { return 0.0; }
}

// Tiny XOR MLP forward pass (2 -> 2 -> 1)
fn xor_forward(x0: f64, x1: f64) -> f64 {
  f64 s0 = x0 * 1.0 + x1 * 1.0 + (-0.5);
  f64 s1 = x0 * 1.0 + x1 * 1.0 + (-1.5);

  f64 h0 = step01(s0);
  f64 h1 = step01(s1);

  f64 out = h0 * 1.0 + h1 * (-2.0) + (-0.5);
  return step01(out);
}

fn xor_predict(x0: f64, x1: f64) -> i32 {
  f64 y = xor_forward(x0, x1);
  if y > 0.5 { return 1; }
  else { return 0; }
}

// Worker shard: compute ys[start..end)
fn worker_shard(start: i32, end: i32, f64[] x0s, f64[] x1s, i32[] ys) {
  i32 i = start;
  while i < end {
    ys[i] = xor_predict(x0s[i], x1s[i]);
    i = i + 1;
  }
}

// 4-way sharded inference.
//
// Today: called sequentially.
// Future: replace the 4 direct calls with thread spawn/join wrappers.
fn batch_predict_4way(f64[] x0s, f64[] x1s, i32[] ys, i32 n) {
  i32 q = n / 4;

  i32 a0 = 0;
  i32 a1 = q;

  i32 b0 = q;
  i32 b1 = 2 * q;

  i32 c0 = 2 * q;
  i32 c1 = 3 * q;

  i32 d0 = 3 * q;
  i32 d1 = n;

  worker_shard(a0, a1, x0s, x1s, ys);
  worker_shard(b0, b1, x0s, x1s, ys);
  worker_shard(c0, c1, x0s, x1s, ys);
  worker_shard(d0, d1, x0s, x1s, ys);
}

fn count_correct(i32[] ys, i32[] exp, i32 n) -> i32 {
  i32 i = 0;
  i32 ok = 0;
  while i < n {
    if ys[i] == exp[i] {
      ok = ok + 1;
    }
    i = i + 1;
  }
  return ok;
}

fn main() -> i32 {
  println("EMP threaded-ready NN demo (4-way sharding)");

  // Repeat XOR cases in a larger batch so shard boundaries are visible.
  i32 n = 16;

  f64[] x0s = [];
  f64[] x1s = [];
  i32[] exp = [];
  i32[] ys = [];

  i32 i = 0;
  while i < n {
    i32 p = i % 4;

    if p == 0 {
      x0s.push(0.0); x1s.push(0.0); exp.push(0);
    } else if p == 1 {
      x0s.push(1.0); x1s.push(0.0); exp.push(1);
    } else if p == 2 {
      x0s.push(0.0); x1s.push(1.0); exp.push(1);
    } else {
      x0s.push(1.0); x1s.push(1.0); exp.push(0);
    }

    ys.push(0);
    i = i + 1;
  }

  batch_predict_4way(x0s, x1s, ys, n);

  i32 ok = count_correct(ys, exp, n);
  if ok == n {
    println("all predictions OK");
  } else {
    println("some predictions FAILED");
  }

  return 0;
}
