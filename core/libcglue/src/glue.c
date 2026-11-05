/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <kernel.h>
#include <timer.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sio.h>

#include <pwd.h>
#include <time.h>
#include <grp.h>
#include <sys/termios.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <sys/utime.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <limits.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/types.h>

// Include all integer types for compile time checking of:
// - compiler (gcc)
// - libc (newlib)
#include <stdint.h>
#include <limits.h>
#include <inttypes.h>
#include <tamtypes.h>

#define NEWLIB_PORT_AWARE
#include "io_common.h"
#include "ps2sdkapi.h"
#include "timer_alarm.h"
#include "fdman.h"

/* Functions from cwd.c */
extern char __cwd[MAXNAMLEN + 1];
extern size_t __cwd_len;
int __path_absolute(const char *in, char *out, int len);

extern void * _end;

char __dummy_passwd_loginbuf[16];
/* the present working directory variable. */
struct passwd __dummy_passwd;
__attribute__((constructor))
static void __dummy_passwd_init(void)
{
	strcpy(__dummy_passwd_loginbuf, "ps2user");
	__dummy_passwd.pw_name = &__dummy_passwd_loginbuf[0];
	__dummy_passwd.pw_passwd = "xxx";
	__dummy_passwd.pw_uid = 1000;
	__dummy_passwd.pw_gid = 1000;
	__dummy_passwd.pw_comment = "";
	__dummy_passwd.pw_gecos = "";
	__dummy_passwd.pw_dir = "/";
	__dummy_passwd.pw_shell = "";
}

static char *__dummy_group_members[2];
struct group __dummy_group;

__attribute__((constructor))
static void __dummy_group_init(void)
{
    __dummy_group.gr_name = "ps2group";
    __dummy_group.gr_passwd = "xxx";
    __dummy_group.gr_gid = 1000;
    __dummy_group.gr_mem = &__dummy_group_members[0];
    __dummy_group_members[0] = &__dummy_passwd_loginbuf[0];
    __dummy_group_members[1] = NULL;
}

int __transform_errno(int res) {
	/* On error, -1 is returned and errno is set to indicate the error. */
	if (res < 0) {
		errno = -res;
		return -1;
	}
	return res;
}

int64_t __transform64_errno(int64_t res) {
	/* On error, -1 is returned and errno is set to indicate the error. */
	if (res < 0) {
		errno = -res;
		return -1;
	}
	return res;
}

#if INT_MAX != 0x7fffffffL
	#error "INT_MAX != 0x7fffffffL"
#endif
#ifndef LONG_MAX
	#error "LONG_MAX not defined"
#endif
#if LONG_MAX != 0x7fffffffL
	#error "LONG_MAX != 0x7fffffffL"
#endif

#define ct_assert(e) {enum { ct_assert_value = 1/(!!(e)) };}
void compile_time_check() {
	// Compiler (ABI n32)
	ct_assert(sizeof(unsigned char)==1);
	ct_assert(sizeof(unsigned short)==2);
	ct_assert(sizeof(unsigned int)==4);
	ct_assert(sizeof(unsigned long)==4);
	ct_assert(sizeof(unsigned long long)==8);
	// clang ignores __attribute__((mode(TI))) applied inline to a type
	// used inside an expression (warns "mode attribute ignored", then the
	// sizeof stays 4, not 16) - it only takes effect on a typedef
	// declaration, same as tamtypes.h's own u128 (checked again below).
	typedef unsigned int __glue_ti_check_t __attribute__((mode(TI)));
	ct_assert(sizeof(__glue_ti_check_t)==16);
	ct_assert(sizeof(void *)==4);

	// Defined in tamtypes.h (ps2sdk)
	ct_assert(sizeof(u8)==1);
	ct_assert(sizeof(u16)==2);
	ct_assert(sizeof(u32)==4);
	ct_assert(sizeof(u64)==8);
	ct_assert(sizeof(u128)==16);

	// Defined in inttypes.h/stdint.h (newlib)
	ct_assert(sizeof(uint8_t)==1);
	ct_assert(sizeof(uint16_t)==2);
	ct_assert(sizeof(uint32_t)==4);
	ct_assert(sizeof(uint64_t)==8);
}

