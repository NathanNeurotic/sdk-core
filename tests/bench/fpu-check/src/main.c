#include <stdio.h>
#include <string.h>
#include <math.h>
#include <kernel.h>

extern int kputs(char *s);

typedef unsigned int u32;

static volatile u32 in[16] = {
    0x3f800000, /* 1.0 */
    0x33c00000, /* 1.5 * 2^-24 */
    0x3f8ccccd, /* 1.1 */
    0x40400000, /* 3.0 */
    0x00000000, /* 0.0 */
    0x7f7fffff, /* FLT_MAX */
    0x00800000, /* FLT_MIN */
    0x00000001, /* smallest denormal */
    0x40000000, /* 2.0 */
    0xc0800000, /* -4.0 */
    0x40200000, /* 2.5 */
    0xc0200000, /* -2.5 */
    0x4f32d05e, /* 3e9 */
    0x7fc00000, /* quiet NaN bits */
    0x7f800000, /* +Inf bits */
    0x80000000, /* -0.0 */
};

static float f(int i)
{
    u32 b = in[i];
    float v;

    memcpy(&v, &b, sizeof(v));
    return v;
}

static u32 bits(float v)
{
    u32 b;

    memcpy(&b, &v, sizeof(b));
    return b;
}

static char line[96];

static void show(const char *name, u32 run, u32 folded)
{
    snprintf(line, sizeof(line), "fpu %-14s %08x %08x%s\n", name, run, folded, run == folded ? "" : " differ");
    kputs(line);
    printf("%s", line);
}

__attribute__((noinline)) static float fma_like(float a, float b, float c)
{
    return a * b + c;
}

int main(void)
{
    show("add_round", bits(f(0) + f(1)), bits(1.0f + 0x1.8p-24f));
    show("mul_round", bits(f(2) * f(2)), bits(1.1f * 1.1f));
    show("div_third", bits(f(0) / f(3)), bits(1.0f / 3.0f));
    show("div_zero", bits(f(0) / f(4)), bits(1.0f / 0.0f));
    show("zero_zero", bits(f(4) / f(4)), bits(0.0f / 0.0f));
    show("overflow", bits(f(5) * f(8)), bits(0x1.fffffep127f * 2.0f));
    show("underflow", bits(f(6) * 0.5f * f(0)), bits(0x1p-126f * 0.5f));
    show("denorm_in", bits(f(7) + f(4)), bits(0x1p-149f + 0.0f));
    show("sqrt2", bits(sqrtf(f(8))), bits(sqrtf(2.0f)));
    show("sqrt_neg", bits(sqrtf(f(9))), bits(sqrtf(-4.0f)));
    show("rsqrt3", bits(f(0) / sqrtf(f(3))), bits(1.0f / sqrtf(3.0f)));
    show("cvt_2.5", (u32)(int)f(10), (u32)(int)2.5f);
    show("cvt_-2.5", (u32)(int)f(11), (u32)(int)-2.5f);
    show("cvt_3e9", (u32)(int)f(12), 0);
    show("i2f_2^24+1", bits((float)(int)(in[0] >> 6 | 1)), bits((float)16646145));
    show("fma_like", bits(fma_like(f(2), f(2), -f(0))), bits(1.1f * 1.1f - 1.0f));
    show("nan_eq", f(13) == f(13), 1);
    show("inf_plus_1", bits(f(14) + f(0)), 0);
    show("neg_zero_abs", bits(fabsf(f(15))), bits(fabsf(-0.0f)));
    show("neg_zero_add", bits(f(15) + f(4)), bits(-0.0f + 0.0f));
    show("fmax", bits(fmaxf(f(9), f(8))), bits(fmaxf(-4.0f, 2.0f)));
    show("fmin_nan", bits(fminf(f(13), f(8))), bits(fminf(__builtin_nanf(""), 2.0f)));
    kputs("fpu-check: done\n");
    printf("[ps2rig] exit=0\n");
    SleepThread();
    return 0;
}
