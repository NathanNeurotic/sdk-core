#include <cstdio>
#include <cstring>

extern "C" {
#include <kernel.h>
}

#include <coroutine>

extern "C" int kputs(char *s);

/* A program must never return from main on the rig: that reboots the console. */
static int rig_failed;
static int rig_kputs(char *s) {
    if (strstr(s, ": FAIL")) rig_failed = 1;
    return kputs(s);
}
#define kputs rig_kputs

struct Generator {
    struct promise_type {
        int value = 0;
        Generator get_return_object() { return Generator{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(int v) noexcept { value = v; return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept {}
    };

    std::coroutine_handle<promise_type> handle;

    explicit Generator(std::coroutine_handle<promise_type> h) : handle(h) {}
    Generator(Generator &&other) noexcept : handle(other.handle) { other.handle = nullptr; }
    ~Generator() { if (handle) handle.destroy(); }

    bool next() {
        handle.resume();
        return !handle.done();
    }
    int value() const { return handle.promise().value; }
};

static Generator count_to(int n) {
    for (int i = 1; i <= n; i++)
        co_yield i;
}

static int test_main() {
    Generator gen = count_to(5);
    int sum = 0;
    while (gen.next())
        sum += gen.value();

    if (sum != 15) {
        kputs((char *)"cxx-coroutines: FAIL wrong sum\n");
        return 1;
    }

    kputs((char *)"cxx-coroutines: PASS\n");
    return 0;
}

int main() {
    int result = test_main();
    printf("[ps2rig] exit=%d\n", (result || rig_failed) ? 1 : 0);
    SleepThread();
    return 0;
}