// Called from newlib openr.c
int _open(const char *buf, int flags, ...) {
	int iop_fd, fd;
	int mode;
	_libcglue_fdman_fd_info_t *info;
	va_list alist;
	char t_fname[MAXNAMLEN + 1];

	va_start(alist, flags);
	mode = va_arg(alist, int);	// Retrieve the mode argument, regardless of whether it is expected or not.
	va_end(alist);

	if(__path_absolute(buf, t_fname, sizeof(t_fname)) < 0) {
		errno = ENAMETOOLONG;
		return -1;
	}

	if (_libcglue_fdman_path_ops == NULL || _libcglue_fdman_path_ops->open == NULL)
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
	iop_fd = _libcglue_fdman_path_ops->open(info, t_fname, flags, mode);
	if (iop_fd < 0)
	{
		__fdman_release_descriptor(fd);
		return __transform_errno(iop_fd);
	}
	__descriptormap[fd]->flags = flags;
	return fd;
}

// Called from newlib wordexp.c, popen.c
int pipe(int fildes[2])
{
	errno = ENOSYS;
	return -1;
}

// Called from newlib closer.c
int _close(int fd) {
	int ret = 0;

	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	if (__descriptormap[fd]->ref_count == 1)
	{
		_libcglue_fdman_fd_info_t *fdinfo;

		fdinfo = &(__descriptormap[fd]->info);
		if (fdinfo->ops != NULL && fdinfo->ops->close != NULL)
		{
			ret = __transform_errno(fdinfo->ops->close(fdinfo->userdata));
		}
	}
	__fdman_release_descriptor(fd);
	return ret;
}

// Called from newlib readr.c
int _read(int fd, void *buf, size_t nbytes) {
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->read == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	return __transform_errno(fdinfo->ops->read(fdinfo->userdata, buf, nbytes));
}

// Called from newlib writer.c
int _write(int fd, const void *buf, size_t nbytes) {
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->write == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	return __transform_errno(fdinfo->ops->write(fdinfo->userdata, buf, nbytes));
}

// Called from newlib statr.c
int _stat(const char *path, struct stat *buf) {
	char dest[MAXNAMLEN + 1];

	if(__path_absolute(path, dest, sizeof(dest)) < 0) {
		errno = ENAMETOOLONG;
		return -1;
	}

	if (_libcglue_fdman_path_ops == NULL || _libcglue_fdman_path_ops->stat == NULL)
	{
		errno = ENOSYS;
		return -1;
	}

	return __transform_errno(_libcglue_fdman_path_ops->stat(dest, buf));
}

// Called from newlib glob.c, nftw.c
int lstat(const char *path, struct stat *buf) {
	return stat(path, buf);
}

// Called from newlib fstatr.c
int _fstat(int fd, struct stat *buf) {
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;
	char *filename;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->getfilename == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	filename = fdinfo->ops->getfilename(fdinfo->userdata);
	if (filename == NULL)
	{
		errno = ENOENT;
		return -1;
	}

	if (_libcglue_fdman_path_ops == NULL || _libcglue_fdman_path_ops->stat == NULL)
	{
		errno = ENOSYS;
		return -1;
	}

	return __transform_errno(_libcglue_fdman_path_ops->stat(filename, buf));
}

// Called from newlib nftw.c
int access(const char *fn, int flags) {
	struct stat s;
	if (stat(fn, &s))
		return -1;
	if (s.st_mode & S_IFDIR)
		return 0;
	if (flags & W_OK) {
		if (s.st_mode & S_IWRITE)
			return 0;
		errno = EACCES;
		return -1;
	}
	return 0;
}

// Called from newlib fcntlr.c
int _fcntl(int fd, int cmd, ...)
{
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	switch (cmd)
	{
		case F_DUPFD:
		{
			return __fdman_get_dup_descriptor(fd);
			break;
		}
		case F_GETFL:
		{
			return __descriptormap[fd]->flags;
			break;
		}
		case F_SETFL:
		{
			int newfl, rv;
			va_list args;
	
			rv = 0;

			va_start (args, cmd);         /* Initialize the argument list. */
			newfl =  va_arg(args, int);
			va_end (args);                /* Clean up. */

			__descriptormap[fd]->flags = newfl;

			{
				_libcglue_fdman_fd_info_t *fdinfo;

				fdinfo = &(__descriptormap[fd]->info);
				
				if (fdinfo->ops != NULL && fdinfo->ops->fcntl_f_setfl != NULL)
				{
					rv = __transform_errno(fdinfo->ops->fcntl_f_setfl(fdinfo->userdata, newfl));
				}
			}
			return rv;
		}
		case F_SETFD:
		{
			int newfl;
			va_list args;
	
			va_start (args, cmd);         /* Initialize the argument list. */
			newfl =  va_arg(args, int);
			va_end (args);                /* Clean up. */

			__descriptormap[fd]->flags = newfl;
			return 0;
			break;
		}
	}

	errno = EBADF;
	return -1;
}

