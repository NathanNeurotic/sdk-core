/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# (C)2001, Gustavo Scotti (gustavo@scotti.com)
# (c) 2003 Marcus R. Brown (mrbrown@0xd6.org)
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/

/**
 * @file
 * IOP executable file loader API.
 * @defgroup loadfile EE LOADFILE: ELF and IRX loader client library.
 */

#include <tamtypes.h>
#include <ps2lib_err.h>
#include <kernel.h>
#include <sifrpc.h>
#include <string.h>
#include <iopcontrol.h>
#include "sifarg.h"

#include <loadfile.h>
#include <iopheap.h>
#include <fcntl.h>
#include <unistd.h>

extern SifRpcClientData_t _lf_cd;

int _SifLoadElfPart(const char *path, const char *secname, t_ExecData *data, int fno);
int _SifLoadModuleBuffer(void *ptr, int arg_len, const char *args, int *modres);

SifRpcClientData_t _lf_cd;

int SifLoadFileInit()
{
    int res;

    if (HasIopRebootedSinceLastCall())
        SifLoadFileExit();

    if (_lf_cd.server)
        return 0;

    sceSifInitRpc(0);

    while ((res = sceSifBindRpc(&_lf_cd, 0x80000006, 0)) >= 0 && !_lf_cd.server)
        nopdelay();

    if (res < 0)
        return -E_SIF_RPC_BIND;

    return 0;
}

void SifLoadFileExit()
{
    memset(&_lf_cd, 0, sizeof _lf_cd);
}

int _SifLoadModule(const char *path, int arg_len, const char *args, int *modres,
                   int fno, int dontwait)
{
    SIF_ARG(struct _lf_module_load_arg, arg);

    if (SifLoadFileInit() < 0)
        return -SCE_EBINDMISS;

    memset(arg, 0, sizeof *arg);

    strlcpy(arg->path, path, sizeof(arg->path));

    if (args && arg_len) {
        arg->p.arg_len = arg_len > LF_ARG_MAX ? LF_ARG_MAX : arg_len;
        memcpy(arg->args, args, arg->p.arg_len);
    } else {
        arg->p.arg_len = 0;
    }

    if (sceSifCallRpc(&_lf_cd, fno, dontwait, arg, sizeof *arg, arg, 8, NULL, NULL) < 0)
        return -SCE_ECALLMISS;

    if (modres)
        *modres = arg->modres;

    return arg->p.result;
}

int SifLoadModule(const char *path, int arg_len, const char *args)
{
    return _SifLoadModule(path, arg_len, args, NULL, LF_F_MOD_LOAD, 0);
}

int SifLoadStartModule(const char *path, int arg_len, const char *args, int *mod_res)
{
    return _SifLoadModule(path, arg_len, args, mod_res, LF_F_MOD_LOAD, 0);
}

int SifLoadModuleEncrypted(const char *path, int arg_len, const char *args)
{
    return _SifLoadModule(path, arg_len, args, NULL, LF_F_MG_MOD_LOAD, 0);
}

int SifStopModule(int id, int arg_len, const char *args, int *mod_res)
{
    SIF_ARG(struct _lf_module_stop_arg, arg);

    if (SifLoadFileInit() < 0)
        return -SCE_EBINDMISS;

    arg->p.id = id;

    if (args && arg_len) {
        arg->q.arg_len = arg_len > LF_ARG_MAX ? LF_ARG_MAX : arg_len;
        memcpy(arg->args, args, arg->q.arg_len);
    } else {
        arg->q.arg_len = 0;
    }

    if (sceSifCallRpc(&_lf_cd, LF_F_MOD_STOP, 0, arg, sizeof *arg, arg, 8, NULL, NULL) < 0)
        return -SCE_ECALLMISS;

    if (mod_res)
        *mod_res = arg->q.modres;

    return arg->p.result;
}

int SifUnloadModule(int id)
{
    SIF_ARG(union _lf_module_unload_arg, arg);

    if (SifLoadFileInit() < 0)
        return -SCE_EBINDMISS;

    arg->id = id;

    if (sceSifCallRpc(&_lf_cd, LF_F_MOD_UNLOAD, 0, arg, sizeof *arg, arg, 4, NULL, NULL) < 0)
        return -SCE_ECALLMISS;

    return arg->result;
}

int SifSearchModuleByName(const char *name)
{
    SIF_ARG(struct _lf_search_module_by_name_arg, arg);
    if (SifLoadFileInit() < 0)
        return -SCE_EBINDMISS;

    strlcpy(arg->name, name, sizeof(arg->name));

    if (sceSifCallRpc(&_lf_cd, LF_F_SEARCH_MOD_BY_NAME, 0, arg, sizeof *arg, arg, 4, NULL, NULL) < 0)
        return -SCE_ECALLMISS;

    return arg->id;
}

int SifSearchModuleByAddress(const void *ptr)
{
    SIF_ARG(struct _lf_search_module_by_address_arg, arg);
    if (SifLoadFileInit() < 0)
        return -SCE_EBINDMISS;

    arg->p.ptr = ptr;

    if (sceSifCallRpc(&_lf_cd, LF_F_SEARCH_MOD_BY_ADDRESS, 0, arg, sizeof *arg, arg, 4, NULL, NULL) < 0)
        return -SCE_ECALLMISS;

    return arg->p.id;
}

int _SifLoadElfPart(const char *path, const char *secname, t_ExecData *data, int fno)
{
    SIF_ARG(struct _lf_elf_load_arg, arg);

    if (SifLoadFileInit() < 0)
        return -SCE_EBINDMISS;

    strlcpy(arg->path, path, sizeof(arg->path));
    strlcpy(arg->secname, secname, sizeof(arg->secname));

    if (sceSifCallRpc(&_lf_cd, fno, 0, arg, sizeof *arg, arg,
                   sizeof(t_ExecData), NULL, NULL) < 0)
        return -SCE_ECALLMISS;

    if (arg->epc != 0) {
        data->epc = arg->epc;
        data->gp  = arg->gp;

        return 0;
    } else
        return -SCE_ELOADMISS;
}

