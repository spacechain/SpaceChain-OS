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
** 文   件   名: romFs.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 06 月 27 日
**
** 描        述: rom 文件系统 sylixos 接口. (一些高可靠性低成本的产品可以使用 rom fs 启动)

** BUG:
2012.07.09  修正注释.
2012.07.17  创建 romfs 文件系统失败时, 需要删除锁.
2012.08.16  支持 pread 操作.
2012.09.01  加入不可强制卸载卷的保护.
2012.12.14  __FAT_FILE_LOCK() 需要判断返回值, 这样更加安全.
2013.01.06  romfs 使用最新 NEW_1 设备驱动类型. 这样可以支持文件锁.
2014.05.24  文件系统加入 uid gid.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
#include "../SylixOS/fs/include/fs_fs.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0 && LW_CFG_ROMFS_EN > 0
#include "romFsLib.h"
/*********************************************************************************************************
  内部全局变量
*********************************************************************************************************/
static INT                              _G_iRomfsDrvNum = PX_ERROR;
/*********************************************************************************************************
  文件类型
*********************************************************************************************************/
#define __ROMFS_FILE_TYPE_NODE          0                               /*  open 打开文件               */
#define __ROMFS_FILE_TYPE_DIR           1                               /*  open 打开目录               */
#define __ROMFS_FILE_TYPE_DEV           2                               /*  open 打开设备               */
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define __ROMFS_FILE_LOCK(promfile)     API_SemaphoreMPend(promfile->ROMFIL_promfs->ROMFS_hVolLock, \
                                        LW_OPTION_WAIT_INFINITE)
#define __ROMFS_FILE_UNLOCK(promfile)   API_SemaphoreMPost(promfile->ROMFIL_promfs->ROMFS_hVolLock)

#define __ROMFS_VOL_LOCK(promfs)        API_SemaphoreMPend(promfs->ROMFS_hVolLock, \
                                        LW_OPTION_WAIT_INFINITE)
#define __ROMFS_VOL_UNLOCK(promfs)      API_SemaphoreMPost(promfs->ROMFS_hVolLock)
/*********************************************************************************************************
  内部函数
*********************************************************************************************************/
static LONG     __romFsOpen(PROM_VOLUME     promfs,
                            PCHAR           pcName,
                            INT             iFlags,
                            INT             iMode);
static INT      __romFsRemove(PROM_VOLUME   promfs,
                              PCHAR         pcName);
static INT      __romFsClose(PLW_FD_ENTRY   pfdentry);
static ssize_t  __romFsRead(PLW_FD_ENTRY    pfdentry,
                            PCHAR           pcBuffer,
                            size_t          stMaxBytes);
static ssize_t  __romFsPRead(PLW_FD_ENTRY    pfdentry,
                             PCHAR           pcBuffer,
                             size_t          stMaxBytes,
                             off_t           oftPos);
static ssize_t  __romFsWrite(PLW_FD_ENTRY  pfdentry,
                             PCHAR         pcBuffer,
                             size_t        stNBytes);
static ssize_t  __romFsPWrite(PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer,
                              size_t        stNBytes,
                              off_t         oftPos);
static INT      __romFsSeek(PLW_FD_ENTRY  pfdentry,
                            off_t         oftOffset);
static INT      __romFsWhere(PLW_FD_ENTRY pfdentry,
                             off_t       *poftPos);
static INT      __romFsStat(PLW_FD_ENTRY  pfdentry, 
                            struct stat  *pstat);
static INT      __romFsLStat(PROM_VOLUME   promfs,
                             PCHAR         pcName,
                             struct stat  *pstat);
static INT      __romFsIoctl(PLW_FD_ENTRY  pfdentry,
                             INT           iRequest,
                             LONG          lArg);
static ssize_t  __romFsReadlink(PROM_VOLUME   promfs,
                                PCHAR         pcName,
                                PCHAR         pcLinkDst,
                                size_t        stMaxSize);
