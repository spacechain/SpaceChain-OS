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
** 文   件   名: gjbext.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 04 月 13 日
**
** 描        述: 兼容 GJB7714 接口库.
*********************************************************************************************************/

#ifndef __GJBEXT_H
#define __GJBEXT_H

#ifndef __PX_GJBEXT_H
#include "../SylixOS/posix/include/px_gjbext.h"
#endif                                                                  /*  __PX_GJBEXT_H               */

#ifndef __PX_PTHREAD_H
#include "../SylixOS/posix/include/px_pthread.h"
#endif                                                                  /*  __PX_PTHREAD_H              */

#ifndef __PX_SEMAPHORE_H
#include "../SylixOS/posix/include/px_semaphore.h"
#endif                                                                  /*  __PX_SEMAPHORE_H            */

#ifndef __PX_MQUEUE_H
#include "../SylixOS/posix/include/px_mqueue.h"
#endif                                                                  /*  __PX_MQUEUE_H               */

#if (LW_CFG_POSIX_EN > 0) && (LW_CFG_GJB7714_EN > 0)
#define mount   gjb_mount
#define umount  gjb_umount
#define format  gjb_format
#define cat     gjb_cat
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_GJB7714_EN > 0       */
#endif                                                                  /*  __GJBEXT_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
