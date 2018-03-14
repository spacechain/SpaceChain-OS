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
** 文   件   名: pmAdapter.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 07 月 19 日
**
** 描        述: 电源管理适配器.
*********************************************************************************************************/

#ifndef __PMADAPTER_H
#define __PMADAPTER_H

/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0

#ifdef  __POWERM_MAIN_FILE
       LW_OBJECT_HANDLE     _G_ulPowerMLock;
       LW_LIST_LINE_HEADER  _G_plinePMAdapter;
#else
extern LW_OBJECT_HANDLE     _G_ulPowerMLock;
extern LW_LIST_LINE_HEADER  _G_plinePMAdapter;
#endif                                                                  /*  __POWERM_MAIN_FILE          */

#define __POWERM_LOCK()     API_SemaphoreMPend(_G_ulPowerMLock, LW_OPTION_WAIT_INFINITE)
#define __POWERM_UNLOCK()   API_SemaphoreMPost(_G_ulPowerMLock)

/*********************************************************************************************************
  驱动程序调用接口
*********************************************************************************************************/

LW_API PLW_PM_ADAPTER  API_PowerMAdapterCreate(CPCHAR  pcName, UINT  uiMaxChan, PLW_PMA_FUNCS  pmafuncs);
LW_API INT             API_PowerMAdapterDelete(PLW_PM_ADAPTER  pmadapter);
LW_API PLW_PM_ADAPTER  API_PowerMAdapterFind(CPCHAR  pcName);

#define pmAdapterCreate     API_PowerMAdapterCreate
#define pmAdapterDelete     API_PowerMAdapterDelete
#define pmAdapterFind       API_PowerMAdapterFind

#endif                                                                  /*  LW_CFG_POWERM_EN            */
#endif                                                                  /*  __PMADAPTER_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
