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
** 文   件   名: _StackCheckInit.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 04 月 16 日
**
** 描        述: 堆栈检查初始化

** BUG:
2013.09.17  加入线程堆栈警戒检查功能.
2014.08.10  堆栈溢出调用相关 hook.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  进程相关
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** 函数名称: _StackCheckInit
** 功能描述: 初始化堆栈检查相关数据
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _StackCheckInit (VOID)
{
    REGISTER INT        i;
    
    _K_stkFreeFlag = (UCHAR)LW_CFG_STK_EMPTY_FLAG;
    for (i = 0; i < (sizeof(LW_STACK) - 1); i++) {
        _K_stkFreeFlag = (LW_STACK)((LW_STACK)(_K_stkFreeFlag << 8) + (UCHAR)LW_CFG_STK_EMPTY_FLAG);
    }
}
/*********************************************************************************************************
** 函数名称: _StackCheckGuard
** 功能描述: 线程堆栈警戒检查
** 输　入  : ptcb      任务控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _StackCheckGuard (PLW_CLASS_TCB  ptcb)
{
    pid_t   pid;

    if ((ptcb->TCB_ulOption & LW_OPTION_THREAD_STK_CLR) &&
        (*ptcb->TCB_pstkStackGuard != _K_stkFreeFlag)) {
#if LW_CFG_MODULELOADER_EN > 0
        pid = vprocGetPidByTcbNoLock(ptcb);
#else
        pid = 0;
#endif
        _DebugFormat(__ERRORMESSAGE_LEVEL, "thread %s id 0x%08lx stack may overflow.\r\n",
                     ptcb->TCB_cThreadName, ptcb->TCB_ulId);
        __LW_STACK_OVERFLOW_HOOK(pid, ptcb->TCB_ulId);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
