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
** 文   件   名: fatFs.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 09 月 26 日
**
** 描        述: FAT 文件系统与 IO 系统接口部分.

** BUG
2008.12.02  在创建 FAT 设备时, 需要复位设备.
2008.12.04  f_chmod_ex() 第四个参数需要为 0xFF
2009.02.19  __fatFsMkMode() 支持 O_CREAT | O_EXCL.
2009.03.05  DWORD 类型确定为 32 位, ULONG 为 32/64 复合根据 CPU 类型确定.
2009.03.10  升级了 errno 系统, 相关的地方进行了修改.
2009.03.16  升级 __fatFsStatfsGet() 函数.
            创建磁盘时, 需要首先打开电源.
            卸载卷时首先回写磁盘 CACHE , 然后断电, 如果是可移动磁盘需要弹出.
2009.03.21  不管出现什么物理错误, 必须可以卸载卷, remove 这里不再判断物理操作错误.
2009.03.27  打开文件时, 加入对只读卷的判断.
2009.04.17  升级了文件系统, 同时升了错误号.
2009.04.17  目前不支持卷标的操作...
            FILINFO 长文件名缓冲需要先初始化.
2009.04.17  readdir 加入长文件名支持.
2009.04.19  __fatFsTruncate() 正确返回时保持文件读写指针位置不变化.
2009.04.22  在卸载卷时, 不关闭所有文件, 而是将有关文件变为异常状态.
2009.05.02  __fatFsClusterSizeGet() 更名为 __fatFsClusterSizeCal() 格式化确定磁盘簇大小时, 需要考虑扇区大
            小.
2009.06.05  加入文件互斥保护, 当一个文件被写方式打开, 他不能被打开. 当文件被读方式打开, 打不能被初读以外
            的方式打开. 
2009.06.10  将 FIOMOVE 与 FIORENAME 合并处理.
            设置文件时间时, 要检测文件的可读写性.
2009.06.18  修正文件锁判断的问题, stat 操作时, mode 需要与文件锁结合操作.
2009.07.05  使用 fsCommon 提供的函数进行文件名判断.
2009.07.06  在卸载磁盘时, 需要对物理磁盘链接进行处理.
2009.07.09  支持 64 bit 文件指针.
2009.08.26  readdir, pos 为 0 时, 首先需要 rewind 操作.
2009.09.03  创建卷时, 记录卷标的时间信息.
2009.10.22  read write 返回值为 ssize_t.
2009.10.28  卷控制块成员: FATVOL_fatfsVol 创建时一定要清零! 否则auto_mount()再到f_syncfs()操作没有格式化的
            磁盘可能导致系统崩溃!
2009.11.18  加入配置, 可选择 errno 符合 sylixos 内部 error 定义或与 posix 兼容.
2009.12.01  加入了操作系统注册函数.
2010.01.08  当系统不使用自动目录压缩时, 在 FAT 打开和关闭时需要使用目录压缩.
2010.01.11  重命名的新文件名需要解压缩.
2010.01.14  支持 FIOSETFL 命令.
2010.03.09  修正 open 中创建目录时调用 f_sync 的 bug!
2010.09.10  __fatFsReadDir() 中加入对 d_type 字段的支持.
2011.03.06  修正 gcc 4.5.1 相关 warning.
2011.03.27  加入对创建文件类型支持类的判断.
2011.03.29  当 iosDevAddEx() 出错时需要记录错误类型.
2011.06.14  __fatFsTruncate() 修正可能覆盖已有正确数据的问题.
2011.08.11  压缩目录不再由文件系统完成, 操作系统已处理了.
2011.11.21  升级文件系统相应的接口.
2012.03.10  修正 __fatFsReadDir() 当读取结束时返回 ENOENT 错误.
2012.06.29  将 inode 参数传入 __filInfoToStat().
2012.08.16  加入 pread 与 pwrite 操作, 但是不是很理想, 已经建议 FatFs 作者加入 f_pread 和 f_pwrite 操作.
2012.09.01  加入不可强制卸载卷的保护.
2012.09.25  fat 允许创建 socket 类型文件. (和普通文件无区别)
2012.10.20  修正部分 errno.
2012.12.08  注册的文件系统名改为 vfat.
2012.12.14  __FAT_FILE_LOCK() 需要判断返回值, 这样更加安全, 适合热插拔卷.
2013.01.08  fatfs 使用最新 NEW_1 设备驱动类型. 这样可以支持文件锁.
            fat 中的文件唯一的编号就是所在的簇 + 目录偏移, 但是这是 32bit + 16bit 所以没有办法编入 32bit
            inode 编号, 所以这里使用一个 hash 表, 保存所有已经操作过的文件通过 unique 分配出的 inode ,
            如果打开以前已经操作过的文件则获取到已经分配过的 unique, 这样可以保证每一个文件都有对应的唯一
            的 unique 编号, 不管何时操作这个文件, 这个编号都是确定的, 也是唯一的.
2013.01.18  所有对只读文件系统带有 O_CREAT 的操作被拒绝.
2013.01.21  提供 shell 命令设置 fat 默认的 uid gid.
2013.04.11  正常卸载卷时, uniq 发生器没有卸载.
2013.06.24  升级 fat 文件系统, 同时支持卷标.
2014.06.24  FAT 设备 64 为序列号为 -1.
2014.12.31  支持 ff10.c 文件系统.
2016.09.18  支持 exFAT 文件系统格式.
2017.04.27  fat 内部使用 O_RDWR 打开, 防止多重打开时内部权限判断错误.
2017.09.22  格式化操作不使用 exFAT 系统, 防止 BIOS 兼容性问题.
2017.12.27  修正 rename 符合 POSIX 标准.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
#include "../SylixOS/fs/unique/unique.h"
/*********************************************************************************************************
  SHELL
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "../SylixOS/shell/include/ttiny_shell.h"
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_FATFS_EN > 0)
#include "ff.h"
/*********************************************************************************************************
  内部结构 (如果不使用 inode 功能会节约内存, 加快操作速度)
*********************************************************************************************************/
#define __FAT_FILE_INOD_EN              1                               /*  是否允许 FAT inode 功能     */
#define __FAT_FILE_HASH_SIZE            64                              /*  inode hash size             */
#define __FAT_FILE_UNIQ_SIZE            1024                            /*  可以记录 1024 * 8 个 inode  */

typedef struct {
    LW_DEV_HDR          FATVOL_devhdrHdr;                               /*  设备头                      */
    BOOL                FATVOL_bForceDelete;                            /*  是否允许强制卸载卷          */
    FATFS               FATVOL_fatfsVol;                                /*  文件系统卷信息              */
    INT                 FATVOL_iDrv;                                    /*  驱动表位置                  */
    BOOL                FATVAL_bValid;                                  /*  有效性标志                  */
    LW_OBJECT_HANDLE    FATVOL_hVolLock;                                /*  卷操作锁                    */
    
    LW_LIST_LINE_HEADER FATVOL_plineFdNodeHeader;                       /*  fd_node 链表                */
    LW_LIST_LINE_HEADER FATVOL_plineHashHeader[__FAT_FILE_HASH_SIZE];   /*  所有已经操作过的文件 hash   */
    PLW_UNIQUE_POOL     FATVOL_puniqPool;                               /*  inode 发生器                */
    
    UINT32              FATVOL_uiTime;                                  /*  卷创建时间 FAT 格式         */
    INT                 FATVOL_iFlag;                                   /*  O_RDONLY or O_RDWR          */
} FAT_VOLUME;
typedef FAT_VOLUME     *PFAT_VOLUME;

typedef struct {
    PFAT_VOLUME         FATFIL_pfatvol;                                 /*  所在卷信息                  */
    union {
        FIL             FFTM_file;                                      /*  文件类型信息                */
        FATDIR          FFTM_fatdir;                                    /*  目录类型信息                */
    } FATFIL_fftm;
    INT                 FATFIL_iFileType;                               /*  文件类型                    */
    UINT64              FATFIL_u64Uniq;                                 /*  64bits 簇 + 目录偏移        */
    ino_t               FATFIL_inode;                                   /*  inode 编号                  */
    CHAR                FATFIL_cName[1];                                /*  文件名                      */
} FAT_FILE;
typedef FAT_FILE       *PFAT_FILE;
/*********************************************************************************************************
  由于 FAT 无法直接获取 inode 编号, 这里为每一个曾经操作过的文件建立一个统一的表, 记录对应的文件 inode
  重复打开同一个文件则会使用唯一一个 inode.
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE        FATHIS_lineManage;
    UINT64              FATHIS_u64Uniq;                                 /*  64bits 簇 + 目录偏移        */
    ino_t               FATHIS_inode;                                   /*  inode 编号                  */
} FAT_FILE_HIS;
typedef FAT_FILE_HIS   *PFAT_FILE_HIS;
/*********************************************************************************************************
  文件类型
*********************************************************************************************************/
#define __FAT_FILE_TYPE_NODE            0                               /*  open 打开文件               */
#define __FAT_FILE_TYPE_DIR             1                               /*  open 打开目录               */
#define __FAT_FILE_TYPE_DEV             2                               /*  open 打开设备               */
/*********************************************************************************************************
  FAT 主设备号与文件系统类型
*********************************************************************************************************/
static INT              _G_iFatDrvNum      = PX_ERROR;
static PCHAR            _G_pcFat12FsString = "FAT12 FileSystem";
static PCHAR            _G_pcFat16FsString = "FAT16 FileSystem";
static PCHAR            _G_pcFat32FsString = "FAT32 FileSystem";
static PCHAR            _G_pcExFatFsString = "exFAT FileSystem";
/*********************************************************************************************************
  FAT 默认 ugid
*********************************************************************************************************/
static uid_t            _G_uidFatDefault;
static gid_t            _G_gidFatDefault;
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
INT             __blockIoDevCreate(PLW_BLK_DEV  pblkdNew);
VOID            __blockIoDevDelete(INT  iIndex);
PLW_BLK_DEV     __blockIoDevGet(INT  iIndex);
INT             __blockIoDevReset(INT  iIndex);
INT             __blockIoDevIoctl(INT  iIndex, INT  iCmd, LONG  lArg);
INT             __blockIoDevIsLogic(INT  iIndex);
INT             __blockIoDevFlag(INT     iIndex);
/*********************************************************************************************************
  类型转换函数声明
*********************************************************************************************************/
VOID            __filInfoToStat(FILINFO     *filinfo, 
                                FATFS       *fatfs,
                                struct stat *pstat, 
                                ino_t        ino);
