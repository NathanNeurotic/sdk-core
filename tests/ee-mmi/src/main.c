#include <stdio.h>
#include <kernel.h>
#include <tamtypes.h>

extern int kputs(char *s);

typedef unsigned long long u64;
typedef unsigned int u32;

static int g_pass, g_fail;

static void check(const char *name, u64 got, u64 want)
{
    char buf[128];

    if (got == want) {
        g_pass++;
        return;
    }
    g_fail++;
    snprintf(buf, sizeof(buf), "ee-mmi: %s got=%08x%08x want=%08x%08x\n", name,
             (u32)(got >> 32), (u32)got, (u32)(want >> 32), (u32)want);
    kputs(buf);
}

#define EXP_PCPYLD_LO 0x2222222222222222ULL
#define EXP_PCPYLD_HI 0x1111111111111111ULL
#define EXP_PCPYUD_LO 0xaaaaaaaaaaaaaaaaULL
#define EXP_PCPYUD_HI 0xbbbbbbbbbbbbbbbbULL
#define EXP_PLZCW     26u

extern void test_pcpyld(void *dst, const void *rs, const void *rt);
extern void test_pcpyud(void *dst, const void *rs, const void *rt);
extern void test_qfsrv0(void *dst, const void *rs, const void *rt);
extern void test_plzcw(void *dst, const void *src);

