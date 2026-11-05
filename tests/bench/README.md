# GCC against LLVM benchmarks

`tests/bench.sh` builds `tests/bench` with GCC and LLVM at `-O1`, `-O2`, `-O3` and `-Os`, runs each build in PCSX2
headless, and prints cycles (COP0 Count) and function size for each kernel with LLVM's ratio to GCC. PCSX2 does
not model the memory system, FPU latencies or cache misses of a real console, so the speed figures below come from
the hardware rig; run the ELFs in `bench/build-<toolchain>-<level>/out/bin` there for hardware numbers.

| Kernel | Covers |
|---|---|
| `memcpy_mmi`, `memset_mmi` | 128-bit `lq`/`sq` copy and fill of 64 KiB |
| `memcpy_libc`, `memcpy_loop` | libc `memcpy` and a word loop over 64 KiB |
| `packet_build` | GIF-style packet of 2000 vertices (RGBAQ, ST, XYZ2) |
| `decompress` | LZ77 decoder over a 16 KiB stream |
| `inflate` | Table-driven raw deflate decoder: 24 KiB zlib level 9 stream to 64 KiB of text and binary data |
| `physics` | 1024 particles for 30 frames: float integrate, bounce, `sqrtf` |
| `raster` | 200 edge-function triangles into a 128x128 buffer |

Every kernel's checksum is identical on both toolchains and on PCSX2 and hardware (`physics` 46fb9806, `raster`
c8cb2e87, `packet_build` 67522a17, `inflate` 3f62b2bb). `gen_inflate_data.py` writes `src/inflate_data.h`; the
checksum is that of the uncompressed data.

Each kernel runs three times with interrupts disabled (`DIntr`/`EIntr` around the timed call) and reports the fastest
run. `physics` and `raster` reset their input before each run, so all three do the same work.

`raster` also varies between boots of the same ELF, in steps: LLVM `-Os` gives 7.63M or 7.89M cycles, GCC `-Os`
9.37M or 9.62M, GCC `-O2` 6.85M to 7.16M. The three runs inside one boot always agree, so the difference is state that
lasts for the whole boot; its cause is not known. The figures below are the minimum over two boots. The
other kernels repeat within 0.3% between boots.

## Results on the console (LLVM cycles / GCC cycles, lower is better)

Compiler: ps2/llvm `r5900-ee` with ps2/llvm#15 (R5900 scheduling model and FP cost changes) and ps2/llvm#16 (integer
`movz`/`movn`). Measured 2026-10-15.

| Kernel | -O1 | -O2 | -O3 | -Os |
|---|---|---|---|---|
| `memcpy_mmi` | 1.00 | 1.00 | 1.00 | 0.99 |
| `memcpy_libc` | 1.00 | 0.95 | 0.95 | 1.00 |
| `memcpy_loop` | 0.65 | 0.95 | 0.95 | 1.00 |
| `memset_mmi` | 1.01 | 1.01 | 1.01 | 1.01 |
| `packet_build` | 0.84 | 0.94 | 0.95 | 0.44 |
| `decompress` | 0.96 | 0.79 | 0.77 | 0.79 |
| `inflate` | 0.92 | 0.98 | 3.14 | 0.84 |
| `physics` | 1.52 | 1.15 | 2.05 | 1.04 |
| `raster` | 1.06 | 1.18 | 1.01 | 0.83 |

Adding `inflate` moved the other kernels' code and data. GCC `-Os` `packet_build` and `decompress` changed by 3% and
7%, and `raster` at `-O3` went from 0.98 to 1.01 and at `-Os` from 0.87 to 0.83.

## Function size (LLVM bytes / GCC bytes)

`inflate` counts `bench_inflate` and `bench_inflate_table`, and every clone of either (GCC `-O3` makes three
`bench_inflate_table` clones).

| Kernel | -O1 | -O2 | -O3 | -Os |
|---|---|---|---|---|
| `memcpy_mmi` | 1.11 | 1.05 | 1.05 | 0.95 |
| `memcpy_libc` | 1.57 | 0.58 | 0.58 | 1.00 |
| `memcpy_loop` | 0.85 | 0.58 | 0.58 | 1.00 |
| `memset_mmi` | 0.95 | 1.00 | 1.00 | 1.00 |
| `packet_build` | 0.89 | 0.98 | 0.98 | 0.66 |
| `decompress` | 0.86 | 0.75 | 0.37 | 0.79 |
| `inflate` | 1.01 | 1.00 | 0.64 | 1.13 |
| `physics` | 0.78 | 1.02 | 0.75 | 0.75 |
| `raster` | 1.09 | 0.81 | 0.89 | 1.19 |
| all kernels | 0.98 | 0.95 | 0.65 | 1.04 |

## Exceptions to the 5% threshold

