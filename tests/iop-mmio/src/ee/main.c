#include <stdio.h>
#include <string.h>
#include <kernel.h>

#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>

extern int kputs(char *s);

extern unsigned char mmio_irx[];
extern unsigned int size_mmio_irx;

static int test_main(void) {
    int result = -1;
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    int id = SifExecModuleBuffer(mmio_irx, size_mmio_irx, 0, NULL, &result);
    /* The module returns 1 (no resident end) when constant-address accesses match explicit ones. */
    kputs(id > 0 && result == 1 ? "iop-mmio: PASS\n" : "iop-mmio: FAIL\n");
    return id > 0 && result == 1 ? 0 : 1;
}

int main(void) {
    int result = test_main();
    printf("[ps2rig] exit=%d\n", result ? 1 : 0);
    SleepThread();
    return 0;
}