int main(void)
{
    /* pand/por/pnor/pxor - plain 64-bit bitwise ops, uniform across the
       register (no per-lane packing), safe to hand-compute. */
    {
        u64 a = 0xff00ff00ff00ff00ULL, b = 0x0f0f0f0f0f0f0f0fULL, r;
        __asm__ __volatile__("pand %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
        check("pand", r, 0x0f000f000f000f00ULL);
    }
    {
        u64 a = 0xf0f0f0f0f0f0f0f0ULL, b = 0x0f0f0f0f0f0f0f0fULL, r;
        __asm__ __volatile__("por %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
        check("por", r, 0xffffffffffffffffULL);
    }
    {
        u64 a = 0ULL, b = 0ULL, r;
        __asm__ __volatile__("pnor %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
        check("pnor", r, 0xffffffffffffffffULL);
    }
    {
        u64 a = 0xaaaaaaaaaaaaaaaaULL, b = 0x5555555555555555ULL, r;
        __asm__ __volatile__("pxor %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
        check("pxor", r, 0xffffffffffffffffULL);
    }

    /* paddb/psubb - byte-lane add/sub, wraparound (no saturation). */
    {
        u64 a = 0x0101010101010101ULL, b = 0xffffffffffffffffULL, r;
        __asm__ __volatile__("paddb %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
        check("paddb (wrap)", r, 0x0000000000000000ULL);
    }
    {
        u64 a = 0x1010101010101010ULL, b = 0x0101010101010101ULL, r;
        __asm__ __volatile__("psubb %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
        check("psubb", r, 0x0f0f0f0f0f0f0f0fULL);
    }

    /* paddsb - signed-saturating byte add: 127 + 1 clamps at 127. */
    {
        u64 a = 0x7f7f7f7f7f7f7f7fULL, b = 0x0101010101010101ULL, r;
        __asm__ __volatile__("paddsb %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
        check("paddsb (saturate)", r, 0x7f7f7f7f7f7f7f7fULL);
    }

    /* movn/movz - conditional move, both the move and no-move branch. */
    {
        u32 rd = 0xdeadbeef, rs = 0x12345678, rt_nz = 5;
        __asm__ __volatile__("movn %0, %1, %2" : "+r"(rd) : "r"(rs), "r"(rt_nz));
        check("movn (rt!=0, moves)", rd, 0x12345678u);
    }
    {
        u32 rd = 0xdeadbeef, rs = 0x12345678, rt_z = 0;
        __asm__ __volatile__("movn %0, %1, %2" : "+r"(rd) : "r"(rs), "r"(rt_z));
        check("movn (rt==0, no move)", rd, 0xdeadbeefu);
    }
    {
        u32 rd = 0xdeadbeef, rs = 0x12345678, rt_z = 0;
        __asm__ __volatile__("movz %0, %1, %2" : "+r"(rd) : "r"(rs), "r"(rt_z));
        check("movz (rt==0, moves)", rd, 0x12345678u);
    }
    {
        u32 rd = 0xdeadbeef, rs = 0x12345678, rt_nz = 5;
        __asm__ __volatile__("movz %0, %1, %2" : "+r"(rd) : "r"(rs), "r"(rt_nz));
        check("movz (rt!=0, no move)", rd, 0xdeadbeefu);
    }

    /* mult1/mfhi1/mflo1 - pipeline-1 32x32->64 signed multiply. */
    {
        u32 a = 1000, b = 2000, hi, lo;
        __asm__ __volatile__("mult1 %2, %3\n\tmfhi1 %0\n\tmflo1 %1"
                              : "=r"(hi), "=r"(lo) : "r"(a), "r"(b));
        check("mult1/mfhi1", hi, 0u);
        check("mult1/mflo1", lo, 2000000u);
    }

    /* mfsa/mtsa - direct SA register round-trip. */
    {
        u32 v = 5, out;
        __asm__ __volatile__("mtsa %1\n\tmfsa %0" : "=r"(out) : "r"(v));
        check("mtsa/mfsa round-trip", out, 5u);
    }

    /* psllh - halfword-lane shift-left by immediate. */
    {
        u64 a = 0x0001000200030004ULL, r;
        __asm__ __volatile__("psllh %0, %1, 4" : "=r"(r) : "r"(a));
        check("psllh", r, 0x0010002000300040ULL);
    }

    /* pcpyld/pcpyud - real 128-bit half select/copy, via lq/sq helpers. */
    {
        __attribute__((aligned(16))) u64 rs_buf[2] = {0x1111111111111111ULL, 0xaaaaaaaaaaaaaaaaULL};
        __attribute__((aligned(16))) u64 rt_buf[2] = {0x2222222222222222ULL, 0xbbbbbbbbbbbbbbbbULL};
        __attribute__((aligned(16))) u64 dst[2] = {0, 0};

        test_pcpyld(dst, rs_buf, rt_buf);
        check("pcpyld lo", dst[0], EXP_PCPYLD_LO);
        check("pcpyld hi", dst[1], EXP_PCPYLD_HI);

        test_pcpyud(dst, rs_buf, rt_buf);
        check("pcpyud lo", dst[0], EXP_PCPYUD_LO);
        check("pcpyud hi", dst[1], EXP_PCPYUD_HI);
    }

    /* qfsrv shift=0 identity - definitionally equals rt regardless of the
       general funnel-shift formula, so this one's safe to hardcode too. */
    {
        __attribute__((aligned(16))) u64 rs_buf[2] = {0x1111111111111111ULL, 0xaaaaaaaaaaaaaaaaULL};
        __attribute__((aligned(16))) u64 rt_buf[2] = {0x2222222222222222ULL, 0xbbbbbbbbbbbbbbbbULL};
        __attribute__((aligned(16))) u64 dst[2] = {0, 0};

        test_qfsrv0(dst, rs_buf, rt_buf);
        check("qfsrv shift=0 lo", dst[0], rt_buf[0]);
        check("qfsrv shift=0 hi", dst[1], rt_buf[1]);
    }

    /* plzcw - leading zero/one count per 32-bit half. */
    {
        u32 src = 0x00000010u, out;
        test_plzcw(&out, &src);
        check("plzcw", out, EXP_PLZCW);
    }


    kputs(g_fail ? "ee-mmi: FAIL\n" : "ee-mmi: PASS\n");
    printf("[ps2rig] exit=%d\n", g_fail ? 1 : 0);
    SleepThread();
    return 0;
}
