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
 * @ingroup core_libcglue
 */
#ifndef LIBCGLUE_ARPA_INET_H
#define LIBCGLUE_ARPA_INET_H

#include <sys/cdefs.h>
/* Used to be a blanket #include <tcpip.h> - see libcglue's own
 * sys/socket.h for the full story on why that header is real,
 * pre-lwip-migration technical debt that conflicts with ps2ip's real
 * lwip headers. ip4_addr_t (used only by pointer below, a forward
 * reference is enough) already comes from <sys/socket.h>. Found for
 * real building ps2_drivers's tcp_burst_client_ee sample, the first
 * real external consumer to include this header far enough to hit
 * the conflict. */
#include <sys/socket.h>

#ifndef LIBCGLUE_ARPA_INET_NO_ALIASES
#define inet_addr(cp) libcglue_inet_addr(cp)
#define inet_ntoa(addr) libcglue_inet_ntoa((const ip4_addr_t*)&(addr))
#define inet_ntoa_r(addr, buf, buflen) libcglue_inet_ntoa_r((const ip4_addr_t*)&(addr), buf, buflen)
#define inet_aton(cp, addr) libcglue_inet_aton(cp, (ip4_addr_t*)addr)
#endif

__BEGIN_DECLS
extern u32 libcglue_inet_addr(const char *cp);
extern char *libcglue_inet_ntoa(const ip4_addr_t *addr);
extern char *libcglue_inet_ntoa_r(const ip4_addr_t *addr, char *buf, int buflen);
extern int libcglue_inet_aton(const char *cp, ip4_addr_t *addr);
__END_DECLS

#endif
