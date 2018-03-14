/*
 *  This file NOT include in open source version. 
 */
 
#include "SylixOS.h"

#if (LW_CFG_SYSPERF_EN > 0) && (LW_CFG_MODULELOADER_EN > 0)

#include "sysperf.h"

LW_API 
__attribute__((weak)) INT  API_SysPerfStart (UINT  uiPipeLen, UINT  uiPerfNum, UINT  uiRefreshPeriod)
{
    errno = ENOSYS;
    return  (PX_ERROR);
}

LW_API 
__attribute__((weak)) INT  API_SysPerfStop (VOID)
{
    errno = ENOSYS;
    return  (PX_ERROR);
}

LW_API 
__attribute__((weak)) INT  API_SysPerfRefresh (VOID)
{
    errno = ENOSYS;
    return  (PX_ERROR);
}

LW_API 
__attribute__((weak)) INT  API_SysPerfInfo (PLW_SYSPERF_INFO  psysperf, UINT  uiPerfNum)
{
    errno = ENOSYS;
    return  (PX_ERROR);
}

#endif  /*  LW_CFG_SYSPERF_EN > 0       */
        /*  LW_CFG_MODULELOADER_EN > 0  */
