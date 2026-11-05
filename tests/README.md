# Regression tests

Small self-checking EE programs. Each prints `<name>: PASS` or `<name>: FAIL ...` through `kputs`,
and `run.sh` builds it with ps2build, runs it headless in PCSX2 and reports the result.

```
tests/run.sh                  # every test, GCC
tests/run.sh --llvm           # every test, LLVM
tests/run.sh cxx-iostream     # one test
```

The runner is a bash script, so run it on Linux (or any system with bash); the Windows install has no bash.

Set `PS2BUILD` and `PCSX2` if the tools are not on `PATH`; `TIMEOUT` (seconds, default 40) is how
long to wait for each result.

Each test directory has one `ps2.yaml`, built with `--toolchain gcc` or `--toolchain llvm`. A `gcc.skip`
or `llvm.skip` file marks a test as not supported by that toolchain; its first line is printed as the reason.

| Test | Covers |
|---|---|
| `cxx-iostream` | `std::ostringstream`, `std::cout`, global constructors |
| `cxx-coroutines` | C++20 coroutines |
| `cxx-promise` | `std::thread`, `std::promise`, `std::future` |
| `cxx-threads` | `std::thread`, `std::promise<void>`, `sleep_for` |

| `cxx-exceptions` | `throw` and `catch` across frames with destructors, rethrow, base-class and `catch (...)`, nested exceptions, and an exception inside a `std::thread` (LLVM only for now: it uses `std::thread`, which does not link with GCC) |
| `cxx-thread-local` | `thread_local` variables and their destructors across threads (LLVM only) |
| `crash-terminate` | Uncaught C++ exception reaches the crash screen with the exception type and `what()` (LLVM only) |
| `crash-break` | `break` instruction reaches the crash screen with PC and registers |
| `crash-trap` | `teq` trap reaches the crash screen |
| `crash-adel`, `crash-ades`, `crash-ri` | Address error on load and store, reserved instruction. PCSX2 does not raise these, so they run only with `HARDWARE=1` |
| `ee-mmi` | R5900 multimedia and pipeline-1 instructions (pand, por, pnor, pxor, paddb, psubb, paddsb, psllh, movn, movz, mult1, mfsa, mtsa, pcpyld, pcpyud, qfsrv, plzcw) checked against known results |
| `iop-module` | An IOP module built by ps2build, embedded in an EE program and loaded with `SifExecModuleBuffer` |
| `iop-mmio` | An IOP module that reads memory-mapped registers through constant addresses and through an explicit load, and checks they agree |
| `iop-boot` | Loads 42 IOP modules with `SifExecModuleBuffer` in dependency order and checks each one links and starts. Modules that cannot load in PCSX2 are listed in `src/modules.def` with the reason |
| `ee-libs` | Links one symbol from each of 32 EE libraries into a single program, so a missing or duplicated symbol fails the build |
| `driver-blockdev` | Example block device driver (`ramdisk_bd`): a 64 KiB RAM disk registered with BDM, mounted by `bdmfs_fatfs` as `ram0:`, then read and written from the EE through fileXio. `mkimage.py` regenerates the FAT12 image in `src/iop/image.c` |
| `driver-tty` | Example character device driver (`rtty`): an iomanX device backed by a ring buffer, opened, written, read and cleared from the EE. An ioctl then makes it the IOP's stdout while still passing the output on to the previous `tty:` (ps2link's console). PCSX2 logs IOP stdout writes itself, so the printf capture check only runs on hardware (it prints `[SKIP]` in PCSX2) |
| `bdmfs-devctl` | The `bdmfs_fatfs` sector size, total size, free size and stop devctls on `mass0:`. Needs a FAT USB disk image on PCSX2's USB1 (skipped when `PCSX2.ini` has none) |

## GCC against LLVM

`tests/diff.sh` builds the programs in `tests/diff` with both toolchains at `-O0`, `-O1`, `-O2`, `-O3` and `-Os`,
runs them in PCSX2 and compares their `<name> <hash>` lines.

| Program | Covers |
|---|---|
| `kernels` | 94 small float and integer kernels, loops and conversions |
| `libm` | The single-precision libm functions |
| `fp-compare` | Comparisons, min, max, clamp, sqrt and abs over NaN, infinities, signed zeros and denormals |

A difference listed in `<program>/expected` (any level) or `<program>/expected.<level>` is accepted, with the reason
in a comment. Any other difference fails. LLVM's results are identical at every level; the accepted differences are
GCC calling the library `fmaxf`, `fminf` and `sqrtf` at `-O0` and `-Os`, and the decimal float literal rounding in libm.

`tests/gate.sh` runs everything: both toolchains' tests and the comparisons.

## Not covered by a boot test

| Package | Why |
|---|---|
| `loadcore`, `sysmem`, `intrman`, `threadman`, `sifman`, `sifcmd`, `sifinit`, `stdio`, `sysclib*`, `timrman*`, `vblank`, `ioman`, `modload`, `secrman`, `cdvd`, `cdvdfsv`, `iopmgr`, `iopreboot`, `reboot`, `udnl*` | Replace the BIOS modules, so loading them into a running IOP is not a meaningful test |
| `mcman*`, `mcserv*`, `padman`, `mtapman*`, `rmman`, `sio2man-nano` | Need `rsio2man`/`xsio2man`, alternate builds of `sio2man` that ps2build cannot embed by name |
| `apa`, `hdd-osd`, `dvr*`, `smap-ps2ip`, `IEEE1394_bd*`, `mx4sio_bd*`, `excepman` | Wait for hardware PCSX2 does not provide, or hang the IOP when loaded after boot |
| `ps2ips` (EE) | Alternative to `ps2ip` that defines the same symbols (`ip_addr_any`), so the two cannot link into one program |
| `libgs`, `libvux`, `libcglue`, `libprofglue`, `libpthreadglue`, `sbv`, `startup` (EE) | Not installed as separate libraries; they ship inside the toolchain or the startup package |

## Benchmarks

`tests/bench.sh` compares speed and function size of GCC and LLVM builds of a set of EE kernels; results and exceptions are in `tests/bench/README.md`.

Bus errors (IBE, DBE) have no test: user mode cannot reach kseg, unmapped kuseg addresses raise a TLB miss instead, and reads of EE register-space holes stall the bus without raising an exception. The crash screen still decodes them if a kernel-mode caller triggers one.
