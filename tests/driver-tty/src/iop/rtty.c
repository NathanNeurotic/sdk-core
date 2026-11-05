#include "irx_imports.h"
#include "rtty.h"

IRX_ID("rtty", 1, 0);

#define RING_SIZE 2048

static char ring[RING_SIZE];
static unsigned int head, count;
static int lock = -1;
static int console_fd = -1;
static int previous_console = -1;
static int opening_console;

static int rtty_init(iop_device_t *device)
{
    iop_sema_t sema;

    (void)device;
    sema.attr    = 0;
    sema.option  = 0;
    sema.initial = 1;
    sema.max     = 1;
    lock         = CreateSema(&sema);
    return lock < 0 ? -1 : 0;
}

static int rtty_deinit(iop_device_t *device)
{
    (void)device;
    DeleteSema(lock);
    return 0;
}

static int rtty_open(iop_file_t *f, const char *name, int flags)
{
    (void)flags;
    if (name[0] != '\0')
        return -ENOENT;
    f->privdata = opening_console ? &console_fd : NULL;
    return 0;
}

static int rtty_close(iop_file_t *f)
{
    (void)f;
    return 0;
}

static int rtty_read(iop_file_t *f, void *ptr, int size)
{
    char *out = ptr;
    int n     = 0;

    (void)f;
    WaitSema(lock);
    while (n < size && count > 0) {
        out[n++] = ring[(head + RING_SIZE - count) % RING_SIZE];
        count--;
    }
    SignalSema(lock);
    return n;
}

static int rtty_write(iop_file_t *f, void *ptr, int size)
{
    const char *in = ptr;

    WaitSema(lock);
    for (int i = 0; i < size; i++) {
        ring[head] = in[i];
        head       = (head + 1) % RING_SIZE;
        if (count < RING_SIZE)
            count++;
    }
    SignalSema(lock);

    if (f->privdata != NULL && previous_console >= 0)
        write(previous_console, ptr, size);
    return size;
}

static int attach_console(void)
{
    if (console_fd >= 0)
        return console_fd;

    previous_console = open("tty:", O_WRONLY);
    close(1);
    opening_console = 1;
    console_fd      = open("rtty:", O_WRONLY);
    opening_console = 0;
    printf("rtty: console attached\n");
    return console_fd;
}

static int rtty_ioctl(iop_file_t *f, int cmd, void *param)
{
    (void)f;
    (void)param;
    switch (cmd) {
        case RTTY_IOCTL_CLEAR:
            WaitSema(lock);
            count = 0;
            SignalSema(lock);
            return 0;
        case RTTY_IOCTL_ATTACH_CONSOLE:
            return attach_console();
        default:
            return -EINVAL;
    }
}

IOMAN_RETURN_VALUE_IMPL(EPERM);

static iop_device_ops_t rtty_ops = {
    &rtty_init,
    &rtty_deinit,
    IOMAN_RETURN_VALUE(EPERM), // format
    &rtty_open,
    &rtty_close,
    &rtty_read,
    &rtty_write,
    IOMAN_RETURN_VALUE(EPERM), // lseek
    &rtty_ioctl,
    IOMAN_RETURN_VALUE(EPERM), // remove
    IOMAN_RETURN_VALUE(EPERM), // mkdir
    IOMAN_RETURN_VALUE(EPERM), // rmdir
    IOMAN_RETURN_VALUE(EPERM), // dopen
    IOMAN_RETURN_VALUE(EPERM), // dclose
    IOMAN_RETURN_VALUE(EPERM), // dread
    IOMAN_RETURN_VALUE(EPERM), // getstat
    IOMAN_RETURN_VALUE(EPERM), // chstat
};

static iop_device_t rtty_device = {
    "rtty",
    IOP_DT_CHAR | IOP_DT_CONS,
    1,
    "Ring buffer tty",
    &rtty_ops,
};

int _start(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    DelDrv(rtty_device.name);
    if (AddDrv(&rtty_device) != 0) {
        printf("rtty: AddDrv failed\n");
        return MODULE_NO_RESIDENT_END;
    }
    return MODULE_RESIDENT_END;
}
