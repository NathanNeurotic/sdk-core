#include <stdio.h>
#include <string.h>
#include <kernel.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

static uint32_t h;
static inline void mix(uint32_t v) { h = (h ^ v) * 16777619u; }
static inline uint32_t bits(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }
static inline void mixd(double d) { uint64_t u; memcpy(&u, &d, 8); mix((uint32_t)u); mix((uint32_t)(u >> 32)); }
static void hex8(uint32_t v, char *o) { for (int i = 7; i >= 0; i--) { o[i] = "0123456789abcdef"[v & 15]; v >>= 4; } o[8] = 0; }
static void report(const char *name) {
    char line[64]; int n = 0;
    while (name[n] && n < 40) { line[n] = name[n]; n++; }
    line[n++] = ' '; hex8(h, line + n); n += 8; line[n++] = '\n'; line[n] = 0; kputs(line);
}
#define START() (h = 2166136261u)

static volatile uint32_t vbits[] = {
    0x3f800000u, 0x40000000u, 0x3f000000u, 0x41200000u, 0x3dcccccdu, 0x3a83126fu, 0x42f6e979u, 0x461c4000u,
    0x40490fdbu, 0x3fc90fdbu, 0x3f490fdbu, 0x3e99999au, 0x44fa0000u, 0x4b800000u, 0x3b03126fu, 0x41a00000u,
    0x3f7fffffu, 0x40a00000u, 0x40e00000u, 0x42c80000u, 0x3f9e0419u, 0x3ea2f983u, 0x40135d8eu, 0x41c80000u};
#define N ((int)(sizeof(vbits) / sizeof(vbits[0])))
static float t[64];

#define F1(NAME, EXPR) NOINL static float NAME(float x) { return EXPR; }
F1(m_sinf, sinf(x)) F1(m_cosf, cosf(x)) F1(m_tanf, tanf(x)) F1(m_asinf, asinf(x / 100.0f))
F1(m_acosf, acosf(x / 100.0f)) F1(m_atanf, atanf(x)) F1(m_expf, expf(x / 8.0f)) F1(m_logf, logf(x))
F1(m_log10f, log10f(x)) F1(m_sqrtf, sqrtf(x)) F1(m_floorf, floorf(x * 1.3f)) F1(m_ceilf, ceilf(x * 1.3f))
F1(m_roundf, roundf(x * 1.3f)) F1(m_truncf, truncf(x * 1.3f)) F1(m_fabsf, fabsf(-x)) F1(m_sinhf, sinhf(x / 8.0f))
F1(m_coshf, coshf(x / 8.0f)) F1(m_tanhf, tanhf(x / 8.0f)) F1(m_cbrtf, cbrtf(x)) F1(m_exp2f, exp2f(x / 4.0f))
F1(m_log2f, log2f(x))
#define F2(NAME, EXPR) NOINL static float NAME(float x, float y) { return EXPR; }
F2(m_powf, powf(x, y / 4.0f)) F2(m_atan2f, atan2f(x, y)) F2(m_fmodf, fmodf(x, y)) F2(m_hypotf, hypotf(x, y))
F2(m_fmaxf, fmaxf(x, y)) F2(m_fminf, fminf(x, y)) F2(m_copysignf, copysignf(x, -y)) F2(m_remainderf, remainderf(x, y))
#define D1(NAME, EXPR) NOINL static double NAME(double x) { return EXPR; }
D1(d_sin, sin(x)) D1(d_cos, cos(x)) D1(d_sqrt, sqrt(x)) D1(d_exp, exp(x / 8.0)) D1(d_log, log(x)) D1(d_atan, atan(x))
D1(d_floor, floor(x * 1.3)) D1(d_fabs, fabs(-x))
NOINL static double d_add(double a, double b) { return a + b; }
NOINL static double d_sub(double a, double b) { return a - b; }
NOINL static double d_mul(double a, double b) { return a * b; }
NOINL static double d_div(double a, double b) { return a / b; }
NOINL static double d_pow(double a, double b) { return pow(a, b / 4.0); }
NOINL static double f2d(float x) { return (double)x; }
NOINL static float d2f(double x) { return (float)x; }
NOINL static double i2d(int x) { return (double)x; }
NOINL static int d2i(double x) { return (int)x; }
NOINL static double u2d(unsigned x) { return (double)x; }
NOINL static long long f2ll(float x) { return (long long)x; }
NOINL static float ll2f(long long x) { return (float)x; }
NOINL static unsigned f2u(float x) { return (unsigned)x; }
NOINL static float u2f(unsigned x) { return (float)x; }

