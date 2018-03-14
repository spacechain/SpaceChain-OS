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
** 文   件   名: _EventSetUnQueue.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 20 日
**
** 描        述: 将线程从等待事件集队列中解除

** BUG:
2009.12.12 修改文件格式.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _EventSetUnQueue
** 功能描述: 将线程从等待事件集队列中解除
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

VOID  _EventSetUnQueue (PLW_CLASS_EVENTSETNODE   pesn)
{
#if LW_CFG_THREAD_DEL_EN > 0
    REGISTER PLW_CLASS_TCB         ptcb;
#endif
    
    REGISTER PLW_CLASS_EVENTSET    pes;
    
    pes = (PLW_CLASS_EVENTSET)pesn->EVENTSETNODE_pesEventSet;
    
    _List_Line_Del(&pesn->EVENTSETNODE_lineManage, &pes->EVENTSET_plineWaitList);
    
#if LW_CFG_THREAD_DEL_EN > 0
    ptcb = (PLW_CLASS_TCB)pesn->EVENTSETNODE_ptcbMe;
    ptcb->TCB_pesnPtr = LW_NULL;
#endif
}

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
