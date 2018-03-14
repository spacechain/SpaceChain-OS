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
** 文   件   名: RmsDelete.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 25 日
**
** 描        述: 删除精度单调调度器

** BUG
2007.11.04  加入 _DebugHandle() 功能.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2011.07.29  加入对象创建/销毁回调.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  注意：
        使用精度单调调度器，不可以设置系统时间，但是可是设置 RTC 时间。
        
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: API_RmsDeleteEx
** 功能描述: 删除精度单调调度器
** 输　入  : 
**           pulId                         RMS 句柄指针
**           bForce                        强制删除
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)

LW_API  
ULONG  API_RmsDeleteEx (LW_OBJECT_HANDLE   *pulId, BOOL  bForce)
{
             INTREG             iregInterLevel;
    REGISTER PLW_CLASS_RMS      prms;
    REGISTER UINT16             usIndex;
    REGISTER LW_OBJECT_HANDLE   ulId;
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
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
    
    if (!bForce) {
        if (prms->RMS_ucStatus == LW_RMS_EXPIRED) {                     /*  状态错误                    */
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核                    */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "RMS status error.\r\n");
            _ErrorHandle(ERROR_RMS_STATUS);
            return  (ERROR_RMS_STATUS);
        }
    }
    
    prms->RMS_ucType = LW_RMS_UNUSED;                                   /*  没有使用                    */
    
    _ObjectCloseId(pulId);
    
    _Free_Rms_Object(prms);
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核                    */
    
    __LW_OBJECT_DELETE_HOOK(ulId);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "RMS \"%s\" has been delete.\r\n", prms->RMS_cRmsName);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_RmsDelete
** 功能描述: 删除精度单调调度器
** 输　入  : 
**           pulId                         RMS 句柄指针
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
ULONG  API_RmsDelete (LW_OBJECT_HANDLE   *pulId)
{
    return  (API_RmsDeleteEx(pulId, LW_FALSE));
}

#endif                                                                  /*  (LW_CFG_RMS_EN > 0)         */
                                                                        /*  (LW_CFG_MAX_RMSS > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
