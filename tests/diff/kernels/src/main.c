#include <stdio.h>
#include <string.h>
#include <kernel.h>

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

static volatile uint32_t vtbl[] = {
    0x00000000u, 0x80000000u, 0x3f800000u, 0xbf800000u, 0x3f000000u, 0x40000000u,
    0x40400000u, 0x41200000u, 0x3dcccccdu, 0x3a83126fu, 0x1e3ce508u, 0x60ad78ecu,
    0x7f7fc99eu, 0xff7fc99eu, 0x0082ab1eu, 0x47f12065u, 0x3f800001u, 0x4b800000u,
    0x3eaaaaaau, 0xc0f00000u, 0x42c80000u, 0x501502f9u, 0x2edbe6ffu, 0x477fe000u,
    0xbf400000u, 0x40200000u, 0x3fc00000u, 0x47c35000u};
#define N ((int)(sizeof(vtbl) / sizeof(vtbl[0])))

static float t[64];

static uint32_t h;
static inline void mix(uint32_t v) { h = (h ^ v) * 16777619u; }
static inline uint32_t bits(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }

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
#define START() (h = 2166136261u)

#define BIN(NAME, EXPR) \
    NOINL static float NAME(float a, float b) { return EXPR; }
BIN(k_add, a + b)
BIN(k_sub, a - b)
BIN(k_mul, a * b)
BIN(k_div, a / b)
BIN(k_max, __builtin_fmaxf(a, b))
BIN(k_min, __builtin_fminf(a, b))
BIN(k_tmax, a > b ? a : b)
BIN(k_tmin, a < b ? a : b)
BIN(k_adivsqrt, a / __builtin_sqrtf(b))
BIN(k_lerp3, a + (b - a) * 0.25f)
BIN(k_chain, (a * b + a) * b - a / (b + 1.5f))
BIN(k_fma, a * b + 3.0f)
BIN(k_msub, 3.0f - a * b)

#define UN(NAME, EXPR) \
    NOINL static float NAME(float a) { return EXPR; }
UN(k_sqrt, __builtin_sqrtf(a))
UN(k_rsqrt, 1.0f / __builtin_sqrtf(a))
UN(k_neg, -a)
UN(k_abs, __builtin_fabsf(a))
UN(k_clamp, a < 0.f ? 0.f : (a > 1.f ? 1.f : a))
UN(k_sq, a * a)
UN(k_cube, a * a * a)
UN(k_half, a * 0.5f + 0x1.921f9ep+1f)
UN(k_recip, 1.0f / a)

#define CMP(NAME, EXPR) \
    NOINL static int NAME(float a, float b) { return EXPR; }
CMP(c_lt, a < b)
CMP(c_le, a <= b)
CMP(c_gt, a > b)
CMP(c_ge, a >= b)
CMP(c_eq, a == b)
CMP(c_ne, a != b)
CMP(c_nlt, !(a < b))
CMP(c_nle, !(a <= b))
CMP(c_ngt, !(a > b))
CMP(c_nge, !(a >= b))
NOINL static float s_lt(float a, float b, float x, float y) { return a < b ? x : y; }
NOINL static float s_le(float a, float b, float x, float y) { return a <= b ? x : y; }
NOINL static float s_gt(float a, float b, float x, float y) { return a > b ? x : y; }
NOINL static float s_ge(float a, float b, float x, float y) { return a >= b ? x : y; }
NOINL static float s_eq(float a, float b, float x, float y) { return a == b ? x : y; }
NOINL static float s_ne(float a, float b, float x, float y) { return a != b ? x : y; }
CMP(br_lt, a < b ? 1 : 2)
CMP(br_ge, a >= b ? 1 : 2)
CMP(br_ule, !(a > b) ? 1 : 2)
CMP(br_ugt, !(a <= b) ? 1 : 2)
CMP(br_ne, a != b ? 1 : 2)
CMP(br_eq, a == b ? 1 : 2)

NOINL static int f2i(float a) { return (int)a; }
NOINL static float i2f(int a) { return (float)a; }
NOINL static int f2i_sat(float a) { return a > 2e9f ? 2000000000 : (a < -2e9f ? -2000000000 : (int)a); }

NOINL static float dot4(const float *a, const float *b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
}
NOINL static void axpy(float *y, const float *x, float a, int n) {
    for (int i = 0; i < n; i++) y[i] += a * x[i];
}
NOINL static void mat4vec(float *o, const float *m, const float *v) {
    for (int r = 0; r < 4; r++)
        o[r] = m[r] * v[0] + m[4 + r] * v[1] + m[8 + r] * v[2] + m[12 + r] * v[3];
}
NOINL static float sumf(const float *a, int n) {
    float s = 0;
    for (int i = 0; i < n; i++) s += a[i];
    return s;
}
NOINL static float lensq(const float *a, int n) {
    float s = 0;
    for (int i = 0; i < n; i++) s += a[i] * a[i];
    return s;
}
NOINL static float norm3(const float *v) {
    float l = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    return l / __builtin_sqrtf(l);
}