INT             __fsInfoToStatfs(FATFS         *fatfs,
                                 INT            iFlag,
                                 struct statfs *pstatfs, 
                                 INT            iDrv);
mode_t          __fsAttrToMode(BYTE  ucAttr);
BYTE            __fsModeToAttr(mode_t  iMode);
UINT32          __timeToFatTime(time_t  *ptime);
UINT32          get_fattime(void);
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define __FAT_FILE_LOCK(pfatfile)   API_SemaphoreMPend(pfatfile->FATFIL_pfatvol->FATVOL_hVolLock, \
                                    LW_OPTION_WAIT_INFINITE)
#define __FAT_FILE_UNLOCK(pfatfile) API_SemaphoreMPost(pfatfile->FATFIL_pfatvol->FATVOL_hVolLock)
#define __FAT_VOL_LOCK(pfatvol)     API_SemaphoreMPend(pfatvol->FATVOL_hVolLock, LW_OPTION_WAIT_INFINITE)
#define __FAT_VOL_UNLOCK(pfatvol)   API_SemaphoreMPost(pfatvol->FATVOL_hVolLock)
/*********************************************************************************************************
  检测路径字串是否为根目录或者直接指向设备
*********************************************************************************************************/
#define __STR_IS_ROOT(pcName)       ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))
/*********************************************************************************************************
  驱动程序声明
*********************************************************************************************************/
static INT      __fatFsCheck(PLW_BLK_DEV  pblkd, UINT8  ucPartType);
static LONG     __fatFsOpen(PFAT_VOLUME     pfatvol,
                            PCHAR           pcName,
                            INT             iFlags,
                            INT             iMode);
static INT      __fatFsRemove(PFAT_VOLUME     pfatvol,
                              PCHAR           pcName);
static INT      __fatFsClose(PLW_FD_ENTRY   pfdentry);
static ssize_t  __fatFsRead(PLW_FD_ENTRY   pfdentry,
                            PCHAR          pcBuffer, 
                            size_t         stMaxBytes);
static ssize_t  __fatFsPRead(PLW_FD_ENTRY   pfdentry,
                             PCHAR          pcBuffer, 
                             size_t         stMaxBytes,
                             off_t      oftPos);
static ssize_t  __fatFsWrite(PLW_FD_ENTRY   pfdentry,
                             PCHAR          pcBuffer, 
                             size_t         stNBytes);
static ssize_t  __fatFsPWrite(PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer, 
                              size_t        stNBytes,
                              off_t         oftPos);
static INT      __fatFsIoctl(PLW_FD_ENTRY   pfdentry,
                             INT            iRequest,
                             LONG           lArg);
static INT      __fatFsSync(PLW_FD_ENTRY   pfdentry, BOOL  bFlushCache);
/*********************************************************************************************************
  文件系统创建函数
*********************************************************************************************************/
LW_API INT      API_FatFsDevCreate(PCHAR   pcName, PLW_BLK_DEV  pblkd);
/*********************************************************************************************************
** 函数名称: __tshellFatUGID
** 功能描述: 系统命令 "fatugid"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellFatUGID (INT  iArgC, PCHAR  ppcArgV[])
{
    uid_t   uid;
    gid_t   gid;

    if (iArgC != 3) {
        printf("vfat current uid: %u gid: %u\n", _G_uidFatDefault, _G_gidFatDefault);
        return  (ERROR_NONE);
    
    } else {
        if (sscanf(ppcArgV[1], "%u", &uid) != 1) {
            fprintf(stderr, "fatugid [uid] [gid]\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        if (sscanf(ppcArgV[2], "%u", &gid) != 1) {
            fprintf(stderr, "fatugid [uid] [gid]\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        __KERNEL_MODE_PROC(
            _G_uidFatDefault = uid;
            _G_gidFatDefault = gid;
        );
        
        return  (ERROR_NONE);
    }
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** 函数名称: API_FatFsDrvInstall
** 功能描述: 安装 FAT 文件系统驱动程序
** 输　入  : 
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_FatFsDrvInstall (VOID)
{
    struct file_operations     fileop;

    if (_G_iFatDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));
    
    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __fatFsOpen;
    fileop.fo_release  = __fatFsRemove;
    fileop.fo_open     = __fatFsOpen;
    fileop.fo_close    = __fatFsClose;
    fileop.fo_read     = __fatFsRead;
    fileop.fo_read_ex  = __fatFsPRead;
    fileop.fo_write    = __fatFsWrite;
    fileop.fo_write_ex = __fatFsPWrite;
    fileop.fo_ioctl    = __fatFsIoctl;
    
    _G_iFatDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);       /*  使用 NEW_1 型设备驱动       */
     
    DRIVER_LICENSE(_G_iFatDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_iFatDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iFatDrvNum, "FAT12/16/32 driver.");
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "microsoft FAT file system installed.\r\n");
    
    __fsRegister("vfat", API_FatFsDevCreate, (FUNCPTR)__fatFsCheck);    /*  注册文件系统                */
    