// This is actually not called from newlib, but the _ioctl symbol is checked by
// ps2sdkapi.c and fileXio_ps2sdkapi.c. Perhaps it was later renamed to _ps2sdk_ioctl?
// For consistency, _ioctl is implemented as an errno alternative to _ps2sdk_ioctl.
int _ioctl(int fd, int request, void *data) {
	_libcglue_fdman_fd_info_t *fdinfo;
	fdinfo = libcglue_get_fd_info(fd);
	if (fdinfo == NULL)
	{
		errno = EBADF;
		return -1;
	}
	if (fdinfo->ops == NULL || fdinfo->ops->ioctl == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	return __transform_errno(fdinfo->ops->ioctl(fdinfo->userdata, request, data));
}

// Called from newlib readdir.c, readdir_r.c
int getdents(int fd, void *dd_buf, int count)
{
	struct dirent *dirp;
	int rv, read;

	read = 0;
	dirp = (struct dirent *)dd_buf;

	{
		_libcglue_fdman_fd_info_t *fdinfo;

		fdinfo = &(__descriptormap[fd]->info);
		rv = -ENOSYS;
		if (fdinfo->ops != NULL && fdinfo->ops->dread != NULL)
		{
			rv = __transform_errno(fdinfo->ops->dread(fdinfo->userdata, dirp));
		}
	}
	if (rv < 0) {
		return __transform_errno(rv);
	} else if (rv == 0) {
		return read;
	}

	read += sizeof(struct dirent);	
	dirp->d_reclen = count;

	return read;
}

// Called from newlib lseekr.c
off_t _lseek(int fd, off_t offset, int whence)
{
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->lseek == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	return __transform_errno(fdinfo->ops->lseek(fdinfo->userdata, offset, whence));
}

off64_t lseek64(int fd, off64_t offset, int whence)
{
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	_libcglue_fdman_fd_info_t *fdinfo;

	fdinfo = &(__descriptormap[fd]->info);
	if (fdinfo->ops == NULL || fdinfo->ops->lseek64 == NULL)
	{
		errno = ENOSYS;
		return -1;
	}
	return __transform64_errno(fdinfo->ops->lseek64(fdinfo->userdata, offset, whence));
}

int chdir(const char *path) {
	char dest[MAXNAMLEN + 1];

	if(__path_absolute(path, dest, sizeof(dest)) < 0) {
		errno = ENAMETOOLONG;
		return -1;
	}

	__cwd_len = snprintf(__cwd, sizeof(__cwd), "%.*s", sizeof(dest) - 1, dest);
	__cwd_len = (__cwd_len > (sizeof(__cwd) - 1)) ? (sizeof(__cwd) - 1) : __cwd_len;
	return 0;
}

int fchdir(int fd)
{
	errno = ENOSYS;
	return -1;
}

// Called from newlib mkdirr.c
int _mkdir(const char *path, mode_t mode)
{
	char dest[MAXNAMLEN + 1];

	if(__path_absolute(path, dest, sizeof(dest)) < 0) {
		errno = ENAMETOOLONG;
		return -1;
	}

	if (_libcglue_fdman_path_ops == NULL || _libcglue_fdman_path_ops->mkdir == NULL)
	{
		errno = ENOSYS;
		return -1;
	}

	return __transform_errno(_libcglue_fdman_path_ops->mkdir(dest, mode));
}

int mkdir(const char *path, mode_t mode)
{
	return _mkdir(path, mode);
}

int rmdir(const char *path) {
	char dest[MAXNAMLEN + 1];

	if(__path_absolute(path, dest, sizeof(dest)) < 0) {
		errno = ENAMETOOLONG;
		return -1;
	}

	if (_libcglue_fdman_path_ops == NULL || _libcglue_fdman_path_ops->rmdir == NULL)
	{
		errno = ENOSYS;
		return -1;
	}

	return __transform_errno(_libcglue_fdman_path_ops->rmdir(dest));
}

// Called from newlib linkr.c
int _link(const char *old, const char *new_) {
	errno = ENOSYS;
	return -1; /* not supported */
}

// Called from newlib unlinkr.c
int _unlink(const char *path) {
	char dest[MAXNAMLEN + 1];
	if(__path_absolute(path, dest, sizeof(dest)) < 0) {
		errno = ENAMETOOLONG;
		return -1;
	}

	if (_libcglue_fdman_path_ops == NULL || _libcglue_fdman_path_ops->remove == NULL)
	{
		errno = ENOSYS;
		return -1;
	}

	return __transform_errno(_libcglue_fdman_path_ops->remove(dest));
}

// Called from newlib renamer.c
int _rename(const char *old, const char *new_) {
	char oldname[MAXNAMLEN + 1];
	char newname[MAXNAMLEN + 1];

	if(__path_absolute(old, oldname, sizeof(oldname)) < 0) {
		errno = ENAMETOOLONG;
		return -1;
	}

	if(__path_absolute(new_, newname, sizeof(newname)) < 0) {
		errno = ENAMETOOLONG;
		return -1;
	}

	if (_libcglue_fdman_path_ops == NULL || _libcglue_fdman_path_ops->rename == NULL)
	{
		errno = ENOSYS;
		return -1;
	}

	return __transform_errno(_libcglue_fdman_path_ops->rename(oldname, newname));
}

int pause(void)
{
	errno = ENOSYS;
	return -1;
}

int getitimer(int which, struct itimerval *value)
{
	errno = ENOSYS;
	return -1;
}

int setitimer(int which, const struct itimerval *value, struct itimerval *ovalue)
{
	errno = ENOSYS;
	return -1;
}

int sched_yield(void)
{
	return 0;
}

// Called from newlib signalr.c
int _getpid(void) {
	return GetThreadId();
}

pid_t getppid(void)
{
	errno = ENOSYS;
	return (pid_t) -1;
}

// Called from newlib signalr.c
int _kill(int pid, int sig) {
#if 0 // needs to be tested first
	// null signal: do error checking on pid only
	if (sig == 0)
		return pid == getpid() ? 0 : -1;

	if (pid == getpid()) {
		ExitDeleteThread();
		// NOTE: ExitDeleteThread does not return
		return 0;
	}
#endif
	(void)pid;
	(void)sig;
	errno = ENOSYS;
	return -1; /* not supported */
}

// Called from newlib hash_page.c
int sigprocmask(int how, const sigset_t *set, sigset_t *oset)
{
	errno = ENOSYS;
	return -1; /* not supported */
}

int sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
	errno = ENOSYS;
	return -1; /* not supported */
}

