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
** 文   件   名: arch_mpu.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 11 月 08 日
**
** 描        述: ARM MPU 管理相关.
*********************************************************************************************************/

#ifndef __ARM_ARCH_MPU_H
#define __ARM_ARCH_MPU_H

#if (LW_CFG_VMM_EN == 0) && (LW_CFG_ARM_MPU > 0)
/*********************************************************************************************************
  映射描述符
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct {
    BOOL        MPUD_bEnable;                                   /*  Enable                              */
    BOOL        MPUD_bDmaPool;                                  /*  DMA buffer pool                     */

    UINT32      MPUD_uiAddr;                                    /*  Start address                       */
    UINT8       MPUD_ucSize;                                    /*  Region size                         */
#define ARM_MPU_REGION_SIZE_END         ((UINT8)0x00)           /*  The last one                        */
#define ARM_MPU_REGION_SIZE_32B         ((UINT8)0x04)
#define ARM_MPU_REGION_SIZE_64B         ((UINT8)0x05)
#define ARM_MPU_REGION_SIZE_128B        ((UINT8)0x06)
#define ARM_MPU_REGION_SIZE_256B        ((UINT8)0x07)
#define ARM_MPU_REGION_SIZE_512B        ((UINT8)0x08)
#define ARM_MPU_REGION_SIZE_1KB         ((UINT8)0x09)
#define ARM_MPU_REGION_SIZE_2KB         ((UINT8)0x0a)
#define ARM_MPU_REGION_SIZE_4KB         ((UINT8)0x0b)
#define ARM_MPU_REGION_SIZE_8KB         ((UINT8)0x0c)
#define ARM_MPU_REGION_SIZE_16KB        ((UINT8)0x0d)
#define ARM_MPU_REGION_SIZE_32KB        ((UINT8)0x0e)
#define ARM_MPU_REGION_SIZE_64KB        ((UINT8)0x0f)
#define ARM_MPU_REGION_SIZE_128KB       ((UINT8)0x10)
#define ARM_MPU_REGION_SIZE_256KB       ((UINT8)0x11)
#define ARM_MPU_REGION_SIZE_512KB       ((UINT8)0x12)
#define ARM_MPU_REGION_SIZE_1MB         ((UINT8)0x13)
#define ARM_MPU_REGION_SIZE_2MB         ((UINT8)0x14)
#define ARM_MPU_REGION_SIZE_4MB         ((UINT8)0x15)
#define ARM_MPU_REGION_SIZE_8MB         ((UINT8)0x16)
#define ARM_MPU_REGION_SIZE_16MB        ((UINT8)0x17)
#define ARM_MPU_REGION_SIZE_32MB        ((UINT8)0x18)
#define ARM_MPU_REGION_SIZE_64MB        ((UINT8)0x19)
#define ARM_MPU_REGION_SIZE_128MB       ((UINT8)0x1a)
#define ARM_MPU_REGION_SIZE_256MB       ((UINT8)0x1b)
#define ARM_MPU_REGION_SIZE_512MB       ((UINT8)0x1c)
#define ARM_MPU_REGION_SIZE_1GB         ((UINT8)0x1d)
#define ARM_MPU_REGION_SIZE_2GB         ((UINT8)0x1e)
#define ARM_MPU_REGION_SIZE_4GB         ((UINT8)0x1f)

    UINT8       MPUD_ucNumber;                                  /*  Region Number                       */
    UINT8       MPUD_ucIsIns;                                   /*  Only for Separated MPU!             */

    UINT32      MPUD_uiAP;                                      /*  Access permission                   */
#define ARM_MPU_AP_P_NA_U_NA    0x00000000                      /*  P: Non Access U: Non Access         */
#define ARM_MPU_AP_P_RW_U_NA    0x00000001                      /*  P: Read Write U: Non Access         */
#define ARM_MPU_AP_P_RW_U_RO    0x00000002                      /*  P: Read Write U: Read only          */
#define ARM_MPU_AP_P_RW_U_RW    0x00000003                      /*  P: Read Write U: Read Write         */
#define ARM_MPU_AP_P_RO_U_NA    0x00000005                      /*  P: Read only  U: Non Access         */
#define ARM_MPU_AP_P_RO_U_RO    0x00000006                      /*  P: Read only  U: Read only          */
#define ARM_MPU_AP_FLAG_EXEC    0x80000000                      /*  Can execute                         */

    UINT32      MPUD_uiCPS;                                     /*  Cache properties & shareability     */
#define ARM_MPU_CPS_STRONG_ORDER_S      0                       /*  Strongly Ordered (S)                */
#define ARM_MPU_CPS_DEVICE              1                       /*  Device                              */
#define ARM_MPU_CPS_DEVICE_S            2                       /*  Device (S)                          */
#define ARM_MPU_CPS_NORMAL_WT           3                       /*  Write through, no write allocate    */
#define ARM_MPU_CPS_NORMAL_WT_S         4                       /*  Write through, no write allocate (S)*/
#define ARM_MPU_CPS_NORMAL_WB           5                       /*  Write back, no write allocate       */
#define ARM_MPU_CPS_NORMAL_WB_S         6                       /*  Write back, no write allocate (S)   */
#define ARM_MPU_CPS_NORMAL_NC           7                       /*  Non-cacheable                       */
#define ARM_MPU_CPS_NORMAL_NC_S         8                       /*  Non-cacheable (S)                   */
#define ARM_MPU_CPS_NORMAL_WBA          9                       /*  Write back, W & R allocate          */
#define ARM_MPU_CPS_NORMAL_WBA_S        10                      /*  Write back, W & R allocate (S)      */

} ARM_MPU_REGION;
typedef ARM_MPU_REGION    *PARM_MPU_REGION;

#endif                                                          /*  __SYLIXOS_KERNEL                    */
#endif                                                          /*  LW_CFG_ARM_MPU                      */
#endif                                                          /*  __ARM_ARCH_MPU_H                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