#if LW_CFG_SHELL_EN > 0
    API_TShellKeywordAdd("fatugid", __tshellFatUGID);
    API_TShellFormatAdd("fatugid", " [uid] [gid]");
    API_TShellHelpAdd("fatugid", "set/get fat volume default uid gid.\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
    
    return  ((_G_iFatDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** 函数名称: API_FatFsDevCreate
** 功能描述: 创建一个 FAT 设备, 例如: API_FatFsDevCreate("/ata0", ...); 
**           与 sylixos 的 yaffs 不同, FAT 每一个卷都是独立的设备.
** 输　入  : pcName            设备名(设备挂接的节点地址)
**           pblkd             块设备驱动
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_FatFsDevCreate (PCHAR   pcName, PLW_BLK_DEV  pblkd)
{
    REGISTER PFAT_VOLUME     pfatvol;
    REGISTER INT             iBlkdIndex;
    REGISTER FRESULT         fresError;
             INT             iErrLevel = 0;
             ULONG           ulError   = ERROR_NONE;

    if (_G_iFatDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "fat Driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    if (pblkd == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "block device invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    if ((pcName == LW_NULL) || __STR_IS_ROOT(pcName)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "volume name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    if ((pblkd->BLKD_iLogic == 0) && (pblkd->BLKD_uiLinkCounter)) {     /*  物理设备被引用后不允许加载  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "logic device has already mount.\r\n");
        _ErrorHandle(ERROR_IO_ACCESS_DENIED);
        return  (PX_ERROR);
    }
    
    iBlkdIndex = __blockIoDevCreate(pblkd);                             /*  加入块设备驱动表            */
    if (iBlkdIndex == -1) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "block device invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    } else if (iBlkdIndex == -2) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "block device table full.\r\n");
        _ErrorHandle(ERROR_IOS_DRIVER_GLUT);
        return  (PX_ERROR);
    }
    
    pfatvol = (PFAT_VOLUME)__SHEAP_ALLOC(sizeof(FAT_VOLUME));
    if (pfatvol == LW_NULL) {
        __blockIoDevDelete(iBlkdIndex);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pfatvol, sizeof(FAT_VOLUME));                             /*  清空卷控制块                */
    
#if __FAT_FILE_INOD_EN > 0
    pfatvol->FATVOL_puniqPool = API_FsUniqueCreate(__FAT_FILE_UNIQ_SIZE, 1);
    if (pfatvol->FATVOL_puniqPool == LW_NULL) {                         /*  创建 inode 发生器           */
        iErrLevel = 1;
        goto    __error_handle;
    }
#endif                                                                  /*  是否允许 inode 发生器       */
    
    pfatvol->FATVOL_bForceDelete = LW_FALSE;                            /*  不允许强制卸载卷            */
    
    pfatvol->FATVOL_iDrv     = iBlkdIndex;                              /*  记录驱动位置                */
    pfatvol->FATVAL_bValid   = LW_TRUE;                                 /*  卷有效                      */
    pfatvol->FATVOL_hVolLock = API_SemaphoreMCreate("fatvol_lock", 
                               LW_PRIO_DEF_CEILING,
                               LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE | 
                               LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                               LW_NULL);
    if (!pfatvol->FATVOL_hVolLock) {                                    /*  无法创建卷锁                */
        iErrLevel = 2;
        goto    __error_handle;
    }
    pfatvol->FATVOL_plineFdNodeHeader = LW_NULL;                        /*  没有文件被打开              */
    pfatvol->FATVOL_uiTime = get_fattime();                             /*  获得当前时间                */
    
    fresError = f_mount_ex(&pfatvol->FATVOL_fatfsVol, (BYTE)iBlkdIndex);/*  挂载文件系统                */
    if (fresError != FR_OK) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "fat driver table full.\r\n");
        ulError = ERROR_IOS_DRIVER_GLUT;
        iErrLevel = 3;
        goto    __error_handle;
    }
    
    pfatvol->FATVOL_iFlag = pblkd->BLKD_iFlag;
    
    if (iosDevAddEx(&pfatvol->FATVOL_devhdrHdr, pcName, _G_iFatDrvNum, DT_DIR)
        != ERROR_NONE) {                                                /*  安装文件系统设备            */
        ulError = API_GetLastError();
        iErrLevel = 4;
        goto    __error_handle;
    }
    __fsDiskLinkCounterAdd(pblkd);                                      /*  增加块设备链接              */
    
    __blockIoDevIoctl(iBlkdIndex, LW_BLKD_CTRL_POWER, LW_BLKD_POWER_ON);/*  打开电源                    */
    __blockIoDevReset(iBlkdIndex);                                      /*  复位磁盘接口                */
    __blockIoDevIoctl(iBlkdIndex, FIODISKINIT, 0);                      /*  初始化磁盘                  */
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "disk \"%s\" mount ok.\r\n", pcName);
    
    return  (ERROR_NONE);
    
    /* 
     *  ERROR OCCUR
     */
__error_handle:
    if (iErrLevel > 3) {
        f_mount_ex(LW_NULL, (BYTE)iBlkdIndex);                          /*  卸载挂载的文件系统          */
    }
    if (iErrLevel > 2) {
        API_SemaphoreMDelete(&pfatvol->FATVOL_hVolLock);
    }
    if (iErrLevel > 1) {
        __blockIoDevDelete(iBlkdIndex);
    }
#if __FAT_FILE_INOD_EN > 0
    if (iErrLevel > 0) {
        API_FsUniqueDelete(pfatvol->FATVOL_puniqPool);
    }
#endif                                                                  /*  __FAT_FILE_INOD_EN          */
    __SHEAP_FREE(pfatvol);
    
    _ErrorHandle(ulError);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_FatFsDevDelete
** 功能描述: 删除一个 FAT 设备, 例如: API_FatFsDevDelete("/mnt/ata0");
** 输　入  : pcName            设备名(设备挂接的节点地址)
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_FatFsDevDelete (PCHAR   pcName)
{
    if (API_IosDevMatchFull(pcName)) {                                  /*  如果是设备, 这里就卸载设备  */
        return  (unlink(pcName));
    
    } else {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __fatFsCheck
** 功能描述: 检测 exFAT 分区是否有效
** 输　入  : pblkd             块设备
**           ucPartType        分区类型 (0x07 is NTFS or exFAT)
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fatFsCheck (PLW_BLK_DEV  pblkd, UINT8  ucPartType)
{
    PUCHAR  pucBuffer;
    ULONG   ulSecSize;
    
    if (!pblkd || (ucPartType != 0x07)) {
        return  (ERROR_NONE);
    }
    
    pblkd->BLKD_pfuncBlkIoctl(pblkd, LW_BLKD_CTRL_POWER, LW_BLKD_POWER_ON);
    pblkd->BLKD_pfuncBlkReset(pblkd);
    pblkd->BLKD_pfuncBlkIoctl(pblkd, FIODISKINIT);
    
    ulSecSize = pblkd->BLKD_ulBytesPerSector;
    if (!ulSecSize) {
        pblkd->BLKD_pfuncBlkIoctl(pblkd, LW_BLKD_GET_SECSIZE, &ulSecSize);
    }
    if (!ulSecSize) {
        return  (PX_ERROR);
    }
    
    pucBuffer = (PUCHAR)__SHEAP_ALLOC((size_t)ulSecSize);
    if (!pucBuffer) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    if (pblkd->BLKD_pfuncBlkRd(pblkd, pucBuffer, 0, 1) < 0) {
        __SHEAP_FREE(pucBuffer);
        return  (PX_ERROR);
    }
    
    if (lib_memcmp(pucBuffer, "\xEB\x76\x90" "EXFAT   ", 11)) {         /*  仅兼容 exFAT 文件系统       */
        __SHEAP_FREE(pucBuffer);
        return  (PX_ERROR);
    }
    
    __SHEAP_FREE(pucBuffer);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __fatFsHisAdd
** 功能描述: FAT FS 将一个已经操作过的文件放入历史记录
** 输　入  : pfatfile          文件
** 输　出  : inode 编号
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ino_t  __fatFsHisAdd (PFAT_FILE  pfatfile)
{
#if __FAT_FILE_INOD_EN > 0
    INT                  iHash        = (INT)(pfatfile->FATFIL_u64Uniq % __FAT_FILE_HASH_SIZE);
    LW_LIST_LINE_HEADER *pplineHeader = &pfatfile->FATFIL_pfatvol->FATVOL_plineHashHeader[iHash];
    PLW_LIST_LINE        plineTemp;
    PFAT_FILE_HIS        pfathis;
    
    for (plineTemp  = *pplineHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pfathis = _LIST_ENTRY(plineTemp, FAT_FILE_HIS, FATHIS_lineManage);
        if (pfathis->FATHIS_u64Uniq == pfatfile->FATFIL_u64Uniq) {
            break;
        }
    }
    
    if (plineTemp) {
        return  (pfathis->FATHIS_inode);
    }
    
    pfathis = (PFAT_FILE_HIS)__SHEAP_ALLOC(sizeof(FAT_FILE_HIS));
    if (pfathis == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (0);
    }
    
    pfathis->FATHIS_inode = API_FsUniqueAlloc(pfatfile->FATFIL_pfatvol->FATVOL_puniqPool);
    if (!API_FsUniqueIsVal(pfatfile->FATFIL_pfatvol->FATVOL_puniqPool, pfathis->FATHIS_inode)) {
        __SHEAP_FREE(pfathis);
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (0);
    }
    pfathis->FATHIS_u64Uniq = pfatfile->FATFIL_u64Uniq;                 /*  保存簇 + 偏移               */
    
    _List_Line_Add_Tail(&pfathis->FATHIS_lineManage, pplineHeader);
    
    return  (pfathis->FATHIS_inode);
#else

    return  (1);
#endif                                                                  /*  __FAT_FILE_INOD_EN          */
}
/*********************************************************************************************************
** 函数名称: __fatFsHisGet
** 功能描述: FAT FS 删除一个卷所有的历史记录 (卸载卷时使用)
** 输　入  : pfatvol          卷控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __fatFsHisDel (PFAT_VOLUME  pfatvol)
{
#if __FAT_FILE_INOD_EN > 0
    INT             i;
    PLW_LIST_LINE   plineTemp;
    PLW_LIST_LINE   plineDel;
    
    for (i = 0; i < __FAT_FILE_HASH_SIZE; i++) {
        plineTemp = pfatvol->FATVOL_plineHashHeader[i];
        while (plineTemp) {
            plineDel  = plineTemp;
            plineTemp = _list_line_get_next(plineTemp);
            __SHEAP_FREE(plineDel);                                     /*  这里不用先退出链表, 直接释放*/
        }
        pfatvol->FATVOL_plineHashHeader[i] = LW_NULL;
    }
#endif                                                                  /*  __FAT_FILE_INOD_EN          */
}
/*********************************************************************************************************
** 函数名称: __fatFsMkMode
** 功能描述: FAT FS 转换文件创建属性
** 输　入  : iFlags           posix 创建属性
** 输　出  : FatFs 创建属性
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BYTE  __fatFsMkMode (INT  iFlags)
{
    REGISTER BYTE   ucMode = 0;
    
    if (iFlags & O_WRONLY) {                                            /*  确定文件系统读写权限        */
        ucMode = FA_WRITE;
    } else if (iFlags & O_RDWR) {
        ucMode = FA_READ | FA_WRITE;
    } else {
        ucMode = FA_READ;
    }
     
    if ((iFlags & O_CREAT) &&
        (iFlags & O_EXCL)) {                                            /*  如果存在, 不新建, 返回错误  */
        ucMode |= FA_CREATE_NEW;                                        /*  有则错误, 无则新建          */
    } else if (iFlags & O_CREAT) {
        ucMode |= FA_OPEN_ALWAYS;                                       /*  有则打开, 无则新建          */
    }

    if (iFlags & O_TRUNC) {                                             /*  创建新文件并截断            */
        if ((iFlags & O_RDWR) ||
            (iFlags & O_WRONLY)) {
            ucMode |= FA_CREATE_ALWAYS;                                 /*  总是创建新文件, 并且trunc   */
        }
    }
    
    return  (ucMode);
}
/*********************************************************************************************************
** 函数名称: __fatFsGetError
** 功能描述: FAT FS 错误编号转换为 posix 标准
** 输　入  : fresError           FatFs 错误号
** 输　出  : POSIX 错误号
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __fatFsGetError (FRESULT    fresError)
{
    switch (fresError) {
    
    case FR_OK:
        return  (ERROR_NONE);
    
    case FR_NOT_READY:
        return  (EIO);
    
    case FR_NO_FILE:
        return  (ENOENT);
    
    case FR_NO_PATH:
        return  (ENOENT);
    
    case FR_INVALID_NAME:
        return  (EFAULT);
    
    case FR_INVALID_DRIVE:
        return  (ENODEV);
    
    case FR_DENIED:
        return  (EACCES);
    
    case FR_EXIST:
        return  (EEXIST);
    
    case FR_DISK_ERR:
        return  (EIO);
    
    case FR_WRITE_PROTECTED:
        return  (EWRPROTECT);
    
    case FR_NOT_ENABLED:
        return  (EOVERFLOW);
    
    case FR_NO_FILESYSTEM:
        return  (EFORMAT);
    
    case FR_INVALID_OBJECT:
        return  (EBADF);
    
    case FR_MKFS_ABORTED:
        return  (EIO);
    
    case FR_TIMEOUT:
        return  (ETIMEDOUT);
    
    case FR_LOCKED:
        return  (EBUSY);
    
    case FR_NOT_ENOUGH_CORE:
        return  (ENOMEM);
    
    case FR_TOO_MANY_OPEN_FILES:
        return  (EMFILE);
    
    case FR_INVALID_PARAMETER:
        return  (EINVAL);
        
    default:
        return  (EIO);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __fatFsGetInfo
** 功能描述: FAT FS 获得临时的基本属性
** 输　入  : pfatfile         fat文件
**           pstat64          64bit 文件属性
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fatFsGetInfo (PFAT_FILE  pfatfile, mode_t  *pmode, off_t  *poftSize)
{
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {           /*  普通文件                    */
        *pmode    = S_IFREG | S_IRWXU | S_IRWXO | S_IRWXO;              /*  临时设置为所有权限都支持    */
        *poftSize = f_size(&pfatfile->FATFIL_fftm.FFTM_file);
    
    } else if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_DIR) {     /*  目录文件                    */
        *pmode    = S_IFDIR | S_IRWXU | S_IRWXO | S_IRWXO;              /*  临时设置为所有权限都支持    */
        *poftSize = 0;
    
    } else {                                                            /*  根目录                      */
        *pmode    = S_IFDIR | S_IRWXU | S_IRWXO | S_IRWXO;              /*  临时设置为所有权限都支持    */
        *poftSize = 0;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __fatFsCloseFile
** 功能描述: FAT FS 关闭文件操作
** 输　入  : pfatfile         fat文件
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __fatFsCloseFile (PFAT_FILE  pfatfile)
{
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {           /*  标准 FAT 文件               */
        f_close(&pfatfile->FATFIL_fftm.FFTM_file);                      /*  FAT 关闭文件                */
    
    } else if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_DIR) {     /*  目录                        */
        f_closedir(&pfatfile->FATFIL_fftm.FFTM_fatdir);
    }
}
/*********************************************************************************************************
** 函数名称: __fatFsSeekFile
** 功能描述: FAT FS 执行内部文件指针操作
** 输　入  : pfatfile         fat文件
**           oftPtr           文件指针
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fatFsSeekFile (PFAT_FILE  pfatfile, off_t  oftPtr)
{
    FRESULT       fresError = FR_OK;
    ULONG         ulError   = ERROR_NONE;
    
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {           /*  标准 FAT 文件               */
        if (f_tell(&pfatfile->FATFIL_fftm.FFTM_file) == oftPtr) {       /*  不需要 seek                 */
            return  (ERROR_NONE);
        }
        
        fresError = f_lseek(&pfatfile->FATFIL_fftm.FFTM_file, oftPtr);
        if ((fresError == FR_OK) && 
            (f_tell(&pfatfile->FATFIL_fftm.FFTM_file) == oftPtr)) {
            return  (ERROR_NONE);
        }
        ulError = __fatFsGetError(fresError);                           /*  转换错误编号                */
    
    } else {
        ulError = EISDIR;
    }
    
    _ErrorHandle(ulError);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __fatFsOpen
