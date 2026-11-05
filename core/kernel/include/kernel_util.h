/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/

/**
 * @file
 * @ingroup core_kernel
 * Kernel utility functions
 */

#ifndef __KERNEL_UTIL_H__
#define __KERNEL_UTIL_H__

#include <tamtypes.h>

#ifdef __cplusplus
extern "C" {
#endif

extern s32 WaitSemaEx(s32 semaid, int signal, u64 *timeout);

#ifdef __cplusplus
}
#endif

#endif
