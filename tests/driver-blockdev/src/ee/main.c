#include <kernel.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sifrpc.h>
#include <loadfile.h>
#include <sbv_patches.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#include "iomanx_irx.h"
#include "filexio_irx.h"
#include "bdm_irx.h"
#include "bdmfs_fatfs_irx.h"
#include "ramdisk_bd_irx.h"

extern int kputs(char *s);

static int failures;

static void expect(const char *name, int ok)
{
    printf("driver-blockdev: [%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok)
        failures++;
}

static int load(const char *name, void *buf, unsigned int size)
{
    int ret = -1;
    int id = SifExecModuleBuffer(buf, size, 0, NULL, &ret);

    printf("driver-blockdev: %s id=%d ret=%d\n", name, id, ret);
    return id > 0 && ret != 1;
}

static int mounted(const char *path)
{
    int fd = fileXioDopen(path);

    if (fd < 0)
        return 0;
    fileXioDclose(fd);
    return 1;
}

static int read_file(const char *path, char *buf, int size)
{
    int fd = fileXioOpen(path, FIO_O_RDONLY);
    int n;

    if (fd < 0)
        return fd;
    n = fileXioRead(fd, buf, size - 1);
    fileXioClose(fd);
    if (n >= 0)
        buf[n] = '\0';
    return n;
}

int main(void)
{
    static const char hello[] = "hello from the ramdisk\n";
    static const char note[]  = "written by the EE through fileXio\n";
    char buf[128];
    int fd, n;

    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    load("iomanx", iomanx_irx, size_iomanx_irx);
    load("filexio", filexio_irx, size_filexio_irx);
    fileXioInit();
    load("bdm", bdm_irx, size_bdm_irx);
    load("bdmfs_fatfs", bdmfs_fatfs_irx, size_bdmfs_fatfs_irx);
    expect("ramdisk_bd loads and stays resident", load("ramdisk_bd", ramdisk_bd_irx, size_ramdisk_bd_irx));

    for (int i = 0; i < 50 && !mounted("ram0:/"); i++)
        usleep(100 * 1000);
    expect("ram0: mounted", mounted("ram0:/"));

    n = read_file("ram0:/HELLO.TXT", buf, sizeof(buf));
    printf("driver-blockdev: HELLO.TXT read=%d\n", n);
    expect("HELLO.TXT holds the seeded text", n == (int)strlen(hello) && strcmp(buf, hello) == 0);

    fd = fileXioOpen("ram0:/NOTE.TXT", FIO_O_WRONLY | FIO_O_CREAT | FIO_O_TRUNC, 0666);
    n  = fd >= 0 ? fileXioWrite(fd, (void *)note, strlen(note)) : fd;
    if (fd >= 0)
        fileXioClose(fd);
    expect("NOTE.TXT written", n == (int)strlen(note));

    n = read_file("ram0:/NOTE.TXT", buf, sizeof(buf));
    expect("NOTE.TXT reads back the same bytes", n == (int)strlen(note) && strcmp(buf, note) == 0);

    n = read_file("ram0:/MISSING.TXT", buf, sizeof(buf));
    expect("a missing file fails to open", n < 0);

    kputs(failures ? "driver-blockdev: FAIL\n" : "driver-blockdev: PASS\n");
    printf("[ps2rig] exit=%d\n", failures ? 1 : 0);
    SleepThread();
    return 0;
}
