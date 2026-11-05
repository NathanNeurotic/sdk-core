#!/bin/bash
# Builds each program in tests/diff with GCC and LLVM at several optimisation levels, runs both in
# PCSX2 headless, and compares the "<name> <hex>" result lines. A difference listed in
# tests/diff/<test>/expected or expected.<level> (one name per line, '#' comments) is accepted; any other
# difference, or a result missing on one side, fails.
#
# Usage: tests/diff.sh [--levels "O0 O1 O2 O3 Os"] [test-name ...]
#   PS2BUILD  ps2build executable (default: ps2build)
#   PCSX2     emulator executable (default: pcsx2-qt, or the Windows install path)
#   TIMEOUT   seconds to wait for each run (default: 40)

set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
PS2BUILD="${PS2BUILD:-ps2build}"
TIMEOUT="${TIMEOUT:-40}"
levels="O0 O1 O2 O3 Os"

if [ -z "${PCSX2:-}" ]; then
  if [ -x "/c/Program Files/PCSX2/pcsx2-qt.exe" ]; then
    PCSX2="/c/Program Files/PCSX2/pcsx2-qt.exe"
  else
    PCSX2="pcsx2-qt"
  fi
fi

if [ "${1:-}" = "--levels" ]; then
  levels="$2"
  shift 2
fi

if [ "$#" -gt 0 ]; then
  names=("$@")
else
  names=()
  for d in "$HERE"/diff/*/; do
    [ -f "$d/ps2.yaml" ] && names+=("$(basename "$d")")
  done
fi

native_path() {
  if command -v cygpath >/dev/null 2>&1; then cygpath -w "$1"; else echo "$1"; fi
}

stop_emulator() {
  local pid="$1"
  if [ -r "/proc/$pid/winpid" ]; then
    taskkill //F //PID "$(cat "/proc/$pid/winpid")" >/dev/null 2>&1
  else
    kill "$pid" 2>/dev/null
  fi
}

# run_one <dir> <toolchain> <level> <out-file>: builds and runs, writes the result lines to <out-file>.
run_one() {
  local dir="$1" tc="$2" level="$3" out="$4" work="$1/build-$2-$3"
  mkdir -p "$work"
  sed "s/^\( *opt:\).*/\1 -$level/" "$dir/ps2.yaml" > "$work/ps2.yaml"
  cp -r "$dir/src" "$work/"
  if ! (cd "$work" && "$PS2BUILD" build --toolchain "$tc" -o out >build.log 2>&1); then
    echo "build failed: $work/build.log" >&2
    return 1
  fi
  local elf log
  elf="$(ls "$work"/out/bin/*.elf | head -1)"
  log="$work/pcsx2.log"
  rm -f "$log"
  "$PCSX2" -elf "$(native_path "$elf")" -nogui -batch -earlyconsolelog -logfile "$(native_path "$log")" >/dev/null 2>&1 &
  local emu=$!
  for _ in $(seq 1 "$TIMEOUT"); do
    sleep 1
    if [ -f "$log" ] && grep -qE ": PASS|: FAIL" "$log"; then break; fi
  done
  stop_emulator "$emu"
  wait "$emu" 2>/dev/null
  sed 's/^\[[^]]*\] //' "$log" | tr -d '\r' | grep -aE '^[A-Za-z0-9_]+ [0-9a-f]{8} *$' > "$out"
  [ -s "$out" ]
}

fail=0
for name in "${names[@]}"; do
  dir="$HERE/diff/$name"
  for level in $levels; do
    g="$dir/result-gcc-$level.txt"; l="$dir/result-llvm-$level.txt"
    if ! run_one "$dir" gcc "$level" "$g" || ! run_one "$dir" llvm "$level" "$l"; then
      echo "FAIL  $name -$level (no results)"
      fail=$((fail + 1))
      continue
    fi
    expected="$dir/expected.$level"
    [ -f "$dir/expected" ] && expected="$dir/expected $expected"
    unexpected="$(python3 - "$g" "$l" $expected <<'PY'
import os, sys
def load(path):
    return dict(line.split() for line in open(path) if line.strip())
gcc, llvm = load(sys.argv[1]), load(sys.argv[2])
accepted = set()
for path in sys.argv[3:]:
    if os.path.exists(path):
        accepted |= {l.split('#')[0].strip() for l in open(path)} - {''}
keys = set(gcc) | set(llvm)
bad = sorted(k for k in keys if gcc.get(k) != llvm.get(k) and k not in accepted)
print(' '.join(bad))
PY
)"
    if [ -n "$unexpected" ]; then
      echo "FAIL  $name -$level (differs: $unexpected)"
      fail=$((fail + 1))
    else
      echo "PASS  $name -$level"
    fi
  done
done

[ "$fail" -eq 0 ]