// Called from newlib execr.c
pid_t _fork(void) {
	errno = ENOSYS;
	return (pid_t) -1; /* not supported */
}

// Called from newlib popen.c
pid_t vfork(void)
{
	errno = ENOSYS;
	return (pid_t) -1; /* not supported */
}

// Called from newlib execr.c
pid_t _wait(int *unused) {
	errno = ENOSYS;
	return (pid_t) -1; /* not supported */
}

// Called from newlib wordexp.c, popen.c
pid_t waitpid(pid_t pid, int *stat_loc, int options)
{
	errno = ENOSYS;
	return (pid_t) -1; /* not supported */
}

// Called from newlib execr.c, execl.c, execle.c, execv.c, execve.c
int _execve(const char *name, char *const argv[], char *const env[]) {
	errno = ENOSYS;
	return (pid_t) -1; /* not supported */
}

int _system(const char *command)
{
	if (!command)
		return 0;
	errno = ENOSYS;
	return -1;
}

// Called from newlib sbrkr.c
void * _sbrk(size_t incr) {
	static void * _heap_ptr = &_end;
	void *mp, *ret = (void *)-1;

	if (incr == 0)
		return _heap_ptr;

	/* If the area we want to allocated is past the end of our heap, we have a problem. */
	mp = _heap_ptr + incr;
	if (mp <= EndOfHeap()) {
		ret = _heap_ptr;
		_heap_ptr = mp;
	}

	return ret;
}

