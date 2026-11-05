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
 * @ingroup core_ps2ip
 */
#ifndef __PS2IPEE_H__
#define	__PS2IPEE_H__

#include <ps2ip.h>
/* t_ip_info's real definition - see ps2sdkapi.h's own comment on it for
 * why it isn't just common/include/tcpip.h. */
#include <ps2sdkapi.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The lwip_* socket functions used to be hand-copied forward
 * declarations here too (each "From include/lwip/sockets.h:") - same
 * stand-in pattern already found and removed from ps2ip.h itself (see
 * that file's own comment). Removed 2026-09-03: ps2ip.h's real <sys/
 * socket.h> chain already declares all of these with the real, current
 * lwip signatures - the stale copies here had drifted (e.g.
 * lwip_accept/lwip_recvfrom/lwip_getsockname/lwip_getpeername's
 * `socklen_t *` parameters vs the real headers' current type), which
 * only actually conflicted once a real consumer (ps2ip_ps2sdk.c, built
 * as part of sdk-core's own full workspace for the first time this
 * session) exercised the mismatch as "incompatible pointer type"
 * errors rather than a silent redeclaration. */

extern int ps2ip_setconfig(const t_ip_info* ip_info);
extern int ps2ip_getconfig(char* netif_name,t_ip_info* ip_info);

/* The "From include/lwip/inet.h:" stand-ins (ipaddr_addr/ip4addr_aton/
 * ip4addr_ntoa/ip4addr_ntoa_r) used to live here too, same category as
 * the lwip_* ones removed above - dropped 2026-09-03: real, reachable
 * now via ps2ip.h's own <sys/socket.h> chain (lwip/ip4_addr.h), and
 * this file's own copy of ipaddr_addr had actually drifted (`u32` vs
 * lwip's real `u32_t` - unsigned int vs unsigned long, a genuine
 * "conflicting types" error once a real consumer needed both). */

#ifdef PS2IP_DNS
/* From include/lwip/netdb.h:  */
extern struct hostent *lwip_gethostbyname(const char *name);
extern int lwip_gethostbyname_r(const char *name, struct hostent *ret, char *buf,
                size_t buflen, struct hostent **result, int *h_errnop);
extern void lwip_freeaddrinfo(struct addrinfo *ai);
extern int lwip_getaddrinfo(const char *nodename,
       const char *servname,
       const struct addrinfo *hints,
       struct addrinfo **res);

/* From include/lwip/dns.h:  */
extern void           dns_setserver(u8 numdns, const ip_addr_t *dnsserver);
extern const ip_addr_t* dns_getserver(u8 numdns);
#endif

#ifdef __cplusplus
}
#endif

#endif	// !defined(__PS2IPEE_H__)
