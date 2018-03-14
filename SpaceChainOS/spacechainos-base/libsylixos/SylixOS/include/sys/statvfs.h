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
** 文   件   名: statvfs.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 09 月 22 日
**
** 描        述: 文件系统信息.
*********************************************************************************************************/

#ifndef __SYS_STATVFS_H
#define __SYS_STATVFS_H

#include "sys/cdefs.h"
#include "sys/types.h"

struct statvfs {
    unsigned long f_bsize;                                              /* File system block size.      */
    unsigned long f_frsize;                                             /* Fundamental file system block*/
                                                                        /* size.                        */
    fsblkcnt_t    f_blocks;                                             /* Total number of blocks on    */
                                                                        /* file system in units of      */
                                                                        /* f_frsize.                    */
    fsblkcnt_t    f_bfree;                                              /* Total number of free blocks. */
    fsblkcnt_t    f_bavail;                                             /* Number of free blocks        */
                                                                        /* available to                 */
                                                                        /* non-privileged process.      */
    fsfilcnt_t    f_files;                                              /* Total number of file serial  */
                                                                        /* numbers.                     */
    fsfilcnt_t    f_ffree;                                              /* Total number of free file    */
                                                                        /* serial numbers.              */
    fsfilcnt_t    f_favail;                                             /* Number of file serial numbers*/
                                                                        /* available to                 */
                                                                        /* non-privileged process.      */
    unsigned long f_fsid;                                               /* File system ID.              */
    unsigned long f_flag;                                               /* Bit mask of f_flag values.   */
    unsigned long f_namemax;                                            /* Maximum filename length.     */
};

__BEGIN_DECLS
int statvfs(const char *, struct statvfs *);
int fstatvfs(int, struct statvfs *);
__END_DECLS

#endif                                                                  /*  __SYS_STATVFS_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