// Called from newlib gettimeofdayr.c
int _gettimeofday(struct timeval *tv, struct timezone *tz)
{
	if (tv == NULL)
	{
		errno = EFAULT;
		return -1;
	}

	{
		u32 busclock_sec;
		u32 busclock_usec;

		TimerBusClock2USec(GetTimerSystemTime(), &busclock_sec, &busclock_usec);
		tv->tv_sec = (time_t)(_libcglue_rtc_get_offset_from_busclk() + ((s64)busclock_sec));
		tv->tv_usec = busclock_usec;
	}

	if (tz != NULL)
	{
		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = 0;
	}

	return 0;
}

// Called from newlib timesr.c
clock_t _times(struct tms *buffer) {
	clock_t clk;
	u32 busclock_sec;
	u32 busclock_usec;

	TimerBusClock2USec(GetTimerSystemTime(), &busclock_sec, &busclock_usec);
	clk = busclock_sec * CLOCKS_PER_SEC + busclock_usec;

	if (buffer != NULL) {
		buffer->tms_utime  = clk;
		buffer->tms_stime  = 0;
		buffer->tms_cutime = 0;
		buffer->tms_cstime = 0;
	}

	return clk;
}

int ftime(struct timeb *tb) {
	struct timeval tv;
	struct timezone tz;

	gettimeofday(&tv, &tz);

	tb->time = tv.tv_sec;
	tb->millitm = tv.tv_usec / 1000;
	tb->timezone = tz.tz_minuteswest;
	tb->dstflag = tz.tz_dsttime;

	return 0;
}

int clock_getres(clockid_t clk_id, struct timespec *res) {
	struct timeval tv;
	int ret;

	ret = gettimeofday(&tv, NULL);

	/* Return the actual time since epoch */
	res->tv_sec = tv.tv_sec;
	res->tv_nsec = tv.tv_usec * 1000;

	return ret;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
	struct timeval tv;
	int res;

	res = gettimeofday(&tv, NULL);

	/* Return the actual time since epoch */
	tp->tv_sec = tv.tv_sec;
	tp->tv_nsec = tv.tv_usec * 1000;

	return res;
}

int clock_settime(clockid_t clk_id, const struct timespec *tp) {
	// TODO: implement using sceCdWriteClock
	errno = EPERM;
	return -1;
}

__attribute__((weak))
ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
	int i;
	ssize_t size_sum;

	size_sum = 0;
	for (i = 0; i < iovcnt; i += 1)
	{
		ssize_t size_cur;

		size_cur = read(fd, iov[i].iov_base, iov[i].iov_len);
		if (size_cur < 0)
			return size_cur;
		size_sum += size_cur;
		if (size_cur != iov[i].iov_len)
			break;
	}
	return size_sum;
}

__attribute__((weak))
ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
	int i;
	ssize_t size_sum;

	size_sum = 0;
	for (i = 0; i < iovcnt; i += 1)
	{
		ssize_t size_cur;

		size_cur = write(fd, iov[i].iov_base, iov[i].iov_len);
		if (size_cur < 0)
			return size_cur;
		size_sum += size_cur;
		if (size_cur != iov[i].iov_len)
			break;
	}
	return size_sum;
}

int truncate(const char *path, off_t length)
{
	ssize_t bytes_read;
	int fd, res;
	char buff[length];

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		errno = ENOENT;
		return -1;
	}

	bytes_read = read(fd, &buff, length);
	close(fd);
	if (bytes_read < length) {
		errno = EFBIG;
		return -1;
	}

	fd = open (path, O_TRUNC|O_WRONLY);
	if (fd < 0) {
		errno = ENOENT;
		return -1;
	}

	res = write(fd, &buff, length);
	close(fd);
	return res;
}

int ftruncate(int fd, off_t length)
{
	errno = ENOSYS;
	return -1; /* not supported */
}

int _symlink(const char *target, const char *linkpath)
{
	char dest_target[MAXNAMLEN + 1];
	char dest_linkpath[MAXNAMLEN + 1];

	if(__path_absolute(target, dest_target, sizeof(dest_target)) < 0) {
		errno = ENAMETOOLONG;
		return -1;
	}

	if(__path_absolute(linkpath, dest_linkpath, sizeof(dest_linkpath)) < 0) {
		errno = ENAMETOOLONG;
		return -1;
	}

	if (_libcglue_fdman_path_ops == NULL || _libcglue_fdman_path_ops->symlink == NULL)
	{
		errno = ENOSYS;
		return -1;
	}

	return __transform_errno(_libcglue_fdman_path_ops->symlink(dest_target, dest_linkpath));
}

