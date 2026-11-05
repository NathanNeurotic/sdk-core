#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <crashscreen.h>

int kputs(char *s);

static void hook(const char *report)
{
    int ok = strstr(report, "Breakpoint") && strstr(report, "PC=") && strstr(report, "BadVAddr=");
    kputs(ok ? "crash-break: PASS\n" : "crash-break: FAIL report\n");
}

static int trigger(void)
{
    __asm__ volatile("break"); return 0;
}

int main(void)
{
    crashscreen_install();
    crashscreen_set_hook(hook);
    int r = trigger();
    kputs("crash-break: FAIL no exception\n");
    SleepThread();
    return r;
}
