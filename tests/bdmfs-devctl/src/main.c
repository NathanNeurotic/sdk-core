#include <kernel.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include <sifrpc.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <sbv_patches.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>

#include "iomanx_irx.h"
#include "filexio_irx.h"
#include "usbd_irx.h"
#include "bdm_irx.h"
#include "bdmfs_fatfs_irx.h"
#include "usbmass_bd_irx.h"

#define DEVCTL_STOP_UNIT       0x0000
#define DEVCTL_STOP_ALL        0x0001
#define DEVCTL_GET_SECTOR_SIZE 0x0002
#define DEVCTL_GET_TOTAL_SIZE  0x0003
#define DEVCTL_GET_FREE_SIZE   0x0004

extern int kputs(char *s);

static int failures;

static void expect(const char *name, int ok)
{
    printf("devctl-test: [%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok)
        failures++;
}

static void load(const char *name, void *buf, unsigned int size)
{
    int ret = -1;
    int id = SifExecModuleBuffer(buf, size, 0, NULL, &ret);

    printf("devctl-test: %s id=%d ret=%d\n", name, id, ret);
}

static int mounted(const char *path)
{
    int fd = fileXioDopen(path);

    if (fd < 0)
        return 0;
    fileXioDclose(fd);
    return 1;
}

int main(void)
{
    uint32_t sector = 0;
    uint64_t total = 0, free_bytes = 0;
    int ret;

    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    load("iomanx", iomanx_irx, size_iomanx_irx);
    load("filexio", filexio_irx, size_filexio_irx);
    fileXioInit();
    load("usbd", usbd_irx, size_usbd_irx);
    load("bdm", bdm_irx, size_bdm_irx);
    load("bdmfs_fatfs", bdmfs_fatfs_irx, size_bdmfs_fatfs_irx);
    load("usbmass_bd", usbmass_bd_irx, size_usbmass_bd_irx);

    for (int i = 0; i < 100 && !mounted("mass0:/"); i++)
        usleep(100 * 1000);
    expect("mass0: mounted", mounted("mass0:/"));

    ret = fileXioDevctl("mass0:", DEVCTL_GET_SECTOR_SIZE, NULL, 0, &sector, sizeof(sector));
    printf("devctl-test: sector size ret=%d value=%u\n", ret, (unsigned)sector);
    expect("sector size is 512", ret == 0 && sector == 512);

    ret = fileXioDevctl("mass0:", DEVCTL_GET_TOTAL_SIZE, NULL, 0, &total, sizeof(total));
    printf("devctl-test: total ret=%d bytes=%u (%u MiB)\n", ret, (unsigned)total, (unsigned)(total >> 20));
    expect("total size is a non-zero multiple of 512", ret == 0 && total > 0 && total % 512 == 0);

    ret = fileXioDevctl("mass0:", DEVCTL_GET_FREE_SIZE, NULL, 0, &free_bytes, sizeof(free_bytes));
    printf("devctl-test: free ret=%d bytes=%u (%u MiB)\n", ret, (unsigned)free_bytes, (unsigned)(free_bytes >> 20));
    expect("free size is non-zero and not above total", ret == 0 && free_bytes > 0 && free_bytes <= total);

    ret = fileXioDevctl("mass0:", DEVCTL_GET_TOTAL_SIZE, NULL, 0, &total, 4);
    expect("short output buffer rejected", ret < 0);
    ret = fileXioDevctl("mass0:", 0x00FF, NULL, 0, NULL, 0);
    expect("unknown code rejected", ret < 0);

    fileXioDevctl("mass5:", DEVCTL_STOP_UNIT, NULL, 0, NULL, 0);
    expect("STOP_UNIT on a missing unit leaves mass0: mounted", mounted("mass0:/"));

    fileXioDevctl("mass0:", DEVCTL_STOP_UNIT, NULL, 0, NULL, 0);
    expect("STOP_UNIT on mass0: unmounts it", !mounted("mass0:/"));

    fileXioDevctl("mass:", DEVCTL_STOP_ALL, NULL, 0, NULL, 0);
    expect("STOP_ALL succeeds without crashing", 1);

    kputs(failures ? "bdmfs-devctl: FAIL\n" : "bdmfs-devctl: PASS\n");
    printf("[ps2rig] exit=%d\n", failures ? 1 : 0);
    SleepThread();
    return 0;
}
