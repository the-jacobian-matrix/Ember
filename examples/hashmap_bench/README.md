# Hashmap benchmark (EMP)

Benchmarks a high-throughput open-addressing hashmap insert workload.

- Entries: `10_000_000`
- Capacity: `16_777_216` (power-of-two)
- Collision handling: open addressing
- Probing policy:
  - While load < 75%: pseudo-random probing (double-hash style odd step)
  - After 75%: linear probing
- Timing: Windows `QueryPerformanceCounter`

## Run

From repo root:

```powershell
# Build + run
powershell -NoProfile -ExecutionPolicy Bypass -File .\examples\hashmap_bench\run.ps1
```
