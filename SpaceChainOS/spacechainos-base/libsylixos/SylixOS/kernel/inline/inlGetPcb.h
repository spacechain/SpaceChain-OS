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
** 文   件   名: inlGetPcb.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 18 日
**
** 描        述: 获得优先级控制块
*********************************************************************************************************/

#ifndef  __INLGETPCB_H
#define  __INLGETPCB_H

/*********************************************************************************************************
  获取优先级控制块
*********************************************************************************************************/

static LW_INLINE PLW_CLASS_PCB  _GetPcb (PLW_CLASS_TCB  ptcb)
{
#if LW_CFG_SMP_EN > 0
    if (ptcb->TCB_bCPULock) {                                           /*  锁定 CPU                    */
        return  (LW_CPU_RDY_PPCB(LW_CPU_GET(ptcb->TCB_ulCPULock), ptcb->TCB_ucPriority));
    } else 
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    {
        return  (LW_GLOBAL_RDY_PPCB(ptcb->TCB_ucPriority));
    }
}

static LW_INLINE PLW_CLASS_PCB  _GetPcbEx (PLW_CLASS_TCB  ptcb, UINT8  ucPriority)
{
#if LW_CFG_SMP_EN > 0
    if (ptcb->TCB_bCPULock) {                                           /*  锁定 CPU                    */
        return  (LW_CPU_RDY_PPCB(LW_CPU_GET(ptcb->TCB_ulCPULock), ucPriority));
    } else 
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    {
        return  (LW_GLOBAL_RDY_PPCB(ucPriority));
    }
}

/*********************************************************************************************************
  优先级控制块状态判断
*********************************************************************************************************/

static LW_INLINE BOOL  _PcbIsEmpty (PLW_CLASS_PCB  ppcb)
{
    return  (ppcb->PCB_pringReadyHeader ? LW_FALSE : LW_TRUE);
}

static LW_INLINE BOOL  _PcbIsOne (PLW_CLASS_PCB  ppcb)
{
    if (ppcb->PCB_pringReadyHeader) {
        if (ppcb->PCB_pringReadyHeader == 
            _list_ring_get_next(ppcb->PCB_pringReadyHeader)) {
            return  (LW_TRUE);    
        }
    }
    
    return  (LW_FALSE);
}

#endif                                                                  /*  __INLGETPCB_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