int SifLoadElfPart(const char *path, const char *secname, t_ExecData *data)
{
    return _SifLoadElfPart(path, secname, data, LF_F_ELF_LOAD);
}

int SifLoadElf(const char *path, t_ExecData *data)
{
    u32 secname = 0x6c6c61; /* "all" */
    return _SifLoadElfPart(path, (char *)&secname, data, LF_F_ELF_LOAD);
}

int SifLoadElfEncrypted(const char *path, t_ExecData *data)
{
    u32 secname = 0x6c6c61; /* "all" */
    return _SifLoadElfPart(path, (char *)&secname, data, LF_F_MG_ELF_LOAD);
}

int SifIopSetVal(u32 iop_addr, int val, int type)
{
    SIF_ARG(struct _lf_iop_val_arg, arg);

    if (SifLoadFileInit() < 0)
        return -SCE_EBINDMISS;

    switch (type) {
        case LF_VAL_BYTE:
            arg->val.b = (u8)(val & 0xff);
            break;
        case LF_VAL_SHORT:
            arg->val.s = (u16)(val & 0xffff);
            break;
        case LF_VAL_LONG:
            arg->val.l = val;
            break;
        default:
            return -E_LIB_INVALID_ARG;
    }

    arg->p.iop_addr = iop_addr;
    arg->type       = type;

    if (sceSifCallRpc(&_lf_cd, LF_F_SET_ADDR, 0, arg, sizeof *arg, arg, 4,
                   NULL, NULL) < 0)
        return -SCE_ECALLMISS;

    return arg->p.result;
}

int SifIopGetVal(u32 iop_addr, void *val, int type)
{
    SIF_ARG(struct _lf_iop_val_arg, arg);

    if (SifLoadFileInit() < 0)
        return -SCE_EBINDMISS;

    arg->p.iop_addr = iop_addr;
    arg->type       = type;

    if (sceSifCallRpc(&_lf_cd, LF_F_GET_ADDR, 0, arg, sizeof *arg, arg, 4,
                   NULL, NULL) < 0)
        return -SCE_ECALLMISS;

    if (val) {
        switch (type) {
            case LF_VAL_BYTE:
                *(u8 *)val = (u8)arg->p.result & 0xff;
                break;
            case LF_VAL_SHORT:
                *(u16 *)val = (u16)arg->p.result & 0xffff;
                break;
            case LF_VAL_LONG:
                *(u32 *)val = arg->p.result;
                break;
        }
    }

    return 0;
}

int _SifLoadModuleBuffer(void *ptr, int arg_len, const char *args, int *modres)
{
    SIF_ARG(struct _lf_module_buffer_load_arg, arg);

    if (SifLoadFileInit() < 0)
        return -SCE_EBINDMISS;

    memset(arg, 0, sizeof *arg);

    arg->p.ptr = ptr;
    if (args && arg_len) {
        arg->q.arg_len = arg_len > LF_ARG_MAX ? LF_ARG_MAX : arg_len;
        memcpy(arg->args, args, arg->q.arg_len);
    } else {
        arg->q.arg_len = 0;
    }

    if (sceSifCallRpc(&_lf_cd, LF_F_MOD_BUF_LOAD, 0, arg, sizeof *arg, arg, 8,
                   NULL, NULL) < 0)
        return -SCE_ECALLMISS;

    if (modres)
        *modres = arg->q.modres;

    return arg->p.result;
}

int SifLoadModuleBuffer(void *ptr, int arg_len, const char *args)
{
    return _SifLoadModuleBuffer(ptr, arg_len, args, NULL);
}

int SifLoadStartModuleBuffer(void *ptr, int arg_len, const char *args, int *mod_res)
{
    return _SifLoadModuleBuffer(ptr, arg_len, args, mod_res);
}

int SifExecModuleBuffer(void *ptr, u32 size, u32 arg_len, const char *args, int *mod_res)
{
    SifDmaTransfer_t dmat;
    void *iop_addr;
    int res;
    unsigned int qid;

    /* Round the size up to the nearest 16 bytes. */
    size = (size + 15) & -16;

    if (!(iop_addr = SifAllocIopHeap(size)))
        return -E_IOP_NO_MEMORY;

    dmat.src  = ptr;
    dmat.dest = iop_addr;
    dmat.size = size;
    dmat.attr = 0;
    sceSifWriteBackDCache(ptr, size);
    qid = sceSifSetDma(&dmat, 1);

    if (!qid)
        return -1; // should have a better error here...

    while (sceSifDmaStat(qid) >= 0)
        ;

    res = _SifLoadModuleBuffer(iop_addr, arg_len, args, mod_res);
    SifFreeIopHeap(iop_addr);

    return res;
}

int SifExecModuleFile(const char *path, u32 arg_len, const char *args, int *mod_res)
{
    void *iop_addr;
    int res, size, fd;

    if ((fd = open(path, O_RDONLY)) < 0)
        return fd;

    if ((size = lseek(fd, 0, SEEK_END)) < 0)
        return size;

    close(fd);

    if (!(iop_addr = SifAllocIopHeap(size)))
        return -E_IOP_NO_MEMORY;

    if ((res = SifLoadIopHeap(path, iop_addr)) < 0) {
        SifFreeIopHeap(iop_addr);
        return res;
    }

    res = _SifLoadModuleBuffer(iop_addr, arg_len, args, mod_res);
    SifFreeIopHeap(iop_addr);

    return res;
}
