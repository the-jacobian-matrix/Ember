# EMP Standard Library (stdlib) — Feature Checklist

This document is a **comprehensive, practical checklist** of what an EMP `std` should provide over time.

- It’s written as **capabilities to implement** (checkboxes), not as promises that they exist today.
- It spans: language/runtime glue, data structures, I/O, OS integration (including Windows), networking, security, tooling, and more.
- When something is OS-specific, it’s marked explicitly.

> Note (Windows system DLLs): typical names are `kernel32.dll`, `user32.dll`, `gdi32.dll`, `advapi32.dll`, `ws2_32.dll`, etc. (Not `kernel.dll` or `window32`.) A good `std` should make **dynamic loading** (LoadLibrary/GetProcAddress) possible in a safe/ergonomic way.

---

## Legend / Conventions

- `[ ]` = planned / needed
- `[x]` = implemented (only check once it truly exists)
- “API sketch” blocks are suggestions, not required syntax.

---

## 0) Guiding principles (still a checklist)

- [x] **Everything is a module**: import explicitly via `use ... from ...`.
- [ ] **No hidden costs**: allocation, I/O, and OS calls are explicit.
- [ ] **Value-first**: core types are structs/enums, not always heap objects.
- [ ] **Safety by default**: bounds/overflow/FFI barriers are on unless locally suppressed.
- [ ] **Default memory management**: values drop deterministically unless a type explicitly opts out.
- [x] **Manual-MM opt-in for alloc**: raw allocation APIs are only usable inside `@emp mm off` (file or block) regions (Zig-style explicit allocator patterns).
- [ ] **Determinism**: stable iteration order and deterministic import resolution.
- [ ] **Capability-aware design** (future): dangerous APIs behind explicit capabilities.

---

## 1) Module layout (proposed)

This is a *shape*, not a hard requirement. The goal is discoverability.

Implementation note (repo + build):

- The stdlib source-of-truth lives in `stdlib/emp_mods/*`.
- CMake copies `stdlib/` next to `emp.exe` on build, so the compiler can resolve bundled modules from `<exe_dir>/stdlib/emp_mods/*`.
- When a project imports from `std.*`, EMP auto-vendors the required `std/*` module files into that project's `emp_mods/` folder (created/filled on demand).

- [ ] `emp_mods/core/*` — primitives, results/options, spans/slices, prelude (opt-in)
- [x] `emp_mods/alloc/*` — allocators, arenas (manual-MM only via `@emp mm off`)
- [ ] `emp_mods/collections/*` — Vec/List/Map/Set/Queue
- [ ] `emp_mods/string/*` — String, encoding, formatting glue
- [x] `emp_mods/io/*` — readers/writers, console, buffering
- [ ] `emp_mods/fs/*` — paths, files, directories
- [x] `emp_mods/time/*` — Duration/Instant/SystemTime
- [ ] `emp_mods/process/*` — args/env/spawn
- [ ] `emp_mods/thread/*` — threads and sync primitives
- [ ] `emp_mods/net/*` — sockets + (optional) HTTP
- [ ] `emp_mods/serde/*` — JSON/binary formats
- [ ] `emp_mods/crypto/*` — hashes/hmac/secure random (optional; careful)
- [ ] `emp_mods/compress/*` — gzip/zstd (optional)
- [ ] `emp_mods/ffi/*` — C ABI and interop helpers
- [ ] `emp_mods/dylib/*` — dynamic loading abstraction
- [ ] `emp_mods/os/*` — OS-specific facades (Windows/POSIX)
- [ ] `emp_mods/gui/*` — optional GUI surface (likely separate package)

---

## 2) `std::prelude` (opt-in default imports)

- [x] Common types: `Option[T]`, `Result[T,E]`
- [ ] Core traits: `Eq`, `Ord`, `Hash`, `Clone`/`Copy`, `Debug`/`Display`
- [ ] Basic iter helpers (if iterators exist)
- [ ] `panic`, `debug_assert`
- [x] `assert`
- [ ] `format` (if formatting exists)
- [x] `println`

---

## 3) `std::core` (language-adjacent)

### 3.1 Foundational types
- [x] `Option[T]` (`Some(T)` / `None`)
- [x] `Result[T,E]` (`Ok(T)` / `Err(E)`)
- [ ] `Error` trait/interface (message + optional cause chain)

