/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# (C)2001, Gustavo Scotti (gustavo@scotti.com)
# (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
# (c) 2023 Francisco Javier Trujillo Mata <fjtrujy@gmail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/

/**
 * @file
 * fdman.c - Manager for fd.
 */

#include <ps2sdkapi.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#define NEWLIB_PORT_AWARE
#include <fileio.h>
#include "iox_stat.h"

typedef struct _fio_file_info_
{
    // cppcheck-suppress unusedStructMember
    int fd;
    // cppcheck-suppress unusedStructMember
    char filename[];
} _fio_file_info_t;

_libcglue_fdman_path_ops_t __fio_fdman_path_ops;
_libcglue_fdman_fd_ops_t __fio_fdman_ops_file;
_libcglue_fdman_fd_ops_t __fio_fdman_ops_dir;

extern void __fioOpsInitializeImpl(void);

__attribute__((constructor))
static void __fioOpsInitialize(void)
{
    __fioOpsInitializeImpl();
}

#define IOP_O_RDONLY       0x0001
#define IOP_O_WRONLY       0x0002
#define IOP_O_RDWR         0x0003
#define IOP_O_DIROPEN      0x0008  // Internal use for dopen
#define IOP_O_NBLOCK       0x0010
#define IOP_O_APPEND       0x0100
#define IOP_O_CREAT        0x0200
#define IOP_O_TRUNC        0x0400
#define IOP_O_EXCL         0x0800
#define IOP_O_NOWAIT       0x8000

int __fioOpenHelper(_libcglue_fdman_fd_info_t *info, const char *buf, int flags, mode_t mode)
{
    int iop_flags = 0;
    int is_dir = 0;
    int iop_fd;

    // newlib flags differ from iop flags
    if ((flags & 3) == O_RDONLY) iop_flags |= IOP_O_RDONLY;
    if ((flags & 3) == O_WRONLY) iop_flags |= IOP_O_WRONLY;
    if ((flags & 3) == O_RDWR  ) iop_flags |= IOP_O_RDWR;
    if (flags & O_NONBLOCK)      iop_flags |= IOP_O_NBLOCK;
    if (flags & O_APPEND)        iop_flags |= IOP_O_APPEND;
    if (flags & O_CREAT)         iop_flags |= IOP_O_CREAT;
    if (flags & O_TRUNC)         iop_flags |= IOP_O_TRUNC;
    if (flags & O_EXCL)          iop_flags |= IOP_O_EXCL;
    //if (flags & O_???)           iop_flags |= IOP_O_NOWAIT;
    if (flags & O_DIRECTORY) {
        iop_flags |= IOP_O_DIROPEN;
        is_dir = 1;
    }

    iop_fd = is_dir ? fioDopen(buf) : fioOpen(buf, iop_flags);
    if (iop_fd >= 0) {
        _fio_file_info_t *userdata;
        int buf_len;

        buf_len = strlen(buf);
        userdata = malloc(sizeof(_fio_file_info_t) + buf_len + 1);
        if (userdata == NULL)
        {
            return -ENOMEM;
        }
        userdata->fd = iop_fd;
        memcpy(userdata->filename, buf, buf_len);
        userdata->filename[buf_len] = '\x00';
        info->userdata = (void *)userdata;
        info->ops = is_dir ? &__fio_fdman_ops_dir : &__fio_fdman_ops_file;
        return 0;
    }
    return iop_fd;
}

int __fioGetFdHelper(void *userdata)
{
    _fio_file_info_t *finfo;

    if (userdata == NULL)
    {
        return -EINVAL;
    }

    finfo = (_fio_file_info_t *)userdata;
    return finfo->fd;
}

char *__fioGetFilenameHelper(void *userdata)
{
    _fio_file_info_t *finfo;

    if (userdata == NULL)
    {
        return NULL;
    }

    finfo = (_fio_file_info_t *)userdata;
    return finfo->filename;
}

int __fioCloseHelper(void *userdata)
{
    int rv;
    int fd;

    fd = __fioGetFdHelper(userdata);
    if (fd < 0)
    {
        return fd;
    }

    rv = fioClose(fd);
    free(userdata);
    return rv;
}

int __fioDcloseHelper(void *userdata)
{
    int rv;
    int fd;

    fd = __fioGetFdHelper(userdata);
    if (fd < 0)
    {
        return fd;
    }

    rv = fioDclose(fd);
    free(userdata);
    return rv;
}

