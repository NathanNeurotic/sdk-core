#include <cstdio>
#include <cstring>

extern "C" {
#include <kernel.h>
}

#include <atomic>
#include <thread>

extern "C" int kputs(char *s);

/* A program must never return from main on the rig: that reboots the console. */
static int rig_failed;
static int rig_kputs(char *s) {
    if (strstr(s, ": FAIL")) rig_failed = 1;
    return kputs(s);
}
#define kputs rig_kputs

static std::atomic<int> destroyed{0};

struct Tracked {
    int value;
    explicit Tracked(int v) : value(v) {}
    ~Tracked() { destroyed++; }
};

thread_local int counter = 100;
thread_local Tracked tracked{7};

static std::atomic<int> sums[2];

static void worker(int index) {
    counter += index + 1;
    tracked.value += index;
    sums[index] = counter * 1000 + tracked.value;
}

static int test_main() {
    counter = 5;
    std::thread a(worker, 0);
    std::thread b(worker, 1);
    a.join();
    b.join();

    bool ok = counter == 5 && tracked.value == 7;
    ok = ok && sums[0] == 101 * 1000 + 7 && sums[1] == 102 * 1000 + 8;
    ok = ok && destroyed == 2;
    kputs(ok ? const_cast<char *>("cxx-thread-local: PASS\n") : const_cast<char *>("cxx-thread-local: FAIL\n"));
    return 0;
}

int main() {
    int result = test_main();
    printf("[ps2rig] exit=%d\n", (result || rig_failed) ? 1 : 0);
    SleepThread();
    return 0;
}
