#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <crashscreen.h>

int kputs(char *s);

static void hook(const char *report)
{
    int ok = strstr(report, "Reserved instruction") && strstr(report, "PC=") && strstr(report, "BadVAddr=");
    kputs(ok ? "crash-ri: PASS\n" : "crash-ri: FAIL report\n");
}

static int trigger(void)
{
    __asm__ volatile(".word 0x74000000"); return 0;
}

int main(void)
{
    crashscreen_install();
    crashscreen_set_hook(hook);
    int r = trigger();
    kputs("crash-ri: FAIL no exception\n");
    SleepThread();
    return r;
}
