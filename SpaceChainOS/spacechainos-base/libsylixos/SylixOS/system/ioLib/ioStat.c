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
** 文   件   名: ioStat.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 09 月 19 日
**
** 描        述: 获得文件或磁盘状态的 POSIX 调用.

** BUG
2008.11.11  今天是搬进新居后第一次升级操作系统, access() 函数在产生错误时, 没有关闭文件.
2008.12.04  chmod(), 需要以只读方式打开文件.
2009.02.13  修改代码以适应新的 I/O 系统结构组织.
2009.05.19  chmod() 以 O_WRONLY 方式打开.
2009.06.18  access 应该使用 USR 权限判断.
2009.06.24  sync() 不应操作异常文件.
2009.07.09  ftruncate() 支持 64 bit 文件.
2009.07.22  sync() 使用 FIOSYNC 而不是 FIOFLUSH. (FIOFLUSH 会造成 tty pipe 等设备丢弃数据).
2009.08.26  fchmod() mode 至少要包含 S_IRUSR 权限.
2009.12.14  fstat() 当设备不支持时, 产生一个缺省的 stat.
2011.08.07  加入 lstat() 方法.
2012.04.11  加入 64bit 文件操作.
2012.10.17  为了兼容性, 加入 chown() 系列函数, 当前并不支持.
2012.12.21  由于每个进程实现了自己独立的文件描述符, 所以这里 sync 改用遍历 fd_entry 的方式.
            注意: sync() 调用时 IO 系统为锁定状态.
2013.01.03  所有对驱动 close 与 ioctl 操作必须通过 _IosFileClose 与 _IosFileIoctl 接口.
2013.01.08  fstat64 首先先使用 FIOFSTATGET64 测试如果成功则直接返回.
2013.01.21  fchmod() 必须拥有可写权限.
            实现 chown().
