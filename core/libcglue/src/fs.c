/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if __has_include(<sys/statvfs.h>)
#include <sys/statvfs.h>
#else
// Toolchains older than newlib-llvm's sys/statvfs.h; remove once every supported toolchain ships it.
struct statvfs {
	unsigned long f_bsize;
	unsigned long f_frsize;
	fsblkcnt_t f_blocks;
	fsblkcnt_t f_bfree;
	fsblkcnt_t f_bavail;
	fsfilcnt_t f_files;
	fsfilcnt_t f_ffree;
	fsfilcnt_t f_favail;
	unsigned long f_fsid;
	unsigned long f_flag;
	unsigned long f_namemax;
};

int statvfs(const char *path, struct statvfs *buf);
int fstatvfs(int fd, struct statvfs *buf);
#endif

extern int __path_absolute(const char *in, char *out, int len);

char *realpath(const char *path, char *resolved)
{
	char absolute[PATH_MAX];
	struct stat st;

	if (path == NULL || *path == '\0') {
		errno = path == NULL ? EINVAL : ENOENT;
		return NULL;
	}
	if (__path_absolute(path, absolute, sizeof(absolute)) < 0) {
		errno = ENAMETOOLONG;
		return NULL;
	}
	if (stat(absolute, &st) < 0)
		return NULL;

	if (resolved == NULL)
		return strdup(absolute);
	return strcpy(resolved, absolute);
}

int statvfs(const char *path, struct statvfs *buf)
{
	(void)path;
	(void)buf;
	errno = ENOSYS;
	return -1;
}

int fstatvfs(int fd, struct statvfs *buf)
{
	(void)fd;
	(void)buf;
	errno = ENOSYS;
	return -1;
}