int __fioReadHelper(void *userdata, void *buf, int nbytes)
{
    int rv;
    int fd;

    fd = __fioGetFdHelper(userdata);
    if (fd < 0)
    {
        return fd;
    }

    rv = fioRead(fd, buf, nbytes);
    return rv;
}

int __fioLseekHelper(void *userdata, int offset, int whence)
{
    int rv;
    int fd;

    fd = __fioGetFdHelper(userdata);
    if (fd < 0)
    {
        return fd;
    }

    rv = fioLseek(fd, offset, whence);
    return rv;
}

int __fioWriteHelper(void *userdata, const void *buf, int nbytes)
{
    int rv;
    int fd;

    fd = __fioGetFdHelper(userdata);
    if (fd < 0)
    {
        return fd;
    }

    rv = fioWrite(fd, buf, nbytes);
    return rv;
}

int __fioIoctlHelper(void *userdata, int request, void *data)
{
    int rv;
    int fd;

    fd = __fioGetFdHelper(userdata);
    if (fd < 0)
    {
        return fd;
    }

    rv = fioIoctl(fd, request, data);
    return rv;
}

int __fioDreadHelper(void *userdata, struct dirent *dir)
{
    int rv;
    int fd;
    // The IOP DMAs the entry into this buffer, so it must own whole 64-byte cache lines.
    struct { io_dirent_t d; } __attribute__((aligned(64))) aligned_dirent;
    io_dirent_t *iodir = &aligned_dirent.d;

    // Took from io_dirent_t
    #define __MAXNAMLEN 256

    fd = __fioGetFdHelper(userdata);
    if (fd < 0)
    {
        return fd;
    }

    rv = fioDread(fd, iodir);
    if (rv < 0) {
        errno = ENOENT;
        return -1;
    }

    dir->d_fileno = rv; // TODO: This number should be in theory a unique number per file
    snprintf(dir->d_name, sizeof(dir->d_name), "%.*s", (int)(sizeof(iodir->name) - 1), iodir->name);
    dir->d_reclen = 0;
    switch (iodir->stat.mode & FIO_SO_IFMT) {
        case FIO_SO_IFLNK: dir->d_type = DT_LNK;     break;
        case FIO_SO_IFDIR: dir->d_type = DT_DIR;     break;
        case FIO_SO_IFREG: dir->d_type = DT_REG;     break;
        default:          dir->d_type = DT_UNKNOWN; break;
    }


    return rv;
}

int __fioLseekDirHelper(void *userdata, int offset, int whence)
{
    int i;
    int fd;
    char *filename;
    _fio_file_info_t *finfo;
    int uid;
    struct dirent dir;

    fd = __fioGetFdHelper(userdata);
    if (fd < 0)
    {
        return fd;
    }

    filename = __fioGetFilenameHelper(userdata);

    if (filename == NULL)
    {
        return -EINVAL;
    }

    if (userdata == NULL)
    {
        return -EINVAL;
    }

    finfo = (_fio_file_info_t *)userdata;

    if (whence != SEEK_SET)
    {
        return -EINVAL;
    }

    fioDclose(fd);
    uid = fioDopen(filename);
    finfo->fd = uid;
    for (i = 0; i < offset; i++) {
        __fioDreadHelper(userdata, &dir);
    }

    return offset;
}

int __libcglue_init_stdio(_libcglue_fdman_fd_info_t *info, int fd)
{
    _fio_file_info_t *userdata;
    int fnlen;
    const char *tty0_str = "tty0:";

    fnlen = strlen(tty0_str) + 1;

    userdata = malloc(sizeof(_fio_file_info_t) + fnlen);
    if (userdata == NULL)
    {
        return -ENOMEM;
    }
    userdata->fd = fd;
    memcpy(userdata->filename, tty0_str, fnlen - 1);
    userdata->filename[fnlen - 1] = '\x00';
    info->userdata = (void *)userdata;
    info->ops = &__fio_fdman_ops_file;
    return 0;
}

/** Inter-library helpers, default value */
int __fioMkdirHelper(const char *path, int mode)
{
    // Old fio mkdir has no mode argument
    (void)mode;

    return fioMkdir(path);
}

static time_t io_to_posix_time(const unsigned char *ps2time)
{
        struct tm tim;
        tim.tm_sec  = ps2time[1];
        tim.tm_min  = ps2time[2];
        tim.tm_hour = ps2time[3];
        tim.tm_mday = ps2time[4];
        tim.tm_mon  = ps2time[5] - 1;
        tim.tm_year = ((u16)ps2time[6] | ((u16)ps2time[7] << 8)) - 1900;
        return mktime(&tim);
}

