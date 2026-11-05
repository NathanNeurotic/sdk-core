#!/bin/bash
# Builds the tests/bench program with GCC and LLVM at several optimisation levels, runs each in PCSX2
# headless and prints cycles (COP0 Count) and function code size for each kernel, with LLVM's ratio to GCC.
# Results with the same checksum on both toolchains are the same computation; a different checksum is flagged.
#
# Usage: tests/bench.sh [--levels "O2 Os"]
#   PS2BUILD  ps2build executable (default: ps2build)
#   PCSX2     emulator executable (default: pcsx2-qt, or the Windows install path)
#   NM        llvm-nm used to read function sizes (default: llvm-nm)
#   TIMEOUT   seconds to wait for each run (default: 60)

set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
PS2BUILD="${PS2BUILD:-ps2build}"
NM="${NM:-llvm-nm}"
TIMEOUT="${TIMEOUT:-60}"
levels="O2 Os"

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

# run_one <toolchain> <level>: writes bench-<toolchain>-<level>.txt with "<kernel> <cycles> <checksum> <bytes>" lines.
run_one() {
  local tc="$1" level="$2" work="$HERE/bench/build-$1-$2"
  mkdir -p "$work"
  sed "s/^\( *opt:\).*/\1 -$level/" "$HERE/bench/ps2.yaml" > "$work/ps2.yaml"
  cp -r "$HERE/bench/src" "$work/"
  if ! (cd "$work" && "$PS2BUILD" build --toolchain "$tc" -o out >build.log 2>&1); then
    echo "build failed: $work/build.log" >&2
    return 1
  fi
  local elf="$work/out/bin/ee-bench.elf" log="$work/pcsx2.log"
  rm -f "$log"
  "$PCSX2" -elf "$(native_path "$elf")" -nogui -batch -earlyconsolelog -logfile "$(native_path "$log")" >/dev/null 2>&1 &
  local emu=$!
  for _ in $(seq 1 "$TIMEOUT"); do
    sleep 1
    if [ -f "$log" ] && grep -qE "ee-bench: (PASS|FAIL)" "$log"; then break; fi
  done
  stop_emulator "$emu"
  wait "$emu" 2>/dev/null
  sed 's/^\[[^]]*\] //' "$log" | tr -d '\r' | grep -aE '^bench [a-z_]+ [0-9]+ [0-9a-f]{8} *$' | sed 's/^bench //' > "$work/cycles.txt"
  "$NM" -S --size-sort "$work/out/obj/ee-bench/ee-bench.unstripped.elf" | tr -d '\r' | awk '{print $4, $2}' > "$work/sizes.txt" 2>/dev/null
  [ -s "$work/cycles.txt" ]
}

declare -A fn=( [memcpy_mmi]=bench_memcpy_mmi [memcpy_libc]=bench_memcpy_libc [memcpy_loop]=bench_memcpy_loop \
  [memset_mmi]=bench_memset_mmi [packet_build]=bench_packet_build [decompress]=bench_decompress \
  [physics]=bench_physics [raster]=bench_raster )

for level in $levels; do
  run_one gcc "$level" || { echo "FAIL  gcc -$level"; continue; }
  run_one llvm "$level" || { echo "FAIL  llvm -$level"; continue; }
  python - "$HERE/bench/build-gcc-$level" "$HERE/bench/build-llvm-$level" "$level" <<'PY'
import sys
gdir, ldir, level = sys.argv[1:]
def cycles(d):
    return {l.split()[0]: (int(l.split()[1]), l.split()[2]) for l in open(d + "/cycles.txt") if l.strip()}
def sizes(d):
    out = {}
    for l in open(d + "/sizes.txt"):
        p = l.split()
        if len(p) == 2:
            k = p[0].split('.')[0]
            out[k] = out.get(k, 0) + int(p[1], 16)
    return out
g, l = cycles(gdir), cycles(ldir)
gs, ls = sizes(gdir), sizes(ldir)
names = {"memcpy_mmi": "bench_memcpy_mmi", "memcpy_libc": "bench_memcpy_libc", "memcpy_loop": "bench_memcpy_loop",
         "memset_mmi": "bench_memset_mmi", "packet_build": "bench_packet_build", "decompress": "bench_decompress",
         "inflate": "bench_inflate+bench_inflate_table", "physics": "bench_physics", "raster": "bench_raster"}
print(f"-{level}")
print(f"{'kernel':<14}{'gcc cycles':>12}{'llvm cycles':>13}{'llvm/gcc':>10}{'gcc bytes':>11}{'llvm bytes':>12}{'llvm/gcc':>10}  checksum")
for k in names:
    if k not in g or k not in l:
        print(f"{k:<14} missing")
        continue
    gc, lc = g[k][0], l[k][0]
    syms = names[k].split("+")
    gb = sum(gs[n] for n in syms) if all(n in gs for n in syms) else None
    lb = sum(ls[n] for n in syms) if all(n in ls for n in syms) else None
    sb = f"{gb:>11}{lb:>12}{lb / gb:>10.2f}" if gb and lb else f"{'-':>11}{'-':>12}{'-':>10}"
    same = "same" if g[k][1] == l[k][1] else f"DIFFERS {g[k][1]} {l[k][1]}"
    print(f"{k:<14}{gc:>12}{lc:>13}{lc / gc:>10.2f}{sb}  {same}")
PY
done
