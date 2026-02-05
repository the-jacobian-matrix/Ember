// Hashmap insert benchmark for EMP (Windows-only timing via QueryPerformanceCounter).
//
// - Open addressing; handles collisions by probing.
// - Probing policy:
//   - Load < 75%: pseudo-random probing (odd step derived from hash)
//   - Load >= 75%: linear probing

extern "C" fn GetStdHandle(nStdHandle: i32) -> *u8;
extern "C" fn lstrlenA(s: *u8) -> i32;
extern "C" fn WriteFile(
  hFile: *u8,
  lpBuffer: *u8,
  nNumberOfBytesToWrite: u32,
  lpNumberOfBytesWritten: *u32,
  lpOverlapped: *u8
) -> i32;

extern "C" fn QueryPerformanceCounter(out: *i64) -> i32;
extern "C" fn QueryPerformanceFrequency(out: *i64) -> i32;

fn write_bytes(p: *u8, n: u32) {
  @emp off {
    let h: *u8 = GetStdHandle(-11);
    let written: u32 = 0;
    WriteFile(h, p, n, &mut written, null);
  }
}

fn print(s: *u8) {
  @emp off {
    let n: u32 = (u32)lstrlenA(s);
    write_bytes(s, n);
  }
}

fn println(s: *u8) {
  print(s);
  print("\n");
}

fn println(s: string) {
  let p: *u8 = string_cstr(&s);
  let n: u32 = (u32)string_len(&s);
  write_bytes(p, n);
  write_bytes("\n", 1);
}

fn now_ticks() -> i64 {
  @emp off {
    let t: i64 = 0;
    QueryPerformanceCounter(&mut t);
    return t;
  }
  return 0;
}

fn qpc_freq() -> i64 {
  @emp off {
    let f: i64 = 0;
    QueryPerformanceFrequency(&mut f);
    return f;
  }
  return 0;
}

fn hash32(k: u32, seed: u32) -> u32 {
  // Fast multiplicative mixing (all-decimal constants).
  // Note: multiplying by an odd constant permutes modulo 2^n.
  let x: u32 = k ^ seed;
  let h: u32 = x * (u32)2654435761;
  return h * (u32)2246822519;
}

fn main() -> i32 {
  // This benchmark intentionally runs in `@emp off` so we can write fast,
  // low-level code without fighting move-only scalar semantics.
  @emp off {
    // Workload.
    // Increase in 1000s and print progress so we can see where/if it hangs.
    let entries_max: i32 = 10000000;
    // Start small; bump this toward entries_max as you test.
    let entries_run: i32 = 1000000;
    let progress_step: i32 = 1000;

    // Choose a power-of-two capacity so masking is fast.
    // Keep load < 75%: cap >= ceil(entries_max / 0.75) ~= entries_max * 4/3.
    let need: i32 = ((entries_run * 4) / 3) + 1;
    let cap: i32 = 1;
    while (cap < need) {
      cap = cap << 1;
    }

    let mask: u32 = (u32)(cap - 1);
    let random_until: i32 = (cap * 3) / 4;

    // Allocate tables.
    // Empty list literals currently can't be typed from annotations alone, so seed+pop.
    let keys: u32[] = [(u32)0];
    let vals: u32[] = [(u32)0];
    let used: u8[] = [(u8)0];
    let _k0: u32 = list_pop(&mut keys);
    let _v0: u32 = list_pop(&mut vals);
    let _u0: u8 = list_pop(&mut used);

    // Reserve storage. We only need to *initialize* `used` to 0; `keys`/`vals` are written on insert.
    list_reserve(&mut keys, cap);
    list_reserve(&mut vals, cap);
    list_reserve(&mut used, cap);

    let i: i32 = 0;
    let next_init_report: i32 = 1000000;
    while (i < cap) {
      list_push(&mut used, 0);
      i += 1;
      // Occasional init progress so huge caps don't look hung.
      if (i == next_init_report) {
        let msg: string = $"init_used={i}";
        println(msg);
        next_init_report += 1000000;
      }
    }

    // Seed for probing.
    let seed: u32 = (u32)2166136261;

    // Benchmark: time only insertion loop.
    let freq: i64 = qpc_freq();
    let t0: i64 = now_ticks();

    let len: i32 = 0;
    let k: i32 = 0;
    let next_report: i32 = progress_step;
    while (k < entries_run) {
      // Avoid an infinite loop if the table fills.
      if (len >= cap) { break; }

      let key: u32 = (u32)k;
      // 0x9E3779B9 in decimal.
      let val: u32 = key ^ (u32)2654435769;

      let h: u32 = hash32(key, seed);
      let step: u32 = (hash32(key, seed ^ (u32)1013904223) | 1) & mask;

      let pos: u32 = h & mask;

      // Probe.
      let probes: i32 = 0;
      while (true) {
        if (probes >= cap) { break; }
        let idx: i32 = (i32)pos;
        if (used[idx] == 0) {
          used[idx] = 1;
          keys[idx] = key;
          vals[idx] = val;
          len += 1;
          break;
        }

        if (len < random_until) {
          pos = (pos + step) & mask;
        } else {
          pos = (pos + 1) & mask;
        }

        probes += 1;
      }

      k += 1;

      if (k == next_report) {
        let t: i64 = now_ticks();
        let ms_now: i64 = ((t - t0) * 1000) / freq;
        let msg: string = $"inserted={k} time_ms={ms_now}";
        println(msg);
        next_report += progress_step;
      }
    }

    let t1: i64 = now_ticks();

    let dt: i64 = t1 - t0;
    let ms: i64 = (dt * 1000) / freq;

    println("hashmap_bench");
    {
      let msg: string = $"entries_target={entries_max} entries_run={entries_run} inserted={k} cap={cap} time_ms={ms}";
      println(msg);
    }

    // Prevent optimizing everything away (very light checksum).
    let checksum: u32 = keys[1] ^ vals[2] ^ (u32)len;
    {
      let msg: string = $"checksum={checksum}";
      println(msg);
    }

    @emp mm off {
      list_free(&mut keys);
      list_free(&mut vals);
      list_free(&mut used);
    }
  }

  return 0;
}
