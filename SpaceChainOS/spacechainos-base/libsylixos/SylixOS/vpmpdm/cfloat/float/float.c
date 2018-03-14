/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#define  __SYLIXOS_KERNEL /* need some kernel function */
#include <sys/cdefs.h>
#include <SylixOS.h>

/*
 * lib_isnan
 */
__weak_reference(lib_isnan, isnan);

int  lib_isnan (double  dX)
{
    return  (__ARCH_DOUBLE_ISNAN(dX));
}

/*
 * lib_isinf
 */
__weak_reference(lib_isinf, isinf);

int  lib_isinf (double  dX)
{
    return  (__ARCH_DOUBLE_ISINF(dX));
}

/*
 * end
 */
