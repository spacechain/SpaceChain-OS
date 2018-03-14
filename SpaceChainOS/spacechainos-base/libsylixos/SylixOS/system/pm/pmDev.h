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
** 文   件   名: pmDev.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 07 月 19 日
**
** 描        述: 电源管理设备接口.
*********************************************************************************************************/

#ifndef __PMDEV_H
#define __PMDEV_H

/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0

#ifdef  __POWERM_MAIN_FILE
        LW_LIST_LINE_HEADER  _G_plinePMDev;
#else
extern  LW_LIST_LINE_HEADER  _G_plinePMDev;
#endif                                                                  /*  __POWERM_MAIN_FILE          */

/*********************************************************************************************************
  驱动程序调用接口
*********************************************************************************************************/

LW_API INT  API_PowerMDevInit(PLW_PM_DEV  pmdev,  PLW_PM_ADAPTER  pmadapter, 
                              UINT        uiChan, PLW_PMD_FUNCS   pmdfunc);
LW_API INT  API_PowerMDevTerm(PLW_PM_DEV  pmdev);
LW_API INT  API_PowerMDevOn(PLW_PM_DEV  pmdev);
LW_API INT  API_PowerMDevOff(PLW_PM_DEV  pmdev);

#define pmDevInit       API_PowerMDevInit
#define pmDevTerm       API_PowerMDevTerm
#define pmDevOn         API_PowerMDevOn
#define pmDevOff        API_PowerMDevOff

#endif                                                                  /*  LW_CFG_POWERM_EN            */
#endif                                                                  /*  __PMDEV_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
