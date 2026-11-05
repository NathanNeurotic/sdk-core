#include <kernel.h>
#include <deci2.h>
#include <stddef.h>
#include <unistd.h>
#include <stdint.h>
#include <crashscreen.h>

#include "crash_font.h"

#define COLS 76
#define LINES_MAX 30
#define REPORT_SIZE 3072
#define BT_MAX 24
#define STACK_SCAN_BYTES 16384
#define FB_WIDTH 640
#define UNCACHED(p) ((uintptr_t)(p) | 0x20000000u)

struct crash_frame {
    uint32_t gpr[32];
    uint32_t status;
    uint32_t cause;
    uint32_t epc;
    uint32_t badvaddr;
};

struct crash_frame __crash_frame;
uint8_t __crash_stack[16384] __attribute__((aligned(16)));

extern void __crash_vector(void);
extern char _ftext[];
extern char _etext[];

typedef void (*terminate_fn)(void);
extern terminate_fn std_set_terminate(terminate_fn) __asm__("_ZSt13set_terminatePFvvE") __attribute__((weak));
extern void *__cxa_current_primary_exception(void) __attribute__((weak));
extern const void *__cxa_current_exception_type(void) __attribute__((weak));
extern const char _ZTISt9exception[] __attribute__((weak));
extern int _Unwind_Backtrace(int (*)(void *, void *), void *) __attribute__((weak));
extern uintptr_t _Unwind_GetIP(void *) __attribute__((weak));

static const char *const exception_names[16] = {
    "Interrupt", "TLB modification", "TLB miss on load or fetch", "TLB miss on store",
    "Address error on load or fetch (AdEL)", "Address error on store (AdES)",
    "Bus error on fetch (IBE)", "Bus error on data (DBE)", "Syscall", "Breakpoint (BREAK)",
    "Reserved instruction (RI)", "Coprocessor unusable (CpU)", "Arithmetic overflow (Ov)", "Trap",
    "Unknown", "Unknown"
};

static const char *const gpr_names[32] = {
    "zr", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};

static const int hooked_causes[] = {1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13};

static void *old_handlers[16];
static int installed;
static terminate_fn old_terminate;
static crashscreen_hook_t user_hook;
static int in_crash;
static int chain_previous;

extern void __crash_chain(const struct crash_frame *f);

static char report[REPORT_SIZE];
static int report_len;
static uint32_t bt[BT_MAX];
static int bt_count;

static uint32_t pkt[(6 + FB_WIDTH) * 4] __attribute__((aligned(16)));

static void rc(char c)
{
    if (report_len < REPORT_SIZE - 1)
        report[report_len++] = c;
}

static void rs(const char *s)
{
    while (s && *s)
        rc(*s++);
}

static void rhex(uint32_t v)
{
    for (int i = 28; i >= 0; i -= 4)
        rc("0123456789ABCDEF"[(v >> i) & 15]);
}

static int addr_in_ram(uint32_t a)
{
    return a >= 0x00100000u && a < 0x02000000u;
}

static int in_text(uint32_t a)
{
    return a >= (uint32_t)_ftext && a < (uint32_t)_etext;
}

static void bt_add(uint32_t a)
{
    if (bt_count < BT_MAX)
        bt[bt_count++] = a;
}

static int looks_like_return(uint32_t a)
{
    if (!in_text(a) || (a & 3) || !in_text(a - 8))
        return 0;
    uint32_t insn = *(const uint32_t *)(a - 8);
    return (insn >> 26) == 3 || (insn & 0xFC00003Fu) == 0x00000009u;
}

static void scan_stack(uint32_t sp)
{
    if (!addr_in_ram(sp))
        return;
    uint32_t end = sp + STACK_SCAN_BYTES;
    if (end > 0x02000000u)
        end = 0x02000000u;
    for (uint32_t p = sp & ~3u; p < end && bt_count < BT_MAX; p += 4) {
        uint32_t v = *(const uint32_t *)p;
        if (looks_like_return(v))
            bt_add(v);
    }
}

static int unwind_cb(void *ctx, void *arg)
{
    (void)arg;
    bt_add((uint32_t)_Unwind_GetIP(ctx));
    return 0;
}

