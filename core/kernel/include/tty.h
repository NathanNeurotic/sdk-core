/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/

/**
 * @file
 * @ingroup core_kernel
 * DECI2 TTY
 */

#ifndef TTY_H_
#define TTY_H_

#ifdef __cplusplus
extern "C" {
#endif

extern int ttyinit;

int sceTtyInit();
int sceTtyWrite(const char *buf, int len);
int sceTtyRead(char *buf, int len);

#ifdef __cplusplus
}
#endif

#endif // TTY_H_
