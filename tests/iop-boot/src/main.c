#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>

extern int kputs(char *s);

static void say(char *s)
{
    kputs(s);
    printf("%s", s);
}

#define MOD(name, skip) extern unsigned char name##_irx[]; extern unsigned int size_##name##_irx;
#include "modules.def"
#undef MOD

struct module {
    const char *name;
    unsigned char *irx;
    unsigned int *size;
    const char *skip;
};

#define MOD(name, skip) {#name, name##_irx, &size_##name##_irx, skip},
static const struct module modules[] = {
#include "modules.def"
};
#undef MOD

static int test_main(void) {
    int loaded = 0, failed = 0, skipped = 0;
    char line[96];
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    for (unsigned i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
        const struct module *m = &modules[i];
        if (m->skip) {
            skipped++;
            snprintf(line, sizeof(line), "iop-boot: SKIP %s (%s)\n", m->name, m->skip);
            say(line);
            continue;
        }
        snprintf(line, sizeof(line), "iop-boot: loading %s\n", m->name);
        say(line);
        int result = 0;
        int id = SifExecModuleBuffer(m->irx, *m->size, 0, NULL, &result);
        if (id < 0) {
            failed++;
            snprintf(line, sizeof(line), "iop-boot: module %s failed to load (%d)\n", m->name, id);
            say(line);
        } else {
            loaded++;
        }
    }
    snprintf(line, sizeof(line), "iop-boot: %d loaded, %d skipped, %d failed\n", loaded, skipped, failed);
    say(line);
    return failed;
}

int main(void) {
    int failed = test_main();
    say(failed ? "iop-boot: FAIL\n" : "iop-boot: PASS\n");
    printf("[ps2rig] exit=%d\n", failed ? 1 : 0);
    SleepThread();
    return 0;
}