static void emit_report(const char *reason, const char *detail, const struct crash_frame *f, uint32_t sp)
{
    report_len = 0;
    rs("PROGRAM CRASHED\n");
    rs("Reason: ");
    rs(reason);
    rc('\n');
    if (detail && *detail) {
        rs(detail);
        rc('\n');
    }
    rc('\n');
    if (f) {
        rs("PC=");
        rhex(f->epc);
        rs("  RA=");
        rhex(f->gpr[31]);
        rs("  SP=");
        rhex(f->gpr[29]);
        rs("\nBadVAddr=");
        rhex(f->badvaddr);
        rs("  Cause=");
        rhex(f->cause);
        rs("  Status=");
        rhex(f->status);
        rs("\n\n");
        for (int i = 1; i < 32; i++) {
            rs(gpr_names[i]);
            rc('=');
            rhex(f->gpr[i]);
            rc((i % 6) == 0 || i == 31 ? '\n' : ' ');
        }
    } else {
        rs("SP=");
        rhex(sp);
        rc('\n');
    }
    rs("\nBacktrace (return addresses):\n");
    for (int i = 0; i < bt_count; i++) {
        rhex(bt[i]);
        rc((i % 8) == 7 || i == bt_count - 1 ? '\n' : ' ');
    }
    if (bt_count == 0)
        rs("none found\n");
    rs("Resolve with: ps2symbolize <elf> <address...>\n");
    report[report_len] = 0;
}

static void send_console(void)
{
    char line[COLS + 4];
    int n = 0;
    kputs("\n");
    for (int i = 0; i <= report_len; i++) {
        char c = report[i];
        if (c == '\n' || c == 0 || n >= COLS) {
            line[n++] = '\n';
            line[n] = 0;
            kputs(line);
            n = 0;
            if (c != '\n' && c != 0)
                line[n++] = c;
        } else {
            line[n++] = c;
        }
    }
}

static void send_stdout(void)
{
    uint32_t st;
    __asm__ volatile("mfc0 %0, $12" : "=r"(st));
    __asm__ volatile("mtc0 %0, $12\n\tsync.p" ::"r"(st | 1u | (1u << 16)));
    write(1, report, report_len);
    write(1, "\n", 1);
    __asm__ volatile("mtc0 %0, $12\n\tsync.p" ::"r"(st));
}

static void gs_setup(int pal)
{
    volatile uint64_t *gs = (volatile uint64_t *)0x12000000;
    volatile uint32_t *dma = (volatile uint32_t *)0x1000A000;
    int height = pal ? 256 : 224;

    dma[0] = 0;
    *(volatile uint32_t *)0x1000E000 = 1;
    *(volatile uint32_t *)0x10003000 = 1;
    *(volatile uint32_t *)0x10003000 = 0;
    *(volatile uint64_t *)0x12001000 = 0x200;
    __asm__ volatile("sync.p");
    *(volatile uint64_t *)0x12001000 = 0;
    GsPutIMR(0xFF00);
    SetGsCrt(0, pal ? 3 : 2, 0);

    gs[0x00 / 8] = 0x8046;
    uint64_t dispfb = (uint64_t)(FB_WIDTH / 64) << 9 | (uint64_t)2 << 15;
    uint64_t display = (uint64_t)(pal ? 656 : 636) | (uint64_t)(pal ? 36 : 25) << 12 | (uint64_t)3 << 23 |
                       (uint64_t)2559 << 32 | (uint64_t)(height - 1) << 44;
    gs[0x70 / 8] = dispfb;
    gs[0x80 / 8] = display;
    gs[0x90 / 8] = dispfb;
    gs[0xA0 / 8] = display;
    gs[0xE0 / 8] = 0;
}

