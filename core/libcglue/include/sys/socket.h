/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/

/*	$NetBSD: socket.h,v 1.131 2021/11/02 20:35:51 christos Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982, 1985, 1986, 1988, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)socket.h	8.6 (Berkeley) 5/3/95
 */

/**
 * @file
 * @ingroup core_libcglue
 */
#ifndef _SYS_SOCKET_H_
#define	_SYS_SOCKET_H_

#if 1
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <unistd.h>
#include <sys/uio.h>
#include <tamtypes.h>

/* Used to be a blanket #include <tcpip.h> - that header is ps2sdk's
 * own pre-lwip-migration "Common definitions file for the LWIP v2.0.0
 * port" (see common/include/tcpip.h's own file comment), a whole
 * parallel, now-superseded copy of lwip's own types/macros. It
 * conflicts outright with the real lwip headers ps2ip actually builds
 * against today (confirmed for real: PBUF_TRANSPORT/PBUF_IP/
 * pbuf_layer/etc redeclaration errors) - and its own socklen_t is
 * `int`, while real lwip's is `u32_t`, a real ABI mismatch that broke
 * ps2ip_ps2sdk.c's fdman callback wiring the moment ps2ip's own
 * headers ever got fixed enough to expose the real lwip chain (see
 * ps2ip.h's own commit history). Replaced with real, minimal
 * equivalents for exactly what this file itself uses below - not
 * everything tcpip.h ever offered, only what's load-bearing here.
 *
 * socklen_t: matches real lwip's own `u32_t socklen_t` exactly (u32 on
 * this platform IS the same real type lwip's u32_t resolves to), using
 * lwip's own SOCKLEN_T_DEFINED convention so a TU that also reaches
 * lwip/sockets.h later doesn't get a conflicting second typedef. */
#ifndef SOCKLEN_T_DEFINED
#define SOCKLEN_T_DEFINED
typedef u32 socklen_t;
#endif

/* struct in_addr: real, full definition (t_ip_info below embeds it by
 * value, so a forward declaration isn't enough here) - guarded on
 * lwip/inet.h's own real include guard (LWIP_HDR_INET_H) so a TU that
 * also reaches that header later doesn't get a real redefinition
 * error; lwip's own struct in_addr has no guard of its own to reuse
 * the other direction. */
#ifndef LWIP_HDR_INET_H
struct in_addr {
	u32 s_addr;
};
#endif

/* ip4_addr_t/ip_addr_t: forward-declared only (every real use below is
 * a pointer, never dereferenced here) - safe because lwip's own real
 * `struct ip4_addr { ... }; typedef struct ip4_addr ip4_addr_t;`
 * (lwip/ip4_addr.h) uses that same real tag name, so this and lwip's
 * own full definition are compatible re-declarations of the same
 * struct if both appear in one TU, not two conflicting types. ip_addr_t
 * itself is a plain alias for ip4_addr_t in lwip's real build config
 * here (LWIP_IPV6 off, this project's actual default - see
 * ps2ip/include/lwipopts.h) - true only because IPv6 is off; if that
 * ever changes, ip_addr_t becomes its own real struct/union upstream
 * and this alias would need revisiting. */
#ifndef LWIP_HDR_IP4_ADDR_H
struct ip4_addr;
typedef struct ip4_addr ip4_addr_t;
typedef struct ip4_addr ip_addr_t;
#endif

/* t_ip_info: real struct, real layout, matches (superseded) tcpip.h's
 * own definition exactly. Real, direct consumers of THIS header
 * (core/ps2ips/src/ps2ipc.c, via ps2ips.h) need it from here - they
 * don't include ps2sdkapi.h at all. ps2sdkapi.h needs it too, and
 * provides its own independent copy (for TUs where <sys/socket.h>
 * resolves to lwip's own real sockets.h instead of this file - see
 * ps2sdkapi.h's own comment) - shared guard so whichever of the two
 * is reached first in a given TU wins and the other is skipped. */
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

/* ioctlsocket()/fcntl() command constants: real, wire-protocol-exact
 * values (ps2ipc_ioctl's `cmd` argument gets RPC'd to the IOP verbatim
 * and interpreted there by lwip's own real lwip_ioctl(), which only
 * understands FIONREAD/FIONBIO) - byte-identical to both (superseded)
 * tcpip.h's own copy and lwip/sockets.h's own real definitions
 * (core/ps2ip/include/lwip_include/lwip/sockets.h), guarded the same
 * way lwip guards them so whichever is reached first in a given TU
 * wins and neither redefines the other. */
