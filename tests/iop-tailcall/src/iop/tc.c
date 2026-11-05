#include <irx.h>
#include <stdio.h>

IRX_ID("tc", 1, 1);

stdio_IMPORTS_start
I_printf
stdio_IMPORTS_end

#define NOINLINE __attribute__((noinline))

NOINLINE int sum5(int a, int b, int c, int d, int e) { return a + 2 * b + 3 * c + 4 * d + 5 * e; }
NOINLINE int sum6(int a, int b, int c, int d, int e, int f) { return a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * f; }
NOINLINE int sum2(int a, int b) { return a * 10 + b; }
NOINLINE int twice(int x) { return x * 2; }

NOINLINE int fwd5(int a, int b, int c, int d, int e) { return sum5(a, b, c, d, e); }
NOINLINE int fwd5_rev(int a, int b, int c, int d, int e) { return sum5(e, d, c, b, a); }
NOINLINE int fwd6(int a, int b, int c, int d, int e, int f) { return sum6(a, b, c, d, e, f); }
NOINLINE int fwd6_rot(int a, int b, int c, int d, int e, int f) { return sum6(f, a, b, c, d, e); }
NOINLINE int grow3(int a, int b, int c) { return sum5(a, b, c, 7, 9); }
NOINLINE int shrink(int a, int b, int c, int d, int e) { return sum2(a + e, b + d); }
NOINLINE int after_call(int x) { int y = twice(x); return sum2(y + 1, x); }

static int (*volatile fp5)(int, int, int, int, int) = sum5;
static int (*volatile fp2)(int, int) = sum2;
NOINLINE int via_ptr5(int a, int b, int c, int d, int e) { return fp5(a, b, c, d, e); }
NOINLINE int via_ptr2(int a, int b) { return fp2(b, a); }

NOINLINE int chain(int a) { return after_call(twice(a)); }

static int bad;
#define CHECK(expr, want) do { int got = (expr); if (got != (want)) { bad++; printf("tc: %s = %d, want %d\n", #expr, got, (int)(want)); } } while (0)

int _start(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    CHECK(fwd5(1, 2, 3, 4, 5), 1 + 4 + 9 + 16 + 25);
    CHECK(fwd5_rev(1, 2, 3, 4, 5), 5 + 8 + 9 + 8 + 5);
    CHECK(fwd6(1, 2, 3, 4, 5, 6), 1 + 4 + 9 + 16 + 25 + 36);
    CHECK(fwd6_rot(1, 2, 3, 4, 5, 6), 6 + 2 + 6 + 12 + 20 + 30);
    CHECK(grow3(1, 2, 3), 1 + 4 + 9 + 28 + 45);
    CHECK(shrink(1, 2, 3, 4, 5), (1 + 5) * 10 + (2 + 4));
    CHECK(after_call(3), 7 * 10 + 3);
    CHECK(via_ptr5(5, 4, 3, 2, 1), 5 + 8 + 9 + 8 + 5);
    CHECK(via_ptr2(3, 4), 40 + 3);
    CHECK(chain(2), (8 + 1) * 10 + 4);

    printf("tc: %s\n", bad ? "mismatch" : "ok");
    return bad ? 2 : 1;
}
