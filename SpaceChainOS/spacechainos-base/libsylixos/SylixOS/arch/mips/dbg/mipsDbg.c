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
** 文   件   名: mipsDbg.c
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 09 月 01 日
**
** 描        述: MIPS 体系构架调试相关.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁减配置
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "dtrace.h"
/*********************************************************************************************************
  MIPS 断点使用 break 指令.
*********************************************************************************************************/
#define MIPS_BREAKPOINT_INS     0x0000000D
#define MIPS_ABORTPOINT_INS     0x0001000D
/*********************************************************************************************************
  SMP
*********************************************************************************************************/
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CACHE_EN > 0) && (LW_CFG_GDB_SMP_TU_LAZY > 0)
static addr_t   ulLastBpAddr[LW_CFG_MAX_PROCESSORS];
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** 函数名称: archDbgBpInsert
** 功能描述: 插入一个断点.
** 输　入  : ulAddr         断点地址
**           stSize         断点大小
**           pulIns         返回的之前的指令
**           bLocal         是否仅更新当前 CPU I-CACHE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archDbgBpInsert (addr_t  ulAddr, size_t stSize, ULONG  *pulIns, BOOL  bLocal)
{
    ULONG ulIns = MIPS_BREAKPOINT_INS;

    lib_memcpy((PCHAR)pulIns, (PCHAR)ulAddr, stSize);                   /*  memcpy 避免对齐问题         */
    lib_memcpy((PCHAR)ulAddr, (PCHAR)&ulIns, stSize);
    KN_SMP_MB();

#if LW_CFG_CACHE_EN > 0
    if (bLocal) {
        API_CacheLocalTextUpdate((PVOID)ulAddr, stSize);
    } else {
        API_CacheTextUpdate((PVOID)ulAddr, stSize);
    }
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** 函数名称: archDbgAbInsert
** 功能描述: 插入一个异常点.
** 输　入  : ulAddr         断点地址
**           pulIns         返回的之前的指令
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 插入异常断点时, 不考虑 CPU 运行模式, 直接插入 32 位断点即可.
*********************************************************************************************************/
VOID  archDbgAbInsert (addr_t  ulAddr, ULONG  *pulIns)
{
    *pulIns = *(ULONG *)ulAddr;
    *(ULONG *)ulAddr = MIPS_ABORTPOINT_INS;
    KN_SMP_MB();

#if LW_CFG_CACHE_EN > 0
    API_CacheTextUpdate((PVOID)ulAddr, sizeof(ULONG));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** 函数名称: archDbgBpRemove
** 功能描述: 删除一个断点.
** 输　入  : ulAddr         断点地址
**           stSize         断点大小
**           pulIns         返回的之前的指令
**           bLocal         是否仅更新当前 CPU I-CACHE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archDbgBpRemove (addr_t  ulAddr, size_t stSize, ULONG  ulIns, BOOL  bLocal)
{
    lib_memcpy((PCHAR)ulAddr, (PCHAR)&ulIns, stSize);
    KN_SMP_MB();

#if LW_CFG_CACHE_EN > 0
    if (bLocal) {
        API_CacheLocalTextUpdate((PVOID)ulAddr, stSize);
    } else {
        API_CacheTextUpdate((PVOID)ulAddr, stSize);
    }
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** 函数名称: archDbgBpPrefetch
** 功能描述: 预取一个指令.
             当指令处于 MMU 共享物理段时, 指令空间为物理只读, 这里需要产生一次缺页中断, 克隆一个物理页面.
** 输　入  : ulAddr         断点地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archDbgBpPrefetch (addr_t  ulAddr)
{
    volatile UINT8  ucByte = *(UINT8 *)ulAddr;                          /*  读取断点处数据              */

    *(UINT8 *)ulAddr = ucByte;                                          /*  执行一次写操作, 产生页面中断*/
}
/*********************************************************************************************************
** 函数名称: archDbgTrapType
** 功能描述: 获取 trap 类型.
** 输　入  : ulAddr         断点地址
**           pvArch         体系结构相关参数 (ARM 处理器为 CPSR)
** 输　出  : LW_TRAP_INVAL / LW_TRAP_BRKPT / LW_TRAP_ABORT
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT  archDbgTrapType (addr_t  ulAddr, PVOID   pvArch)
{
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CACHE_EN > 0) && (LW_CFG_GDB_SMP_TU_LAZY > 0)
    ULONG   ulCPUId;
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    if (API_DtraceIsValid() == LW_FALSE) {                              /*  不存在调试节点              */
        return  (LW_TRAP_INVAL);
    }

    switch (*(ULONG *)ulAddr) {

    case MIPS_BREAKPOINT_INS:
        return  (LW_TRAP_BRKPT);

    case MIPS_ABORTPOINT_INS:
        return  (LW_TRAP_ABORT);

    default:
        break;
    }

#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CACHE_EN > 0) && (LW_CFG_GDB_SMP_TU_LAZY > 0)
    if (API_CacheGetOption() & CACHE_TEXT_UPDATE_MP) {
        ulCPUId = LW_CPU_GET_CUR_ID();
        if (ulLastBpAddr[ulCPUId] == ulAddr) {                          /*  不是断点的停止              */
            ulLastBpAddr[ulCPUId] =  (addr_t)PX_ERROR;                  /*  同一地址连续失效            */
            return  (LW_TRAP_INVAL);

        } else {
            ulLastBpAddr[ulCPUId] = ulAddr;
            API_CacheLocalTextUpdate((PVOID)ulAddr, sizeof(ulAddr));    /*  刷新一次 I CACHE 再去尝试   */
            return  (LW_TRAP_RETRY);
        }
    } else
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
                                                                        /*  LW_CFG_CACHE_EN > 0         */
                                                                        /*  LW_CFG_GDB_SMP_TU_LAZY > 0  */
    {
        return  (LW_TRAP_INVAL);
    }
}
/*********************************************************************************************************
** 函数名称: archDbgBpAdjust
** 功能描述: 根据体系结构调整断点地址.
** 输　入  : pvDtrace       dtrace 节点
**           pdtm           获取的信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archDbgBpAdjust (PVOID  pvDtrace, PVOID  pvtm)
{
    ARCH_REG_CTX    regctx;
    ARCH_REG_T      regSp;
    PLW_DTRACE_MSG  pdtm = (PLW_DTRACE_MSG)pvtm;

    API_DtraceGetRegs(pvDtrace, pdtm->DTM_ulThread, &regctx, &regSp);

    /*
     * 如果 Cause 寄存器 BD 位置为 1，则说明引发中断的为分支延时槽指令，PC 寄存器值需调整
     */
    if (regctx.REG_uiCP0Cause & M_CauseBD) {
        pdtm->DTM_ulAddr += sizeof(ULONG);
    }
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