static void gs_send_row(int y, const char *text, int len, int color_red)
{
    volatile uint32_t *q = (volatile uint32_t *)UNCACHED(pkt);
    volatile uint16_t *px = (volatile uint16_t *)UNCACHED(&pkt[6 * 4]);
    volatile uint32_t *dma = (volatile uint32_t *)0x1000A000;

    q[0] = 4 | (1 << 15);
    q[1] = 0x10000000;
    q[2] = 0xE;
    q[3] = 0;
    q[4] = 0;
    q[5] = (10u << 16) | (2u << 24);
    q[6] = 0x50;
    q[7] = 0;
    q[8] = 0;
    q[9] = (uint32_t)y << 16;
    q[10] = 0x51;
    q[11] = 0;
    q[12] = FB_WIDTH;
    q[13] = 8;
    q[14] = 0x52;
    q[15] = 0;
    q[16] = 0;
    q[17] = 0;
    q[18] = 0x53;
    q[19] = 0;
    q[20] = FB_WIDTH | (1 << 15);
    q[21] = 2u << 26;
    q[22] = 0;
    q[23] = 0;

    for (int i = 0; i < FB_WIDTH * 8; i++)
        px[i] = 0x8000;
    uint16_t color = color_red ? 0x801F : 0xFFFF;
    for (int k = 0; k < len; k++) {
        unsigned char c = (unsigned char)text[k];
        if (c < 32 || c > 126)
            c = '?';
        const unsigned char *g = crash_font[c - 32];
        int x0 = 16 + k * 8;
        for (int row = 0; row < 8; row++)
            for (int b = 0; b < 8; b++)
                if ((g[row] >> b) & 1)
                    px[row * FB_WIDTH + x0 + b] = color;
    }

    __asm__ volatile("sync.l" ::: "memory");
    dma[0x20 / 4] = 6 + FB_WIDTH;
    dma[0x10 / 4] = (uint32_t)((uintptr_t)pkt & 0x0FFFFFFF);
    dma[0] = 0x101;
    for (int t = 0; t < 20000000 && (dma[0] & 0x100); t++)
        ;
}

static void draw_report(int pal)
{
    int height = pal ? 256 : 224;
    int pixel_rows = height / 8;
    int text_rows = pixel_rows - 2;
    char line[COLS + 1];
    int pos = 0;

    uint32_t d_ctrl = *(volatile uint32_t *)0x1000E000;

    gs_setup(pal);
    for (int r = 0; r < pixel_rows; r++) {
        int len = 0;
        int red = (r == 1);
        if (r >= 1 && r <= text_rows) {
            while (pos < report_len && report[pos] != '\n' && len < COLS)
                line[len++] = report[pos++];
            if (pos < report_len && report[pos] == '\n')
                pos++;
        }
        gs_send_row(r * 8, line, len, red);
    }
    *(volatile uint32_t *)0x1000E010 = 1u << 2;
    *(volatile uint32_t *)0x1000E000 = d_ctrl;
}

static int console_is_pal(void)
{
    return *(volatile char *)0xBFC7FF52 == 'E';
}

static void quiesce(void)
{
    uint32_t st;
    __asm__ volatile("mfc0 %0, $12" : "=r"(st));
    st &= ~((1u << 0) | (1u << 1) | (3u << 3) | (1u << 16));
    __asm__ volatile("mtc0 %0, $12\n\tsync.p" ::"r"(st));
}

void __crash_sleep(void)
{
    for (;;)
        SleepThread();
}

extern void __crash_leave(void (*fn)(void));

static void idle(void)
{
    __crash_leave(__crash_sleep);
}

static void finish(const char *reason, const char *detail, const struct crash_frame *f, uint32_t sp)
{
    if (in_crash)
        for (;;)
            ;
    in_crash = 1;
    quiesce();
    emit_report(reason, detail, f, sp);
    draw_report(console_is_pal());
    send_console();
    send_stdout();
    if (user_hook)
        user_hook(report);
    if (chain_previous && f) {
        int c = (f->cause >> 2) & 15;
        if (old_handlers[c]) {
            if (c <= 3)
                SetVTLBRefillHandler(c, old_handlers[c]);
            else
                SetVCommonHandler(c, old_handlers[c]);
            FlushCache(0);
            FlushCache(2);
            __crash_chain(f);
        }
    }
    idle();
}

void __crash_cpu_exception(struct crash_frame *f)
{
    int code = (f->cause >> 2) & 31;
    bt_count = 0;
    if (looks_like_return(f->gpr[31]))
        bt_add(f->gpr[31]);
    scan_stack(f->gpr[29]);
    finish(exception_names[code & 15], NULL, f, f->gpr[29]);
}

