# EMP Packages (Draft)

Goal: make `@emp off` rare in application code by shipping a real standard library, and make dependency management deterministic (fast, cacheable builds).

## Package layout

A package is a folder containing:

- `emp.toml` (or `emp.json`) — package manifest
- `src/` — package sources
  - `src/main.em` (binary entry)
  - `src/lib.em` (library entry)
  - `src/<module>.em`
- `emp_mods/` — local module root (standard)
- `out/` — build outputs (cacheable)

## Module resolution (deterministic)

Resolution order for `use ... from ...`:

1) Same folder as the importing module
2) Package `src/` root
3) Package `emp_mods/` root (stdlib and local deps)
4) Dependency packages (pinned versions) under a local store

A dependency is resolved to an immutable folder path based on:

- package name
- version
- source (registry/git/path)

## Cache keys

To keep builds reproducible and fast, the build cache key includes:

- compiler version
- target triple
- optimization flags
- full transitive dependency lockfile hash
- content hash of each source file

## Proposed CLI

- `emp init` — create a new package skeleton
- `emp build` — build current package (default: native exe)
- `emp run` — build + run
- `emp test` — run package tests
- `emp add <name>` — add dependency
- `emp vendor` — copy dependencies locally (offline builds)

## Lockfile

Use a lockfile (e.g. `emp.lock`) that pins:

- exact versions
- source URLs
- content hashes

This enables deterministic module resolution and safe caching.
