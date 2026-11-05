#include <stdio.h>
#include <string.h>
#include <math.h>
#include <kernel.h>
#include <tamtypes.h>

extern int kputs(char *s);

typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

static inline u32 cycles(void)
{
    u32 v;
    __asm__ __volatile__("mfc0 %0, $9" : "=r"(v) : : "memory");
    return v;
}

#define BUF_BYTES (64 * 1024)

static u8 src_buf[BUF_BYTES] __attribute__((aligned(64)));
static u8 dst_buf[BUF_BYTES] __attribute__((aligned(64)));

static u32 checksum(const void *p, u32 bytes)
{
    const u8 *b = p;
    u32 h = 2166136261u;

    for (u32 i = 0; i < bytes; i++)
        h = (h ^ b[i]) * 16777619u;
    return h;
}

static u32 rng_state = 12345;

static u32 rng(void)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state >> 8;
}

/* 128-bit loads and stores, the way an MMI memcpy is written. */
__attribute__((noinline)) static void bench_memcpy_mmi(void *dst, const void *src, u32 bytes)
{
    u64 *d = dst;
    const u64 *s = src;

    for (u32 i = 0; i < bytes / 64; i++) {
        __asm__ __volatile__(
            "lq $8, 0(%1)\n\tlq $9, 16(%1)\n\tlq $10, 32(%1)\n\tlq $11, 48(%1)\n\t"
            "sq $8, 0(%0)\n\tsq $9, 16(%0)\n\tsq $10, 32(%0)\n\tsq $11, 48(%0)"
            :
            : "r"(d), "r"(s)
            : "$8", "$9", "$10", "$11", "memory");
        d += 8;
        s += 8;
    }
}

__attribute__((noinline)) static void bench_memset_mmi(void *dst, u32 bytes)
{
    u64 *d = dst;

    for (u32 i = 0; i < bytes / 64; i++) {
        __asm__ __volatile__(
            "pcpyld $8, %1, %1\n\tsq $8, 0(%0)\n\tsq $8, 16(%0)\n\tsq $8, 32(%0)\n\tsq $8, 48(%0)"
            :
            : "r"(d), "r"(0x0101010101010101ull)
            : "$8", "memory");
        d += 8;
    }
}

__attribute__((noinline)) static void bench_memcpy_libc(void *dst, const void *src, u32 bytes)
{
    memcpy(dst, src, bytes);
}

__attribute__((noinline)) static void bench_memcpy_loop(u32 *dst, const u32 *src, u32 words)
{
    for (u32 i = 0; i < words; i++)
        dst[i] = src[i];
}

/* GIF-style packet building: a tag, then per vertex an RGBAQ, an ST-like pair and an XYZ2 quadword. */
__attribute__((noinline)) static u32 bench_packet_build(u64 *out, u32 vertices)
{
    u64 *p = out;

    *p++ = 0x1000000000008001ull | ((u64)vertices << 0);
    *p++ = 0x5551ull;
    for (u32 i = 0; i < vertices; i++) {
        u32 x = (i * 37) & 0x1ff, y = (i * 91) & 0x1ff, z = i * 3;
        u32 r = (i * 5) & 0xff, g = (i * 11) & 0xff, b = (i * 17) & 0xff;

        *p++ = (u64)r | ((u64)g << 8) | ((u64)b << 16) | (0x80ull << 24);
        *p++ = 0x3f800000ull;
        *p++ = ((u64)(x << 4)) | ((u64)(y << 4) << 16) | ((u64)z << 32);
        *p++ = 0;
    }
    return (u32)(p - out);
}

/* A small LZ77 decoder: literals and (length, distance) matches. */
__attribute__((noinline)) static u32 bench_decompress(u8 *out, const u8 *in, u32 in_len)
{
    u8 *o = out;
    const u8 *end = in + in_len;

    while (in < end) {
        u32 tag = *in++;

        if (tag < 0x80) {
            u32 n = tag + 1;

            while (n--)
                *o++ = *in++;
        } else {
            u32 len = (tag & 0x7f) + 3;
            u32 dist = ((u32)in[0] << 8 | in[1]) + 1;
            const u8 *m = o - dist;

            in += 2;
            while (len--)
                *o++ = *m++;
        }
    }
    return (u32)(o - out);
}

