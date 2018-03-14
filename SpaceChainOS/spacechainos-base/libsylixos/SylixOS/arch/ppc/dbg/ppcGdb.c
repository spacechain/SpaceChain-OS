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
** 文   件   名: ppcGdb.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 12 月 15 日
**
** 描        述: PowerPC 体系构架 GDB 调试接口.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁减配置
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "dtrace.h"
#include "arch/arch_gdb.h"
/*********************************************************************************************************
  Xfer:features:read:arch-core.xml 回应包
*********************************************************************************************************/
static const CHAR   cPpcCore[] = \
        "l<?xml version=\"1.0\"?>"
        "<!-- Copyright (C) 2006-2017 ACOINFO co.,ltd."
             "Copying and distribution of this file, with or without modification,"
             "are permitted in any medium without royalty provided the copyright"
             "notice and this notice are preserved.  -->"
        "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">"
        "<feature name=\"org.gnu.gdb.power.core\">"
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
        "<reg name=\"r13\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"r14\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"r15\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"r16\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"r17\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"r18\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"r19\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"r20\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"r21\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"r22\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"r23\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"r24\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"r25\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"r26\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"r27\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"r28\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"r29\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"r30\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"r31\" bitsize=\"32\" type=\"uint32\"/>"

        "<reg name=\"pc\" bitsize=\"32\" type=\"code_ptr\"/>"
        "<reg name=\"msr\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"cr\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"lr\" bitsize=\"32\" type=\"code_ptr\"/>"
        "<reg name=\"ctr\" bitsize=\"32\" type=\"uint32\"/>"
        "<reg name=\"xer\" bitsize=\"32\" type=\"uint32\"/>"
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
  特殊功能寄存在数组中的索引
*********************************************************************************************************/
#define GDB_PPC_PC_INDEX    32
#define GDB_PPC_MSR_INDEX   33
#define GDB_PPC_CR_INDEX    34
#define GDB_PPC_LR_INDEX    35
#define GDB_PPC_CTR_INDEX   36
#define GDB_PPC_XER_INDEX   37
#define GDB_PPC_REG_NR      38
/*********************************************************************************************************
  定义指令类型
*********************************************************************************************************/
typedef UINT32              INSTR_T;
/*********************************************************************************************************
  定义指令码相关宏
*********************************************************************************************************/
#define BRANCH_MASK         0xf0000000
#define OP_BRANCH           0x40000000
#define BCCTR_MASK          0xfc0007fe
#define BCLR_MASK           0xfc0007fe
#define AA_MASK             0x00000002

#define BO_NB_BIT           5                                           /*  nb bits of BO field         */
#define CR_NB_BIT           32                                          /*  nb bits of CR register      */

#define _OP(opcd, xo)       ((opcd << 26) + (xo << 1))

#define INST_BCCTR          _OP(19, 528)                                /*  BCCTR instruction opcode    */
#define INST_BCLR           _OP(19, 16)                                 /*  BCLR instruction opcode     */

#define _IFIELD_LI(x)       ((0x02000000 & x) ? ((0xf6000000 | x) & ~3) : \
                            ((0x03fffffc & x) & ~3))
#define _IFIELD_BD(x)       ((0x00008000 & x) ? (0xffff0000 | (x & ~3)) : \
                            (0x0000fffc & x))
                                                                        /*  ^ signed branch displ       */
#define _IFIELD_BO(x)       ((0x03e00000 & x) >> 21)                    /*  branch options              */
#define _IFIELD_BI(x)       ((0x001f0000 & x) >> 16)                    /*  branch condition            */

