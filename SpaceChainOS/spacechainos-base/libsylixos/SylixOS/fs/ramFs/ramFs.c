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
** 文   件   名: ramFs.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 05 月 24 日
**
** 描        述: 内存文件系统.
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
#if LW_CFG_MAX_VOLUMES > 0 && LW_CFG_RAMFS_EN > 0
#include "ramFsLib.h"
/*********************************************************************************************************
  内部全局变量
*********************************************************************************************************/
static INT                              _G_iRamfsDrvNum = PX_ERROR;
/*********************************************************************************************************
  文件类型
*********************************************************************************************************/
#define __RAMFS_FILE_TYPE_NODE          0                               /*  open 打开文件               */
#define __RAMFS_FILE_TYPE_DIR           1                               /*  open 打开目录               */
#define __RAMFS_FILE_TYPE_DEV           2                               /*  open 打开设备               */
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define __RAMFS_FILE_LOCK(pramn)        API_SemaphoreMPend(pramn->RAMN_pramfs->RAMFS_hVolLock, \
                                        LW_OPTION_WAIT_INFINITE)
#define __RAMFS_FILE_UNLOCK(pramn)      API_SemaphoreMPost(pramn->RAMN_pramfs->RAMFS_hVolLock)

#define __RAMFS_VOL_LOCK(pramfs)        API_SemaphoreMPend(pramfs->RAMFS_hVolLock, \
                                        LW_OPTION_WAIT_INFINITE)
#define __RAMFS_VOL_UNLOCK(pramfs)      API_SemaphoreMPost(pramfs->RAMFS_hVolLock)
/*********************************************************************************************************
  内部函数
*********************************************************************************************************/
static LONG     __ramFsOpen(PRAM_VOLUME     pramfs,
                            PCHAR           pcName,
                            INT             iFlags,
                            INT             iMode);
static INT      __ramFsRemove(PRAM_VOLUME   pramfs,
                              PCHAR         pcName);
static INT      __ramFsClose(PLW_FD_ENTRY   pfdentry);
static ssize_t  __ramFsRead(PLW_FD_ENTRY    pfdentry,
                            PCHAR           pcBuffer,
                            size_t          stMaxBytes);
static ssize_t  __ramFsPRead(PLW_FD_ENTRY    pfdentry,
                             PCHAR           pcBuffer,
                             size_t          stMaxBytes,
                             off_t           oftPos);
static ssize_t  __ramFsWrite(PLW_FD_ENTRY  pfdentry,
                             PCHAR         pcBuffer,
                             size_t        stNBytes);
static ssize_t  __ramFsPWrite(PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer,
                              size_t        stNBytes,
                              off_t         oftPos);
static INT      __ramFsSeek(PLW_FD_ENTRY  pfdentry,
                            off_t         oftOffset);
static INT      __ramFsWhere(PLW_FD_ENTRY pfdentry,
                             off_t       *poftPos);
static INT      __ramFsStat(PLW_FD_ENTRY  pfdentry, 
                            struct stat  *pstat);
static INT      __ramFsLStat(PRAM_VOLUME   pramfs,
                             PCHAR         pcName,
                             struct stat  *pstat);
static INT      __ramFsIoctl(PLW_FD_ENTRY  pfdentry,
                             INT           iRequest,
                             LONG          lArg);
static INT      __ramFsSymlink(PRAM_VOLUME   pramfs,
                               PCHAR         pcName,
                               CPCHAR        pcLinkDst);
static ssize_t  __ramFsReadlink(PRAM_VOLUME   pramfs,
                                PCHAR         pcName,
                                PCHAR         pcLinkDst,
                                size_t        stMaxSize);
