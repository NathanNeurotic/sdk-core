#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <crashscreen.h>

int kputs(char *s);

static void hook(const char *report)
{
    int ok = strstr(report, "Trap") && strstr(report, "PC=") && strstr(report, "BadVAddr=");
    kputs(ok ? "crash-trap: PASS\n" : "crash-trap: FAIL report\n");
}

static int trigger(void)
{
    int z = 1; __asm__ volatile("teq %0, $0" :: "r"(z - 1)); return 0;
}

int main(void)
{
    crashscreen_install();
    crashscreen_set_hook(hook);
    int r = trigger();
    kputs("crash-trap: FAIL no exception\n");
    SleepThread();
    return r;
}