/* Builds a compressed stream of repeated phrases and literals. */
static u32 make_compressed(u8 *out)
{
    u8 *o = out;
    u32 produced = 0;

    for (u32 i = 0; i < 2000; i++) {
        u32 n = 4 + (rng() & 7);

        *o++ = (u8)(n - 1);
        for (u32 k = 0; k < n; k++)
            *o++ = (u8)rng();
        produced += n;
        if (produced > 64) {
            u32 len = 3 + (rng() & 15), dist = rng() % 48;

            *o++ = (u8)(0x80 | (len - 3));
            *o++ = (u8)(dist >> 8);
            *o++ = (u8)dist;
            produced += len;
        }
    }
    return (u32)(o - out);
}

#include "inflate_data.h"

/* A table-driven raw deflate (RFC 1951) decoder: 9-bit lookup, canonical walk for longer codes. Like zlib's
   inflate_table, the table builder is a function of its own; its size counts towards inflate. */
#define INF_FAST_BITS 9

struct inf_huff {
    u16 count[16];
    u16 symbol[320];
    u16 fast[1 << INF_FAST_BITS];
};

struct inf_state {
    const u8 *in, *in_end;
    u32 bitbuf, bitcnt;
};

static const u16 inf_len_base[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
                                     35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
static const u8 inf_len_extra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
                                     3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
static const u16 inf_dist_base[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
                                      257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
static const u8 inf_dist_extra[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
                                      7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
static const u8 inf_clen_order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

static inline __attribute__((always_inline)) void inf_refill(struct inf_state *s)
{
    while (s->bitcnt <= 24) {
        u32 b = s->in < s->in_end ? *s->in++ : 0;

        s->bitbuf |= b << s->bitcnt;
        s->bitcnt += 8;
    }
}

static inline __attribute__((always_inline)) u32 inf_bits(struct inf_state *s, u32 n)
{
    u32 v;

    inf_refill(s);
    v = s->bitbuf & ((1u << n) - 1);
    s->bitbuf >>= n;
    s->bitcnt -= n;
    return v;
}

__attribute__((noinline)) static void bench_inflate_table(struct inf_huff *h, const u8 *lens, u32 n)
{
    u16 offs[16], next[16];
    u32 code = 0;

    memset(h->count, 0, sizeof(h->count));
    memset(h->fast, 0, sizeof(h->fast));
    for (u32 i = 0; i < n; i++)
        h->count[lens[i]]++;
    h->count[0] = 0;
    offs[1] = 0;
    for (u32 len = 1; len < 15; len++)
        offs[len + 1] = offs[len] + h->count[len];
    for (u32 len = 1; len < 16; len++) {
        code = (code + h->count[len - 1]) << 1;
        next[len] = (u16)code;
    }
    for (u32 i = 0; i < n; i++) {
        u32 len = lens[i];

        if (len == 0)
            continue;
        h->symbol[offs[len]++] = (u16)i;
        if (len <= INF_FAST_BITS) {
            u32 c = next[len], rev = 0;

            for (u32 b = 0; b < len; b++)
                rev |= ((c >> b) & 1) << (len - 1 - b);
            for (u32 k = rev; k < (1u << INF_FAST_BITS); k += 1u << len)
                h->fast[k] = (u16)((len << 9) | i);
        }
        next[len]++;
    }
}

static inline __attribute__((always_inline)) u32 inf_decode(struct inf_state *s, const struct inf_huff *h)
{
    u32 e, code = 0, first = 0, index = 0;

    inf_refill(s);
    e = h->fast[s->bitbuf & ((1u << INF_FAST_BITS) - 1)];
    if (e) {
        s->bitbuf >>= e >> 9;
        s->bitcnt -= e >> 9;
        return e & 0x1ff;
    }
    for (u32 len = 1; len < 16; len++) {
        u32 count = h->count[len];

        code |= s->bitbuf & 1;
        s->bitbuf >>= 1;
        s->bitcnt--;
        if (code - first < count)
            return h->symbol[index + code - first];
        index += count;
        first = (first + count) << 1;
        code <<= 1;
    }
    return 0xffff;
}

static struct inf_huff inf_lit, inf_dist;

__attribute__((noinline)) static u32 bench_inflate(u8 *out, const u8 *in, u32 in_len)
{
    struct inf_state s = {in, in + in_len, 0, 0};
    u8 *o = out;
    u32 last;

    do {
        u32 type;

        last = inf_bits(&s, 1);
        type = inf_bits(&s, 2);
        if (type == 0) {
            u32 n;

            s.bitbuf >>= s.bitcnt & 7;
            s.bitcnt -= s.bitcnt & 7;
            n = inf_bits(&s, 16);
            inf_bits(&s, 16);
            while (n--)
                *o++ = (u8)inf_bits(&s, 8);
            continue;
        }
        if (type == 1) {
            u8 lens[320];
            u32 i = 0;

            for (; i < 144; i++)
                lens[i] = 8;
            for (; i < 256; i++)
                lens[i] = 9;
            for (; i < 280; i++)
                lens[i] = 7;
            for (; i < 288; i++)
                lens[i] = 8;
            bench_inflate_table(&inf_lit, lens, 288);
            for (i = 0; i < 30; i++)
                lens[i] = 5;
            bench_inflate_table(&inf_dist, lens, 30);
        } else {
            u8 lens[320];
            u32 hlit = inf_bits(&s, 5) + 257, hdist = inf_bits(&s, 5) + 1, hclen = inf_bits(&s, 4) + 4;
            u32 i;

            memset(lens, 0, 19);
            for (i = 0; i < hclen; i++)
                lens[inf_clen_order[i]] = (u8)inf_bits(&s, 3);
            bench_inflate_table(&inf_lit, lens, 19);
            for (i = 0; i < hlit + hdist;) {
                u32 sym = inf_decode(&s, &inf_lit), rep, val = 0;

                if (sym < 16) {
                    lens[i++] = (u8)sym;
                    continue;
                }
                if (sym == 16) {
                    val = lens[i - 1];
                    rep = 3 + inf_bits(&s, 2);
                } else if (sym == 17) {
                    rep = 3 + inf_bits(&s, 3);
                } else {
                    rep = 11 + inf_bits(&s, 7);
                }
                while (rep--)
                    lens[i++] = (u8)val;
            }
            bench_inflate_table(&inf_lit, lens, hlit);
            bench_inflate_table(&inf_dist, lens + hlit, hdist);
        }
        for (;;) {
            u32 sym = inf_decode(&s, &inf_lit);

            if (sym < 256) {
                *o++ = (u8)sym;
            } else if (sym == 256) {
                break;
            } else {
                u32 len, dist;
                const u8 *m;

                sym -= 257;
                len = inf_len_base[sym] + inf_bits(&s, inf_len_extra[sym]);
                sym = inf_decode(&s, &inf_dist);
                dist = inf_dist_base[sym] + inf_bits(&s, inf_dist_extra[sym]);
                m = o - dist;
                while (len--)
                    *o++ = *m++;
            }
        }
    } while (!last);
    return (u32)(o - out);
}

#define PARTICLES 1024

static float px[PARTICLES], py[PARTICLES], pz[PARTICLES];
static float vx[PARTICLES], vy[PARTICLES], vz[PARTICLES];

static void init_particles(u32 seed)
{
    rng_state = seed;
    for (u32 i = 0; i < PARTICLES; i++) {
        px[i] = (float)(rng() & 15) - 8.0f;
        py[i] = (float)(rng() & 15);
        pz[i] = (float)(rng() & 15) - 8.0f;
        vx[i] = (float)(rng() & 7) - 4.0f;
        vy[i] = 0.0f;
        vz[i] = (float)(rng() & 7) - 4.0f;
    }
}

__attribute__((noinline)) static float bench_physics(u32 frames)
{
    float energy = 0.0f;
    const float dt = 1.0f / 60.0f;

    for (u32 f = 0; f < frames; f++) {
        for (u32 i = 0; i < PARTICLES; i++) {
            vy[i] -= 9.8f * dt;
            px[i] += vx[i] * dt;
            py[i] += vy[i] * dt;
            pz[i] += vz[i] * dt;
            if (py[i] < 0.0f) {
                py[i] = -py[i];
                vy[i] = -vy[i] * 0.8f;
            }
            float d2 = px[i] * px[i] + pz[i] * pz[i];
            if (d2 > 100.0f) {
                float inv = 1.0f / sqrtf(d2);
                vx[i] = -px[i] * inv;
                vz[i] = -pz[i] * inv;
            }
        }
    }
    for (u32 i = 0; i < PARTICLES; i++)
        energy += vx[i] * vx[i] + vy[i] * vy[i] + vz[i] * vz[i];
    return energy;
}

#define FB_W 128
#define FB_H 128

static u32 fb[FB_W * FB_H];

__attribute__((noinline)) static void bench_raster(u32 triangles)
{
    for (u32 t = 0; t < triangles; t++) {
        int x0 = (int)(rng() % FB_W), y0 = (int)(rng() % FB_H);
        int x1 = (int)(rng() % FB_W), y1 = (int)(rng() % FB_H);
        int x2 = (int)(rng() % FB_W), y2 = (int)(rng() % FB_H);
        u32 color = rng();
        int minx = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
        int maxx = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
        int miny = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2);
        int maxy = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);
        int area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);

        if (area == 0)
            continue;
        for (int y = miny; y <= maxy; y++) {
            for (int x = minx; x <= maxx; x++) {
                int w0 = (x1 - x0) * (y - y0) - (x - x0) * (y1 - y0);
                int w1 = (x2 - x1) * (y - y1) - (x - x1) * (y2 - y1);
                int w2 = (x0 - x2) * (y - y2) - (x - x2) * (y0 - y2);

                if (area > 0 ? (w0 >= 0 && w1 >= 0 && w2 >= 0) : (w0 <= 0 && w1 <= 0 && w2 <= 0))
                    fb[y * FB_W + x] = color;
            }
        }
    }
}