** 功能描述: FAT FS open 操作
** 输　入  : pfatvol          卷控制块
**           pcName           文件名
**           iFlags           方式
**           iMode            方法
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  __fatFsOpen (PFAT_VOLUME     pfatvol,
                          PCHAR           pcName,
                          INT             iFlags,
                          INT             iMode)
{
    REGISTER PFAT_FILE      pfatfile;
    REGISTER BYTE           ucMode;
    REGISTER FRESULT        fresError;
    REGISTER ULONG          ulError;
             PLW_FD_NODE    pfdnode;
             BOOL           bIsNew;
             FILINFO        fileinfo;
             off_t          oftSize;


    if (pcName == LW_NULL) {                                            /*  无文件名                    */
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (__blockIoDevFlag(pfatvol->FATVOL_iDrv) == O_RDONLY) {       /*  此卷写保护, 处于只读状态    */
            if (iFlags & (O_CREAT | O_TRUNC | O_RDWR | O_WRONLY)) {
                _ErrorHandle(ERROR_IO_WRITE_PROTECTED);
                return  (PX_ERROR);
            }
        }
        if (iFlags & O_CREAT) {
            if (pfatvol->FATVOL_iFlag == O_RDONLY) {
                _ErrorHandle(EROFS);                                    /*  只读文件系统                */
                return  (PX_ERROR);
            }
            if (__fsCheckFileName(pcName)) {
                _ErrorHandle(ENOENT);
                return  (PX_ERROR);
            }
            if (S_ISFIFO(iMode) || 
                S_ISBLK(iMode)  ||
                S_ISCHR(iMode)) {                                       /*  这里不过滤 socket 文件      */
                _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);                /*  不支持以上这些格式          */
                return  (PX_ERROR);
            }
        }
    
        ucMode = __fatFsMkMode(iFlags);                                 /*  文件创建属性                */

        pfatfile = (PFAT_FILE)__SHEAP_ALLOC(sizeof(FAT_FILE) + 
                                            lib_strlen(pcName));        /*  创建文件内存                */
        if (pfatfile == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        lib_strcpy(pfatfile->FATFIL_cName, pcName);                     /*  记录文件名                  */
    
        pfatfile->FATFIL_pfatvol = pfatvol;                             /*  记录卷信息                  */
        
        ulError = __FAT_FILE_LOCK(pfatfile);
        if ((pfatvol->FATVAL_bValid == LW_FALSE) ||
            (ulError != ERROR_NONE)) {                                  /*  卷正在被卸载                */
            __FAT_FILE_UNLOCK(pfatfile);
            __SHEAP_FREE(pfatfile);
            _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
            return  (PX_ERROR);
        }
        
        if ((iFlags & O_CREAT) && S_ISDIR(iMode)) {                     /*  创建目录                    */
            fresError = f_mkdir_ex(&pfatvol->FATVOL_fatfsVol, pcName);
            if ((fresError != FR_OK) && (iFlags & O_EXCL)) {
                __FAT_FILE_UNLOCK(pfatfile);
                __SHEAP_FREE(pfatfile);
                ulError = __fatFsGetError(fresError);                   /*  转换错误编号                */
                _ErrorHandle(ulError);
                return  (PX_ERROR);
            }
        }
        
        fresError = f_open_ex(&pfatvol->FATVOL_fatfsVol, 
                              &pfatfile->FATFIL_fftm.FFTM_file,
                              &pfatfile->FATFIL_u64Uniq,
                              pcName, ucMode);
        pfatfile->FATFIL_iFileType = __FAT_FILE_TYPE_NODE;              /*  可以被 close 掉             */
        
        if (fresError != FR_OK) {                                       /*  打开失败                    */
            fresError =  f_stat_ex(&pfatvol->FATVOL_fatfsVol, 
                                   pcName, 
                                   &fileinfo);                          /*  检查是否为目录文件          */
            if ((fresError == FR_OK) &&
                (fileinfo.fattrib & AM_DIR)) {
                f_opendir_ex(&pfatvol->FATVOL_fatfsVol, 
                             &pfatfile->FATFIL_fftm.FFTM_fatdir,
                             &pfatfile->FATFIL_u64Uniq,
                             (CPCHAR)pcName);                           /*  打开目录                    */
                pfatfile->FATFIL_iFileType = __FAT_FILE_TYPE_DIR;       /*  目录文件                    */
                goto    __file_open_ok;                                 /*  文件打开正常                */
            }
            
            if (__STR_IS_ROOT(pfatfile->FATFIL_cName)) {
                pfatfile->FATFIL_iFileType = __FAT_FILE_TYPE_DEV;       /*  仅仅打开设备, 未格式化      */
                pfatfile->FATFIL_u64Uniq   = (UINT64)~0;                /*  不与任何文件重复(根目录)    */
                goto    __file_open_ok;                                 /*  文件打开正常                */
            }
            
            __FAT_FILE_UNLOCK(pfatfile);
            __SHEAP_FREE(pfatfile);
            ulError = __fatFsGetError(fresError);                       /*  转换错误编号                */
            _ErrorHandle(ulError);
            return  (PX_ERROR);
        
        } else {                                                        /*  普通文件打开成功            */
            pfatfile->FATFIL_fftm.FFTM_file.flag |= (FA_READ | FA_WRITE);
            f_sync(&pfatfile->FATFIL_fftm.FFTM_file);                   /*  回写磁盘(更新时间等信息)    */
        }
        
__file_open_ok:
        if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_DEV) {
            pfatfile->FATFIL_inode = 0;                                 /*  不与任何文件重复(根目录)    */
        } else {
            pfatfile->FATFIL_inode = __fatFsHisAdd(pfatfile);
            if (pfatfile->FATFIL_inode == 0) {                          /*  无法获取 inode              */
                goto    __file_open_error;
            }
        }
        
        __fatFsGetInfo(pfatfile, &iMode, &oftSize);                     /*  获得一些基本信息            */
        
        pfdnode = API_IosFdNodeAdd(&pfatvol->FATVOL_plineFdNodeHeader,
                                   (dev_t)&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                                   pfatfile->FATFIL_u64Uniq,            /*  这里使用 簇+偏移作为识别码  */
                                   iFlags, iMode, 0, 0, oftSize, 
                                   (PVOID)pfatfile,
                                   &bIsNew);                            /*  添加文件节点                */
        if (pfdnode == LW_NULL) {                                       /*  无法创建 fd_node 节点       */
            goto    __file_open_error;
        }
        
        if ((iFlags & O_TRUNC) && ((iFlags & O_ACCMODE) != O_RDONLY)) { /*  需要截断                    */
            if (bIsNew == LW_FALSE) {                                   /*  不允许重复打开确需清空      */
                API_IosFdNodeDec(&pfatvol->FATVOL_plineFdNodeHeader,
                                 pfdnode, LW_NULL);
                _ErrorHandle(EBUSY);
                goto    __file_open_error;
            
            } else if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {
                f_truncate(&pfatfile->FATFIL_fftm.FFTM_file);           /*  截断为 0                    */
            }
        }
        
        LW_DEV_INC_USE_COUNT(&pfatvol->FATVOL_devhdrHdr);               /*  更新计数器                  */
        
        if (bIsNew == LW_FALSE) {                                       /*  有重复打开                  */
            __fatFsCloseFile(pfatfile);
            __FAT_FILE_UNLOCK(pfatfile);
            __SHEAP_FREE(pfatfile);
        
        } else {
            __FAT_FILE_UNLOCK(pfatfile);
        }
        
        return  ((LONG)pfdnode);
    }
    