### 3.2 Panics and assertions
- [ ] `panic(msg: string)`
- [x] `assert(cond: bool)`
- [ ] `assert(cond: bool, msg?: string)`
- [ ] `unreachable()`
- [ ] panic strategy selection (abort vs unwind), documented

### 3.3 Conversions and parsing
- [ ] string ↔ number conversions
- [ ] safe parsing with `Result`
- [ ] checked/saturating/wrapping arithmetic helpers

### 3.4 Memory and pointer primitives
- [ ] `mem::size_of[T]`, `align_of[T]`
- [ ] `mem::swap`, `mem::replace`, `mem::take`
- [ ] raw pointer module (unsafe): address-of, load/store, arithmetic
- [ ] explicit `unsafe` FFI boundary helpers

---

## 4) `std::alloc`

- [x] Policy: allocation primitives are *not* generally available; they require `@emp mm off` (manual memory management mode)
- [ ] Prefer Zig-style explicit allocator passing (global allocator optional)
- [ ] `Allocator` interface (alloc/realloc/free + alignment)
- [x] `Arena` allocator (manual-MM helper)
- [ ] `Box[T]` / owned heap allocation type (optional depending on EMP design)
- [ ] `Rc` / `Arc` (optional; only if the language wants ref-counting)
- [ ] allocation diagnostics: OOM strategy (panic vs error)

---

## 5) Strings, text, and formatting (`std::string`, `std::fmt`)

### 5.1 String types
- [ ] `String` (owned, growable)
- [ ] `Str` or string-slice (borrowed view)
- [ ] `CString` (NUL-terminated for C FFI)
- [ ] `WString` or UTF-16 helpers (Windows)

### 5.2 Text operations
- [ ] concat/join
- [ ] split/trim/lines
- [ ] find/replace
- [ ] case conversion
- [ ] UTF-8 validation and iteration

### 5.3 Formatting
- [ ] `format("...", args...) -> String`
- [ ] `println` / `eprintln`
- [ ] `Debug`/`Display`-style formatting traits

### 5.4 Regex (optional)
- [ ] regex matching and captures
- [ ] safe limits to avoid worst-case runtime

---

## 6) Collections (`std::collections`)

### 6.1 Linear
- [ ] `Vec[T]` (growable list)
- [ ] `Deque[T]`
- [ ] `Slice[T]` / `Span[T]` view type

### 6.2 Maps and sets
- [ ] `HashMap[K,V]`
- [ ] `HashSet[T]`
- [ ] ordered map/set (tree-based)

### 6.3 Algorithms
- [ ] sort (stable/unstable)
- [ ] binary search
- [ ] dedup

### 6.4 Iteration model
- [ ] iterators (or a clear alternative)
- [ ] deterministic iteration rules documented (especially for hash maps)

---

## 7) I/O (`std::io`)

- [ ] `Read`/`Write` traits
- [ ] `BufReader`/`BufWriter`
- [ ] `stdin`/`stdout`/`stderr`
- [ ] error model (`io::Error` with kind/category)
- [ ] text helpers: `read_line`, `write_line`
- [ ] terminal detection (TTY)
- [ ] ANSI colors + Windows console support

---

## 8) Filesystem and paths (`std::fs`, `std::path`)

- [ ] `Path` (platform-aware)
- [ ] open/read/write/append
- [ ] atomic write (write temp then rename)
- [ ] directory creation/removal
- [ ] recursive walk
- [ ] metadata (size, times)
- [ ] symlink support (optional)

---

## 9) Time (`std::time`)

- [x] `Duration`
- [ ] `Instant` (monotonic)
- [ ] `SystemTime`
- [ ] sleep
- [ ] timers (optional)

---

## 10) Threads and synchronization (`std::thread`, `std::sync`)

- [ ] spawn/join
- [ ] `Mutex[T]`
- [ ] `RwLock[T]`
- [ ] `Condvar`
- [ ] atomics
- [ ] channels (mpsc)
- [ ] thread-local storage

Async is optional and large:

- [ ] `Future` model
- [ ] executor/runtime
- [ ] async I/O integration

---