int symlink(const char *target, const char *linkpath)
{
	return _symlink(target, linkpath);
}

ssize_t _readlink(const char *path, char *buf, size_t bufsiz)
{
	char dest[MAXNAMLEN + 1];

	if(__path_absolute(path, dest, sizeof(dest)) < 0) {
		errno = ENAMETOOLONG;
		return -1;
	}

	if (_libcglue_fdman_path_ops == NULL || _libcglue_fdman_path_ops->readlink == NULL)
	{
		errno = ENOSYS;
		return -1;
	}

	return __transform_errno(_libcglue_fdman_path_ops->readlink(dest, buf, bufsiz));
}

ssize_t readlink(const char *path, char *buf, size_t bufsiz)
{
	return _readlink(path, buf, bufsiz);
}

int utime(const char *pathname, const struct utimbuf *times)
{
	// TODO: implement in terms of chstat
	errno = ENOSYS;
	return -1; /* not supported */
}

int utimes(const char *filename, const struct timeval times[2])
{
	// TODO: implement in terms of chstat
	errno = ENOSYS;
	return -1; /* not supported */
}

int fchown(int fd, uid_t owner, gid_t group)
{
	errno = ENOSYS;
	return -1; /* not supported */
}

ssize_t getrandom(void *buf, size_t buflen, unsigned int flags)
{
	(void)flags;

	arc4random_buf(buf, buflen);
	return buflen;
}

// Called from newlib getentropyr.c
int _getentropy(void *buf, size_t buflen)
{
	u8 *buf_cur = buf;
	int i;
	// Restrict buffer size as documented in the man page
	if (buflen > 256)
	{
		errno = EIO;
		return -1;
	}
	// TODO: get proper entropy from e.g.
	// * RTC
	// * uninitialized memory
	// * Mechacon temperature
	for (i = 0; i < buflen; i += 1)
	{
		// Performance counter low buts should be changed for each call to cpu_ticks
		buf_cur[i] = (u8)(cpu_ticks() & 0xff);
	}
	return 0;
}

mode_t umask(mode_t mask)
{
	return 0;
}

int chmod(const char *pathname, mode_t mode)
{
	errno = ENOSYS;
	return -1; /* not supported */
}

int fchmod(int fd, mode_t mode)
{
	errno = ENOSYS;
	return -1; /* not supported */
}

int _chown(const char *path, uid_t owner, gid_t group)
{
	errno = ENOSYS;
	return -1; /* not supported */
}

int chown(const char *path, uid_t owner, gid_t group)
{
	return _chown(path, owner, group);
}

static long __pathconf_value(int name)
{
	switch (name) {
	case _PC_NAME_MAX:
		return MAXNAMLEN;
	case _PC_PATH_MAX:
		return PATH_MAX;
	default:
		errno = EINVAL;
		return -1;
	}
}

long pathconf(const char *path, int name)
{
	return __pathconf_value(name);
}

long fpathconf(int fd, int name)
{
	return __pathconf_value(name);
}

int fsync(int fd) {
	errno = ENOSYS;
	return -1; /* not supported */
}

long sysconf(int name)
{
	errno = EINVAL;
	return -1;
}

int tcgetattr(int fd, struct termios *tp)
{
	errno = ENOSYS;
	return -1;
}

int tcsetattr(int fd, int opts, const struct termios *tp)
{
	errno = ENOSYS;
	return -1;
}

// Called from newlib glob.c
char *getlogin(void)
{
	return __dummy_passwd.pw_name;
}

// Called from newlib glob.c
uid_t getuid(void)
{
	return __dummy_passwd.pw_uid;
}

uid_t geteuid(void)
{
	return __dummy_passwd.pw_uid;
}

gid_t getgid(void)
{
	return __dummy_passwd.pw_gid;
}

gid_t getegid(void)
{
	return __dummy_passwd.pw_gid;
}

// Called from newlib glob.c
struct passwd *getpwuid(uid_t uid) {
	/* There's no support for users */
	return &__dummy_passwd;
}