static int test_main(void) {
    for (int i = 0; i < N; i++) { uint32_t u = vbits[i]; memcpy(&t[i], &u, 4); }
#define R1(K) do { START(); for (int i = 0; i < N; i++) mix(bits(K(t[i]))); report(#K); } while (0)
#define R2(K) do { START(); for (int i = 0; i < N; i++) for (int j = 0; j < N; j++) mix(bits(K(t[i], t[j]))); report(#K); } while (0)
#define RD1(K) do { START(); for (int i = 0; i < N; i++) mixd(K((double)t[i])); report(#K); } while (0)
#define RD2(K) do { START(); for (int i = 0; i < N; i++) for (int j = 0; j < N; j++) mixd(K((double)t[i], (double)t[j])); report(#K); } while (0)
    R1(m_sinf); R1(m_cosf); R1(m_tanf); R1(m_asinf); R1(m_acosf); R1(m_atanf); R1(m_expf); R1(m_logf);
    R1(m_log10f); R1(m_sqrtf); R1(m_floorf); R1(m_ceilf); R1(m_roundf); R1(m_truncf); R1(m_fabsf);
    R1(m_sinhf); R1(m_coshf); R1(m_tanhf); R1(m_cbrtf); R1(m_exp2f); R1(m_log2f);
    R2(m_powf); R2(m_atan2f); R2(m_fmodf); R2(m_hypotf); R2(m_fmaxf); R2(m_fminf); R2(m_copysignf); R2(m_remainderf);
    RD1(d_sin); RD1(d_cos); RD1(d_sqrt); RD1(d_exp); RD1(d_log); RD1(d_atan); RD1(d_floor); RD1(d_fabs);
    RD2(d_add); RD2(d_sub); RD2(d_mul); RD2(d_div); RD2(d_pow);
    START(); for (int i = 0; i < N; i++) mixd(f2d(t[i])); report("f2d");
    START(); for (int i = 0; i < N; i++) mix(bits(d2f((double)t[i] * 1.0000001))); report("d2f");
    START(); for (int i = -100; i < 100; i++) mixd(i2d(i * 123457)); report("i2d");
    START(); for (int i = 0; i < N; i++) mix((uint32_t)d2i((double)t[i] * 1000.0)); report("d2i");
    START(); for (unsigned i = 0; i < 100; i++) mixd(u2d(i * 40000001u)); report("u2d");
    START(); for (int i = 0; i < N; i++) { long long v = f2ll(t[i] * 100000.0f); mix((uint32_t)v); mix((uint32_t)(v >> 32)); } report("f2ll");
    START(); for (int i = -50; i < 50; i++) mix(bits(ll2f((long long)i * 1234567891LL))); report("ll2f");
    START(); for (int i = 0; i < N; i++) mix(f2u(t[i] * 3.0f)); report("f2u");
    START(); for (unsigned i = 0; i < 100; i++) mix(bits(u2f(i * 40000001u))); report("u2f");

    /* formatted output and parsing */
    {
        char buf[96];
        START();
        for (int i = 0; i < N; i++) {
            int n = snprintf(buf, sizeof buf, "%f %.3f %e %g", (double)t[i], (double)t[i], (double)t[i], (double)t[i]);
            for (int k = 0; k < n; k++) mix((uint32_t)buf[k]);
        }
        report("snprintf_f");
        START();
        for (int i = 0; i < N; i++) {
            int n = snprintf(buf, sizeof buf, "%d %u %x %5.2f %s %c %ld", i * -7, i * 90000u, i * 77, (double)t[i], "str", 'a' + i % 20, (long)i * 100000L);
            for (int k = 0; k < n; k++) mix((uint32_t)buf[k]);
        }
        report("snprintf_i");
        START();
        {
            static const char *strs[] = {"1.5", "0.1", "3.14159", "1e10", "123456.789", "-2.5e-3", "100", "0.333333333", "65504", "1e-5", "2.718281828", "17"};
            for (int i = 0; i < 12; i++) { mix(bits(strtof(strs[i], NULL))); mixd(strtod(strs[i], NULL)); mix((uint32_t)atoi(strs[i])); }
        }
        report("strto");
    }
    {
        START();
        srand(12345);
        for (int i = 0; i < 100; i++) mix((uint32_t)rand());
        report("rand");
        START();
        for (int i = 0; i < 64; i++) mix((uint32_t)abs((int)(i * 1000003u) - 30000000));
        report("abs");
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
