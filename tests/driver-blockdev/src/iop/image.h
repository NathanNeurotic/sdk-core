#ifndef RAMDISK_IMAGE_H
#define RAMDISK_IMAGE_H

#define RAMDISK_SECTOR_SIZE 512

struct ramdisk_seed
{
    unsigned int lba;
    unsigned char data[RAMDISK_SECTOR_SIZE];
};

extern const unsigned int ramdisk_sector_count;
extern const struct ramdisk_seed ramdisk_seed[];
extern const unsigned int ramdisk_seed_count;

#endif
