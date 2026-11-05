#include <cstdio>
#include <cstring>

extern "C" {
#include <kernel.h>
}

#include <exception>
#include <stdexcept>
#include <string>
#include <thread>
#include <atomic>

extern "C" int kputs(char *s);

/* A program must never return from main on the rig: that reboots the console. */
static int rig_failed;
static int rig_kputs(char *s) {
    if (strstr(s, ": FAIL")) rig_failed = 1;
    return kputs(s);
}
#define kputs rig_kputs

static int dtors;
struct Guard {
    ~Guard() { dtors++; }
};

struct Base { virtual ~Base() {} virtual int id() const { return 1; } };
struct Derived : Base { int id() const override { return 2; } };

static int deep(int n) {
    Guard g;
    if (n == 0) throw std::runtime_error("deep");
    return deep(n - 1) + 1;
}

static int through_noexcept_boundary() {
    try { return deep(5); } catch (...) { throw; }
}

static std::atomic<int> thread_result{0};

static int test_main() {
    int fails = 0;
    auto check = [&](bool ok, int line) { if (!ok) { fails++; (void)line; } };

    // unwinding through six frames runs every destructor
    try { deep(5); check(false, __LINE__); } catch (const std::runtime_error &e) { check(std::string(e.what()) == "deep", __LINE__); }
    check(dtors == 6, __LINE__);

    // rethrow keeps the exception type
    try { through_noexcept_boundary(); check(false, __LINE__); } catch (const std::exception &e) { check(std::string(e.what()) == "deep", __LINE__); }

    // catch by base class, catch (...) and polymorphic throw
    try { throw Derived(); } catch (const Base &b) { check(b.id() == 2, __LINE__); }
    try { throw 7; } catch (...) { check(true, __LINE__); }
    try { throw 7; } catch (int v) { check(v == 7, __LINE__); }

    // an exception thrown and caught in another thread
    std::thread t([] {
        try { throw std::logic_error("thread"); } catch (const std::logic_error &) { thread_result = 5; }
    });
    t.join();
    check(thread_result == 5, __LINE__);

    // nested exceptions
    try {
        try { throw std::runtime_error("inner"); }
        catch (...) { std::throw_with_nested(std::runtime_error("outer")); }
    } catch (const std::exception &e) {
        try { std::rethrow_if_nested(e); check(false, __LINE__); }
        catch (const std::exception &inner) { check(std::string(inner.what()) == "inner", __LINE__); }
    }

    kputs(fails == 0 ? (char *)"cxx-exceptions: PASS\n" : (char *)"cxx-exceptions: FAIL\n");
    return 0;
}

int main() {
    int result = test_main();
    printf("[ps2rig] exit=%d\n", (result || rig_failed) ? 1 : 0);
    SleepThread();
    return 0;
}