## 11) Process and environment (`std::process`, `std::env`)

- [ ] `args()`
- [ ] get/set environment
- [ ] current directory
- [ ] spawn processes with pipes
- [ ] exit codes
- [ ] signals/CTRL-C handling (platform-specific)

---

## 12) Networking (`std::net`)

- [ ] TCP client/server
- [ ] UDP sockets
- [ ] IPv4/IPv6
- [ ] DNS resolution
- [ ] timeouts

Optional:

- [ ] HTTP client
- [ ] TLS hooks (likely via external package)

---

## 13) Serialization (`std::serde`)

- [ ] JSON encode/decode
- [ ] binary format (msgpack-like)
- [ ] schema/versioning story

---

## 14) Crypto and security (`std::crypto`) — optional but common

If included, it must be correct, reviewed, and well-tested.

- [ ] secure random generator
- [ ] hashes: SHA-256 / SHA-512 / BLAKE3
- [ ] HMAC
- [ ] constant-time comparisons
- [ ] key derivation (HKDF)

---

## 15) Random (`std::rand`)

- [ ] fast deterministic PRNG (non-crypto)
- [ ] seeding controls
- [ ] distributions
- [ ] CSPRNG integration (if `std::crypto` exists)

---

## 16) Compression and archives (`std::compress`) — optional

- [ ] gzip/deflate
- [ ] zstd
- [ ] zip/tar reading

---

## 17) Logging and diagnostics (`std::log`)

- [ ] log levels
- [ ] structured logs (key/value)
- [ ] sinks: console/file
- [ ] timestamps and thread ids

---

## 18) FFI (`std::ffi`) and dynamic libraries (`std::dylib`)

### 18.1 C ABI interop
- [x] `extern` functions
- [ ] calling convention control where needed
- [x] safe wrappers for unsafe calls
- [ ] `CString` helpers

### 18.2 Dynamic library loading (cross-platform abstraction)

- [ ] `dylib::open(path) -> Result[Library, Error]`
- [ ] `Library::symbol(name) -> Result[T, Error]`
- [ ] unload policy documented

API sketch:

```text
let lib = dylib::open("user32.dll")?;
let msgbox = lib.symbol[fn(ptr u16, ptr u16, ptr u16, u32) -> i32]("MessageBoxW")?;
```

---

## 19) OS layer (`std::os`)

### 19.1 Cross-platform
- [ ] unified OS error type
- [ ] file descriptor / handle wrappers

### 19.2 Windows (`std::os::windows`)

- [ ] `GetLastError` + error formatting
- [ ] UTF-16 path helpers
- [ ] handle RAII wrappers (`CloseHandle`)
- [ ] process creation wrappers (`CreateProcessW`)
- [ ] registry access (optional)
- [ ] Winsock init and socket wrappers

---

## 20) GUI (`std::gui`) — optional

This is often better as a separate package, but a “batteries-included” ecosystem commonly wants it.

- [ ] Win32 window creation + message loop (Windows-only)
- [ ] drawing: GDI / Direct2D bindings
- [ ] input events
- [ ] cross-platform bindings (SDL/GLFW) (optional)

---

## 21) Testing and tooling (`std::test`)

- [ ] unit test harness
- [ ] benchmarks
- [ ] snapshot testing

---

## 22) Minimal “v1 std” (pragmatic milestone)

If you want a tight, high-impact initial scope that still enables real programs:

- [ ] `String` + formatting
- [ ] `Vec` (+ maybe `HashMap`)
- [ ] `Option`/`Result` + `Error`
- [ ] filesystem read/write + `Path`
- [ ] process spawn/wait + environment
- [ ] time (`Duration`, `Instant`, `sleep`)
- [ ] FFI + dynamic libraries (enables Win32 / libc access)

---

## 23) “Windows app” enablement checklist (your example)

To build real Windows programs (including calling system DLLs to create windows), `std` should provide:

- [ ] UTF-16 string support or conversion utilities
- [ ] `dylib` loader for `kernel32.dll` / `user32.dll` / `gdi32.dll` / etc.
- [ ] safe wrappers for Win32 handles and error codes
- [ ] bindings (manual or generated) for core Win32 APIs
- [ ] a small helper layer for window creation/message loop (optional)