__file_open_error:
    __fatFsCloseFile(pfatfile);
    __FAT_FILE_UNLOCK(pfatfile);
    __SHEAP_FREE(pfatfile);
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __fatFsRemove
** 功能描述: FAT FS remove 操作
** 输　入  : pfatvol          卷控制块
**           pcName           文件名
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fatFsRemove (PFAT_VOLUME     pfatvol,
                           PCHAR           pcName)
{
    REGISTER FRESULT        fresError;
    REGISTER ULONG          ulError = ERROR_NONE;
             PLW_BLK_DEV    pblkd;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (__STR_IS_ROOT(pcName)) {                                    /*  根目录或者设备文件          */
            ulError = __FAT_VOL_LOCK(pfatvol);
            if (ulError) {
                _ErrorHandle(ENXIO);
                return  (PX_ERROR);                                     /*  正在被其他任务卸载          */
            }
            
            if (pfatvol->FATVAL_bValid == LW_FALSE) {
                __FAT_VOL_UNLOCK(pfatvol);
                return  (ERROR_NONE);                                   /*  正在被其他任务卸载          */
            }
            
__re_umount_vol:
            if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)pfatvol)) {          /*  检查是否有正在工作的文件    */
                if (!pfatvol->FATVOL_bForceDelete) {
                    __FAT_VOL_UNLOCK(pfatvol);
                    _ErrorHandle(EBUSY);
                    return  (PX_ERROR);                                 /*  有文件打开, 不能被卸载      */
                }
                
                pfatvol->FATVAL_bValid = LW_FALSE;                      /*  开始卸载卷, 文件再无法打开  */
                
                __FAT_VOL_UNLOCK(pfatvol);
                
                _DebugHandle(__ERRORMESSAGE_LEVEL, "disk have open file.\r\n");
                iosDevFileAbnormal(&pfatvol->FATVOL_devhdrHdr);         /*  将所有相关文件设为异常状态  */
                
                __FAT_VOL_LOCK(pfatvol);
                goto    __re_umount_vol;
                
            } else {
                pfatvol->FATVAL_bValid = LW_FALSE;                      /*  开始卸载卷, 文件再无法打开  */
            }
            
            /*
             *  不管出现什么物理错误, 必须可以卸载卷, 这里不再判断错误.
             */
            f_syncfs(&pfatvol->FATVOL_fatfsVol);                        /*  回写文件系统与磁盘          */
            
            __blockIoDevIoctl(pfatvol->FATVOL_iDrv,
                              FIOFLUSH, 0);                             /*  回写 DISK CACHE             */
            __blockIoDevIoctl(pfatvol->FATVOL_iDrv, LW_BLKD_CTRL_POWER, 
                              LW_BLKD_POWER_OFF);                       /*  设备断电                    */
            __blockIoDevIoctl(pfatvol->FATVOL_iDrv, LW_BLKD_CTRL_EJECT,
                              0);                                       /*  将设备弹出                  */
            
            pblkd = __blockIoDevGet(pfatvol->FATVOL_iDrv);              /*  获得块设备控制块            */
            if (pblkd) {
                __fsDiskLinkCounterDec(pblkd);                          /*  减少链接次数                */
            }
            
            iosDevDelete((LW_DEV_HDR *)pfatvol);                        /*  IO 系统移除设备             */
            
            f_mount_ex(LW_NULL, (BYTE)pfatvol->FATVOL_iDrv);            /*  卸载挂载的文件系统          */
            __blockIoDevIoctl(pfatvol->FATVOL_iDrv,
                              FIOUNMOUNT, 0);                           /*  执行底层脱离任务            */
            __blockIoDevDelete(pfatvol->FATVOL_iDrv);                   /*  在驱动表中移除              */
            
            __fatFsHisDel(pfatvol);                                     /*  释放所有历史记录            */
            
#if __FAT_FILE_INOD_EN > 0
            API_FsUniqueDelete(pfatvol->FATVOL_puniqPool);              /*  卸载 uniq 发生器            */
