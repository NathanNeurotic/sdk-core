/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/

/**
 * @file
 * @ingroup core_ps2ip
 * Imports and definitions for ps2ip.
 */

#ifndef __PS2IP_H__
#define __PS2IP_H__

#ifndef LIBCGLUE_SYS_SOCKET_ALIASES
#define LIBCGLUE_SYS_SOCKET_ALIASES 1
#endif

#include <tamtypes.h>
#include <stddef.h>
#include <sys/socket.h>
#include <time.h>

/* err_t (used by ps2ip_input's own real signature below) used to be
 * reached transitively via <sys/socket.h> - fragile for an EXTERNAL
 * consumer (anything outside this same sdk-core workspace, e.g. a
 * plain CMake project globbing packages/core/*\/include
 * alphabetically): whether <sys/socket.h> resolves to lwip's own real
 * one (which reaches lwip/err.h) or libcglue's minimal one (which
 * doesn't) depends on include-path search order, which such a
 * consumer has no control over - found for real fixing ps2_drivers,
 * whose own ps2_eeip_driver.c hit "unknown type name 'err_t'" here.
 * Included directly instead - unambiguous regardless of resolution
 * order, since this package's own include_dirs always exposes
 * lwip_include (see package.yaml). */
#include <lwip/err.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Initializes PS2IP. Specify a dummy address like "169.254.0.1" if DHCP is to be used, before enabling DHCP via ps2ip_setconfig(). */
extern int ps2ipInit(struct ip4_addr *ip_address, struct ip4_addr *subnet_mask, struct ip4_addr *gateway);
extern void ps2ipDeinit(void);

/* ps2ip_getconfig/ps2ip_setconfig: real, previously-missing
 * declarations. Both are implemented in src/ps2ip.c and linked into
 * libps2ip.a, but neither was ever re-declared here, so no consumer's
 * #include <ps2ip.h> could actually see them (found for real:
 * nettest's DHCP bring-up, unbuildable without this).
 *
 * t_ip_info itself is NOT redefined here - libcglue's ps2sdkapi.h
 * already defines the one real, canonical t_ip_info behind a
 * T_IP_INFO_DEFINED guard for exactly this "more than one header wants
 * to supply it" situation (see its own comment). Guarding this copy
 * the same way means whichever header a consumer reaches first wins,
 * instead of a hard redefinition conflict - found for real rebuilding
 * this package after first pasting an unconditional copy here, which
 * broke ps2ip's own ps2ip_ps2sdk.c (reaches ps2sdkapi.h's copy first). */
#ifndef T_IP_INFO_DEFINED
#define T_IP_INFO_DEFINED
typedef struct
{
	char netif_name[4];
	struct in_addr ipaddr;
	struct in_addr netmask;
	struct in_addr gw;
	u32 dhcp_enabled;
	u32 dhcp_status;
	u8 hw_addr[8];
} t_ip_info;
#endif

extern int ps2ip_getconfig(char *pszName, t_ip_info *pInfo);
extern int ps2ip_setconfig(const t_ip_info *pInfo);

/* Real, ps2sdk-specific t_ip_info.dhcp_status values (not lwip's own -
 * lwip 2.2.1's internal DHCP state machine isn't part of its public
 * API at all). Used to live in the legacy common/include/tcpip.h -
 * real, previously-latent gap for any consumer needing to interpret
 * ps2ip_getconfig()'s own t_ip_info output, found for real fixing
 * ps2_drivers's ps2_eeip_driver.c. */
#ifndef DHCP_STATE_OFF
#define DHCP_STATE_OFF         0
#define DHCP_STATE_REQUESTING  1
#define DHCP_STATE_INIT        2
#define DHCP_STATE_REBOOTING   3
#define DHCP_STATE_REBINDING   4
#define DHCP_STATE_RENEWING    5
#define DHCP_STATE_SELECTING   6
#define DHCP_STATE_INFORMING   7
#define DHCP_STATE_CHECKING    8
#define DHCP_STATE_PERMANENT   9  /* not yet implemented */
#define DHCP_STATE_BOUND       10
#define DHCP_STATE_RELEASING   11 /* not yet implemented */
#define DHCP_STATE_BACKING_OFF 12
#endif
/**
 * Use to specify the number of H-sync ticks per milisecond (Default: 16). 
 * Use this function to keep timings accurate, if a mode like 480P (~31KHz H-sync) is used instead of NTSC/PAL (~16KHz H-sync). 
 * This function is obsolete, so it is stubbed for compatibility purposes.
 */
extern void ps2ipSetHsyncTicksPerMSec(unsigned char ticks);

extern err_t ps2ip_input(struct pbuf *p, struct netif *inp);

/* etharp_output, tcpip_input, the netif_ functions, and the pbuf_
 * functions used to be hand-copied forward declarations here (each
 * with a "From include/lwip/X.h:"
 * comment saying so) - a stand-in from whenever this header didn't
 * itself pull in the real lwip headers. Removed 2026-09-03: once
 * <sys/socket.h> (now on this header's own include chain - see
 * ps2ip's package.yaml) transitively includes the real lwip/netif.h,
 * lwip/pbuf.h, lwip/etharp.h and lwip/tcpip.h, these stand-ins
 * conflict outright with the real declarations (some had drifted -
 * e.g. this file's own pbuf_clen was typed "u8", lwip/pbuf.h's real
 * one is "u16_t"). Found building sdk-world/aalib, the first real
 * consumer to include this header far enough to hit them.
 */

#ifdef __cplusplus
}
#endif

#endif /* __PS2IP_H__ */
