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
** 文   件   名: memdev.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 06 月 04 日
**
** 描        述: VxWorks 内存设备兼容接口.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_MEMDEV_EN > 0)
/*********************************************************************************************************
  设备结构
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR          MEM_devhdr;
    MEM_DRV_DIRENTRY    MEM_memdir;
    INT                 MEM_iAllowOft;                                  /*  是否允许设置偏移量          */
    time_t              MEM_time;
    uid_t               MEM_uid;
    gid_t               MEM_gid;
} LW_MEM_DEV;
typedef LW_MEM_DEV     *PLW_MEM_DEV;

typedef struct {
    PLW_MEM_DEV         MEMF_memdev;
    MEM_DRV_DIRENTRY   *MEMF_pmemdir;
    size_t              MEMF_stOffset;                                  /*  当前文件偏移量              */
} LW_MEM_FILE;
typedef LW_MEM_FILE    *PLW_MEM_FILE;
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static INT      _G_iMemDrvNum = PX_ERROR;
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
static LONG     _MemOpen(PLW_MEM_DEV   pmemdev, 
                         PCHAR         pcName,   
                         INT           iFlags, 
                         INT           iMode);
static INT      _MemClose(PLW_MEM_FILE  pmemfile);
static ssize_t  _MemRead(PLW_MEM_FILE  pmemfile, 
                         PCHAR         pcBuffer, 
                         size_t        stMaxBytes);
static ssize_t  _MemPRead(PLW_MEM_FILE  pmemfile, 
                          PCHAR         pcBuffer, 
                          size_t        stMaxBytes,
                          off_t         oftOffset);
static ssize_t  _MemWrite(PLW_MEM_FILE  pmemfile, 
                          PCHAR         pcBuffer, 
                          size_t        stNBytes);
static ssize_t  _MemPWrite(PLW_MEM_FILE  pmemfile, 
                           PCHAR         pcBuffer, 
                           size_t        stNBytes,
                           off_t         oftOffset);
static INT      _MemIoctl(PLW_MEM_FILE  pmemfile,
                          INT           iRequest,
                          LONG          lArg);
