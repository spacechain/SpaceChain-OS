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
** 文   件   名: px_utsname.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 03 月 10 日
**
** 描        述: posix utsname 兼容库.
*********************************************************************************************************/

#ifndef __PX_UTSNAME_H
#define __PX_UTSNAME_H

#include "SylixOS.h"                                                    /*  操作系统头文件              */
#include "limits.h"

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  utsname structure
*********************************************************************************************************/

#define __PX_UTSNAME_SYSNAME_SIZE           16
#define __PX_UTSNAME_NODENAME_SIZE          MAX_FILENAME_LENGTH
#define __PX_UTSNAME_RELEASE_SIZE           64
#define __PX_UTSNAME_VERSION_SIZE           128
#define __PX_UTSNAME_MACHINE_SIZE           64

struct utsname {
    char  sysname[__PX_UTSNAME_SYSNAME_SIZE];                           /* Name of this implementation  */
                                                                        /* of the operating system.     */
                                                                        
    char  nodename[__PX_UTSNAME_NODENAME_SIZE];                         /* Name of this node within the */
                                                                        /* communications network to    */
                                                                        /* which this node is attached, */
                                                                        /* if any.                      */
    
    char  release[__PX_UTSNAME_RELEASE_SIZE];                           /* Current release level of this*/
                                                                        /* implementation.              */
                                                                        
    char  version[__PX_UTSNAME_VERSION_SIZE];                           /* Current version level of this*/
                                                                        /* release.                     */
                                                                        
    char  machine[__PX_UTSNAME_MACHINE_SIZE];                           /* Name of the hardware type on */
                                                                        /* which the system is running. */
};

/*********************************************************************************************************
  timeb api
*********************************************************************************************************/

LW_API int  uname(struct utsname *name);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
#endif                                                                  /*  __PX_UTSNAME_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
