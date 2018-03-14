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
** 文   件   名: fatstat.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 09 月 26 日
**
** 描        述: FatFs 文件系统相关文件属性和 posix 标准类型的转换.

** BUG:
2009.03.14  修改 __fsInfoToStatfs() 和 __filInfoToStat() 的相关功能.
2009.04.17  扇区的大小不一定是 512 字节.
2009.06.08  修正了 __filInfoToStat() 中 st_blksize 的计算错误.
2010.01.09  加入新的 statfs 文件名最大长度信息.
2011.11.21  升级 fat 文件系统对应的字段.
2012.06.29  修正 __filInfoToStat() 中对 st_dev 和 st_ino 的赋值.
2012.12.08  修正 __filInfoToStat() 没有格式化时的错误.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_type.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_FATFS_EN > 0)
#include "ff.h"
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
INT     __fatFsGetError(FRESULT    fresError);
time_t  __fattimeToTime(UINT32  dwFatTime);
INT     __blockIoDevIoctl(INT  iIndex, INT  iCmd, LONG  lArg);
/*********************************************************************************************************
** 函数名称: __fsAttrToMode
** 功能描述: 将 FATFS fattrib 转换为 mode_t 格式
** 输　入  : ucAttr      fattrib
** 输　出  : mode_t
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
mode_t  __fsAttrToMode (BYTE  ucAttr)
{
    mode_t      mode = 0;
    
    if (ucAttr & AM_RDO) {
        mode |= (S_IRUSR | S_IRGRP | S_IROTH);
    
    } else {
        mode |= (S_IRUSR | S_IRGRP | S_IROTH) | (S_IWUSR | S_IWGRP | S_IWOTH);
    }
    
    if (ucAttr & AM_DIR) {
        mode |= S_IFDIR;
    
    } else {
        mode |= S_IFREG;
        mode |= S_IXUSR | S_IXGRP;                                      /*  owner gourp 拥有可执行权限  */
    }
    
    return  (mode);
}
/*********************************************************************************************************
** 函数名称: __fsModeToAttr
** 功能描述: 将 FATFS fattrib 转换为 mode_t 格式
** 输　入  : ucAttr      fattrib
** 输　出  : mode_t
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
BYTE  __fsModeToAttr (mode_t  iMode)
{
    BYTE  ucAttr = AM_ARC;

    if ((iMode & S_IRUSR) || (iMode & S_IRGRP) || (iMode & S_IROTH)) {
        /*
         *  can read
         */
    }
    
    if ((iMode & S_IWUSR) || (iMode & S_IWGRP) || (iMode & S_IWOTH)) {
        /*
         *  can write
         */
    } else {
        ucAttr |= AM_RDO;
    }
    
    if ((iMode & S_IXUSR) || (iMode & S_IXGRP) || (iMode & S_IXOTH)) {
        ucAttr |= AM_SYS;
    }
    
    return  (ucAttr);
}
/*********************************************************************************************************
** 函数名称: __filInfoToStat
** 功能描述: 将 FILINFO 结构体转换为 stat 结构
** 输　入  : filinfo     FILINFO 结构体
**           fatfs       文件系统结构
**           pstat       stat 结构
**           ino         inode
** 输　出  : fat time
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __filInfoToStat (FILINFO     *filinfo, 
                       FATFS       *fatfs,
                       struct stat *pstat, 
                       ino_t        ino)
{
    UINT32  dwCrtTime = (DWORD)((filinfo->fcdate << 16) | (filinfo->fctime));
    UINT32  dwWrtTime = (DWORD)((filinfo->fdate  << 16) | (filinfo->ftime));

    pstat->st_dev   = (dev_t)fatfs;
    pstat->st_ino   = ino;
    
    pstat->st_nlink = 1;
    pstat->st_uid   = 0;                                                /*  不支持                      */
    pstat->st_gid   = 0;                                                /*  不支持                      */
    pstat->st_rdev  = 1;                                                /*  不支持                      */
    pstat->st_size  = (off_t)filinfo->fsize;
    
    if ((fatfs->csize == 0) || (fatfs->ssize == 0)) {                   /*  文件系统格式有错误          */
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
    
    } else {
        pstat->st_blksize = (long)(fatfs->csize * fatfs->ssize);
        pstat->st_blocks  = (long)((filinfo->fsize % pstat->st_blksize) 
                          ? ((filinfo->fsize / pstat->st_blksize) + 1)
                          : (filinfo->fsize / pstat->st_blksize));
    }
                      
    /*
     *  st_atime, st_mtime, st_ctime 为 UTC 时间
     */
    pstat->st_atime = __fattimeToTime(dwWrtTime);                       /*  仅使用修改时间              */
    pstat->st_mtime = pstat->st_atime;
    pstat->st_ctime = __fattimeToTime(dwCrtTime);

    pstat->st_mode  = __fsAttrToMode(filinfo->fattrib);
}
/*********************************************************************************************************
** 函数名称: __fsInfoToStatfs
** 功能描述: 将 FATFS 结构体转换为 statfs 结构
** 输　入  : fatfs       文件系统结构
**           iFlag       文件系统 flags
**           pstatfs     statfs 结构
**           iDrv        驱动号
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT   __fsInfoToStatfs (FATFS         *fatfs,
                        INT            iFlag,
                        struct statfs *pstatfs, 
                        INT            iDrv)
{
    REGISTER ULONG      ulClusterSize;
             ULONG      ulError;
             FRESULT    fresError;
             DWORD      dwFree;
    
    fresError = f_getfree("", &dwFree, &fatfs);                         /*  获得空闲簇数                */
    if (fresError) {
        ulError = __fatFsGetError(fresError);
        _ErrorHandle(ulError);
        return  (PX_ERROR);
    }
    
    if (fatfs->csize == 0) {                                            /*  簇信息错误                  */
        _ErrorHandle(ERROR_IO_DEVICE_ERROR);
        return  (PX_ERROR);
    }
    ulClusterSize = fatfs->csize * (ULONG)fatfs->ssize;                 /*  获得簇大小                  */
    
    pstatfs->f_type   = MSDOS_SUPER_MAGIC;
    pstatfs->f_bsize  = (long)ulClusterSize;
    pstatfs->f_blocks = (long)(fatfs->n_fatent);
    pstatfs->f_bfree  = (long)dwFree;
    pstatfs->f_bavail = (long)dwFree;
    
    pstatfs->f_files  = 0;
    pstatfs->f_ffree  = 0;
    
    pstatfs->f_fsid.val[0] = (int32_t)fatfs->id;
    pstatfs->f_fsid.val[1] = 0;
    
    pstatfs->f_flag = ST_NOSUID;
    if ((iFlag & O_ACCMODE) == O_RDONLY) {
        pstatfs->f_flag |= ST_RDONLY;
    }
    pstatfs->f_namelen = _MAX_LFN;                                      /*  最长的文件名                */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MAX_VOLUMES          */
                                                                        /*  LW_CFG_FATFS_EN             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
