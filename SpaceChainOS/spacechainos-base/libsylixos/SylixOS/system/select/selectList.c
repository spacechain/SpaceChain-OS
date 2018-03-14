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
** 文   件   名: selectList.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 11 月 07 日
**
** 描        述:  IO 系统 select 子系统 WAKEUP 链表操作基础.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)
#include "select.h"
/*********************************************************************************************************
  WAKE UP LIST 锁属性
*********************************************************************************************************/
#define __WAKEUPLIST_LOCK_OPTION        (LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |  \
                                         LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL)
/*********************************************************************************************************
** 函数名称: API_SelWakeupListInit
** 功能描述: 初始化 WAKEUP LIST 链表控制结构
** 输　入  : pselwulList        select wake up list 控制结构
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID    API_SelWakeupListInit (PLW_SEL_WAKEUPLIST  pselwulList)
{
    if (!pselwulList) {                                                 /*  检查指针                    */
        return;
    }
    
    pselwulList->SELWUL_hListLock = API_SemaphoreMCreate("sellist_lock", 
                                    LW_PRIO_DEF_CEILING, __WAKEUPLIST_LOCK_OPTION, 
                                    LW_NULL);                           /*  建立锁信号量                */
                                    
    pselwulList->SELWUL_ulWakeCounter = 0;                              /*  没有等待的线程              */
    pselwulList->SELWUL_plineHeader   = LW_NULL;                        /*  初始化链表头                */
}
/*********************************************************************************************************
** 函数名称: API_SelWakeupListTerm
** 功能描述: 中止指定的 WAKEUP LIST 链表控制结构
** 输　入  : pselwulList        select wake up list 控制结构
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 单核优先级 0 的 exc 任务会优先运行, 所以不需要清除 exc 工作队列的任务, 多核系统可能会与本函数
             同时执行, 所以确保安全期间, 需要删除掉队列内部关于本 select list 的操作.
             
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID    API_SelWakeupListTerm (PLW_SEL_WAKEUPLIST  pselwulList)
{
    if (!pselwulList) {                                                 /*  检查指针                    */
        return;
    }
    
#if LW_CFG_SMP_EN > 0
    _excJobDel(2, (VOIDFUNCPTR)API_SelWakeupAll, (PVOID)pselwulList,
               0, 0, 0, 0, 0);
#endif                                                                  /*  LW_CFG_SMP_EN               */
               
    API_SelWakeupTerm(pselwulList);                                     /*  删除内部缓存的节点          */
    
    API_SemaphoreMDelete(&pselwulList->SELWUL_hListLock);               /*  删除信号量                  */
}
/*********************************************************************************************************
** 函数名称: API_SelWakeupListLen
** 功能描述: 获得指定的 WAKEUP LIST 链表控制结构等待线程的数量
** 输　入  : pselwulList        select wake up list 控制结构
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
UINT    API_SelWakeupListLen (PLW_SEL_WAKEUPLIST  pselwulList)
{
    if (!pselwulList) {                                                 /*  检查指针                    */
        return  (0);
    }
    
    return  (pselwulList->SELWUL_ulWakeCounter);                        /*  返回等待线程的数量          */
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SELECT_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
