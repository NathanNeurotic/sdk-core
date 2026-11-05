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
#ifndef LIBCGLUE_NETINET_IN_H
#define LIBCGLUE_NETINET_IN_H

#include <sys/socket.h>

/* Used to be a blanket #include <tcpip.h> - see libcglue's own
 * sys/socket.h for the full story on why that header is real,
 * pre-lwip-migration technical debt that conflicts with ps2ip's real
 * lwip headers. Real, minimal, guarded definitions instead - struct
 * in_addr/sa_family_t/socklen_t already come from <sys/socket.h>
 * above; in_port_t, struct sockaddr_in, and the INADDR_/IPPROTO_
 * prefixed constants are real BSD sockets values, byte-identical to
 * tcpip.h's own values (verified
 * for real), not connected to the lwip ABI conflict at all - safe to
 * define directly. Found for real building ps2_drivers's
 * select_test_ee sample, the first real external consumer to include
 * this header far enough to hit the conflict. */
#ifndef IN_PORT_T_DEFINED
#define IN_PORT_T_DEFINED
typedef u16 in_port_t;
#endif

#ifndef _SOCKADDR_IN_STRUCT_DEFINED
#define _SOCKADDR_IN_STRUCT_DEFINED
struct sockaddr_in
{
	u8 sin_len;
	sa_family_t sin_family;
	in_port_t sin_port;
	struct in_addr sin_addr;
#define SIN_ZERO_LEN 8
	char sin_zero[SIN_ZERO_LEN];
};
#endif

/* Real values, matching lwip/ip4_addr.h's own IPADDR_* exactly. */
#ifndef INADDR_NONE
#define INADDR_NONE      ((u32)0xffffffffUL)
#endif
#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK  ((u32)0x7f000001UL)
#endif
#ifndef INADDR_ANY
#define INADDR_ANY       ((u32)0x00000000UL)
#endif
#ifndef INADDR_BROADCAST
#define INADDR_BROADCAST ((u32)0xffffffffUL)
#endif

#ifndef IPPROTO_IP
#define IPPROTO_IP   0
#endif
#ifndef IPPROTO_ICMP
#define IPPROTO_ICMP 1
#endif
#ifndef IPPROTO_TCP
#define IPPROTO_TCP  6
#endif
#ifndef IPPROTO_UDP
#define IPPROTO_UDP  17
#endif
#ifndef IPPROTO_IPV6
#define IPPROTO_IPV6   41
#endif
#ifndef IPPROTO_ICMPV6
#define IPPROTO_ICMPV6 58
#endif
#ifndef IPPROTO_UDPLITE
#define IPPROTO_UDPLITE 136
#endif
#ifndef IPPROTO_RAW
#define IPPROTO_RAW     255
#endif

#endif