2013.09.16  fstat() 设备如果失败, 则 st_dev 使用 pfdentry 设备指针.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
  文件系统相关
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)
#include "../SylixOS/fs/include/fs_fs.h"
#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
/*********************************************************************************************************
** 函数名称: fstat
** 功能描述: 获得文件的相关信息.
** 输　入  : iFd           文件描述符
**           pstat         获得的状态缓冲区
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  fstat (INT  iFd, struct stat *pstat)
{
LW_API time_t  API_RootFsTime(time_t  *time);

    INT     iErrCode;

    if (iFd < 0) {
        errno = EBADF;
        return  (PX_ERROR);
    }
    
    if (!pstat) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    iErrCode = API_IosFstat(iFd, pstat);                                /*  优先使用驱动表函数          */
    if (iErrCode >= ERROR_NONE) {
        return  (iErrCode);
    }
    
    iErrCode = ioctl(iFd, FIOFSTATGET, (LONG)pstat);
    if ((iErrCode != ERROR_NONE) && 
        ((iErrCode == ENOSYS) || (errno == ENOSYS))) {
        PLW_FD_ENTRY  pfdentry = _IosFileGet(iFd, LW_FALSE);
        
        if (pfdentry) {
            pstat->st_dev = (dev_t)pfdentry->FDENTRY_pdevhdrHdr;
        } else {
            return  (PX_ERROR);
        }
        
        pstat->st_ino     = 0;
        pstat->st_mode    = 0666 | S_IFCHR;                             /*  默认属性                    */
        pstat->st_nlink   = 0;
        pstat->st_uid     = 0;
        pstat->st_gid     = 0;
        pstat->st_rdev    = 1;
        pstat->st_size    = 0;
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        pstat->st_atime   = API_RootFsTime(LW_NULL);                    /*  默认使用 root fs 基准时间   */
        pstat->st_mtime   = API_RootFsTime(LW_NULL);
        pstat->st_ctime   = API_RootFsTime(LW_NULL);
        
        iErrCode = ERROR_NONE;
    }
    
    return  (iErrCode);
}
/*********************************************************************************************************
** 函数名称: fstat64
** 功能描述: 获得文件的相关信息.
** 输　入  : iFd           文件描述符
**           pstat64       获得的状态缓冲区
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  fstat64 (INT  iFd, struct stat64 *pstat64)
{
    struct stat statFile;
    INT         iErrCode;
    
    if (!pstat64) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    iErrCode = ioctl(iFd, FIOFSTATGET64, (LONG)pstat64);
    if (iErrCode == ERROR_NONE) {
        return  (iErrCode);
    }
    
    iErrCode = fstat(iFd, &statFile);
    if (iErrCode == ERROR_NONE) {
        pstat64->st_dev     = statFile.st_dev;
        pstat64->st_ino     = (ino64_t)statFile.st_ino;
        pstat64->st_mode    = statFile.st_mode;
        pstat64->st_nlink   = statFile.st_nlink;
        pstat64->st_uid     = statFile.st_uid;
        pstat64->st_gid     = statFile.st_gid;
        pstat64->st_rdev    = statFile.st_rdev;
        pstat64->st_size    = (off64_t)statFile.st_size;
        pstat64->st_atime   = statFile.st_atime;
        pstat64->st_mtime   = statFile.st_mtime;
        pstat64->st_ctime   = statFile.st_ctime;
        pstat64->st_blksize = statFile.st_blksize;
        pstat64->st_blocks  = (blkcnt64_t)statFile.st_blocks;
    }
        
    return  (iErrCode);
}
/*********************************************************************************************************
** 函数名称: stat
** 功能描述: 通过文件名获得文件的相关信息.
** 输　入  : pcName        文件名
**           pstat         获得的状态缓冲区
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  stat (CPCHAR  pcName, struct stat *pstat)
{
    REGISTER INT    iFd;
    REGISTER INT    iError;
    
    iFd = open(pcName, O_RDONLY, 0);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = fstat(iFd, pstat);
    
    if (iError < 0) {
        iError = errno;
        close(iFd);
        errno  = iError;
        return  (PX_ERROR);
    
    } else {
        close(iFd);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: stat64
** 功能描述: 通过文件名获得文件的相关信息.
** 输　入  : pcName        文件名
**           pstat64       获得的状态缓冲区
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  stat64 (CPCHAR  pcName, struct stat64 *pstat64)
{
    REGISTER INT    iFd;
    REGISTER INT    iError;
    
    iFd = open(pcName, O_RDONLY, 0);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = fstat64(iFd, pstat64);
    
    if (iError < 0) {
        iError = errno;
        close(iFd);
        errno  = iError;
        return  (PX_ERROR);
    
    } else {
        close(iFd);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: lstat
** 功能描述: 通过文件名获得文件的相关信息. (如果是链接文件, 则返回连接文件相关信息)
** 输　入  : pcName        文件名
**           pstat         获得的状态缓冲区
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  lstat (CPCHAR  pcName, struct stat *pstat)
{
    REGISTER LONG  lValue;
    PLW_FD_ENTRY   pfdentry;
    PLW_DEV_HDR    pdevhdrHdr;
    CHAR           cFullFileName[MAX_FILENAME_LENGTH];
    PCHAR          pcLastTimeName;
    INT            iLinkCount = 0;
    
    INT            iError;
    
    if (pcName == LW_NULL) {                                            /*  检查文件名                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    if (pcName[0] == PX_EOS) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    if (lib_strcmp(pcName, ".") == 0) {                                 /*  滤掉当前目录                */
        pcName++;
    }
    
    if (ioFullFileNameGet(pcName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    pcLastTimeName = (PCHAR)__SHEAP_ALLOC(MAX_FILENAME_LENGTH);
    if (pcLastTimeName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_strlcpy(pcLastTimeName, cFullFileName, MAX_FILENAME_LENGTH);
    
    pfdentry = _IosFileNew(pdevhdrHdr, cFullFileName);                  /*  创建一个临时的 fd_entry     */
    if (pfdentry == LW_NULL) {
        __SHEAP_FREE(pcLastTimeName);
        return  (PX_ERROR);
    }
    
    for (;;) {
        lValue = iosOpen(pdevhdrHdr, cFullFileName, O_RDONLY, 0);
        if (lValue != FOLLOW_LINK_TAIL) {                               /*  FOLLOW_LINK_FILE 直接退出   */
            break;
        
        } else {
            if (iLinkCount++ > _S_iIoMaxLinkLevels) {                   /*  链接文件层数太多            */
                _IosFileDelete(pfdentry);
                __SHEAP_FREE(pcLastTimeName);
                _ErrorHandle(ELOOP);
                return  (PX_ERROR);
            }
        }
    
        /*
         *  驱动程序如果返回 FOLLOW_LINK_????, cFullFileName内部一定是目标的绝对地址, 即以/起始的文件名.
         */
        if (ioFullFileNameGet(cFullFileName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
            _IosFileDelete(pfdentry);
            __SHEAP_FREE(pcLastTimeName);
            _ErrorHandle(EXDEV);
            return  (PX_ERROR);
        }
        lib_strlcpy(pcLastTimeName, cFullFileName, MAX_FILENAME_LENGTH);
    }
    
    if ((lValue != PX_ERROR) && (lValue != FOLLOW_LINK_FILE)) {
        _IosFileSet(pfdentry, pdevhdrHdr, lValue, O_RDONLY);
        _IosFileClose(pfdentry);                                        /*  关闭                        */
    }
    
    _IosFileDelete(pfdentry);                                           /*  删除临时的 fd_entry         */
    
    iError = API_IosLstat(pdevhdrHdr, pcLastTimeName, pstat);
    
    __SHEAP_FREE(pcLastTimeName);
    
    if (iError < ERROR_NONE) {
        return  (stat(pcName, pstat));
    }
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: lstat64
** 功能描述: 通过文件名获得文件的相关信息. (如果是链接文件, 则返回连接文件相关信息)
** 输　入  : pcName        文件名
**           pstat64       获得的状态缓冲区
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  lstat64 (CPCHAR  pcName, struct stat64 *pstat64)
{
    struct stat statFile;
    INT         iErrCode;
    
    if (!pstat64) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    iErrCode = lstat(pcName, &statFile);
    if (iErrCode == ERROR_NONE) {
        pstat64->st_dev     = statFile.st_dev;
        pstat64->st_ino     = (ino64_t)statFile.st_ino;
        pstat64->st_mode    = statFile.st_mode;
        pstat64->st_nlink   = statFile.st_nlink;
        pstat64->st_uid     = statFile.st_uid;
        pstat64->st_gid     = statFile.st_gid;
        pstat64->st_rdev    = statFile.st_rdev;
        pstat64->st_size    = (off64_t)statFile.st_size;
        pstat64->st_atime   = statFile.st_atime;
        pstat64->st_mtime   = statFile.st_mtime;
        pstat64->st_ctime   = statFile.st_ctime;
        pstat64->st_blksize = statFile.st_blksize;
        pstat64->st_blocks  = (blkcnt64_t)statFile.st_blocks;
    }
        
    return  (iErrCode);
}
/*********************************************************************************************************
** 函数名称: fstatfs
** 功能描述: 获得文件系统的相关信息.
** 输　入  : iFd           文件描述符
**           pstatfs       文件系统状态缓冲区
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  fstatfs (INT  iFd, struct statfs *pstatfs)
{
    if (!pstatfs) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (ioctl(iFd, FIOFSTATFSGET, (LONG)pstatfs));
}
/*********************************************************************************************************
** 函数名称: statfs
** 功能描述: 通过文件名获得文件系统的相关信息.
** 输　入  : pcName        文件名
**           pstatfs       文件系统状态缓冲区
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  statfs (CPCHAR  pcName, struct statfs *pstatfs)
{
    REGISTER INT    iFd;
    REGISTER INT    iError;
    
    iFd = open(pcName, O_RDONLY, 0);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = fstatfs(iFd, pstatfs);
    
    if (iError < 0) {
        iError = errno;
        close(iFd);
        errno  = iError;
        return  (PX_ERROR);
    
    } else {
        close(iFd);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: ftruncate
** 功能描述: 缩减或扩展文件长度。如果之前的文件长度比length指定的长度大，额外的数据会丢失。
**           如果之前的文件长度比指定的长度小，将会用‘/0’填充增大部分。
** 输　入  : iFd           文件描述符
**           oftLength     文件长度
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  ftruncate (INT  iFd, off_t  oftLength)
{
    return  (ioctl(iFd, FIOTRUNC, (LONG)&oftLength));
}
/*********************************************************************************************************
** 函数名称: ftruncate64
** 功能描述: 缩减或扩展文件长度。如果之前的文件长度比length指定的长度大，额外的数据会丢失。
**           如果之前的文件长度比指定的长度小，将会用‘/0’填充增大部分。
** 输　入  : iFd           文件描述符
**           oftLength     文件长度
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  ftruncate64 (INT  iFd, off64_t  oftLength)
{
    return  (ioctl(iFd, FIOTRUNC, (LONG)&oftLength));
}
/*********************************************************************************************************
** 函数名称: truncate
** 功能描述: 缩减或扩展文件长度。如果之前的文件长度比length指定的长度大，额外的数据会丢失。
**           如果之前的文件长度比指定的长度小，将会用‘/0’填充增大部分。
** 输　入  : pcName        文件名
**           oftLength     文件长度
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  truncate (CPCHAR  pcName, off_t  oftLength)
{
    REGISTER INT    iFd;
    REGISTER INT    iError;
    
    iFd = open(pcName, O_WRONLY, 0);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = ftruncate(iFd, oftLength);
    
    if (iError < 0) {
        iError = errno;
        close(iFd);
        errno  = iError;
        return  (PX_ERROR);
    
    } else {
        close(iFd);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: truncate64
** 功能描述: 缩减或扩展文件长度。如果之前的文件长度比length指定的长度大，额外的数据会丢失。
**           如果之前的文件长度比指定的长度小，将会用‘/0’填充增大部分。
** 输　入  : pcName        文件名
**           oftLength     文件长度
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  truncate64 (CPCHAR  pcName, off64_t  oftLength)
{
    REGISTER INT    iFd;
    REGISTER INT    iError;
    
    iFd = open(pcName, O_WRONLY, 0);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = ftruncate64(iFd, oftLength);
    
    if (iError < 0) {
        iError = errno;
        close(iFd);
        errno  = iError;
        return  (PX_ERROR);
    
    } else {
        close(iFd);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: fchmod
** 功能描述: 尝试将 filename 所指定文件的模式改成 mode 所给定的
** 输　入  : iFd           文件描述符
**           iMode         模式
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  fchmod (INT  iFd, INT  iMode)
{
    struct stat statBuf;

    iMode |= S_IRUSR;                                                   /*  必须保证所有者能读          */
    iMode &= ~S_IFMT;                                                   /*  去掉文件类型                */
    
    if (fstat(iFd, &statBuf) < 0) {
        return  (PX_ERROR);
    }
    
    if (_IosCheckPermissions(O_WRONLY, LW_FALSE, statBuf.st_mode, 
                             statBuf.st_uid, statBuf.st_gid) < ERROR_NONE) {
        _ErrorHandle(EACCES);
        return  (PX_ERROR);
    }

    return  (ioctl(iFd, FIOCHMOD, iMode));
}
/*********************************************************************************************************
** 函数名称: chmod
** 功能描述: 尝试将 filename 所指定文件的模式改成 mode 所给定的
** 输　入  : pcName        文件名
**           iMode         模式
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  chmod (CPCHAR  pcName, INT  iMode)
{
    REGISTER INT    iFd;
    REGISTER INT    iError;
    
    iFd = open(pcName, O_RDONLY, 0);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = fchmod(iFd, iMode);
    
    if (iError < 0) {
        iError = errno;
        close(iFd);
        errno  = iError;
        return  (PX_ERROR);
    
    } else {
        close(iFd);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: fchown
** 功能描述: 更改文件的用户ID和，组ID
** 输　入  : iFd           文件描述符
**           uid           所有者 ID
**           gid           所有者组 ID
** 输　出  : ERROR_NONE    没有错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  fchown (INT  iFd, uid_t uid, gid_t gid)
{
    struct stat statBuf;
    LW_IO_USR   usr;
    
    usr.IOU_uid = uid;
    usr.IOU_gid = gid;
    
    if (fstat(iFd, &statBuf) < 0) {
        return  (PX_ERROR);
    }
    
    if (_IosCheckPermissions(O_WRONLY, LW_FALSE, statBuf.st_mode, 
                             statBuf.st_uid, statBuf.st_gid) < ERROR_NONE) {
        _ErrorHandle(EACCES);
        return  (PX_ERROR);
    }

    return  (ioctl(iFd, FIOCHOWN, (LONG)&usr));
}
/*********************************************************************************************************
** 函数名称: chown
** 功能描述: 更改文件的用户ID和，组ID
** 输　入  : pcName        文件名
**           uid           所有者 ID
**           gid           所有者组 ID
** 输　出  : ERROR_NONE    没有错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  chown (CPCHAR  pcName, uid_t uid, gid_t gid)
{
    REGISTER INT    iFd;
    REGISTER INT    iError;
    
    iFd = open(pcName, O_RDONLY, 0);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = fchown(iFd, uid, gid);
    
    if (iError < 0) {
        iError = errno;
        close(iFd);
        errno  = iError;
        return  (PX_ERROR);
    
    } else {
        close(iFd);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: lchown
** 功能描述: 更改文件的用户ID和，组ID
** 输　入  : pcName        文件名
**           uid           所有者 ID
**           gid           所有者组 ID
** 输　出  : ERROR_NONE    没有错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  lchown (CPCHAR  pcName, uid_t uid, gid_t gid)
{
    errno = ENOSYS;
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: fsync
** 功能描述: 将文件缓存写入磁盘
** 输　入  : iFd           文件描述符
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  fsync (INT  iFd)
{
    INT     iRet;

    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */

    iRet = ioctl(iFd, FIOSYNC);
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: fdatasync
** 功能描述: 将文件缓存写入磁盘(数据部分)
** 输　入  : iFd           文件描述符
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  fdatasync (INT  iFd)
{
    INT     iRet;
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    iRet = ioctl(iFd, FIODATASYNC);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: sync
** 功能描述: 系统 IO 缓存写入磁盘 (有危险, 设备驱动中不能擦偶偶)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID  sync (VOID)
{
    REGISTER PLW_LIST_LINE  plineFdEntry;
    REGISTER PLW_FD_ENTRY   pfdentry;
    REGISTER PLW_DEV_HDR    pdevhdr;
    
    _IosLock();                                                         /*  进入 IO 临界区              */
    
    _IosFileListLock();                                                 /*  开始遍历                    */
    
    plineFdEntry = _S_plineFileEntryHeader;
    while (plineFdEntry) {
        pfdentry = _LIST_ENTRY(plineFdEntry, LW_FD_ENTRY, FDENTRY_lineManage);
        pdevhdr = pfdentry->FDENTRY_pdevhdrHdr;
        plineFdEntry = _list_line_get_next(plineFdEntry);
        
        if ((pdevhdr->DEVHDR_ucType == DT_CHR)  ||
            (pdevhdr->DEVHDR_ucType == DT_FIFO) ||
            (pdevhdr->DEVHDR_ucType == DT_SOCK)) {                      /*  CHR, FIFO, SOCK 不操作      */
            continue;
        }
        
        if (pfdentry->FDENTRY_iAbnormity) {
            continue;
        }
        
        _IosUnlock();                                                   /*  退出 IO 临界区              */
        _IosFileIoctl(pfdentry, FIOSYNC, 0);                            /*  调用驱动同步数据            */
        _IosLock();                                                     /*  进入 IO 临界区              */
    }
    
    _IosUnlock();                                                       /*  退出 IO 临界区              */
    
    _IosFileListUnlock();                                               /*  结束遍历, 删除请求删除的节点*/

#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)
    API_DiskCacheSync(LW_NULL);                                         /*  回写所有磁盘缓冲            */
#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
}
/*********************************************************************************************************
** 函数名称: access
** 功能描述: 判断文件操作权限
** 输　入  : pcPath        File or Direction 
**           iMode         mode
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  access (CPCHAR pcPath, INT  iMode)
{
    REGISTER INT          iError;
    REGISTER INT          iFd = open(pcPath, O_RDONLY, 0);
             struct stat  statFile;
    
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = fstat(iFd, &statFile);
    if (iError < 0) {
        iError = errno;
        close(iFd);
        errno  = iError;
        return  (PX_ERROR);
    }
    
    if (iMode & R_OK) {
        if ((statFile.st_mode & S_IREAD) == 0) {
            goto    __error_handle;
        }
    }
    
    if (iMode & W_OK) {
        if ((statFile.st_mode & S_IWRITE) == 0) {
            goto    __error_handle;
        }
    }
    
    if (iMode & X_OK) {
        if ((statFile.st_mode & S_IEXEC) == 0) {
            goto    __error_handle;
        }
    }
    close(iFd);
    
    return  (ERROR_NONE);
    
__error_handle:
    close(iFd);
    errno = EACCES;
    return  (PX_ERROR);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
