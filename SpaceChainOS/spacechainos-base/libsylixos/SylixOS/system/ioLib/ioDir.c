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
** 文   件   名: ioDir.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 09 月 19 日
**
** 描        述: 系统目录操作库.

** BUG:
2009.03.10  errno 的清零使用 _ErrorHandle().
2009.07.21  mkdir() 的 mode 参数暂时未使用.
2009.08.26  目录操作加入 readdir_r() 线程安全函数, 需要 dir 锁配合.
2011.05.15  修正 rmdir 与 utime 参数为 const 类型.
2012.03.11  加入 futimes() utimes() 函数.
2012.09.21  加入 dirfd() 函数.
2012.12.11  dir 加入进程原始资源管理.
2012.12.22  使用 freedir 来释放进程没有关闭的 DIR, freedir 中不关闭文件描述符, 
            因为回收进程或者内核已经不是分配时进程的文件描述符表, 文件的关闭统一由进程回收器完成.
2013.01.08  创建目录需要有 O_EXCL 参数进行排他性创建.
2013.03.12  加入 readdir64 与 readdir64_r.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
** 函数名称: mkdir
** 功能描述: 创建一个新的目录
** 输　入  : pcDirName     目录名
**           mode          方式
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  mkdir (CPCHAR  pcDirName, mode_t  mode)
{
    REGISTER INT    iFd;
    
    mode &= ~S_IFMT;
    
    iFd = open(pcDirName, O_RDWR | O_CREAT | O_EXCL, S_IFDIR | mode);   /*  排他性创建                  */
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    close(iFd);
    
    return (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: rmdir
** 功能描述: 删除一个存在的目录
** 输　入  : pcDirName     目录名
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  rmdir (CPCHAR  pcDirName)
{
    return  (unlink(pcDirName));
}
/*********************************************************************************************************
** 函数名称: dirfd
** 功能描述: 从 DIR 结构中获取目录文件描述符
** 输　入  : pdir     目录控制块
** 输　出  : 文件描述符
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  dirfd (DIR  *pdir)
{
    if (!pdir) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (pdir->dir_fd);
}
/*********************************************************************************************************
** 函数名称: freedir
** 功能描述: 释放一个DIR结构, 不关闭文件, 由回收器统一关闭文件
             (用于进程资源回收, 不可以使用 closedir 因为文件描述符已不是结束进程的文件描述符表)
** 输　入  : pdir     目录控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

static VOID  freedir (DIR  *pdir)
{
    if (pdir) {
        API_SemaphoreBDelete(&pdir->dir_lock);
        
        __resDelRawHook(&pdir->dir_resraw);
    
        __SHEAP_FREE(pdir);
    }
}

#endif
/*********************************************************************************************************
** 函数名称: opendir
** 功能描述: 打开一个存在的目录
** 输　入  : pcDirName     目录名
** 输　出  : 目录控制块指针, 错误返回 NULL, 请查阅相关 errno 含义
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
DIR  *opendir (CPCHAR   pcPathName)
{
    REGISTER INT            iFd;
    REGISTER INT            iError;
             struct stat    statFile;
             DIR           *pdir;
    
    iFd = open(pcPathName, O_RDONLY, 0);
    if (iFd < 0) {
        return  (LW_NULL);
    }
    
    iError = fstat(iFd, &statFile);
    if (iError < 0) {
        close(iFd);
        return  (LW_NULL);
    }
    
    if (!S_ISDIR(statFile.st_mode)) {                                   /*  检测是否为目录文件          */
        close(iFd);
        errno = ENOTDIR;
        return  (LW_NULL);
    }
    
    pdir = (DIR *)__SHEAP_ALLOC(sizeof(DIR));                           /*  开辟 DIR 结构目录           */
    if (!pdir) {
        close(iFd);
        errno = ERROR_SYSTEM_LOW_MEMORY;
        return  (LW_NULL);
    }
    lib_bzero(pdir, sizeof(DIR));
    
    pdir->dir_fd    = iFd;
    pdir->dir_pos   = 0;
    pdir->dir_lock  = API_SemaphoreBCreate("dir_lock", LW_TRUE, LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pdir->dir_lock == LW_OBJECT_HANDLE_INVALID) {                   /*  是否成功创建操作锁          */
        close(iFd);
        __SHEAP_FREE(pdir);
        return  (LW_NULL);
    }
                                                                        /*  加入原始资源表              */
    __resAddRawHook(&pdir->dir_resraw, (VOIDFUNCPTR)freedir, pdir, 0, 0, 0, 0, 0);
    
    return  (pdir);
}
/*********************************************************************************************************
** 函数名称: closedir
** 功能描述: 关闭一个已经打开的目录
** 输　入  : pdir     目录控制块
** 输　出  : ERROR_NONE 表示正确, 负数表示错误.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  closedir (DIR  *pdir)
{
    REGISTER INT   iError;

    if (!pdir) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    iError = close(pdir->dir_fd);
    if (iError < 0) {
        return  (PX_ERROR);
    }
    pdir->dir_fd = PX_ERROR;                                            /*  表明已经关闭                */
    
    API_SemaphoreBDelete(&pdir->dir_lock);
    
    __resDelRawHook(&pdir->dir_resraw);
    
    __SHEAP_FREE(pdir);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: readdir
** 功能描述: 获取一个已经打开的目录的单条信息
** 输　入  : pdir     目录控制块
** 输　出  : 单条目信息指针, 错误返回 NULL, 请查阅相关 errno 含义
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
struct dirent *readdir (DIR  *pdir)
{
    REGISTER INT   iError;