static const char *exception_what(char *name_out, size_t name_size)
{
    name_out[0] = 0;
    if (!__cxa_current_primary_exception || !__cxa_current_exception_type)
        return NULL;
    const void *type = __cxa_current_exception_type();
    void *obj = __cxa_current_primary_exception();
    if (!type)
        return NULL;
    const char *tn = *(const char *const *)((const char *)type + 4);
    if (addr_in_ram((uint32_t)tn)) {
        size_t i = 0;
        for (; tn[i] && i + 1 < name_size; i++)
            name_out[i] = tn[i];
        name_out[i] = 0;
    }
    if (!obj || !_ZTISt9exception || !addr_in_ram((uint32_t)obj))
        return NULL;
    const uint32_t *t = (const uint32_t *)type;
    for (int depth = 0; t != (const uint32_t *)_ZTISt9exception; depth++) {
        if (depth == 8 || !addr_in_ram(t[2]) || !addr_in_ram(((const uint32_t *)t[2])[1]))
            return NULL;
        t = (const uint32_t *)t[2];
    }
    void **svptr = *(void ***)obj;
    if (!addr_in_ram((uint32_t)svptr))
        return NULL;
    const char *(*what)(void *) = (const char *(*)(void *))svptr[2];
    return what(obj);
}

static void terminate_handler(void)
{
    char name[96];
    char detail[200];
    int n = 0;
    uint32_t sp;
    __asm__ volatile("move %0, $29" : "=r"(sp));

    const char *what = exception_what(name, sizeof(name));
    const char *prefix = name[0] ? "Type: " : "";
    for (const char *s = prefix; *s && n < (int)sizeof(detail) - 1; s++)
        detail[n++] = *s;
    for (const char *s = name; *s && n < (int)sizeof(detail) - 1; s++)
        detail[n++] = *s;
    if (what) {
        const char *sep = name[0] ? "  what(): " : "what(): ";
        for (const char *s = sep; *s && n < (int)sizeof(detail) - 1; s++)
            detail[n++] = *s;
        for (int i = 0; what[i] && i < 120 && n < (int)sizeof(detail) - 1; i++)
            detail[n++] = what[i];
    }
    detail[n] = 0;

    bt_count = 0;
    if (_Unwind_Backtrace && _Unwind_GetIP) {
        _Unwind_Backtrace(unwind_cb, NULL);
        if (bt_count > 0) {
            for (int i = 1; i < bt_count; i++)
                bt[i - 1] = bt[i];
            bt_count--;
        }
    }
    if (bt_count == 0)
        scan_stack(sp);
    finish("std::terminate (uncaught exception)", detail, NULL, sp);
}

void crashscreen_abort(const char *reason)
{
    uint32_t sp;
    __asm__ volatile("move %0, $29" : "=r"(sp));
    bt_count = 0;
    scan_stack(sp);
    finish(reason ? reason : "crashscreen_abort", NULL, NULL, sp);
    for (;;)
        ;
}

void crashscreen_set_chain(int enable)
{
    chain_previous = enable;
}

void crashscreen_set_hook(crashscreen_hook_t hook)
{
    user_hook = hook;
}

void crashscreen_install(void)
{
    if (installed)
        return;
    for (unsigned i = 0; i < sizeof(hooked_causes) / sizeof(hooked_causes[0]); i++) {
        int c = hooked_causes[i];
        old_handlers[c] = GetExceptionHandler(c);
        if (c <= 3)
            SetVTLBRefillHandler(c, __crash_vector);
        else
            SetVCommonHandler(c, __crash_vector);
    }
    if (std_set_terminate)
        old_terminate = std_set_terminate(terminate_handler);
    FlushCache(0);
    FlushCache(2);
    installed = 1;
}

void crashscreen_remove(void)
{
    if (!installed)
        return;
    for (unsigned i = 0; i < sizeof(hooked_causes) / sizeof(hooked_causes[0]); i++) {
        int c = hooked_causes[i];
        if (!old_handlers[c])
            continue;
        if (c <= 3)
            SetVTLBRefillHandler(c, old_handlers[c]);
        else
            SetVCommonHandler(c, old_handlers[c]);
        old_handlers[c] = NULL;
    }
    if (std_set_terminate)
        std_set_terminate(old_terminate);
    FlushCache(0);
    FlushCache(2);
    installed = 0;
}
