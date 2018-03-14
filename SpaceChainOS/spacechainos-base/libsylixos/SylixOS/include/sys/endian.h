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
** 文   件   名: endian.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 05 月 22 日
**
** 描        述: 系统大小端.
*********************************************************************************************************/

#ifndef __SYS_ENDIAN_H
#define __SYS_ENDIAN_H

#include "../SylixOS/config/cpu/cpu_cfg.h"
#include "../SylixOS/config/kernel/endian_cfg.h"

#undef  BYTE_ORDER
#undef  __BYTE_ORDER

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN               1234
#endif                                                                  /*  LITTLE_ENDIAN               */
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN             LITTLE_ENDIAN
#endif                                                                  /*  __LITTLE_ENDIAN             */

#ifndef BIG_ENDIAN
#define BIG_ENDIAN                  4321
#endif                                                                  /*  BIG_ENDIAN                  */
#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN                BIG_ENDIAN
#endif                                                                  /*  __BIG_ENDIAN                */

#if LW_CFG_CPU_ENDIAN > 0
#define BYTE_ORDER                  BIG_ENDIAN
#define __BYTE_ORDER                BIG_ENDIAN
#else
#define BYTE_ORDER                  LITTLE_ENDIAN
#define __BYTE_ORDER                LITTLE_ENDIAN
#endif                                                                  /*  LW_CFG_CPU_ENDIAN > 0       */

#endif                                                                  /*  __SYS_ENDIAN_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
