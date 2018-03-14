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
** 文   件   名: cdumpLib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 01 日
**
** 描        述: 系统/应用崩溃信息记录.
*********************************************************************************************************/

#ifndef __CDUMP_LIB_H
#define __CDUMP_LIB_H

/*********************************************************************************************************
  宏定义
*********************************************************************************************************/

#define LW_CDUMP_MAGIC_0    0xab
#define LW_CDUMP_MAGIC_1    0x56
#define LW_CDUMP_MAGIC_2    0xef
#define LW_CDUMP_MAGIC_3    0x33
#define LW_CDUMP_MAGIC_LEN  4

/*********************************************************************************************************
  内部函数
*********************************************************************************************************/

VOID  _CrashDumpAbortStkOf(addr_t  ulRetAddr, addr_t  ulAbortAddr, CPCHAR  pcInfo, PLW_CLASS_TCB  ptcb);
VOID  _CrashDumpAbortFatal(addr_t  ulRetAddr, addr_t  ulAbortAddr, CPCHAR  pcInfo);
VOID  _CrashDumpAbortKernel(LW_OBJECT_HANDLE   ulOwner, 
                            CPCHAR             pcKernelFunc, 
                            PVOID              pvCtx,
                            CPCHAR             pcInfo, 
                            CPCHAR             pcTail);
VOID  _CrashDumpAbortAccess(PVOID  pcCtx, CPCHAR  pcInfo);
                            
VOID  _CrashDumpSet(PVOID  pvCdump, size_t  stSize);
PVOID _CrashDumpGet(size_t  *pstSize);

#endif                                                                  /*  __CDUMP_LIB_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
