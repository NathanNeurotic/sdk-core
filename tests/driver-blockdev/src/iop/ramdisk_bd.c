#include "irx_imports.h"
#include "image.h"

IRX_ID("ramdisk_bd", 1, 0);

static u8 *disk;
static int lock = -1;
static struct block_device bd;

static int in_range(struct block_device *b, u64 sector, u16 count)
{
    return sector < b->sectorCount && count <= b->sectorCount - sector;
}

static int ramdisk_read(struct block_device *b, u64 sector, void *buffer, u16 count)
{
    if (!in_range(b, sector, count))
        return -EIO;

    WaitSema(lock);
    memcpy(buffer, disk + (u32)sector * RAMDISK_SECTOR_SIZE, count * RAMDISK_SECTOR_SIZE);
    SignalSema(lock);
    return count;
}

static int ramdisk_write(struct block_device *b, u64 sector, const void *buffer, u16 count)
{
    if (!in_range(b, sector, count))
        return -EIO;

    WaitSema(lock);
    memcpy(disk + (u32)sector * RAMDISK_SECTOR_SIZE, buffer, count * RAMDISK_SECTOR_SIZE);
    SignalSema(lock);
    return count;
}

static void ramdisk_flush(struct block_device *b)
{
    (void)b;
}

static int ramdisk_stop(struct block_device *b)
{
    (void)b;
    return 0;
}

int _start(int argc, char *argv[])
{
    iop_sema_t sema;
    u32 size = ramdisk_sector_count * RAMDISK_SECTOR_SIZE;

    (void)argc;
    (void)argv;

    disk = AllocSysMemory(ALLOC_FIRST, size, NULL);
    if (disk == NULL) {
        printf("ramdisk_bd: cannot allocate %u bytes\n", (unsigned int)size);
        return MODULE_NO_RESIDENT_END;
    }
    memset(disk, 0, size);
    for (u32 i = 0; i < ramdisk_seed_count; i++)
        memcpy(disk + ramdisk_seed[i].lba * RAMDISK_SECTOR_SIZE, ramdisk_seed[i].data, RAMDISK_SECTOR_SIZE);

    sema.attr    = 0;
    sema.option  = 0;
    sema.initial = 1;
    sema.max     = 1;
    lock         = CreateSema(&sema);
    if (lock < 0) {
        FreeSysMemory(disk);
        return MODULE_NO_RESIDENT_END;
    }

    bd.priv         = NULL;
    bd.name         = "ram";
    bd.path         = "ram";
    bd.devNr        = 0;
    bd.parNr        = 0;
    bd.parId        = 0;
    bd.sectorSize   = RAMDISK_SECTOR_SIZE;
    bd.sectorOffset = 0;
    bd.sectorCount  = ramdisk_sector_count;
    bd.read         = ramdisk_read;
    bd.write        = ramdisk_write;
    bd.flush        = ramdisk_flush;
    bd.stop         = ramdisk_stop;
    bdm_connect_bd(&bd);

    printf("ramdisk_bd: %u sectors registered\n", ramdisk_sector_count);
    return MODULE_RESIDENT_END;
}
