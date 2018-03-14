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
** 文   件   名: armGdb.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2014 年 05 月 06 日
**
** 描        述: ARM 体系构架 GDB 调试接口.
**
** BUG:
2014.12.08  添加thumb指令分支预测功能.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁减配置
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "dtrace.h"
#include "../arm_gdb.h"
/*********************************************************************************************************
  Xfer:features:read:arch-core.xml 回应包
*********************************************************************************************************/
static const CHAR   cArmCore[] = \
        "l<?xml version=\"1.0\"?>"
        "<!-- Copyright (C) 2006-2017 ACOINFO co.,ltd."
             "Copying and distribution of this file, with or without modification,"
             "are permitted in any medium without royalty provided the copyright"
             "notice and this notice are preserved.  -->"
        "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">"
        "<feature name=\"org.gnu.gdb.arm.core\">"
          "<reg name=\"r0\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"r1\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"r2\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"r3\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"r4\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"r5\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"r6\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"r7\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"r8\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"r9\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"r10\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"r11\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"r12\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"sp\" bitsize=\"32\" type=\"data_ptr\"/>"
          "<reg name=\"lr\" bitsize=\"32\"/>"
          "<reg name=\"pc\" bitsize=\"32\" type=\"code_ptr\"/>"
          "<!-- The CPSR is register 25, rather than register 16, because"
               "the FPA registers historically were placed between the PC"
               "and the CPSR in the \"g\" packet.  -->"
          "<reg name=\"cpsr\" bitsize=\"32\" regnum=\"25\"/>"
        "</feature>";
/*********************************************************************************************************
  Xfer:features:read:target.xml 回应包
*********************************************************************************************************/
static const CHAR   cTargetSystem[] = \
        "l<?xml version=\"1.0\"?>"
        "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
        "<target>"
            "<xi:include href=\"arch-core.xml\"/>"
        "</target>";
