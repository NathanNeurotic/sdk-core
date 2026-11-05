/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# (c) 2003 Marcus R. Brown (mrbrown@0xd6.org)
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/

/**
 * @file
 * EE kernel glue and utility routines.
 */

#include "kernel.h"
#include <mipscopaccess.h>

int DIntr()
{
    int eie, res;

    eie = get_mips_cop_reg(0, COP0_REG_Status);
    eie &= 0x10000;
    res = eie != 0;

    if (!eie)
        return 0;

    __asm__ (".p2align 3");
    do {
        __asm__ __volatile__("di");
        EE_SYNCP();
        eie = get_mips_cop_reg(0, COP0_REG_Status);
        eie &= 0x10000;
    } while (eie);

    return res;
}

int EIntr()
{
    int eie;

    eie = get_mips_cop_reg(0, COP0_REG_Status);
    eie &= 0x10000;
    __asm__ __volatile__("ei");

    return eie != 0;
}

int EnableIntc(int intc)
{
    int eie, res;

    eie = get_mips_cop_reg(0, COP0_REG_Status);
    eie &= 0x10000;

    if (eie)
        DI();

    res = _EnableIntc(intc);
    EE_SYNC();

    if (eie)
        EI();

    return res;
}

int DisableIntc(int intc)
{
    int eie, res;

    eie = get_mips_cop_reg(0, COP0_REG_Status);
    eie &= 0x10000;

    if (eie)
        DI();

    res = _DisableIntc(intc);
    EE_SYNC();

    if (eie)
        EI();

    return res;
}

int EnableDmac(int dmac)
{
    int eie, res;

    eie = get_mips_cop_reg(0, COP0_REG_Status);
    eie &= 0x10000;

    if (eie)
        DI();

    res = _EnableDmac(dmac);
    EE_SYNC();

    if (eie)
        EI();

    return res;
}

int DisableDmac(int dmac)
{
    int eie, res;

    eie = get_mips_cop_reg(0, COP0_REG_Status);
    eie &= 0x10000;

    if (eie)
        DI();

    res = _DisableDmac(dmac);
    EE_SYNC();

    if (eie)
        EI();

    return res;
}

int iEnableIntc(int intc)
{
    int res = _iEnableIntc(intc);
    EE_SYNC();

    return res;
}

int iDisableIntc(int intc)
{
    int res = _iDisableIntc(intc);
    EE_SYNC();

    return res;
}

int iEnableDmac(int dmac)
{
    int res = _iEnableDmac(dmac);
    EE_SYNC();

    return res;
}

int iDisableDmac(int dmac)
{
    int res = _iDisableDmac(dmac);
    EE_SYNC();

    return res;
}

void SyncDCache(void *start, void *end)
{
    int eie;

    eie = get_mips_cop_reg(0, COP0_REG_Status);
    eie &= 0x10000;

    if (eie)
        DI();

    _SyncDCache((void *)((u32)start & 0xffffffc0), (void *)((u32)end & 0xffffffc0));

    if (eie)
        EI();
}

void iSyncDCache(void *start, void *end)
{
    _SyncDCache((void *)((u32)start & 0xffffffc0), (void *)((u32)end & 0xffffffc0));
}

void InvalidDCache(void *start, void *end)
{
    int eie;

    eie = get_mips_cop_reg(0, COP0_REG_Status);
    eie &= 0x10000;

    if (eie)
        DI();

    _InvalidDCache((void *)((u32)start & 0xffffffc0), (void *)((u32)end & 0xffffffc0));

    if (eie)
        EI();
}

void iInvalidDCache(void *start, void *end)
{
    _InvalidDCache((void *)((u32)start & 0xffffffc0), (void *)((u32)end & 0xffffffc0));
}
