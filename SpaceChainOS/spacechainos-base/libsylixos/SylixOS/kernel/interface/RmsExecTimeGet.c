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
** 文   件   名: RmsExecTimeGet.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 11 日
**
** 描        述: 获得当前任务从调用 API_RmsPeriod() 函数到目前执行的时间，单位为: Tick.

** BUG
2007.11.04  加入 _DebugHandle() 功能.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_RmsExecTimeGet
** 功能描述: 获得当前任务从调用 API_RmsPeriod() 函数到目前执行的时间，单位为: Tick.
** 输　入  : 
**           ulId                          RMS 句柄
**           pulExecTime                   运行时间
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)

LW_API  
ULONG   API_RmsExecTimeGet (LW_OBJECT_HANDLE  ulId, ULONG  *pulExecTime)
{
             INTREG         iregInterLevel;
    REGISTER PLW_CLASS_RMS  prms;
    REGISTER UINT16         usIndex;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pulExecTime) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pulExecTime invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_BUFFER_NULL);
        return  (ERROR_KERNEL_BUFFER_NULL);
    }
    
    if (!_ObjectClassOK(ulId, _OBJECT_RMS)) {                           /*  对象类型检查                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "RMS handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Rms_Index_Invalid(usIndex)) {                                  /*  缓冲区索引检查              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "RMS handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核                    */
    if (_Rms_Type_Invalid(usIndex)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "RMS handle invalidate.\r\n");
        _ErrorHandle(ERROR_RMS_NULL);
        return  (ERROR_RMS_NULL);
    }
#else
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核                    */
#endif

    prms = &_K_rmsBuffer[usIndex];
    
    if (prms->RMS_ucStatus != LW_RMS_ACTIVE) {                          /*  状态错误                    */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _ErrorHandle(ERROR_RMS_STATUS);
        return  (ERROR_RMS_STATUS);
    }
    
    *pulExecTime = _RmsGetExecTime(prms);                               /*  获得运行时间                */
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核                    */

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_RMS_EN > 0)         */
                                                                        /*  (LW_CFG_MAX_RMSS > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