/* integer and memory kernels: stress the scheduler, hi/lo, delay slots */
NOINL static uint32_t crc32_(const uint8_t *p, int n) {
    uint32_t c = 0xffffffffu;
    for (int i = 0; i < n; i++) {
        c ^= p[i];
        for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1)));
    }
    return ~c;
}
NOINL static int32_t imul(int32_t a, int32_t b) { return a * b; }
NOINL static int64_t imul64(int64_t a, int64_t b) { return a * b; }
NOINL static int32_t idiv(int32_t a, int32_t b) { return b ? a / b : 0; }
NOINL static int32_t imod(int32_t a, int32_t b) { return b ? a % b : 0; }
NOINL static uint32_t udiv(uint32_t a, uint32_t b) { return b ? a / b : 0; }
NOINL static uint32_t umod(uint32_t a, uint32_t b) { return b ? a % b : 0; }
NOINL static int64_t idiv64(int64_t a, int64_t b) { return b ? a / b : 0; }
NOINL static uint64_t umul64(uint32_t a, uint32_t b) { return (uint64_t)a * b; }
NOINL static void bsort(int *a, int n) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j + 1 < n - i; j++)
            if (a[j] > a[j + 1]) { int x = a[j]; a[j] = a[j + 1]; a[j + 1] = x; }
}
NOINL static int fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }
NOINL static int sw(int x) {
    switch (x & 7) {
    case 0: return x * 3;
    case 1: return x + 100;
    case 2: return x ^ 0x5a5a;
    case 3: return x << 3;
    case 4: return -x;
    case 5: return x * x;
    case 6: return x >> 1;
    default: return 7;
    }
}
struct S { int8_t a; int16_t b; int32_t c; int64_t d; float f; uint8_t g; };
NOINL static int64_t structs(struct S *s, int n) {
    int64_t acc = 0;
    for (int i = 0; i < n; i++) {
        acc += s[i].a + s[i].b + s[i].c + s[i].d + (int)s[i].f + s[i].g;
        s[i].c = (int32_t)acc;
    }
    return acc;
}
NOINL static void copyl(long long *d, const long long *s, int n) { for (int i = 0; i < n; i++) d[i] = s[i]; }
NOINL static int isum(const int *a, int n) { int s = 0; for (int i = 0; i < n; i++) s += a[i]; return s; }

static uint32_t rng = 12345;
static uint32_t next(void) { rng = rng * 1664525u + 1013904223u; return rng; }

#define RUN_BIN(K) do { START(); for (int i = 0; i < N; i++) for (int j = 0; j < N; j++) mix(bits(K(t[i], t[j]))); report(#K); } while (0)
#define RUN_UN(K) do { START(); for (int i = 0; i < N; i++) mix(bits(K(t[i]))); report(#K); } while (0)
#define RUN_CMP(K) do { START(); for (int i = 0; i < N; i++) for (int j = 0; j < N; j++) mix((uint32_t)K(t[i], t[j])); report(#K); } while (0)
#define RUN_SEL(K) do { START(); for (int i = 0; i < N; i++) for (int j = 0; j < N; j++) mix(bits(K(t[i], t[j], t[i], t[j]))); report(#K); } while (0)