#if !defined(FIONREAD) || !defined(FIONBIO)
#ifndef IOCPARM_MASK
#define IOCPARM_MASK 0x7fUL
#define IOC_VOID 0x20000000UL
#define IOC_OUT 0x40000000UL
#define IOC_IN 0x80000000UL
#define IOC_INOUT (IOC_IN | IOC_OUT)
#define _IO(x, y) ((long)(IOC_VOID | ((x) << 8) | (y)))
#define _IOR(x, y, t) ((long)(IOC_OUT | ((sizeof(t) & IOCPARM_MASK) << 16) | ((x) << 8) | (y)))
#define _IOW(x, y, t) ((long)(IOC_IN | ((sizeof(t) & IOCPARM_MASK) << 16) | ((x) << 8) | (y)))
#endif
#endif
#ifndef FIONREAD
#define FIONREAD _IOR('f', 127, unsigned long)
#endif
#ifndef FIONBIO
#define FIONBIO _IOW('f', 126, unsigned long)
#endif

#ifndef F_GETFL
#define F_GETFL 3
#endif
#ifndef F_SETFL
#define F_SETFL 4
#endif

#ifndef O_NONBLOCK
#define O_NONBLOCK 1
#endif
#ifndef O_NDELAY
#define O_NDELAY O_NONBLOCK
#endif

/* sa_family_t/struct sockaddr: real, active definitions - the
 * disabled `#if 0` block further down (originally real NetBSD source,
 * gated behind #include <sys/ansi.h>/<sys/featuretest.h>, neither of
 * which exist in this tree) used to be dead code even before the
 * (superseded) blanket `#include <tcpip.h>` removal - tcpip.h's own
 * hand-copied sa_family_t/struct sockaddr (byte-identical to the
 * disabled block below and to real lwip/sockets.h's SA_FAMILY_T_DEFINED
 * convention) was the only ACTIVE source for these before. Real
 * consumers embedding `struct sockaddr` by value (not just by pointer)
 * need the complete type, not just the forward reference further down
 * in this file - e.g. common/include/ps2ip_rpc.h's RPC packet structs. */
#ifndef SA_FAMILY_T_DEFINED
#define SA_FAMILY_T_DEFINED
typedef u8 sa_family_t;
#endif

#ifndef _SOCKADDR_STRUCT_DEFINED
#define _SOCKADDR_STRUCT_DEFINED
struct sockaddr {
	u8 sa_len;		/* total length */
	sa_family_t sa_family;	/* address family */
	char sa_data[14];	/* actually longer; address value */
};
#endif

/* Socket types, common address families, and SOL_SOCKET/common SO_*
 * option constants: same real, disabled-`#if 0`-block story as
 * sa_family_t/struct sockaddr above - real, byte-identical values,
 * just never given an active home. A deliberately trimmed subset (not
 * the whole exotic BSD AF_* list - AF_APPLETALK/AF_ISDN/AF_BLUETOOTH
 * and friends have no real use on this platform) rather than fully
 * reviving the disabled block, which itself depends on real, missing
 * headers (<sys/featuretest.h>, <sys/ansi.h>, <machine/ansi.h>) and
 * would cascade into more errors than it fixes. Found for real
 * building ps2_drivers's select_test_ee sample ("AF_INET undeclared"). */
#ifndef SOCK_STREAM
#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3
#define SOCK_RDM        4
#define SOCK_SEQPACKET  5
#endif

#ifndef AF_UNSPEC
#define AF_UNSPEC       0
#define AF_LOCAL        1
#define AF_UNIX         AF_LOCAL
#define AF_INET         2
#define AF_INET6        24
#define AF_ROUTE        34
#define AF_MAX          37
#endif
#ifndef PF_UNSPEC
#define PF_UNSPEC       AF_UNSPEC
#define PF_LOCAL        AF_LOCAL
#define PF_UNIX         PF_LOCAL
#define PF_INET         AF_INET
#define PF_INET6        AF_INET6
#define PF_ROUTE        AF_ROUTE
#define PF_MAX          AF_MAX
#endif

#ifndef SOL_SOCKET
#define SOL_SOCKET      0xffff
#endif
#ifndef SO_DEBUG
#define SO_DEBUG        0x0001
#define SO_ACCEPTCONN   0x0002
#define SO_REUSEADDR    0x0004
#define SO_KEEPALIVE    0x0008
#define SO_DONTROUTE    0x0010
#define SO_BROADCAST    0x0020
#define SO_USELOOPBACK  0x0040
#define SO_LINGER       0x0080
#define SO_OOBINLINE    0x0100
#define SO_REUSEPORT    0x0200
#endif
#ifndef SO_SNDBUF
#define SO_SNDBUF       0x1001
#define SO_RCVBUF       0x1002
#define SO_SNDLOWAT     0x1003
#define SO_RCVLOWAT     0x1004
#define SO_ERROR        0x1007
#define SO_TYPE         0x1008
#define SO_SNDTIMEO     0x100b
#define SO_RCVTIMEO     0x100c
#endif

#ifndef _LINGER_STRUCT_DEFINED
#define _LINGER_STRUCT_DEFINED
struct linger {
	int l_onoff;	/* option on/off */
	int l_linger;	/* linger time in seconds */
};
#endif
#endif

/*
 * Maximum queue length specifiable by listen(2).
 */
#ifndef SOMAXCONN
#define	SOMAXCONN	128
#endif

/*
 * Message header for recvmsg and sendmsg calls.
 * Used value-result for recvmsg, value only for sendmsg.
 */