// Called from newlib glob.c
struct passwd *getpwnam(const char *name) {
	/* There's no support for users */
	return &__dummy_passwd;
}

// Called from newlib glob.c
int issetugid(void)
{
	return 0;
}

struct group *getgrgid(gid_t gid)
{
	return &__dummy_group;
}

struct group *getgrnam(const char *nam)
{
	return &__dummy_group;
}

_libcglue_fdman_fd_info_t *libcglue_get_fd_info(int fd) {
	if (!__IS_FD_VALID(fd)) {
		errno = EBADF;
		return NULL;
	}

	return &(__descriptormap[fd]->info);
}

int ps2sdk_get_iop_fd(int fd)
{
	_libcglue_fdman_fd_info_t *fdinfo;
	fdinfo = libcglue_get_fd_info(fd);
	if (fdinfo == NULL)
	{
		return -EBADF;
	}
	if (fdinfo->ops == NULL || fdinfo->ops->getfd == NULL)
	{
		return -ENOSYS;
	}
	return fdinfo->ops->getfd(fdinfo->userdata);
}

char *ps2sdk_get_iop_filename(int fd)
{
	_libcglue_fdman_fd_info_t *fdinfo;
	fdinfo = libcglue_get_fd_info(fd);
	if (fdinfo == NULL)
	{
		return NULL;
	}
	if (fdinfo->ops == NULL || fdinfo->ops->getfilename == NULL)
	{
		return NULL;
	}
	return fdinfo->ops->getfilename(fdinfo->userdata);
}

int _ps2sdk_close(int fd)
{
	_libcglue_fdman_fd_info_t *fdinfo;
	fdinfo = libcglue_get_fd_info(fd);
	if (fdinfo == NULL)
	{
		return -EBADF;
	}
	if (fdinfo->ops == NULL || fdinfo->ops->close == NULL)
	{
		return -ENOSYS;
	}
	return fdinfo->ops->close(fdinfo->userdata);
}

int _ps2sdk_dclose(int fd)
{
	return _ps2sdk_close(fd);
}

int _ps2sdk_read(int fd, void *buf, int nbytes)
{
	_libcglue_fdman_fd_info_t *fdinfo;
	fdinfo = libcglue_get_fd_info(fd);
	if (fdinfo == NULL)
	{
		return -EBADF;
	}
	if (fdinfo->ops == NULL || fdinfo->ops->read == NULL)
	{
		return -ENOSYS;
	}
	return fdinfo->ops->read(fdinfo->userdata, buf, nbytes);
}

int _ps2sdk_lseek(int fd, int offset, int whence)
{
	_libcglue_fdman_fd_info_t *fdinfo;
	fdinfo = libcglue_get_fd_info(fd);
	if (fdinfo == NULL)
	{
		return -EBADF;
	}
	if (fdinfo->ops == NULL || fdinfo->ops->lseek == NULL)
	{
		return -ENOSYS;
	}
	return fdinfo->ops->lseek(fdinfo->userdata, offset, whence);
}

int64_t _ps2sdk_lseek64(int fd, int64_t offset, int whence)
{
	_libcglue_fdman_fd_info_t *fdinfo;
	fdinfo = libcglue_get_fd_info(fd);
	if (fdinfo == NULL)
	{
		return -EBADF;
	}
	if (fdinfo->ops == NULL || fdinfo->ops->lseek64 == NULL)
	{
		return -ENOSYS;
	}
	return fdinfo->ops->lseek64(fdinfo->userdata, offset, whence);
}

int _ps2sdk_write(int fd, const void *buf, int nbytes)
{
	_libcglue_fdman_fd_info_t *fdinfo;
	fdinfo = libcglue_get_fd_info(fd);
	if (fdinfo == NULL)
	{
		return -EBADF;
	}
	if (fdinfo->ops == NULL || fdinfo->ops->write == NULL)
	{
		return -ENOSYS;
	}
	return fdinfo->ops->write(fdinfo->userdata, buf, nbytes);
}

int _ps2sdk_ioctl(int fd, int request, void *data)
{
	_libcglue_fdman_fd_info_t *fdinfo;
	fdinfo = libcglue_get_fd_info(fd);
	if (fdinfo == NULL)
	{
		return -EBADF;
	}
	if (fdinfo->ops == NULL || fdinfo->ops->ioctl == NULL)
	{
		return -ENOSYS;
	}
	return fdinfo->ops->ioctl(fdinfo->userdata, request, data);
}

