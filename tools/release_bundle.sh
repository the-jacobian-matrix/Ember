#!/usr/bin/env bash
set -euo pipefail

# Create a downloadable release archive from an already-built EMP binary.
# Usage:
#   tools/release_bundle.sh --binary ./emp --target linux-x64 --version v0.1.0

BIN=""
TARGET="linux-x64"
VERSION="dev"
OUTDIR="dist"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --binary) BIN="$2"; shift 2 ;;
    --target) TARGET="$2"; shift 2 ;;
    --version) VERSION="$2"; shift 2 ;;
    --outdir) OUTDIR="$2"; shift 2 ;;
    *) echo "Unknown arg: $1" >&2; exit 2 ;;
  esac
done

if [[ -z "$BIN" ]]; then
  echo "Missing --binary <path-to-emp-or-emp.exe>" >&2
  exit 2
fi
if [[ ! -f "$BIN" ]]; then
  echo "Binary not found: $BIN" >&2
  exit 1
fi

STAGE="$OUTDIR/emp-${TARGET}"
rm -rf "$STAGE"
mkdir -p "$STAGE"

cp "$BIN" "$STAGE/"
cp -r emp_mods "$STAGE/emp_mods"
cp README.md LICENSE "$STAGE/"

if [[ -d docs ]]; then
  cp -r docs "$STAGE/docs"
fi
if [[ -d examples ]]; then
  cp -r examples "$STAGE/examples"
fi

ARCHIVE="$OUTDIR/emp-${TARGET}-${VERSION}.tar.gz"
mkdir -p "$OUTDIR"
tar -C "$OUTDIR" -czf "$ARCHIVE" "emp-${TARGET}"

SHA="$ARCHIVE.sha256"
sha256sum "$ARCHIVE" > "$SHA"

echo "Created: $ARCHIVE"
echo "Checksum: $SHA"