struct msghdr {
	void		*msg_name;	/* optional address */
	socklen_t	msg_namelen;	/* size of address */
	struct iovec	*msg_iov;	/* scatter/gather array */
	int		msg_iovlen;	/* # elements in msg_iov */
	void		*msg_control;	/* ancillary data, see below */
	socklen_t	msg_controllen;	/* ancillary data buffer len */
	int		msg_flags;	/* flags on received message */
};

// TODO: Not available in LWIP
#define	MSG_TRUNC	0x0000		/* data discarded before delivery */

#if 0
#include <sys/featuretest.h>

/*
 * Definitions related to sockets: types, address families, options.
 */

/*
 * Data types.
 */
#include <sys/ansi.h>

#ifndef sa_family_t
typedef __sa_family_t	sa_family_t;
#define sa_family_t	__sa_family_t
#endif

#ifndef socklen_t
typedef __socklen_t	socklen_t;
#define socklen_t	__socklen_t
#endif

#include <machine/ansi.h>

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

#ifdef	_BSD_SSIZE_T_
typedef	_BSD_SSIZE_T_	ssize_t;
#undef	_BSD_SSIZE_T_
#endif

#include <sys/uio.h>
#include <sys/sigtypes.h>

/*
 * Socket types.
 */
#define	SOCK_STREAM	1		/* stream socket */
#define	SOCK_DGRAM	2		/* datagram socket */
#define	SOCK_RAW	3		/* raw-protocol interface */
#define	SOCK_RDM	4		/* reliably-delivered message */
#define	SOCK_SEQPACKET	5		/* sequenced packet stream */
#define	SOCK_CONN_DGRAM	6		/* connection-orientated datagram */
#define	SOCK_DCCP	SOCK_CONN_DGRAM

#define	SOCK_CLOEXEC	0x10000000	/* set close on exec on socket */
#define	SOCK_NONBLOCK	0x20000000	/* set non blocking i/o socket */
#define	SOCK_NOSIGPIPE	0x40000000	/* don't send sigpipe */
#define	SOCK_FLAGS_MASK	0xf0000000	/* flags mask */

/*
 * Option flags per-socket.
 */
#define	SO_DEBUG	0x0001		/* turn on debugging info recording */
#define	SO_ACCEPTCONN	0x0002		/* socket has had listen() */
#define	SO_REUSEADDR	0x0004		/* allow local address reuse */
#define	SO_KEEPALIVE	0x0008		/* keep connections alive */
#define	SO_DONTROUTE	0x0010		/* just use interface addresses */
#define	SO_BROADCAST	0x0020		/* permit sending of broadcast msgs */
#define	SO_USELOOPBACK	0x0040		/* bypass hardware when possible */
#define	SO_LINGER	0x0080		/* linger on close if data present */
#define	SO_OOBINLINE	0x0100		/* leave received OOB data in line */
#define	SO_REUSEPORT	0x0200		/* allow local address & port reuse */
/* 	SO_OTIMESTAMP	0x0400		*/
#define	SO_NOSIGPIPE	0x0800		/* no SIGPIPE from EPIPE */
#define	SO_ACCEPTFILTER	0x1000		/* there is an accept filter */
#define	SO_TIMESTAMP	0x2000		/* timestamp received dgram traffic */
#define	SO_RERROR	0x4000		/* Keep track of receive errors */

/* Allowed default option flags */
#define SO_DEFOPTS	(SO_DEBUG|SO_REUSEADDR|SO_KEEPALIVE|SO_DONTROUTE| \
    SO_BROADCAST|SO_USELOOPBACK|SO_LINGER|SO_OOBINLINE|SO_REUSEPORT| \
    SO_NOSIGPIPE|SO_TIMESTAMP|SO_RERROR)

#define __SO_OPTION_BITS \
	"\20" \
	"\1SO_DEBUG" \
	"\2SO_ACCEPTCONN" \
	"\3SO_REUSEADDR" \
	"\4SO_KEEPALIVE" \
	"\5SO_DONTROUTE" \
	"\6SO_BROADCAST" \
	"\7SO_USELOOPBACK" \
	"\10SO_LINGER" \
	"\11SO_OOBINLINE" \
	"\12SO_REUSEPORT" \
	"\13SO_OTIMESTAMP" \
	"\14SO_NOSIGPIPE" \
	"\15SO_ACCEPTFILTER" \
	"\16SO_TIMESTAMP" \
	"\17SO_RERROR"

/*
 * Additional options, not kept in so_options.
 */
#define SO_SNDBUF	0x1001		/* send buffer size */
#define SO_RCVBUF	0x1002		/* receive buffer size */
#define SO_SNDLOWAT	0x1003		/* send low-water mark */
#define SO_RCVLOWAT	0x1004		/* receive low-water mark */
/* SO_OSNDTIMEO		0x1005 */
/* SO_ORCVTIMEO		0x1006 */
#define	SO_ERROR	0x1007		/* get error status and clear */
#define	SO_TYPE		0x1008		/* get socket type */
#define	SO_OVERFLOWED	0x1009		/* datagrams: return packets dropped */