#endif                                                                  /*  __FAT_FILE_INOD_EN          */
            
            API_SemaphoreMDelete(&pfatvol->FATVOL_hVolLock);            /*  删除卷锁                    */
            
            __SHEAP_FREE(pfatvol);                                      /*  释放卷控制块                */
            
            _DebugHandle(__LOGMESSAGE_LEVEL, "disk unmount ok.\r\n");
            
            return  (ERROR_NONE);
        
        } else {                                                        /*  删除文件或目录              */
            if (__blockIoDevFlag(pfatvol->FATVOL_iDrv) == O_RDONLY) {   /*  此卷写保护, 处于只读状态    */
                _ErrorHandle(ERROR_IO_WRITE_PROTECTED);
                return  (PX_ERROR);
            }
            
            if (__fsCheckFileName(pcName) < 0) {                        /*  检查文件名是否合法          */
                _ErrorHandle(ENOENT);                                   /*  文件未找到                  */
                return  (PX_ERROR);
            }
            
            ulError = __FAT_VOL_LOCK(pfatvol);
            if ((pfatvol->FATVAL_bValid == LW_FALSE) ||
                (ulError != ERROR_NONE)) {                              /*  卷正在被卸载                */
                __FAT_VOL_UNLOCK(pfatvol);
                _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
                return  (PX_ERROR);
            }
            
            fresError = f_unlink_ex(&pfatvol->FATVOL_fatfsVol,
                                    pcName);                            /*  执行删除操作                */
            __FAT_VOL_UNLOCK(pfatvol);
            
            ulError = __fatFsGetError(fresError);                       /*  转换错误编号                */
            _ErrorHandle(ulError);
            
            if (ulError) {
                return  (PX_ERROR);                                     /*  删除失败                    */
            } else {
                return  (ERROR_NONE);                                   /*  删除正确                    */
            }
        }
    }
}
/*********************************************************************************************************
** 函数名称: __fatFsClose
** 功能描述: FAT FS close 操作
** 输　入  : pfdentry         文件控制块
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fatFsClose (PLW_FD_ENTRY    pfdentry)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE     pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    PFAT_VOLUME   pfatvol  = pfatfile->FATFIL_pfatvol;
    BOOL          bFree    = LW_FALSE;
    BOOL          bRemove  = LW_FALSE;

    if (pfatfile) {
        if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
            _ErrorHandle(ENXIO);                                        /*  设备出错                    */
            return  (PX_ERROR);
        }
        
        if (API_IosFdNodeDec(&pfatvol->FATVOL_plineFdNodeHeader,
                             pfdnode, &bRemove) == 0) {                 /*  fd_node 是否完全释放        */
            __fatFsCloseFile(pfatfile);
            bFree = LW_TRUE;
        }
        
        LW_DEV_DEC_USE_COUNT(&pfatvol->FATVOL_devhdrHdr);               /*  更新计数器                  */
        
        if (bRemove) {
            f_unlink_ex(&pfatvol->FATVOL_fatfsVol, 
                        pfatfile->FATFIL_cName);                        /*  删除文件                    */
        }
        
        __FAT_FILE_UNLOCK(pfatfile);
        
        if (bFree) {
            __SHEAP_FREE(pfatfile);                                     /*  释放内存                    */
        }
        
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __fatFsRead
** 功能描述: FAT FS read 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __fatFsRead (PLW_FD_ENTRY   pfdentry,
                             PCHAR          pcBuffer, 
                             size_t         stMaxBytes)
{
    PLW_FD_NODE   pfdnode   = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE     pfatfile  = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT       fresError = FR_OK;
    ULONG         ulError   = ERROR_NONE;
    UINT          uiReadNum = 0;
    
    if (!pcBuffer || !stMaxBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (pfatfile->FATFIL_iFileType != __FAT_FILE_TYPE_NODE) {
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__fatFsSeekFile(pfatfile, pfdentry->FDENTRY_oftPtr)) {          /*  调整内部文件指针            */
        __FAT_FILE_UNLOCK(pfatfile);
        return  (PX_ERROR);
    }
    
    fresError = f_read(&pfatfile->FATFIL_fftm.FFTM_file,
                       (PVOID)pcBuffer, 
                       (UINT)stMaxBytes, 
                       &uiReadNum);
    if ((fresError == FR_OK) && (uiReadNum > 0)) {
        pfdentry->FDENTRY_oftPtr += (off_t)uiReadNum;                   /*  更新文件指针                */
    }
    
    ulError = __fatFsGetError(fresError);                               /*  转换错误编号                */
    
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  ((ssize_t)uiReadNum);
}
/*********************************************************************************************************
** 函数名称: __fatFsPRead
** 功能描述: FAT FS pread 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
**           oftPos           位置
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __fatFsPRead (PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer, 
                              size_t        stMaxBytes,
                              off_t         oftPos)
{
    PLW_FD_NODE   pfdnode   = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE     pfatfile  = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT       fresError = FR_OK;
    ULONG         ulError   = ERROR_NONE;
    UINT          uiReadNum = 0;
    
    if (!pcBuffer || !stMaxBytes || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (pfatfile->FATFIL_iFileType != __FAT_FILE_TYPE_NODE) {
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__fatFsSeekFile(pfatfile, oftPos)) {                            /*  调整内部文件指针            */
        __FAT_FILE_UNLOCK(pfatfile);
        return  (PX_ERROR);
    }
    
    fresError = f_read(&pfatfile->FATFIL_fftm.FFTM_file,
                       (PVOID)pcBuffer, 
                       (UINT)stMaxBytes, 
                       &uiReadNum);
                       
    ulError = __fatFsGetError(fresError);                               /*  转换错误编号                */
    
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  ((ssize_t)uiReadNum);
}
/*********************************************************************************************************
** 函数名称: __fatFsWrite
** 功能描述: FAT FS write 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         缓冲区
**           stNBytes         需要写入的数据
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __fatFsWrite (PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer, 
                              size_t        stNBytes)
{
    PLW_FD_NODE   pfdnode    = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE     pfatfile   = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT       fresError  = FR_OK;
    ULONG         ulError    = ERROR_NONE;
    UINT          uiWriteNum = 0;

    if (!pcBuffer || !stNBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (pfatfile->FATFIL_pfatvol->FATVOL_iFlag == O_RDONLY) {
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(EROFS);
        return  (PX_ERROR);
    }
    
    if (pfatfile->FATFIL_iFileType != __FAT_FILE_TYPE_NODE) {
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (pfdentry->FDENTRY_iFlag & O_APPEND) {                           /*  追加模式                    */
        pfdentry->FDENTRY_oftPtr = pfdnode->FDNODE_oftSize;             /*  移动读写指针到末尾          */
    }
    
    if (__fatFsSeekFile(pfatfile, pfdentry->FDENTRY_oftPtr)) {          /*  调整内部文件指针            */
        __FAT_FILE_UNLOCK(pfatfile);
        return  (PX_ERROR);
    }
    
    fresError = f_write(&pfatfile->FATFIL_fftm.FFTM_file,
                        (CPVOID)pcBuffer, 
                        (UINT)stNBytes, 
                        &uiWriteNum);
    if ((fresError == FR_OK) && (uiWriteNum > 0)) {
        pfdentry->FDENTRY_oftPtr += (off_t)uiWriteNum;                  /*  更新文件指针                */
        pfdnode->FDNODE_oftSize   = (off_t)f_size(&pfatfile->FATFIL_fftm.FFTM_file);
    }                                                                   /*  更新文件大小                */
    
    ulError = __fatFsGetError(fresError);                               /*  转换错误编号                */
    
    __FAT_FILE_UNLOCK(pfatfile);
    
    if (ulError == ERROR_NONE) {
        if (pfdentry->FDENTRY_iFlag & (O_SYNC | O_DSYNC)) {             /*  需要立即同步                */
            ulError = __fatFsSync(pfdentry, LW_TRUE);
        }
    }
    
    _ErrorHandle(ulError);
    return  ((ssize_t)uiWriteNum);
}
/*********************************************************************************************************
** 函数名称: __fatFsPWrite
** 功能描述: FAT FS pwrite 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         缓冲区
**           stNBytes         需要写入的数据
**           oftPos           位置
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __fatFsPWrite (PLW_FD_ENTRY  pfdentry,
                               PCHAR         pcBuffer, 
                               size_t        stNBytes,
                               off_t         oftPos)
{
    PLW_FD_NODE   pfdnode    = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE     pfatfile   = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT       fresError  = FR_OK;
    ULONG         ulError    = ERROR_NONE;
    UINT          uiWriteNum = 0;

    if (!pcBuffer || !stNBytes || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (pfatfile->FATFIL_pfatvol->FATVOL_iFlag == O_RDONLY) {
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(EROFS);
        return  (PX_ERROR);
    }
    
    if (pfatfile->FATFIL_iFileType != __FAT_FILE_TYPE_NODE) {
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__fatFsSeekFile(pfatfile, oftPos)) {                            /*  调整内部文件指针            */
        __FAT_FILE_UNLOCK(pfatfile);
        return  (PX_ERROR);
    }
    
    fresError = f_write(&pfatfile->FATFIL_fftm.FFTM_file,
                        (CPVOID)pcBuffer, 
                        (UINT)stNBytes, 
                        &uiWriteNum);
    if ((fresError == FR_OK) && (uiWriteNum > 0)) {
        pfdnode->FDNODE_oftSize   = (off_t)f_size(&pfatfile->FATFIL_fftm.FFTM_file);
    }                                                                   /*  更新文件大小                */
    
    ulError = __fatFsGetError(fresError);                               /*  转换错误编号                */
    
    __FAT_FILE_UNLOCK(pfatfile);
    
    if (ulError == ERROR_NONE) {
        if (pfdentry->FDENTRY_iFlag & (O_SYNC | O_DSYNC)) {             /*  需要立即同步                */
            ulError = __fatFsSync(pfdentry, LW_TRUE);
        }
    }
    
    _ErrorHandle(ulError);
    return  ((ssize_t)uiWriteNum);
}
/*********************************************************************************************************
** 函数名称: __fatFsClusterSizeCal
** 功能描述: 通过磁盘的扇区数确定磁盘的簇大小
** 输　入  : pfatfile        fat文件
**           pulClusterSize  簇大小 
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fatFsClusterSizeCal (PFAT_FILE   pfatfile, ULONG  *pulClusterSize)
{
    ULONG       ulSecNum  = 0;
    ULONG       ulSecSize = 0;
    INT         iError;
    
#define __FAT_CLUSTER_CAL_BASE(ulSecSize)   ((ulSecSize / 512) * LW_CFG_KB_SIZE)
                                                                        /*  簇大小基数                  */

    iError = __blockIoDevIoctl(pfatfile->FATFIL_pfatvol->FATVOL_iDrv,
                               LW_BLKD_GET_SECNUM,
                               (LONG)&ulSecNum);                        /*  获得设备扇区数量            */
    if (iError < 0) {
        _ErrorHandle(ERROR_IO_DEVICE_ERROR);
        return  (PX_ERROR);
    }
    iError = __blockIoDevIoctl(pfatfile->FATFIL_pfatvol->FATVOL_iDrv,
                               LW_BLKD_GET_SECSIZE,
                               (LONG)&ulSecSize);                       /*  获得磁盘扇区大小            */
    if ((iError < 0) || (ulSecSize < 512)) {
        _ErrorHandle(ERROR_IO_DEVICE_ERROR);
        return  (PX_ERROR);
    }
    
    if (ulSecNum >= 0x800000) {                                         /*  4 GB 以上磁盘               */
        *pulClusterSize = 8 * __FAT_CLUSTER_CAL_BASE(ulSecSize);
    
    } else if (ulSecNum >= 0x200000) {                                  /*  1 GB 以上磁盘               */
        *pulClusterSize = 4 * __FAT_CLUSTER_CAL_BASE(ulSecSize);
    
    } else if (ulSecNum >= 0x100000) {                                  /*  512 MB 以上磁盘             */
        *pulClusterSize = 2 * __FAT_CLUSTER_CAL_BASE(ulSecSize);
    
    } else {
        *pulClusterSize = 1 * __FAT_CLUSTER_CAL_BASE(ulSecSize);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __fatFsFormat
** 功能描述: FAT FS 格式化媒质
** 输　入  : pfdentry            文件控制块
**           lArg                参数
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fatFsFormat (PLW_FD_ENTRY  pfdentry, LONG  lArg)
{
    PLW_FD_NODE pfdnode   = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE   pfatfile  = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    ULONG       ulClusterSize;
    FRESULT     fresError;
    INT         iError;
    ULONG       ulError;
    
    if (!__STR_IS_ROOT(pfatfile->FATFIL_cName)) {                       /*  检查是否为设备文件          */
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {           /*  是否需要提前关闭文件        */
        pfatfile->FATFIL_iFileType =  __FAT_FILE_TYPE_DEV;              /*  不需要再次关闭              */
        f_close(&pfatfile->FATFIL_fftm.FFTM_file);
    }
    
    iError = __fatFsClusterSizeCal(pfatfile, &ulClusterSize);           /*  获得簇大小                  */
    if (iError < 0) {
        __FAT_FILE_UNLOCK(pfatfile);
        return  (PX_ERROR);
    }
    
    if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)pfatfile->FATFIL_pfatvol)
        > 1) {                                                          /*  检查是否有正在工作的文件    */
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(ERROR_IOS_TOO_MANY_OPEN_FILES);                    /*  有其他文件打开              */
        return  (PX_ERROR);
    }
    
    (VOID)__blockIoDevIoctl(pfatfile->FATFIL_pfatvol->FATVOL_iDrv,
                            FIOCANCEL, 0);                              /*  CACHE 停止 (不回写数据)     */
    
    iError = __blockIoDevIoctl(pfatfile->FATFIL_pfatvol->FATVOL_iDrv,
                               FIODISKFORMAT,
                               lArg);                                   /*  底层格式化                  */
    if (iError < 0) {
        PVOID  pvWork = __SHEAP_ALLOC(_MAX_SS);                         /*  分配缓冲区                  */
        
        if (pvWork == LW_NULL) {
            __FAT_FILE_UNLOCK(pfatfile);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        
        if (__blockIoDevIsLogic(pfatfile->FATFIL_pfatvol->FATVOL_iDrv)) {
            fresError = f_mkfs((BYTE)pfatfile->FATFIL_pfatvol->FATVOL_iDrv, 
                               (BYTE)(FM_SFD | FM_FAT | FM_FAT32),
                               (UINT16)ulClusterSize,
                               pvWork, _MAX_SS);                        /*  此磁盘为逻辑磁盘不需要分区表*/
        } else {
            fresError = f_mkfs((BYTE)pfatfile->FATFIL_pfatvol->FATVOL_iDrv, 
                               (BYTE)(FM_FAT | FM_FAT32),
                               (UINT16)ulClusterSize, 
                               pvWork, _MAX_SS);                        /*  格式化带有分区表            */
        }
        
        __SHEAP_FREE(pvWork);                                           /*  释放缓冲区                  */
        
        ulError = __fatFsGetError(fresError);                           /*  转换错误编号                */
    
    } else {
        ulError = ERROR_NONE;
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    if (ulError) {                                                      /*  返回                        */
        return  (PX_ERROR);
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __fatFsRename
** 功能描述: FAT FS 更名
** 输　入  : pfdentry            文件控制块
**           pcNewName           新名字
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
** 注  意  : 为了减小内存分片的产生, 将文件名和控制结构分配在同一内存分段, 这时无法更改 pfatfile 保存的
             文件名, 所以文件名完全错误, 这时, 此文件不能再次操作, 所以, 用户必须调用 rename() 函数来完成
             此操作, 不能独立调用 ioctl() 来操作.
*********************************************************************************************************/
static INT  __fatFsRename (PLW_FD_ENTRY  pfdentry, PCHAR  pcNewName)
{
    REGISTER FRESULT        fresError;
    REGISTER INT            iError  = ERROR_NONE;
    REGISTER ULONG          ulError = ERROR_NONE;
    
             CHAR           cNewPath[PATH_MAX + 1];
    REGISTER PCHAR          pcNewPath = &cNewPath[0];
             PLW_FD_NODE    pfdnode   = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
             PFAT_FILE      pfatfile  = (PFAT_FILE)pfdnode->FDNODE_pvFile;
             PFAT_VOLUME    pfatvolNew;
             FILINFO        fileinfo;

    if (__STR_IS_ROOT(pfatfile->FATFIL_cName)) {                        /*  检查是否为设备文件          */
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                         /*  不支持设备重命名            */
        return  (PX_ERROR);
    }
    if (pcNewName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    if (__STR_IS_ROOT(pcNewName)) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_DEV) {            /*  设备本身不能改名            */
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    if (ioFullFileNameGet(pcNewName, 
                          (LW_DEV_HDR **)&pfatvolNew, 
                          cNewPath) != ERROR_NONE) {                    /*  获得新目录路径              */
        __FAT_FILE_UNLOCK(pfatfile);
        return  (PX_ERROR);
    }
    
    if (pfatvolNew != pfatfile->FATFIL_pfatvol) {                       /*  必须为同一个卷              */
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    
    if (cNewPath[0] == PX_DIVIDER) {                                    /*  FatFs 文件系统 rename 的第  */
        pcNewPath++;                                                    /*  2 个参数不能以'/'为起始字符 */
    }
    
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {
        f_sync(&pfatfile->FATFIL_fftm.FFTM_file);

    } else if (_PathMoveCheck(pfatfile->FATFIL_cName, pcNewPath)) {     /*  如果是目录项则检查          */
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    fresError = f_rename_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                            pfatfile->FATFIL_cName, pcNewPath);         /*  开始重命名操作              */
    if (fresError == FR_EXIST) {
        if (f_stat_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                      pcNewPath, &fileinfo)) {
            __FAT_FILE_UNLOCK(pfatfile);
            _ErrorHandle(EIO);
            return  (PX_ERROR);
        }
        if ((pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) &&
            (fileinfo.fattrib & AM_DIR)) {
            __FAT_FILE_UNLOCK(pfatfile);
            _ErrorHandle(EISDIR);
            return  (PX_ERROR);
        }
        if ((pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_DIR) &&
            !(fileinfo.fattrib & AM_DIR)) {
            __FAT_FILE_UNLOCK(pfatfile);
            _ErrorHandle(ENOTDIR);
            return  (PX_ERROR);
        }
        
        fresError = f_unlink_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                                pcNewPath);                             /*  删除目标                    */
        if (fresError == FR_DENIED) {
            if (fileinfo.fattrib & AM_RDO) {
                ulError = EROFS;
            } else {
                ulError = ENOTEMPTY;
            }
            __FAT_FILE_UNLOCK(pfatfile);
            _ErrorHandle(ulError);
            return  (PX_ERROR);
        }
        
        fresError = f_rename_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                                pfatfile->FATFIL_cName, pcNewPath);     /*  重新重命名                  */
    }
    
    if (fresError) {
        ulError = __fatFsGetError(fresError);
        iError  = PX_ERROR;
    
    } else {
        iError  = ERROR_NONE;
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __fatFsSeek
** 功能描述: FAT FS 文件定位
** 输　入  : pfdentry            文件控制块
**           oftPos              定位点
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fatFsSeek (PLW_FD_ENTRY  pfdentry, off_t  oftPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    if (pfatfile->FATFIL_pfatvol->FATVOL_iFlag == O_RDONLY) {
        if (oftPos > pfdnode->FDNODE_oftSize) {                         /*  只读文件系统不能扩大文件    */
            __FAT_FILE_UNLOCK(pfatfile);
            _ErrorHandle(EROFS);
            return  (PX_ERROR);
        }
    }
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {
        if (oftPos < 0) {
            ulError = EOVERFLOW;
            iError  = PX_ERROR;
        
        } else {
            pfdentry->FDENTRY_oftPtr = oftPos;
        }
    
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __fatFsWhere
** 功能描述: FAT FS 获得文件当前读写指针位置 (使用参数作为返回值, 与 FIOWHERE 的要求稍有不同)
** 输　入  : pfdentry            文件控制块
**           poftPos             读写指针位置
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fatFsWhere (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {
        *poftPos = pfdentry->FDENTRY_oftPtr;
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __fatFsNRead
** 功能描述: FAT FS 获得文件剩余的信息量
** 输　入  : pfdentry            文件控制块
**           piPos               剩余数据量
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fatFsNRead (PLW_FD_ENTRY  pfdentry, INT  *piPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    
    if (piPos == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {
        *piPos = (INT)(pfdnode->FDNODE_oftSize - pfdentry->FDENTRY_oftPtr);
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __fatFsNRead64
** 功能描述: FAT FS 获得文件剩余的信息量
** 输　入  : pfdentry            文件控制块
**           poftPos             剩余数据量
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fatFsNRead64 (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {
        *poftPos = pfdnode->FDNODE_oftSize - pfdentry->FDENTRY_oftPtr;
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __fatFsVolLabel
** 功能描述: FAT FS 文件系统卷标处理函数
** 输　入  : pfdentry            文件控制块
**           pcLabel             卷标缓存
**           bSet                设置还是获取
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fatFsVolLabel (PLW_FD_ENTRY  pfdentry, PCHAR  pcLabel, BOOL  bSet)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT        fresError;
    ULONG          ulError;
    
    if (pcLabel == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    if (bSet) {
        fresError = f_setlabel_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol, pcLabel);
    } else {
        fresError = f_getlabel_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                                  "", pcLabel, LW_NULL);
    }
    ulError = __fatFsGetError(fresError);                               /*  转换错误编号                */
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    
    if (ulError) {
        return  (PX_ERROR);
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __fatFsStatGet
** 功能描述: FAT FS 获得文件状态及属性
** 输　入  : pfdentry            文件控制块
**           pstat               stat 结构
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fatFsStatGet (PLW_FD_ENTRY  pfdentry, struct stat *pstat)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT        fresError;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    FILINFO        fileinfo;

    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    lib_bzero(&fileinfo, sizeof(FILINFO));                              /*  清零                        */
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    fresError = f_stat_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                          (CPCHAR)pfatfile->FATFIL_cName,
                          &fileinfo);
    ulError = __fatFsGetError(fresError);                               /*  转换错误编号                */
    if (ulError != ERROR_NONE) {
        if (__STR_IS_ROOT(pfatfile->FATFIL_cName)) {                    /*  为根目录                    */
            fileinfo.fsize    = 0;
            /*
             *  记录卷的创建时间. FATVOL_uiTime 为 FAT 时间格式
             *  fdate 为 FATVOL_uiTime 高 16 位, ftime 为低 16 位.
             *  (FATVOL_uiTime & 0xFFFF) 更容易理解.
             */
            fileinfo.fcdate   = (WORD)(pfatfile->FATFIL_pfatvol->FATVOL_uiTime >> 16);
            fileinfo.fctime   = (WORD)(pfatfile->FATFIL_pfatvol->FATVOL_uiTime & 0xFFFF);
            fileinfo.fdate    = fileinfo.fcdate;
            fileinfo.ftime    = fileinfo.fctime;
            fileinfo.fattrib  = AM_DIR;
            fileinfo.fname[0] = PX_ROOT;
            fileinfo.fname[1] = PX_EOS;
            ulError           = ERROR_NONE;
        } else {
            iError = PX_ERROR;
        }
    }
    
    if (iError == ERROR_NONE) {                                         /*  转换为 POSIX 标准结构       */
        fileinfo.fsize = pfdnode->FDNODE_oftSize;
        __filInfoToStat(&fileinfo, 
                        &pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                        pstat,
                        pfatfile->FATFIL_inode);
        pstat->st_uid = _G_uidFatDefault;
        pstat->st_gid = _G_gidFatDefault;
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __fatFsStatfsGet
** 功能描述: FAT FS 获得文件系统状态及属性
** 输　入  : pfdentry            文件控制块
**           pstatfs             statfs 结构
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fatFsStatfsGet (PLW_FD_ENTRY  pfdentry, struct statfs *pstatfs)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    
    if (!pstatfs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    if (__fsInfoToStatfs(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                         pfatfile->FATFIL_pfatvol->FATVOL_iFlag,
                         pstatfs,
                         pfatfile->FATFIL_pfatvol->FATVOL_iDrv) < 0) {
        iError = PX_ERROR;
    
    } else {
        if (pfatfile->FATFIL_pfatvol->FATVOL_iFlag == O_RDONLY) {
            pstatfs->f_flag |= ST_RDONLY;
        }
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __fatFsReadDir
** 功能描述: FAT FS 获得指定目录信息
** 输　入  : pfdentry            文件控制块
**           dir                 目录结构
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fatFsReadDir (PLW_FD_ENTRY  pfdentry, DIR  *dir)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT        fresError;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
             
    mode_t         mode;
    FILINFO        fileinfo;

    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_DEV) {            /*  根目录方式                  */
        f_opendir_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol, 
                     &pfatfile->FATFIL_fftm.FFTM_fatdir,
                     &pfatfile->FATFIL_u64Uniq,
                     PX_STR_ROOT);                                      /*  打开目录                    */
        pfatfile->FATFIL_iFileType =  __FAT_FILE_TYPE_DIR;              /*  转换为目录形式              */
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    if (pfatfile->FATFIL_iFileType != __FAT_FILE_TYPE_DIR) {
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    pfatfile->FATFIL_fftm.FFTM_fatdir.dptr = (UINT16)dir->dir_pos;
    if (dir->dir_pos == 0) {
        f_readdir(&pfatfile->FATFIL_fftm.FFTM_fatdir, LW_NULL);         /*  rewind dir                  */
    }
    
    fresError = f_readdir(&pfatfile->FATFIL_fftm.FFTM_fatdir,
                          &fileinfo);
    ulError = __fatFsGetError(fresError);                               /*  转换错误编号                */
    if (ulError != ERROR_NONE) {                                        /*  发生错误                    */
        iError  = PX_ERROR;
    
    } else if (fileinfo.fname[0] == PX_EOS) {                           /*  目录结束                    */
        ulError = ENOENT;
        iError  = PX_ERROR;
    
    } else {
        dir->dir_pos = pfatfile->FATFIL_fftm.FFTM_fatdir.dptr;          /*  记录下次地点                */
        
        /*
         *  拷贝文件名
         */
        lib_strcpy(dir->dir_dirent.d_shortname, fileinfo.altname);
        if (fileinfo.fname[0]) {
            lib_strcpy(dir->dir_dirent.d_name, fileinfo.fname);         /*  完整文件名                  */
        } else {
            lib_strcpy(dir->dir_dirent.d_name, fileinfo.altname);       /*  完整文件名                  */
        }
        
        /*
         *  保存文件类型
         */
        mode = __fsAttrToMode(fileinfo.fattrib);
        dir->dir_dirent.d_type = IFTODT(mode);                          /*  转换为 d_type               */
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __fatFsTimeset
** 功能描述: FAT FS 设置文件时间
** 输　入  : pfdentry            文件控制块
**           utim                utimbuf 结构
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
** 注  意  : 目前此函数仅设置修改时间.
*********************************************************************************************************/
static INT  __fatFsTimeset (PLW_FD_ENTRY  pfdentry, struct utimbuf  *utim)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT        fresError;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    UINT32         dwTime   = __timeToFatTime(&utim->modtime);
    FILINFO        fileinfo;

    if (__STR_IS_ROOT(pfatfile->FATFIL_cName)) {                        /*  检查是否为设备文件          */
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                         /*  不支持设备重命名            */
        return  (PX_ERROR);
    }
    
    fileinfo.fdate = (UINT16)(dwTime >> 16);
    fileinfo.ftime = (UINT16)(dwTime & 0xFFFF);
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    fresError = f_utime_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                           (CPCHAR)pfatfile->FATFIL_cName,
                           &fileinfo);
    ulError = __fatFsGetError(fresError);                               /*  转换错误编号                */
    if (ulError != ERROR_NONE) {
        iError  = PX_ERROR;
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __fatFsFillZero
** 功能描述: FAT FS 添加 0.
** 输　入  : pfatfile            FAT 文件控制块
**           oftSize             文件大小
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __fatFsFillZero (PFAT_FILE  pfatfile, off_t  oftSize)
{
    static const CHAR     cZero[2048];                                  /*  创建 C 环境时清零           */
    
    REGISTER INT          i;
    REGISTER INT          iTimes  = (INT)(oftSize >> 11);
    REGISTER UINT         uiSpace = (INT)(oftSize &  2047);
             UINT         uiWrNum;
    
    for (i = 0; i < iTimes; i++) {
        f_write(&pfatfile->FATFIL_fftm.FFTM_file, cZero, sizeof(cZero), &uiWrNum);
    }
    
    if (uiSpace) {
        f_write(&pfatfile->FATFIL_fftm.FFTM_file, cZero, uiSpace, &uiWrNum);
    }
}
/*********************************************************************************************************
** 函数名称: __fatFsTruncate
** 功能描述: FAT FS 设置文件大小
** 输　入  : pfdentry            文件控制块
**           oftSize             文件大小
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fatFsTruncate (PLW_FD_ENTRY  pfdentry, off_t  oftSize)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT        fresError;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    
    off_t          oftOldSize;
    off_t          oftNewSize;
             
    if ((oftSize < 0) || (oftSize > (off_t)((DWORD)(~0)))) {            /*  FAT 文件必须在 4GB 内       */
        _ErrorHandle(EOVERFLOW);
        return  (PX_ERROR);
    
    } else {
        oftNewSize = oftSize;
    }

    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {
        oftOldSize = f_size(&pfatfile->FATFIL_fftm.FFTM_file);          /*  获得当前文件大小            */
        
        if (oftSize > pfdnode->FDNODE_oftSize) {                        /*  需要放大文件                */
            fresError = f_lseek(&pfatfile->FATFIL_fftm.FFTM_file, oftNewSize);
            if (fresError == FR_OK) {                                   /*  POSIX 规定新空间必须填充 0  */
                f_lseek(&pfatfile->FATFIL_fftm.FFTM_file, oftOldSize);  /*  文件指针移动到 old size 处  */
                __fatFsFillZero(pfatfile, oftNewSize - oftOldSize);
            }
        
        } else if (oftSize < pfdnode->FDNODE_oftSize) {                 /*  缩小文件                    */
            fresError = f_lseek(&pfatfile->FATFIL_fftm.FFTM_file, oftNewSize);
            if (fresError == FR_OK) {                                   /*  从指定位置截断文件          */
                fresError = f_truncate(&pfatfile->FATFIL_fftm.FFTM_file);
            }
        
        } else {                                                        /*  没有变化                    */
            fresError = FR_OK;
        }
                                                                        /*  更新文件大小                */
        pfdnode->FDNODE_oftSize = f_size(&pfatfile->FATFIL_fftm.FFTM_file);
        
        ulError = __fatFsGetError(fresError);                           /*  转换错误编号                */
    
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __fatFsSync
** 功能描述: FAT FS 将文件缓存写入磁盘
** 输　入  : pfdentry            文件控制块
**           bFlushCache         是否同时清空 CACHE
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fatFsSync (PLW_FD_ENTRY  pfdentry, BOOL  bFlushCache)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT        fresError;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {
        fresError = f_sync(&pfatfile->FATFIL_fftm.FFTM_file);           /*  清空缓存                    */
    } else {                                                            /*  将文件系统内缓存数据回写    */
        fresError = f_syncfs(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol);
    }
    ulError = __fatFsGetError(fresError);                               /*  转换错误编号                */
    
    if (bFlushCache) {
        iError = __blockIoDevIoctl(pfatfile->FATFIL_pfatvol->FATVOL_iDrv,
                                   FIOSYNC, 0);                         /*  清除 CACHE 回写磁盘         */
        if (iError < 0) {
            ulError = ERROR_IO_DEVICE_ERROR;                            /*  设备出错, 无法清空          */
        }
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __fatFsSync
** 功能描述: FAT FS 将文件缓存写入磁盘
** 输　入  : pfdentry            文件控制块
**           iMode               新的 mode
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fatFsChmod (PLW_FD_ENTRY  pfdentry, INT  iMode)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT        fresError;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    BYTE           ucAttr;
    
    iMode |= S_IRUSR;
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    if ((pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) ||
        (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_DIR)) {
        ucAttr = __fsModeToAttr(iMode);
        fresError = f_chmod_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol, 
                               pfatfile->FATFIL_cName, 
                               ucAttr, (BYTE)0xFF);
        ulError = __fatFsGetError(fresError);                           /*  转换错误编号                */
    } else {
        ulError = ENOSYS;
        iError  = PX_ERROR;
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __fatFsIoctl
** 功能描述: FAT FS ioctl 操作
** 输　入  : pfdentry            文件控制块
**           request,            命令
**           arg                 命令参数
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fatFsIoctl (PLW_FD_ENTRY  pfdentry,
                          INT           iRequest,
                          LONG          lArg)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    INT            iError;
    off_t          oftTemp  = 0;                                        /*  临时变量                    */
    
    switch (iRequest) {                                                 /*  只读文件判断                */
    
    case FIOCONTIG:
    case FIOTRUNC:
    case FIOLABELSET:
    case FIOATTRIBSET:
    case FIOSQUEEZE:
    case FIODISKFORMAT:
        if ((pfdentry->FDENTRY_iFlag & O_ACCMODE) == O_RDONLY) {
            _ErrorHandle(ERROR_IO_WRITE_PROTECTED);
            return  (PX_ERROR);
        }
        if (pfatfile->FATFIL_pfatvol->FATVOL_iFlag == O_RDONLY) {
            _ErrorHandle(EROFS);
            return  (PX_ERROR);
        }
	}
	
	switch (iRequest) {                                                 /*  只读文件系统判断            */
    
    case FIORENAME:
    case FIOTIMESET:
    case FIOCHMOD:
        if (pfatfile->FATFIL_pfatvol->FATVOL_iFlag == O_RDONLY) {
            _ErrorHandle(EROFS);
            return  (PX_ERROR);
        }
    }

    switch (iRequest) {
    
    case FIODISKFORMAT:                                                 /*  卷格式化                    */
        return  (__fatFsFormat(pfdentry, lArg));
    
    case FIODISKINIT:                                                   /*  磁盘初始化                  */
        return  (__blockIoDevIoctl(pfatfile->FATFIL_pfatvol->FATVOL_iDrv,
                                   FIODISKINIT, lArg));
    
    case FIORENAME:                                                     /*  文件重命名                  */
        return  (__fatFsRename(pfdentry, (PCHAR)lArg));
    
    /*
     *  FIOSEEK, FIOWHERE is 64 bit operate.
     */
    case FIOSEEK:                                                       /*  文件重定位                  */
        oftTemp = *(off_t *)lArg;
        return  (__fatFsSeek(pfdentry, oftTemp));
    
    case FIOWHERE:                                                      /*  获得文件当前读写指针        */
        iError = __fatFsWhere(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }
    
    case FIONREAD:                                                      /*  获得文件剩余字节数          */
        return  (__fatFsNRead(pfdentry, (INT *)lArg));
    
    case FIONREAD64:                                                    /*  获得文件剩余字节数          */
        iError = __fatFsNRead64(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }
    
    case FIOLABELGET:                                                   /*  获取卷标                    */
        return  (__fatFsVolLabel(pfdentry, (PCHAR)lArg, LW_FALSE));
    
    case FIOLABELSET:                                                   /*  设置卷标                    */
        return  (__fatFsVolLabel(pfdentry, (PCHAR)lArg, LW_TRUE));
    
    case FIOFSTATGET:                                                   /*  获得文件状态                */
        return  (__fatFsStatGet(pfdentry, (struct stat *)lArg));
    
    case FIOFSTATFSGET:                                                 /*  获得文件系统状态            */
        return  (__fatFsStatfsGet(pfdentry, (struct statfs *)lArg));
        
    case FIOREADDIR:                                                    /*  获取一个目录信息            */
        return  (__fatFsReadDir(pfdentry, (DIR *)lArg));
    
    case FIOTIMESET:                                                    /*  设置文件时间                */
        return  (__fatFsTimeset(pfdentry, (struct utimbuf *)lArg));

    /*
     *  FIOTRUNC is 64 bit operate.
     */
    case FIOTRUNC:                                                      /*  改变文件大小                */
        oftTemp = *(off_t *)lArg;
        return  (__fatFsTruncate(pfdentry, oftTemp));
        
    case FIOSYNC:                                                       /*  将文件缓存回写              */
    case FIODATASYNC:
        return  (__fatFsSync(pfdentry, LW_TRUE /*  LW_FALSE  ???*/));
        
    case FIOFLUSH:
        return  (__fatFsSync(pfdentry, LW_TRUE));                       /*  文件与缓存全部回写          */
        
    case FIOCHMOD:
        return  (__fatFsChmod(pfdentry, (INT)lArg));                    /*  改变文件访问权限            */
        
    case FIOSETFL:                                                      /*  设置新的 flag               */
        if ((INT)lArg & O_NONBLOCK) {
            pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);
        
    case FIOFSTYPE:                                                     /*  获得文件系统类型            */
        switch (pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol.fs_type) {
        
        case FS_FAT12:
            *(PCHAR *)lArg = _G_pcFat12FsString;
            break;
            
        case FS_FAT16:
            *(PCHAR *)lArg = _G_pcFat16FsString;
            break;
            
        case FS_FAT32:
            *(PCHAR *)lArg = _G_pcFat32FsString;
            break;
            
        case FS_EXFAT:
            *(PCHAR *)lArg = _G_pcExFatFsString;
            break;
            
        default:
            *(PCHAR *)lArg = "Unknown FAT Type";
        }
        return  (ERROR_NONE);
        
    case FIOGETFORCEDEL:                                                /*  强制卸载设备是否被允许      */
        *(BOOL *)lArg = pfatfile->FATFIL_pfatvol->FATVOL_bForceDelete;
        return  (ERROR_NONE);
        
    case FIOSETFORCEDEL:                                                /*  设置强制卸载使能            */
        pfatfile->FATFIL_pfatvol->FATVOL_bForceDelete = (BOOL)lArg;
        return  (ERROR_NONE);
        
    default:                                                            /*  无法识别的命令              */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    return  (PX_ERROR);
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_FATFS_EN > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
