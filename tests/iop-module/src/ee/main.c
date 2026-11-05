#include <stdio.h>
#include <string.h>
#include <kernel.h>

#include <kernel.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>

extern int kputs(char *s);

/* A program must never return from main on the rig: that reboots the console. */
static int rig_failed;
static int rig_kputs(char *s) {
    if (strstr(s, ": FAIL")) rig_failed = 1;
    return kputs(s);
}
#define kputs rig_kputs
extern unsigned char probe_irx[];
extern unsigned int size_probe_irx;

static int test_main(void) {
    int result = -1;
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    int id = SifExecModuleBuffer(probe_irx, size_probe_irx, 0, NULL, &result);
    /* The module returns 100 + b + c: 113 when the shared-lui style code is relocated right. */
    kputs(id > 0 && result == 113 ? "iop-module: PASS\n" : "iop-module: FAIL\n");
    return 0;
}

int main(void) {
    int result = test_main();
    printf("[ps2rig] exit=%d\n", (result || rig_failed) ? 1 : 0);
    SleepThread();
    return 0;
}