#define _REG32_BIT(reg, no) (0x00000001 & (reg >> (no)))                /*  value of no th bit of the 32*/
                                                                        /*  bits register reg, bit 0 is */
                                                                        /*  the LSB                     */
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
** 功能描述: 获得 Xfer:features:read:arch-core.xml 回复 XML
** 输　入  : NONE
** 输　出  : 回复 XML
** 全局变量:
** 调用模块:
*********************************************************************************************************/
CPCHAR  archGdbCoreXml (VOID)
{
    return  (cPpcCore);
}
/*********************************************************************************************************
** 函数名称: archGdbRegsGet
** 功能描述: 获取寄存器值
** 输　入  : pvDtrace       侦听 ip
**           ulThread       侦听端口
**
** 输　出  : pregset        gdb 寄存器结构
**           返回值         成功-- ERROR_NONE，失败-- PX_ERROR.
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archGdbRegsGet (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, GDB_REG_SET  *pregset)
{
    ARCH_REG_CTX  regctx;
    ARCH_REG_T    regSp;

    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);

    lib_bzero(pregset, sizeof(GDB_REG_SET));

    pregset->GDBR_iRegCnt = GDB_PPC_REG_NR;

    pregset->regArr[0].GDBRA_ulValue   = regctx.REG_uiR0;
    pregset->regArr[1].GDBRA_ulValue   = regSp;
    pregset->regArr[2].GDBRA_ulValue   = regctx.REG_uiR2;
    pregset->regArr[3].GDBRA_ulValue   = regctx.REG_uiR3;
    pregset->regArr[4].GDBRA_ulValue   = regctx.REG_uiR4;
    pregset->regArr[5].GDBRA_ulValue   = regctx.REG_uiR5;
    pregset->regArr[6].GDBRA_ulValue   = regctx.REG_uiR6;
    pregset->regArr[7].GDBRA_ulValue   = regctx.REG_uiR7;
    pregset->regArr[8].GDBRA_ulValue   = regctx.REG_uiR8;
    pregset->regArr[9].GDBRA_ulValue   = regctx.REG_uiR9;

    pregset->regArr[10].GDBRA_ulValue  = regctx.REG_uiR10;
    pregset->regArr[11].GDBRA_ulValue  = regctx.REG_uiR11;
    pregset->regArr[12].GDBRA_ulValue  = regctx.REG_uiR12;
    pregset->regArr[13].GDBRA_ulValue  = regctx.REG_uiR13;
    pregset->regArr[14].GDBRA_ulValue  = regctx.REG_uiR14;
    pregset->regArr[15].GDBRA_ulValue  = regctx.REG_uiR15;
    pregset->regArr[16].GDBRA_ulValue  = regctx.REG_uiR16;
    pregset->regArr[17].GDBRA_ulValue  = regctx.REG_uiR17;
    pregset->regArr[18].GDBRA_ulValue  = regctx.REG_uiR18;
    pregset->regArr[19].GDBRA_ulValue  = regctx.REG_uiR19;

    pregset->regArr[20].GDBRA_ulValue  = regctx.REG_uiR20;
    pregset->regArr[21].GDBRA_ulValue  = regctx.REG_uiR21;
    pregset->regArr[22].GDBRA_ulValue  = regctx.REG_uiR22;
    pregset->regArr[23].GDBRA_ulValue  = regctx.REG_uiR23;
    pregset->regArr[24].GDBRA_ulValue  = regctx.REG_uiR24;
    pregset->regArr[25].GDBRA_ulValue  = regctx.REG_uiR25;
    pregset->regArr[26].GDBRA_ulValue  = regctx.REG_uiR26;
    pregset->regArr[27].GDBRA_ulValue  = regctx.REG_uiR27;
    pregset->regArr[28].GDBRA_ulValue  = regctx.REG_uiR28;
    pregset->regArr[29].GDBRA_ulValue  = regctx.REG_uiR29;

    pregset->regArr[30].GDBRA_ulValue  = regctx.REG_uiR30;
    pregset->regArr[31].GDBRA_ulValue  = regctx.REG_uiFp;

    pregset->regArr[GDB_PPC_PC_INDEX].GDBRA_ulValue  = regctx.REG_uiSrr0;
    pregset->regArr[GDB_PPC_MSR_INDEX].GDBRA_ulValue = regctx.REG_uiSrr1;
    pregset->regArr[GDB_PPC_CR_INDEX].GDBRA_ulValue  = regctx.REG_uiCr;
    pregset->regArr[GDB_PPC_LR_INDEX].GDBRA_ulValue  = regctx.REG_uiLr;
    pregset->regArr[GDB_PPC_CTR_INDEX].GDBRA_ulValue = regctx.REG_uiCtr;
    pregset->regArr[GDB_PPC_XER_INDEX].GDBRA_ulValue = regctx.REG_uiXer;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archGdbRegsSet
** 功能描述: 设置寄存器值
** 输　入  : pvDtrace       侦听 ip
**           ulThread       侦听端口
**           pregset        gdb 寄存器结构
** 输　出  : 成功-- ERROR_NONE，失败-- PX_ERROR.
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archGdbRegsSet (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, GDB_REG_SET  *pregset)
{
    ARCH_REG_CTX  regctx;

    lib_bzero(&regctx, sizeof(ARCH_REG_CTX));

    regctx.REG_uiR0   = pregset->regArr[0].GDBRA_ulValue;
    regctx.REG_uiR2   = pregset->regArr[2].GDBRA_ulValue;
    regctx.REG_uiR3   = pregset->regArr[3].GDBRA_ulValue;
    regctx.REG_uiR4   = pregset->regArr[4].GDBRA_ulValue;
    regctx.REG_uiR5   = pregset->regArr[5].GDBRA_ulValue;
    regctx.REG_uiR6   = pregset->regArr[6].GDBRA_ulValue;
    regctx.REG_uiR7   = pregset->regArr[7].GDBRA_ulValue;
    regctx.REG_uiR8   = pregset->regArr[8].GDBRA_ulValue;
    regctx.REG_uiR9   = pregset->regArr[9].GDBRA_ulValue;

    regctx.REG_uiR10  = pregset->regArr[10].GDBRA_ulValue;
    regctx.REG_uiR11  = pregset->regArr[11].GDBRA_ulValue;
    regctx.REG_uiR12  = pregset->regArr[12].GDBRA_ulValue;
    regctx.REG_uiR13  = pregset->regArr[13].GDBRA_ulValue;
    regctx.REG_uiR14  = pregset->regArr[14].GDBRA_ulValue;
    regctx.REG_uiR15  = pregset->regArr[15].GDBRA_ulValue;
    regctx.REG_uiR16  = pregset->regArr[16].GDBRA_ulValue;
    regctx.REG_uiR17  = pregset->regArr[17].GDBRA_ulValue;
    regctx.REG_uiR18  = pregset->regArr[18].GDBRA_ulValue;
    regctx.REG_uiR19  = pregset->regArr[19].GDBRA_ulValue;

    regctx.REG_uiR20  = pregset->regArr[20].GDBRA_ulValue;
    regctx.REG_uiR21  = pregset->regArr[21].GDBRA_ulValue;
    regctx.REG_uiR22  = pregset->regArr[22].GDBRA_ulValue;
    regctx.REG_uiR23  = pregset->regArr[23].GDBRA_ulValue;
    regctx.REG_uiR24  = pregset->regArr[24].GDBRA_ulValue;
    regctx.REG_uiR25  = pregset->regArr[25].GDBRA_ulValue;
    regctx.REG_uiR26  = pregset->regArr[26].GDBRA_ulValue;
    regctx.REG_uiR27  = pregset->regArr[27].GDBRA_ulValue;
    regctx.REG_uiR28  = pregset->regArr[28].GDBRA_ulValue;
    regctx.REG_uiR29  = pregset->regArr[29].GDBRA_ulValue;

    regctx.REG_uiR30  = pregset->regArr[30].GDBRA_ulValue;
    regctx.REG_uiFp   = pregset->regArr[31].GDBRA_ulValue;

    regctx.REG_uiSrr0 = pregset->regArr[GDB_PPC_PC_INDEX].GDBRA_ulValue;
    regctx.REG_uiSrr1 = pregset->regArr[GDB_PPC_MSR_INDEX].GDBRA_ulValue;
    regctx.REG_uiCr   = pregset->regArr[GDB_PPC_CR_INDEX].GDBRA_ulValue;
    regctx.REG_uiLr   = pregset->regArr[GDB_PPC_LR_INDEX].GDBRA_ulValue;
    regctx.REG_uiCtr  = pregset->regArr[GDB_PPC_CTR_INDEX].GDBRA_ulValue;
    regctx.REG_uiXer  = pregset->regArr[GDB_PPC_XER_INDEX].GDBRA_ulValue;

    API_DtraceSetRegs(pvDtrace, ulThread, &regctx);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archGdbRegSetPc
** 功能描述: 设置 pc 寄存器值
** 输　入  : pvDtrace       侦听ip
**           ulThread       侦听端口
**           ulPC           pc 寄存器值
** 输　出  : 成功-- ERROR_NONE，失败-- PX_ERROR.
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archGdbRegSetPc (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, ULONG  ulPc)
{
    ARCH_REG_CTX  regctx;
    ARCH_REG_T    regSp;

    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);

    regctx.REG_uiSrr0 = (ARCH_REG_T)ulPc;

    API_DtraceSetRegs(pvDtrace, ulThread, &regctx);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archGdbRegGetPc
** 功能描述: 获取 pc 寄存器值
** 输　入  : pRegs       寄存器数组
** 输　出  : PC 寄存器值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG archGdbRegGetPc (GDB_REG_SET  *pRegs)
{
    return  (pRegs->regArr[GDB_PPC_PC_INDEX].GDBRA_ulValue);
}
/*********************************************************************************************************
** 函数名称: archGdbGetNextPc
** 功能描述: 获取下一条指令地址，含分支预测
** 输　入  : pRegs       寄存器数组
** 输　出  : 下一条指令地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG  archGdbGetNextPc (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, GDB_REG_SET  *pRegs)
{
    INSTR_T         machInstr;                                          /*  Machine instruction         */
    INSTR_T        *nextPc;                                             /*  next program counter        */
    UINT32          uiBranchType;
    UINT32          uiLI;                                               /*  LI field                    */
    UINT32          uiLR;                                               /*  LR field                    */
    UINT32          uiBD;                                               /*  BD field                    */
    UINT32          uiBO;                                               /*  BO field                    */
    UINT32          uiBI;                                               /*  BI field                    */
    ARCH_REG_T      uiCTR;                                              /*  CTR register                */
    INSTR_T        *uiCR;                                               /*  CR register                 */
    BOOL            bCond;
    UINT32          uiBO0, uiBO1, uiBO2, uiBO3, uiCRBI;                 /*  bits values                 */

                                                                        /*  Default next PC             */
    nextPc    =  (INSTR_T *)(pRegs->regArr[GDB_PPC_PC_INDEX].GDBRA_ulValue + 4);
    machInstr = *(INSTR_T *)(pRegs->regArr[GDB_PPC_PC_INDEX].GDBRA_ulValue);

    /*
     * tests for branch instructions:
     * bits 0-3 are common for all branch
     */
    if ((machInstr & BRANCH_MASK) == OP_BRANCH) {

        uiBranchType = (machInstr & 0xfc000000) >> 26;                  /*  opcode bits 0 to 5 equal    */
                                                                        /*  16,17,18 or 19              */

        uiCTR  = pRegs->regArr[GDB_PPC_CTR_INDEX].GDBRA_ulValue;
        uiCR   = (INSTR_T *)pRegs->regArr[GDB_PPC_CR_INDEX].GDBRA_ulValue;
        uiLR   = pRegs->regArr[GDB_PPC_LR_INDEX].GDBRA_ulValue;
        uiLI   = _IFIELD_LI(machInstr);
        uiBO   = _IFIELD_BO(machInstr);
        uiBI   = _IFIELD_BI(machInstr);
        uiBD   = _IFIELD_BD(machInstr);

        uiBO0  = _REG32_BIT(uiBO, (BO_NB_BIT - 1));                     /*  bit 0 of BO                 */
        uiBO1  = _REG32_BIT(uiBO, (BO_NB_BIT - 2));                     /*  bit 1 of BO                 */
        uiBO2  = _REG32_BIT(uiBO, (BO_NB_BIT - 3));                     /*  bit 2 of BO                 */
        uiBO3  = _REG32_BIT(uiBO, (BO_NB_BIT - 4));                     /*  bit 3 of BO                 */
                                                                        /*  bit bi of CR                */
        uiCRBI = _REG32_BIT((UINT32)uiCR, (CR_NB_BIT - 1 - ((UINT32)uiBI)));

        /* Switch on the type of branch (Bx, BCx, BCCTRx, BCLRx) */
        switch (uiBranchType) {

        case (16):                                                      /*  BC - Branch Conditional     */
        {
            if (uiBO2 == 0) {                                           /*  bit 2 of BO == 0            */
                uiCTR--;                                                /*  decrement CTR register      */
            }

            /* test branch condition */
            bCond = LW_FALSE;
            if ((uiBO2 == 1) || ((uiCTR != 0) ^ (uiBO3 != 0))) {
                if ((uiBO0 == 1) || (uiBO1 == uiCRBI)) {
                    bCond = LW_TRUE;
                }
            }
            if (bCond) {
                if (machInstr & AA_MASK) {                              /*  AA = 1 : absolute branch    */
                    nextPc = (INSTR_T *)uiBD;
                } else {                                                /*  AA = 0 : relative branch    */
                    nextPc = (INSTR_T *)(pRegs->regArr[GDB_PPC_PC_INDEX].GDBRA_ulValue + uiBD);
                }
            }
        }
            break;

        case (18):                                                      /*  B - Unconditional Branch    */
        {
            if (machInstr & AA_MASK) {                                  /*  AA = 1 : absolute branch    */
                nextPc = (INSTR_T *)uiLI;
            } else {                                                    /*  AA = 0 : relative branch    */
                nextPc = (INSTR_T *)(pRegs->regArr[GDB_PPC_PC_INDEX].GDBRA_ulValue + uiLI);
            }
        }
            break;

        case (19):                                                      /*  Bcctr or Bclr - Branch      */
                                                                        /*  Conditional to Register     */
        {
            if ((machInstr & BCCTR_MASK) == INST_BCCTR) {               /*  Bcctr - Branch Conditional  */
                                                                        /*  to Count Register           */
                if (uiBO2 == 0) {                                       /*  bit 2 of BO == 0            */
                    uiCTR--;                                            /*  decrement CTR register      */
                }

                /* test branch condition */
                bCond = LW_FALSE;
                if ((uiBO2 == 1) || ((uiCTR == 0) && (uiBO3 == 0))) {
                    if ((uiBO0 == 1) || (uiBO1 == uiCRBI)) {
                        bCond = LW_TRUE;
                    }
                }
                if (bCond) {                                            /*  branch relative to CTR      */
                    nextPc = (INSTR_T *)(uiCTR & 0xfffffffc);
                }

            } else if ((machInstr & BCLR_MASK) == INST_BCLR) {          /*  Bclr - Branch Conditional to*/
                                                                        /*  Link Register               */
                if (uiBO2 == 0) {                                       /*  bit 2 of BO == 0            */
                    uiCTR--;                                            /*  decrement CTR register      */
                }

                /* test branch condition */
                bCond = LW_FALSE;
                if ((uiBO2 == 1) || ((uiCTR == 0) && (uiBO3 == 0))) {
                    if ((uiBO0 == 1) || (uiBO1 == uiCRBI)) {
                        bCond = LW_TRUE;
                    }
                }
                if (bCond) {
                    nextPc = (INSTR_T *)(uiLR & 0xfffffffc);            /*  branch relative to LR       */
                }
            }
        }
            break;
        }
    }

    return  ((ULONG)nextPc);
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
