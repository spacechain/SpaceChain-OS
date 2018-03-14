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
** 文   件   名: cpio.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 03 月 05 日
**
** 描        述: 兼容 posix cpio.
*********************************************************************************************************/

#ifndef __CPIO_H
#define __CPIO_H

#define C_IRUSR                 000400
#define C_IWUSR                 000200
#define C_IXUSR                 000100

#define C_IRGRP                 000040
#define C_IWGRP                 000020
#define C_IXGRP                 000010

#define C_IROTH                 000004
#define C_IWOTH                 000002
#define C_IXOTH                 000001

#define C_ISUID                 004000
#define C_ISGID                 002000
#define C_ISVTX                 001000

#define C_ISBLK                 060000
#define C_ISCHR                 020000
#define C_ISDIR                 040000
#define C_ISFIFO                010000
#define C_ISSOCK                0140000
#define C_ISLNK                 0120000
#define C_ISCTG                 0110000
#define C_ISREG                 0100000

#ifndef MAGIC
#define MAGIC                   "070707"

#endif                                                                  /*  __CPIO_H                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
