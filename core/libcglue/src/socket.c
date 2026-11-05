/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/

#define LIBCGLUE_SYS_SOCKET_NO_ALIASES
#define LIBCGLUE_ARPA_INET_NO_ALIASES
#include <sys/socket.h>
#include <ps2sdkapi.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "fdman.h"

/* The following is defined in glue.c */
extern int __transform_errno(int res);

int	socket(int domain, int type, int protocol)
{
	int fd, iop_fd;
	_libcglue_fdman_fd_info_t *info;

	if (_libcglue_fdman_socket_ops == NULL || _libcglue_fdman_socket_ops->socket == NULL)
	{
		errno = ENOSYS;
		return -1;
	}

	fd = __fdman_get_new_descriptor();
	if (fd == -1)
	{
		errno = ENOMEM;
		return -1;
	}

	info = &(__descriptormap[fd]->info);
	iop_fd = _libcglue_fdman_socket_ops->socket(info, domain, type, protocol);
	if (iop_fd < 0)
	{
		__fdman_release_descriptor(fd);
		return __transform_errno(iop_fd);
	}
	__descriptormap[fd]->flags = 0;

	return fd;
}

int	accept(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
	int new_iop_fd, new_fd;
	_libcglue_fdman_fd_info_t *info;

	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->accept == NULL)
	{
		errno = ENOSYS;
		return -1;
	}

	new_fd = __fdman_get_new_descriptor();
	if (new_fd == -1)
	{
		errno = ENOMEM;
		return -1;
	}

	info = &(__descriptormap[new_fd]->info);
	new_iop_fd = fdinfo->ops->accept(fdinfo->userdata, info, addr, addrlen);
	if (new_iop_fd < 0)
	{
		__fdman_release_descriptor(new_fd);
		return __transform_errno(new_iop_fd);
	}
	__descriptormap[new_fd]->flags = 0;
	return new_fd;
}

int	bind(int fd, const struct sockaddr *my_addr, socklen_t addrlen)
{
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->bind == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	return __transform_errno(fdinfo->ops->bind(fdinfo->userdata, my_addr, addrlen));
}

int	connect(int fd, const struct sockaddr *serv_addr, socklen_t addrlen)
{
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->connect == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	return __transform_errno(fdinfo->ops->connect(fdinfo->userdata, serv_addr, addrlen));
}

int	listen(int fd, int backlog)
{
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->listen == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	return __transform_errno(fdinfo->ops->listen(fdinfo->userdata, backlog));
}

ssize_t	recv(int fd, void *buf, size_t len, int flags)
{
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->recv == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	return __transform_errno(fdinfo->ops->recv(fdinfo->userdata, buf, len, flags));
}

ssize_t	recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen)
{
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->recvfrom == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	return __transform_errno(fdinfo->ops->recvfrom(fdinfo->userdata, buf, len, flags, from, fromlen));
}

ssize_t recvmsg(int fd, struct msghdr *msg, int flags)
{
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->recvmsg == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	return __transform_errno(fdinfo->ops->recvmsg(fdinfo->userdata, msg, flags));
}

ssize_t	send(int fd, const void *buf, size_t len, int flags)
{
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->send == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	return __transform_errno(fdinfo->ops->send(fdinfo->userdata, buf, len, flags));
}

ssize_t	sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->sendto == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	return __transform_errno(fdinfo->ops->sendto(fdinfo->userdata, buf, len, flags, to, tolen));
}

ssize_t sendmsg(int fd, const struct msghdr *msg, int flags)
{
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->sendmsg == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	return __transform_errno(fdinfo->ops->sendmsg(fdinfo->userdata, msg, flags));
}

int	getsockopt(int fd, int level, int optname, void *optval, socklen_t *optlen)
{
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->getsockopt == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	return __transform_errno(fdinfo->ops->getsockopt(fdinfo->userdata, level, optname, optval, optlen));
}

int	setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen)
{
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->setsockopt == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	return __transform_errno(fdinfo->ops->setsockopt(fdinfo->userdata, level, optname, optval, optlen));
}

int	shutdown(int fd, int how)
{
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->shutdown == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	return __transform_errno(fdinfo->ops->shutdown(fdinfo->userdata, how));
}

int	getpeername(int fd, struct sockaddr *name, socklen_t *namelen)
{
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->getpeername == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	return __transform_errno(fdinfo->ops->getpeername(fdinfo->userdata, name, namelen));
}

int	getsockname(int fd, struct sockaddr *name, socklen_t *namelen)
{
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->getsockname == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	return __transform_errno(fdinfo->ops->getsockname(fdinfo->userdata, name, namelen));
}

u32 libcglue_inet_addr(const char *cp)
{
	if (_libcglue_fdman_inet_ops == NULL || _libcglue_fdman_inet_ops->inet_addr == NULL)
	{
		return 0;
	}

	return _libcglue_fdman_inet_ops->inet_addr(cp);
}

char *libcglue_inet_ntoa(const ip4_addr_t *addr)
{
	if (_libcglue_fdman_inet_ops == NULL || _libcglue_fdman_inet_ops->inet_ntoa == NULL)
	{
		return NULL;
	}

	return _libcglue_fdman_inet_ops->inet_ntoa(addr);
}

char *libcglue_inet_ntoa_r(const ip4_addr_t *addr, char *buf, int buflen)
{
	if (_libcglue_fdman_inet_ops == NULL || _libcglue_fdman_inet_ops->inet_ntoa_r == NULL)
	{
		return NULL;
	}

	return _libcglue_fdman_inet_ops->inet_ntoa_r(addr, buf, buflen);
}

int libcglue_inet_aton(const char *cp, ip4_addr_t *addr)
{
	if (_libcglue_fdman_inet_ops == NULL || _libcglue_fdman_inet_ops->inet_aton == NULL)
	{
		return 0;
	}

	return _libcglue_fdman_inet_ops->inet_aton(cp, addr);
}

