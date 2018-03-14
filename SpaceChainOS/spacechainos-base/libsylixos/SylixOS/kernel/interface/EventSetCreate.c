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
** 文   件   名: EventSetCreate.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 20 日
**
** 描        述: 建立事件集

** BUG
2008.01.13  加入 _DebugHandle() 功能。
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock(); 
2008.05.31  使用 __KERNEL_MODE_...().
2009.04.08  加入对 SMP 多核的支持.
2009.07.28  自旋锁的初始化放在初始化所有的控制块中, 这里去除相关操作.
2011.02.23  加入创建选项保存.
2011.07.29  加入对象创建/销毁回调.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_EventSetCreate
** 功能描述: 建立事件集
** 输　入  : 
**           pcName          事件集名缓冲区
**           ulInitEvent     初始化事件
**           ulOption        事件集选项
**           pulId           事件集ID指针
** 输　出  : 事件句柄
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

LW_API  
LW_OBJECT_HANDLE  API_EventSetCreate (CPCHAR             pcName, 
                                      ULONG              ulInitEvent, 
                                      ULONG              ulOption,
                                      LW_OBJECT_ID      *pulId)
{
    REGISTER PLW_CLASS_EVENTSET    pes;
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
        pes = _Allocate_EventSet_Object();                              /*  获得一个事件控制块          */
    );
    
    if (!pes) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is no ID to buide a eventset.\r\n");
        _ErrorHandle(ERROR_EVENTSET_FULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (pcName) {                                                       /*  拷贝名字                    */
        lib_strcpy(pes->EVENTSET_cEventSetName, pcName);
    } else {
        pes->EVENTSET_cEventSetName[0] = PX_EOS;                        /*  清空名字                    */
    }
    
    pes->EVENTSET_ucType        = LW_TYPE_EVENT_EVENTSET;
    pes->EVENTSET_plineWaitList = LW_NULL;
    pes->EVENTSET_ulEventSets   = ulInitEvent;
    pes->EVENTSET_ulOption      = ulOption;                             /*  记录选项                    */
    
    ulIdTemp = _MakeObjectId(_OBJECT_EVENT_SET, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             pes->EVENTSET_usIndex);                    /*  构建对象 id                 */
    
    if (pulId) {
        *pulId = ulIdTemp;
    }
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, ulOption);
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_ESET, MONITOR_EVENT_ESET_CREATE, 
                      ulIdTemp, ulInitEvent, ulOption, pcName);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "eventset \"%s\" has been create.\r\n", (pcName ? pcName : ""));
    
    return  (ulIdTemp);
}

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
