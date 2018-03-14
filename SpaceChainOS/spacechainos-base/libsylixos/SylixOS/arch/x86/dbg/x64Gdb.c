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
** 文   件   名: x64Gdb.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 06 月 07 日
**
** 描        述: x86-64 体系构架 GDB 调试接口.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
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
static const CHAR   _G_cX86CoreXml[] = \
    "l<?xml version=\"1.0\"?>"
    "<!-- Copyright (C) 2006-2017 ACOINFO co.,ltd."
        "Copying and distribution of this file, with or without modification,"
        "are permitted in any medium without royalty provided the copyright"
        "notice and this notice are preserved.  -->"
        "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">"
    "<feature name=\"org.gnu.gdb.i386.core\">"
        "<flags id=\"i386_eflags\" size=\"4\">"
        "<field name=\"CF\" start=\"0\" end=\"0\"/>"
        "<field name=\"\" start=\"1\" end=\"1\"/>"
        "<field name=\"PF\" start=\"2\" end=\"2\"/>"
        "<field name=\"AF\" start=\"4\" end=\"4\"/>"
        "<field name=\"ZF\" start=\"6\" end=\"6\"/>"
        "<field name=\"SF\" start=\"7\" end=\"7\"/>"
        "<field name=\"TF\" start=\"8\" end=\"8\"/>"
        "<field name=\"IF\" start=\"9\" end=\"9\"/>"
        "<field name=\"DF\" start=\"10\" end=\"10\"/>"
        "<field name=\"OF\" start=\"11\" end=\"11\"/>"
        "<field name=\"NT\" start=\"14\" end=\"14\"/>"
        "<field name=\"RF\" start=\"16\" end=\"16\"/>"
        "<field name=\"VM\" start=\"17\" end=\"17\"/>"
        "<field name=\"AC\" start=\"18\" end=\"18\"/>"
        "<field name=\"VIF\" start=\"19\" end=\"19\"/>"
        "<field name=\"VIP\" start=\"20\" end=\"20\"/>"
        "<field name=\"ID\" start=\"21\" end=\"21\"/>"
      "</flags>"
      "<reg name=\"rax\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"rbx\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"rcx\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"rdx\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"rsi\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"rdi\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"rbp\" bitsize=\"64\" type=\"data_ptr\"/>"
      "<reg name=\"rsp\" bitsize=\"64\" type=\"data_ptr\"/>"
      "<reg name=\"r8\"  bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"r9\"  bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"r10\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"r11\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"r12\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"r13\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"r14\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"r15\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"rip\" bitsize=\"64\" type=\"code_ptr\"/>"
      "<reg name=\"eflags\" bitsize=\"32\" type=\"i386_eflags\"/>"
      "<reg name=\"cs\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"ss\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"ds\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"es\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"fs\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"gs\" bitsize=\"32\" type=\"int32\"/>"
    "</feature>";
/*********************************************************************************************************
  Xfer:features:read:target.xml 回应包
*********************************************************************************************************/
static const CHAR   _G_cX86TargetXml[] = \
    "l<?xml version=\"1.0\"?>"
    "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
    "<target>"
        "<architecture>i386:x86-64</architecture>"
        "<xi:include href=\"arch-core.xml\"/>"
    "</target>";