#define	SO_NOHEADER	0x100a		/* user supplies no header to kernel;
					 * kernel removes header and supplies
					 * payload
					 */
#define SO_SNDTIMEO	0x100b		/* send timeout */
#define SO_RCVTIMEO	0x100c		/* receive timeout */
/*
 * Structure used for manipulating linger option.
 */
struct	linger {
	int	l_onoff;		/* option on/off */
	int	l_linger;		/* linger time in seconds */
};

struct	accept_filter_arg {
	char	af_name[16];
	char	af_arg[256-16];
};

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define	SOL_SOCKET	0xffff		/* options for socket level */

/*
 * Address families.
 */
#define	AF_UNSPEC	0		/* unspecified */
#define	AF_LOCAL	1		/* local to host */
#define	AF_UNIX		AF_LOCAL	/* backward compatibility */
#define	AF_INET		2		/* internetwork: UDP, TCP, etc. */
#define	AF_IMPLINK	3		/* arpanet imp addresses */
#define	AF_PUP		4		/* pup protocols: e.g. BSP */
#define	AF_CHAOS	5		/* mit CHAOS protocols */
#define	AF_NS		6		/* XEROX NS protocols */
#define	AF_ISO		7		/* ISO protocols */
#define	AF_OSI		AF_ISO
#define	AF_ECMA		8		/* european computer manufacturers */
#define	AF_DATAKIT	9		/* datakit protocols */
#define	AF_CCITT	10		/* CCITT protocols, X.25 etc */
#define	AF_SNA		11		/* IBM SNA */
#define AF_DECnet	12		/* DECnet */
#define AF_DLI		13		/* DEC Direct data link interface */
#define AF_LAT		14		/* LAT */
#define	AF_HYLINK	15		/* NSC Hyperchannel */
#define	AF_APPLETALK	16		/* Apple Talk */
#define	AF_OROUTE	17		/* Internal Routing Protocol */
#define	AF_LINK		18		/* Link layer interface */
#if defined(_NETBSD_SOURCE)
#define	pseudo_AF_XTP	19		/* eXpress Transfer Protocol (no AF) */
#endif
#define	AF_COIP		20		/* connection-oriented IP, aka ST II */
#define	AF_CNT		21		/* Computer Network Technology */
#if defined(_NETBSD_SOURCE)
#define pseudo_AF_RTIP	22		/* Help Identify RTIP packets */
#endif
#define	AF_IPX		23		/* Novell Internet Protocol */
#define	AF_INET6	24		/* IP version 6 */
#if defined(_NETBSD_SOURCE)
#define pseudo_AF_PIP	25		/* Help Identify PIP packets */
#endif
#define AF_ISDN		26		/* Integrated Services Digital Network*/
#define AF_E164		AF_ISDN		/* CCITT E.164 recommendation */
#define AF_NATM		27		/* native ATM access */
#define AF_ARP		28		/* (rev.) addr. res. prot. (RFC 826) */
#if defined(_NETBSD_SOURCE)
#define pseudo_AF_KEY	29		/* Internal key management protocol  */
#define	pseudo_AF_HDRCMPLT 30		/* Used by BPF to not rewrite hdrs
					   in interface output routine */
#endif
#define AF_BLUETOOTH	31		/* Bluetooth: HCI, SCO, L2CAP, RFCOMM */
#define	AF_IEEE80211	32		/* IEEE80211 */
#define	AF_MPLS		33		/* MultiProtocol Label Switching */
#define	AF_ROUTE	34		/* Internal Routing Protocol */
#define	AF_CAN		35
#define	AF_ETHER	36
#define	AF_MAX		37

/*
 * Structure used by kernel to store most
 * addresses.
 */
struct sockaddr {
	__uint8_t	sa_len;		/* total length */
	sa_family_t	sa_family;	/* address family */
	char		sa_data[14];	/* actually longer; address value */
};

#if defined(_KERNEL)
/*
 * Structure used by kernel to pass protocol
 * information in raw sockets.
 */
struct sockproto {
	u_short	sp_family;		/* address family */
	u_short	sp_protocol;		/* protocol */
};

/*
 * we make the entire struct at least UCHAR_MAX + 1 in size since existing
 * use of sockaddr_un permits a path up to 253 bytes + '\0'.
 * sizeof(sb_len) + sizeof(sb_family) + 253 + '\0'
 */
#define _SB_DATASIZE	254
struct sockaddr_big {
    union {
	struct {
	    __uint8_t	sb_len;
	    sa_family_t	sb_family;
	    char	sb_data[_SB_DATASIZE];
	};
	uint64_t dummy; /* solicit natural alignment */
    };
};

#endif /* _KERNEL */

/*
 * RFC 2553: protocol-independent placeholder for socket addresses
 */
#define _SS_MAXSIZE	128
#define _SS_ALIGNSIZE	(sizeof(__int64_t))
#define _SS_PAD1SIZE	(_SS_ALIGNSIZE - 2)
#define _SS_PAD2SIZE	(_SS_MAXSIZE - 2 - _SS_PAD1SIZE - _SS_ALIGNSIZE)

