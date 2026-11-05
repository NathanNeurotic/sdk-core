#!/bin/bash
# Release gate: runs every regression test with both toolchains, then the GCC against LLVM comparisons.
# Run it against a clean install (`ps2build update`, then `ps2build update llvm-toolchain`) before tagging.
#
# Usage: tests/gate.sh
#   PS2BUILD, PCSX2, TIMEOUT: as for run.sh and diff.sh

HERE="$(cd "$(dirname "$0")" && pwd)"
status=0
"$HERE/run.sh" || status=1
"$HERE/run.sh" --llvm || status=1
"$HERE/diff.sh" || status=1
[ "$status" -eq 0 ] && echo "gate passed" || echo "gate FAILED"
exit "$status"
