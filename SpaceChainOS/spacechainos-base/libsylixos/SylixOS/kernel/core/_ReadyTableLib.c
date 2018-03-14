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
** 文   件   名: _ReadyTableLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 11 月 11 日
**
** 描        述: 这是系统就绪表操作函数库。
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _ReadyTableAdd
** 功能描述: 指定线程加入就绪表
** 输　入  : ptcb      线程控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ReadyTableAdd (PLW_CLASS_TCB  ptcb)
{
    PLW_CLASS_PCBBMAP   ppcbbmap;
    
#if LW_CFG_SMP_EN > 0
    if (ptcb->TCB_bCPULock) {
        ppcbbmap = LW_CPU_RDY_PCBBMAP(LW_CPU_GET(ptcb->TCB_ulCPULock));
    } else 
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */    
    {
        ppcbbmap = LW_GLOBAL_RDY_PCBBMAP();
    }
    
    _BitmapAdd(&ppcbbmap->PCBM_bmap, ptcb->TCB_ucPriority);
}
/*********************************************************************************************************
** 函数名称: _ReadyTableDel
** 功能描述: 指定线程退出就绪表
** 输　入  : ptcb      线程控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ReadyTableDel (PLW_CLASS_TCB  ptcb)
{
    PLW_CLASS_PCBBMAP   ppcbbmap;
    
#if LW_CFG_SMP_EN > 0
    if (ptcb->TCB_bCPULock) {
        ppcbbmap = LW_CPU_RDY_PCBBMAP(LW_CPU_GET(ptcb->TCB_ulCPULock));
    } else 
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */    
    {
        ppcbbmap = LW_GLOBAL_RDY_PCBBMAP();
    }
    
    _BitmapDel(&ppcbbmap->PCBM_bmap, ptcb->TCB_ucPriority);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