struct sockaddr_storage {
	__uint8_t	ss_len;		/* address length */
	sa_family_t	ss_family;	/* address family */
	char		__ss_pad1[_SS_PAD1SIZE];
	__int64_t     __ss_align;/* force desired structure storage alignment */
	char		__ss_pad2[_SS_PAD2SIZE];
};

#if defined(_NETBSD_SOURCE)
#define	sstosa(__ss)	((struct sockaddr *)(__ss))
#define	sstocsa(__ss)	((const struct sockaddr *)(__ss))
#endif /* _NETBSD_SOURCE */

/*
 * Protocol families, same as address families for now.
 */
#define	PF_UNSPEC	AF_UNSPEC
#define	PF_LOCAL	AF_LOCAL
#define	PF_UNIX		PF_LOCAL	/* backward compatibility */
#define	PF_INET		AF_INET
#define	PF_IMPLINK	AF_IMPLINK
#define	PF_PUP		AF_PUP
#define	PF_CHAOS	AF_CHAOS
#define	PF_NS		AF_NS
#define	PF_ISO		AF_ISO
#define	PF_OSI		AF_ISO
#define	PF_ECMA		AF_ECMA
#define	PF_DATAKIT	AF_DATAKIT
#define	PF_CCITT	AF_CCITT
#define	PF_SNA		AF_SNA
#define PF_DECnet	AF_DECnet
#define PF_DLI		AF_DLI
#define PF_LAT		AF_LAT
#define	PF_HYLINK	AF_HYLINK
#define	PF_APPLETALK	AF_APPLETALK
#define	PF_OROUTE	AF_OROUTE
#define	PF_LINK		AF_LINK
#if defined(_NETBSD_SOURCE)
#define	PF_XTP		pseudo_AF_XTP	/* really just proto family, no AF */
#endif
#define	PF_COIP		AF_COIP
#define	PF_CNT		AF_CNT
#define	PF_INET6	AF_INET6
#define	PF_IPX		AF_IPX		/* same format as AF_NS */
#if defined(_NETBSD_SOURCE)
#define PF_RTIP		pseudo_AF_RTIP	/* same format as AF_INET */
#define PF_PIP		pseudo_AF_PIP
#endif
#define PF_ISDN		AF_ISDN		/* same as E164 */
#define PF_E164		AF_E164
#define PF_NATM		AF_NATM
#define PF_ARP		AF_ARP
#if defined(_NETBSD_SOURCE)
#define PF_KEY 		pseudo_AF_KEY	/* like PF_ROUTE, only for key mgmt */
#endif
#define PF_BLUETOOTH	AF_BLUETOOTH
#define	PF_MPLS		AF_MPLS
#define	PF_ROUTE	AF_ROUTE
#define	PF_CAN		AF_CAN
#define	PF_ETHER	AF_ETHER

#define	PF_MAX		AF_MAX

#if defined(_NETBSD_SOURCE)

#ifndef pid_t
typedef __pid_t		pid_t;		/* process id */
#define pid_t		__pid_t
#endif

#ifndef	gid_t
typedef	__gid_t		gid_t;		/* group id */
#define	gid_t		__gid_t
#endif

#ifndef	uid_t
typedef	__uid_t		uid_t;		/* user id */
#define	uid_t		__uid_t
#endif

/*
 * Socket credentials.
 */
struct sockcred {
	pid_t	sc_pid;			/* process id */
	uid_t	sc_uid;			/* real user id */
	uid_t	sc_euid;		/* effective user id */
	gid_t	sc_gid;			/* real group id */
	gid_t	sc_egid;		/* effective group id */
	int	sc_ngroups;		/* number of supplemental groups */
	gid_t	sc_groups[1];		/* variable length */
};

/*
 * Compute size of a sockcred structure with groups.
 *
 * The (ngrps - 1) is to account for struct sockcred being defined with
 * already one group member. This code works correctly when ngroups == 0
 * because of unsigned arithmetic wrap-around.
 */
#define	SOCKCREDSIZE(ngrps) \
	(/*LINTED*/sizeof(struct sockcred) + (sizeof(gid_t) * ((ngrps) - 1)))
#endif /* _NETBSD_SOURCE */


#if defined(_NETBSD_SOURCE)
/* Definition for CTL_NET PCB fetching sysctls */
struct kinfo_pcb {
	__uint64_t	ki_pcbaddr;	/* PTR: pcb addr */
	__uint64_t	ki_ppcbaddr;	/* PTR: ppcb addr */
	__uint64_t	ki_sockaddr;	/* PTR: socket addr */

	__uint32_t	ki_family;	/* INT: protocol family */
	__uint32_t	ki_type;	/* INT: socket type */
	__uint32_t	ki_protocol;	/* INT: protocol */
	__uint32_t	ki_pflags;	/* INT: generic protocol flags */

