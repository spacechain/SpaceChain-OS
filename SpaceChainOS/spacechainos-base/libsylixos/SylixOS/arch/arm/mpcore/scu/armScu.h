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
** 文   件   名: armScu.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 01 月 03 日
**
** 描        述: ARM SNOOP CONTROL UNIT.
*********************************************************************************************************/

#ifndef __ARMSCU_H
#define __ARMSCU_H

/*********************************************************************************************************
  寄存器定义
*********************************************************************************************************/

#define SCU_FEATURE_IC_STANDBY                     (1 << 6)
#define SCU_FEATURE_SCU_STANDBY                    (1 << 5)
#define SCU_FEATURE_ALL_DEV_TO_PORT0               (1 << 4)
#define SCU_FEATURE_SCU_SPECULATIVE_LINEFILL       (1 << 3)
#define SCU_FEATURE_SCU_RAMS_PARITY                (1 << 2)
#define SCU_FEATURE_ADDRESS_FILTERING              (1 << 1)
#define SCU_FEATURE_SCU                            (1 << 0)

VOID    armScuFeatureEnable(UINT32  uiFeatures);
VOID    armScuFeatureDisable(UINT32  uiFeatures);
UINT32  armScuFeatureGet(VOID);
UINT32  armScuTagRamSize(VOID);
UINT32  armScuCpuMpStatus(VOID);
UINT32  armScuCpuNumber(VOID);
VOID    armScuSecureInvalidateAll(UINT32  uiCPUId,  UINT32  uiWays);
VOID    armScuFilteringSet(UINT32  uiStart,  UINT32  uiEnd);
VOID    armScuAccessCtrlSet(UINT32  uiCpuBits);
VOID    armScuNonAccessCtrlSet(UINT32  uiValue);
UINT32  armScuCpuPowerStatusGet(VOID);

#endif                                                                  /*  __ARMSCU_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
