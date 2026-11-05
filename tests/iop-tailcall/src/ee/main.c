#include <stdio.h>
#include <string.h>
#include <kernel.h>

#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>

extern int kputs(char *s);

extern unsigned char tc_irx[];
extern unsigned int size_tc_irx;

static int test_main(void) {
    int result = -1;
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    int id = SifExecModuleBuffer(tc_irx, size_tc_irx, 0, NULL, &result);
    /* The module returns 1 (no resident end) when every call form returned the expected value. */
    kputs(id > 0 && result == 1 ? "iop-tailcall: PASS\n" : "iop-tailcall: FAIL\n");
    return id > 0 && result == 1 ? 0 : 1;
}

int main(void) {
    int result = test_main();
    printf("[ps2rig] exit=%d\n", result ? 1 : 0);
    SleepThread();
    return 0;
}
