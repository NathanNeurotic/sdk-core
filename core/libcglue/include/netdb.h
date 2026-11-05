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
#ifndef LIBCGLUE_NETDB_H
#define LIBCGLUE_NETDB_H

#include <sys/cdefs.h>
/* Used to be a blanket #include <tcpip.h> - see libcglue's own
 * sys/socket.h for the full story on why that header is real,
 * pre-lwip-migration technical debt that conflicts with ps2ip's real
 * lwip headers. struct hostent/struct addrinfo are only ever used by
 * pointer below - real, file-scope forward declarations instead
 * (same reasoning, and the same "declared inside parameter list"
 * hazard, as ps2sdkapi.h's own - see that file's comment on why a
 * shared, real tag is needed rather than none at all). */
/* Error codes, numbered like lwip's so values from the ps2ip stack pass through unchanged. */
#ifndef EAI_NONAME
#define EAI_NONAME     200
#define EAI_SERVICE    201
#define EAI_FAIL       202
#define EAI_MEMORY     203
#define EAI_FAMILY     204
#endif
#ifndef HOST_NOT_FOUND
#define HOST_NOT_FOUND 210
#define NO_DATA        211
#define NO_ADDRESS     NO_DATA
#define NO_RECOVERY    212
#define TRY_AGAIN      213
#endif

struct hostent;
struct addrinfo;

__BEGIN_DECLS
extern struct hostent *gethostbyaddr(const void *addr, int len, int type);
extern struct hostent *gethostbyname(const char *name);
extern int gethostbyname_r(const char *name, struct hostent *ret, char *buf, size_t buflen, struct hostent **result, int *h_errnop);
extern void freeaddrinfo(struct addrinfo *ai);
extern int getaddrinfo(const char *nodename, const char *servname, const struct addrinfo *hints, struct addrinfo **res);
__END_DECLS

#endif