static int test_main(void) {
    for (int i = 0; i < N; i++) { uint32_t u = vtbl[i]; memcpy(&t[i], &u, 4); }
    for (int i = 0; i < N; i++) {
        char nm[8] = {'t', (char)('a' + i / 10), (char)('0' + i % 10), 0};
        START(); mix(bits(t[i])); report(nm);
    }

    RUN_BIN(k_add); RUN_BIN(k_sub); RUN_BIN(k_mul); RUN_BIN(k_div);
    RUN_BIN(k_max); RUN_BIN(k_min); RUN_BIN(k_tmax); RUN_BIN(k_tmin);
    RUN_BIN(k_adivsqrt); RUN_BIN(k_lerp3); RUN_BIN(k_chain); RUN_BIN(k_fma); RUN_BIN(k_msub);
    RUN_UN(k_sqrt); RUN_UN(k_rsqrt); RUN_UN(k_neg); RUN_UN(k_abs); RUN_UN(k_clamp);
    RUN_UN(k_sq); RUN_UN(k_cube); RUN_UN(k_half); RUN_UN(k_recip);
    RUN_CMP(c_lt); RUN_CMP(c_le); RUN_CMP(c_gt); RUN_CMP(c_ge); RUN_CMP(c_eq); RUN_CMP(c_ne);
    RUN_CMP(c_nlt); RUN_CMP(c_nle); RUN_CMP(c_ngt); RUN_CMP(c_nge);
    RUN_SEL(s_lt); RUN_SEL(s_le); RUN_SEL(s_gt); RUN_SEL(s_ge); RUN_SEL(s_eq); RUN_SEL(s_ne);
    RUN_CMP(br_lt); RUN_CMP(br_ge); RUN_CMP(br_ule); RUN_CMP(br_ugt); RUN_CMP(br_ne); RUN_CMP(br_eq);

    START(); for (int i = 0; i < N; i++) mix((uint32_t)f2i_sat(t[i])); report("f2i_sat");
    START(); for (int i = 0; i < N; i++) if (t[i] < 1e9f && t[i] > -1e9f) mix((uint32_t)f2i(t[i])); report("f2i");
    START(); for (int i = -200; i < 200; i++) mix(bits(i2f(i * 99991))); report("i2f");

    START();
    for (int i = 0; i + 4 <= N; i++) for (int j = 0; j + 4 <= N; j++) mix(bits(dot4(t + i, t + j)));
    report("dot4");
    START();
    for (int i = 0; i + 16 <= N; i++) {
        float o[4];
        mat4vec(o, t + i, t + (i % 8));
        for (int r = 0; r < 4; r++) mix(bits(o[r]));
    }
    report("mat4vec");
    START();
    {
        float y[16];
        for (int a = 0; a < N; a++) {
            for (int i = 0; i < 16; i++) y[i] = t[(i + a) % N];
            axpy(y, t, t[a], 16);
            for (int i = 0; i < 16; i++) mix(bits(y[i]));
        }
    }
    report("axpy");
    START(); for (int n = 0; n <= 16; n++) for (int i = 0; i + n <= N; i++) mix(bits(sumf(t + i, n))); report("sumf");
    START(); for (int n = 0; n <= 16; n++) for (int i = 0; i + n <= N; i++) mix(bits(lensq(t + i, n))); report("lensq");
    START(); for (int i = 0; i + 3 <= N; i++) mix(bits(norm3(t + i))); report("norm3");

    {
        uint8_t buf[256];
        for (int i = 0; i < 256; i++) buf[i] = (uint8_t)next();
        START(); for (int n = 0; n < 256; n += 7) mix(crc32_(buf, n)); report("crc32");
    }
    START(); for (int i = 0; i < 300; i++) { int32_t a = (int32_t)next(), b = (int32_t)next(); mix((uint32_t)imul(a, b)); } report("imul");
    START();
    for (int i = 0; i < 300; i++) {
        int64_t a = ((int64_t)next() << 32) | next(), b = ((int64_t)next() << 16) | next();
        int64_t r = imul64(a, b);
        mix((uint32_t)r); mix((uint32_t)(r >> 32));
    }
    report("imul64");
    START(); for (int i = 0; i < 300; i++) { int32_t a = (int32_t)next(), b = (int32_t)(next() >> (i & 31)); mix((uint32_t)idiv(a, b)); mix((uint32_t)imod(a, b)); } report("idivmod");
    START(); for (int i = 0; i < 300; i++) { uint32_t a = next(), b = next() >> (i & 31); mix(udiv(a, b)); mix(umod(a, b)); } report("udivmod");
    START();
    for (int i = 0; i < 100; i++) {
        int64_t a = ((int64_t)next() << 32) | next();
        int64_t b = (int64_t)(int32_t)next() >> (i & 15);
        int64_t r = idiv64(a, b);
        mix((uint32_t)r); mix((uint32_t)(r >> 32));
    }
    report("idiv64");
    START(); for (int i = 0; i < 100; i++) { uint64_t r = umul64(next(), next()); mix((uint32_t)r); mix((uint32_t)(r >> 32)); } report("umul64");
    {
        int a[64];
        for (int i = 0; i < 64; i++) a[i] = (int)(next() >> 8);
        bsort(a, 64);
        START(); for (int i = 0; i < 64; i++) mix((uint32_t)a[i]); report("bsort");
    }
    START(); mix((uint32_t)fib(20)); report("fib");
    START(); for (int i = 0; i < 400; i++) mix((uint32_t)sw(i * 7 + 3)); report("switch");
    {
        struct S s[16];
        for (int i = 0; i < 16; i++) {
            s[i].a = (int8_t)next(); s[i].b = (int16_t)next(); s[i].c = (int32_t)next();
            s[i].d = ((int64_t)next() << 32) | next(); s[i].f = (float)(int16_t)next(); s[i].g = (uint8_t)next();
        }
        START();
        int64_t r = structs(s, 16);
        mix((uint32_t)r); mix((uint32_t)(r >> 32));
        for (int i = 0; i < 16; i++) mix((uint32_t)s[i].c);
        report("structs");
    }
    {
        long long a[32], b[32];
        for (int i = 0; i < 32; i++) a[i] = ((long long)next() << 32) | next();
        copyl(b, a, 32);
        START();
        for (int i = 0; i < 32; i++) { mix((uint32_t)b[i]); mix((uint32_t)(b[i] >> 32)); }
        report("copyl");
    }
    {
        int a[64];
        for (int i = 0; i < 64; i++) a[i] = (int)next();
        START(); for (int n = 0; n <= 64; n++) mix((uint32_t)isum(a, n)); report("isum");
    }

    kputs("fpdiff: PASS\n");
    return 0;
}

int main(void) {
    int result = test_main();
    printf("[ps2rig] exit=%d\n", (result || rig_failed) ? 1 : 0);
    SleepThread();
    return 0;
}
