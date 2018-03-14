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
** 文   件   名: armL2.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 09 日
**
** 描        述: ARM 体系构架 L2 CACHE 驱动.
*********************************************************************************************************/

#ifndef __ARML2_H
#define __ARML2_H

/*********************************************************************************************************
  裁减配置
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_ARM_CACHE_L2 > 0

/*********************************************************************************************************
  L2 cache contorller register
*********************************************************************************************************/

#define L2C_CACHE_ID            0x000
#define L2C_CACHE_TYPE          0x004
#define L2C_CTRL                0x100
#define L2C_AUX_CTRL            0x104
#define L2C_TAG_LATENCY_CTRL    0x108
#define L2C_DATA_LATENCY_CTRL   0x10C
#define L2C_EVENT_CNT_CTRL      0x200
#define L2C_EVENT_CNT1_CFG      0x204
#define L2C_EVENT_CNT0_CFG      0x208
#define L2C_EVENT_CNT1_VAL      0x20C
#define L2C_EVENT_CNT0_VAL      0x210
#define L2C_INTR_MASK           0x214
#define L2C_MASKED_INTR_STAT    0x218
#define L2C_RAW_INTR_STAT       0x21C
#define L2C_INTR_CLEAR          0x220
#define L2C_CACHE_SYNC          0x730
#define L2C_DUMMY_REG           0x740
#define L2C_INV_LINE_PA         0x770
#define L2C_INV_WAY             0x77C
#define L2C_CLEAN_LINE_PA       0x7B0
#define L2C_CLEAN_LINE_IDX      0x7B8
#define L2C_CLEAN_WAY           0x7BC
#define L2C_CLEAN_INV_LINE_PA   0x7F0
#define L2C_CLEAN_INV_LINE_IDX  0x7F8
#define L2C_CLEAN_INV_WAY       0x7FC

#define L2C_D_LOCKDOWN0         0x900
#define L2C_I_LOCKDOWN0         0x904
#define L2C_D_LOCKDOWN1         0x908
#define L2C_I_LOCKDOWN1         0x90c
#define L2C_D_LOCKDOWN2         0x910
#define L2C_I_LOCKDOWN2         0x914
#define L2C_D_LOCKDOWN3         0x918
#define L2C_I_LOCKDOWN3         0x91c
#define L2C_D_LOCKDOWN4         0x920
#define L2C_I_LOCKDOWN4         0x924
#define L2C_D_LOCKDOWN5         0x928
#define L2C_I_LOCKDOWN5         0x92c
#define L2C_D_LOCKDOWN6         0x930
#define L2C_I_LOCKDOWN6         0x934
#define L2C_D_LOCKDOWN7         0x938
#define L2C_I_LOCKDOWN7         0x93c
#define L2C_LOCK_LINE_EN        0x950
#define L2C_UNLOCK_WAY          0x954

#define L2C_ADDR_FILTER_START   0xc00
#define L2C_ADDR_FILTER_END     0xc04

#define L2C_DEBUG_CTL           0xf40
#define L2C_PREFETCH_CTL        0xf60
#define L2C_POWER_CTL           0xf80

#define L2C_WAY_SIZE_SHIFT      3

#define L2C_AUX_CTRL_MASK                   0xc0000fff
#define L2C_AUX_CTRL_DATA_RD_LATENCY_SHIFT  0
#define L2C_AUX_CTRL_DATA_RD_LATENCY_MASK   0x7
#define L2C_AUX_CTRL_DATA_WR_LATENCY_SHIFT  3
#define L2C_AUX_CTRL_DATA_WR_LATENCY_MASK   (0x7 << 3)
#define L2C_AUX_CTRL_TAG_LATENCY_SHIFT      6
#define L2C_AUX_CTRL_TAG_LATENCY_MASK       (0x7 << 6)
#define L2C_AUX_CTRL_DIRTY_LATENCY_SHIFT    9
#define L2C_AUX_CTRL_DIRTY_LATENCY_MASK     (0x7 << 9)
#define L2C_AUX_CTRL_ASSOCIATIVITY_SHIFT    16
#define L2C_AUX_CTRL_WAY_SIZE_SHIFT         17
#define L2C_AUX_CTRL_WAY_SIZE_MASK          (0x7 << 17)
#define L2C_AUX_CTRL_SHARE_OVERRIDE_SHIFT   22
#define L2C_AUX_CTRL_NS_LOCKDOWN_SHIFT      26
#define L2C_AUX_CTRL_NS_INT_CTRL_SHIFT      27
#define L2C_AUX_CTRL_DATA_PREFETCH_SHIFT    28
#define L2C_AUX_CTRL_INSTR_PREFETCH_SHIFT   29
#define L2C_AUX_CTRL_EARLY_BRESP_SHIFT      30

/*********************************************************************************************************
  L2 cache driver struct
*********************************************************************************************************/

typedef struct {
    CPCHAR          L2CD_pcName;                                        /*  L2 CACHE 控制器名称         */
    addr_t          L2CD_ulBase;                                        /*  寄存器基地址                */
    UINT32          L2CD_uiWayMask;                                     /*  路索引掩码                  */
    UINT32          L2CD_uiAux;
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

#define L2C_BASE(pl2cdrv)       ((pl2cdrv)->L2CD_ulBase)
#define L2C_WAYMASK(pl2cdrv)    ((pl2cdrv)->L2CD_uiWayMask)
#define L2C_AUX(pl2cdrv)        ((pl2cdrv)->L2CD_uiAux)

/*********************************************************************************************************
  初始化
*********************************************************************************************************/

VOID    armL2Init(CACHE_MODE   uiInstruction,
                  CACHE_MODE   uiData,
                  CPCHAR       pcMachineName);
CPCHAR  armL2Name(VOID);
VOID    armL2Enable(VOID);
VOID    armL2Disable(VOID);
BOOL    armL2IsEnable(VOID);
VOID    armL2Sync(VOID);
VOID    armL2FlushAll(VOID);
VOID    armL2Flush(PVOID  pvPdrs, size_t  stBytes);
VOID    armL2InvalidateAll(VOID);
VOID    armL2Invalidate(PVOID  pvPdrs, size_t  stBytes);
VOID    armL2ClearAll(VOID);
VOID    armL2Clear(PVOID  pvPdrs, size_t  stBytes);

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                                                                        /*  LW_CFG_ARM_CACHE_L2 > 0     */
#endif                                                                  /*  __ARML2_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
