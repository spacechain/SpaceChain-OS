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
** 文   件   名: mount.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 04 月 02 日
**
** 描        述: mount 挂载库.
*********************************************************************************************************/

#ifndef __SYS_MOUNT_H
#define __SYS_MOUNT_H

/*********************************************************************************************************
  These are the fs-independent mount-flags: only support MS_RDONLY flag. 
*********************************************************************************************************/

enum {
  MS_RDONLY = 1,                                            /* Mount read-only.                         */
#define MS_RDONLY           MS_RDONLY
  MS_NOSUID = 2,                                            /* Ignore suid and sgid bits.               */
#define MS_NOSUID           MS_NOSUID
  MS_NODEV = 4,                                             /* Disallow access to device                */
#define MS_NODEV            MS_NODEV                        /* special files.Disallow program execution.*/
  MS_NOEXEC = 8,
#define MS_NOEXEC           MS_NOEXEC
  MS_SYNCHRONOUS = 16,                                      /* Writes are synced at once.               */
#define MS_SYNCHRONOUS      MS_SYNCHRONOUS
  MS_REMOUNT = 32,                                          /* Alter flags of a mounted FS.             */
#define MS_REMOUNT          MS_REMOUNT
  MS_MANDLOCK = 64,                                         /* Allow mandatory locks on FS              */
#define MS_MANDLOCK         MS_MANDLOCK
  S_WRITE = 128,                                            /* Write on file/directory/symlink.         */
#define S_WRITE             S_WRITE
  S_APPEND = 256,                                           /* Append-only file.                        */
#define S_APPEND            S_APPEND
  S_IMMUTABLE = 512,                                        /* Immutable file.                          */
#define S_IMMUTABLE         S_IMMUTABLE
  MS_NOATIME = 1024,                                        /* Do not update access times.              */
#define MS_NOATIME          MS_NOATIME
  MS_NODIRATIME = 2048,                                     /* Do not update directory access times.    */
#define MS_NODIRATIME       MS_NODIRATIME
  MS_BIND = 4096                                            /* Bind directory at different place.       */
#define MS_BIND             MS_BIND
};

/*********************************************************************************************************
  Flags that can be altered by MS_REMOUNT 
*********************************************************************************************************/

#define MS_RMT_MASK (MS_RDONLY | MS_SYNCHRONOUS | MS_MANDLOCK | MS_NOATIME | \
                     MS_NODIRATIME)

/*********************************************************************************************************
  Magic mount flag number. Has to be or-ed to the flag values. 
*********************************************************************************************************/

#define MS_MGC_VAL  0xc0ed0000                              /* Magic flag number to indicate "new" flags*/
#define MS_MGC_MSK  0xffff0000                              /* Magic flag number mask                   */

/*********************************************************************************************************
  Possible value for FLAGS parameter of `umount2'.  
*********************************************************************************************************/

enum {
  MNT_FORCE = 1                                             /* Force unmounting.                        */
#define MNT_FORCE MNT_FORCE
};

#ifdef __cplusplus
extern "C" {
#endif

extern int mount(const char *source, const char *target,
                 const char *fstype, unsigned long int rwflag,
                 const void *data);                         /* Mount a filesystem.                      */

extern int umount(const char *target);                      /* Unmount a filesystem.                    */

extern int umount2(const char *target, int flags);          /* Unmount a filesystem.                    */
                                                            /* use MNT_FORCE Force unmount              */
#ifdef __cplusplus
}
#endif

#endif                                                      /*  __SYS_MOUNT_H                           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