    if (!pdir) {
        errno = EINVAL;
        return  (LW_NULL);
    }
    
    API_SemaphoreBPend(pdir->dir_lock, LW_OPTION_WAIT_INFINITE);
    iError = ioctl(pdir->dir_fd, FIOREADDIR, (LONG)pdir);
    API_SemaphoreBPost(pdir->dir_lock);
    
    if (iError < 0) {
        return  (LW_NULL);
    }
    
    return  (&pdir->dir_dirent);
}
/*********************************************************************************************************
** 函数名称: readdir64
** 功能描述: 获取一个已经打开的目录的单条信息
** 输　入  : pdir     目录控制块
** 输　出  : 单条目信息指针, 错误返回 NULL, 请查阅相关 errno 含义
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
struct dirent64 *readdir64 (DIR  *pdir)
{
    return  ((struct dirent64 *)readdir(pdir));
}
/*********************************************************************************************************
** 函数名称: readdir_r
** 功能描述: 获取一个已经打开的目录的单条信息 (可重入)
** 输　入  : pdir              目录控制块
**           pdirentEntry      获得的目录条目信息缓冲
**           ppdirentResult    当成功时, 此指针指向 pdirentEntry, 当到达末位时, 此指针为 NULL.
** 输　出  : ERROR_NONE 表示正确, 负数表示错误.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  readdir_r (DIR             *pdir, 
                struct dirent   *pdirentEntry,
                struct dirent  **ppdirentResult)
{
    REGISTER INT   iError;

    if (!pdir || !pdirentEntry || !ppdirentResult) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    API_SemaphoreBPend(pdir->dir_lock, LW_OPTION_WAIT_INFINITE);
    iError = ioctl(pdir->dir_fd, FIOREADDIR, (LONG)pdir);
    if (iError < 0) {
        API_SemaphoreBPost(pdir->dir_lock);
        *ppdirentResult = LW_NULL;
        return  (PX_ERROR);
    }
    *pdirentEntry   = pdir->dir_dirent;
    *ppdirentResult = pdirentEntry;
    API_SemaphoreBPost(pdir->dir_lock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: readdir64_r
** 功能描述: 获取一个已经打开的目录的单条信息 (可重入)
** 输　入  : pdir              目录控制块
**           pdirent64Entry    获得的目录条目信息缓冲
**           ppdirent64Result  当成功时, 此指针指向 pdirentEntry, 当到达末位时, 此指针为 NULL.
** 输　出  : ERROR_NONE 表示正确, 负数表示错误.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  readdir64_r (DIR               *pdir, 
                  struct dirent64   *pdirent64Entry,
                  struct dirent64  **ppdirent64Result)
{
    return  (readdir_r(pdir, (struct dirent *)pdirent64Entry, (struct dirent **)ppdirent64Result));
}
/*********************************************************************************************************
** 函数名称: rewinddir
** 功能描述: 复位当前目录指针
** 输　入  : pdir     目录控制块
** 输　出  : ERROR_NONE 表示正确, 负数表示错误.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  rewinddir (DIR   *pdir)
{
    if (!pdir) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    API_SemaphoreBPend(pdir->dir_lock, LW_OPTION_WAIT_INFINITE);
    pdir->dir_pos = 0;                                                  /*  退回到原点                  */
    API_SemaphoreBPost(pdir->dir_lock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: futimes
** 功能描述: 设置文件时间
** 输　入  : iFd        文件描述符
**           tvp        时间
** 输　出  : ERROR_NONE 表示正确, 负数表示错误.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  futimes (INT iFd, struct timeval tvp[2])
{
    struct utimbuf  utimbNow;
    
    if (!tvp) {
        utimbNow.actime  = lib_time(LW_NULL);
        utimbNow.modtime = utimbNow.actime;
    } else {
        utimbNow.actime  = tvp[0].tv_sec;
        utimbNow.modtime = tvp[1].tv_sec;
    }
    
    return  (ioctl(iFd, FIOTIMESET, (LONG)&utimbNow));
}
/*********************************************************************************************************
** 函数名称: utimes
** 功能描述: 设置文件时间
** 输　入  : iFd        文件描述符
**           tvp        时间
** 输　出  : ERROR_NONE 表示正确, 负数表示错误.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  utimes (CPCHAR  pcFile, struct timeval tvp[2])
{
    REGISTER INT            iError;
    REGISTER INT            iFd;
    
    iFd = open(pcFile, O_RDONLY, 0);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = futimes(iFd, tvp);
    
    close(iFd);
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: utime
** 功能描述: 设置文件时间
** 输　入  : pcFile     文件名
**           utimbNew   新的文件时间
** 输　出  : ERROR_NONE 表示正确, 负数表示错误.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  utime (CPCHAR  pcFile, const struct utimbuf *utimbNew)
{
    REGISTER INT            iError;
    REGISTER INT            iFd;
    struct utimbuf          utimbNow;
    
    iFd = open(pcFile, O_RDONLY, 0);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    if (!utimbNew) {
        utimbNew = &utimbNow;
        utimbNow.actime  = lib_time(LW_NULL);
        utimbNow.modtime = utimbNow.actime;
    }
    
    iError = ioctl(iFd, FIOTIMESET, (LONG)utimbNew);
    
    close(iFd);
    
    return  (iError);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
