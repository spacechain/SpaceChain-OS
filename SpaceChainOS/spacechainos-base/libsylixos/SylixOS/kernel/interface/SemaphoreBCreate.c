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
** 文   件   名: SemaphoreBCreate.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 21 日
**
** 描        述: 二进制型信号量建立

** BUG
2007.07.21  加入 _DebugHandle() 功能。
2008.03.30  整理代码.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2008.05.31  使用 __KERNEL_MODE_...().
2009.04.08  加入对 SMP 多核的支持.
2009.07.28  自旋锁的初始化放在初始化所有的控制块中, 这里去除相关操作.
2011.07.29  加入对象创建/销毁回调.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_SemaphoreBCreate
** 功能描述: 二进制型信号量建立
** 输　入  : 
**           pcName          事件名
**           bInitValue      初始化值
**           ulOption        事件选项
**           pulId           事件ID指针
** 输　出  : 事件句柄
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_SEMB_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
LW_OBJECT_HANDLE  API_SemaphoreBCreate (CPCHAR             pcName,
                                        BOOL               bInitValue,
                                        ULONG              ulOption,
                                        LW_OBJECT_ID      *pulId)
{
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER ULONG                 ulI;
             PLW_CLASS_WAITQUEUE   pwqTemp[2];
    REGISTER ULONG                 ulIdTemp;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }

    if (_Object_Name_Invalid(pcName)) {                                 /*  检查名字有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }

    __KERNEL_MODE_PROC(
        pevent = _Allocate_Event_Object();                              /*  获得一个事件控制块          */
    );
    
    if (!pevent) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is no ID to buide a semaphore.\r\n");
        _ErrorHandle(ERROR_EVENT_FULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (pcName) {                                                       /*  拷贝名字                    */
        lib_strcpy(pevent->EVENT_cEventName, pcName);
    } else {
        pevent->EVENT_cEventName[0] = PX_EOS;                           /*  清空名字                    */
    }
                                                                        /*  开始初始化                  */
    pevent->EVENT_ucType       = LW_TYPE_EVENT_SEMB;
    pevent->EVENT_ulCounter    = (ULONG)bInitValue;
    pevent->EVENT_ulMaxCounter = (ULONG)LW_TRUE;
    pevent->EVENT_ulOption     = ulOption;
    pevent->EVENT_pvPtr        = LW_NULL;
    pevent->EVENT_pvTcbOwn     = LW_NULL;
    
    pwqTemp[0] = &pevent->EVENT_wqWaitQ[0];
    pwqTemp[1] = &pevent->EVENT_wqWaitQ[1];
    pwqTemp[0]->WQ_usNum = 0;                                           /*  没有线程                    */
    pwqTemp[1]->WQ_usNum = 0;
    
    for (ulI = 0; ulI < __EVENT_Q_SIZE; ulI++) {
        pwqTemp[0]->WQ_wlQ.WL_pringPrio[ulI] = LW_NULL;
        pwqTemp[1]->WQ_wlQ.WL_pringPrio[ulI] = LW_NULL;
    }
    
    ulIdTemp = _MakeObjectId(_OBJECT_SEM_B, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             pevent->EVENT_usIndex);                    /*  构建对象 id                 */
    
    if (pulId) {
        *pulId = ulIdTemp;
    }
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, ulOption);
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_SEMB, MONITOR_EVENT_SEM_CREATE, 
                      ulIdTemp, bInitValue, ulOption, pcName);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "semaphore \"%s\" has been create.\r\n", (pcName ? pcName : ""));
    
    return  (ulIdTemp);
}

#endif                                                                  /*  (LW_CFG_SEMB_EN > 0)        */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
