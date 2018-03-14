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
** 文   件   名: lib_statvfs.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 09 月 22 日
**
** 描        述: statvfs.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
#include "sys/statvfs.h"
/*********************************************************************************************************
** 函数名称: fstatvfs
** 功能描述: 获得文件系统信息
** 输　入  : iFd           文件描述符
**           pstatvfs      文件系统信息结构
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int fstatvfs (int  iFd, struct statvfs *pstatvfs)
{
    int iRet;
    struct statfs statfsBuf;

    if (!pstatvfs) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    iRet = fstatfs(iFd, &statfsBuf);
    if (iRet < 0) {
        return  (iRet);
    }
    
    pstatvfs->f_bsize   = statfsBuf.f_bsize;
    pstatvfs->f_frsize  = statfsBuf.f_bsize;
    pstatvfs->f_blocks  = statfsBuf.f_blocks;
    pstatvfs->f_bfree   = statfsBuf.f_bfree;
    pstatvfs->f_bavail  = statfsBuf.f_bavail;
    pstatvfs->f_files   = statfsBuf.f_files;
    pstatvfs->f_ffree   = statfsBuf.f_ffree;
    pstatvfs->f_favail  = 0;
#if LW_CFG_CPU_WORD_LENGHT == 64
    pstatvfs->f_fsid    = ((unsigned long)statfsBuf.f_fsid.val[0] << 32) | statfsBuf.f_fsid.val[1];
#else
    pstatvfs->f_fsid    = statfsBuf.f_fsid.val[0];
#endif
    pstatvfs->f_flag    = statfsBuf.f_flag;
    pstatvfs->f_namemax = statfsBuf.f_namelen;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: statvfs
** 功能描述: 文件控制
** 输　入  : pcVolume      文件系统
**           pstatvfs      文件系统信息结构
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int statvfs (const char *pcVolume, struct statvfs *pstatvfs)
{
    int iRet;
    struct statfs statfsBuf;

    if (!pstatvfs) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    iRet = statfs(pcVolume, &statfsBuf);
    if (iRet < 0) {
        return  (iRet);
    }
    
    pstatvfs->f_bsize   = statfsBuf.f_bsize;
    pstatvfs->f_frsize  = statfsBuf.f_bsize;
    pstatvfs->f_blocks  = statfsBuf.f_blocks;
    pstatvfs->f_bfree   = statfsBuf.f_bfree;
    pstatvfs->f_bavail  = statfsBuf.f_bavail;
    pstatvfs->f_files   = statfsBuf.f_files;
    pstatvfs->f_ffree   = statfsBuf.f_ffree;
    pstatvfs->f_favail  = 0;
#if LW_CFG_CPU_WORD_LENGHT == 64
    pstatvfs->f_fsid    = ((unsigned long)statfsBuf.f_fsid.val[0] << 32) | statfsBuf.f_fsid.val[1];
#else
    pstatvfs->f_fsid    = statfsBuf.f_fsid.val[0];
#endif
    pstatvfs->f_flag    = statfsBuf.f_flag;
    pstatvfs->f_namemax = statfsBuf.f_namelen;
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
