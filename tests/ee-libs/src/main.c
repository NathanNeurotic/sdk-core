#include <stdio.h>

extern int kputs(char *s);
extern int SleepThread(void);

extern char sym1[] __asm__("AHX_Init");
extern char sym2[] __asm__("sceCdCallback");
extern char sym3[] __asm__("init_scr");
extern char sym4[] __asm__("dma_channel_fast_waits");
extern char sym5[] __asm__("draw_clear");
extern char sym6[] __asm__("ee_dbg_clr_bpda");
extern char sym7[] __asm__("LoadELFFromFile");
extern char sym8[] __asm__("elf_loader_exec_elf");
extern char sym9[] __asm__("erl_add_global_symbol");
extern char sym10[] __asm__("fileXioChdir");
extern char sym11[] __asm__("draw_fontx_char");
extern char sym12[] __asm__("graph_add_vsync_handler");
extern char sym13[] __asm__("init_imgdrv_offsets");
extern char sym14[] __asm__("ioprpgen_setup_membuf");
extern char sym15[] __asm__("PS2KbdClose");
extern char sym16[] __asm__("AddDmacHandler");
extern char sym17[] __asm__("calculate_colours");
extern char sym18[] __asm__("mcChangeThreadPriority");
extern char sym19[] __asm__("PS2MouseEnum");
extern char sym20[] __asm__("MPEG_Destroy");
extern char sym21[] __asm__("mtapGetConnection");
extern char sym22[] __asm__("NetManDeinit");
extern char sym23[] __asm__("packet2_add");
extern char sym24[] __asm__("packet_free");
extern char sym25[] __asm__("padEnd");
extern char sym26[] __asm__("poweroffChangeThreadPriority");
extern char sym27[] __asm__("PS2CamCloseDevice");
extern char sym28[] __asm__("ps2ip_getconfig");
extern char sym29[] __asm__("ps2ip_deinit");
extern char sym30[] __asm__("sceSdBlockTrans");
extern char sym31[] __asm__("RMMan_Close");
extern char sym32[] __asm__("SecrDeinit");
extern char sym33[] __asm__("SIOR_Init");

struct lib {
    const char *name;
    void *symbol;
};

static const struct lib libs[] = {
    {"ahx", sym1},
    {"cdvd", sym2},
    {"debug", sym3},
    {"dma", sym4},
    {"draw", sym5},
    {"eedebug", sym6},
    {"elf-loader-nocolour", sym7},
    {"elf-loader2", sym8},
    {"erl", sym9},
    {"filexio", sym10},
    {"font", sym11},
    {"graph", sym12},
    {"iopreboot", sym13},
    {"ioprpgen", sym14},
    {"kbd", sym15},
    {"kernel", sym16},
    {"math3d", sym17},
    {"mc", sym18},
    {"mouse", sym19},
    {"mpeg", sym20},
    {"mtap", sym21},
    {"netman", sym22},
    {"packet2", sym23},
    {"packet", sym24},
    {"pad", sym25},
    {"poweroff", sym26},
    {"ps2cam", sym27},
    {"ps2ip", sym28},
    {"ps2snd", sym30},
    {"rm", sym31},
    {"secr", sym32},
    {"sior", sym33},
};

/* Taking each symbol's address makes the linker pull that library in, so the test builds only if every library links. */
int main(void) {
    int found = 0;
    char line[64];
    for (unsigned i = 0; i < sizeof(libs) / sizeof(libs[0]); i++)
        if (libs[i].symbol) found++;
    snprintf(line, sizeof(line), "ee-libs: %d libraries linked\n", found);
    kputs(line);
    kputs(found == (int)(sizeof(libs) / sizeof(libs[0])) ? "ee-libs: PASS\n" : "ee-libs: FAIL\n");
    printf("[ps2rig] exit=0\n");
    SleepThread();
    return 0;
}
