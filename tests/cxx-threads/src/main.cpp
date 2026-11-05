#include <cstdio>
#include <cstring>

extern "C" {
#include <kernel.h>
}

#include <chrono>
#include <future>
#include <numeric>
#include <thread>
#include <vector>

extern "C" int kputs(char *s);

/* A program must never return from main on the rig: that reboots the console. */
static int rig_failed;
static int rig_kputs(char *s) {
    if (strstr(s, ": FAIL")) rig_failed = 1;
    return kputs(s);
}
#define kputs rig_kputs

static void accumulate(std::vector<int>::iterator first, std::vector<int>::iterator last,
                       std::promise<int> result) {
    result.set_value(std::accumulate(first, last, 0));
}

static void signal_after_sleep(std::promise<void> barrier) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    barrier.set_value();
}

static int test_main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5, 6};
    std::promise<int> sum_promise;
    std::future<int> sum_future = sum_promise.get_future();
    std::thread summer(accumulate, numbers.begin(), numbers.end(), std::move(sum_promise));
    int sum = sum_future.get();
    summer.join();

    std::promise<void> barrier;
    std::future<void> barrier_future = barrier.get_future();
    std::thread sleeper(signal_after_sleep, std::move(barrier));
    barrier_future.wait();
    sleeper.join();

    if (sum != 21) {
        kputs((char *)"cxx-threads: FAIL wrong sum\n");
        return 1;
    }

    kputs((char *)"cxx-threads: PASS\n");
    return 0;
}

int main() {
    int result = test_main();
    printf("[ps2rig] exit=%d\n", (result || rig_failed) ? 1 : 0);
    SleepThread();
    return 0;
}
