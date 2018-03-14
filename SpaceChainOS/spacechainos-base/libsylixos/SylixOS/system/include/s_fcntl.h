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
** 文   件   名: s_fcntl.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: fcntl.h (2013.01.05 从 lib 文件夹移至此处)
*********************************************************************************************************/

#ifndef __S_FCNTL_H
#define __S_FCNTL_H

/*********************************************************************************************************
  POSIX flags
*********************************************************************************************************/

#define _FREAD                      0x0001                              /* read enabled                 */
#define _FWRITE                     0x0002                              /* write enabled                */
#define _FAPPEND                    0x0008                              /* append (writes guaranteed at */
                                                                        /* the end)                     */
#define _FDSYNC                     0x0020                              /* data sync                    */
#define _FASYNC                     0x0040                              /* signal pgrp when data ready  */
#define _FSHLOCK                    0x0080                              /* BSD flock() shared lock      */
                                                                        /* present                      */
#define _FEXLOCK                    0x0100                              /* BSD flock() exclusive lock   */
                                                                        /* present                      */
#define _FCREAT                     0x0200                              /* open with file create        */
#define _FTRUNC                     0x0400                              /* open with truncation         */
#define _FEXCL                      0x0800                              /* error on open if file exists */
#define _FNBIO                      0x1000                              /* non blocking I/O (sys5)      */
#define _FSYNC                      0x2000                              /* do all writes synchronously  */
#define _FNONBLOCK                  0x4000                              /* non blocking I/O (POSIX)     */
#define _FNOCTTY                    0x8000                              /* don't assign a ctty on this  */
                                                                        /* open                         */
#define _FBINARY                    0x10000                             /* binary mode                  */
#define _FNOFOLLOW                  0x20000                             /* don't follow symlink.        */
                                                                        
/*********************************************************************************************************
  POSIX 默认 mode
*********************************************************************************************************/

#ifndef DEFFILEMODE
#ifdef  DEFAULT_FILE_PERM
#define DEFFILEMODE                 DEFAULT_FILE_PERM
#else
#define DEFFILEMODE                 0644
#endif                                                                  /* DEFAULT_FILE_PERM            */
#endif                                                                  /* DEFFILEMODE                  */

/*********************************************************************************************************
  POSIX open flag
*********************************************************************************************************/

#define O_RDONLY                    0x0000
#define O_WRONLY                    0x0001
#define O_RDWR                      0x0002

#define O_ACCMODE                   (O_RDONLY | O_WRONLY | O_RDWR)

#define O_SHLOCK                    0x0010
#define O_EXLOCK                    0x0020
#define O_CREAT                     _FCREAT
#define O_TRUNC                     _FTRUNC
#define O_APPEND                    _FAPPEND
#define O_EXCL                      _FEXCL
#define O_NONBLOCK                  _FNONBLOCK
#define O_NDELAY                    _FNONBLOCK
#define O_SYNC                      _FSYNC
#define O_FSYNC                     _FSYNC
#define O_DSYNC                     _FDSYNC
#define O_ASYNC                     _FASYNC
#define O_NOCTTY                    _FNOCTTY
#define O_BINARY                    _FBINARY
#define O_NOFOLLOW                  _FNOFOLLOW
#define O_CLOEXEC                   0x80000
#define O_LARGEFILE                 0x100000

/*********************************************************************************************************
  fcntl
*********************************************************************************************************/

#define F_DUPFD                     0                                   /* Duplicate fildes             */
#define F_GETFD                     1                                   /* Get fildes flags (close on exec) 
                                                                                                        */
#define F_SETFD                     2                                   /* Set fildes flags (close on exec)
                                                                                                        */
#define F_GETFL                     3                                   /* Get file flags               */
#define F_SETFL                     4                                   /* Set file flags               */

#define F_GETOWN                    5                                   /* Get owner - for ASYNC        */
#define F_SETOWN                    6                                   /* Set owner - for ASYNC        */

#define F_GETLK                     7                                   /* Get record-locking           */
                                                                        /* information                  */
#define F_SETLK                     8                                   /* Set or Clear a record-lock   */
                                                                        /* (Non-Blocking)               */
#define F_SETLKW                    9                                   /* Set or Clear a record-lock   */
                                                                        /* (Blocking)                   */

#define F_RGETLK                    10                                  /* Test a remote lock to see if */
                                                                        /* it is blocked                */
#define F_RSETLK                    11                                  /* Set or unlock a remote lock  */
#define F_CNVT                      12                                  /* Convert a fhandle to an open */
                                                                        /* fd                           */
#define F_RSETLKW                   13                                  /* Set or Clear remote          */
                                                                        /* record-lock(Blocking)        */
#define F_GETSIG                    14                                  /* Get device notify signo      */
#define F_SETSIG                    15                                  /* Set device notify signo      */

/*********************************************************************************************************
  POSIX.1-2008
*********************************************************************************************************/

#define F_DUPFD_CLOEXEC             17                                  /* dup() fildes with cloexec    */

#define F_DUP2FD                    18                                  /* dup2() fildes                */
#define F_DUP2FD_CLOEXEC            19                                  /* dup2() fildes with cloexec   */
                                                                        
/*********************************************************************************************************
  FD_CLOEXEC
*********************************************************************************************************/

#ifndef FD_CLOEXEC
#define FD_CLOEXEC                  1
#endif                                                                  /* FD_CLOEXEC                   */

/*********************************************************************************************************
  warning : file lock only support in NEW_1 or higher version drivers.
*********************************************************************************************************/

#define F_RDLCK                     1
#define F_WRLCK                     2
#define F_UNLCK                     3

struct flock {
    short   l_type;                                                     /* F_RDLCK, F_WRLCK, or F_UNLCK */
    short   l_whence;                                                   /* flag to choose starting      */
                                                                        /* offset                       */
    off_t   l_start;                                                    /* relative offset, in bytes    */
    off_t   l_len;                                                      /* length, in bytes; 0 means    */
                                                                        /* lock to EOF                  */
    pid_t   l_pid;                                                      /* returned with F_GETLK        */
    
    long    l_xxx[4];                                                   /* reserved for future use      */
};

/*********************************************************************************************************
  eflock NOT SUPPORT NOW
*********************************************************************************************************/

struct eflock {
    short   l_type;                                                     /* F_RDLCK, F_WRLCK, or F_UNLCK */
    short   l_whence;                                                   /* flag to choose start offset  */
    off_t   l_start;                                                    /* relative offset, in bytes    */
    off_t   l_len;                                                      /* length, in bytes; 0 means EOF*/
    pid_t   l_pid;                                                      /* returned with F_GETLK        */
    pid_t   l_rpid;                                                     /* Remote process id wanting    */
    long    l_rsys;                                                     /* Remote system id wanting     */
    long    l_xxx[4];                                                   /* reserved for future use      */
};

LW_API INT  fcntl(INT  iFd, INT  iCmd, ...);

/*********************************************************************************************************
  flock() options
*********************************************************************************************************/

#define LOCK_SH                     F_RDLCK
#define LOCK_EX                     F_WRLCK
#define LOCK_NB                     0x0080
#define LOCK_UN                     F_UNLCK

/*********************************************************************************************************
  lockf() command
*********************************************************************************************************/

#define F_ULOCK                     0                                   /* unlock previously locked sec */
#define F_LOCK                      1                                   /* lock sec for exclusive use   */
#define F_TLOCK                     2                                   /* test & lock sec for excl use */
#define F_TEST                      3                                   /* test section for other locks */

#endif                                                                  /*  __S_FCNTL_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