	__uint32_t	ki_sostate;	/* INT: socket state */
	__uint32_t	ki_prstate;	/* INT: protocol state */
	__int32_t	ki_tstate;	/* INT: tcp state */
	__uint32_t	ki_tflags;	/* INT: tcp flags */

	__uint64_t	ki_rcvq;	/* U_LONG: receive queue len */
	__uint64_t	ki_sndq;	/* U_LONG: send queue len */

	union {
		struct sockaddr	_kis_src; /* STRUCT: local address */
		char _kis_pad[256 + 8];		/* pad to max addr length */
	} ki_s;
	union {
		struct sockaddr	_kid_dst; /* STRUCT: remote address */
		char _kid_pad[256 + 8];		/* pad to max addr length */
	} ki_d;

	__uint64_t	ki_inode;	/* INO_T: fake inode number */
	__uint64_t	ki_vnode;	/* PTR: if associated with file */
	__uint64_t	ki_conn;	/* PTR: control block of peer */
	__uint64_t	ki_refs;	/* PTR: referencing socket */
	__uint64_t	ki_nextref;	/* PTR: link in refs list */
};

#define ki_src ki_s._kis_src
#define ki_dst ki_d._kid_dst
#define ki_spad ki_s._kis_pad
#define ki_dpad ki_d._kid_pad

#define PCB_SLOP		20
#define PCB_ALL			0

#endif /* _NETBSD_SOURCE */

#if defined(_NETBSD_SOURCE)
/*
 * PF_ROUTE - Routing table
 *
 * Three additional levels are defined:
 *	Fourth: address family, 0 is wildcard
 *	Fifth: type of info, defined below
 *	Sixth: flag(s) to mask with for NET_RT_FLAGS
 */
#define	NET_RT_DUMP		1	/* dump; may limit to a.f. */
#define	NET_RT_FLAGS		2	/* by flags, e.g. RESOLVING */
#define	NET_RT_OOOIFLIST	3	/* old NET_RT_IFLIST (pre 1.5) */
#define	NET_RT_OOIFLIST		4	/* old NET_RT_IFLIST (pre-64bit time) */
#define	NET_RT_OIFLIST		5	/* old NET_RT_IFLIST (pre 8.0) */
#define	NET_RT_IFLIST		6	/* survey interface list */

#endif /* _NETBSD_SOURCE */

/*
 * Maximum queue length specifiable by listen(2).
 */
#ifndef SOMAXCONN
#define	SOMAXCONN	128
#endif

#include <sys/cdefs.h>

/*
 * Message header for recvmsg and sendmsg calls.
 * Used value-result for recvmsg, value only for sendmsg.
 */
struct msghdr {
	void		*msg_name;	/* optional address */
	socklen_t	msg_namelen;	/* size of address */
	struct iovec	*msg_iov;	/* scatter/gather array */
	int		msg_iovlen;	/* # elements in msg_iov */
	void		*msg_control;	/* ancillary data, see below */
	socklen_t	msg_controllen;	/* ancillary data buffer len */
	int		msg_flags;	/* flags on received message */
};

#define	MSG_OOB		0x0001		/* process out-of-band data */
#define	MSG_PEEK	0x0002		/* peek at incoming message */
#define	MSG_DONTROUTE	0x0004		/* send without using routing tables */
#define	MSG_EOR		0x0008		/* data completes record */
#define	MSG_TRUNC	0x0010		/* data discarded before delivery */
#define	MSG_CTRUNC	0x0020		/* control data lost before delivery */
#define	MSG_WAITALL	0x0040		/* wait for full request or error */
#define	MSG_DONTWAIT	0x0080		/* this message should be nonblocking */
#define	MSG_BCAST	0x0100		/* this message was rcvd using link-level brdcst */
#define	MSG_MCAST	0x0200		/* this message was rcvd using link-level mcast */
#define	MSG_NOSIGNAL	0x0400		/* do not generate SIGPIPE on EOF */
#if defined(_NETBSD_SOURCE)
#define	MSG_CMSG_CLOEXEC 0x0800		/* close on exec receiving fd */
#define	MSG_NBIO	0x1000		/* use non-blocking I/O */
#define	MSG_WAITFORONE	0x2000		/* recvmmsg() wait for one message */
#define	MSG_NOTIFICATION 0x4000		/* SCTP notification */

struct mmsghdr {
	struct msghdr msg_hdr;
	unsigned int msg_len;
};
#endif

/* Extra flags used internally only */
#define	MSG_USERFLAGS	0x0ffffff
#define MSG_NAMEMBUF	0x1000000	/* msg_name is an mbuf */
#define MSG_CONTROLMBUF	0x2000000	/* msg_control is an mbuf */
#define MSG_IOVUSRSPACE	0x4000000	/* msg_iov is in user space */
#define MSG_LENUSRSPACE	0x8000000	/* address length is in user space */

/*
 * Header for ancillary data objects in msg_control buffer.
 * Used for additional information with/about a datagram
 * not expressible by flags.  The format is a sequence
 * of message elements headed by cmsghdr structures.
 */
