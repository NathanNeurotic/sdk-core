#include <irx.h>
#include <stdio.h>

IRX_ID("probe", 1, 1);

stdio_IMPORTS_start
I_printf
stdio_IMPORTS_end

static volatile struct { int a, b, c, d; } g = {1, 2, 3, 4};

int _start(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    g.a = g.b + 1;
    g.c = g.d + 1;
    g.b = g.a + g.c;
    printf("probe: loaded\n");
    return 100 + g.b + g.c;
}
