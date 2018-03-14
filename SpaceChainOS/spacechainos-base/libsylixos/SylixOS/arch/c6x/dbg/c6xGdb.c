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
** 文   件   名: c6xGdb.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2017 年 09 月 06 日
**
** 描        述: C6X 体系构架 GDB 调试接口.
**
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁减配置
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "dtrace.h"
#include "../c6x_gdb.h"
/*********************************************************************************************************
  Xfer:features:read:arch-core.xml 回应包
*********************************************************************************************************/
static const CHAR   cC6xCore[] = \
        "l<?xml version=\"1.0\"?>"
        "<!-- Copyright (C) 2006-2017 ACOINFO co.,ltd."
             "Copying and distribution of this file, with or without modification,"
             "are permitted in any medium without royalty provided the copyright"
             "notice and this notice are preserved.  -->"
        "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">"
        "<feature name=\"org.gnu.gdb.tic6x.core\">"
		"<reg name=\"A0\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A1\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A2\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A3\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A4\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A5\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A6\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A7\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A8\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A9\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A10\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A11\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A12\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A13\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A14\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A15\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B0\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B1\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B2\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B3\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B4\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B5\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B6\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B7\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B8\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B9\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B10\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B11\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B12\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B13\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B14\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B15\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"None\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"PC\" bitsize=\"32\" type=\"code_ptr\"/>"
		"<reg name=\"IRP\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"IFR\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"NPR\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A16\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A17\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A18\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A19\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A20\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A21\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A22\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A23\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A24\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A25\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A26\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A27\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A28\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A29\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A30\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A31\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B16\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B17\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B18\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B19\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B20\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B21\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B22\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B23\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B24\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B25\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B26\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B27\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B28\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B29\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B30\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B31\" bitsize=\"32\" type=\"uint32\"/>"
        "</feature>";
/*********************************************************************************************************
  Xfer:features:read:target.xml 回应包
*********************************************************************************************************/
static const CHAR   cTargetSystem[] = \
        "l<?xml version=\"1.0\"?>"
        "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
        "<target>"
		"<architecture>tic6x</architecture>"
		"<osabi>GNU/Linux</osabi>"
		"<xi:include href=\"arch-core.xml\"/>"
        "</target>";
/*********************************************************************************************************
  PC 与堆栈寄存器在 GDB_REG_SET 结构中的索引
*********************************************************************************************************/
#define C6X_REG_INDEX_PC        55
#define C6X_REG_INDEX_B15       31
/*********************************************************************************************************
  指令分析相关宏定义
*********************************************************************************************************/
#define TIC6X_OPCODE_SIZE       4                                       /* 指令宽度                     */
#define TIC6X_FETCH_PACKET_SIZE 32
#define INST_S_BIT(INST)        ((INST >>  1) & 1)
#define INST_X_BIT(INST)        ((INST >> 12) & 1)
/*********************************************************************************************************
  CPSR 寄存器在 GDB_REG_SET 结构中的索引
*********************************************************************************************************/
#define C6X_REGS_NUMS           69
/*********************************************************************************************************
  寄存器映射表
*********************************************************************************************************/
#if LW_CFG_CPU_ENDIAN == 1
static INT iRegMap[] = {
    /*
     * A0 - A15
     */
    77, 76, 73, 72, 69, 68, 65, 64,
    61, 60, 80, 81,  3,  2, 11, 10,

    /*
     * B0 - B15
     */
    79, 78, 75, 74, 71, 70, 67, 66,
    63, 62,  9, 82,  5,  4, 12, -1,

    /*
     * CSR PC IRP IFR NPR
     */
    13, 16, 16, -1, -1,

    /*
     * A16 - A31
     */
    57, 56, 53, 52, 49, 48, 45, 44,
    41, 40, 37, 36, 33, 32, 29, 28,

    /*
     * B16 -B31
     */
    59, 58, 55, 54, 51, 52, 47, 46,
    43, 42, 39, 38, 35, 34, 31, 30,
};

