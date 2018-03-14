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
** 文   件   名: openocd.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2015 年 11 月 19 日
**
** 描        述: SylixOS OpenOCD 调试器支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_HWDBG_OPENOCD_EN > 0
#include "endian.h"
#include "stddef.h"
#if LW_CFG_MODULELOADER_EN > 0
#include "loader/include/loader_lib.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  OpenOCD 信息结构
*********************************************************************************************************/
LW_STRUCT_PACK_BEGIN
struct lw_openocd_info {
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiFlags);
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiSzLong);
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiSzPtr);
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiSzPid);
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiSzObjName);
    
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiMaxThreads);
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiMaxCpus);
    
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiCpuTcbCur);
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiCpuTcbHigh);
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiCpuIntNest);
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiCpuActStat);
    
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiThreadId);
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiThreadPrio);
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiThreadErrNo);
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiThreadCpu);
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiThreadStat);
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiThreadOpt);
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiThreadName);
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiThreadStk);
    
    LW_STRUCT_PACK_FIELD(UINT32     OOCD_uiPid);
} LW_STRUCT_PACK_STRUCT;
LW_STRUCT_PACK_END

typedef struct lw_openocd_info          LW_OPENOCD_INFO;
typedef struct lw_openocd_info         *PLW_OPENOCD_INFO;
/*********************************************************************************************************
  全局符号
*********************************************************************************************************/
LW_OPENOCD_INFO     _G_openocdInfo;
/*********************************************************************************************************
** 函数名称: API_OpenOCDStep1
** 功能描述: 初始化 OpenOCD 调试接口 Step 1
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID  API_OpenOCDStep1 (VOID)
{
    PLW_OPENOCD_INFO   popenocd = &_G_openocdInfo;
    
    popenocd->OOCD_uiFlags = htole32(0);
    
    popenocd->OOCD_uiSzLong    = htole32(sizeof(LONG));
    popenocd->OOCD_uiSzPtr     = htole32(sizeof(PVOID));
    popenocd->OOCD_uiSzPid     = htole32(sizeof(pid_t));
    popenocd->OOCD_uiSzObjName = htole32(LW_CFG_OBJECT_NAME_SIZE);
    
    popenocd->OOCD_uiMaxThreads = htole32(LW_CFG_MAX_THREADS);
    popenocd->OOCD_uiMaxCpus    = htole32(LW_NCPUS);
    
    popenocd->OOCD_uiCpuTcbCur  = htole32(offsetof(LW_CLASS_CPU, CPU_ptcbTCBCur));
    popenocd->OOCD_uiCpuTcbHigh = htole32(offsetof(LW_CLASS_CPU, CPU_ptcbTCBHigh));
    popenocd->OOCD_uiCpuIntNest = htole32(offsetof(LW_CLASS_CPU, CPU_ulInterNesting));
    popenocd->OOCD_uiCpuActStat = htole32(offsetof(LW_CLASS_CPU, CPU_ulStatus));
    
    popenocd->OOCD_uiThreadId    = htole32(offsetof(LW_CLASS_TCB, TCB_ulId));
    popenocd->OOCD_uiThreadPrio  = htole32(offsetof(LW_CLASS_TCB, TCB_ucPriority));
    popenocd->OOCD_uiThreadErrNo = htole32(offsetof(LW_CLASS_TCB, TCB_ulLastError));
    popenocd->OOCD_uiThreadCpu   = htole32(offsetof(LW_CLASS_TCB, TCB_ulCPUId));
    popenocd->OOCD_uiThreadStat  = htole32(offsetof(LW_CLASS_TCB, TCB_usStatus));
    popenocd->OOCD_uiThreadOpt   = htole32(offsetof(LW_CLASS_TCB, TCB_ulOption));
    popenocd->OOCD_uiThreadName  = htole32(offsetof(LW_CLASS_TCB, TCB_cThreadName));
    popenocd->OOCD_uiThreadStk   = htole32(offsetof(LW_CLASS_TCB, TCB_pstkStackNow));
    
#if LW_CFG_MODULELOADER_EN > 0
    popenocd->OOCD_uiPid = htole32(offsetof(LW_LD_VPROC, VP_pid));
#else
    popenocd->OOCD_uiPid = 0;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
}
/*********************************************************************************************************
** 函数名称: API_OpenOCDStep2
** 功能描述: 初始化 OpenOCD 调试接口 Step 2
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID  API_OpenOCDStep2 (VOID)
{
    PLW_OPENOCD_INFO   popenocd = &_G_openocdInfo;
    
    popenocd->OOCD_uiFlags = htole32(1);
}
/*********************************************************************************************************
** 函数名称: API_OpenOCDStep3
** 功能描述: 初始化 OpenOCD 调试接口 Step 3
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID  API_OpenOCDStep3 (VOID)
{
    PLW_OPENOCD_INFO   popenocd = &_G_openocdInfo;
    
    popenocd->OOCD_uiFlags = htole32(0);
}

#endif                                                                  /*  LW_CFG_HWDBG_OPENOCD_EN > 0 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