static mode_t io_to_posix_mode(unsigned int ps2mode)
{
        mode_t posixmode = 0;
        if (ps2mode & FIO_SO_IFREG) posixmode |= S_IFREG;
        if (ps2mode & FIO_SO_IFDIR) posixmode |= S_IFDIR;
        if (ps2mode & FIO_SO_IROTH) posixmode |= S_IRUSR|S_IRGRP|S_IROTH;
        if (ps2mode & FIO_SO_IWOTH) posixmode |= S_IWUSR|S_IWGRP|S_IWOTH;
        if (ps2mode & FIO_SO_IXOTH) posixmode |= S_IXUSR|S_IXGRP|S_IXOTH;
        return posixmode;
}

static void __fill_stat(struct stat *stat, const io_stat_t *fiostat)
{
        stat->st_dev = 0;
        stat->st_ino = 0;
        stat->st_mode = io_to_posix_mode(fiostat->mode);
        stat->st_nlink = 0;
        stat->st_uid = 0;
        stat->st_gid = 0;
        stat->st_rdev = 0;
        stat->st_size = ((off_t)fiostat->hisize << 32) | (off_t)fiostat->size;
        stat->st_atime = io_to_posix_time(fiostat->atime);
        stat->st_mtime = io_to_posix_time(fiostat->mtime);
        stat->st_ctime = io_to_posix_time(fiostat->ctime);
        stat->st_blksize = 16*1024;
        stat->st_blocks = stat->st_size / 512;
}

int __fioGetstatHelper(const char *path, struct stat *buf)
{
    // The IOP DMAs the result into this buffer, so it must own whole 64-byte cache lines.
    struct { io_stat_t st; } __attribute__((aligned(64))) aligned_stat;
    io_stat_t *fiostat_p = &aligned_stat.st;

    if (strncmp(path, "tty", 3) == 0 && path[3] >= '0' && path[3] <= '9' && path[4] == ':')
    {
        memset(buf, 0, sizeof(struct stat));
        buf->st_mode = S_IFCHR;
        return 0;
    }

    if (fioGetstat(path, fiostat_p) < 0) {
        return -ENOENT;
    }

    __fill_stat(buf, fiostat_p);

    return 0;
}

int __attribute__((weak)) _open(const char *buf, int flags, ...);
int __attribute__((weak)) _unlink(const char *path);
int __attribute__((weak)) mkdir(const char *path, mode_t mode);
int __attribute__((weak)) rmdir(const char *path);
int __attribute__((weak)) _stat(const char *path, struct stat *buf);
int __attribute__((weak)) _fstat(int fd, struct stat *buf);
int __attribute__((weak)) _close(int fd);
int __attribute__((weak)) _read(int fd, void *buf, size_t nbytes);
off_t __attribute__((weak)) _lseek(int fd, off_t offset, int whence);
int __attribute__((weak)) _write(int fd, const void *buf, size_t nbytes);
int __attribute__((weak)) _ioctl(int fd, int request, void *data);
int __attribute__((weak)) _ps2sdk_ioctl(int fd, int request, void *data);
int __attribute__((weak)) getdents(int fd, void *dd_buf, int count);

