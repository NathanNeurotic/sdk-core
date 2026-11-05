#define SYSCLIB_DISABLE_BUILTINS /* bcmp's own definition below can't be redirected to __builtin_bcmp - clang errors on redefining a builtin name, gcc doesn't */
#include <sysclib.h>

int bcmp(const void *s1, const void *s2, size_t n)
{
	return memcmp(s1, s2, n);
}
