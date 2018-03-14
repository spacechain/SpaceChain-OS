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
** 文   件   名: arch_mmu.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 11 月 26 日
**
** 描        述: PowerPC 内存管理相关.
*********************************************************************************************************/

#ifndef __PPC_ARCH_MMU_H
#define __PPC_ARCH_MMU_H

/*********************************************************************************************************
  L4 微内核虚拟机 MMU
*********************************************************************************************************/

#define LW_CFG_VMM_L4_HYPERVISOR_EN           0                         /*  是否使用 L4 虚拟机 MMU      */

/*********************************************************************************************************
  是否需要内核超过 3 级页表支持
*********************************************************************************************************/

#define LW_CFG_VMM_PAGE_4L_EN                 0                         /*  是否需要 4 级页表支持       */

/*********************************************************************************************************
  虚拟内存页表相关配置
*********************************************************************************************************/

#define LW_CFG_VMM_PAGE_SHIFT                 12                        /*  2^12 = 4096                 */
#define LW_CFG_VMM_PAGE_SIZE                  (1ul << LW_CFG_VMM_PAGE_SHIFT)
#define LW_CFG_VMM_PAGE_MASK                  (~(LW_CFG_VMM_PAGE_SIZE - 1))

#define LW_CFG_VMM_PMD_SHIFT                  20                        /*  NO PMD same as PGD          */
#define LW_CFG_VMM_PMD_SIZE                   (1ul << LW_CFG_VMM_PMD_SHIFT)
#define LW_CFG_VMM_PMD_MASK                   (~(LW_CFG_VMM_PMD_SIZE - 1))

#define LW_CFG_VMM_PGD_SHIFT                  20                        /*  2^20 = 1MB                  */
#define LW_CFG_VMM_PGD_SIZE                   (1ul << LW_CFG_VMM_PGD_SHIFT)
#define LW_CFG_VMM_PGD_MASK                   (~(LW_CFG_VMM_PGD_SIZE - 1))

/*********************************************************************************************************
  物理内存分组数量
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

#define LW_CFG_VMM_ZONE_NUM                   8                         /*  物理分区数                  */
#define LW_CFG_VMM_VIR_NUM                    8                         /*  虚拟分区数                  */

/*********************************************************************************************************
  MMU 转换条目类型
*********************************************************************************************************/
#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

typedef UINT32  LW_PGD_TRANSENTRY;                                      /*  页目录类型                  */
typedef UINT32  LW_PMD_TRANSENTRY;                                      /*  中间页目录类型              */

typedef union {
    struct {
        UINT        PTE_uiRPN       : 20;                               /*  物理页号                    */
        UINT        PTE_ucReserved1 :  3;                               /*  保留                        */
        UINT        PTE_bRef        :  1;                               /*  引用位                      */
        UINT        PTE_bChange     :  1;                               /*  修改位                      */
        UINT        PTE_ucWIMG      :  4;                               /*  内存和 CACHE 属性位         */
        UINT        PTE_bReserved2  :  1;                               /*  保留                        */
        UINT        PTE_ucPP        :  2;                               /*  页保护权限位                */
    };                                                                  /*  通用的 PPC32 PTE            */
    UINT32          PTE_uiValue;                                        /*  值                          */

    struct {
        /*
         * 以下值用于 TLB MISS 时重装到 MAS2 MAS3 寄存器
         */
        UINT        MAS3_uiRPN      : 20;                               /*  物理页号                    */

        UINT        MAS3_ucReserved0:  2;                               /*  保留                        */

        /*
         * 以下用户属性用于 TLB MISS 时重装到 MAS2 寄存器
         *
         * MAS2 寄存器还有 X0 X1 G E 位, G 和 E 位固定为 0
         * X0 X1 位由 MAS4 寄存器的 X0D X1D 自动重装
         */
#define MAS3_bValid      MAS3_bUserAttr0                                /*  是否有效                    */
#define MAS3_bWT         MAS3_bUserAttr1                                /*  是否写穿透                  */
#define MAS3_bUnCache    MAS3_bUserAttr2                                /*  是否不可 Cache              */
#define MAS3_bMemCoh     MAS3_bUserAttr3                                /*  是否多核内存一致性          */

        UINT        MAS3_bUserAttr0 :  1;                               /*  用户属性 0                  */
        UINT        MAS3_bUserAttr1 :  1;                               /*  用户属性 1                  */
        UINT        MAS3_bUserAttr2 :  1;                               /*  用户属性 2                  */
        UINT        MAS3_bUserAttr3 :  1;                               /*  用户属性 3                  */

        UINT        MAS3_bUserExec  :  1;                               /*  用户可执行权限              */
        UINT        MAS3_bSuperExec :  1;                               /*  管理员可执行权限            */

        UINT        MAS3_bUserWrite :  1;                               /*  用户可写权限                */
        UINT        MAS3_bSuperWrite:  1;                               /*  管理员可写权限              */

        UINT        MAS3_bUserRead  :  1;                               /*  用户可读权限                */
        UINT        MAS3_bSuperRead :  1;                               /*  管理员可读权限              */
    };                                                                  /*  E500 PTE                    */
    UINT32          MAS3_uiValue;                                       /*  值                          */
} LW_PTE_TRANSENTRY;                                                    /*  页表条目类型                */

