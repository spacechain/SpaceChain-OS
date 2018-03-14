/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#include <sys/cdefs.h>
#include <SylixOS.h>

/*
 * lib_difftime
 */
__weak_reference(lib_difftime, difftime);

double  lib_difftime (time_t  time1, time_t  time2)
{
    return  ((double)time1 - (double)time2);
}

/*
 * end
 */
