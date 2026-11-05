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
#ifndef LIBCGLUE_NETINET_TCP_H
#define LIBCGLUE_NETINET_TCP_H

/* Used to be a blanket #include <tcpip.h> - see libcglue's own
 * sys/socket.h for the full story on why that header is real,
 * pre-lwip-migration technical debt that conflicts with ps2ip's real
 * lwip headers. Real, minimal, guarded values instead - byte-
 * identical to both tcpip.h's own copy and real lwip's own
 * lwip/sockets.h (core/ps2ip/include/lwip_include/lwip/sockets.h). */
#ifndef TCP_NODELAY
#define TCP_NODELAY   0x01
#endif
#ifndef TCP_KEEPALIVE
#define TCP_KEEPALIVE 0x02
#endif
#ifndef TCP_KEEPIDLE
#define TCP_KEEPIDLE  0x03
#endif
#ifndef TCP_KEEPINTVL
#define TCP_KEEPINTVL 0x04
#endif
#ifndef TCP_KEEPCNT
#define TCP_KEEPCNT   0x05
#endif

#endif
