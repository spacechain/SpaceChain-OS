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
** 文   件   名: ppcMmuE500Tlb1.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 05 月 06 日
**
** 描        述: PowerPC E500 体系构架 MMU TLB1 函数库.
**
** 注        意: 不依赖于任何操作系统服务, 可以在操作系统初始化前调用.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "arch/ppc/arch_e500.h"
#include "arch/ppc/common/e500/ppcSprE500.h"
#include "./ppcMmuE500Reg.h"
#include "alloca.h"
/*********************************************************************************************************
    The E500 MMU has two levels.  In level 1, instruction and data are
    split, while they are unified in level 2.  Level 1 is maintained by
    the hardware and level 2 is maintained by the OS.  The number of entries
    are as follows:

    Name      Level   Type No of pg sizes Assoc     #entries     filled by
    I-L1VSP     L1    Instr     9         Full         4         TLB1 hit
    I-L1TLB4K   L1    Instr   1(4k)       4-way        64        TLB0 hit
    D-L1VSP     L1    Data      9         Full         4         TLB1 hit
    D-L1TLB4K   L1    Data    1(4k)       4-way        64        TLB0 hit
    TLB1        L2    I/D       9         Full         16        s/w tlbwe
    TLB0        L2    I/D     1(4k)       2-way        256       s/w tlbwe

    The VSP (variable sized page) are used as static entries like the BATs,
    while the 4k page are dynamic entries that gets loaded with the PTEs.
    When a TLB miss occur in TLB0, an exception occurs and the OS walks the
    data structure and copies a PTE into a TLB0 entry.  Hence, TLB1 is
    filled with mapping from the _G_tlb1StaticMapDesc[] array(in bspMap.h), and TLB0 is
    filled with mapping from the ppcE500Mmu.c.

    Note that the E500 MMU cannot be turned off.
*********************************************************************************************************/
/*********************************************************************************************************
  定义
*********************************************************************************************************/
#define MMU_MAS2_M              _G_bMas2MBit                            /*  是否多核一致性              */
/*********************************************************************************************************
  MAS 寄存器数组
*********************************************************************************************************/
typedef struct {
    ULONG               MASR_ulFlag;                                    /*  映射标志                    */
    MAS1_REG            MASR_uiMAS1;                                    /*  MAS1                        */
    MAS2_REG            MASR_uiMAS2;                                    /*  MAS2                        */
    MAS3_REG            MASR_uiMAS3;                                    /*  MAS3                        */
    MAS7_REG            MASR_uiMAS7;                                    /*  MAS7                        */
} MAS_REGS;
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static UINT             _G_uiTlbSize = 0;                               /*  TLB 数组大小                */
static BOOL             _G_bMas2MBit = LW_FALSE;                        /*  多核一致性                  */
static BOOL             _G_bHasMAS7  = LW_FALSE;                        /*  是否有 MAS7 寄存器          */
static BOOL             _G_bHasHID1  = LW_FALSE;                        /*  是否有 HID1 寄存器          */
/*********************************************************************************************************
  外部接口声明
*********************************************************************************************************/
extern VOID    ppcE500MmuTLB1Invalidate(VOID);
extern VOID    ppcE500MmuTLB1InvalidateEA(addr_t  ulAddr);
extern VOID    ppcE500MmuInvalidateTLB(VOID);
/*********************************************************************************************************
** 函数名称: ppcE500MmuGlobalInit
** 功能描述: 调用 BSP 对 MMU TLB1 初始化
** 输　入  : pcMachineName     使用的机器名称
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppcE500MmuTLB1GlobalInit (CPCHAR  pcMachineName)
{
    MMUCFG_REG  uiMMUCFG;
    MAS4_REG    uiMAS4;
    UINT32      uiHID1;

    /*
     * 设置 PID
     */
    uiMMUCFG.MMUCFG_uiValue = ppcE500MmuGetMMUCFG();
    ppcE500MmuSetPID0(0);
    if (uiMMUCFG.MMUCFG_ucNPIDS > 1) {
        ppcE500MmuSetPID1(0);
        if (uiMMUCFG.MMUCFG_ucNPIDS > 2) {
            ppcE500MmuSetPID2(0);
        }
    }

    /*
     * 设置 MAS4
     */
    uiMAS4.MAS4_uiValue   = 0;
    uiMAS4.MAS4_bTLBSELD  = 0;
    uiMAS4.MAS4_ucTIDSELD = 0;
    uiMAS4.MAS4_ucTSIZED  = MMU_TRANS_SZ_4K;
    uiMAS4.MAS4_bX0D      = 0;
    uiMAS4.MAS4_bX1D      = 0;
    uiMAS4.MAS4_bWD       = LW_FALSE;
    uiMAS4.MAS4_bID       = LW_TRUE;
    uiMAS4.MAS4_bMD       = LW_TRUE;
    uiMAS4.MAS4_bGD       = LW_FALSE;
    uiMAS4.MAS4_bED       = LW_FALSE;

    ppcE500MmuSetMAS4(uiMAS4.MAS4_uiValue);

    /*
     * 使能地址广播
     */
    if (_G_bHasHID1) {
        uiHID1 = ppcE500GetHID1();
        if (MMU_MAS2_M) {
            uiHID1 |=  ARCH_PPC_HID1_ABE;
        } else {
            uiHID1 &= ~ARCH_PPC_HID1_ABE;
        }
        ppcE500SetHID1(uiHID1);
    }

    /*
     * 有 MAS7 寄存器, 则使能 MAS7 寄存器的访问
     */
    if (_G_bHasMAS7) {
        UINT32  uiHID0;

        uiHID0  = ppcE500GetHID0();
        uiHID0 |= ARCH_PPC_HID0_MAS7EN;
        ppcE500SetHID0(uiHID0);
    }

    ppcE500MmuInvalidateTLB();
    ppcE500MmuTLB1Invalidate();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuTLB1Init