static char line[96];

static void report(const char *name, u32 cyc, u32 sum)
{
    snprintf(line, sizeof(line), "bench %s %u %08x\n", name, cyc, sum);
    kputs(line);
    printf("%s", line);
}

#define TIMED_SETUP(name, setup, call, sum)  \
    do {                                     \
        u32 best = 0xffffffffu;              \
        for (int rep = 0; rep < 3; rep++) {  \
            setup;                           \
            DIntr();                         \
            u32 t0 = cycles();               \
            call;                            \
            u32 t1 = cycles() - t0;          \
            EIntr();                         \
            if (t1 < best)                   \
                best = t1;                   \
        }                                    \
        report(name, best, sum);             \
    } while (0)

#define TIMED(name, call, sum) TIMED_SETUP(name, (void)0, call, sum)

int main(void)
{
    static u8 comp[16384], plain[65536];
    static u64 packets[8192];
    u32 comp_len;

    for (u32 i = 0; i < BUF_BYTES; i++)
        src_buf[i] = (u8)(i * 7 + (i >> 8));

    TIMED("memcpy_mmi", bench_memcpy_mmi(dst_buf, src_buf, BUF_BYTES), checksum(dst_buf, BUF_BYTES));
    memset(dst_buf, 0, sizeof(dst_buf));
    TIMED("memcpy_libc", bench_memcpy_libc(dst_buf, src_buf, BUF_BYTES), checksum(dst_buf, BUF_BYTES));
    memset(dst_buf, 0, sizeof(dst_buf));
    TIMED("memcpy_loop", bench_memcpy_loop((u32 *)dst_buf, (const u32 *)src_buf, BUF_BYTES / 4), checksum(dst_buf, BUF_BYTES));
    TIMED("memset_mmi", bench_memset_mmi(dst_buf, BUF_BYTES), checksum(dst_buf, BUF_BYTES));
    TIMED("packet_build", bench_packet_build(packets, 2000), checksum(packets, 2000 * 32));

    comp_len = make_compressed(comp);
    TIMED("decompress", bench_decompress(plain, comp, comp_len), checksum(plain, 4096));
    TIMED("inflate", bench_inflate(plain, inflate_in, sizeof(inflate_in)), checksum(plain, INFLATE_OUT_BYTES));

    {
        u32 seed = rng_state;
        float e = 0.0f;
        u32 bits;

        TIMED_SETUP("physics", init_particles(seed), e = bench_physics(30), (memcpy(&bits, &e, sizeof(bits)), bits));
    }
    {
        u32 seed = rng_state;

        TIMED_SETUP("raster", rng_state = seed, bench_raster(200), checksum(fb, sizeof(fb)));
    }

    kputs("ee-bench: PASS\n");
    printf("[ps2rig] exit=0\n");
    SleepThread();
    return 0;
}