/*********************************************************************************************************
  E500 TLB1 条目映射描述
*********************************************************************************************************/

typedef struct {
    UINT64                   TLB1D_ui64PhyAddr;                         /*  物理地址 (页对齐地址)       */
    ULONG                    TLB1D_ulVirMap;                            /*  需要初始化的映射关系        */
    ULONG                    TLB1D_stSize;                              /*  物理内存区长度 (页对齐长度) */
    ULONG                    TLB1D_ulFlag;                              /*  物理内存区间类型            */
#define E500_TLB1_FLAG_VALID               0x01                         /*  映射有效                    */
#define E500_TLB1_FLAG_UNVALID             0x00                         /*  映射无效                    */

#define E500_TLB1_FLAG_ACCESS              0x02                         /*  可以访问                    */
#define E500_TLB1_FLAG_UNACCESS            0x00                         /*  不能访问                    */

#define E500_TLB1_FLAG_WRITABLE            0x04                         /*  可以写操作                  */
#define E500_TLB1_FLAG_UNWRITABLE          0x00                         /*  不可以写操作                */

#define E500_TLB1_FLAG_EXECABLE            0x08                         /*  可以执行代码                */
#define E500_TLB1_FLAG_UNEXECABLE          0x00                         /*  不可以执行代码              */

#define E500_TLB1_FLAG_CACHEABLE           0x10                         /*  可以缓冲                    */
#define E500_TLB1_FLAG_UNCACHEABLE         0x00                         /*  不可以缓冲                  */

#define E500_TLB1_FLAG_GUARDED             0x40                         /*  进行严格的权限检查          */
#define E500_TLB1_FLAG_UNGUARDED           0x00                         /*  不进行严格的权限检查        */

#define E500_TLB1_FLAG_TEMP                0x80                         /*  临时映射                    */

#define E500_TLB1_FLAG_MEM      (E500_TLB1_FLAG_VALID    | \
                                 E500_TLB1_FLAG_ACCESS   | \
                                 E500_TLB1_FLAG_WRITABLE | \
                                 E500_TLB1_FLAG_EXECABLE | \
                                 E500_TLB1_FLAG_CACHEABLE)              /*  普通内存                    */

#define E500_TLB1_FLAG_BOOTSFR  (E500_TLB1_FLAG_VALID      | \
                                 E500_TLB1_FLAG_GUARDED    | \
                                 E500_TLB1_FLAG_ACCESS     | \
                                 E500_TLB1_FLAG_WRITABLE   | \
                                 E500_TLB1_FLAG_UNEXECABLE | \
                                 E500_TLB1_FLAG_UNCACHEABLE)            /*  特殊功能寄存器              */
} E500_TLB1_MAP_DESC;
typedef E500_TLB1_MAP_DESC      *PE500_TLB1_MAP_DESC;

/*********************************************************************************************************
  E500 bsp 需要调用以下函数初始化 TLB1
*********************************************************************************************************/

INT  archE500MmuTLB1GlobalMap(CPCHAR               pcMachineName,
                              PE500_TLB1_MAP_DESC  pdesc,
                              VOID                 (*pfuncPreRemoveTempMap)(VOID));

#endif
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __PPC_ARCH_MMU_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