void __fioOpsInitializeImpl(void)
{
    memset(&__fio_fdman_path_ops, 0, sizeof(__fio_fdman_path_ops));
    // cppcheck-suppress knownConditionTrueFalse
    if (&_open) __fio_fdman_path_ops.open = __fioOpenHelper;
    // cppcheck-suppress knownConditionTrueFalse
    if (&_unlink) __fio_fdman_path_ops.remove = fioRemove;
    // cppcheck-suppress knownConditionTrueFalse
    if (&mkdir) __fio_fdman_path_ops.mkdir = __fioMkdirHelper;
    // cppcheck-suppress knownConditionTrueFalse
    if (&rmdir) __fio_fdman_path_ops.rmdir = fioRmdir;
    // cppcheck-suppress knownConditionTrueFalse
    if ((&_stat) || (&_fstat)) __fio_fdman_path_ops.stat = __fioGetstatHelper;

    memset(&__fio_fdman_ops_file, 0, sizeof(__fio_fdman_ops_file));
    __fio_fdman_ops_file.getfd = __fioGetFdHelper;
    __fio_fdman_ops_file.getfilename = __fioGetFilenameHelper;
    // cppcheck-suppress knownConditionTrueFalse
    if (&_close) __fio_fdman_ops_file.close = __fioCloseHelper;
    // cppcheck-suppress knownConditionTrueFalse
    if (&_read) __fio_fdman_ops_file.read = __fioReadHelper;
    // cppcheck-suppress knownConditionTrueFalse
    if (&_lseek) __fio_fdman_ops_file.lseek = __fioLseekHelper;
    // cppcheck-suppress knownConditionTrueFalse
    if (&_write) __fio_fdman_ops_file.write = __fioWriteHelper;
    // cppcheck-suppress knownConditionTrueFalse
    if ((&_ioctl) || (&_ps2sdk_ioctl)) __fio_fdman_ops_file.ioctl = __fioIoctlHelper;

    memset(&__fio_fdman_ops_dir, 0, sizeof(__fio_fdman_ops_dir));
    __fio_fdman_ops_dir.getfd = __fioGetFdHelper;
    __fio_fdman_ops_dir.getfilename = __fioGetFilenameHelper;
    // cppcheck-suppress knownConditionTrueFalse
    if (&_close) __fio_fdman_ops_dir.close = __fioDcloseHelper;
    // cppcheck-suppress knownConditionTrueFalse
    if (&_lseek) __fio_fdman_ops_dir.lseek = __fioLseekDirHelper;
    // cppcheck-suppress knownConditionTrueFalse
    if (&getdents) __fio_fdman_ops_dir.dread = __fioDreadHelper;
}

extern _libcglue_fdman_path_ops_t * _ps2sdk_get_default_fdman_path_ops(void);
_libcglue_fdman_path_ops_t * __attribute__((weak)) _ps2sdk_get_default_fdman_path_ops(void)
{
    return &__fio_fdman_path_ops;
}

_libcglue_fdman_path_ops_t *_libcglue_fdman_path_ops = NULL;

__attribute__((constructor))
static void __libcglue_fdman_path_ops_initialize(void)
{
    _libcglue_fdman_path_ops = _ps2sdk_get_default_fdman_path_ops();
}

_libcglue_fdman_socket_ops_t *_libcglue_fdman_socket_ops = NULL;

/* sys/socket.h #defines ps2ip_getconfig/ps2ip_setconfig/dns_setserver/
 * dns_getserver to these four libcglue_-prefixed names for any real
 * caller (found for real getting Open-PS2-Loader and ps2_drivers to
 * build: both call the plain names directly) - the real backend side
 * of this dispatch (core/ps2ips's own ps2ipc_ps2sdk.c, which points
 * _libcglue_fdman_socket_ops at its own real RPC-client implementation
 * from _ps2sdk_ps2ipc_init()) already existed, but libcglue itself
 * never defined the trampoline functions callers actually link
 * against - a real, previously-latent gap, only ever exercised by a
 * program that both includes this header AND actually calls one of
 * these four names, which nothing in this tree's own samples did
 * until now. ENETDOWN (not ENOSYS) when nothing is registered - the
 * real, standard errno for "no network subsystem is up", matching
 * what these fail from is a *missing* ps2ipcInit()-style call, not an
 * unimplemented feature. */
int libcglue_ps2ip_setconfig(const t_ip_info *ip_info)
{
    if (_libcglue_fdman_socket_ops == NULL || _libcglue_fdman_socket_ops->setconfig == NULL)
    {
        errno = ENETDOWN;
        return -1;
    }
    return _libcglue_fdman_socket_ops->setconfig(ip_info);
}

int libcglue_ps2ip_getconfig(char *netif_name, t_ip_info *ip_info)
{
    if (_libcglue_fdman_socket_ops == NULL || _libcglue_fdman_socket_ops->getconfig == NULL)
    {
        errno = ENETDOWN;
        return -1;
    }
    return _libcglue_fdman_socket_ops->getconfig(netif_name, ip_info);
}

void libcglue_dns_setserver(u8 numdns, const ip_addr_t *dnsserver)
{
    if (_libcglue_fdman_socket_ops == NULL || _libcglue_fdman_socket_ops->dns_setserver == NULL)
    {
        return;
    }
    _libcglue_fdman_socket_ops->dns_setserver(numdns, dnsserver);
}

const ip_addr_t *libcglue_dns_getserver(u8 numdns)
{
    if (_libcglue_fdman_socket_ops == NULL || _libcglue_fdman_socket_ops->dns_getserver == NULL)
    {
        return NULL;
    }
    return _libcglue_fdman_socket_ops->dns_getserver(numdns);
}

_libcglue_fdman_inet_ops_t *_libcglue_fdman_inet_ops = NULL;
