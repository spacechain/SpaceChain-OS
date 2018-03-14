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
** 文   件   名: busSystem.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 10 月 27 日
**
** 描        述: 总线系统管理 (目前仅限于打印信息).
*********************************************************************************************************/

#ifndef __BUSSYSTEM_H
#define __BUSSYSTEM_H

/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

#ifdef    __SYLIXOS_KERNEL
/*********************************************************************************************************
  总线适配器定义 (必须作为所有类型总线适配器的首元素)
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE             BUSADAPTER_lineManage;                     /*  管理链表                    */
    atomic_t                 BUSADAPTER_atomicCounter;                  /*  设备计数节点                */
    CHAR                     BUSADAPTER_cName[LW_CFG_OBJECT_NAME_SIZE]; /*  适配器名字                  */
} LW_BUS_ADAPTER;
typedef LW_BUS_ADAPTER      *PLW_BUS_ADAPTER;                           /*  总线适配器                  */

static LW_INLINE INT  LW_BUS_INC_DEV_COUNT(PLW_BUS_ADAPTER  pbusadapter)
{
    return  ((pbusadapter) ? (API_AtomicInc(&pbusadapter->BUSADAPTER_atomicCounter)) : (PX_ERROR));
}

static LW_INLINE INT  LW_BUS_DEC_DEV_COUNT(PLW_BUS_ADAPTER  pbusadapter)
{
    return  ((pbusadapter) ? (API_AtomicDec(&pbusadapter->BUSADAPTER_atomicCounter)) : (PX_ERROR));
}

static LW_INLINE INT  LW_BUS_GET_DEV_COUNT(PLW_BUS_ADAPTER  pbusadapter)
{
    return  ((pbusadapter) ? (API_AtomicGet(&pbusadapter->BUSADAPTER_atomicCounter)) : (PX_ERROR));
}

/*********************************************************************************************************
  总线基本操作
*********************************************************************************************************/

VOID                __busSystemInit(VOID);                              /*  总线系统初始化              */

/*********************************************************************************************************
  总线基本操作
*********************************************************************************************************/

INT                 __busAdapterCreate(PLW_BUS_ADAPTER  pbusadapter, CPCHAR  pcName);
INT                 __busAdapterDelete(CPCHAR  pcName);
PLW_BUS_ADAPTER     __busAdapterGet(CPCHAR  pcName);

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  总线 API 操作
*********************************************************************************************************/

LW_API VOID         API_BusShow(VOID);                                  /*  打印总线信息                */

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
#endif                                                                  /*  __BUSSYSTEM_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
