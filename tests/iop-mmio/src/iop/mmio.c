#include <irx.h>
#include <stdio.h>

IRX_ID("mmio", 1, 1);

stdio_IMPORTS_start
I_printf
stdio_IMPORTS_end

#define I_MASK 0xbf801074u
#define DPCR   0xbf8010f0u
#define DICR   0xbf8010f4u

static unsigned read_explicit(unsigned addr)
{
    unsigned v;
    __asm__ volatile(
        ".set push\n\t.set noreorder\n\t.set noat\n\t"
        "move $1, %1\n\t"
        "lw %0, 0($1)\n\tnop\n\t"
        ".set pop"
        : "=r"(v)
        : "r"(addr)
        : "$1", "memory");
    return v;
}

#define CHECK(reg)                                                     \
    do {                                                               \
        unsigned folded = *(volatile unsigned *)(reg);                 \
        unsigned plain = read_explicit(reg);                           \
        if (folded != plain)                                           \
            bad++;                                                     \
    } while (0)

int _start(int argc, char *argv[])
{
    int bad = 0;
    unsigned mask;

    (void)argc;
    (void)argv;

    CHECK(I_MASK);
    CHECK(DPCR);
    CHECK(DICR);

    mask = *(volatile unsigned *)I_MASK;
    *(volatile unsigned *)I_MASK = mask;
    if (*(volatile unsigned *)I_MASK != mask)
        bad++;

    printf("mmio: %s\n", bad ? "mismatch" : "ok");
    return bad ? 2 : 1;
}
