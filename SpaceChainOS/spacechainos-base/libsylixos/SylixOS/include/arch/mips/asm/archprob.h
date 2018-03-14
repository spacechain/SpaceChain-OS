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
** 文   件   名: archprob.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 06 月 08 日
**
** 描        述: 平台编译探测.
*********************************************************************************************************/

#ifndef __ARCHPROB_H
#define __ARCHPROB_H

/*********************************************************************************************************
  定义 MIPS 架构是大/小端存储
*********************************************************************************************************/

#if defined(MIPSEB) || defined(__MIPSEB__)
#undef __SYLIXOS_MIPS_BYTE_ORDER
#define __SYLIXOS_MIPS_BYTE_ORDER   _BIG_ENDIAN

#elif defined(MIPSEL) || defined(__MIPSEL__)
#undef __SYLIXOS_MIPS_BYTE_ORDER
#define __SYLIXOS_MIPS_BYTE_ORDER   _LITTLE_ENDIAN

#else
#warning "One of MIPSEL or MIPSEB must be defined."
#endif

/*********************************************************************************************************
  MIPS32 / MIPS64
*********************************************************************************************************/

#define __SYLIXOS_CPU_MIPS_32BIT    ((__SYLIXOS_CPU == __SYLIXOS_MIPS32)  || \
                                     (__SYLIXOS_CPU == __SYLIXOS_MIPSI2)  || \
                                     (__SYLIXOS_CPU == __SYLIXOS_MIPSI32) || \
                                     (__SYLIXOS_CPU == __SYLIXOS_MIPSI32R2))

#define __SYLIXOS_CPU_MIPS_64BIT    ((__SYLIXOS_CPU == __SYLIXOS_MIPS64)  || \
                                     (__SYLIXOS_CPU == __SYLIXOS_MIPSI3)  || \
                                     (__SYLIXOS_CPU == __SYLIXOS_MIPSI64) || \
                                     (__SYLIXOS_CPU == __SYLIXOS_MIPSI64R2))

#endif                                                                  /*  __ARCHPROB_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
