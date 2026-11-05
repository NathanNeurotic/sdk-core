#define SYSCLIB_DISABLE_BUILTINS /* bzero's own definition below can't be redirected to __builtin_bzero - clang errors on redefining a builtin name, gcc doesn't */
#include <sysclib.h>

void bzero(void *s, size_t n)
{
	memset(s, 0, n);
}