/*********************************************************************************************************
** 函数名称: API_RamFsDrvInstall
** 功能描述: 安装 ramfs 文件系统驱动程序
** 输　入  :
** 输　出  : < 0 表示失败
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_RamFsDrvInstall (VOID)
{
    struct file_operations     fileop;
    
    if (_G_iRamfsDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));

    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __ramFsOpen;
    fileop.fo_release  = __ramFsRemove;
    fileop.fo_open     = __ramFsOpen;
    fileop.fo_close    = __ramFsClose;
    fileop.fo_read     = __ramFsRead;
    fileop.fo_read_ex  = __ramFsPRead;
    fileop.fo_write    = __ramFsWrite;
    fileop.fo_write_ex = __ramFsPWrite;
    fileop.fo_lstat    = __ramFsLStat;
    fileop.fo_ioctl    = __ramFsIoctl;
    fileop.fo_symlink  = __ramFsSymlink;
    fileop.fo_readlink = __ramFsReadlink;
    
    _G_iRamfsDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);     /*  使用 NEW_1 型设备驱动程序   */

    DRIVER_LICENSE(_G_iRamfsDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iRamfsDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iRamfsDrvNum, "ramfs driver.");

    _DebugHandle(__LOGMESSAGE_LEVEL, "ram file system installed.\r\n");
                                     
    __fsRegister("ramfs", API_RamFsDevCreate, LW_NULL);                 /*  注册文件系统                */

    return  ((_G_iRamfsDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** 函数名称: API_RamFsDevCreate
** 功能描述: 创建 ramfs 文件系统设备.
** 输　入  : pcName            设备名(设备挂接的节点地址)
**           pblkd             使用 pblkd->BLKD_pcName 作为 最大大小 标示.
** 输　出  : < 0 表示失败
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_RamFsDevCreate (PCHAR   pcName, PLW_BLK_DEV  pblkd)
{
    PRAM_VOLUME     pramfs;
    size_t          stMax;

    if (_G_iRamfsDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ramfs Driver invalidate.\r\n");
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
    if (sscanf(pblkd->BLKD_pcName, "%zu", &stMax) != 1) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "max size invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pramfs = (PRAM_VOLUME)__SHEAP_ALLOC(sizeof(RAM_VOLUME));
    if (pramfs == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pramfs, sizeof(RAM_VOLUME));                              /*  清空卷控制块                */
    
    pramfs->RAMFS_bValid = LW_TRUE;
    
    pramfs->RAMFS_hVolLock = API_SemaphoreMCreate("ramvol_lock", LW_PRIO_DEF_CEILING,
                                             LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE | 
                                             LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                             LW_NULL);
    if (!pramfs->RAMFS_hVolLock) {                                      /*  无法创建卷锁                */
        __SHEAP_FREE(pramfs);
        return  (PX_ERROR);
    }
    
    pramfs->RAMFS_mode     = S_IFDIR | DEFAULT_DIR_PERM;
    pramfs->RAMFS_uid      = getuid();
    pramfs->RAMFS_gid      = getgid();
    pramfs->RAMFS_time     = lib_time(LW_NULL);
    pramfs->RAMFS_ulCurBlk = 0ul;
    
    if (stMax == 0) {
#if LW_CFG_CPU_WORD_LENGHT == 32
        pramfs->RAMFS_ulMaxBlk = (__ARCH_ULONG_MAX / __RAM_BSIZE);
#else
        pramfs->RAMFS_ulMaxBlk = ((ULONG)(128ul * LW_CFG_GB_SIZE) / __RAM_BSIZE);
#endif
    } else {
        pramfs->RAMFS_ulMaxBlk = (ULONG)(stMax / __RAM_BSIZE);
    }

    __ram_mount(pramfs);
    
    if (iosDevAddEx(&pramfs->RAMFS_devhdrHdr, pcName, _G_iRamfsDrvNum, DT_DIR)
        != ERROR_NONE) {                                                /*  安装文件系统设备            */
        API_SemaphoreMDelete(&pramfs->RAMFS_hVolLock);
        __SHEAP_FREE(pramfs);
        return  (PX_ERROR);
    }
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "target \"%s\" mount ok.\r\n", pcName);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_RamFsDevDelete
** 功能描述: 删除一个 ramfs 文件系统设备, 例如: API_RamFsDevDelete("/mnt/ram0");
** 输　入  : pcName            文件系统设备名(物理设备挂接的节点地址)
** 输　出  : < 0 表示失败
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_RamFsDevDelete (PCHAR   pcName)
{
    if (API_IosDevMatchFull(pcName)) {                                  /*  如果是设备, 这里就卸载设备  */
        return  (unlink(pcName));
    
    } else {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __ramFsOpen
** 功能描述: 打开或者创建文件
** 输　入  : pramfs           ramfs 文件系统
**           pcName           文件名
**           iFlags           方式
**           iMode            mode_t
** 输　出  : < 0 错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LONG __ramFsOpen (PRAM_VOLUME     pramfs,
                         PCHAR           pcName,
                         INT             iFlags,
                         INT             iMode)
{
    PLW_FD_NODE pfdnode;
    PRAM_NODE   pramn;
    PRAM_NODE   pramnFather;
    BOOL        bRoot;
    BOOL        bLast;
    PCHAR       pcTail;
    BOOL        bIsNew;
    BOOL        bCreate = LW_FALSE;
    struct stat statGet;
    
    if (pcName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    if (iFlags & O_CREAT) {                                             /*  创建操作                    */
        if (__fsCheckFileName(pcName)) {
            _ErrorHandle(ENOENT);
            return  (PX_ERROR);
        }
        if (S_ISFIFO(iMode) || 
            S_ISBLK(iMode)  ||
            S_ISCHR(iMode)) {
            _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);                    /*  不支持以上这些格式          */
            return  (PX_ERROR);
        }
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    pramn = __ram_open(pramfs, pcName, &pramnFather, &bRoot, &bLast, &pcTail);
    if (pramn) {
        if (!S_ISLNK(pramn->RAMN_mode)) {
            if ((iFlags & O_CREAT) && (iFlags & O_EXCL)) {              /*  排他创建文件                */
                __RAMFS_VOL_UNLOCK(pramfs);
                _ErrorHandle(EEXIST);                                   /*  已经存在文件                */
                return  (PX_ERROR);
            
            } else {
                goto    __file_open_ok;
            }
        }
    
    } else if ((iFlags & O_CREAT) && bLast) {                           /*  创建节点                    */
        pramn = __ram_maken(pramfs, pcName, pramnFather, iMode, LW_NULL);
        if (pramn) {
            bCreate = LW_TRUE;
            goto    __file_open_ok;
        
        } else {
            return  (PX_ERROR);
        }
    }
    
    if (pramn) {                                                        /*  符号链接处理                */
        INT     iError;
        INT     iFollowLinkType;
        PCHAR   pcSymfile = pcTail - lib_strlen(pramn->RAMN_pcName) - 1;
        PCHAR   pcPrefix;
        
        if (*pcSymfile != PX_DIVIDER) {
            pcSymfile--;
        }
        if (pcSymfile == pcName) {
            pcPrefix = LW_NULL;                                         /*  没有前缀                    */
        } else {
            pcPrefix = pcName;
            *pcSymfile = PX_EOS;
        }
        if (pcTail && lib_strlen(pcTail)) {
            iFollowLinkType = FOLLOW_LINK_TAIL;                         /*  连接目标内部文件            */
        } else {
            iFollowLinkType = FOLLOW_LINK_FILE;                         /*  链接文件本身                */
        }
        
        iError = _PathBuildLink(pcName, MAX_FILENAME_LENGTH, 
                                LW_NULL, pcPrefix, pramn->RAMN_pcLink, pcTail);
        if (iError) {
            __RAMFS_VOL_UNLOCK(pramfs);
            return  (PX_ERROR);                                         /*  无法建立被链接目标目录      */
        } else {
            __RAMFS_VOL_UNLOCK(pramfs);
            return  (iFollowLinkType);
        }
    
    } else if (bRoot == LW_FALSE) {                                     /*  不是打开根目录              */
        __RAMFS_VOL_UNLOCK(pramfs);
        _ErrorHandle(ENOENT);                                           /*  没有找到文件                */
        return  (PX_ERROR);
    }
    
__file_open_ok:
    __ram_stat(pramn, pramfs, &statGet);
    pfdnode = API_IosFdNodeAdd(&pramfs->RAMFS_plineFdNodeHeader,
                               statGet.st_dev,
                               (ino64_t)statGet.st_ino,
                               iFlags,
                               iMode,
                               statGet.st_uid,
                               statGet.st_gid,
                               statGet.st_size,
                               (PVOID)pramn,
                               &bIsNew);                                /*  添加文件节点                */
    if (pfdnode == LW_NULL) {                                           /*  无法创建 fd_node 节点       */
        __RAMFS_VOL_UNLOCK(pramfs);
        if (bCreate) {
            __ram_unlink(pramn);                                        /*  删除新建的节点              */
        }
        return  (PX_ERROR);
    }
    
    pfdnode->FDNODE_pvFsExtern = (PVOID)pramfs;                         /*  记录文件系统信息            */
    
    if ((iFlags & O_TRUNC) && ((iFlags & O_ACCMODE) != O_RDONLY)) {     /*  需要截断                    */
        if (pramn) {
            __ram_truncate(pramn, 0);
            pfdnode->FDNODE_oftSize = 0;
        }
    }
    
    LW_DEV_INC_USE_COUNT(&pramfs->RAMFS_devhdrHdr);                     /*  更新计数器                  */
    
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  ((LONG)pfdnode);                                            /*  返回文件节点                */
}
/*********************************************************************************************************
** 函数名称: __ramFsRemove
** 功能描述: ramfs remove 操作
** 输　入  : pramfs           卷设备
**           pcName           文件名
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ramFsRemove (PRAM_VOLUME   pramfs,
                           PCHAR         pcName)
{
    PRAM_NODE  pramn;
    BOOL       bRoot;
    PCHAR      pcTail;
    INT        iError;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
        
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    pramn = __ram_open(pramfs, pcName, LW_NULL, &bRoot, LW_NULL, &pcTail);
    if (pramn) {
        if (S_ISLNK(pramn->RAMN_mode)) {                                /*  符号链接                    */
            size_t  stLenTail = 0;
            if (pcTail) {
                stLenTail = lib_strlen(pcTail);                         /*  确定 tail 长度              */
            }
            if (stLenTail) {                                            /*  指向其他文件                */
                PCHAR   pcSymfile = pcTail - lib_strlen(pramn->RAMN_pcName) - 1;
                PCHAR   pcPrefix;
                
                if (*pcSymfile != PX_DIVIDER) {
                    pcSymfile--;
                }
                if (pcSymfile == pcName) {
                    pcPrefix = LW_NULL;                                 /*  没有前缀                    */
                } else {
                    pcPrefix = pcName;
                    *pcSymfile = PX_EOS;
                }
                
                if (_PathBuildLink(pcName, MAX_FILENAME_LENGTH, 
                                   LW_NULL, pcPrefix, pramn->RAMN_pcLink, pcTail) < ERROR_NONE) {
                    __RAMFS_VOL_UNLOCK(pramfs);
                    return  (PX_ERROR);                                 /*  无法建立被链接目标目录      */
                } else {
                    __RAMFS_VOL_UNLOCK(pramfs);
                    return  (FOLLOW_LINK_TAIL);
                }
            }
        }
        
        iError = __ram_unlink(pramn);
        __RAMFS_VOL_UNLOCK(pramfs);
        return  (iError);
            
    } else if (bRoot) {                                                 /*  删除 ramfs 文件系统         */
        if (pramfs->RAMFS_bValid == LW_FALSE) {
            __RAMFS_VOL_UNLOCK(pramfs);
            return  (ERROR_NONE);                                       /*  正在被其他任务卸载          */
        }
        
__re_umount_vol:
        if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)pramfs)) {
            if (!pramfs->RAMFS_bForceDelete) {
                __RAMFS_VOL_UNLOCK(pramfs);
                _ErrorHandle(EBUSY);
                return  (PX_ERROR);
            }
            
            pramfs->RAMFS_bValid = LW_FALSE;
            
            __RAMFS_VOL_UNLOCK(pramfs);
            
            _DebugHandle(__ERRORMESSAGE_LEVEL, "disk have open file.\r\n");
            iosDevFileAbnormal(&pramfs->RAMFS_devhdrHdr);               /*  将所有相关文件设为异常模式  */
            
            __RAMFS_VOL_LOCK(pramfs);
            goto    __re_umount_vol;
        
        } else {
            pramfs->RAMFS_bValid = LW_FALSE;
        }
        
        iosDevDelete((LW_DEV_HDR *)pramfs);                             /*  IO 系统移除设备             */
        API_SemaphoreMDelete(&pramfs->RAMFS_hVolLock);
        
        __ram_unmount(pramfs);                                          /*  释放所有文件内容            */
        __SHEAP_FREE(pramfs);
        
        _DebugHandle(__LOGMESSAGE_LEVEL, "romfs unmount ok.\r\n");
        
        return  (ERROR_NONE);
        
    } else {
        __RAMFS_VOL_UNLOCK(pramfs);
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __ramFsClose
** 功能描述: ramfs close 操作
** 输　入  : pfdentry         文件控制块
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ramFsClose (PLW_FD_ENTRY    pfdentry)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    BOOL          bRemove = LW_FALSE;
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (API_IosFdNodeDec(&pramfs->RAMFS_plineFdNodeHeader, 
                         pfdnode, &bRemove) == 0) {
        if (pramn) {
            __ram_close(pramn, pfdentry->FDENTRY_iFlag);
        }
    }
    
    LW_DEV_DEC_USE_COUNT(&pramfs->RAMFS_devhdrHdr);
    
    if (bRemove && pramn) {
        __ram_unlink(pramn);
    }
        
    __RAMFS_VOL_UNLOCK(pramfs);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ramFsRead
** 功能描述: ramfs read 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __ramFsRead (PLW_FD_ENTRY pfdentry,
                             PCHAR        pcBuffer,
                             size_t       stMaxBytes)
{
    PLW_FD_NODE   pfdnode    = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn      = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    ssize_t       sstReadNum = PX_ERROR;
    
    if (!pcBuffer || !stMaxBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pramn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_FILE_LOCK(pramn) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pramn->RAMN_mode)) {
        __RAMFS_FILE_UNLOCK(pramn);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    sstReadNum = __ram_read(pramn, pcBuffer, stMaxBytes, (size_t)pfdentry->FDENTRY_oftPtr);
    if (sstReadNum > 0) {
        pfdentry->FDENTRY_oftPtr += (off_t)sstReadNum;                  /*  更新文件指针                */
    }
    
    __RAMFS_FILE_UNLOCK(pramn);
    
    return  (sstReadNum);
}
/*********************************************************************************************************
** 函数名称: __ramFsPRead
** 功能描述: ramfs pread 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
**           oftPos           位置
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __ramFsPRead (PLW_FD_ENTRY pfdentry,
                              PCHAR        pcBuffer,
                              size_t       stMaxBytes,
                              off_t        oftPos)
{
    PLW_FD_NODE   pfdnode    = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn      = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    ssize_t       sstReadNum = PX_ERROR;
    
    if (!pcBuffer || !stMaxBytes || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pramn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_FILE_LOCK(pramn) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pramn->RAMN_mode)) {
        __RAMFS_FILE_UNLOCK(pramn);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    sstReadNum = __ram_read(pramn, pcBuffer, stMaxBytes, (size_t)oftPos);
    
    __RAMFS_FILE_UNLOCK(pramn);
    
    return  (sstReadNum);
}
/*********************************************************************************************************
** 函数名称: __ramFsWrite
** 功能描述: ramfs write 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         缓冲区
**           stNBytes         需要写入的数据
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __ramFsWrite (PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer,
                              size_t        stNBytes)
{
    PLW_FD_NODE   pfdnode     = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn       = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    ssize_t       sstWriteNum = PX_ERROR;
    
    if (!pcBuffer || !stNBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pramn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_FILE_LOCK(pramn) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pramn->RAMN_mode)) {
        __RAMFS_FILE_UNLOCK(pramn);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (pfdentry->FDENTRY_iFlag & O_APPEND) {                           /*  追加模式                    */
        pfdentry->FDENTRY_oftPtr = pfdnode->FDNODE_oftSize;             /*  移动读写指针到末尾          */
    }
    
    sstWriteNum = __ram_write(pramn, pcBuffer, stNBytes, (size_t)pfdentry->FDENTRY_oftPtr);
    if (sstWriteNum > 0) {
        pfdentry->FDENTRY_oftPtr += (off_t)sstWriteNum;                 /*  更新文件指针                */
        pfdnode->FDNODE_oftSize   = (off_t)pramn->RAMN_stSize;
    }
    
    __RAMFS_FILE_UNLOCK(pramn);
    
    return  (sstWriteNum);
}
/*********************************************************************************************************
** 函数名称: __ramFsPWrite
** 功能描述: ramfs pwrite 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         缓冲区
**           stNBytes         需要写入的数据
**           oftPos           位置
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __ramFsPWrite (PLW_FD_ENTRY  pfdentry,
                               PCHAR         pcBuffer,
                               size_t        stNBytes,
                               off_t         oftPos)
{
    PLW_FD_NODE   pfdnode     = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn       = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    ssize_t       sstWriteNum = PX_ERROR;
    
    if (!pcBuffer || !stNBytes || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pramn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_FILE_LOCK(pramn) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pramn->RAMN_mode)) {
        __RAMFS_FILE_UNLOCK(pramn);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    sstWriteNum = __ram_write(pramn, pcBuffer, stNBytes, (size_t)oftPos);
    if (sstWriteNum > 0) {
        pfdnode->FDNODE_oftSize = (off_t)pramn->RAMN_stSize;
    }
    
    __RAMFS_FILE_UNLOCK(pramn);
    
    return  (sstWriteNum);
}
/*********************************************************************************************************
** 函数名称: __ramFsNRead
** 功能描述: ramFs nread 操作
** 输　入  : pfdentry         文件控制块
**           piNRead          剩余数据量
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ramFsNRead (PLW_FD_ENTRY  pfdentry, INT  *piNRead)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    
    if (piNRead == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pramn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_FILE_LOCK(pramn) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pramn->RAMN_mode)) {
        __RAMFS_FILE_UNLOCK(pramn);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    *piNRead = (INT)(pramn->RAMN_stSize - (size_t)pfdentry->FDENTRY_oftPtr);
    
    __RAMFS_FILE_UNLOCK(pramn);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ramFsNRead64
** 功能描述: ramFs nread 操作
** 输　入  : pfdentry         文件控制块
**           poftNRead        剩余数据量
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ramFsNRead64 (PLW_FD_ENTRY  pfdentry, off_t  *poftNRead)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    
    if (poftNRead == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pramn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_FILE_LOCK(pramn) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pramn->RAMN_mode)) {
        __RAMFS_FILE_UNLOCK(pramn);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    *poftNRead = (off_t)(pramn->RAMN_stSize - (size_t)pfdentry->FDENTRY_oftPtr);
    
    __RAMFS_FILE_UNLOCK(pramn);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ramFsRename
** 功能描述: ramFs rename 操作
** 输　入  : pfdentry         文件控制块
**           pcNewName        新的名称
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ramFsRename (PLW_FD_ENTRY  pfdentry, PCHAR  pcNewName)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    PRAM_VOLUME   pramfsNew;
    CHAR          cNewPath[PATH_MAX + 1];
    INT           iError;
    
    if (pramn == LW_NULL) {                                             /*  检查是否为设备文件          */
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                         /*  不支持设备重命名            */
        return (PX_ERROR);
    }
    
    if (pcNewName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return (PX_ERROR);
    }
    
    if (__STR_IS_ROOT(pcNewName)) {
        _ErrorHandle(ENOENT);
        return (PX_ERROR);
    }
    
    if (__RAMFS_FILE_LOCK(pramn) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (ioFullFileNameGet(pcNewName, 
                          (LW_DEV_HDR **)&pramfsNew, 
                          cNewPath) != ERROR_NONE) {                    /*  获得新目录路径              */
        __RAMFS_FILE_UNLOCK(pramn);
        return  (PX_ERROR);
    }
    
    if (pramfsNew != pramfs) {                                          /*  必须为同一设备节点          */
        __RAMFS_FILE_UNLOCK(pramn);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    
    iError = __ram_move(pramn, cNewPath);
    
    __RAMFS_FILE_UNLOCK(pramn);
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __ramFsSeek
** 功能描述: ramFs seek 操作
** 输　入  : pfdentry         文件控制块
**           oftOffset        偏移量
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ramFsSeek (PLW_FD_ENTRY  pfdentry,
                         off_t         oftOffset)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    
    if (pramn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (oftOffset > (size_t)~0) {
        _ErrorHandle(EOVERFLOW);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_FILE_LOCK(pramn) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pramn->RAMN_mode)) {
        __RAMFS_FILE_UNLOCK(pramn);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    pfdentry->FDENTRY_oftPtr = oftOffset;
    if (pramn->RAMN_stVSize < (size_t)oftOffset) {
        pramn->RAMN_stVSize = (size_t)oftOffset;
    }
    
    __RAMFS_FILE_UNLOCK(pramn);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ramFsWhere
** 功能描述: ramFs 获得文件当前读写指针位置 (使用参数作为返回值, 与 FIOWHERE 的要求稍有不同)
** 输　入  : pfdentry            文件控制块
**           poftPos             读写指针位置
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ramFsWhere (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    if (poftPos) {
        *poftPos = (off_t)pfdentry->FDENTRY_oftPtr;
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __ramFsStatGet
** 功能描述: ramFs stat 操作
** 输　入  : pfdentry         文件控制块
**           pstat            文件状态
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ramFsStat (PLW_FD_ENTRY  pfdentry, struct stat *pstat)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    __ram_stat(pramn, pramfs, pstat);
    
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ramFsLStat
** 功能描述: ramFs stat 操作
** 输　入  : pramfs           romfs 文件系统
**           pcName           文件名
**           pstat            文件状态
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ramFsLStat (PRAM_VOLUME  pramfs, PCHAR  pcName, struct stat *pstat)
{
    PRAM_NODE     pramn;
    BOOL          bRoot;
    
    if (!pcName || !pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    pramn = __ram_open(pramfs, pcName, LW_NULL, &bRoot, LW_NULL, LW_NULL);
    if (pramn) {
        __ram_stat(pramn, pramfs, pstat);
    
    } else if (bRoot) {
        __ram_stat(LW_NULL, pramfs, pstat);
    
    } else {
        __RAMFS_VOL_UNLOCK(pramfs);
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    __RAMFS_VOL_UNLOCK(pramfs);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ramFsStatfs
** 功能描述: ramFs statfs 操作
** 输　入  : pfdentry         文件控制块
**           pstatfs          文件系统状态
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ramFsStatfs (PLW_FD_ENTRY  pfdentry, struct statfs *pstatfs)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    if (!pstatfs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    __ram_statfs(pramfs, pstatfs);
    
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ramFsReadDir
** 功能描述: ramFs 获得指定目录信息
** 输　入  : pfdentry            文件控制块
**           dir                 目录结构
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ramFsReadDir (PLW_FD_ENTRY  pfdentry, DIR  *dir)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    INT                i;
    LONG               iStart = dir->dir_pos;
    INT                iError = ERROR_NONE;
    PLW_LIST_LINE      plineTemp;
    PLW_LIST_LINE      plineHeader;
    PRAM_NODE          pramnTemp;
    
    if (!dir) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    if (pramn == LW_NULL) {
        plineHeader = pramfs->RAMFS_plineSon;
    } else {
        if (!S_ISDIR(pramn->RAMN_mode)) {
            __RAMFS_VOL_UNLOCK(pramfs);
            _ErrorHandle(ENOTDIR);
            return  (PX_ERROR);
        }
        plineHeader = pramn->RAMN_plineSon;
    }
    
    for ((plineTemp  = plineHeader), (i = 0); 
         (plineTemp != LW_NULL) && (i < iStart); 
         (plineTemp  = _list_line_get_next(plineTemp)), (i++));         /*  忽略                        */
    
    if (plineTemp == LW_NULL) {
        _ErrorHandle(ENOENT);
        iError = PX_ERROR;                                              /*  没有多余的节点              */
    
    } else {
        pramnTemp = _LIST_ENTRY(plineTemp, RAM_NODE, RAMN_lineBrother);
        dir->dir_pos++;
        
        lib_strlcpy(dir->dir_dirent.d_name, 
                    pramnTemp->RAMN_pcName, 
                    sizeof(dir->dir_dirent.d_name));
                    
        dir->dir_dirent.d_type = IFTODT(pramnTemp->RAMN_mode);
        
        dir->dir_dirent.d_shortname[0] = PX_EOS;
    }
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __ramFsTimeset
** 功能描述: ramfs 设置文件时间
** 输　入  : pfdentry            文件控制块
**           utim                utimbuf 结构
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ramFsTimeset (PLW_FD_ENTRY  pfdentry, struct utimbuf  *utim)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    if (!utim) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (pramn) {
        pramn->RAMN_timeAccess = utim->actime;
        pramn->RAMN_timeChange = utim->modtime;
    
    } else {
        pramfs->RAMFS_time = utim->modtime;
    }
    
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ramFsTruncate
** 功能描述: ramfs 设置文件大小
** 输　入  : pfdentry            文件控制块
**           oftSize             文件大小
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ramFsTruncate (PLW_FD_ENTRY  pfdentry, off_t  oftSize)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    size_t        stTru;
    
    if (pramn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (oftSize > (size_t)~0) {
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pramn->RAMN_mode)) {
        __RAMFS_VOL_UNLOCK(pramfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    stTru = (size_t)oftSize;
    
    if (stTru > pramn->RAMN_stSize) {
        __ram_increase(pramn, stTru);
        
    } else if (stTru < pramn->RAMN_stSize) {
        __ram_truncate(pramn, stTru);
    }
    
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ramFsChmod
** 功能描述: ramfs chmod 操作
** 输　入  : pfdentry            文件控制块
**           iMode               新的 mode
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ramFsChmod (PLW_FD_ENTRY  pfdentry, INT  iMode)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    iMode |= S_IRUSR;
    iMode &= ~S_IFMT;
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (pramn) {
        pramn->RAMN_mode &= S_IFMT;
        pramn->RAMN_mode |= iMode;
    } else {
        pramfs->RAMFS_mode &= S_IFMT;
        pramfs->RAMFS_mode |= iMode;
    }
    
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ramFsChown
** 功能描述: ramfs chown 操作
** 输　入  : pfdentry            文件控制块
**           pusr                新的所属用户
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ramFsChown (PLW_FD_ENTRY  pfdentry, LW_IO_USR  *pusr)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    if (!pusr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (pramn) {
        pramn->RAMN_uid = pusr->IOU_uid;
        pramn->RAMN_gid = pusr->IOU_gid;
    } else {
        pramfs->RAMFS_uid = pusr->IOU_uid;
        pramfs->RAMFS_gid = pusr->IOU_gid;
    }
    
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ramFsSymlink
** 功能描述: ramFs 创建符号链接文件
** 输　入  : pramfs              romfs 文件系统
**           pcName              链接原始文件名
**           pcLinkDst           链接目标文件名
**           stMaxSize           缓冲大小
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ramFsSymlink (PRAM_VOLUME   pramfs,
                            PCHAR         pcName,
                            CPCHAR        pcLinkDst)
{
    PRAM_NODE     pramn;
    PRAM_NODE     pramnFather;
    BOOL          bRoot;
    
    if (!pcName || !pcLinkDst) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__fsCheckFileName(pcName)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    pramn = __ram_open(pramfs, pcName, &pramnFather, &bRoot, LW_NULL, LW_NULL);
    if (pramn || bRoot) {
        __RAMFS_VOL_UNLOCK(pramfs);
        _ErrorHandle(EEXIST);
        return  (PX_ERROR);
    }
    
    pramn = __ram_maken(pramfs, pcName, pramnFather, S_IFLNK | DEFAULT_SYMLINK_PERM, pcLinkDst);
    if (pramn == LW_NULL) {
        __RAMFS_VOL_UNLOCK(pramfs);
        return  (PX_ERROR);
    }
    
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ramFsReadlink
** 功能描述: ramFs 读取符号链接文件内容
** 输　入  : pramfs              romfs 文件系统
**           pcName              链接原始文件名
**           pcLinkDst           链接目标文件名
**           stMaxSize           缓冲大小
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t __ramFsReadlink (PRAM_VOLUME   pramfs,
                                PCHAR         pcName,
                                PCHAR         pcLinkDst,
                                size_t        stMaxSize)
{
    PRAM_NODE   pramn;
    size_t      stLen;
    
    if (!pcName || !pcLinkDst || !stMaxSize) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    pramn = __ram_open(pramfs, pcName, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    if ((pramn == LW_NULL) || !S_ISLNK(pramn->RAMN_mode)) {
        __RAMFS_VOL_UNLOCK(pramfs);
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    stLen = lib_strlen(pramn->RAMN_pcLink);
    
    lib_strncpy(pcLinkDst, pramn->RAMN_pcLink, stMaxSize);
    
    if (stLen > stMaxSize) {
        stLen = stMaxSize;                                              /*  计算有效字节数              */
    }
    
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  ((ssize_t)stLen);
}
/*********************************************************************************************************
** 函数名称: __ramFsIoctl
** 功能描述: ramFs ioctl 操作
** 输　入  : pfdentry           文件控制块
**           request,           命令
**           arg                命令参数
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ramFsIoctl (PLW_FD_ENTRY  pfdentry,
                          INT           iRequest,
                          LONG          lArg)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    off_t         oftTemp;
    INT           iError;
    
    switch (iRequest) {
    
    case FIOCONTIG:
    case FIOTRUNC:
    case FIOLABELSET:
    case FIOATTRIBSET:
    case FIOSQUEEZE:
        if ((pfdentry->FDENTRY_iFlag & O_ACCMODE) == O_RDONLY) {
            _ErrorHandle(ERROR_IO_WRITE_PROTECTED);
            return  (PX_ERROR);
        }
	}
	
	switch (iRequest) {
	
	case FIODISKINIT:                                                   /*  磁盘初始化                  */
        return  (ERROR_NONE);
        
    case FIOSEEK:                                                       /*  文件重定位                  */
        oftTemp = *(off_t *)lArg;
        return  (__ramFsSeek(pfdentry, oftTemp));

    case FIOWHERE:                                                      /*  获得文件当前读写指针        */
        iError = __ramFsWhere(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }
        
    case FIONREAD:                                                      /*  获得文件剩余字节数          */
        return  (__ramFsNRead(pfdentry, (INT *)lArg));
        
    case FIONREAD64:                                                    /*  获得文件剩余字节数          */
        iError = __ramFsNRead64(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }

    case FIORENAME:                                                     /*  文件重命名                  */
        return  (__ramFsRename(pfdentry, (PCHAR)lArg));
    
    case FIOLABELGET:                                                   /*  获取卷标                    */
    case FIOLABELSET:                                                   /*  设置卷标                    */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    
    case FIOFSTATGET:                                                   /*  获得文件状态                */
        return  (__ramFsStat(pfdentry, (struct stat *)lArg));
    
    case FIOFSTATFSGET:                                                 /*  获得文件系统状态            */
        return  (__ramFsStatfs(pfdentry, (struct statfs *)lArg));
    
    case FIOREADDIR:                                                    /*  获取一个目录信息            */
        return  (__ramFsReadDir(pfdentry, (DIR *)lArg));
    
    case FIOTIMESET:                                                    /*  设置文件时间                */
        return  (__ramFsTimeset(pfdentry, (struct utimbuf *)lArg));
        
    case FIOTRUNC:                                                      /*  改变文件大小                */
        oftTemp = *(off_t *)lArg;
        return  (__ramFsTruncate(pfdentry, oftTemp));
    
    case FIOSYNC:                                                       /*  将文件缓存回写              */
    case FIOFLUSH:
    case FIODATASYNC:
        return  (ERROR_NONE);
        
    case FIOCHMOD:
        return  (__ramFsChmod(pfdentry, (INT)lArg));                    /*  改变文件访问权限            */
    
    case FIOSETFL:                                                      /*  设置新的 flag               */
        if ((INT)lArg & O_NONBLOCK) {
            pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);
    
    case FIOCHOWN:                                                      /*  修改文件所属关系            */
        return  (__ramFsChown(pfdentry, (LW_IO_USR *)lArg));
    
    case FIOFSTYPE:                                                     /*  获得文件系统类型            */
        *(PCHAR *)lArg = "RAM FileSystem";
        return  (ERROR_NONE);
    
    case FIOGETFORCEDEL:                                                /*  强制卸载设备是否被允许      */
        *(BOOL *)lArg = pramfs->RAMFS_bForceDelete;
        return  (ERROR_NONE);
        
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_RAMFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