/*********************************************************************************************************
** 函数名称: API_MemDrvInstall
** 功能描述: 安装内存设备驱动程序
** 输　入  : VOID
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_MemDrvInstall (VOID)
{
    struct file_operations     fileop;
    
    if (_G_iMemDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));

    fileop.owner       = THIS_MODULE;
    fileop.fo_open     = _MemOpen;
    fileop.fo_close    = _MemClose;
    fileop.fo_read     = _MemRead;
    fileop.fo_read_ex  = _MemPRead;
    fileop.fo_write    = _MemWrite;
    fileop.fo_write_ex = _MemPWrite;
    fileop.fo_ioctl    = _MemIoctl;

    _G_iMemDrvNum =  iosDrvInstallEx(&fileop);
    
    DRIVER_LICENSE(_G_iMemDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iMemDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iMemDrvNum, "VxWorks memory device driver.");
    
    return  ((_G_iMemDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** 函数名称: API_MemDevCreate
** 功能描述: 创建一个内存设备
** 输　入  : name      设备名
**           base      内存基地址
**           length    内存长度
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_MemDevCreate (char *name, char *base, size_t length)
{
    PLW_MEM_DEV  pmemdev;
    
    if (!name || !length) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pmemdev = (PLW_MEM_DEV)__SHEAP_ALLOC(sizeof(LW_MEM_DEV));
    if (pmemdev == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pmemdev, sizeof(LW_MEM_DEV));
    
    pmemdev->MEM_memdir.name   = "";
    pmemdev->MEM_memdir.base   = base;
    pmemdev->MEM_memdir.pDir   = LW_NULL;
    pmemdev->MEM_memdir.length = length;
    pmemdev->MEM_iAllowOft     = 1;
    
    pmemdev->MEM_uid = getuid();
    pmemdev->MEM_gid = getgid();
    
    if (iosDevAddEx(&pmemdev->MEM_devhdr, name, _G_iMemDrvNum, DT_REG) != ERROR_NONE) {
        __SHEAP_FREE(pmemdev);
        return  (PX_ERROR);
    }
    
    pmemdev->MEM_time = lib_time(LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_MemDevCreateDir
** 功能描述: 创建一个内存设备目录
** 输　入  : name      设备名
**           files     内存文件列表
**           numFiles  文件数量
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_MemDevCreateDir (char *name, MEM_DRV_DIRENTRY *files, int numFiles)
{
    PLW_MEM_DEV  pmemdev;
    
    if (!name || !files || (numFiles < 1)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pmemdev = (PLW_MEM_DEV)__SHEAP_ALLOC(sizeof(LW_MEM_DEV));
    if (pmemdev == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pmemdev, sizeof(LW_MEM_DEV));
    
    pmemdev->MEM_memdir.name   = "";
    pmemdev->MEM_memdir.base   = LW_NULL;
    pmemdev->MEM_memdir.pDir   = files;
    pmemdev->MEM_memdir.length = (size_t)numFiles;
    pmemdev->MEM_iAllowOft     = 0;
    
    if (iosDevAddEx(&pmemdev->MEM_devhdr, name, _G_iMemDrvNum, DT_DIR) != ERROR_NONE) {
        __SHEAP_FREE(pmemdev);
        return  (PX_ERROR);
    }
    
    pmemdev->MEM_time = lib_time(LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_MemDevDelete
** 功能描述: 删除一个内存设备
** 输　入  : name      设备名
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_MemDevDelete (char *name)
{
    PLW_MEM_DEV   pmemdev;
    PCHAR         pcTail = LW_NULL;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    pmemdev = (PLW_MEM_DEV)iosDevFind(name, &pcTail);
    if ((pmemdev == LW_NULL) || (name == pcTail)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device not found.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    
    if (LW_DEV_GET_USE_COUNT(&pmemdev->MEM_devhdr)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "too many open files.\r\n");
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }
    
    iosDevFileAbnormal(&pmemdev->MEM_devhdr);
    iosDevDelete(&pmemdev->MEM_devhdr);
    
    __SHEAP_FREE(pmemdev);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _MemFind
** 功能描述: 查找一个内存文件
** 输　入  : pmemdev          内存设备控制块
**           pcName           名称
**           pmemdir          查找目录
**           pstFileOft       文件偏移量
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static MEM_DRV_DIRENTRY *_MemFind (PLW_MEM_DEV          pmemdev, 
                                   PCHAR                pcName, 
                                   MEM_DRV_DIRENTRY    *pmemdir, 
                                   size_t              *pstFileOft)
{
    MEM_DRV_DIRENTRY  *pmemdirFile = LW_NULL;
    
    *pstFileOft = 0;
    
    if (*pcName == PX_ROOT) {
        pcName++;
    }
    
    if (lib_strcmp(pmemdir->name, pcName) == 0) {
        pmemdirFile = pmemdir;
    
    } else if (lib_strncmp(pmemdir->name, pcName, lib_strlen(pmemdir->name)) == 0) {
        INT  i;
        pcName += lib_strlen(pmemdir->name);
        if (pmemdir->pDir != LW_NULL) {                                 /*  查找子目录                  */
	        for (i = 0; i < pmemdir->length; i++) {
	            pmemdirFile = _MemFind(pmemdev, pcName, &pmemdir->pDir[i], pstFileOft);
	            if (pmemdirFile) {
	                break;
	            }
	        }
        }
        
    } else if (pmemdev->MEM_iAllowOft) {
        size_t  stOff = 0;
        if (*pcName == PX_ROOT) {
            pcName++;
        }
        if (sscanf(pcName, "%zu", &stOff) == 1) {
            pmemdirFile = pmemdir;
            *pstFileOft = stOff;
        }
    }
    
    return  (pmemdirFile);
}
/*********************************************************************************************************
** 函数名称: _MemOpen
** 功能描述: 打开内存设备
** 输　入  : pmemdev          内存设备控制块
**           pcName           名称
**           iFlags           方式
**           iMode            方法
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  _MemOpen (PLW_MEM_DEV   pmemdev, 
                       PCHAR         pcName,   
                       INT           iFlags, 
                       INT           iMode)
{
    MEM_DRV_DIRENTRY  *pmemdirFile;
    PLW_MEM_FILE       pmemfile;
    size_t             stOft;
    
    if (iFlags & O_CREAT) {
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    pmemdirFile = _MemFind(pmemdev, pcName, &pmemdev->MEM_memdir, &stOft);
    if (pmemdirFile == LW_NULL) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    pmemfile = (PLW_MEM_FILE)__SHEAP_ALLOC(sizeof(LW_MEM_FILE));
    if (pmemfile == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    
    pmemfile->MEMF_memdev   = pmemdev;
    pmemfile->MEMF_pmemdir  = pmemdirFile;
    pmemfile->MEMF_stOffset = stOft;
    
    LW_DEV_INC_USE_COUNT(&pmemdev->MEM_devhdr);
    
    return  ((LONG)pmemfile);
}
/*********************************************************************************************************
** 函数名称: _MemClose
** 功能描述: 关闭内存设备
** 输　入  : pmemfile          内存设备文件
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _MemClose (PLW_MEM_FILE  pmemfile)
{
    if (pmemfile) {
        LW_DEV_DEC_USE_COUNT(&pmemfile->MEMF_memdev->MEM_devhdr);
        __SHEAP_FREE(pmemfile);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _MemRead
** 功能描述: 读内存文件
** 输　入  : pmemfile         内存设备文件
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  _MemRead (PLW_MEM_FILE  pmemfile, 
                          PCHAR         pcBuffer, 
                          size_t        stMaxBytes)
{
    MEM_DRV_DIRENTRY  *pmemdirFile;
    size_t             stCopeBytes;
    
    if (!pcBuffer || !stMaxBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pmemdirFile = pmemfile->MEMF_pmemdir;
    if (pmemdirFile->pDir) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (pmemfile->MEMF_stOffset >= pmemdirFile->length) {
        return  (0);
    }
    
    stCopeBytes = __MIN(stMaxBytes, pmemdirFile->length - pmemfile->MEMF_stOffset);
    lib_memcpy(pcBuffer, (CPVOID)(pmemdirFile->base + pmemfile->MEMF_stOffset), stCopeBytes);
    
    pmemfile->MEMF_stOffset += stCopeBytes;
    
    return  ((size_t)stCopeBytes);
}
/*********************************************************************************************************
** 函数名称: _MemPRead
** 功能描述: 扩展读内存文件
** 输　入  : pmemfile         内存设备文件
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
**           oftOffset        文件偏移量
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  _MemPRead (PLW_MEM_FILE  pmemfile, 
                           PCHAR         pcBuffer, 
                           size_t        stMaxBytes,
                           off_t         oftOffset)
{
    MEM_DRV_DIRENTRY  *pmemdirFile;
    size_t             stCopeBytes;
    
    if (!pcBuffer || !stMaxBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pmemdirFile = pmemfile->MEMF_pmemdir;
    if (pmemdirFile->pDir) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (oftOffset >= pmemdirFile->length) {
        return  (0);
    }
    
    stCopeBytes = __MIN(stMaxBytes, pmemdirFile->length - (size_t)oftOffset);
    lib_memcpy(pcBuffer, (CPVOID)(pmemdirFile->base + (size_t)oftOffset), stCopeBytes);
    
    return  ((size_t)stCopeBytes);
}
/*********************************************************************************************************
** 函数名称: _MemWrite
** 功能描述: 写内存文件
** 输　入  : pmemfile         内存设备文件
**           pcBuffer         发送缓冲区
**           stNBytes         写入数目
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  _MemWrite (PLW_MEM_FILE  pmemfile, 
                           PCHAR         pcBuffer, 
                           size_t        stNBytes)
{
    MEM_DRV_DIRENTRY  *pmemdirFile;
    size_t             stCopeBytes;
    
    if (!pcBuffer || !stNBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pmemdirFile = pmemfile->MEMF_pmemdir;
    if (pmemdirFile->pDir) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (pmemfile->MEMF_stOffset >= pmemdirFile->length) {
        return  (0);
    }
    
    stCopeBytes = __MIN(stNBytes, pmemdirFile->length - pmemfile->MEMF_stOffset);
    lib_memcpy((PVOID)(pmemdirFile->base + pmemfile->MEMF_stOffset), pcBuffer, stCopeBytes);
    
    pmemfile->MEMF_stOffset += stCopeBytes;
    
    KN_SMP_MB();
    
    return  ((size_t)stCopeBytes);
}
/*********************************************************************************************************
** 函数名称: _MemPWrite
** 功能描述: 扩展写内存文件
** 输　入  : pmemfile         内存设备文件
**           pcBuffer         发送缓冲区
**           stNBytes         写入数目
**           oftOffset        文件偏移量
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  _MemPWrite (PLW_MEM_FILE  pmemfile, 
                            PCHAR         pcBuffer, 
                            size_t        stNBytes,
                            off_t         oftOffset)
{
    MEM_DRV_DIRENTRY  *pmemdirFile;
    size_t             stCopeBytes;
    
    if (!pcBuffer || !stNBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pmemdirFile = pmemfile->MEMF_pmemdir;
    if (pmemdirFile->pDir) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (oftOffset >= pmemdirFile->length) {
        return  (0);
    }
    
    stCopeBytes = __MIN(stNBytes, pmemdirFile->length - (size_t)oftOffset);
    lib_memcpy((PVOID)(pmemdirFile->base + (size_t)oftOffset), pcBuffer, stCopeBytes);
    
    KN_SMP_MB();
    
    return  ((size_t)stCopeBytes);
}
/*********************************************************************************************************
** 函数名称: _MemStats
** 功能描述: 获得文件信息
** 输　入  : pmemfile         内存设备文件
**           pstat            文件信息
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _MemStats (PLW_MEM_FILE  pmemfile, struct stat *pstat)
{
    BOOL               bIsDir;
    PLW_MEM_DEV        pmemdev;
    MEM_DRV_DIRENTRY  *pmemdirFile;
    
    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pmemdev     = pmemfile->MEMF_memdev;
    pmemdirFile = pmemfile->MEMF_pmemdir;
    bIsDir      = (pmemdirFile->pDir) ? LW_TRUE : LW_FALSE;
    
    pstat->st_dev = (dev_t)pmemdev;
    pstat->st_ino = (ino_t)pmemdirFile;
    
    if (bIsDir) {
        pstat->st_mode = S_IFDIR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    } else {
        pstat->st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    }
    
    pstat->st_nlink = 1;
    pstat->st_uid   = pmemdev->MEM_uid;
    pstat->st_gid   = pmemdev->MEM_gid;
    pstat->st_rdev  = 1;
    
    if (bIsDir) {
        pstat->st_size   = 0;
        pstat->st_blocks = 1;
    } else {
        pstat->st_size   = (off_t)pmemdirFile->length;
        pstat->st_blocks = (blkcnt_t)pmemdirFile->length;
    }
    
    pstat->st_atime   = pmemdev->MEM_time;
    pstat->st_mtime   = pmemdev->MEM_time;
    pstat->st_ctime   = pmemdev->MEM_time;
    pstat->st_blksize = 1;
    
    pstat->st_resv1 = LW_NULL;
    pstat->st_resv2 = LW_NULL;
    pstat->st_resv3 = LW_NULL;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _MemStatsfs
** 功能描述: 获得文件系统信息
** 输　入  : pmemfile         内存设备文件
**           pstatfs          文件系统信息
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _MemStatsfs (PLW_MEM_FILE  pmemfile, struct statfs *pstatfs)
{
    if (!pstatfs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pstatfs->f_type   = DEVFS_SUPER_MAGIC;
    pstatfs->f_bsize  = 0;
    pstatfs->f_blocks = 1;
    pstatfs->f_bfree  = 0;
    pstatfs->f_bavail = 1;
    
    pstatfs->f_files  = 0;
    pstatfs->f_ffree  = 0;
    
#if LW_CFG_CPU_WORD_LENGHT == 64
    pstatfs->f_fsid.val[0] = (int32_t)((addr_t)pmemfile->MEMF_memdev >> 32);
    pstatfs->f_fsid.val[1] = (int32_t)((addr_t)pmemfile->MEMF_memdev & 0xffffffff);
#else
    pstatfs->f_fsid.val[0] = (int32_t)pmemfile->MEMF_memdev;
    pstatfs->f_fsid.val[1] = 0;
#endif
    
    pstatfs->f_flag    = 0;
    pstatfs->f_namelen = PATH_MAX;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _MemReadDir
** 功能描述: 读取目录信息
** 输　入  : pmemfile         内存设备文件
**           dir              目录信息
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _MemReadDir (PLW_MEM_FILE  pmemfile, DIR  *dir)
{
    BOOL               bIsDir;
    MEM_DRV_DIRENTRY  *pmemdirFile;
    
    pmemdirFile = pmemfile->MEMF_pmemdir;
    bIsDir      = (pmemdirFile->pDir) ? LW_TRUE : LW_FALSE;
    
    if (bIsDir == LW_FALSE) {
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }
    
    if (dir->dir_pos >= pmemdirFile->length) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    lib_strlcpy(dir->dir_dirent.d_name, 
                pmemdirFile->pDir[dir->dir_pos].name, 
                sizeof(dir->dir_dirent.d_name));
                
    if (bIsDir) {
        dir->dir_dirent.d_type = DT_DIR;
    } else {
        dir->dir_dirent.d_type = DT_REG;
    }
    
    dir->dir_dirent.d_shortname[0] = PX_EOS;
    
    dir->dir_pos++;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _MemIoctl
** 功能描述: memdev ioctl 操作
** 输　入  : pmemfile           内存设备文件
**           request,           命令
**           arg                命令参数
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  _MemIoctl (PLW_MEM_FILE  pmemfile,
                       INT           iRequest,
                       LONG          lArg)
{
    BOOL               bIsDir;
    MEM_DRV_DIRENTRY  *pmemdirFile;
    
    pmemdirFile = pmemfile->MEMF_pmemdir;
    bIsDir      = (pmemdirFile->pDir) ? LW_TRUE : LW_FALSE;
    
    switch (iRequest) {
    
    case FIOSEEK:
        if (bIsDir) {
            _ErrorHandle(EISDIR);
            return  (PX_ERROR);
        } else {
            pmemfile->MEMF_stOffset = (size_t)(*(off_t *)lArg);
        }
        break;
        
    case FIOWHERE:
        if (bIsDir) {
            _ErrorHandle(EISDIR);
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = (off_t)pmemfile->MEMF_stOffset;
        }
        break;
        
    case FIONREAD:
        if (bIsDir) {
            _ErrorHandle(EISDIR);
            return  (PX_ERROR);
        } else {
            if (pmemfile->MEMF_stOffset >= pmemdirFile->length) {
                *(INT *)lArg = 0;
            } else {
                *(INT *)lArg = pmemdirFile->length - pmemfile->MEMF_stOffset;
            }
        }
        break;
        
    case FIONREAD64:
        if (bIsDir) {
            _ErrorHandle(EISDIR);
            return  (PX_ERROR);
        } else {
            if (pmemfile->MEMF_stOffset >= pmemdirFile->length) {
                *(off_t *)lArg = 0;
            } else {
                *(off_t *)lArg = (off_t)pmemdirFile->length - pmemfile->MEMF_stOffset;
            }
        }
        break;
        
    case FIOFSTATGET:
        return  (_MemStats(pmemfile, (struct stat *)lArg));
        
    case FIOFSTATFSGET:
        return  (_MemStatsfs(pmemfile, (struct statfs *)lArg));
        
    case FIOREADDIR:
        return  (_MemReadDir(pmemfile, (DIR *)lArg));
        
    case FIOSYNC:
    case FIOFLUSH:
    case FIODATASYNC:
#if LW_CFG_CACHE_EN > 0
        if (bIsDir == FALSE) {
            API_CacheFlush(DATA_CACHE, pmemdirFile->base, pmemdirFile->length);
        }
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
        break;
    
    case FIOFSTYPE:                                                     /*  获得文件系统类型            */
        *(PCHAR *)lArg = "Memory FileSystem";
        break;
    
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_MEMDEV_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
