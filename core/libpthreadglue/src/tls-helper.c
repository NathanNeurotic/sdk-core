/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/

#include <ps2_osal.h>

int *__keysUsed;

int __maxTlsValues;

pte_osMutexHandle __globalTlsLock;

/* Structure used to emulate TLS on non-POSIX threads.  
 * This limits us to one non-POSIX thread that can
 * call pthread functions. */
void *__globalTls;

pte_osResult pteTlsGlobalInit(int maxEntries)
{
    int i;
    pte_osResult result;

    pte_osMutexCreate(&__globalTlsLock);
    __keysUsed = (int *)malloc(maxEntries * sizeof(int));

    if (__keysUsed != NULL) {
        for (i = 0; i < maxEntries; i++) {
            __keysUsed[i] = 0;
        }

        __maxTlsValues = maxEntries;
        result = PTE_OS_OK;
    } else {
        result = PTE_OS_NO_RESOURCES;
    }

    return result;
}

void *pteTlsThreadInit(void)
{
    void **pTlsStruct;
    int i;

    pTlsStruct = (void **)malloc(__maxTlsValues * sizeof(void *));

    // PTE library assumes that keys are initialized to zero
    for (i = 0; i < __maxTlsValues; i++) {
        pTlsStruct[i] = 0;
    }

    return (void *)pTlsStruct;
}

pte_osResult __pteTlsAlloc(unsigned int *pKey)
{
    int i;
    pte_osResult result = PTE_OS_NO_RESOURCES;

    pte_osMutexLock(__globalTlsLock);

    for (i = 0; i < __maxTlsValues; i++) {
        if (__keysUsed[i] == 0) {
            __keysUsed[i] = 1;

            *pKey = i + 1;
            result = PTE_OS_OK;
            break;
        }
    }

    pte_osMutexUnlock(__globalTlsLock);
    return result;
}

void *pteTlsGetValue(void *pTlsThreadStruct, unsigned int index)
{
    void **pTls = (void **)pTlsThreadStruct;

    if (__keysUsed[index - 1] && pTls != NULL) {
        return pTls[index - 1];
    } else {
        return NULL;
    }
}

pte_osResult __pteTlsSetValue(void *pTlsThreadStruct, unsigned int index, void *value)
{
    pte_osResult result;
    void **pTls = (void **)pTlsThreadStruct;

    if (pTls != NULL) {
        pTls[index - 1] = value;
        result = PTE_OS_OK;
    } else {
        result = PTE_OS_INVALID_PARAM;
    }

    return result;
}

void *__getTlsStructFromThread(s32 thid)
{
    struct OsalThreadInfo *threadInfo = &__threadInfo[thid];
    void *pTls;

    /* If we were called from a pthread, use the TLS allocated when the thread
    * was created.  Otherwise, we were called from a non-pthread, so use the
    * "global".  This is a pretty bad hack, but necessary due to lack of TLS on PS2.
    */
    if (threadInfo->tlsPtr) {
        pTls = threadInfo->tlsPtr;
    } else {
        pTls = __globalTls;
    }

  return pTls;
}

pte_osResult pteTlsFree(unsigned int index)
{
    pte_osResult result;

    if (__keysUsed != NULL) {
        pte_osMutexLock(__globalTlsLock);
        __keysUsed[index - 1] = 0;
        pte_osMutexUnlock(__globalTlsLock);

        result = PTE_OS_OK;
    } else {
        result = PTE_OS_GENERAL_FAILURE;
    }

    return result;
}

void pteTlsThreadDestroy(void *pTlsThreadStruct)
{
    free(pTlsThreadStruct);
}

void pteTlsGlobalDestroy(void)
{
    pte_osMutexDelete(__globalTlsLock);
    free(__keysUsed);
}

pte_osResult pte_osTlsSetValue(unsigned int key, void * value)
{
  void *pTls;

  pTls = __getTlsStructFromThread(GetThreadId());

  return __pteTlsSetValue(pTls, key, value);
}

void * pte_osTlsGetValue(unsigned int index)
{
  void *pTls;

  pTls = __getTlsStructFromThread(GetThreadId());

  return (void *) pteTlsGetValue(pTls, index);

}

pte_osResult pte_osTlsAlloc(unsigned int *pKey)
{
  __getTlsStructFromThread(GetThreadId());

  return __pteTlsAlloc(pKey);

}

pte_osResult pte_osTlsFree(unsigned int index)
{
  return pteTlsFree(index);
}
