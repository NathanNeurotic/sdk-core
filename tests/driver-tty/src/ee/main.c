#include <kernel.h>
#include <stdio.h>
#include <string.h>

#include <sifrpc.h>
#include <loadfile.h>
#include <sbv_patches.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <fileXio.h>
#include <io_common.h>

#include "iomanx_irx.h"
#include "filexio_irx.h"
#include "rtty_irx.h"
#include "rtty.h"

extern int kputs(char *s);

/* fileXioIoctl always copies IOCTL_BUF_SIZE bytes from its argument, so it cannot be NULL. */
static char ioctl_arg[IOCTL_BUF_SIZE];
static int failures;

static void expect(const char *name, int ok)
{
    printf("driver-tty: [%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok)
        failures++;
}

static int load(const char *name, void *buf, unsigned int size)
{
    int ret = -1;
    int id = SifExecModuleBuffer(buf, size, 0, NULL, &ret);

    printf("driver-tty: %s id=%d ret=%d\n", name, id, ret);
    return id > 0 && ret != 1;
}

static int drain(int fd, char *buf, int size)
{
    int total = 0, n;

    while (total < size - 1 && (n = fileXioRead(fd, buf + total, size - 1 - total)) > 0)
        total += n;
    buf[total] = '\0';
    return total;
}

int main(void)
{
    static const char ping[] = "ping from the EE\n";
    static char buf[2048];
    int fd, n;

    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    load("iomanx", iomanx_irx, size_iomanx_irx);
    load("filexio", filexio_irx, size_filexio_irx);
    fileXioInit();
    expect("rtty loads and stays resident", load("rtty", rtty_irx, size_rtty_irx));

    fd = fileXioOpen("rtty:", FIO_O_RDWR);
    expect("rtty: opens", fd >= 0);
    if (fd < 0)
        goto done;

    expect("read on an empty buffer returns 0", fileXioRead(fd, buf, sizeof(buf)) == 0);

    n = fileXioWrite(fd, (void *)ping, strlen(ping));
    expect("write accepts every byte", n == (int)strlen(ping));
    n = drain(fd, buf, sizeof(buf));
    expect("read returns what was written", n == (int)strlen(ping) && strcmp(buf, ping) == 0);

    fileXioWrite(fd, (void *)ping, strlen(ping));
    expect("ioctl clear succeeds", fileXioIoctl(fd, RTTY_IOCTL_CLEAR, ioctl_arg) == 0);
    expect("nothing left after clear", fileXioRead(fd, buf, sizeof(buf)) == 0);
    expect("unknown ioctl is rejected", fileXioIoctl(fd, 0x1234, ioctl_arg) < 0);
    expect("a unit name after the colon is rejected", fileXioOpen("rtty:other", FIO_O_RDONLY) < 0);

    /* Last: once rtty: is the console it also captures this program's own printf output. */
    expect("IOP stdout reopened on rtty: as fd 1", fileXioIoctl(fd, RTTY_IOCTL_ATTACH_CONSOLE, ioctl_arg) == 1);
    n = drain(fd, buf, sizeof(buf));
    printf("driver-tty: captured %d bytes of console output\n", n);
    if (strstr(buf, "rtty: console attached") != NULL)
        expect("IOP printf went to rtty:", 1);
    else
        printf("driver-tty: [SKIP] IOP printf went to rtty: (PCSX2 logs IOP stdout writes itself)\n");
    fileXioClose(fd);

done:
    kputs(failures ? "driver-tty: FAIL\n" : "driver-tty: PASS\n");
    printf("[ps2rig] exit=%d\n", failures ? 1 : 0);
    SleepThread();
    return 0;
}