/*********************************************************************************************************
  寄存器在数组中的索引
*********************************************************************************************************/
#define GDB_X64_RAX_INDEX       0
#define GDB_X64_RBX_INDEX       1
#define GDB_X64_RCX_INDEX       2
#define GDB_X64_RDX_INDEX       3
#define GDB_X64_RSI_INDEX       4
#define GDB_X64_RDI_INDEX       5
#define GDB_X64_RBP_INDEX       6
#define GDB_X64_RSP_INDEX       7
#define GDB_X64_R8_INDEX        8
#define GDB_X64_R9_INDEX        9
#define GDB_X64_R10_INDEX       10
#define GDB_X64_R11_INDEX       11
#define GDB_X64_R12_INDEX       12
#define GDB_X64_R13_INDEX       13
#define GDB_X64_R14_INDEX       14
#define GDB_X64_R15_INDEX       15
#define GDB_X64_RIP_INDEX       16
#define GDB_X64_EFLAGS_INDEX    17
#define GDB_X64_CS_INDEX        18
#define GDB_X64_SS_INDEX        19
#define GDB_X64_DS_INDEX        20
#define GDB_X64_ES_INDEX        21
#define GDB_X64_FS_INDEX        22
#define GDB_X64_GS_INDEX        23
#define GDB_X64_REG_NR          24
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
    return  (_G_cX86TargetXml);
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
    return  (_G_cX86CoreXml);
}
/*********************************************************************************************************
** 函数名称: archGdbRegsGet
** 功能描述: 获取寄存器值
** 输　入  : pvDtrace       dtrace 节点
**           ulThread       线程句柄
**           pregset        gdb 寄存器结构
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archGdbRegsGet (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, GDB_REG_SET  *pregset)
{
    ARCH_REG_CTX  regctx;
    ARCH_REG_T    regSp;

    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);

    lib_bzero(pregset, sizeof(GDB_REG_SET));

    pregset->GDBR_iRegCnt = GDB_X64_REG_NR;

    pregset->regArr[GDB_X64_RAX_INDEX].GDBRA_ulValue    = regctx.REG_ulRAX;
    pregset->regArr[GDB_X64_RBX_INDEX].GDBRA_ulValue    = regctx.REG_ulRBX;
    pregset->regArr[GDB_X64_RCX_INDEX].GDBRA_ulValue    = regctx.REG_ulRCX;
    pregset->regArr[GDB_X64_RDX_INDEX].GDBRA_ulValue    = regctx.REG_ulRDX;

    pregset->regArr[GDB_X64_RSI_INDEX].GDBRA_ulValue    = regctx.REG_ulRSI;
    pregset->regArr[GDB_X64_RDI_INDEX].GDBRA_ulValue    = regctx.REG_ulRDI;

    pregset->regArr[GDB_X64_RBP_INDEX].GDBRA_ulValue    = regctx.REG_ulRBP;
    pregset->regArr[GDB_X64_RSP_INDEX].GDBRA_ulValue    = regSp;

    pregset->regArr[GDB_X64_R8_INDEX].GDBRA_ulValue     = regctx.REG_ulR8;
    pregset->regArr[GDB_X64_R9_INDEX].GDBRA_ulValue     = regctx.REG_ulR9;
    pregset->regArr[GDB_X64_R10_INDEX].GDBRA_ulValue    = regctx.REG_ulR10;
    pregset->regArr[GDB_X64_R11_INDEX].GDBRA_ulValue    = regctx.REG_ulR11;
    pregset->regArr[GDB_X64_R12_INDEX].GDBRA_ulValue    = regctx.REG_ulR12;
    pregset->regArr[GDB_X64_R13_INDEX].GDBRA_ulValue    = regctx.REG_ulR13;
    pregset->regArr[GDB_X64_R14_INDEX].GDBRA_ulValue    = regctx.REG_ulR14;
    pregset->regArr[GDB_X64_R15_INDEX].GDBRA_ulValue    = regctx.REG_ulR15;

    pregset->regArr[GDB_X64_RIP_INDEX].GDBRA_ulValue    = regctx.REG_ulRIP;

    pregset->regArr[GDB_X64_EFLAGS_INDEX].GDBRA_ulValue = regctx.REG_ulRFLAGS;

    pregset->regArr[GDB_X64_CS_INDEX].GDBRA_ulValue     = regctx.REG_ulCS;
    pregset->regArr[GDB_X64_SS_INDEX].GDBRA_ulValue     = regctx.REG_ulSS;
    pregset->regArr[GDB_X64_DS_INDEX].GDBRA_ulValue     = regctx.REG_usDS;
    pregset->regArr[GDB_X64_ES_INDEX].GDBRA_ulValue     = regctx.REG_usES;
    pregset->regArr[GDB_X64_FS_INDEX].GDBRA_ulValue     = regctx.REG_usFS;
    pregset->regArr[GDB_X64_GS_INDEX].GDBRA_ulValue     = regctx.REG_usGS;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archGdbRegsSet
** 功能描述: 设置寄存器值
** 输　入  : pvDtrace       dtrace 节点
**           ulThread       线程句柄
**           pregset        gdb 寄存器结构
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archGdbRegsSet (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, GDB_REG_SET  *pregset)
{
    ARCH_REG_CTX  regctx;

    lib_bzero(&regctx, sizeof(ARCH_REG_CTX));

    regctx.REG_ulRAX    = pregset->regArr[GDB_X64_RAX_INDEX].GDBRA_ulValue;
    regctx.REG_ulRBX    = pregset->regArr[GDB_X64_RBX_INDEX].GDBRA_ulValue;
    regctx.REG_ulRCX    = pregset->regArr[GDB_X64_RCX_INDEX].GDBRA_ulValue;
    regctx.REG_ulRDX    = pregset->regArr[GDB_X64_RDX_INDEX].GDBRA_ulValue;

    regctx.REG_ulRSI    = pregset->regArr[GDB_X64_RSI_INDEX].GDBRA_ulValue;
    regctx.REG_ulRDI    = pregset->regArr[GDB_X64_RDI_INDEX].GDBRA_ulValue;

    regctx.REG_ulRBP    = pregset->regArr[GDB_X64_RBP_INDEX].GDBRA_ulValue;

    regctx.REG_ulR8     = pregset->regArr[GDB_X64_R8_INDEX].GDBRA_ulValue;
    regctx.REG_ulR9     = pregset->regArr[GDB_X64_R9_INDEX].GDBRA_ulValue;
    regctx.REG_ulR10    = pregset->regArr[GDB_X64_R10_INDEX].GDBRA_ulValue;
    regctx.REG_ulR11    = pregset->regArr[GDB_X64_R11_INDEX].GDBRA_ulValue;
    regctx.REG_ulR12    = pregset->regArr[GDB_X64_R12_INDEX].GDBRA_ulValue;
    regctx.REG_ulR13    = pregset->regArr[GDB_X64_R13_INDEX].GDBRA_ulValue;
    regctx.REG_ulR14    = pregset->regArr[GDB_X64_R14_INDEX].GDBRA_ulValue;
    regctx.REG_ulR15    = pregset->regArr[GDB_X64_R15_INDEX].GDBRA_ulValue;

    regctx.REG_ulRIP    = pregset->regArr[GDB_X64_RIP_INDEX].GDBRA_ulValue;

    regctx.REG_ulRFLAGS = pregset->regArr[GDB_X64_EFLAGS_INDEX].GDBRA_ulValue;

    regctx.REG_ulCS     = pregset->regArr[GDB_X64_CS_INDEX].GDBRA_ulValue;
    regctx.REG_ulSS     = pregset->regArr[GDB_X64_SS_INDEX].GDBRA_ulValue;
    regctx.REG_usDS     = pregset->regArr[GDB_X64_DS_INDEX].GDBRA_ulValue;
    regctx.REG_usES     = pregset->regArr[GDB_X64_ES_INDEX].GDBRA_ulValue;
    regctx.REG_usFS     = pregset->regArr[GDB_X64_FS_INDEX].GDBRA_ulValue;
    regctx.REG_usGS     = pregset->regArr[GDB_X64_GS_INDEX].GDBRA_ulValue;

    API_DtraceSetRegs(pvDtrace, ulThread, &regctx);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archGdbRegSetPc
** 功能描述: 设置 PC 寄存器值
** 输　入  : pvDtrace       dtrace 节点
**           ulThread       线程句柄
**           ulPC           PC 寄存器值
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archGdbRegSetPc (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, ULONG  ulPc)
{
    ARCH_REG_CTX  regctx;
    ARCH_REG_T    regSp;

    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);

    regctx.REG_ulRIP = ulPc;

    API_DtraceSetRegs(pvDtrace, ulThread, &regctx);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archGdbRegGetPc
** 功能描述: 获取 PC 寄存器值
** 输　入  : pRegs       寄存器数组
** 输　出  : PC 寄存器值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG archGdbRegGetPc (GDB_REG_SET  *pRegs)
{
    return  (pRegs->regArr[GDB_X64_RIP_INDEX].GDBRA_ulValue);
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