int _ps2sdk_ioctl2(int fd, int request, void *arg, unsigned int arglen, void *buf, unsigned int buflen)
{
	_libcglue_fdman_fd_info_t *fdinfo;
	fdinfo = libcglue_get_fd_info(fd);
	if (fdinfo == NULL)
	{
		return -EBADF;
	}
	if (fdinfo->ops == NULL || fdinfo->ops->ioctl == NULL)
	{
		return -ENOSYS;
	}
	return fdinfo->ops->ioctl2(fdinfo->userdata, request, arg, arglen, buf, buflen);
}

int _ps2sdk_dread(int fd, struct dirent *dir)
{
	_libcglue_fdman_fd_info_t *fdinfo;
	fdinfo = libcglue_get_fd_info(fd);
	if (fdinfo == NULL)
	{
		return -EBADF;
	}
	if (fdinfo->ops == NULL || fdinfo->ops->dread == NULL)
	{
		return -ENOSYS;
	}
	return fdinfo->ops->dread(fdinfo->userdata, dir);
}

/* ATFILE functions */

int openat(int dirfd, const char *pathname, int flags, ...)
{
	// TODO: Do better implementation following https://linux.die.net/man/2/openat
	// for now use the same as open
	
	// Extract mode from variable arguments
    va_list args;
    va_start(args, flags);

    // Get the mode argument
    int mode = va_arg(args, int);

    // Clean up the va_list
    va_end(args);
	return open(pathname, flags, mode);
}

int renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath)
{
	// TODO: Do better implementation following https://linux.die.net/man/2/renameat
	// for now use the same as rename
	return rename(oldpath, newpath);
}

int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags)
{
	// TODO: Do better implementation following https://linux.die.net/man/2/fchmodat
	// for now use the same as chmod
	return chmod(pathname, mode);
}

int fstatat(int dirfd, const char *pathname, struct stat *buf, int flags)
{
	// TODO: Do better implementation following https://linux.die.net/man/2/fstatat
	// for now use the same as stat
	return stat(pathname, buf);
}

int mkdirat(int dirfd, const char *pathname, mode_t mode)
{
	// TODO: Do better implementation following https://linux.die.net/man/2/mkdirat
	// for now use the same as mkdir
	return mkdir(pathname, mode);
}

int faccessat(int dirfd, const char *pathname, int mode, int flags)
{
	// TODO: Do better implementation following https://linux.die.net/man/2/faccessat
	// for now use the same as access
	return access(pathname, mode);
}

int fchownat(int dirfd, const char *pathname, uid_t owner, gid_t group, int flags)
{
	// TODO: Do better implementation following https://linux.die.net/man/2/fchownat
	// for now use the same as chown
	return chown(pathname, owner, group);
}

int linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags) {
	// TODO: Do better implementation following https://linux.die.net/man/2/linkat
	// for now use the same as link
	return link(oldpath, newpath);
}

int readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz)
{
	// TODO: Do better implementation following https://linux.die.net/man/2/linkat
	// for now use the same as readlink
	return readlink(pathname, buf, bufsiz);
}

int unlinkat(int dirfd, const char *pathname, int flags)
{
	// If flags contains AT_REMOVEDIR, then the path refers to a directory.
	// Otherwise, the path refers to a file.
	if (flags & AT_REMOVEDIR) {
		return rmdir(pathname);
	}
	else {
		return unlink(pathname);
	}
}

int dup(int oldfd)
{
	if (!__IS_FD_VALID(oldfd)) {
		errno = EBADF;
		return -1;
	}

	return __fdman_get_dup_descriptor(oldfd);
}

// Called from wordexp.c, popen.c
int dup2(int oldfd, int newfd)
{
	if (!__IS_FD_VALID(oldfd)) {
		errno = EBADF;
		return -1;
	}

	if (oldfd == newfd) {
		return oldfd;
	}
	if (newfd < 0) {
		errno = EBADF;
		return -1;
	}
	if (__descriptormap[newfd]) {
		close(newfd);
	}
	return __fdman_get_dup2_descriptor(oldfd, newfd);
}
