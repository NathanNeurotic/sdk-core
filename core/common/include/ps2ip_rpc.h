/**
 * @file
 * @ingroup core_common
 * PS2IP RPC definitions
 * This header conflicts with fileXio.h
 */

#ifndef __PS2IP_RPC_H__
#define __PS2IP_RPC_H__

#include <tamtypes.h>
/* sys/socket.h is libcglue's own POSIX-named header - EE-only, doesn't
 * exist for an IOP build at all (no libcglue there). The one real IOP
 * consumer of this file (core/ps2ips-iop's ps2ips.c) always includes
 * <ps2ip.h> first, which already includes the real, still-present
 * common/tcpip.h (see ps2ip.h's own comment - never actually deleted,
 * just no longer auto-included from libcglue) and so already provides
 * struct sockaddr/socklen_t/etc by the time this header is reached -
 * this include is both unavailable and unnecessary for _IOP. */
#ifndef _IOP
#include <sys/socket.h>
#endif
#include <sys/time.h>

/* Used to be a blanket #include <tcpip.h> - see libcglue's own
 * sys/socket.h for the full story on why that header is real,
 * pre-lwip-migration technical debt that conflicts with ps2ip's real
 * lwip headers. struct sockaddr/sa_family_t/socklen_t now come from
 * <sys/socket.h> above (byte-identical layout to what tcpip.h used to
 * provide, verified for real against tcpip.h's own struct sockaddr).
 *
 * ip4_addr_t/ip_addr_t need a real, COMPLETE definition here (embedded
 * by value in the RPC packet structs below, not just a pointer) -
 * matches lwip's own real struct ip4_addr { u32_t addr; } exactly, and
 * is a safe re-declaration if lwip's own header is ever also reached
 * in the same TU (same real tag name, same real single-field layout). */
#ifndef LWIP_HDR_IP4_ADDR_H
struct ip4_addr {
    u32 addr;
};
typedef struct ip4_addr ip4_addr_t;
typedef struct ip4_addr ip_addr_t;
#endif

/* MEMP_NUM_NETCONN: the real wire size of ps2ip_rpc_fd_set below
 * depends on this exactly matching whatever lwip was actually built
 * with - real config lives in ps2ip's own lwipopts.h (currently the
 * plain lwip default, unoverridden: MEMP_NUM_TCP_PCB=5,
 * MEMP_NUM_UDP_PCB=4, verified against lwip/opt.h directly). Guarded,
 * not hardcoded unconditionally, so an already-active real lwip config
 * in this TU always wins over this fallback default. */
#ifndef MEMP_NUM_UDP_PCB
#define MEMP_NUM_UDP_PCB 4
#endif
#ifndef MEMP_NUM_TCP_PCB
#define MEMP_NUM_TCP_PCB 5
#endif
#ifndef MEMP_NUM_NETCONN
#define MEMP_NUM_NETCONN (MEMP_NUM_TCP_PCB + MEMP_NUM_UDP_PCB)
#endif

#define PS2IP_IRX 0xB0125F2

enum PS2IPS_RPC_ID {
    PS2IPS_ID_ACCEPT = 1,
    PS2IPS_ID_BIND,
    PS2IPS_ID_DISCONNECT,
    PS2IPS_ID_CONNECT,
    PS2IPS_ID_LISTEN,
    PS2IPS_ID_RECV,
    PS2IPS_ID_RECVFROM,
    PS2IPS_ID_SEND,
    PS2IPS_ID_SENDTO,
    PS2IPS_ID_SOCKET,
    PS2IPS_ID_SETCONFIG,
    PS2IPS_ID_GETCONFIG,
    PS2IPS_ID_SELECT,
    PS2IPS_ID_IOCTL,
    PS2IPS_ID_GETSOCKNAME,
    PS2IPS_ID_GETPEERNAME,
    PS2IPS_ID_GETSOCKOPT,
    PS2IPS_ID_SETSOCKOPT,
    PS2IPS_ID_GETHOSTBYNAME,
#ifdef PS2IP_DNS
    /*    Not implemented:
            gethostbyname_r -> Redudant because it gets called over the RPC.
            freeaddrinfo    -> too complicated and probably nobody uses it?
            getaddrinfo     -> too complicated and probably nobody uses it?    */

