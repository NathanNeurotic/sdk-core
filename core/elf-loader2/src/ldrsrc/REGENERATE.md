`../ldrsrc_bin.c` is generated from this directory's `ldrsrc.c` (real,
unmodified upstream source - `ps2sdk/ee/elf-loader2/src/ldrsrc/src/ldrsrc.c`)
via a standalone build outside ps2build's own pipeline, matching real
upstream's own nested Makefile: compile `ldrsrc.c` against `elf-loader.h`/
`elf_loader_common.h`, link it alone (`-nostdlib -nostartfiles -T linkfile`,
against only `libkernel.a` + `libc_nano.a`), `objcopy -Obinary` the result,
then `bin2c` the raw binary into `ldrsrc` / `size_ldrsrc`.

Regenerate only if `ldrsrc.c` itself changes (rare - it's a small, stable
relocator stub):

```
mips64r5900el-ps2-elf-gcc -D_EE -G0 -O2 -mno-gpopt \
  -I$PS2DEV/toolchain/ee/include -I$PS2DEV/packages/core/common/include \
  -I$PS2DEV/packages/core/kernel/include -I$PS2DEV/packages/core/cglue/include \
  -I../include -c ldrsrc.c -o ldrsrc.o

mips64r5900el-ps2-elf-gcc -D_EE -G0 -mno-gpopt -o ldrsrc.elf ldrsrc.o \
  -nostdlib -nostartfiles -Tlinkfile -s \
  $PS2DEV/packages/core/kernel/lib/libkernel.a \
  -L$PS2DEV/toolchain/ee/mips64r5900el-ps2-elf/lib -lc_nano \
  $PS2DEV/packages/core/kernel/lib/libkernel.a

mips64r5900el-ps2-elf-objcopy -Obinary ldrsrc.elf ldrsrc.bin
$PS2DEV/tools/bin2c ldrsrc.bin ../ldrsrc_bin.c ldrsrc
```
