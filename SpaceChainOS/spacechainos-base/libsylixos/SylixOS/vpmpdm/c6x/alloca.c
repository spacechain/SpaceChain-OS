/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#define __SYLIXOS_KERNEL
#include "stdlib.h"
#include "SylixOS.h"

/*
 *  alloca config
 */
#define ALLOCA_BUF_SIZE  4096
#define ALLOCA_ALIGN     8

static UINT8 *__alloca_buf = NULL;
static UINT8 *__alloca_now = NULL;

/*
 * Dangerous! this function not safe in DSP.
 */
void *alloca (size_t size)
{
    UINT8 *ret;

    size = ROUND_UP(size, ALLOCA_ALIGN);
    if (size > ALLOCA_BUF_SIZE) {
        return  (NULL);
    }

    API_ThreadLock();

    if (!__alloca_buf) {
        __alloca_buf = (UINT8 *)lib_malloc(ALLOCA_BUF_SIZE); /* align at least 8 bytes */
        if (!__alloca_buf) {
            API_ThreadUnlock();
            return  (NULL);
        }
        __alloca_now = __alloca_buf;
    }

    if ((__alloca_now + size) > (__alloca_buf + ALLOCA_BUF_SIZE)) {
        __alloca_now = __alloca_buf;
        ret = __alloca_now;

    } else {
        ret = __alloca_now;
        __alloca_now += size;
    }

    API_ThreadUnlock();

    return  ((void *)ret);
}

/*
 * end
 */
