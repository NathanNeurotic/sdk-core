/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/

/**
 * @file
 * @ingroup core_libcglue
 */
#ifndef LIBCGLUE_PS2SDKAPI_H
#define LIBCGLUE_PS2SDKAPI_H

#include <dirent.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <timer.h>
#include <errno.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Inter-library helpers */

struct _libcglue_fdman_fd_info_;

typedef int (*_libcglue_fdman_getfd_cb_t)(void *userdata);
typedef char *(*_libcglue_fdman_getfilename_cb_t)(void *userdata);
typedef int (*_libcglue_fdman_close_cb_t)(void *userdata);
typedef int (*_libcglue_fdman_read_cb_t)(void *userdata, void *buf, int nbytes);
typedef int (*_libcglue_fdman_lseek_cb_t)(void *userdata, int offset, int whence);
typedef int64_t (*_libcglue_fdman_lseek64_cb_t)(void *userdata, int64_t offset, int whence);
typedef int (*_libcglue_fdman_write_cb_t)(void *userdata, const void *buf, int nbytes);
typedef int (*_libcglue_fdman_ioctl_cb_t)(void *userdata, int request, void *data);
typedef int (*_libcglue_fdman_ioctl2_cb_t)(void *userdata, int request, void *arg, unsigned int arglen, void *buf, unsigned int buflen);
typedef int (*_libcglue_fdman_dread_cb_t)(void *userdata, struct dirent *dir);
typedef int (*_libcglue_fdman_fcntl_f_setfl_cb_t)(void *userdata, int newfl);
typedef int (*_libcglue_fdman_accept_cb_t)(void *userdata, struct _libcglue_fdman_fd_info_ *info, struct sockaddr *addr, socklen_t *addrlen);
typedef int (*_libcglue_fdman_bind_cb_t)(void *userdata, const struct sockaddr *my_addr, socklen_t addrlen);
typedef int (*_libcglue_fdman_connect_cb_t)(void *userdata, const struct sockaddr *serv_addr, socklen_t addrlen);
typedef int (*_libcglue_fdman_listen_cb_t)(void *userdata, int backlog);
typedef ssize_t (*_libcglue_fdman_recv_cb_t)(void *userdata, void *buf, size_t len, int flags);
typedef ssize_t (*_libcglue_fdman_recvfrom_cb_t)(void *userdata, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
typedef ssize_t (*_libcglue_fdman_recvmsg_cb_t)(void *userdata, struct msghdr *msg, int flags);
typedef ssize_t (*_libcglue_fdman_send_cb_t)(void *userdata, const void *buf, size_t len, int flags);
typedef ssize_t (*_libcglue_fdman_sendto_cb_t)(void *userdata, const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
typedef ssize_t (*_libcglue_fdman_sendmsg_cb_t)(void *userdata, const struct msghdr *msg, int flags);
typedef int (*_libcglue_fdman_getsockopt_cb_t)(void *userdata, int level, int optname, void *optval, socklen_t *optlen);
typedef int (*_libcglue_fdman_setsockopt_cb_t)(void *userdata, int level, int optname, const void *optval, socklen_t optlen);
typedef int (*_libcglue_fdman_shutdown_cb_t)(void *userdata, int how);
typedef int (*_libcglue_fdman_getpeername_cb_t)(void *userdata, struct sockaddr *name, socklen_t *namelen);
typedef int (*_libcglue_fdman_getsockname_cb_t)(void *userdata, struct sockaddr *name, socklen_t *namelen);

typedef struct _libcglue_fdman_fd_ops_
{
	_libcglue_fdman_getfd_cb_t getfd;
	_libcglue_fdman_getfilename_cb_t getfilename;
	_libcglue_fdman_close_cb_t close;
	_libcglue_fdman_read_cb_t read;
	_libcglue_fdman_lseek_cb_t lseek;
	_libcglue_fdman_lseek64_cb_t lseek64;
	_libcglue_fdman_write_cb_t write;
	_libcglue_fdman_ioctl_cb_t ioctl;
	_libcglue_fdman_ioctl2_cb_t ioctl2;
	_libcglue_fdman_dread_cb_t dread;
	_libcglue_fdman_fcntl_f_setfl_cb_t fcntl_f_setfl;
	_libcglue_fdman_accept_cb_t accept;
	_libcglue_fdman_bind_cb_t bind;
	_libcglue_fdman_connect_cb_t connect;
	_libcglue_fdman_listen_cb_t listen;
	_libcglue_fdman_recv_cb_t recv;
	_libcglue_fdman_recvfrom_cb_t recvfrom;
	_libcglue_fdman_recvmsg_cb_t recvmsg;
	_libcglue_fdman_send_cb_t send;
	_libcglue_fdman_sendto_cb_t sendto;
	_libcglue_fdman_sendmsg_cb_t sendmsg;
	_libcglue_fdman_getsockopt_cb_t getsockopt;
	_libcglue_fdman_setsockopt_cb_t setsockopt;
	_libcglue_fdman_shutdown_cb_t shutdown;
	_libcglue_fdman_getpeername_cb_t getpeername;
	_libcglue_fdman_getsockname_cb_t getsockname;
} _libcglue_fdman_fd_ops_t;

typedef struct _libcglue_fdman_fd_info_
{
	void *userdata;
	_libcglue_fdman_fd_ops_t *ops;
} _libcglue_fdman_fd_info_t;

typedef int (*_libcglue_fdman_open_cb_t)(_libcglue_fdman_fd_info_t *info, const char *buf, int flags, mode_t mode);
typedef int (*_libcglue_fdman_remove_cb_t)(const char *path);
typedef int (*_libcglue_fdman_rename_cb_t)(const char *old, const char *new_);
typedef int (*_libcglue_fdman_mkdir_cb_t)(const char *path, int mode);
typedef int (*_libcglue_fdman_rmdir_cb_t)(const char *path);
typedef int (*_libcglue_fdman_stat_cb_t)(const char *path, struct stat *buf);
typedef int (*_libcglue_fdman_readlink_cb_t)(const char *path, char *buf, size_t bufsiz);
typedef int (*_libcglue_fdman_symlink_cb_t)(const char *target, const char *linkpath);

typedef struct _libcglue_fdman_path_ops_
{
	_libcglue_fdman_open_cb_t open;
	_libcglue_fdman_remove_cb_t remove;
	_libcglue_fdman_rename_cb_t rename;
	_libcglue_fdman_mkdir_cb_t mkdir;
	_libcglue_fdman_rmdir_cb_t rmdir;
	_libcglue_fdman_stat_cb_t stat;
	_libcglue_fdman_readlink_cb_t readlink;
	_libcglue_fdman_symlink_cb_t symlink;
} _libcglue_fdman_path_ops_t;

extern _libcglue_fdman_path_ops_t *_libcglue_fdman_path_ops;

/* ip4_addr_t/ip_addr_t/socklen_t/struct in_addr are all real types
 * this file needs but doesn't define itself - <sys/socket.h>, included
 * above, is their real, canonical home (see its own comments for why -
 * the short version: this package's own legacy <sys/socket.h> used to
 * pull them in via a blanket #include <tcpip.h>, a whole parallel,
 * pre-lwip-migration copy of lwip's own types that conflicts outright
 * with ps2ip's real lwip headers; replaced with real, minimal,
 * lwip-compatible definitions). By the time this file's own typedefs
 * below run, <sys/socket.h> has already provided all of them.
 *
 * t_ip_info is different: unlike the above, <sys/socket.h> does NOT
 * reliably provide it. Which real file that literal include name
 * resolves to depends on the consuming package's own include_dirs
 * order - ps2ips (this package's small RPC-client sibling) wants
 * libcglue's own <sys/socket.h>, but ps2ip (the real lwip stack
 * package) deliberately puts lwip's own POSIX-compat <sys/socket.h>
 * (a one-line "#include <lwip/sockets.h>" wrapper) earlier in its own
 * include_dirs instead, since ps2ip.h/ps2ipee.h/ps2ip_ps2sdk.c need
 * the real lwip_accept/err_t/pbuf/netif/tcpip chain that only lwip's
 * own sockets.h transitively provides - see core/ps2ip/ps2.yaml's own
 * comment. lwip's real sockets.h obviously has no concept of
 * t_ip_info (a ps2sdk-specific struct, not a POSIX/lwip one), so it
 * can't be left to <sys/socket.h> to supply here - given its own,
 * independent, real definition instead, shared with libcglue's
 * <sys/socket.h> via the same T_IP_INFO_DEFINED guard so whichever of
 * the two headers is reached first in a given TU wins. */
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

typedef int (*_libcglue_fdman_setconfig_cb_t)(const t_ip_info *ip_info);
typedef int (*_libcglue_fdman_getconfig_cb_t)(char *netif_name, t_ip_info *ip_info);
typedef void (*_libcglue_fdman_dns_setserver_cb_t)(u8 numdns, const ip_addr_t *dnsserver);
typedef const ip_addr_t *(*_libcglue_fdman_dns_getserver_cb_t)(u8 numdns);
typedef int (*_libcglue_fdman_socket_cb_t)(_libcglue_fdman_fd_info_t *info, int domain, int type, int protocol);
typedef int (*_libcglue_fdman_select_cb_t)(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

/* Real, file-scope forward declarations - without these, each `struct
 * hostent *`/`struct addrinfo *` below (used only inside a function-
 * pointer parameter list, never given a real definition reachable from
 * this file) would otherwise each create its OWN, function-scope-only
 * tag ("declared inside parameter list will not be visible outside of
 * this definition or declaration"), incompatible with any other real
 * struct hostent/addrinfo a consumer TU later reaches (e.g. lwip's own
 * real lwip/netdb.h) - the exact "incompatible pointer type" pattern
 * already hit and fixed for several other fdman callbacks this
 * session. One shared tag here instead, real definition supplied
 * wherever a TU actually needs one (lwip/netdb.h, or the guarded local
 * copies in ps2ips/src/ps2ipc.c and libcglue/include/netdb.h). */
struct hostent;
struct addrinfo;

typedef struct hostent *(*_libcglue_fdman_gethostbyaddr_cb_t)(const void *addr, int len, int type);
typedef struct hostent *(*_libcglue_fdman_gethostbyname_cb_t)(const char *name);
typedef int (*_libcglue_fdman_gethostbyname_r_cb_t)(const char *name, struct hostent *ret, char *buf, size_t buflen, struct hostent **result, int *h_errnop);
typedef void (*_libcglue_fdman_freeaddrinfo_cb_t)(struct addrinfo *ai);
typedef int (*_libcglue_fdman_getaddrinfo_cb_t)(const char *nodename, const char *servname, const struct addrinfo *hints, struct addrinfo **res);

typedef struct _libcglue_fdman_socket_ops_
{
	_libcglue_fdman_setconfig_cb_t setconfig;
	_libcglue_fdman_getconfig_cb_t getconfig;
	_libcglue_fdman_dns_setserver_cb_t dns_setserver;
	_libcglue_fdman_dns_getserver_cb_t dns_getserver;
	_libcglue_fdman_socket_cb_t socket;
	_libcglue_fdman_select_cb_t select;
	_libcglue_fdman_gethostbyaddr_cb_t gethostbyaddr;
	_libcglue_fdman_gethostbyname_cb_t gethostbyname;
	_libcglue_fdman_gethostbyname_r_cb_t gethostbyname_r;
	_libcglue_fdman_freeaddrinfo_cb_t freeaddrinfo;
	_libcglue_fdman_getaddrinfo_cb_t getaddrinfo;
} _libcglue_fdman_socket_ops_t;

extern _libcglue_fdman_socket_ops_t *_libcglue_fdman_socket_ops;

typedef u32 (*_libcglue_fdman_inet_addr_cb_t)(const char *cp);
typedef char *(*_libcglue_fdman_inet_ntoa_cb_t)(const ip4_addr_t *addr);
typedef char *(*_libcglue_fdman_inet_ntoa_r_cb_t)(const ip4_addr_t *addr, char *buf, int buflen);
typedef int (*_libcglue_fdman_inet_aton_cb_t)(const char *cp, ip4_addr_t *addr);

typedef struct _libcglue_fdman_inet_ops_
{
	_libcglue_fdman_inet_addr_cb_t inet_addr;
	_libcglue_fdman_inet_ntoa_cb_t inet_ntoa;
	_libcglue_fdman_inet_ntoa_r_cb_t inet_ntoa_r;
	_libcglue_fdman_inet_aton_cb_t inet_aton;
} _libcglue_fdman_inet_ops_t;

extern _libcglue_fdman_inet_ops_t *_libcglue_fdman_inet_ops;

/* Functions from cwd.c */
extern char __cwd[MAXNAMLEN + 1];
extern int __path_absolute(const char *in, char *out, int len);

#define PS2_CLOCKS_PER_SEC kBUSCLKBY256 // 576.000
#define PS2_CLOCKS_PER_MSEC (PS2_CLOCKS_PER_SEC / 1000) // 576

/* Disable the auto start of pthread on init for reducing binary size if not used. */
#define PS2_DISABLE_AUTOSTART_PTHREAD() \
	void __libpthreadglue_init() {} \
    void __libpthreadglue_deinit() {}

/* Namco system 246/256 dont have CDVDFSV module loaded on iop reboot. therefore, any libcglue code calling CDVDMAN RPCs will freeze*/
#define LIBCGLUE_SUPPORT_NAMCO_SYSTEM_2x6() \
    void _libcglue_rtc_update() {}

typedef uint64_t ps2_clock_t;
static inline ps2_clock_t ps2_clock(void) {
    // DEPRECATED VERSION USE INSTEAD GetTimerSystemTime
    return (ps2_clock_t)(GetTimerSystemTime() >> 8);
}

extern s64 _ps2sdk_rtc_offset_from_busclk;
s64 _libcglue_rtc_get_offset_from_busclk(void);
extern void _libcglue_rtc_update_impl();
extern void _libcglue_rtc_update();

// The newlib port does not support 64bit
// this should have been defined in unistd.h
typedef int64_t off64_t;
extern off64_t lseek64(int fd, off64_t offset, int whence);

// Functions to be used related to timezone
extern void _libcglue_timezone_update_impl();
extern void _libcglue_timezone_update();

extern void ps2sdk_setTimezone(int timezone);
extern void ps2sdk_setDaylightSaving(int daylightSaving);

extern _libcglue_fdman_fd_info_t *libcglue_get_fd_info(int fd);
extern int __libcglue_init_stdio(_libcglue_fdman_fd_info_t *info, int fd);

/* The fd we provide to final user aren't actually the same than IOP's fd
* so this function allow you to get actual IOP's fd from public fd
*/
extern int ps2sdk_get_iop_fd(int fd);
extern char *ps2sdk_get_iop_filename(int fd);
extern int _ps2sdk_close(int fd);
extern int _ps2sdk_dclose(int fd);
extern int _ps2sdk_read(int fd, void *buf, int nbytes);
extern int _ps2sdk_lseek(int fd, int offset, int whence);
extern int64_t _ps2sdk_lseek64(int fd, int64_t offset, int whence);
extern int _ps2sdk_write(int fd, const void *buf, int nbytes);
extern int _ps2sdk_ioctl(int fd, int request, void *data);
extern int _ps2sdk_ioctl2(int fd, int request, void *arg, unsigned int arglen, void *buf, unsigned int buflen);
extern int _ps2sdk_dread(int fd, struct dirent *dir);

#ifdef __cplusplus
}
#endif

#endif /* LIBCGLUE_PS2SDKAPI_H */
