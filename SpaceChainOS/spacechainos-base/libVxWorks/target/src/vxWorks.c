/**
 * @file
 * system basice functions.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#include <stdio.h>
#include "vxWorks.h"
#include "version.h"

/*
 * vxWorks
 */
size_t allocAlignSize;
size_t stackAlignSize;
size_t cacheAlignSize;
size_t vmPageSize;

/*
 * version
 */
char *creationDate;
char *runtimeVersion;
char *runtimeName;
char *vxWorksVersion;

const unsigned int vxWorksVersionMajor = _WRS_VXWORKS_MAJOR;
const unsigned int vxWorksVersionMinor = _WRS_VXWORKS_MINOR;
const unsigned int vxWorksVersionMaint = _WRS_VXWORKS_MAINT;

/*
 * VxWorks taskLib init.
 */
extern void  vxWorksTaskInit(void);

/*
 * VxWorks logLib init & deinit.
 */
extern void  vxWorksLogInit(void);
extern void  vxWorksLogDeinit(void);

/*
 * VxWorks compatibility init.
 */
LW_CONSTRUCTOR_BEGIN
LW_LIB_HOOK_STATIC void  __vxWorksInit (void)
{
    allocAlignSize = sizeof(size_t);
    stackAlignSize = sizeof(LW_STACK);
    cacheAlignSize = (size_t)(API_CacheLine(DATA_CACHE) >> 2);
    vmPageSize     = LW_CFG_VMM_PAGE_SIZE;

    creationDate   = __DATE__ __TIME__;
    runtimeVersion = RUNTIME_VERSION;
    runtimeName    = RUNTIME_NAME;
    vxWorksVersion = VXWORKS_VERSION;

    vxWorksTaskInit();
    vxWorksLogInit();

    printf("VxWorks compatibility library enter.\n");
}
LW_CONSTRUCTOR_END(__vxWorksInit)

/*
 * VxWorks compatibility deinit.
 */
LW_DESTRUCTOR_BEGIN
LW_LIB_HOOK_STATIC void  __vxWorksDeinit (void)
{
    vxWorksLogDeinit();

    printf("VxWorks compatibility library exit.\n");
}
LW_DESTRUCTOR_END(__vxWorksDeinit)

/*
 * VxWorks system Clk Connect.
 */
STATUS  sysClkConnect(FUNCPTR routine, int arg)
{
    errno = ENOSYS;
    return  (ERROR);
}

/*
 * VxWorks system Clk Disable.
 */
void  sysClkDisable (void)
{
    errno = ENOSYS;
}

/*
 * VxWorks system Clk Enable.
 */
void  sysClkEnable (void)
{
    errno = ENOSYS;
}

/*
 * VxWorks system Clk Enable.
 */
int  sysClkRateGet (void)
{
    return  ((int)API_TimeGetFrequency());
}

STATUS  sysClkRateSet (int ticksPerSecond)
{
    errno = ENOSYS;
    return  (ERROR);
}

/*
 * end
 */