#elif LW_CFG_CPU_ENDIAN == 0
static INT iRegMap[] = {
    /*
     * A0 - A15
     */
    76, 77, 72, 73, 68, 69, 64, 65,
    60, 61, 80, 81,  2,  3, 10, 11,

    /*
     * B0 - B15
     */
    78, 79, 74, 75, 70, 71, 66, 67,
    62, 63,  8, 82,  4,  5, 12, -1,

    /*
     * CSR PC IRP IFR NPR
     */
    13, 16, 16, -1, -1,

    /*
     * A16 - A31
     */
    56, 57, 52, 53, 48, 49, 44, 45,
    40, 41, 36, 37, 32, 33, 28, 29,

    /*
     * B16 -B31
     */
    58, 59, 54, 55, 50, 51, 46, 47,
    42, 43, 38, 39, 34, 35, 30, 31,
};

#else
#error "TI Compiler does not specify endianness"
#endif
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
    return  (cC6xCore);
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
INT  archGdbRegsGet (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, GDB_REG_SET  *pregset)
{
    ARCH_REG_IRQ_CTX  regctx;
    ARCH_REG_T        regSp;
    INT               i = 0;
    ULONG            *pulRegVals;

    API_DtraceGetRegs(pvDtrace, ulThread, (ARCH_REG_CTX *)&regctx, &regSp);

    lib_bzero(pregset, sizeof(GDB_REG_SET));

    pregset->GDBR_iRegCnt = C6X_REGS_NUMS;
    pulRegVals = (ULONG *)&regctx;

    for (i = 0; i < pregset->GDBR_iRegCnt; i++) {
        if (iRegMap[i] != -1) {
            pregset->regArr[i].GDBRA_ulValue = pulRegVals[iRegMap[i]];
        }
    }

    pregset->regArr[C6X_REG_INDEX_B15].GDBRA_ulValue = regSp - 4;

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
INT  archGdbRegsSet (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, GDB_REG_SET  *pregset)
{
    ARCH_REG_IRQ_CTX  regctx;
    INT               i = 0;
    ULONG            *pulRegVals;

    pregset->GDBR_iRegCnt = C6X_REGS_NUMS;
    pulRegVals = (ULONG *)&regctx;

    for (i = 0; i < pregset->GDBR_iRegCnt; i++) {
        if (iRegMap[i] != -1) {
            pulRegVals[iRegMap[i]] = pregset->regArr[i].GDBRA_ulValue;
        }
    }

    API_DtraceSetRegs(pvDtrace, ulThread, (const ARCH_REG_CTX *)&regctx);

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
INT  archGdbRegSetPc (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, ULONG  ulPc)
{
    ARCH_REG_IRQ_CTX  regctx;
    ARCH_REG_T        regSp;

    API_DtraceGetRegs(pvDtrace, ulThread, (ARCH_REG_CTX *)&regctx, &regSp);

    regctx.REG_uiIrp = (ARCH_REG_T)ulPc;

    API_DtraceSetRegs(pvDtrace, ulThread, (const ARCH_REG_CTX *)&regctx);

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
ULONG  archGdbRegGetPc (GDB_REG_SET  *pRegs)
{
    return  (pRegs->regArr[C6X_REG_INDEX_PC].GDBRA_ulValue);
}
/*********************************************************************************************************
  以下代码实现分支预测
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: archTiC6xConditionTrue
** 功能描述: 判断指令执行条件是否满足
** 输　入  : pRegs          寄存器列表
**           ulInst         指令
** 输　出  : 条件满足返回1，否则返回0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  archTiC6xConditionTrue (GDB_REG_SET  *pRegs, ULONG  ulInst)
{
    static const INT  iRegisterNumbers[8] = { -1, 16, 17, 18, 1, 2, 0, -1 };
                 INT  iRegisterNumber;
                 INT  iRegisterValue;

    iRegisterNumber = iRegisterNumbers[(ulInst >> 29) & 7];
    if (iRegisterNumber == -1) {
        return 1;
    }

    iRegisterValue = pRegs->regArr[iRegisterNumber].GDBRA_ulValue;
    if ((ulInst & 0x10000000) != 0) {
        return iRegisterValue == 0;
    }

    return  (iRegisterValue != 0);
}
/*********************************************************************************************************
** 函数名称: archTiC6xRegisterNumber
** 功能描述: 转换寄存器号为gdb中的寄存器偏移
** 输　入  : iReg           寄存器号
**           iSide          side位
**           iCrossPath     c位
** 输　出  : 寄存器号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  archTiC6xRegisterNumber (INT  iReg, INT  iSide, INT  iCrossPath)
{
    INT  iRet = (iReg & 15) | ((iCrossPath ^ iSide) << 4);
	
    if ((iReg & 16) != 0) {                                             /* A16 - A31, B16 - B31         */
        iRet += 37;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: archTiC6xExtractSignedField
** 功能描述: 获取指令中的指定位域
** 输　入  : iValue         指令值
**           iLowBit        起始位
**           pRegs          位数
** 输　出  : 位域值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  archTiC6xExtractSignedField (INT  iValue, INT  iLowBit, INT  iBits)
{
    INT  iMask = (1 << iBits) - 1;
    INT  iRet  = (iValue >> iLowBit) & iMask;
	
    if ((iRet & (1 << (iBits - 1))) != 0) {
        iRet -= iMask + 1;
    }

    return  (iRet);
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
ULONG  archGdbGetNextPc (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, GDB_REG_SET  *pRegs)
{
    ULONG   ulNextPc;
    ULONG   uiInst;
    INT     iRegNum;

    do {
        uiInst = *((ULONG *)pRegs->regArr[C6X_REG_INDEX_PC].GDBRA_ulValue);

    	if (archTiC6xConditionTrue(pRegs, uiInst)) {
    	    if ((uiInst & 0x0000007c) == 0x00000010) {                  /* B with displacement          */
                ulNextPc &= ~(TIC6X_FETCH_PACKET_SIZE - 1);
                ulNextPc += archTiC6xExtractSignedField(uiInst, 7, 21) << 2;
                break;
            }

    	    if ((uiInst & 0x0f83effc) == 0x00000360) {                  /* B with register              */
                iRegNum = archTiC6xRegisterNumber((uiInst >> 18) & 0x1f,
                                                  INST_S_BIT (uiInst),
                                                  INST_X_BIT (uiInst));
                ulNextPc = pRegs->regArr[iRegNum].GDBRA_ulValue;
                break;
            }

            if ((uiInst & 0x00001ffc) == 0x00001020) {                  /* BDEC                         */
                iRegNum = archTiC6xRegisterNumber((uiInst >> 23) & 0x1f,
                                                  INST_S_BIT (uiInst), 0);
                if (((INT)pRegs->regArr[iRegNum].GDBRA_ulValue) >= 0) {
                    ulNextPc &= ~(TIC6X_FETCH_PACKET_SIZE - 1);
                    ulNextPc += archTiC6xExtractSignedField(uiInst, 7, 10) << 2;
                }
                break;
            }

            if ((uiInst & 0x00001ffc) == 0x00000120) {                  /* BNOP with displacement       */
                ulNextPc &= ~(TIC6X_FETCH_PACKET_SIZE - 1);
                ulNextPc += archTiC6xExtractSignedField(uiInst, 16, 12) << 2;
                break;
            }

            if ((uiInst & 0x0f830ffe) == 0x00800362) {                  /* BNOP with register           */
                iRegNum = archTiC6xRegisterNumber((uiInst >> 18) & 0x1f,
                                                  1, INST_X_BIT (uiInst));
                ulNextPc = pRegs->regArr[iRegNum].GDBRA_ulValue;
                break;
            }

            if ((uiInst & 0x00001ffc) == 0x00000020) {                  /* BPOS                         */
                iRegNum = archTiC6xRegisterNumber((uiInst >> 23) & 0x1f,
                                                  INST_S_BIT (uiInst), 0);
                if (((INT)pRegs->regArr[iRegNum].GDBRA_ulValue) >= 0) {
                    ulNextPc &= ~(TIC6X_FETCH_PACKET_SIZE - 1);
                    ulNextPc += archTiC6xExtractSignedField(uiInst, 13, 10) << 2;
                }
                break;
            }

            if ((uiInst & 0xf000007c) == 0x10000010) {                  /* CALLP                        */
                ulNextPc &= ~(TIC6X_FETCH_PACKET_SIZE - 1);
                ulNextPc += archTiC6xExtractSignedField(uiInst, 7, 21) << 2;
                break;
            }
        }

        ulNextPc += TIC6X_OPCODE_SIZE;
    } while ((uiInst & 1));

    return  (ulNextPc);
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