/*********************************************************************************************************
** 函数名称: API_RomFsDrvInstall
** 功能描述: 安装 romfs 文件系统驱动程序
** 输　入  :
** 输　出  : < 0 表示失败
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_RomFsDrvInstall (VOID)
{
    struct file_operations     fileop;
    
    if (_G_iRomfsDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));

    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __romFsOpen;
    fileop.fo_release  = __romFsRemove;
    fileop.fo_open     = __romFsOpen;
    fileop.fo_close    = __romFsClose;
    fileop.fo_read     = __romFsRead;
    fileop.fo_read_ex  = __romFsPRead;
    fileop.fo_write    = __romFsWrite;
    fileop.fo_write_ex = __romFsPWrite;
    fileop.fo_lstat    = __romFsLStat;
    fileop.fo_ioctl    = __romFsIoctl;
    fileop.fo_readlink = __romFsReadlink;
    
    _G_iRomfsDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);     /*  使用 NEW_1 型设备驱动程序   */

    DRIVER_LICENSE(_G_iRomfsDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iRomfsDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iRomfsDrvNum, "romfs driver.");

    _DebugHandle(__LOGMESSAGE_LEVEL, "rom file system installed.\r\n");
    
    __fsRegister("romfs", API_RomFsDevCreate, LW_NULL);                 /*  注册文件系统                */

    return  ((_G_iRomfsDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** 函数名称: API_RomFsDevCreate
** 功能描述: 创建 romfs 文件系统设备.
** 输　入  : pcName            设备名(设备挂接的节点地址)
**           pblkd             romfs 物理设备.
** 输　出  : < 0 表示失败
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_RomFsDevCreate (PCHAR   pcName, PLW_BLK_DEV  pblkd)
{
    PROM_VOLUME     promfs;

    if (_G_iRomfsDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "romfs Driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    if ((pblkd == LW_NULL) || (pblkd->BLKD_pcName == LW_NULL)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "block device invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    if ((pcName == LW_NULL) || __STR_IS_ROOT(pcName)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "mount name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    promfs = (PROM_VOLUME)__SHEAP_ALLOC(sizeof(ROM_VOLUME));
    if (promfs == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(promfs, sizeof(ROM_VOLUME));                              /*  清空卷控制块                */
    
    promfs->ROMFS_bValid = LW_TRUE;
    
    promfs->ROMFS_hVolLock = API_SemaphoreMCreate("romvol_lock", LW_PRIO_DEF_CEILING,
                                             LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE | 
                                             LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                             LW_NULL);
    if (!promfs->ROMFS_hVolLock) {                                      /*  无法创建卷锁                */
        __SHEAP_FREE(promfs);
        return  (PX_ERROR);
    }
    promfs->ROMFS_pblkd = pblkd;                                        /*  记录物理设备                */
    
    promfs->ROMFS_ulSectorSize = pblkd->BLKD_ulBytesPerSector;
    if (promfs->ROMFS_ulSectorSize == 0) {
        if (pblkd->BLKD_pfuncBlkIoctl(pblkd, 
                                      LW_BLKD_GET_SECSIZE, 
                                      &promfs->ROMFS_ulSectorSize) < ERROR_NONE) {
            API_SemaphoreMDelete(&promfs->ROMFS_hVolLock);
            __SHEAP_FREE(promfs);
            return  (PX_ERROR);
        }
    }
    
    promfs->ROMFS_ulSectorNum = pblkd->BLKD_ulNSector;
    if (promfs->ROMFS_ulSectorNum == 0) {
        if (pblkd->BLKD_pfuncBlkIoctl(pblkd, 
                                      LW_BLKD_GET_SECNUM, 
                                      &promfs->ROMFS_ulSectorNum) < ERROR_NONE) {
            API_SemaphoreMDelete(&promfs->ROMFS_hVolLock);
            __SHEAP_FREE(promfs);
            return  (PX_ERROR);
        }
    }
    
    promfs->ROMFS_pcSector = (PCHAR)__SHEAP_ALLOC((size_t)promfs->ROMFS_ulSectorSize);
    if (promfs->ROMFS_pcSector == LW_NULL) {
        API_SemaphoreMDelete(&promfs->ROMFS_hVolLock);
        __SHEAP_FREE(promfs);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    promfs->ROMFS_ulCurSector = (ULONG)-1;                              /*  当前窗口无效                */
    
    if (__rfs_mount(promfs)) {
        API_SemaphoreMDelete(&promfs->ROMFS_hVolLock);
        __SHEAP_FREE(promfs->ROMFS_pcSector);
        __SHEAP_FREE(promfs);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "format unknown.\r\n");
        _ErrorHandle(ERROR_IO_VOLUME_ERROR);
        return  (PX_ERROR);
    }
    
    promfs->ROMFS_uid = getuid();
    promfs->ROMFS_gid = getgid();
    
    lib_time(&promfs->ROMFS_time);                                      /*  获得当前时间                */
    
    if (iosDevAddEx(&promfs->ROMFS_devhdrHdr, pcName, _G_iRomfsDrvNum, DT_DIR)
        != ERROR_NONE) {                                                /*  安装文件系统设备            */
        API_SemaphoreMDelete(&promfs->ROMFS_hVolLock);
        __SHEAP_FREE(promfs->ROMFS_pcSector);
        __SHEAP_FREE(promfs);
        return  (PX_ERROR);
    }
    
    __fsDiskLinkCounterAdd(pblkd);                                      /*  增加块设备链接              */

    _DebugFormat(__LOGMESSAGE_LEVEL, "target \"%s\" mount ok.\r\n", pcName);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_RomFsDevDelete
** 功能描述: 删除一个 romfs 文件系统设备, 例如: API_RomFsDevDelete("/mnt/rom0");
** 输　入  : pcName            文件系统设备名(物理设备挂接的节点地址)
** 输　出  : < 0 表示失败
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_RomFsDevDelete (PCHAR   pcName)
{
    if (API_IosDevMatchFull(pcName)) {                                  /*  如果是设备, 这里就卸载设备  */
        return  (unlink(pcName));
    
    } else {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __romFsOpen
** 功能描述: 打开或者创建文件
** 输　入  : promfs           romfs 文件系统
**           pcName           文件名
**           iFlags           方式
**           iMode            mode_t
** 输　出  : < 0 错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LONG __romFsOpen (PROM_VOLUME     promfs,
                         PCHAR           pcName,
                         INT             iFlags,
                         INT             iMode)
{
    INT             iError;
    PROM_FILE       promfile;
    PLW_FD_NODE     pfdnode;
    size_t          stSize;
    BOOL            bIsNew;
    struct stat    *pstat;
    
    INT             iFollowLinkType;
    PCHAR           pcTail, pcSymfile, pcPrefix;
    
    if (pcName == LW_NULL) {                                            /*  无文件名                    */
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        stSize   = sizeof(ROM_FILE) + lib_strlen(pcName);
        promfile = (PROM_FILE)__SHEAP_ALLOC(stSize);
        if (promfile == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        lib_bzero(promfile, stSize);
        lib_strcpy(promfile->ROMFIL_cName, pcName);                     /*  记录文件名                  */
        
        promfile->ROMFIL_promfs = promfs;
        
        if (__ROMFS_VOL_LOCK(promfs) != ERROR_NONE) {
            _ErrorHandle(ENXIO);
            return  (PX_ERROR);
        }
        if (__STR_IS_ROOT(promfile->ROMFIL_cName)) {
            promfile->ROMFIL_iFileType = __ROMFS_FILE_TYPE_DEV;
            goto    __file_open_ok;                                     /*  设备打开正常                */
        }
        
        if (promfs->ROMFS_bValid == LW_FALSE) {
            __ROMFS_VOL_UNLOCK(promfs);
            __SHEAP_FREE(promfile);
            _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
            return  (PX_ERROR);
        }
        
        iError = __rfs_open(promfs, pcName, &pcTail, &pcSymfile, &promfile->ROMFIL_romfsdnt);
        if (iError) {
            __ROMFS_VOL_UNLOCK(promfs);
            __SHEAP_FREE(promfile);
            if (iFlags & O_CREAT) {
                _ErrorHandle(EROFS);                                    /*  只读文件系统                */
            }
            return  (PX_ERROR);
        }
        
        /*
         *  首先处理符号链接文件情况
         */
        if (S_ISLNK(promfile->ROMFIL_romfsdnt.ROMFSDNT_stat.st_mode)) {
            pcSymfile--;                                                /*  从 / 开始                   */
            if (pcSymfile == pcName) {
                pcPrefix = LW_NULL;                                     /*  没有前缀                    */
            } else {
                pcPrefix = pcName;
                *pcSymfile = PX_EOS;
            }
            if (pcTail && lib_strlen(pcTail)) {
                iFollowLinkType = FOLLOW_LINK_TAIL;                     /*  连接目标内部文件            */
            } else {
                iFollowLinkType = FOLLOW_LINK_FILE;                     /*  链接文件本身                */
            }
        
            if (__rfs_path_build_link(promfs, &promfile->ROMFIL_romfsdnt, pcName, PATH_MAX + 1,
                                      pcPrefix, pcTail) == ERROR_NONE) {
                                                                        /*  构造链接目标                */
                __ROMFS_VOL_UNLOCK(promfs);
                __SHEAP_FREE(promfile);
                return  (iFollowLinkType);
            
            } else {                                                    /*  构造连接失败                */
                __ROMFS_VOL_UNLOCK(promfs);
                __SHEAP_FREE(promfile);
                return  (PX_ERROR);
            }
        
        } else if (S_ISDIR(promfile->ROMFIL_romfsdnt.ROMFSDNT_stat.st_mode)) {
            promfile->ROMFIL_iFileType = __ROMFS_FILE_TYPE_DIR;
        
        } else {
            promfile->ROMFIL_iFileType = __ROMFS_FILE_TYPE_NODE;
        }
        
__file_open_ok:
        pstat   = &promfile->ROMFIL_romfsdnt.ROMFSDNT_stat;
        pfdnode = API_IosFdNodeAdd(&promfs->ROMFS_plineFdNodeHeader,
                                    pstat->st_dev,
                                    (ino64_t)pstat->st_ino,
                                    iFlags,
                                    iMode,
                                    pstat->st_uid,
                                    pstat->st_gid,
                                    pstat->st_size,
                                    (PVOID)promfile,
                                    &bIsNew);                           /*  添加文件节点                */
        if (pfdnode == LW_NULL) {                                       /*  无法创建 fd_node 节点       */
            __ROMFS_VOL_UNLOCK(promfs);
            __SHEAP_FREE(promfile);
            return  (PX_ERROR);
        }
        
        LW_DEV_INC_USE_COUNT(&promfs->ROMFS_devhdrHdr);                 /*  更新计数器                  */
        
        __ROMFS_VOL_UNLOCK(promfs);
        
        if (bIsNew == LW_FALSE) {                                       /*  有重复打开                  */
            __SHEAP_FREE(promfile);
        }
        
        return  ((LONG)pfdnode);                                        /*  返回文件节点                */
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __romFsRemove
** 功能描述: romfs remove 操作
** 输　入  : promfs           卷设备
**           pcName           文件名
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __romFsRemove (PROM_VOLUME   promfs,
                           PCHAR         pcName)
{
    PLW_BLK_DEV    pblkd;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
    
    if (__STR_IS_ROOT(pcName)) {
        if (__ROMFS_VOL_LOCK(promfs) != ERROR_NONE) {
            _ErrorHandle(ENXIO);
            return  (PX_ERROR);
        }
        
        if (promfs->ROMFS_bValid == LW_FALSE) {
            __ROMFS_VOL_UNLOCK(promfs);
            return  (ERROR_NONE);                                       /*  正在被其他任务卸载          */
        }

__re_umount_vol:
        if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)promfs)) {               /*  检查是否有正在工作的文件    */
            if (!promfs->ROMFS_bForceDelete) {
                __ROMFS_VOL_UNLOCK(promfs);
                _ErrorHandle(EBUSY);
                return  (PX_ERROR);
            }
            
            promfs->ROMFS_bValid = LW_FALSE;
            
            __ROMFS_VOL_UNLOCK(promfs);
            
            _DebugHandle(__ERRORMESSAGE_LEVEL, "disk have open file.\r\n");
            iosDevFileAbnormal(&promfs->ROMFS_devhdrHdr);               /*  将所有相关文件设为异常模式  */
            
            __ROMFS_VOL_LOCK(promfs);
            goto    __re_umount_vol;
        
        } else {
            promfs->ROMFS_bValid = LW_FALSE;
        }
        
        pblkd = promfs->ROMFS_pblkd;
        if (pblkd) {
            __fsDiskLinkCounterDec(pblkd);                              /*  减少链接次数                */
        }

        iosDevDelete((LW_DEV_HDR *)promfs);                             /*  IO 系统移除设备             */
        API_SemaphoreMDelete(&promfs->ROMFS_hVolLock);
        
        __SHEAP_FREE(promfs->ROMFS_pcSector);
        __SHEAP_FREE(promfs);

        _DebugHandle(__LOGMESSAGE_LEVEL, "romfs unmount ok.\r\n");
        
        return  (ERROR_NONE);
    
    } else {
        PCHAR           pcTail = LW_NULL, pcSymfile, pcPrefix;
        ROMFS_DIRENT    romfsdnt;
    
        if (__ROMFS_VOL_LOCK(promfs) != ERROR_NONE) {
            _ErrorHandle(ENXIO);
            return  (PX_ERROR);
        }
        if (__rfs_open(promfs, pcName, &pcTail, &pcSymfile, &romfsdnt) == ERROR_NONE) {
            if (S_ISLNK(romfsdnt.ROMFSDNT_stat.st_mode)) {
                if (pcTail && lib_strlen(pcTail)) {                     /*  连接文件有后缀              */
                    pcSymfile--;                                        /*  从 / 开始                   */
                    if (pcSymfile == pcName) {
                        pcPrefix = LW_NULL;                             /*  没有前缀                    */
                    } else {
                        pcPrefix = pcName;
                        *pcSymfile = PX_EOS;
                    }
                    if (__rfs_path_build_link(promfs, &romfsdnt, pcName, PATH_MAX + 1,
                                              pcPrefix, pcTail) == ERROR_NONE) {
                        __ROMFS_VOL_UNLOCK(promfs);
                        return  (FOLLOW_LINK_TAIL);
                    }
                }
            }
        }
        __ROMFS_VOL_UNLOCK(promfs);
        
        _ErrorHandle(EROFS);
        return  (PX_ERROR);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __romFsClose
** 功能描述: romfs close 操作
** 输　入  : pfdentry         文件控制块
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __romFsClose (PLW_FD_ENTRY    pfdentry)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE     promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;
    PROM_VOLUME   promfs   = promfile->ROMFIL_promfs;
    BOOL          bFree    = LW_FALSE;

    if (promfile) {
        if (__ROMFS_VOL_LOCK(promfs) != ERROR_NONE) {
            _ErrorHandle(ENXIO);
            return  (PX_ERROR);
        }
        if (API_IosFdNodeDec(&promfs->ROMFS_plineFdNodeHeader,
                             pfdnode, LW_NULL) == 0) {
            bFree = LW_TRUE;
        }
        
        LW_DEV_DEC_USE_COUNT(&promfs->ROMFS_devhdrHdr);
        
        __ROMFS_VOL_UNLOCK(promfs);
        
        if (bFree) {
            __SHEAP_FREE(promfile);
        }
        
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __romFsRead
** 功能描述: romfs read 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __romFsRead (PLW_FD_ENTRY pfdentry,
                             PCHAR        pcBuffer,
                             size_t       stMaxBytes)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE     promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;
    ssize_t       sstRet;
    
    if (!pcBuffer || !stMaxBytes) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    if (promfile->ROMFIL_iFileType != __ROMFS_FILE_TYPE_NODE) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__ROMFS_FILE_LOCK(promfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    sstRet = __rfs_pread(promfile->ROMFIL_promfs, &promfile->ROMFIL_romfsdnt, 
                         pcBuffer, stMaxBytes, (UINT32)pfdentry->FDENTRY_oftPtr);
    if (sstRet > 0) {
        pfdentry->FDENTRY_oftPtr += (off_t)sstRet;                      /*  更新文件指针                */
    }
                         
    __ROMFS_FILE_UNLOCK(promfile);
    
    return  (sstRet);
}
/*********************************************************************************************************
** 函数名称: __romFsPRead
** 功能描述: romfs pread 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
**           oftPos           位置
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __romFsPRead (PLW_FD_ENTRY pfdentry,
                              PCHAR        pcBuffer,
                              size_t       stMaxBytes,
                              off_t        oftPos)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE     promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;
    ssize_t       sstRet;
    
    if (!pcBuffer || !stMaxBytes || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    if (promfile->ROMFIL_iFileType != __ROMFS_FILE_TYPE_NODE) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__ROMFS_FILE_LOCK(promfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    sstRet = __rfs_pread(promfile->ROMFIL_promfs, &promfile->ROMFIL_romfsdnt, 
                         pcBuffer, stMaxBytes, (UINT32)oftPos);
    __ROMFS_FILE_UNLOCK(promfile);
    
    return  (sstRet);
}
/*********************************************************************************************************
** 函数名称: __romFsWrite
** 功能描述: romfs write 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         缓冲区
**           stNBytes         需要写入的数据
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __romFsWrite (PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer,
                              size_t        stNBytes)
{
    _ErrorHandle(EROFS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __romFsPWrite
** 功能描述: romfs pwrite 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         缓冲区
**           stNBytes         需要写入的数据
**           oftPos           位置
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __romFsPWrite (PLW_FD_ENTRY  pfdentry,
                               PCHAR         pcBuffer,
                               size_t        stNBytes,
                               off_t         oftPos)
{
    _ErrorHandle(EROFS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __romFsNRead
** 功能描述: romFs nread 操作
** 输　入  : pfdentry         文件控制块
**           piNRead          剩余数据量
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __romFsNRead (PLW_FD_ENTRY  pfdentry, INT  *piNRead)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE     promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;

    if (piNRead == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (promfile->ROMFIL_iFileType != __ROMFS_FILE_TYPE_NODE) {
        return  (PX_ERROR);
    }
    
    if (__ROMFS_FILE_LOCK(promfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    *piNRead = (INT)(promfile->ROMFIL_romfsdnt.ROMFSDNT_stat.st_size - 
                     (UINT32)pfdentry->FDENTRY_oftPtr);
    __ROMFS_FILE_UNLOCK(promfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __romFsNRead64
** 功能描述: romFs nread 操作
** 输　入  : pfdentry         文件控制块
**           poftNRead        剩余数据量
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __romFsNRead64 (PLW_FD_ENTRY  pfdentry, off_t  *poftNRead)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE     promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;

    if (promfile->ROMFIL_iFileType != __ROMFS_FILE_TYPE_NODE) {
        return  (PX_ERROR);
    }
    
    if (__ROMFS_FILE_LOCK(promfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    *poftNRead = (off_t)(promfile->ROMFIL_romfsdnt.ROMFSDNT_stat.st_size - 
                         (UINT32)pfdentry->FDENTRY_oftPtr);
    __ROMFS_FILE_UNLOCK(promfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __romFsSeek
** 功能描述: romFs seek 操作
** 输　入  : pfdentry         文件控制块
**           oftOffset        偏移量
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __romFsSeek (PLW_FD_ENTRY  pfdentry,
                         off_t         oftOffset)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE     promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;

    if (promfile->ROMFIL_iFileType != __ROMFS_FILE_TYPE_NODE) {
        return  (PX_ERROR);
    }
    
    if (oftOffset > promfile->ROMFIL_romfsdnt.ROMFSDNT_stat.st_size) {
        _ErrorHandle(EROFS);
        return  (PX_ERROR);
    }
    
    if (__ROMFS_FILE_LOCK(promfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (promfile->ROMFIL_iFileType != __ROMFS_FILE_TYPE_NODE) {
        __ROMFS_FILE_UNLOCK(promfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    pfdentry->FDENTRY_oftPtr = oftOffset;
    
    __ROMFS_FILE_UNLOCK(promfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __romFsWhere
** 功能描述: romFs 获得文件当前读写指针位置 (使用参数作为返回值, 与 FIOWHERE 的要求稍有不同)
** 输　入  : pfdentry            文件控制块
**           poftPos             读写指针位置
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __romFsWhere (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    if (poftPos) {
        *poftPos = (off_t)pfdentry->FDENTRY_oftPtr;
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __romFsStatGet
** 功能描述: romFs stat 操作
** 输　入  : pfdentry         文件控制块
**           pstat            文件状态
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __romFsStat (PLW_FD_ENTRY  pfdentry, struct stat *pstat)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE     promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;
    PROM_VOLUME   promfs   = promfile->ROMFIL_promfs;

    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (promfile->ROMFIL_iFileType == __ROMFS_FILE_TYPE_DEV) {
        pstat->st_dev     = (dev_t)promfile->ROMFIL_promfs;
        pstat->st_ino     = (ino_t)0;                                   /*  绝不会与文件重复(根目录)    */
        pstat->st_mode    = (S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH | S_IFDIR);
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;
        pstat->st_gid     = 0;
        pstat->st_rdev    = 0;
        pstat->st_size    = 0;
        pstat->st_blksize = promfs->ROMFS_ulSectorSize;
        pstat->st_blocks  = 0;
        
        pstat->st_atime   = promfs->ROMFS_time;
        pstat->st_mtime   = promfs->ROMFS_time;
        pstat->st_ctime   = promfs->ROMFS_time;
        
        return  (ERROR_NONE);
        
    } else {
        *pstat = promfile->ROMFIL_romfsdnt.ROMFSDNT_stat;
        
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __romFsLStat
** 功能描述: romFs stat 操作
** 输　入  : promfs           romfs 文件系统
**           pcName           文件名
**           pstat            文件状态
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __romFsLStat (PROM_VOLUME     promfs, PCHAR  pcName, struct stat *pstat)
{
    ROMFS_DIRENT    romfsdnt;
    PCHAR           pcTail, pcSymfile;
    INT             iError;

    if (!pcName || !pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__ROMFS_VOL_LOCK(promfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    iError = __rfs_open(promfs, pcName, &pcTail, &pcSymfile, &romfsdnt);
    if (iError) {
        __ROMFS_VOL_UNLOCK(promfs);
        
        if (__STR_IS_ROOT(pcName)) {
            pstat->st_dev     = (dev_t)promfs;
            pstat->st_ino     = (ino_t)0;                               /*  绝不会与文件重复(根目录)    */
            pstat->st_mode    = (S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH | S_IFDIR);
            pstat->st_nlink   = 1;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 0;
            pstat->st_size    = 0;
            pstat->st_blksize = promfs->ROMFS_ulSectorSize;
            pstat->st_blocks  = 0;
            
            pstat->st_atime   = promfs->ROMFS_time;
            pstat->st_mtime   = promfs->ROMFS_time;
            pstat->st_ctime   = promfs->ROMFS_time;
            
            return  (ERROR_NONE);
        
        } else {
            return  (PX_ERROR);
        }
    }
    __ROMFS_VOL_UNLOCK(promfs);
    
    *pstat = romfsdnt.ROMFSDNT_stat;
        
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __romFsStatfs
** 功能描述: romFs statfs 操作
** 输　入  : pfdentry         文件控制块
**           pstatfs          文件系统状态
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __romFsStatfs (PLW_FD_ENTRY  pfdentry, struct statfs *pstatfs)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE     promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;

    if (!pstatfs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pstatfs->f_type  = ROMFS_MAGIC;                                     /* type of info, zero for now   */
    pstatfs->f_bsize = promfile->ROMFIL_promfs->ROMFS_ulSectorSize;
    
    pstatfs->f_blocks = (promfile->ROMFIL_promfs->ROMFS_uiTotalSize 
                      / promfile->ROMFIL_promfs->ROMFS_ulSectorSize)
                      + 1;
    pstatfs->f_bfree  = 0;
    pstatfs->f_bavail = 1;
    
    pstatfs->f_files   = 0;                                             /* total file nodes in fs       */
    pstatfs->f_ffree   = 0;                                             /* free file nodes in fs        */
    
#if LW_CFG_CPU_WORD_LENGHT == 64
    pstatfs->f_fsid.val[0] = (int32_t)((addr_t)promfile->ROMFIL_promfs >> 32);
    pstatfs->f_fsid.val[1] = (int32_t)((addr_t)promfile->ROMFIL_promfs & 0xffffffff);
#else
    pstatfs->f_fsid.val[0] = (int32_t)promfile->ROMFIL_promfs;
    pstatfs->f_fsid.val[1] = 0;
#endif
    
    pstatfs->f_flag    = ST_RDONLY;
    pstatfs->f_namelen = PATH_MAX;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __romFsReadDir
** 功能描述: romFs 获得指定目录信息
** 输　入  : pfdentry            文件控制块
**           dir                 目录结构
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __romFsReadDir (PLW_FD_ENTRY  pfdentry, DIR  *dir)
{
    PLW_FD_NODE     pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE       promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;
    ROMFS_DIRENT    romfsdnt;
    INT             iError;

    if (!dir) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (promfile->ROMFIL_iFileType == __ROMFS_FILE_TYPE_NODE) {
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }
    
    if (dir->dir_pos == 0) {
        if (promfile->ROMFIL_iFileType == __ROMFS_FILE_TYPE_DEV) {
            promfile->ROMFIL_ulCookieDir = promfile->ROMFIL_promfs->ROMFS_uiRootAddr;
        
        } else if (promfile->ROMFIL_iFileType == __ROMFS_FILE_TYPE_DIR) {
            if (promfile->ROMFIL_romfsdnt.ROMFSDNT_uiSpec == 
                promfile->ROMFIL_romfsdnt.ROMFSDNT_uiMe) {
                promfile->ROMFIL_ulCookieDir = 0;                       /*  此目录下没有文件            */
            
            } else {
                promfile->ROMFIL_ulCookieDir = promfile->ROMFIL_romfsdnt.ROMFSDNT_uiSpec;
            }
        
        } else {
            _ErrorHandle(ENOTDIR);
            return  (PX_ERROR);
        }
    }

__get_next:
    if (promfile->ROMFIL_ulCookieDir == 0) {                            /*  遍历结束                    */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    if (__ROMFS_FILE_LOCK(promfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    if (__rfs_getfile(promfile->ROMFIL_promfs, promfile->ROMFIL_ulCookieDir, &romfsdnt)) {
        __ROMFS_FILE_UNLOCK(promfile);
        return  (PX_ERROR);
    }
    __ROMFS_FILE_UNLOCK(promfile);
    
    if ((lib_strcmp(romfsdnt.ROMFSDNT_cName, ".") == 0) || 
        (lib_strcmp(romfsdnt.ROMFSDNT_cName, "..") == 0)) {             /*  忽略 . 和 ..                */
        promfile->ROMFIL_ulCookieDir = romfsdnt.ROMFSDNT_uiNext;        /*  记录下一个位置              */
        goto    __get_next;
    }
    
    lib_strcpy(dir->dir_dirent.d_name, romfsdnt.ROMFSDNT_cName);
    dir->dir_dirent.d_shortname[0] = PX_EOS;                            /*  不支持短文件名              */
    dir->dir_dirent.d_type = IFTODT(romfsdnt.ROMFSDNT_stat.st_mode);
    dir->dir_pos++;
    iError = ERROR_NONE;
    
    promfile->ROMFIL_ulCookieDir = romfsdnt.ROMFSDNT_uiNext;            /*  记录下一个位置              */
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __romFsReadlink
** 功能描述: romFs 读取符号链接文件内容
** 输　入  : promfs              romfs 文件系统
**           pcName              链接原始文件名
**           pcLinkDst           链接目标文件名
**           stMaxSize           缓冲大小
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t __romFsReadlink (PROM_VOLUME   promfs,
                                PCHAR         pcName,
                                PCHAR         pcLinkDst,
                                size_t        stMaxSize)
{
    ROMFS_DIRENT    romfsdnt;
    PCHAR           pcTail, pcSymfile;
    INT             iError;
    ssize_t         sstRet;
    
    if (__ROMFS_VOL_LOCK(promfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    iError = __rfs_open(promfs, pcName, &pcTail, &pcSymfile, &romfsdnt);
    if (iError) {
        __ROMFS_VOL_UNLOCK(promfs);
        return  (PX_ERROR);
    }
    
    if (!pcTail || (*pcTail == PX_EOS)) {                               /*  存在文件                    */
        if (!S_ISLNK(romfsdnt.ROMFSDNT_stat.st_mode)) {
            __ROMFS_VOL_UNLOCK(promfs);
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        
        sstRet = __rfs_readlink(promfs, &romfsdnt, pcLinkDst, stMaxSize);
        __ROMFS_VOL_UNLOCK(promfs);
        
        return  (sstRet);
    }
    __ROMFS_VOL_UNLOCK(promfs);
    
    _ErrorHandle(EINVAL);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __romFsIoctl
** 功能描述: romFs ioctl 操作
** 输　入  : pfdentry           文件控制块
**           request,           命令
**           arg                命令参数
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __romFsIoctl (PLW_FD_ENTRY  pfdentry,
                          INT           iRequest,
                          LONG          lArg)
{
    PLW_FD_NODE     pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE       promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;
    off_t           oftTemp;
    INT             iError;
    
    switch (iRequest) {

    case FIOCONTIG:
    case FIOTRUNC:
    case FIOLABELSET:
    case FIOATTRIBSET:
    case FIOSQUEEZE:
        _ErrorHandle(EROFS);
        return (PX_ERROR);
    }
    
    switch (iRequest) {

    case FIODISKINIT:                                                   /*  磁盘初始化                  */
        return  (ERROR_NONE);
    
    /*
     *  FIOSEEK, FIOWHERE is 64 bit operate.
     */
    case FIOSEEK:                                                       /*  文件重定位                  */
        oftTemp = *(off_t *)lArg;
        return  (__romFsSeek(pfdentry, oftTemp));

    case FIOWHERE:                                                      /*  获得文件当前读写指针        */
        iError = __romFsWhere(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }

    case FIONREAD:                                                      /*  获得文件剩余字节数          */
        return  (__romFsNRead(pfdentry, (INT *)lArg));
        
    case FIONREAD64:                                                    /*  获得文件剩余字节数          */
        iError = __romFsNRead64(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }
        
    case FIOFSTATGET:                                                   /*  获得文件状态                */
        return  (__romFsStat(pfdentry, (struct stat *)lArg));

    case FIOFSTATFSGET:                                                 /*  获得文件系统状态            */
        return  (__romFsStatfs(pfdentry, (struct statfs *)lArg));

    case FIOREADDIR:                                                    /*  获取一个目录信息            */
        return  (__romFsReadDir(pfdentry, (DIR *)lArg));
        
    case FIOSETFL:                                                      /*  设置新的 flag               */
        if ((INT)lArg & O_NONBLOCK) {
            pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);

    case FIOFSTYPE:                                                     /*  获得文件系统类型            */
        *(PCHAR *)lArg = "ROM FileSystem";
        return  (ERROR_NONE);
    
    case FIOGETFORCEDEL:                                                /*  强制卸载设备是否被允许      */
        *(BOOL *)lArg = promfile->ROMFIL_promfs->ROMFS_bForceDelete;
        return  (ERROR_NONE);
    
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_ROMFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