/*********************************************************************************************************
  PC 寄存器在 GDB_REG_SET 结构中的索引
*********************************************************************************************************/
#define ARM_REG_INDEX_PC   15
/*********************************************************************************************************
  CPSR 寄存器在 GDB_REG_SET 结构中的索引
*********************************************************************************************************/
#define ARM_REG_INDEX_CPSR 16
/*********************************************************************************************************
  编译器移位操作必须支持符号位扩展
*********************************************************************************************************/
#if (((INT32)-1L) >> 1) > 0
#  error right shifting an int does not perform sign extension
#endif
/*********************************************************************************************************
  分支预测过程中使用的位操作
*********************************************************************************************************/
#define BIT(n)          ((UINT32)1U << (n))
#define BITSET(x,n)     (((UINT32)(x) & (1U<<(n))) >> (n))
#define BITS(x,m,n)     (((UINT32)((x) & (BIT(n) - BIT(m) + BIT(n)))) >> (m))
/*********************************************************************************************************
  结合cpsr值，用于判断指令执行条件
*********************************************************************************************************/
static UINT32 uiCCTable[] = {
    0xF0F0, 0x0F0F, 0xCCCC, 0x3333, 0xFF00, 0x00FF, 0xAAAA, 0x5555,
    0x0C0C, 0xF3F3, 0xAA55, 0x55AA, 0x0A05, 0xF5FA, 0xFFFF, 0x0000
};
/*********************************************************************************************************
  部分指令为无条件执行指令，下面结构体用于描述这类指令
*********************************************************************************************************/
typedef struct {
    UINT32  UNC_uiIns;                                                  /* 无条件执行指令值             */
    UINT32  UNC_uiMask;                                                 /* 掩码                         */
} ARM_UNCOND;
/*********************************************************************************************************
  无条件执行指令表
*********************************************************************************************************/
static ARM_UNCOND  armuncondTable [] = {
    {0xF5500000, 0xFD500000},                                           /* PLD                          */
    {0xFA000000, 0xFE000000},                                           /* BLX (1)                      */
    {0xFC000000, 0xFE100000},                                           /* STC2                         */
    {0xFC100000, 0xFE100000},                                           /* LDC2                         */
    {0xFE000000, 0xFE000001},                                           /* CDP2                         */
    {0xFE000000, 0xFE000001},                                           /* MCR2                         */
    {0xFE100000, 0xFE000001},                                           /* MRC2                         */
    {0x0,        0x0}
};
/*********************************************************************************************************
** 函数名称: archGdbTargetXml
** 功能描述: 获得 Xfer:features:read:target.xml 回复 XML
** 输　入  : NONE
** 输　出  : 回复 XML
** 全局变量:
** 调用模块:
*********************************************************************************************************/
CPCHAR  archGdbTargetXml (VOID)
{
    return  (cTargetSystem);
}
/*********************************************************************************************************
** 函数名称: archGdbCoreXml
** 功能描述: 获得 Xfer:features:read:arm-core.xml 回复 XML
** 输　入  : NONE
** 输　出  : 回复 XML
** 全局变量:
** 调用模块:
*********************************************************************************************************/
CPCHAR  archGdbCoreXml (VOID)
{
    return  (cArmCore);
}
/*********************************************************************************************************
** 函数名称: archGdbRegsGet
** 功能描述: 获取寄存器值
** 输　入  : pvDtrace       dtrace 句柄
**           ulThread       被调试线程
** 输　出  : pregset        gdb 寄存器结构
**           返回值         成功-- ERROR_NONE，失败-- PX_ERROR.
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archGdbRegsGet (PVOID pvDtrace, LW_OBJECT_HANDLE ulThread, GDB_REG_SET *pregset)
{
    ARCH_REG_CTX  regctx;
    ARCH_REG_T    regSp;

    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);

    lib_bzero(pregset, sizeof(GDB_REG_SET));

    pregset->GDBR_iRegCnt = 17;

    pregset->regArr[0].GDBRA_ulValue  = regctx.REG_uiR0;
    pregset->regArr[1].GDBRA_ulValue  = regctx.REG_uiR1;
    pregset->regArr[2].GDBRA_ulValue  = regctx.REG_uiR2;
    pregset->regArr[3].GDBRA_ulValue  = regctx.REG_uiR3;
    pregset->regArr[4].GDBRA_ulValue  = regctx.REG_uiR4;
    pregset->regArr[5].GDBRA_ulValue  = regctx.REG_uiR5;
    pregset->regArr[6].GDBRA_ulValue  = regctx.REG_uiR6;
    pregset->regArr[7].GDBRA_ulValue  = regctx.REG_uiR7;
    pregset->regArr[8].GDBRA_ulValue  = regctx.REG_uiR8;
    pregset->regArr[9].GDBRA_ulValue  = regctx.REG_uiR9;
    pregset->regArr[10].GDBRA_ulValue = regctx.REG_uiR10;
    pregset->regArr[11].GDBRA_ulValue = regctx.REG_uiFp;
    pregset->regArr[12].GDBRA_ulValue = regctx.REG_uiIp;
    pregset->regArr[13].GDBRA_ulValue = regSp;
    pregset->regArr[14].GDBRA_ulValue = regctx.REG_uiLr;
    pregset->regArr[15].GDBRA_ulValue = regctx.REG_uiPc;
    pregset->regArr[16].GDBRA_ulValue = regctx.REG_uiCpsr;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archGdbRegsSet
** 功能描述: 设置寄存器值
** 输　入  : pvDtrace       dtrace 句柄
**           ulThread       被调试线程
**           pregset        gdb 寄存器结构
** 输　出  : 成功-- ERROR_NONE，失败-- PX_ERROR.
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archGdbRegsSet (PVOID pvDtrace, LW_OBJECT_HANDLE ulThread, GDB_REG_SET *pregset)
{
    ARCH_REG_CTX  regctx;

    regctx.REG_uiR0   = pregset->regArr[0].GDBRA_ulValue;
    regctx.REG_uiR1   = pregset->regArr[1].GDBRA_ulValue;
    regctx.REG_uiR2   = pregset->regArr[2].GDBRA_ulValue;
    regctx.REG_uiR3   = pregset->regArr[3].GDBRA_ulValue;
    regctx.REG_uiR4   = pregset->regArr[4].GDBRA_ulValue;
    regctx.REG_uiR5   = pregset->regArr[5].GDBRA_ulValue;
    regctx.REG_uiR6   = pregset->regArr[6].GDBRA_ulValue;
    regctx.REG_uiR7   = pregset->regArr[7].GDBRA_ulValue;
    regctx.REG_uiR8   = pregset->regArr[8].GDBRA_ulValue;
    regctx.REG_uiR9   = pregset->regArr[9].GDBRA_ulValue;
    regctx.REG_uiR10  = pregset->regArr[10].GDBRA_ulValue;
    regctx.REG_uiFp   = pregset->regArr[11].GDBRA_ulValue;
    regctx.REG_uiIp   = pregset->regArr[12].GDBRA_ulValue;
    regctx.REG_uiLr   = pregset->regArr[14].GDBRA_ulValue;
    regctx.REG_uiPc   = pregset->regArr[15].GDBRA_ulValue;
    regctx.REG_uiCpsr = pregset->regArr[16].GDBRA_ulValue;

    API_DtraceSetRegs(pvDtrace, ulThread, &regctx);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archGdbRegSetPc
** 功能描述: 设置 pc 寄存器值
** 输　入  : pvDtrace       dtrace 句柄
**           ulThread       被调试线程
**           ulPC           pc 寄存器值
** 输　出  : 成功-- ERROR_NONE，失败-- PX_ERROR.
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archGdbRegSetPc (PVOID pvDtrace, LW_OBJECT_HANDLE ulThread, ULONG ulPc)
{
    ARCH_REG_CTX  regctx;
    ARCH_REG_T    regSp;

    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);

    regctx.REG_uiPc = (ARCH_REG_T)ulPc;

    API_DtraceSetRegs(pvDtrace, ulThread, &regctx);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archGdbRegGetPc
** 功能描述: 获取 pc 寄存器值
** 输　入  : pRegs       寄存器数组
** 输　出  : PC寄存器值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG archGdbRegGetPc (GDB_REG_SET *pRegs)
{
    return  (pRegs->regArr[ARM_REG_INDEX_PC].GDBRA_ulValue);
}
/*********************************************************************************************************
** 函数名称: armShiftedRegVal
** 功能描述: 对寄存器操作数进行指令中指定的移位操作
** 输　入  : pRegs       寄存器数组
**           instr       指令
**           cFlag       CPSR 中的标记位
** 输　出  : 移位后的操作数值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT32  armShiftedRegVal (GDB_REG_SET *pRegs,
                                 UINT32       uiInstr,
                                 INT          iFlag)
{
    UINT32 uiRes;
    UINT32 uiShift;
    UINT32 uiRm;
    UINT32 uiRs;
    UINT32 uiShiftType;

    uiRm        = BITS(uiInstr, 0, 3);
    uiShiftType = BITS(uiInstr, 5, 6);

    if (BITSET(uiInstr, 4)) {
        uiRs = BITS(uiInstr, 8, 11);
        uiShift = ((uiRs == 15) 
                ? ((UINT32)pRegs->regArr[ARM_REG_INDEX_PC].GDBRA_ulValue + 8) 
                : (pRegs->regArr[uiRs].GDBRA_ulValue) & 0xFF);
    } else {
        uiShift = BITS(uiInstr, 7, 11);
    }

    uiRes = (uiRm == 15)
          ? ((UINT32)pRegs->regArr[ARM_REG_INDEX_PC].GDBRA_ulValue + (BITSET(uiInstr, 4) ? 12 : 8))
          : pRegs->regArr[uiRm].GDBRA_ulValue;

    switch (uiShiftType) {
    
    case 0:                                                             /* LSL                          */
        uiRes = (uiShift >= 32) ? 0 : (uiRes << uiShift);
        break;

    case 1:                                                             /* LSR                          */
        uiRes = (uiShift >= 32) ? 0 : (uiRes >> uiShift);
        break;

    case 2:                                                             /* ASR                          */
        if (uiShift >= 32) {
            uiShift =  31;
        }
        uiRes = (uiRes & 0x80000000L) ? (~((~uiRes) >> uiShift)) : (uiRes >> uiShift);
        break;

    case 3:                                                             /* ROR                          */
        uiShift &= 31;
        if (uiShift == 0) {
            uiRes = (uiRes >> 1) | (iFlag ? 0x80000000L : 0);
        } else {
            uiRes = (uiRes >> uiShift) | (uiRes << (32 - uiShift));
        }
        break;
    }

    return  (uiRes);
}
/*********************************************************************************************************
** 函数名称: armGetNextPc
** 功能描述: arm指令模式下获取下一条指令地址，含分支预测
** 输　入  : pRegs       寄存器数组
** 输　出  : 下一条指令地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG  armGetNextPc (GDB_REG_SET *pRegs)
{
    UINT32      uiPc;
    UINT32      uiNPc;
    UINT32      uiInstr;
    INT         i;
    INT         iFound = 0;

    uiPc  = (UINT32)pRegs->regArr[ARM_REG_INDEX_PC].GDBRA_ulValue;      /* 当前 PC                      */
    uiNPc = uiPc + 4;                                                   /* 默认为当前 PC + 4            */

    uiInstr = *(UINT*)uiPc;

    /*
     *  判断条件字段，如果当前指令不需要执行，则直接返回 uiPc + 4
     */
    if (((uiCCTable[(uiInstr >> 28) & 0xF] >>
        (pRegs->regArr[ARM_REG_INDEX_CPSR].GDBRA_ulValue >> 28)) & 1) == 0) {
        for (i=0; armuncondTable[i].UNC_uiIns != 0; i++) {              /* 部分指令必须执行             */
            if (((uiInstr & 0xF0000000) == 0xF0000000) &&
                (uiInstr & armuncondTable[i].UNC_uiMask) ==
                (armuncondTable[i].UNC_uiIns &
                 armuncondTable[i].UNC_uiMask)) {
                iFound = 1;
                break;
            }
        }
        if (!iFound) {
            return  ((UINT)uiNPc);
        }
    }

    /*
     * 下面指令会影响 pc 值:
     *    B
     *    BL
     *    BLX (2)
     *    赋值给pc
     *    LDR 目标寄存器是pc
     *    LDM 目标寄存器含
     *    下面代码参考ARM symbolic debugger
     */
    switch (BITS(uiInstr, 24, 27)) {
    
    case 1:
        if (BITSET(uiInstr, 4)         &&
            BITSET(uiInstr, 7)         &&
            BITSET(uiInstr, 20)        &&
            (BITS(uiInstr, 5, 6) != 0) &&
            (BITS(uiInstr, 12, 15) == 15)) {
            break;                                                      /* 错误的指令                   */
        }

    case 0:                                                             /* 数据处理指令                 */
    case 2:
    case 3:
        {
            UINT32 rn, op1, op2, cFlag;

            if (BITS(uiInstr, 12, 15) != 15) {                          /* 不影响pc的指令               */
                break;
            }

            if (BITS(uiInstr, 22, 25) == 0 &&
                BITS(uiInstr, 4, 7) == 9) {                             /* 乘法指令不能以pc为目标       */
                break;
            }

            if (BITS(uiInstr, 4, 23) == 0x2FFF1 ||
                BITS(uiInstr, 4, 23) == 0x2FFF2 ||
                BITS(uiInstr, 4, 23) == 0x2FFF3) {
                rn = BITS(uiInstr, 0, 3);
                uiNPc = (rn == 15 ? uiPc + 8 : pRegs->regArr[rn].GDBRA_ulValue) & ~1;
                break;
            }

            cFlag = BITSET(pRegs->regArr[ARM_REG_INDEX_CPSR].GDBRA_ulValue, 29);
            rn = BITS(uiInstr, 16, 19);
            op1 = rn == 15 ? uiPc + 8 : pRegs->regArr[rn].GDBRA_ulValue;

            if (BITSET(uiInstr, 25)) {
                UINT32 immVal, rotate;

                immVal = BITS(uiInstr, 0, 7);
                rotate = 2 * BITS(uiInstr, 8, 11);
                op2 = (immVal >> rotate) | (immVal << (32 - rotate));
            
            } else {
                op2 = armShiftedRegVal(pRegs, uiInstr, cFlag);
            }

            switch (BITS(uiInstr, 21, 24)) {
            
            case 0x0:                                                   /* AND                          */
                uiNPc = op1 & op2;
                break;
            
            case 0x1:                                                   /* EOR                          */
                uiNPc = op1 ^ op2;
                break;
            
            case 0x2:                                                   /* SUB                          */
                uiNPc = op1 - op2;
                break;
            
            case 0x3:                                                   /* RSB                          */
                uiNPc = op2 - op1;
                break;
            
            case 0x4:                                                   /* ADD                          */
                uiNPc = op1 + op2;
                break;
            
            case 0x5:                                                   /* ADC                          */
                uiNPc = op1 + op2 + cFlag;
                break;
            
            case 0x6:                                                   /* SBC                          */
                uiNPc = op1 - op2 + cFlag;
                break;
            
            case 0x7:                                                   /* RSC                          */
                uiNPc = op2 - op1 + cFlag;
                break;
            
            case 0x8:                                                   /* TST                          */
            case 0x9:                                                   /* TEQ                          */
            case 0xa:                                                   /* CMP                          */
            case 0xb:                                                   /* CMN                          */
                break;
            
            case 0xc:                                                   /* ORR                          */
                uiNPc = op1 | op2;
                break;
            
            case 0xd:                                                   /* MOV                          */
                uiNPc = op2;
                break;
            
            case 0xe:                                                   /* BIC                          */
                uiNPc = op1 & ~op2;
                break;
            
            case 0xf:                                                   /* MVN                          */
                uiNPc = ~op2;
                break;
            }
        }
        break;

    case 4:                                                             /* 数据传输                     */
    case 5:
    case 6:
    case 7:
        if (BITSET(uiInstr, 20) && (BITS(uiInstr, 12, 15) == 15) &&
            !BITSET(uiInstr, 22)) {                                     /* load数据到pc                 */
            UINT32 rn, cFlag, base;
            INT32  offset;

            rn = BITS(uiInstr, 16, 19);
            base = rn == 15 ? uiPc + 8 : pRegs->regArr[rn].GDBRA_ulValue;
            cFlag = BITSET(pRegs->regArr[ARM_REG_INDEX_CPSR].GDBRA_ulValue, 29);
            offset = BITSET(uiInstr, 25)
                    ? armShiftedRegVal(pRegs, uiInstr, cFlag)
                    : BITS(uiInstr, 0, 11);

            if (!BITSET(uiInstr, 23)) {
                offset = -offset;
            }

            if (BITSET(uiInstr, 24)) {
                base += offset;
            }

            uiNPc = *(UINT *)base;
        }
        break;

    case 8:
    case 9:                                                             /* 块数据传输                   */
        if (BITSET(uiInstr, 20) && BITSET(uiInstr, 15)) {
            UINT32 rn;
            INT32 offset = 0;

            rn = BITS(uiInstr, 16, 19);
            if (BITSET(uiInstr, 23)) {
                UINT32 regBit, regList;

                for (regList = BITS(uiInstr, 0, 14); regList != 0; regList &= ~regBit) {
                    regBit = regList & (-regList);
                    offset += 4;
                }

                if (BITSET(uiInstr, 24)) {
                    offset += 4;
                }
            } else {
                if (BITSET(uiInstr, 24)) {
                    offset = -4;
                }
            }

            uiNPc = *(UINT32 *)(pRegs->regArr[rn].GDBRA_ulValue + offset);
        }
        break;

    case 0xA:                                                           /* branch, BLX                  */
    case 0xB:                                                           /* branch & link                */
        uiNPc = uiPc + 8 + ((INT32)(uiInstr << 8) >> 6);
        break;

    case 0xC:
    case 0xD:
    case 0xE:                                                           /* coproc ops                   */
    case 0xF:                                                           /* SWI                          */
        break;
    }

    return  ((ULONG)uiNPc);
}
/*********************************************************************************************************
** 函数名称: thumbGetNpc
** 功能描述: thumb指令模式下获取下一条指令地址，含分支预测
** 输　入  : pRegs       寄存器数组
** 输　出  : 下一条指令地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ULONG   thumbGetNextPc (GDB_REG_SET *pRegs)
{
    UINT32      uiPc;
    UINT32      uiNPc;
    UINT32      uiInstr;

    uiPc  = (UINT32)pRegs->regArr[ARM_REG_INDEX_PC].GDBRA_ulValue;      /* 当前 PC                      */
    uiNPc = uiPc + 2;                                                   /* 默认为当前 PC + 4            */

    uiInstr = *(UINT*)uiPc;

    if (BITS(uiInstr, 13, 15) == 7 && BITS(uiInstr, 11, 12) != 0) {     /* thumb2 32位指令              */
        uiNPc += 2;
        switch (BITS(uiInstr, 11, 12)) {
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        }
    } else {

        switch (BITS(uiInstr, 12, 15)) {

        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3:
        case 0x5:
        case 0x6:
        case 0x7:
        case 0x8:
        case 0x9:
        case 0xA:
        case 0xC:
            /*
             * no effect on PC - next instruction executes
             */
            break;

        case 4:
            if (BITS(uiInstr, 7, 11) == 0x0E) {                         /* BX                           */
                int rn;

                rn = BITS(uiInstr, 3, 6);
                uiNPc = rn == 15 ? uiPc + 4 : pRegs->regArr[rn].GDBRA_ulValue;
                break;
            }

            if (BITSET(uiInstr, 7) &&
               (BITS(uiInstr, 0, 11) & 0xC07) == 0x407) {               /* 修改pc                       */
                INT rn;
                UINT32 operand;

                rn = BITS(uiInstr, 3, 6);
                operand = rn == 15 ? uiPc + 4 : pRegs->regArr[rn].GDBRA_ulValue;
                switch (BITS(uiInstr, 8, 9)) {
                case 0:                                                 /* ADD                          */
                    uiNPc = uiPc + 4 + operand;
                break;
                case 1:                                                 /* CMP                          */
                break;
                case 2:                                                 /* MOV                          */
                    uiNPc = operand;
                break;
                case 3:                                                 /* BX - already handled         */
                break;
               }
            }
            break;

        case 0xB:
            if (BITS(uiInstr, 8, 11) == 0xD) {                          /* POP {rlist, pc}              */

                INT32 offset = 0;
                UINT32 regList, regBit;

                for (regList = BITS(uiInstr, 0, 7);
                     regList != 0;
                     regList &= ~regBit) {
                    regBit = regList & (-regList);
                    offset += 4;
                }
                uiNPc = *(UINT32 *)(pRegs->regArr[13].GDBRA_ulValue + offset);
            }
            break;

        case 0xD:
            {                                                           /* SWI or conditional branch    */
                UINT32 cond;

                cond = (uiInstr >> 8) & 0xF;
                if (cond == 0xF) {
                    break;                                              /* SWI                          */
                }

                /* Conditional branch
                 * Use the same mechanism as armGetNpc() to determine whether
                 * the branch will be taken
                 */
                if (((uiCCTable[cond] >>
                     (pRegs->regArr[ARM_REG_INDEX_CPSR].GDBRA_ulValue >>
                     28)) & 1) == 0) {
                    break;                                              /* instruction will not execute */
                }

                uiNPc = uiPc + 4 + (((uiInstr & 0x00FF) << 1) |
                    (BITSET(uiInstr, 7)  ? 0xFFFFFE00 : 0));
            }
            break;

        case 0xE:
            if (BITSET(uiInstr, 11) == 0) {                             /* Unconditional branch         */
                uiNPc = uiPc + 4 + (((uiInstr & 0x07FF) << 1) |
                        (BITSET(uiInstr, 10) ? 0xFFFFF000 : 0));
            }
            break;

        case 0xF:                                                       /* BL                           */
            if (BITSET(uiInstr, 11)) {
                uiNPc = pRegs->regArr[14].GDBRA_ulValue + ((uiInstr & 0x07FF) << 1);
            } else {
                UINT32 nextBit;
                INT32 reloc;

                nextBit = *(UINT16 *)(uiPc + 2);
                if ((nextBit & 0xF800) != 0xF800)
                    break;

                reloc = (INT32)(((uiInstr & 0x7FF) << 12) |
                    ((nextBit & 0x7FF) << 1));
                reloc = (reloc ^ 0x00400000) - 0x00400000;

                uiNPc = uiPc + 4 + reloc;
            }

            break;

        }
    }

    return  (ULONG)((UINT32)uiNPc & ~1);
}
/*********************************************************************************************************
** 函数名称: archGdbGetNextPc
** 功能描述: 获取下一条指令地址，含分支预测
** 输　入  : pvDtrace       dtrace 句柄
**           ulThread       被调试线程
**           pRegs          寄存器数组
** 输　出  : 下一条指令地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG  archGdbGetNextPc (PVOID pvDtrace, LW_OBJECT_HANDLE ulThread, GDB_REG_SET *pRegs)
{
    ULONG   ulNextPc;
    
    if (pRegs->regArr[ARM_REG_INDEX_CPSR].GDBRA_ulValue & (1 << 5)) {   /* 根据cpsr判断当前指令状态     */
        ulNextPc = thumbGetNextPc(pRegs);
    
    } else {
        ulNextPc = armGetNextPc(pRegs);
    }

    return  (ulNextPc);
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
