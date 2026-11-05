#include <stdio.h>
#include <string.h>
#include <kernel.h>

#include <math.h>
#include <stdint.h>
#include <string.h>

extern int kputs(char *s);

/* A program must never return from main on the rig: that reboots the console. */
static int rig_failed;
static int rig_kputs(char *s) {
    if (strstr(s, ": FAIL")) rig_failed = 1;
    printf("%s", s);
    return kputs(s);
}
#define kputs rig_kputs

#define NOINL __attribute__((noinline))

/* Includes NaN, infinities, signed zeros, denormals and extremes on purpose: the R5900 FPU has no NaN or
   infinity, so what each comparison returns for them is whatever the instruction sequence does. */
static volatile uint32_t vtbl[] = {
    0x00000000u, 0x80000000u, 0x3f800000u, 0xbf800000u, 0x7fc00000u, 0xffc00000u, 0x7f800000u,
    0xff800000u, 0x00000001u, 0x80000001u, 0x007fffffu, 0x00800000u, 0x7f7fffffu, 0xff7fffffu,
    0x7fffffffu, 0x3f800001u, 0x40490fdbu, 0xc0490fdbu};
#define N ((int)(sizeof(vtbl) / sizeof(vtbl[0])))

static uint32_t h;
static inline void mix(uint32_t v) { h = (h ^ v) * 16777619u; }
static inline uint32_t bits(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }
static inline float val(int i) { float f; uint32_t u = vtbl[i]; memcpy(&f, &u, 4); return f; }

static void hex8(uint32_t v, char *o) {
    for (int i = 7; i >= 0; i--) { o[i] = "0123456789abcdef"[v & 15]; v >>= 4; }
    o[8] = 0;
}
static void report(const char *name) {
    char line[64];
    int n = 0;
    while (name[n] && n < 40) { line[n] = name[n]; n++; }
    line[n++] = ' ';
    hex8(h, line + n);
    n += 8;
    line[n++] = '\n';
    line[n] = 0;
    kputs(line);
}

NOINL static int lt(float a, float b) { return a < b; }
NOINL static int le(float a, float b) { return a <= b; }
NOINL static int gt(float a, float b) { return a > b; }
NOINL static int ge(float a, float b) { return a >= b; }
NOINL static int eq(float a, float b) { return a == b; }
NOINL static int ne(float a, float b) { return a != b; }
NOINL static int unordered(float a, float b) { return __builtin_isunordered(a, b); }
NOINL static int isn(float a) { return a != a; }
NOINL static float mx(float a, float b) { return a > b ? a : b; }
NOINL static float mn(float a, float b) { return a < b ? a : b; }
NOINL static float fmxf(float a, float b) { return fmaxf(a, b); }
NOINL static float fmnf(float a, float b) { return fminf(a, b); }
NOINL static float clamp(float a, float lo, float hi) { return a < lo ? lo : (a > hi ? hi : a); }
NOINL static float sq(float a) { return sqrtf(a); }
NOINL static float absf(float a) { return fabsf(a); }

#define CMP(fn) do { h = 2166136261u; for (int i = 0; i < N; i++) for (int j = 0; j < N; j++) mix((uint32_t)fn(val(i), val(j))); report(#fn); } while (0)
#define BIN(fn) do { h = 2166136261u; for (int i = 0; i < N; i++) for (int j = 0; j < N; j++) mix(bits(fn(val(i), val(j)))); report(#fn); } while (0)

static int test_main(void) {
    CMP(lt); CMP(le); CMP(gt); CMP(ge); CMP(eq); CMP(ne); CMP(unordered);
    h = 2166136261u; for (int i = 0; i < N; i++) mix((uint32_t)isn(val(i))); report("isnan");
    BIN(mx); BIN(mn); BIN(fmxf); BIN(fmnf);
    h = 2166136261u;
    for (int i = 0; i < N; i++) for (int j = 0; j < N; j++) for (int k = 0; k < N; k++) mix(bits(clamp(val(i), val(j), val(k))));
    report("clamp");
    h = 2166136261u; for (int i = 0; i < N; i++) mix(bits(sq(val(i)))); report("sqrtf");
    h = 2166136261u; for (int i = 0; i < N; i++) mix(bits(absf(val(i)))); report("fabsf");
    kputs("fp-compare: PASS\n");
    return 0;
}

int main(void) {
    int result = test_main();
    printf("[ps2rig] exit=%d\n", (result || rig_failed) ? 1 : 0);
    SleepThread();
    return 0;
}
