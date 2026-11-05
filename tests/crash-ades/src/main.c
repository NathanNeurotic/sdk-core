#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <crashscreen.h>

int kputs(char *s);

static void hook(const char *report)
{
    int ok = strstr(report, "Address error on store") && strstr(report, "PC=") && strstr(report, "BadVAddr=");
    kputs(ok ? "crash-ades: PASS\n" : "crash-ades: FAIL report\n");
}

static int trigger(void)
{
    volatile int *p = (volatile int *)0x00100001; *p = 1; return 0;
}

int main(void)
{
    crashscreen_install();
    crashscreen_set_hook(hook);
    int r = trigger();
    kputs("crash-ades: FAIL no exception\n");
    SleepThread();
    return r;
}