struct cmsghdr {
	socklen_t	cmsg_len;	/* data byte count, including hdr */
	int		cmsg_level;	/* originating protocol */
	int		cmsg_type;	/* protocol-specific type */
/* followed by	u_char  cmsg_data[]; */
};

/*
 * Alignment requirement for CMSG struct manipulation.
 * This basically behaves the same as ALIGN() ARCH/include/param.h.
 * We declare it separately for two reasons:
 * (1) avoid dependency between machine/param.h, and (2) to sync with kernel's
 * idea of ALIGNBYTES at runtime.
 * without (2), we can't guarantee binary compatibility in case of future
 * changes in ALIGNBYTES.
 */
#define __CMSG_ALIGN(n)	(((n) + __ALIGNBYTES) & ~__ALIGNBYTES)

#ifdef _KERNEL
#define CMSG_ALIGN(n)	__CMSG_ALIGN(n)
#endif

#define __CMSG_ASIZE	__CMSG_ALIGN(sizeof(struct cmsghdr))
#define __CMSG_MSGNEXT(cmsg) \
    (__CASTV(char *, cmsg) + __CMSG_ALIGN((cmsg)->cmsg_len))
#define __CMSG_MSGEND(mhdr) \
    (__CASTV(char *, (mhdr)->msg_control) + (mhdr)->msg_controllen)

/* given pointer to struct cmsghdr, return pointer to data */
#define	CMSG_DATA(cmsg) (__CASTV(u_char *, cmsg) + __CMSG_ASIZE)
#define	CCMSG_DATA(cmsg) (__CASTCV(const u_char *, cmsg) + __CMSG_ASIZE)

/* given pointer to struct cmsghdr, return pointer to next cmsghdr */
#define	CMSG_NXTHDR(mhdr, cmsg)	\
    __CASTV(struct cmsghdr *,  \
	__CMSG_MSGNEXT(cmsg) + __CMSG_ASIZE > __CMSG_MSGEND(mhdr) ? 0 : \
	__CMSG_MSGNEXT(cmsg))

/*
 * RFC 2292 requires to check msg_controllen, in case that the kernel returns
 * an empty list for some reasons.
 */
#define	CMSG_FIRSTHDR(mhdr) \
    __CASTV(struct cmsghdr *, \
	(mhdr)->msg_controllen < sizeof(struct cmsghdr) ? 0 : \
	(mhdr)->msg_control)

#define CMSG_SPACE(l)	(__CMSG_ASIZE + __CMSG_ALIGN(l))
#define CMSG_LEN(l)	(__CMSG_ASIZE + (l))

/* "Socket"-level control message types: */
#define	SCM_RIGHTS	0x01		/* access rights (array of int) */
#if defined(_NETBSD_SOURCE)
/*			0x02		   timestamp (struct timeval50) */
/*			0x04		   credentials (struct sockcred70) */
#define	SCM_TIMESTAMP	0x08		/* timestamp (struct timeval) */
#define	SCM_CREDS	0x10		/* credentials (struct sockcred) */
#endif

/*
 * Types of socket shutdown(2).
 */
#define	SHUT_RD		0		/* Disallow further receives. */
#define	SHUT_WR		1		/* Disallow further sends. */
#define	SHUT_RDWR	2		/* Disallow further sends/receives. */

#ifdef	_KERNEL
static __inline socklen_t
sockaddr_getlen(const struct sockaddr *sa)
{
	return sa->sa_len;
}

__BEGIN_DECLS
socklen_t sockaddr_getsize_by_family(sa_family_t);
struct sockaddr *sockaddr_copy(struct sockaddr *, socklen_t,
    const struct sockaddr *);
struct sockaddr *sockaddr_externalize(struct sockaddr *, socklen_t,
    const struct sockaddr *);
struct sockaddr *sockaddr_alloc(sa_family_t, socklen_t, int);
const void *sockaddr_const_addr(const struct sockaddr *, socklen_t *);
void *sockaddr_addr(struct sockaddr *, socklen_t *);
const struct sockaddr *sockaddr_any(const struct sockaddr *);
const struct sockaddr *sockaddr_any_by_family(sa_family_t);
const void *sockaddr_anyaddr(const struct sockaddr *, socklen_t *);
int sockaddr_cmp(const struct sockaddr *, const struct sockaddr *);
struct sockaddr *sockaddr_dup(const struct sockaddr *, int);
int sockaddr_format(const struct sockaddr *, char *, size_t);
void sockaddr_free(struct sockaddr *);
__END_DECLS
#endif /* _KERNEL */
#endif

#ifndef	_KERNEL

__BEGIN_DECLS
extern int	accept(int, struct sockaddr * __restrict, socklen_t * __restrict);
#if 0
extern int	accept4(int, struct sockaddr * __restrict, socklen_t * __restrict, int);
#endif
extern int bind(int, const struct sockaddr *, socklen_t);
extern int connect(int, const struct sockaddr *, socklen_t);
extern int getpeername(int, struct sockaddr * __restrict, socklen_t * __restrict);
extern int getsockname(int, struct sockaddr * __restrict, socklen_t * __restrict);
extern int getsockopt(int, int, int, void *__restrict, socklen_t * __restrict);
#if 0
extern int	getsockopt2(int, int, int, void *__restrict, socklen_t * __restrict);
#endif
extern int	listen(int, int);
#if 0
extern int	paccept(int, struct sockaddr * __restrict, socklen_t * __restrict,
	const sigset_t * __restrict, int);
