/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: loader_affinity.c
**
** 创   建   人: Han.hui (韩辉)
**
** 文件创建日期: 2014 年 11 月 11 日
**
** 描        述: 进程 CPU 亲和度.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
/*********************************************************************************************************
** 函数名称: vprocSetAffinity
** 功能描述: 设置进程调度的 CPU 集合
** 输　入  : pid           进程
**           stSize        CPU 掩码集内存大小
**           pcpuset       CPU 掩码
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vprocSetAffinity (pid_t  pid, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset)
{
    INT          iRet;
    LW_LD_VPROC *pvproc;

    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (!pvproc) {
        LW_LD_UNLOCK();
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    
    iRet = vprocThreadAffinity(pvproc, stSize, pcpuset);
    LW_LD_UNLOCK();
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: vprocGetAffinity
** 功能描述: 获取进程调度的 CPU 集合
** 输　入  : pid           进程
**           stSize        CPU 掩码集内存大小
**           pcpuset       CPU 掩码
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vprocGetAffinity (pid_t  pid, size_t  stSize, PLW_CLASS_CPUSET  pcpuset)
{
    LW_OBJECT_HANDLE  ulId;
    
    if ((stSize < sizeof(LW_CLASS_CPUSET)) || !pcpuset) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    ulId = vprocMainThread(pid);
    if (ulId == LW_OBJECT_HANDLE_INVALID) {
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    
    if (API_ThreadGetAffinity(ulId, stSize, pcpuset)) {
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
