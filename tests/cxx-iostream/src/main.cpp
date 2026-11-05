#include <cstdio>
#include <cstring>

extern "C" {
#include <kernel.h>
}

#include <iostream>
#include <sstream>

extern "C" int kputs(char *s);

/* A program must never return from main on the rig: that reboots the console. */
static int rig_failed;
static int rig_kputs(char *s) {
    if (strstr(s, ": FAIL")) rig_failed = 1;
    return kputs(s);
}
#define kputs rig_kputs

static int test_main() {
    std::ostringstream oss;
    oss << "ostringstream: " << (1 + 41) << " " << 3.5 << "\n";
    std::string text = oss.str();
    if (text != "ostringstream: 42 3.5\n") {
        kputs((char *)"cxx-iostream: FAIL ostringstream\n");
        return 1;
    }

    std::cout << "cout line" << std::endl;

    kputs((char *)"cxx-iostream: PASS\n");
    return 0;
}

int main() {
    int result = test_main();
    printf("[ps2rig] exit=%d\n", (result || rig_failed) ? 1 : 0);
    SleepThread();
    return 0;
}
