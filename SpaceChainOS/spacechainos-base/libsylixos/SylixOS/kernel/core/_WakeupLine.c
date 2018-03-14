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
** 文   件   名: _WakeupLine.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 03 月 29 日
**
** 描        述: 这是系统等待唤醒链表操作函数 (调用以下函数时, 一定要在内核状态)

** BUG:
2010.01.22  昨晚观看了 3D 版 AVATAR. 和 TITANIC 一样经典!
            将队列改为 FIFO 形式 (由于历史原因, 暂不改为环表)
2013.09.03  使用新的差分时间链表唤醒队列机制.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _WakeupAdd
** 功能描述: 将一个 wakeup 节点加入管理器
** 输　入  : pwu           wakeup 管理器
**           pwun          节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _WakeupAdd (PLW_CLASS_WAKEUP  pwu, PLW_CLASS_WAKEUP_NODE  pwun)
{
    PLW_LIST_LINE           plineTemp;
    PLW_CLASS_WAKEUP_NODE   pwunTemp;
    
    plineTemp = pwu->WU_plineHeader;
    while (plineTemp) {
        pwunTemp = _LIST_ENTRY(plineTemp, LW_CLASS_WAKEUP_NODE, WUN_lineManage);
        if (pwun->WUN_ulCounter >= pwunTemp->WUN_ulCounter) {           /*  需要继续向后找              */
            pwun->WUN_ulCounter -= pwunTemp->WUN_ulCounter;
            plineTemp = _list_line_get_next(plineTemp);
        
        } else {
            if (plineTemp == pwu->WU_plineHeader) {                     /*  如果是链表头                */
                _List_Line_Add_Ahead(&pwun->WUN_lineManage, &pwu->WU_plineHeader);
            } else {
                _List_Line_Add_Left(&pwun->WUN_lineManage, plineTemp);  /*  不是表头则插在左边          */
            }
            pwunTemp->WUN_ulCounter -= pwun->WUN_ulCounter;             /*  右侧的点从新就算计数器      */
            break;
        }
    }
    
    if (plineTemp == LW_NULL) {
        if (pwu->WU_plineHeader == LW_NULL) {
            _List_Line_Add_Ahead(&pwun->WUN_lineManage, &pwu->WU_plineHeader);
        } else {
            _List_Line_Add_Right(&pwun->WUN_lineManage, &pwunTemp->WUN_lineManage);
        }
    }
    
    pwun->WUN_bInQ = LW_TRUE;
}
/*********************************************************************************************************
** 函数名称: _WakeupDel
** 功能描述: 从 wakeup 管理器中删除指定节点
** 输　入  : pwu           wakeup 管理器
**           pwun          节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _WakeupDel (PLW_CLASS_WAKEUP  pwu, PLW_CLASS_WAKEUP_NODE  pwun)
{
    PLW_LIST_LINE           plineRight;
    PLW_CLASS_WAKEUP_NODE   pwunRight;

    if (&pwun->WUN_lineManage == pwu->WU_plineOp) {
        pwu->WU_plineOp = _list_line_get_next(pwu->WU_plineOp);
    }
    
    plineRight = _list_line_get_next(&pwun->WUN_lineManage);
    if (plineRight) {
        pwunRight = _LIST_ENTRY(plineRight, LW_CLASS_WAKEUP_NODE, WUN_lineManage);
        pwunRight->WUN_ulCounter += pwun->WUN_ulCounter;
    }
    
    _List_Line_Del(&pwun->WUN_lineManage, &pwu->WU_plineHeader);
    pwun->WUN_bInQ = LW_FALSE;
}
/*********************************************************************************************************
** 函数名称: _WakeupStatus
** 功能描述: 获得指定节点等待信息
** 输　入  : pwu           wakeup 管理器
**           pwun          节点
**           pulLeft       剩余时间
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _WakeupStatus (PLW_CLASS_WAKEUP  pwu, PLW_CLASS_WAKEUP_NODE  pwun, ULONG  *pulLeft)
{
    PLW_LIST_LINE           plineTemp;
    PLW_CLASS_WAKEUP_NODE   pwunTemp;
    ULONG                   ulCounter = 0;
    
    for (plineTemp  = pwu->WU_plineHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pwunTemp   = _LIST_ENTRY(plineTemp, LW_CLASS_WAKEUP_NODE, WUN_lineManage);
        ulCounter += pwunTemp->WUN_ulCounter;
        if (pwunTemp == pwun) {
            break;
        }
    }
    
    if (plineTemp) {
        *pulLeft = ulCounter;
    
    } else {
        *pulLeft = 0ul;                                                 /*  没有找到节点                */
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