** 功能描述: MMU TLB1 系统初始化
** 输　入  : pcMachineName     使用的机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcE500MmuTLB1Init (CPCHAR  pcMachineName)
{
    TLBCFG_REG  uiTLB1CFG;

    /*
     * 多核一致性须使能 HID1[ABE] 位
     */
    MMU_MAS2_M = (LW_CFG_MAX_PROCESSORS > 1) ? 1 : 0;                   /*  多核一致性位设置            */

    if ((lib_strcmp(pcMachineName, PPC_MACHINE_E500V2) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E500MC) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E5500)  == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E6500)  == 0)) {
        _G_bHasMAS7 = LW_TRUE;
    } else {
        _G_bHasMAS7 = LW_FALSE;
    }

    if ((lib_strcmp(pcMachineName, PPC_MACHINE_E500MC) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E5500)  == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E6500)  == 0)) {
        _G_bHasHID1 = LW_FALSE;
    } else {
        _G_bHasHID1 = LW_TRUE;
    }

    /*
     * 获得 TLB1 条目数
     */
    uiTLB1CFG.TLBCFG_uiValue = ppcE500MmuGetTLB1CFG();
    _G_uiTlbSize = uiTLB1CFG.TLBCFG_usNENTRY;
}
/*********************************************************************************************************
** 函数名称: archE500MmuTLB1GlobalMap
** 功能描述: MMU TLB1 全局映射
** 输　入  : pcMachineName          使用的机器名称
**           pdesc                  映射关系数组
**           pfuncPreRemoveTempMap  移除临时映射前的回调函数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 必须在关中断情况下调用
*********************************************************************************************************/
INT  archE500MmuTLB1GlobalMap (CPCHAR               pcMachineName,
                               PE500_TLB1_MAP_DESC  pdesc,
                               VOID                 (*pfuncPreRemoveTempMap)(VOID))
{
    E500_TLB1_MAP_DESC      desc;
    MAS0_REG                uiMAS0;
    MAS1_REG                uiMAS1;
    MAS2_REG                uiMAS2;
    MAS3_REG                uiMAS3;
    MAS7_REG                uiMAS7;
    UINT                    i;
    size_t                  stRemainSize;
    MAS_REGS               *masRegs;

    if (!pdesc) {
        return  (PX_ERROR);
    }

    if (LW_CPU_GET_CUR_ID() == 0) {
        ppcE500MmuTLB1Init(pcMachineName);
    }

    ppcE500MmuTLB1GlobalInit(pcMachineName);

    /*
     * 第一步: 分析物理内存信息描述
     */
    masRegs = (MAS_REGS *)alloca(sizeof(MAS_REGS) * _G_uiTlbSize);      /*  从栈里分配                  */
    lib_bzero(masRegs, sizeof(MAS_REGS) * _G_uiTlbSize);

    desc         = *pdesc;                                              /*  从第一个开始分析            */
    stRemainSize = desc.TLB1D_stSize;

    for (i = 0; (i < _G_uiTlbSize) && stRemainSize;) {
        if (!(desc.TLB1D_ulFlag & E500_TLB1_FLAG_VALID)) {              /*  无效的映射关系              */
            pdesc++;
            desc         = *pdesc;
            stRemainSize = desc.TLB1D_stSize;
            continue;
        }

        /*
         * MAS1
         */
        uiMAS1.MAS1_uiValue = 0;
        uiMAS1.MAS1_bVaild  = LW_TRUE;
        uiMAS1.MAS1_bIPROT  = LW_TRUE;
        uiMAS1.MAS1_ucTID   = 0;
        uiMAS1.MAS1_bTS     = 0;

        if ((desc.TLB1D_stSize >= 1 * LW_CFG_GB_SIZE) &&
           !(desc.TLB1D_ui64PhyAddr & (1 * LW_CFG_GB_SIZE - 1))) {
            desc.TLB1D_stSize   = 1 * LW_CFG_GB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_1G;

        } else if ((desc.TLB1D_stSize >= 256 * LW_CFG_MB_SIZE) &&
                  !(desc.TLB1D_ui64PhyAddr & (256 * LW_CFG_MB_SIZE - 1))) {
            desc.TLB1D_stSize   = 256 * LW_CFG_MB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_256M;

        } else if ((desc.TLB1D_stSize >= 64 * LW_CFG_MB_SIZE) &&
                  !(desc.TLB1D_ui64PhyAddr & (64 * LW_CFG_MB_SIZE - 1))) {
            desc.TLB1D_stSize   = 64 * LW_CFG_MB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_64M;

        } else if ((desc.TLB1D_stSize >= 16 * LW_CFG_MB_SIZE) &&
                  !(desc.TLB1D_ui64PhyAddr & (16 * LW_CFG_MB_SIZE - 1))) {
            desc.TLB1D_stSize   = 16 * LW_CFG_MB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_16M;

        } else if ((desc.TLB1D_stSize >= 4 * LW_CFG_MB_SIZE) &&
                  !(desc.TLB1D_ui64PhyAddr & (4 * LW_CFG_MB_SIZE - 1))) {
            desc.TLB1D_stSize   = 4 * LW_CFG_MB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_4M;

        } else if ((desc.TLB1D_stSize >= 1 * LW_CFG_MB_SIZE) &&
                  !(desc.TLB1D_ui64PhyAddr & (1 * LW_CFG_MB_SIZE - 1))) {
            desc.TLB1D_stSize   = 1 * LW_CFG_MB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_1M;

        } else if ((desc.TLB1D_stSize >= 256 * LW_CFG_KB_SIZE) &&
                  !(desc.TLB1D_ui64PhyAddr & (256 * LW_CFG_KB_SIZE - 1))) {
            desc.TLB1D_stSize   = 256 * LW_CFG_KB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_256K;

        } else if ((desc.TLB1D_stSize >= 64 * LW_CFG_KB_SIZE) &&
                  !(desc.TLB1D_ui64PhyAddr & (64 * LW_CFG_KB_SIZE - 1))) {
            desc.TLB1D_stSize   = 64 * LW_CFG_KB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_64K;

        } else if ((desc.TLB1D_stSize >= 16 * LW_CFG_KB_SIZE) &&
                  !(desc.TLB1D_ui64PhyAddr & (16 * LW_CFG_KB_SIZE - 1))) {
            desc.TLB1D_stSize   = 16 * LW_CFG_KB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_16K;

        } else if ((desc.TLB1D_stSize >= 4 * LW_CFG_KB_SIZE) &&
                  !(desc.TLB1D_ui64PhyAddr & (4 * LW_CFG_KB_SIZE - 1))) {
            desc.TLB1D_stSize   = 4 * LW_CFG_KB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_4K;

        } else {
            _BugFormat(LW_TRUE, LW_TRUE, "map size 0x%x is NOT 4K align!\r\n", pdesc->TLB1D_stSize);
        }

        /*
         * MAS2
         */
        uiMAS2.MAS2_uiValue = 0;
        uiMAS2.MAS2_uiEPN   = desc.TLB1D_ulVirMap >> LW_CFG_VMM_PAGE_SHIFT;
        uiMAS2.MAS2_bLittleEndian = LW_FALSE;

        if (desc.TLB1D_ulFlag & E500_TLB1_FLAG_GUARDED) {               /*  进行严格的权限检查          */
            uiMAS2.MAS2_bGuarded = LW_TRUE;
        }

        if (!(desc.TLB1D_ulFlag & E500_TLB1_FLAG_CACHEABLE)) {
            uiMAS2.MAS2_bUnCache = LW_TRUE;                             /*  不可 Cache                  */
        }

        if ((desc.TLB1D_ulFlag & E500_TLB1_FLAG_CACHEABLE) && MMU_MAS2_M) {
            uiMAS2.MAS2_bMemCoh = LW_TRUE;                              /*  多核一致性                  */
        }

        /*
         * MAS3
         */
        uiMAS3.MAS3_uiValue = 0;                                        /*  MAS3                        */
        uiMAS3.MAS3_uiRPN   = (desc.TLB1D_ui64PhyAddr & 0xffffffff) >> LW_CFG_VMM_PAGE_SHIFT;

        if (desc.TLB1D_ulFlag & E500_TLB1_FLAG_ACCESS) {
            uiMAS3.MAS3_bUserRead  = LW_FALSE;                          /*  用户态不可读                */
            uiMAS3.MAS3_bSuperRead = LW_TRUE;                           /*  可读                        */
        }

        if (desc.TLB1D_ulFlag & E500_TLB1_FLAG_WRITABLE) {
            uiMAS3.MAS3_bUserWrite  = LW_FALSE;                         /*  用户态不可写                */
            uiMAS3.MAS3_bSuperWrite = LW_TRUE;                          /*  可写                        */
        }

        if (desc.TLB1D_ulFlag & E500_TLB1_FLAG_EXECABLE) {
            uiMAS3.MAS3_bUserExec  = LW_FALSE;                          /*  用户态不可执行              */
            uiMAS3.MAS3_bSuperExec = LW_TRUE;                           /*  可执行                      */
        }

        /*
         * MAS7
         */
        uiMAS7.MAS7_uiValue    = 0;
        uiMAS7.MAS7_uiHigh4RPN = desc.TLB1D_ui64PhyAddr >> 32;

        /*
         * 将分析结果记录到 masRegs 数组
         */
        masRegs[i].MASR_ulFlag = desc.TLB1D_ulFlag;
        masRegs[i].MASR_uiMAS1 = uiMAS1;
        masRegs[i].MASR_uiMAS2 = uiMAS2;
        masRegs[i].MASR_uiMAS3 = uiMAS3;
        masRegs[i].MASR_uiMAS7 = uiMAS7;

        i++;

        stRemainSize = stRemainSize - desc.TLB1D_stSize;
        if (stRemainSize > 0) {                                         /*  有未"映射"的部分            */
            desc.TLB1D_ui64PhyAddr += desc.TLB1D_stSize;                /*  继续"映射"剩余的部分        */
            desc.TLB1D_ulVirMap    += desc.TLB1D_stSize;
            desc.TLB1D_stSize       = stRemainSize;

        } else {
            pdesc++;                                                    /*  "映射"下一个                */
            desc         = *pdesc;
            stRemainSize = desc.TLB1D_stSize;
        }
    }

    _BugHandle(i == _G_uiTlbSize, LW_TRUE, "to many map desc!\r\n");

    /*
     * 第二步: 用 masRegs 数组记录的值来进行真正的映射
     */
    PPC_EXEC_INS("SYNC");

    for (i = 0; i < _G_uiTlbSize; i++) {
        uiMAS0.MAS0_uiValue = 0;                                        /*  MAS0                        */
        uiMAS0.MAS0_bTLBSEL = 1;
        uiMAS0.MAS0_ucESEL  = i;
        uiMAS0.MAS0_ucNV    = 0;
        ppcE500MmuSetMAS0(uiMAS0.MAS0_uiValue);

        ppcE500MmuSetMAS0(uiMAS0.MAS0_uiValue);
        ppcE500MmuSetMAS1(masRegs[i].MASR_uiMAS1.MAS1_uiValue);
        ppcE500MmuSetMAS2(masRegs[i].MASR_uiMAS2.MAS2_uiValue);
        ppcE500MmuSetMAS3(masRegs[i].MASR_uiMAS3.MAS3_uiValue);

        if (_G_bHasMAS7) {
            ppcE500MmuSetMAS7(masRegs[i].MASR_uiMAS7.MAS7_uiValue);
        }

        PPC_EXEC_INS("ISYNC");
        PPC_EXEC_INS("SYNC");
        PPC_EXEC_INS("TLBWE");
        PPC_EXEC_INS("TLBSYNC");
        PPC_EXEC_INS("ISYNC");
    }

    if (pfuncPreRemoveTempMap) {
        pfuncPreRemoveTempMap();
    }

    /*
     * 第三步: 删除临时映射
     */
    for (i = 0; i < _G_uiTlbSize; i++) {
        if (masRegs[i].MASR_ulFlag & E500_TLB1_FLAG_TEMP) {
            uiMAS0.MAS0_uiValue = 0;                                    /*  MAS0                        */
            uiMAS0.MAS0_bTLBSEL = 1;
            uiMAS0.MAS0_ucESEL  = i;
            uiMAS0.MAS0_ucNV    = 0;
            ppcE500MmuSetMAS0(uiMAS0.MAS0_uiValue);

            ppcE500MmuSetMAS1(0);
            ppcE500MmuSetMAS2(0);
            ppcE500MmuSetMAS3(0);

            if (_G_bHasMAS7) {
                ppcE500MmuSetMAS7(0);
            }

            PPC_EXEC_INS("ISYNC");
            PPC_EXEC_INS("SYNC");
            PPC_EXEC_INS("TLBWE");
            PPC_EXEC_INS("TLBSYNC");
            PPC_EXEC_INS("ISYNC");
        }
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
