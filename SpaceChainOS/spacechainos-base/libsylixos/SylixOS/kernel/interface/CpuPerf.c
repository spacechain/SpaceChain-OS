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
** 文   件   名: CpuPerf.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 11 月 25 日
**
** 描        述: CPU 性能计算.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  运算结果存储
*********************************************************************************************************/
static ULONG        _K_ulKInsPerSec[LW_CFG_MAX_PROCESSORS];
/*********************************************************************************************************
** 函数名称: _CpuBogoMips
** 功能描述: 粗略计算 CPU 运算速度.
** 输　入  : ulCPUId           CPU ID
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _CpuBogoMips (ULONG  ulCPUId)
{
    ULONG   ulLoopCnt = 1600000;                                        /*  认为处理器至少为 64MHz      */
    ULONG   ulKInsPerSec;
    clock_t clockTick;
    clock_t clockPer100ms = CLOCKS_PER_SEC / 10;
    
    while (ulLoopCnt <<= 1) {
        clockTick = lib_clock();
        __ARCH_BOGOMIPS_LOOP(ulLoopCnt);
        clockTick = lib_clock() - clockTick;
        
        if (clockTick >= clockPer100ms) {
            ulKInsPerSec = (ulLoopCnt / clockTick / 1000) * CLOCKS_PER_SEC;
            _K_ulKInsPerSec[ulCPUId] = ulKInsPerSec * __ARCH_BOGOMIPS_INS_PER_LOOP;
            break;
        }
    }
}
/*********************************************************************************************************
** 函数名称: API_CpuBogoMips
** 功能描述: 粗略计算 CPU 运算速度.
** 输　入  : ulCPUId           CPU ID
**           pulKInsPerSec     每秒执行的指令数量 (千条指令为单位)
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_CpuBogoMips (ULONG  ulCPUId, ULONG  *pulKInsPerSec)
{
#if LW_CFG_SMP_EN > 0
    LW_CLASS_CPUSET     pcpusetNew;
    LW_CLASS_CPUSET     pcpusetOld;
    LW_OBJECT_HANDLE    ulMe = API_ThreadIdSelf();
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    if (ulCPUId >= LW_NCPUS) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if (!LW_SYS_STATUS_IS_RUNNING()) {
        _ErrorHandle(ERROR_KERNEL_NOT_RUNNING);
        return  (ERROR_KERNEL_NOT_RUNNING);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    if (_K_ulKInsPerSec[ulCPUId]) {                                     /*  之前已经计算过              */
        if (pulKInsPerSec) {
            *pulKInsPerSec = _K_ulKInsPerSec[ulCPUId];
        }
        return  (ERROR_NONE);
    }
    
#if LW_CFG_SMP_EN > 0
    if (LW_NCPUS > 1) {
        if (API_CpuIsUp(ulCPUId)) {
            API_ThreadGetAffinity(ulMe, sizeof(pcpusetOld), &pcpusetOld);
            LW_CPU_ZERO(&pcpusetNew);
            LW_CPU_SET(ulCPUId, &pcpusetNew);                           /*  锁定 CPU                    */
            API_ThreadSetAffinity(ulMe, sizeof(pcpusetNew), &pcpusetNew);   
            API_TimeSleep(1);                                           /*  确保进入指定的 CPU 运行     */
            _CpuBogoMips(ulCPUId);
            API_ThreadSetAffinity(ulMe, sizeof(pcpusetOld), &pcpusetOld);
        }
    } else 
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    {
        _CpuBogoMips(ulCPUId);
    }
    
    if (pulKInsPerSec) {
        *pulKInsPerSec = _K_ulKInsPerSec[ulCPUId];
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
