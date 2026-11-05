#include <cstdio>
#include <cstring>
#include <stdexcept>

extern "C" {
#include <kernel.h>
#include <crashscreen.h>
int kputs(char *s);
}

static void hook(const char *report) {
    bool ok = strstr(report, "std::terminate") && strstr(report, "boom") && strstr(report, "Backtrace");
    kputs(ok ? (char *)"crash-terminate: PASS\n" : (char *)"crash-terminate: FAIL report\n");
    SleepThread();
}

int main() {
    crashscreen_install();
    crashscreen_set_hook(hook);
    throw std::runtime_error("boom");
}
