/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: armL2A8.c
**
** 创   建   人: Jiao.Jinxing (焦进星)
**
** 文件创建日期: 2014 年 05 月 11 日
**
** 描        述: ARM Cortex A8 体系构架 L2 CACHE 控制器驱动.
*********************************************************************************************************/
#define  __SYLIXOS_IO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁减配置
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_ARM_CACHE_L2 > 0
#include "armL2.h"
#include "../../../common/cp15/armCp15.h"
/*********************************************************************************************************
** 函数名称: armL2A8Enable
** 功能描述: 使能 L2 CACHE 控制器
** 输　入  : pl2cdrv            驱动结构
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 如果有 lockdown 必须首先 unlock & invalidate 才能启动 L2.
*********************************************************************************************************/
static VOID armL2A8Enable (L2C_DRVIER  *pl2cdrv)
{
    armAuxControlFeatureEnable(AUX_CTRL_A8_L2EN | AUX_CTRL_A8_IBE);
}
/*********************************************************************************************************
** 函数名称: armL2A8Disable
** 功能描述: 禁能 L2 CACHE 控制器
** 输　入  : pl2cdrv            驱动结构
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID armL2A8Disable (L2C_DRVIER  *pl2cdrv)
{
    armAuxControlFeatureDisable(AUX_CTRL_A8_L2EN);
}
/*********************************************************************************************************
** 函数名称: armL2A8IsEnable
** 功能描述: 检查 L2 CACHE 控制器是否使能
** 输　入  : pl2cdrv            驱动结构
** 输　出  : 是否使能
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL armL2A8IsEnable (L2C_DRVIER  *pl2cdrv)
{
    UINT32  uiAux = armCp15AuxCtrlReg();
    
    return  ((uiAux & AUX_CTRL_A8_L2EN) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: armL2A8Init
** 功能描述: 初始化 L2 CACHE 控制器
** 输　入  : pl2cdrv            驱动结构
**           uiInstruction      指令 CACHE 类型
**           uiData             数据 CACHE 类型
**           pcMachineName      机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID armL2A8Init (L2C_DRVIER  *pl2cdrv,
                  CACHE_MODE   uiInstruction,
                  CACHE_MODE   uiData,
                  CPCHAR       pcMachineName)
{
    pl2cdrv->L2CD_pfuncEnable        = armL2A8Enable;
    pl2cdrv->L2CD_pfuncDisable       = armL2A8Disable;
    pl2cdrv->L2CD_pfuncIsEnable      = armL2A8IsEnable;
    pl2cdrv->L2CD_pfuncSync          = LW_NULL;
    pl2cdrv->L2CD_pfuncFlush         = LW_NULL;
    pl2cdrv->L2CD_pfuncFlushAll      = LW_NULL;
    pl2cdrv->L2CD_pfuncInvalidate    = LW_NULL;
    pl2cdrv->L2CD_pfuncInvalidateAll = LW_NULL;
    pl2cdrv->L2CD_pfuncClear         = LW_NULL;
    pl2cdrv->L2CD_pfuncClearAll      = LW_NULL;
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                                                                        /*  LW_CFG_ARM_CACHE_L2 > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
