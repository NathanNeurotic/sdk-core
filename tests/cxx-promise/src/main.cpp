#include <cstdio>
#include <cstring>

extern "C" {
#include <kernel.h>
}

#include <cstdio>
#include <future>
#include <thread>

extern "C" int kputs(char *s);

/* A program must never return from main on the rig: that reboots the console. */
static int rig_failed;
static int rig_kputs(char *s) {
    if (strstr(s, ": FAIL")) rig_failed = 1;
    return kputs(s);
}
#define kputs rig_kputs

static int test_main() {
    std::promise<int> prom;
    std::future<int> fut = prom.get_future();

    std::thread worker([&prom]() { prom.set_value(21 * 2); });

    int result = fut.get();
    worker.join();

    if (result != 42) {
        kputs((char *)"cxx-promise: FAIL wrong result\n");
        return 1;
    }

    kputs((char *)"cxx-promise: PASS\n");
    return 0;
}

int main() {
    int result = test_main();
    printf("[ps2rig] exit=%d\n", (result || rig_failed) ? 1 : 0);
    SleepThread();
    return 0;
}