    PS2IPS_ID_DNS_SETSERVER,
    PS2IPS_ID_DNS_GETSERVER,
#endif

    PS2IPS_ID_COUNT
};

typedef struct
{
    s32 domain;
    s32 type;
    s32 protocol;
} socket_pkt;

typedef struct
{
    s32 ssize;
    s32 esize;
    u8 *sbuf;
    u8 *ebuf;
    u8 sbuffer[64];
    u8 ebuffer[64];
} rests_pkt;

typedef struct
{
    s32 socket;
    s32 length;
    s32 flags;
    void *ee_addr;
    struct sockaddr sockaddr; // sizeof = 16
    s32 malign;
    /** buffer for sending misaligned portion */
    u8 malign_buff[64];
} send_pkt;

typedef struct
{
    s32 socket;
    s32 length;
    s32 flags;
    void *ee_addr;
    void *intr_data;
} s_recv_pkt;

typedef struct
{
    s32 ret;
    struct sockaddr sockaddr;
} r_recv_pkt;

typedef struct
{
    s32 socket;
    struct sockaddr sockaddr;
    s32 len;
} cmd_pkt;

typedef struct
{
    s32 retval;
    struct sockaddr sockaddr;
} ret_pkt;

typedef struct
{
    s32 s;
    s32 backlog;
} listen_pkt;

typedef struct
{
    s32 s;
    s32 level;
    s32 optname;
} getsockopt_pkt;

typedef struct
{
    s32 result;
    s32 optlen;
    u8 buffer[128];
} getsockopt_res_pkt;

typedef struct
{
    s32 s;
    s32 level;
    s32 optname;
    s32 optlen;
    u8 buffer[128];
} setsockopt_pkt;

/* On-the-wire fd_set for the select RPC. The struct fd_set seen on EE
 * (newlib, ~128 bytes) and IOP (lwIP, MEMP_NUM_NETCONN/8 bytes) compile
 * units differs in size, which used to scramble select_pkt's layout
 * across the SIF RPC boundary — IOP would only touch the first few
 * bytes of EE's wider readset and never see writeset/exceptset at the
 * right offsets. Using a fixed-size struct here keeps select_pkt
 * byte-identical on both sides regardless of which struct fd_set is
 * in scope at the include site. */
typedef struct
{
    unsigned char fd_bits[(MEMP_NUM_NETCONN + 7) / 8];
} ps2ip_rpc_fd_set;

/* struct timeval differs across the boundary too — newlib's tv_sec/tv_usec
 * are 64-bit on EE while the IOP defines them as 32-bit longs. Carry
 * explicit 32-bit fields here so the layout is byte-identical on both
 * sides; each end reconstructs its native struct timeval. */
typedef struct
{
    union
    {
        s32 maxfdp1;
        s32 result;
    };
    void *timeout_p;     /* opaque NULL/non-NULL flag */
    s32 timeout_sec;
    s32 timeout_usec;
    void *readset_p;     /* opaque: only NULL/non-NULL is meaningful across RPC */
    void *writeset_p;
    void *exceptset_p;
    ps2ip_rpc_fd_set readset;
    ps2ip_rpc_fd_set writeset;
    ps2ip_rpc_fd_set exceptset;
} select_pkt;

typedef struct
{
    union
    {
        s32 s;
        s32 result;
    };
    u32 cmd;
    void *argp;
    u32 value;
} ioctl_pkt;

#ifdef PS2IP_DNS
struct hostent_res
{
    s16 h_addrtype;
    s16 h_length;
    ip_addr_t h_addr;
};

typedef struct
{
    s32 result;
    struct hostent_res hostent;
} gethostbyname_res_pkt;

typedef struct
{
    ip_addr_t dnsserver;
    u8 numdns;
} dns_setserver_pkt;

typedef struct
{
    ip_addr_t dnsserver;
} dns_getserver_res_pkt;

#endif

#endif /* __PS2IP_RPC_H__ */
