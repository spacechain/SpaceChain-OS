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
** 文   件   名: ppcL2.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 01 月 28 日
**
** 描        述: PowerPC 体系构架 L2 CACHE 驱动.
*********************************************************************************************************/

#ifndef __ARCH_PPCL2_H
#define __ARCH_PPCL2_H

/*********************************************************************************************************
  裁减配置
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_PPC_CACHE_L2 > 0

/*********************************************************************************************************
  L2 cache contorller register
*********************************************************************************************************/
/*********************************************************************************************************
  L2 cache driver struct
*********************************************************************************************************/

typedef struct {
    CPCHAR          L2CD_pcName;                                        /*  L2 CACHE 控制器名称         */
    UINT32          L2CD_uiType;                                        /*  L2 CACHE 控制器类型         */
    UINT32          L2CD_uiRelease;                                     /*  L2 CACHE 控制器子版本       */
    size_t          L2CD_stSize;                                        /*  L2 CACHE 大小               */
    
    VOIDFUNCPTR     L2CD_pfuncEnable;
    VOIDFUNCPTR     L2CD_pfuncDisable;
    BOOLFUNCPTR     L2CD_pfuncIsEnable;
    VOIDFUNCPTR     L2CD_pfuncSync;
    VOIDFUNCPTR     L2CD_pfuncFlush;
    VOIDFUNCPTR     L2CD_pfuncFlushAll;
    VOIDFUNCPTR     L2CD_pfuncInvalidate;
    VOIDFUNCPTR     L2CD_pfuncInvalidateAll;
    VOIDFUNCPTR     L2CD_pfuncClear;
    VOIDFUNCPTR     L2CD_pfuncClearAll;
} L2C_DRVIER;

/*********************************************************************************************************
  初始化
*********************************************************************************************************/

VOID    ppcL2Init(CACHE_MODE   uiInstruction,
                  CACHE_MODE   uiData,
                  CPCHAR       pcMachineName);
CPCHAR  ppcL2Name(VOID);
VOID    ppcL2Enable(VOID);
VOID    ppcL2Disable(VOID);
BOOL    ppcL2IsEnable(VOID);
VOID    ppcL2Sync(VOID);
VOID    ppcL2FlushAll(VOID);
VOID    ppcL2Flush(PVOID  pvPdrs, size_t  stBytes);
VOID    ppcL2InvalidateAll(VOID);
VOID    ppcL2Invalidate(PVOID  pvPdrs, size_t  stBytes);
VOID    ppcL2ClearAll(VOID);
VOID    ppcL2Clear(PVOID  pvPdrs, size_t  stBytes);

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                                                                        /*  LW_CFG_PPC_CACHE_L2 > 0     */
#endif                                                                  /*  __ARCH_PPCL2_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
