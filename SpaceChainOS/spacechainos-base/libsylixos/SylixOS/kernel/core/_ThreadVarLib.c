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
** 文   件   名: _ThreadVarLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 线程内部全局变量恢复
**

** BUG
2007.03.20  在 FREE 操作时使用 Lock() 和 Unlok() 提高安全性
2007.11.13  使用链表库对链表操作进行完全封装.
2007.11.18  整理注释.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2008.05.31  使用 __KERNEL_MODE_...().
2012.07.04  合并 _ThreadVarSwith() 函数到此处.
2014.07.22  加入 _ThreadVarSave() 作为 CPU 停止时保存运行的线程私有变量.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _ThreadVarDelete
** 功能描述: 线程内部全局变量恢复
** 输　入  : ptcb      线程控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)

VOID  _ThreadVarDelete (PLW_CLASS_TCB  ptcb)
{
    REGISTER PLW_LIST_LINE          plineVar;
    REGISTER PLW_CLASS_TCB          ptcbCur;
    REGISTER PLW_CLASS_THREADVAR    pthreadvar;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (ptcb != ptcbCur) {                                              /*  不是当前线程删除            */
        for (plineVar  = ptcb->TCB_plinePrivateVars;                    /*  获得线程私有变量            */
             plineVar != LW_NULL; 
             plineVar  = _list_line_get_next(plineVar)) {
            
            pthreadvar = _LIST_ENTRY(plineVar, LW_CLASS_THREADVAR, PRIVATEVAR_lineVarList);
            
            __KERNEL_MODE_PROC(
                _Free_ThreadVar_Object(pthreadvar);                     /*  释放变量块                  */
            );
        }
    } else {                                                            /*  当前线程删除                */
        for (plineVar  = ptcb->TCB_plinePrivateVars;                    /*  获得线程私有变量            */
             plineVar != LW_NULL; 
             plineVar  = _list_line_get_next(plineVar)) {
            
            pthreadvar = _LIST_ENTRY(plineVar, LW_CLASS_THREADVAR, PRIVATEVAR_lineVarList);
            *pthreadvar->PRIVATEVAR_pulAddress = pthreadvar->PRIVATEVAR_ulValueSave;
                                                                        /*  恢复外部变量的值            */
            __KERNEL_MODE_PROC(
                _Free_ThreadVar_Object(pthreadvar);                     /*  释放变量块                  */
            );
        }
    }
}
/*********************************************************************************************************
** 函数名称: _ThreadVarSwith.
** 功能描述: 线程内部全局变量切换
** 输　入  : ptcbOld       即将被换出的线程
**           ptcbNew       将被换入的线程
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadVarSwith (PLW_CLASS_TCB  ptcbOld, PLW_CLASS_TCB  ptcbNew)
{
    REGISTER PLW_LIST_LINE          plineOldVar;
    REGISTER PLW_LIST_LINE          plineNewVar;
    REGISTER PLW_CLASS_THREADVAR    pthreadvar;
    
    REGISTER ULONG  ulSwitch;
    
    if (_Thread_Exist(ptcbOld)) {                                       /*  旧线程可能不复存在          */
        for (plineOldVar  = ptcbOld->TCB_plinePrivateVars;              /*  获得旧线程私有变量          */
             plineOldVar != LW_NULL; 
             plineOldVar  = _list_line_get_next(plineOldVar)) {
        
            pthreadvar = _LIST_ENTRY(plineOldVar, LW_CLASS_THREADVAR, PRIVATEVAR_lineVarList);
            ulSwitch   = pthreadvar->PRIVATEVAR_ulValueSave;
            pthreadvar->PRIVATEVAR_ulValueSave = *pthreadvar->PRIVATEVAR_pulAddress;
            *pthreadvar->PRIVATEVAR_pulAddress = ulSwitch;
        }
    }
    
    for (plineNewVar  = ptcbNew->TCB_plinePrivateVars;
         plineNewVar != LW_NULL; 
         plineNewVar  = _list_line_get_next(plineNewVar)) {             /*  获得新线程私有变量          */
         
        pthreadvar = _LIST_ENTRY(plineNewVar, LW_CLASS_THREADVAR, PRIVATEVAR_lineVarList);
        ulSwitch   = pthreadvar->PRIVATEVAR_ulValueSave;
        pthreadvar->PRIVATEVAR_ulValueSave = *pthreadvar->PRIVATEVAR_pulAddress;
        *pthreadvar->PRIVATEVAR_pulAddress = ulSwitch;
    }
}
/*********************************************************************************************************
** 函数名称: _ThreadVarSave.
** 功能描述: 线程内部全局变量切换
** 输　入  : ptcbCur       当前正在执行的线程
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SMP_CPU_DOWN_EN > 0

VOID  _ThreadVarSave (PLW_CLASS_TCB  ptcbCur)
{
    REGISTER PLW_LIST_LINE          plineCurVar;
    REGISTER PLW_CLASS_THREADVAR    pthreadvar;
    
    REGISTER ULONG  ulSwitch;
    
    if (_Thread_Exist(ptcbCur)) {                                       /*  旧线程可能不复存在          */
        for (plineCurVar  = ptcbCur->TCB_plinePrivateVars;              /*  获得旧线程私有变量          */
             plineCurVar != LW_NULL; 
             plineCurVar  = _list_line_get_next(plineCurVar)) {
        
            pthreadvar = _LIST_ENTRY(plineCurVar, LW_CLASS_THREADVAR, PRIVATEVAR_lineVarList);
            ulSwitch   = pthreadvar->PRIVATEVAR_ulValueSave;
            pthreadvar->PRIVATEVAR_ulValueSave = *pthreadvar->PRIVATEVAR_pulAddress;
            *pthreadvar->PRIVATEVAR_pulAddress = ulSwitch;
        }
    }
}

#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
#endif                                                                  /*  LW_CFG_SMP_EN == 0          */
                                                                        /*  (LW_CFG_THREAD_PRIVATE_VA...*/
                                                                        /*  (LW_CFG_MAX_THREAD_GLB_VA...*/
/*********************************************************************************************************
  END
*********************************************************************************************************/