- `physics` (1.52x at `-O1`, 1.15x at `-O2`, 2.05x at `-O3`): the six particle arrays are 4 KB each and 4 KB apart, so
  element `i` of all six maps to the same set of the 2-way, 8 KB data cache. Every particle needs six lines in a set
  that holds two, and the miss count depends on the order of the loads and stores, which differs between compilers
  and levels. GCC `-O1` (9.84M cycles) is faster than GCC `-O2` (12.84M) for this reason. Padding each array by one
  64-byte line, with the same code and checksum, makes both compilers 3.7x to 7.1x faster:

  | Padded `physics` | GCC | LLVM | LLVM/GCC |
  |---|---|---|---|
  | `-O1` | 2.64M | 2.20M | 0.83 |
  | `-O2` | 1.93M | 2.07M | 1.07 |
  | `-O3` | 1.50M | 2.07M | 1.38 |

  What remains is code: at `-O2` LLVM computes `px*px + pz*pz` after the bounce branch, so a `nop` sits between the
  second `c.lt.s` and `bc1f`; GCC computes it before the first branch. At `-O3` GCC specializes the function for the
  constant frame count and handles two frames per particle pass.
- `raster` at `-O1` (1.06x) and `-O2` (1.18x): per pixel LLVM tests `area > 0` and then each of `w0`, `w1`, `w2` with
  its own branch, each followed by a `nop`. GCC moves the `area > 0` test out of the pixel loop and tests the sign of
  `w0 | w1 | w2` with one branch. LLVM moves the invariant test out only at `-O3` (non-trivial unswitching), where it
  is 1.01x.
- Size at `-O1`: `memcpy_libc` (1.57x) and `memcpy_mmi` (1.11x) are static functions with one caller. LLVM copies the
  constant arguments into the function, so it loads the two buffer addresses and the length itself (four to five
  instructions) and the caller passes nothing; GCC does this only from `-O2`. At `-O2` and `-O3` `memcpy_mmi` is one
  instruction longer because LLVM keeps a separate loop counter.
- Size of `raster` at `-O1` (1.09x) and `-Os` (1.19x): the per-pixel branches above, plus the bounding-box setup.
  Before #16 LLVM also built every `min`/`max` from a branch and a `nop`; it now uses `movz`/`movn` like GCC.

- `inflate` at `-O3` (3.14x): LLVM fully unrolls the 15-step canonical Huffman walk used for codes longer than 9
  bits. With 20% random bytes in the data many literal codes are 10 to 12 bits, so that path is hot, and the unrolled
  code rebuilds each code with long shift-and-mask chains and reloads spilled counts. With only that loop kept rolled
  (`#pragma clang loop unroll(disable)`) LLVM takes 1.25M cycles, 1.01x GCC; `-fno-unroll-loops` gives 1.26M. At
  `-O2` LLVM does not unroll it.
- Size of `inflate` at `-Os` (1.13x): the bit reader is inlined at about 15 sites, and LLVM emits more compares and
  constants at each refill (+33 `sltiu`, +26 `li` over the function). Over all kernels this makes LLVM `-Os` 1.04x
  GCC; without `inflate` it is 0.90x.

Over all kernels LLVM is smaller than GCC at `-O1`, `-O2` and `-O3`.

## R5900 short-loop erratum

`shortloop_scan.py <objdump> <elf or .a>...` lists loops of six instructions or fewer, with one conditional branch at
the end, whose delay slot is not a `nop`, which is the case the R5900 erratum breaks. Over the bench ELFs at every level
and every library `ps2build` installs, it finds none in EE code for either toolchain; the only hits are in IOP
libraries (`libbdm`, `libspu2`, `libsnd2`), where the erratum does not apply. With `-mattr=-fix-r5900` it flags the
loop in `llvm/test/CodeGen/Mips/r5900-short-loop.ll`.

LLVM's assembler never moves an instruction into a delay slot, so assembly written without `.set noreorder` is safe;
GNU as `-march=r5900` pads the same loop with a `nop`.

## FPU behaviour

`fpu-check` prints 22 float operations on edge values, each computed at run time from `volatile` inputs and again from
constants the compiler folds. On the console, GCC and LLVM give the same run-time results, except:

- `FLT_MAX * 2.0f`: GCC `-O2` emits `mul.s` with the operands swapped and gets `0x7ffffffe`; LLVM gets `0x7fffffff`.
- `sqrtf(-4.0f)`: GCC `-O0` calls newlib and gets 1.0; with `sqrt.s` (GCC `-O2`, LLVM `-O0` and `-O2`) it is 2.0.

The folded results differ from the console for both compilers. LLVM folds with IEEE rules: `1.0f / 0.0f` and
overflows fold to `0x7f800000`, inexact sums round to nearest, where `add.s` truncates. GCC folds with a
round-towards-zero, no-infinity model, which matches `add.s` and the clamping but is off by 1 to 3 units in the last
place for `1.0f / 3.0f` and `1.1f * 1.1f`. Both fold underflows to denormals, which the console flushes to zero.
These need IEEE-independent float folding to change and are documented as exceptions.

R5900 unroll-and-jam and partial unrolling closed the `physics`, `raster` and `packet_build` gaps, but they needed
changes to generic LLVM passes and made `-O2` code 1.70x GCC's size, so they are not in the compiler.