#endif
extern ssize_t recv(int, void *, size_t, int);
extern ssize_t recvfrom(int, void *__restrict, size_t, int,
      struct sockaddr * __restrict, socklen_t * __restrict);
extern ssize_t recvmsg(int, struct msghdr *, int);
extern ssize_t send(int, const void *, size_t, int);
extern ssize_t sendto(int, const void *,
      size_t, int, const struct sockaddr *, socklen_t);
extern ssize_t sendmsg(int, const struct msghdr *, int);
extern int setsockopt(int, int, int, const void *, socklen_t);
extern int shutdown(int, int);
#if 0
extern int	sockatmark(int);
#endif
extern int	socket(int, int, int)
#if 0
#if !defined(__LIBC12_SOURCE__) && !defined(_STANDALONE)
__RENAME(__socket30)
#endif
#endif
			     ;
#if 0
extern int	socketpair(int, int, int, int *);
#endif

#if 0
#if defined(_NETBSD_SOURCE)
extern int	sendmmsg(int, struct mmsghdr *, unsigned int, unsigned int);
struct timespec;
extern int	recvmmsg(int, struct mmsghdr *, unsigned int, unsigned int,
    struct timespec *);
#endif
#endif
#if 1
/* setconfig/dns_setserver take const - matches both real backend
 * implementations exactly (core/ps2ip's own ps2ip_setconfig/
 * dns_setserver and core/ps2ips's own ps2ipc_ps2ip_setconfig/
 * ps2ipc_dns_setserver, and lwip's own real dns_setserver prototype).
 * Real, previously-latent bug found for real porting Open-PS2-Loader:
 * this was declared non-const, silently fine only because nothing had
 * ever included both this header and a real backend's own declaration
 * (ps2ipee.h, ps2ipc.h) in the same TU until now. */
extern int libcglue_ps2ip_setconfig(const t_ip_info *ip_info);
extern int libcglue_ps2ip_getconfig(char *netif_name, t_ip_info *ip_info);
extern void libcglue_dns_setserver(u8 numdns, const ip_addr_t *dnsserver);
extern const ip_addr_t *libcglue_dns_getserver(u8 numdns);
#endif
__END_DECLS

#ifndef LIBCGLUE_SYS_SOCKET_ALIASES
#define LIBCGLUE_SYS_SOCKET_ALIASES 0
#endif

#if LIBCGLUE_SYS_SOCKET_ALIASES
#define closesocket(...) close(__VA_ARGS__)
#define readsocket(...) read(__VA_ARGS__)
#define writesocket(...) write(__VA_ARGS__)
#define ioctlsocket(...) ioctl(__VA_ARGS__)
#define fcntlsocket(...) fcntl(__VA_ARGS__)
#define lwip_accept(...) accept(__VA_ARGS__)
#define lwip_bind(...) bind(__VA_ARGS__)
#define lwip_shutdown(...) shutdown(__VA_ARGS__)
#define lwip_getpeername(...) getpeername(__VA_ARGS__)
#define lwip_getsockname(...) getsockname(__VA_ARGS__)
#define lwip_getsockopt(...) getsockopt(__VA_ARGS__)
#define lwip_setsockopt(...) setsockopt(__VA_ARGS__)
#define lwip_close(...) close(__VA_ARGS__)
#define lwip_connect(...) connect(__VA_ARGS__)
#define lwip_listen(...) listen(__VA_ARGS__)
#define lwip_recv(...) recv(__VA_ARGS__)
#define lwip_read(...) read(__VA_ARGS__)
#define lwip_recvfrom(...) recvfrom(__VA_ARGS__)
#define lwip_send(...) send(__VA_ARGS__)
#define lwip_sendto(...) sendto(__VA_ARGS__)
#define lwip_socket(...) socket(__VA_ARGS__)
#define lwip_write(...) write(__VA_ARGS__)
#define lwip_select(...) select(__VA_ARGS__)
#define lwip_ioctl(...) ioctl(__VA_ARGS__)
#define lwip_fcntl(...) fcntl(__VA_ARGS__)
#define disconnect(...) close(__VA_ARGS__)
#define ps2ip_setconfig(...) libcglue_ps2ip_setconfig(__VA_ARGS__)
#define ps2ip_getconfig(...) libcglue_ps2ip_getconfig(__VA_ARGS__)
#define dns_setserver(...) libcglue_dns_setserver(__VA_ARGS__)
#define dns_getserver(...) libcglue_dns_getserver(__VA_ARGS__)
#endif

#endif /* !_KERNEL */

#endif /* !_SYS_SOCKET_H_ */
