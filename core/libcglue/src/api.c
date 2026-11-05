#include <libcglue.h>

void libcglue_set_timezone(int timezone)
{
	ps2sdk_setTimezone(timezone);
}

void libcglue_set_daylight_saving(int daylightSaving)
{
	ps2sdk_setDaylightSaving(daylightSaving);
}

int libcglue_get_iop_fd(int fd)
{
	return ps2sdk_get_iop_fd(fd);
}

char *libcglue_get_iop_filename(int fd)
{
	return ps2sdk_get_iop_filename(fd);
}
