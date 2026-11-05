/**
 * @file
 * @ingroup core_libcglue
 * Timezone and fd helpers under their libcglue names. The ps2sdk_* names
 * declared in ps2sdkapi.h keep working.
 */
#ifndef LIBCGLUE_H
#define LIBCGLUE_H

#include <ps2sdkapi.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Sets the offset from UTC in minutes. */
extern void libcglue_set_timezone(int timezone);
/** Enables or disables daylight saving time. */
extern void libcglue_set_daylight_saving(int daylightSaving);
/** Returns the IOP file descriptor behind a user-visible fd. */
extern int libcglue_get_iop_fd(int fd);
/** Returns the IOP file name behind a user-visible fd. */
extern char *libcglue_get_iop_filename(int fd);

#ifdef __cplusplus
}
#endif

#endif
