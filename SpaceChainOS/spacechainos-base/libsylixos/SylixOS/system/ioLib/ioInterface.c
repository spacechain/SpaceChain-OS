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
** 文   件   名: ioInterface.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 16 日
**
** 描        述: 系统 IO 功能函数库，API 部分

** BUG
2007.09.12  加入对裁剪的支持。
2007.09.24  加入 _DebugHandle() 功能。
2008.05.31  将 ioctl 第 3 个参数提升为 long 型, 以适应 64 位系统指针需求.
2008.06.08  将 I/O safe 机制, 改为 posix 规定的对于 cancel 方式的要求.
2008.06.27  修改 errhandle 标号的命名.
2008.07.23  加入 ttyname() api 函数.
2008.08.10  北京奥运开始后第一次升级操作系统, 心情很激动, 祝奥运会成功!
            升级 API_Dup2(), 这个函数将可以用作 I/O 重定向.
2008.09.28  今天神舟七号飞船, 将要返回地面, 预祝返回成功!
            升级 _IoCreateOrOpen() 在错误产生时保证 errno 产生的正确性.
            更正了 API_IoLseek() 第一处 BUG.
2008.10.31  _IoCreateOrOpen() 驱动程序无法打开文件时, 不报错误.
2009.02.13  修改代码以适应新的 I/O 系统结构组织.
2009.02.19  修改了 _IoCreateOrOpen() 对打开失败的处理.
2009.03.10  errno的清零使用 _ErrorHandle.
2009.03.13  open, ioctl 改为可变长参数函数.
            creat 改为与 unix 定义完全相同.
2009.05.19  API_IoRename() 应该以 O_WRONLY 打开文件.
2009.06.07  API_IoOpen() 第三个参数默认为 DEFAULT_FILE_PERM.
2009.06.30  API_Ioctl() 加入 va_end 操作.
2009.07.01  加入 fchdir() 功能.
            writev, readv 需要加入线程异步状态设置.
2009.07.09  API_IoLseek() 支持 64 bit 文件. SEEK_END 首先试探 FIONREAD64 方法, 如果不行, 在使用 fstat.
2009.07.25  _IoCreateOrOpen() 避免创建没有任何权限的文件.
            _IoCreateOrOpen() 修正 creat 调用参数问题.
2009.10.12  dup(), dup2() 不应该对设备计数器进行操作.
2009.10.22  修改 read write 参数及返回值类型.
2009.11.21  加入 ioPrivateEnv() 函数.
2009.11.22  加入 API_IoFullFileNameGet2() 函数, 个进行目录压缩.
            IoFullFileNameGet 函数需要在路径结尾去掉多余的 / 符号.
2009.11.23  open 函数根据 LW_CFG_PATH_AUTO_CONDENSE 配置, 可以将设备后端的设备相关目录 . 和 .. 进行自动
            压缩, 例如 open(/DEVICE/./aaa/../bbb) 会自动变为 open(/DEVICE/bbb)
2009.12.04  更新注释.
2009.12.14  API_IoDefPathCat() 首先压缩目录再确定设备的有效性.
2009.12.15  修正 _IoOpen() 函数对链接文件的处理错误.
2010.01.21  加入 ttyname_r() 函数.
2010.08.03  支持 SMP 多核.
2011.05.16  修正 rename() 新名称为符号链接的问题.
            API_IoFullFileNameGet() API_IoFullFileNameGet2() 修正设备为根文件系统导致的 tail 缺少 '/' 起始
            的错误.
2011.08.07  整体升级对符号链接的支持, 使他支持连接目标可以是相对目录.
2011.08.11  去掉 API_IoFullFileNameGet2 函数, 因为操作系统从此默认进行目录压缩.
2011.08.15  重大决定: 将所有 posix 定义的函数以函数方式(非宏)引出.
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2012.04.11  加入 64bit 文件标准操作.
2012.06.28  加入了 makedev major minor 这三个 linux 兼容函数.
2012.08.16  加入 pread 与 pwrite 接口, 需要对应设备(文件系统)驱动程序支持.
2012.09.25  mknod() 需要有 O_EXCL 参数.
2012.10.16  加入 realpath() 功能.
2012.10.24  fchdir() 必须保证是目录文件才能设置.
            _IoOpen() 以 full name get 后的名字作为参数传给 fdset, 这样文件描述符内部保存文件绝对路径.
2012.11.21  unlink() 操作如果遇到打开的文件则设置标志, 等文件关闭时, 自动 unlink()
2012.12.21  今天是传说中的世界末日, 我还在维护 SylixOS 呢.
            *StdSet 和 *StdGet 函数只准对内核线程, 进程内线程不起作用.
2012.12.24  加入 dup2kernel 进程可以吧文件描述符 dup 到内核中.
2013.01.03  必须使用 _IosFileClose 来调用驱动的 close 函数.
2013.01.05  加入 flock 函数.
2013.01.09  加入 umask 函数.
2013.01.17  open 加入 O_CLOEXEC / O_SHLOCK 和 O_EXLOCK 的支持.
2013.03.12  加入 open64.
2013.05.31  unlink 操作每一次都需要检查当前已经被打开的文件.
2013.07.17  open 如果带有 O_NONBLOCK 选项, 记录锁也是非阻塞的.
            支持 O_NOFOLLOW 选项.
2013.11.18  加入 dupminfd 支持.
2017.08.11  ioFullFileNameGet() 内部需要处理 errno.
*********************************************************************************************************/
#define  __SYLIXOS_STDARG
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
  VAR & MACRO
*********************************************************************************************************/
#define STD_VALID(fd)     (((fd) >= 0) && ((fd) < 3))
static INT _G_iIoStdFd[3] = {PX_ERROR, PX_ERROR, PX_ERROR};             /*  全局标准输入输出文件        */
/*********************************************************************************************************
** 函数名称: _IoOpen
** 功能描述: 创建或打开一个文件
** 输　入  : pcName                        文件名
**           iFlag                         方式         O_RDONLY  O_WRONLY  O_RDWR  O_CREAT
**           iMode                         UNIX MODE
** 输　出  : 文件描述符
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT _IoOpen (PCHAR            pcName,
                    INT              iFlag,
                    INT              iMode,
                    BOOL             bCreate)
{
             PLW_DEV_HDR    pdevhdrHdr;
             PLW_IO_ENV     pioeDef;
    
    REGISTER LONG           lValue;
    REGISTER INT            iFd = (PX_ERROR);
    
             CHAR           cFullFileName[MAX_FILENAME_LENGTH];
             
    REGISTER INT            iErrLevel  = 0;
             INT            iLinkCount = 0;
             
             ULONG          ulError = ERROR_NONE;
             
    pioeDef = _IosEnvGetDef();
    iMode = (iMode & (~pioeDef->IOE_modeUMask)) | S_IRUSR;              /*  必须保证所有者能读          */

    __THREAD_CANCEL_POINT();

    if (pcName == LW_NULL) {                                            /*  检查文件名                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        ulError   = EFAULT;                                             /*  Bad address                 */
        iErrLevel = 1;
        goto    __error_handle;
    }
    
    if (pcName[0] == PX_EOS) {
        ulError   = ENOENT;
        iErrLevel = 1;
        goto    __error_handle;
    }
    
    if (lib_strcmp(pcName, ".") == 0) {                                 /*  滤掉当前目录                */
        pcName++;
    }
    
    if (ioFullFileNameGet(pcName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
        ulError   = API_GetLastError();
        iErrLevel = 1;
        goto    __error_handle;
    }
    
    iFd = iosFdNew(pdevhdrHdr, cFullFileName, PX_ERROR, iFlag);         /*  此时就确定了文件描述符名    */
    if (iFd == PX_ERROR) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file can not create.\r\n");
        iErrLevel = 1;
        goto    __error_handle;
    }
    
    for (;;) {
        if (bCreate) {
            lValue = iosCreate(pdevhdrHdr, cFullFileName, iFlag, iMode);
        } else {
            lValue = iosOpen(pdevhdrHdr, cFullFileName, iFlag, iMode);
        }
        
        if (lValue == PX_ERROR) {
            ulError   = API_GetLastError();
            iErrLevel = 2;
            goto    __error_handle;
        }
        
        /*
         *  驱动程序如果返回 FOLLOW_LINK_????, cFullFileName内部一定是目标的绝对地址, 即以/起始的文件名.
         */
        if ((lValue != FOLLOW_LINK_FILE) && 
            (lValue != FOLLOW_LINK_TAIL)) {                             /*  非链接文件直接退出          */
            break;
        
        } else {
            if (((iFlag & O_NOFOLLOW) && (lValue == FOLLOW_LINK_FILE)) ||
                (iLinkCount++ > _S_iIoMaxLinkLevels)) {                 /*  不允许符号链接              */
                ulError   = ELOOP;                                      /*  链接文件层数太多            */
                iErrLevel = 2;
                goto    __error_handle;
            }
        }
        
        if (ioFullFileNameGet(cFullFileName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
            ulError   = EXDEV;
            iErrLevel = 2;
            goto    __error_handle;
        }
    }

    if (iosFdSet(iFd, pdevhdrHdr, lValue, iFlag) != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file can not config.\r\n");
        ulError   = API_GetLastError();
        iErrLevel = 3;
        goto    __error_handle;
    }

    if (iLinkCount) {                                                   /*  内部存在符号链接            */
        if (iosFdRealName(iFd, cFullFileName)) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "file can not set real name.\r\n");
            ulError   = API_GetLastError();
            iErrLevel = 3;
            goto    __error_handle;
        }
    }

    if ((iFlag & O_SHLOCK) || (iFlag & O_EXLOCK)) {                     /*  需要直接加锁                */
        REGISTER PLW_FD_ENTRY  pfdentry;
        pfdentry = _IosFileGet(iFd, LW_FALSE);
        if (pfdentry && (pfdentry->FDENTRY_iType == LW_DRV_TYPE_NEW_1)) {
            INT  iType = (iFlag & O_EXLOCK) ? LOCK_EX : LOCK_SH;
            if (iFlag & O_NONBLOCK) {
                iType |= LOCK_NB;                                       /*  非阻塞等待记录锁            */
            }
            if (_FdLockfProc(pfdentry, iType, __PROC_GET_PID_CUR())) {
                close(iFd);                                             /*  加锁失败                    */
                iFd = PX_ERROR;
            }
        }
    }
    
    if (iFd >= 0) {
        if (iFlag & O_CLOEXEC) {
            API_IosFdSetCloExec(iFd, FD_CLOEXEC);                       /*  需要 FD_CLOEXEC 操作        */
        }
        MONITOR_EVT_INT2(MONITOR_EVENT_ID_IO, (bCreate ? MONITOR_EVENT_IO_CREAT : MONITOR_EVENT_IO_OPEN), 
                         iFlag, iMode, pcName);
    }
    
    return  (iFd);
    
__error_handle:
    if ((iErrLevel > 2) && (iFlag & O_CREAT)) {
        /*
         *  这里目前不应该删除这个文件. 
         *  iosDelete(pdevhdrHdr, cFullFileName);
         */
    }
    if (iErrLevel > 1) {
        iosFdFree(iFd);
    }
    if (iErrLevel > 0) {
        iFd = (PX_ERROR);
    }
    
    _ErrorHandle(ulError);
    return  (iFd);
}
/*********************************************************************************************************
** 函数名称: mknod
** 功能描述: create a new file named by the pathname to which the argument path points.
** 输　入  : pcFifoName    目录名
**           mode          方式 (目前未使用)
**           dev           目前未使用
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  mknod (CPCHAR  pcNodeName, mode_t  mode, dev_t dev)
{
    REGISTER INT    iFd;
    
    iFd = open(pcNodeName, O_RDWR | O_CREAT | O_EXCL, mode | DEFAULT_DIR_PERM);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    close(iFd);
    
    return (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: creat
** 功能描述: 创建一个文件
** 输　入  : pcName                        文件名
**           iMode                         属性
** 输　出  : 文件描述符
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  creat (CPCHAR  cpcName, INT  iMode)
{
    return  (_IoOpen((PCHAR)cpcName, (O_WRONLY | O_CREAT | O_TRUNC), iMode, LW_TRUE));
}
/*********************************************************************************************************
** 函数名称: open
** 功能描述: 打开(创建)一个文件
** 输　入  : pcName                        文件名
**           iFlag                         方式         O_RDONLY  O_WRONLY  O_RDWR  O_CREAT ...
**           ...                           创建模式, 仅在新创建文件时, 此参数有效.
** 输　出  : 文件描述符
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  open (CPCHAR  cpcName, INT  iFlag, ...)
{
    INT         iMode = DEFAULT_FILE_PERM;
    INT         iRet;
    va_list     varlist;
    
    va_start(varlist, iFlag);
    if (iFlag & O_CREAT) {                                              /*  创建相关                    */
        iMode = va_arg(varlist, INT);
    }
    
    iRet = _IoOpen((PCHAR)cpcName, iFlag, iMode, LW_FALSE);

    va_end(varlist);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: open64
** 功能描述: 打开(创建)一个文件
** 输　入  : pcName                        文件名
**           iFlag                         方式         O_RDONLY  O_WRONLY  O_RDWR  O_CREAT ...
**           ...                           创建模式, 仅在新创建文件时, 此参数有效.
** 输　出  : 文件描述符
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  open64 (CPCHAR  cpcName, INT  iFlag, ...)
{
    INT         iMode = DEFAULT_FILE_PERM;
    INT         iRet;
    va_list     varlist;
    
    va_start(varlist, iFlag);
    if (iFlag & O_CREAT) {                                              /*  创建相关                    */
        iMode = va_arg(varlist, INT);
    }
    
    iRet = _IoOpen((PCHAR)cpcName, iFlag | O_LARGEFILE, iMode, LW_FALSE);

    va_end(varlist);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: unlink
** 功能描述: delete a file (ANSI: unlink() )
** 输　入  : 
**           pcName                        文件名
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  unlink (CPCHAR       pcName)
{
    PLW_DEV_HDR    pdevhdrHdr;
    CHAR           cFullFileName[MAX_FILENAME_LENGTH];
             
    INT            iRet;
    INT            iLinkCount = 0;

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    if (pcName[0] == PX_EOS) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    if (ioFullFileNameGet(pcName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    /*
     *  驱动中的 delete 函数一定要判断 cFullFileName 参数, 如果为链接文件则需要做两个判断
     *  cFullFileName 指向被链接对象的内部文件或目录时, 应该返回 FOLLOW_LINK_TAIL 用来删除
     *  被链接目标(真实目标)中的文件或目录, 当 cFullFileName 指向的是连接文件本身, 那么
     *  应该直接删除链接文件即可, 不应该返回 FOLLOW_LINK_???? 删除链接目标! 切记! 切记!
     */
    for (;;) {
        iRet = iosFdUnlink(pdevhdrHdr, cFullFileName);                  /*  检查正在打开的文件          */
        if (iRet == 1) {
            _ErrorHandle(EBUSY);                                        /*  文件被锁定, 不能删除        */
            return  (PX_ERROR);
        
        } else if (iRet == ERROR_NONE) {                                /*  删除操作延迟自动执行        */
            return  (ERROR_NONE);
        }
        
        iRet = iosDelete(pdevhdrHdr, cFullFileName);
        if (iRet != FOLLOW_LINK_TAIL) {                                 /*  非链接文件直接退出          */
            break;
        
        } else {
            if (iLinkCount++ > _S_iIoMaxLinkLevels) {                   /*  链接文件层数太多            */
                _ErrorHandle(ELOOP);
                return  (PX_ERROR);
            }
        }
        
        /*
         *  驱动程序如果返回 FOLLOW_LINK_????, cFullFileName内部一定是目标的绝对地址, 即以/起始的文件名.
         */
        if (ioFullFileNameGet(cFullFileName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
            _ErrorHandle(EXDEV);
            return  (PX_ERROR);
        }
    }
    
    MONITOR_EVT(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_UNLINK, pcName);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: rename
** 功能描述: rename a file (ANSI: rename() )
** 输　入  : 
**           pcOldName                     旧文件名
**           pcNewName                     新文件名
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  rename (CPCHAR       pcOldName, CPCHAR       pcNewName)
{
    REGISTER LONG   lValue;
    PLW_FD_ENTRY    pfdentry;
    PLW_DEV_HDR     pdevhdrHdr;
    CHAR            cFullFileName[MAX_FILENAME_LENGTH];
    
    INT             iLinkCount = 1;

    REGISTER INT    iFd;
    REGISTER INT    iRet;
             ULONG  ulErrNo;
    
    if ((pcOldName == LW_NULL) || (pcNewName == LW_NULL)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    iFd = open(pcOldName, O_RDONLY, 0);                                 /*  read or write only? XXX     */
    if (iFd < 0) {                                                      /*  open failed                 */
        return  (PX_ERROR);
    }
    
    if (ioFullFileNameGet(pcNewName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
        ulErrNo = API_GetLastError();
        close(iFd);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        _ErrorHandle(ulErrNo);
        return  (PX_ERROR);
    }
    
    pfdentry = _IosFileNew(pdevhdrHdr, cFullFileName);                  /*  创建一个临时的 fd_entry     */
    if (pfdentry == LW_NULL) {
        close(iFd);
        return  (PX_ERROR);
    }
    
    for (;;) {
        lValue = iosOpen(pdevhdrHdr, cFullFileName, O_RDONLY, 0);
        if ((lValue != FOLLOW_LINK_FILE) && 
            (lValue != FOLLOW_LINK_TAIL)) {                             /*  非链接文件直接退出          */
            break;
        
        } else {
            if (iLinkCount++ > _S_iIoMaxLinkLevels) {                   /*  链接文件层数太多            */
                _IosFileDelete(pfdentry);
                close(iFd);
                _ErrorHandle(ELOOP);
                return  (PX_ERROR);
            }
        }
    
        /*
         *  驱动程序如果返回 FOLLOW_LINK_????, cFullFileName内部一定是目标的绝对地址, 即以/起始的文件名.
         */
        if (ioFullFileNameGet(cFullFileName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
            _IosFileDelete(pfdentry);
            close(iFd);
            _ErrorHandle(EXDEV);
            return  (PX_ERROR);
        }
    }
    
    if (lValue != PX_ERROR) {
        _IosFileSet(pfdentry, pdevhdrHdr, lValue, O_RDONLY);
        _IosFileClose(pfdentry);                                        /*  关闭                        */
    }
    
    _IosFileDelete(pfdentry);                                           /*  删除临时的 fd_entry         */

    if (_PathBuildLink(cFullFileName, MAX_FILENAME_LENGTH, 
                       LW_NULL, LW_NULL,
                       pdevhdrHdr->DEVHDR_pcName, cFullFileName) < ERROR_NONE) {
        close(iFd);
        return  (PX_ERROR);
    }
    
    iRet = ioctl(iFd, FIORENAME, (LONG)cFullFileName);
    
    close(iFd);
    
    if (iRet == ERROR_NONE) {
        MONITOR_EVT(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_MOVE_FROM, pcOldName);
        MONITOR_EVT(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_MOVE_TO,   pcNewName);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: close
** 功能描述: close a file (ANSI: close() )
** 输　入  : 
**           iFd                           文件描述符
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  close (INT  iFd)
{
    REGISTER INT    iRet;
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    iRet = iosClose(iFd);
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: read
** 功能描述: read bytes from a file or device (ANSI: read() )
** 输　入  : 
**           iFd                           文件描述符
**           pvBuffer                      接收缓冲区
**           stMaxBytes                    接收缓冲区大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ssize_t  read (INT     iFd,
               PVOID   pvBuffer,
               size_t  stMaxBytes)
{
    REGISTER ssize_t  sstRet;
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    sstRet = iosRead(iFd, (PCHAR)pvBuffer, stMaxBytes);

    return  (sstRet);
}
/*********************************************************************************************************
** 函数名称: pread
** 功能描述: read bytes from a file or device with a given position (ANSI: pread() )
** 输　入  : 
**           iFd                           文件描述符
**           pvBuffer                      接收缓冲区
**           stMaxBytes                    接收缓冲区大小
**           oftPos                        指定位置
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ssize_t  pread (INT     iFd,
                PVOID   pvBuffer,
                size_t  stMaxBytes,
                off_t   oftPos)
{
    REGISTER ssize_t  sstRet;
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    sstRet = iosPRead(iFd, (PCHAR)pvBuffer, stMaxBytes, oftPos);

    return  (sstRet);
}
/*********************************************************************************************************
** 函数名称: pread64
** 功能描述: read bytes from a file or device with a given position (ANSI: pread() )
** 输　入  : 
**           iFd                           文件描述符
**           pvBuffer                      接收缓冲区
**           stMaxBytes                    接收缓冲区大小
**           oftPos                        指定位置
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ssize_t  pread64 (INT     iFd,
                  PVOID   pvBuffer,
                  size_t  stMaxBytes,
                  off64_t oftPos)
{
    return  (pread(iFd, pvBuffer, stMaxBytes, oftPos));
}
/*********************************************************************************************************
** 函数名称: write
** 功能描述: write bytes from a file or device (ANSI: write() )
** 输　入  : 
**           iFd                           文件描述符
**           pvBuffer                      包含脚要写入文件数据的缓冲区
**           stNBytes                      写入的字节数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ssize_t  write (INT     iFd,
                CPVOID  pvBuffer,
                size_t  stNBytes)
{
    REGISTER ssize_t  sstRet;
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    sstRet = iosWrite(iFd, (CPCHAR)pvBuffer, stNBytes);
    
    return  (sstRet);
}
/*********************************************************************************************************
** 函数名称: pwrite
** 功能描述: write bytes from a file or device with a given position (ANSI: pwrite() )
** 输　入  : 
**           iFd                           文件描述符
**           pvBuffer                      包含脚要写入文件数据的缓冲区
**           stNBytes                      写入的字节数
**           oftPos                        指定位置
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ssize_t  pwrite (INT     iFd,
                 CPVOID  pvBuffer,
                 size_t  stNBytes,
                 off_t   oftPos)
{
    REGISTER ssize_t  sstRet;
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    sstRet = iosPWrite(iFd, (CPCHAR)pvBuffer, stNBytes, oftPos);

    return  (sstRet);
}
/*********************************************************************************************************
** 函数名称: pwrite64
** 功能描述: write bytes from a file or device with a given position (ANSI: pwrite() )
** 输　入  : 
**           iFd                           文件描述符
**           pvBuffer                      包含脚要写入文件数据的缓冲区
**           stNBytes                      写入的字节数
**           oftPos                        指定位置
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ssize_t  pwrite64 (INT     iFd,
                   CPVOID  pvBuffer,
                   size_t  stNBytes,
                   off64_t oftPos)
{
    return  (pwrite(iFd, pvBuffer, stNBytes, oftPos));
}
/*********************************************************************************************************
** 函数名称: ioctl
** 功能描述: perform an I/O control function (  ANSI: ioctl() )
** 输　入  : 
**           iFd                           文件描述符
**           iFunction                     功能
**           lArg                          参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  ioctl (INT   iFd,
            INT   iFunction,
            ...)
{
    va_list     varlist;
    LONG        lArg;                                                   /*  can store a void *          */
    INT         iRet;
    
    va_start(varlist, iFunction);
    lArg = va_arg(varlist, LONG);
    
    iRet = iosIoctl(iFd, iFunction, lArg);
    
    va_end(varlist);
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: lseek
** 功能描述: set a file read/write pointer
** 输　入  : 
**           iFd                           文件描述符
**           oftOffset                     偏移量
**           iWhence                       定位基准
** 输　出  : 当前文件指针
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
off_t  lseek (INT      iFd,
              off_t    oftOffset,
              INT      iWhence)
{
             off_t      oftWhere;
             off_t      oftNBytes;
             
    REGISTER INT        iError;
    REGISTER off_t      oftRetVal = API_IosLseek(iFd, oftOffset, iWhence);
      struct stat       statFile;
             
    if (oftRetVal != PX_ERROR) {                                        /*  优先考虑驱动函数            */
        return  (oftRetVal);
    }
    
    switch (iWhence) {
    
    case SEEK_SET:
        /*
         *  以当前指针为指针, 直接设置指针.
         */
        if (oftOffset < 0) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        return  ((ioctl(iFd, FIOSEEK, (LONG)&oftOffset) == 0) ? oftOffset : (PX_ERROR));
        
    case SEEK_CUR:
        /*
         *  获得当前文件指针.
         */
        iError = ioctl(iFd, FIOWHERE, (LONG)&oftWhere);
        if (iError == (PX_ERROR)) {
            return  (PX_ERROR);
        }
        
        /*
         *  计算偏移量并重新设置文件指针.
         */
        oftOffset += oftWhere;
        if (oftOffset < 0) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        return  ((ioctl(iFd, FIOSEEK, (LONG)&oftOffset) == 0) ? oftOffset : (PX_ERROR));
    
    case SEEK_END:
        /*
         *  获得当前文件指针.
         */
        iError = ioctl(iFd, FIOWHERE, (LONG)&oftWhere);
        if (iError == (PX_ERROR)) {
            goto    __fiofstat;
        }
        
        /*
         *  获得剩余数据大小.
         */
        if (ioctl(iFd, FIONREAD64, (LONG)&oftNBytes) == (PX_ERROR)) {
            goto    __fiofstat;
        }
        
        oftOffset += (oftWhere + oftNBytes);
        if (oftOffset < 0) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        return  ((ioctl(iFd, FIOSEEK, (LONG)&oftOffset) == 0) ? oftOffset : (PX_ERROR));
        
__fiofstat:
        /*
         *  获得文件属性
         */
        iError = fstat(iFd, &statFile);
        if (iError == (PX_ERROR)) {
            return  (PX_ERROR);
        }
        
        /*
         *  计算新的指针位置并重新设置文件指针.
         */
        oftOffset += statFile.st_size;
        if (oftOffset < 0) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        return  ((ioctl(iFd, FIOSEEK, (LONG)&oftOffset) == 0) ? oftOffset : (PX_ERROR));
    
    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: lseek64
** 功能描述: set a file read/write pointer
** 输　入  : 
**           iFd                           文件描述符
**           oftOffset                     偏移量
**           iWhence                       定位基准
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
off64_t  lseek64 (INT      iFd,
                  off64_t  oftOffset,
                  INT      iWhence)
{
    return  (lseek(iFd, oftOffset, iWhence));
}
/*********************************************************************************************************
** 函数名称: llseek
** 功能描述: set a file read/write pointer
** 输　入  : 
**           iFd                           文件描述符
**           oftOffset                     偏移量
**           iWhence                       定位基准
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
off64_t  llseek (INT      iFd,
                 off64_t  oftOffset,
                 INT      iWhence)
{
    return  (lseek(iFd, oftOffset, iWhence));
}
/*********************************************************************************************************
** 函数名称: readv
** 功能描述: read data from a device into scattered buffers  (可以用来支持环形缓冲区)
** 输　入  : 
**           iFd                           文件描述符
**           piovec                        接收缓冲区列表
**           iIovcnt                       接收缓冲区列表中缓冲区的个数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ssize_t  readv (INT             iFd,
                struct iovec    *piovec,
                INT             iIovcnt)
{
    REGISTER INT          iI;
    REGISTER PCHAR        pcBuf;
    REGISTER ssize_t      sstBytesToRead;
    REGISTER ssize_t      sstTotalBytesRead = 0;
    REGISTER ssize_t      sstBytesRead;
             ssize_t      sstRet = 0;
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    for (iI = 0; iI < iIovcnt; iI++) {                                  /*  开始逐个缓冲区接收          */
    
        pcBuf          = (PCHAR)piovec[iI].iov_base;
        sstBytesToRead = (ssize_t)piovec[iI].iov_len;
        
        while (sstBytesToRead > 0) {                                    /*  当前缓冲区是否已满          */
            
            sstBytesRead = iosRead(iFd, pcBuf, (size_t)sstBytesToRead);
            
            if (sstBytesRead < 0) {                                     /*  出错                        */
                if (sstTotalBytesRead > 0) {
                    sstRet = sstTotalBytesRead;
                } else {
                    sstRet = PX_ERROR;
                }
                goto    __read_over;
            }
            
            if (sstBytesRead == 0) {                                    /*  文件已经没有数据            */
                sstRet = sstTotalBytesRead;
                goto    __read_over;
            }
            
            sstTotalBytesRead += sstBytesRead;
            sstBytesToRead    -= sstBytesRead;
            
            pcBuf += sstBytesRead;
        }
    }
    sstRet = sstTotalBytesRead;

__read_over:
    return  (sstRet);
}
/*********************************************************************************************************
** 函数名称: writev
** 功能描述: read data from a device into scattered buffers  (可以用来支持环形缓冲区)
** 输　入  : 
**           iFd                           文件描述符
**           piovec                        发送缓冲区列表
**           uiMaxBytes                    发送缓冲区列表中缓冲区的个数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ssize_t  writev (INT                    iFd,
                 const struct iovec    *piovec,
                 INT                    iIovcnt)
{
    REGISTER INT          iI;
    REGISTER PCHAR        pcData;
    REGISTER ssize_t      sstBytesToWrite;
    REGISTER ssize_t      sstTotalBytesWriten = 0;
    REGISTER ssize_t      sstBytesWriten;
             ssize_t      sstRet = 0;
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    for (iI = 0; iI < iIovcnt; iI++) {                                  /*  开始逐个缓冲区发送          */
        
        pcData          = (PCHAR)piovec[iI].iov_base;
        sstBytesToWrite = (ssize_t)piovec[iI].iov_len;
        
        while (sstBytesToWrite > 0) {
            
            sstBytesWriten = iosWrite(iFd, pcData, (size_t)sstBytesToWrite);
            
            if (sstBytesWriten < 0) {
                if (sstTotalBytesWriten > 0) {
                    sstRet = sstTotalBytesWriten;
                } else {
                    sstRet = PX_ERROR;
                }
                goto    __write_over;
            }
            
            sstTotalBytesWriten += sstBytesWriten;
            sstBytesToWrite     -= sstBytesWriten;
            
            pcData += sstBytesWriten;
        }
    }
    sstRet = sstTotalBytesWriten;
    
__write_over:
    return  (sstRet);
}
/*********************************************************************************************************
** 函数名称: API_IoFullFileNameGet
** 功能描述: 获得完整文件名
** 输　入  : 
**           pcPathName                    路径名
**           ppdevhdr                      设备头双指针 (获取该路径对应的设备)
**           pcFullFileName                完整路径名   (输出缓冲: 裁减掉设备名后, 完整的设备内名字字串)
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
** 注  意  :  pcFullFileName 输出缓冲至少要有 PATH_MAX + 1 个字节的空间!
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IoFullFileNameGet (CPCHAR         pcPathName,
                            PLW_DEV_HDR   *ppdevhdr,
                            PCHAR          pcFullFileName)
{
    size_t   stFullLen;
    PCHAR    pcTail;
    CHAR     cFullPathName[MAX_FILENAME_LENGTH];
    
    lib_bzero(cFullPathName, MAX_FILENAME_LENGTH);                      /*  CLEAR                       */
    
    if (_PathCat(_PathGetDef(), pcPathName, cFullPathName) != ERROR_NONE) {
        _ErrorHandle(ENAMETOOLONG);
        return  (PX_ERROR);
    }
    
    /*
     *  需要在结尾处理掉多余的 / 符号, 例如: /aaa/bbb/ccc/ 应为 /aaa/bbb/ccc
     */
    stFullLen = lib_strlen(cFullPathName);
    if (stFullLen > 1) {                                                /*  本身就是 "/" 不能略去       */
        if (cFullPathName[stFullLen - 1] == PX_DIVIDER) {
            cFullPathName[stFullLen - 1] =  PX_EOS;                     /*  去掉末尾的 /                */
        }
        _PathCondense(cFullPathName);                                   /*  去除 ../ ./                 */
    }
    
    *ppdevhdr = iosDevFind(cFullPathName, &pcTail);
    if ((*ppdevhdr) == LW_NULL) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    /*
     *  如果 ppdevhdr == rootfs dev header 则, pcTail 起始位置没有 '/' 字符, 
     *  此处需要修正由于根文件系统设备名为 "/" 产生的问题.
     */
    if (pcTail && ((*pcTail != PX_EOS) && (*pcTail != PX_DIVIDER))) {
        lib_strlcpy(&pcFullFileName[1], pcTail, MAX_FILENAME_LENGTH - 1);
        pcFullFileName[0] = PX_ROOT;
    
    } else {
        lib_strlcpy(pcFullFileName, pcTail, MAX_FILENAME_LENGTH);
    }
    
    return  ((INT)ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_IoPathCondense
** 功能描述: 将一个含有 . 和 .. 的目录压缩为标准路径
** 输　入  : pcPath            输入未压缩的路径
**           pcPathCondense    输出压缩后的路径
**           stSize            输出缓冲大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IoPathCondense (CPCHAR  pcPath, PCHAR  pcPathCondense, size_t  stSize)
{
    size_t   stFullLen;
    CHAR     cFullPathName[MAX_FILENAME_LENGTH];
    
    lib_bzero(cFullPathName, MAX_FILENAME_LENGTH);                      /*  CLEAR                       */
    
    if (_PathCat(_PathGetDef(), pcPath, cFullPathName) != ERROR_NONE) {
        _ErrorHandle(ENAMETOOLONG);
        return  (PX_ERROR);
    }
    
    /*
     *  需要在结尾处理掉多余的 / 符号, 例如: /aaa/bbb/ccc/ 应为 /aaa/bbb/ccc
     */
    stFullLen = lib_strlen(cFullPathName);
    if (stFullLen > 1) {                                                /*  本身就是 "/" 不能略去       */
        if (cFullPathName[stFullLen - 1] == PX_DIVIDER) {
            cFullPathName[stFullLen - 1] =  PX_EOS;                     /*  去掉末尾的 /                */
        }
        _PathCondense(cFullPathName);                                   /*  去除 ../ ./                 */
    }
    
    lib_strlcpy(pcPathCondense, cFullPathName, stSize);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_IoPrivateEnv
** 功能描述: 当前线程进入 io 私有环境 
             当前任务拥有私有的相对目录, 主要用于系统服务, 用户不要轻易使用, 
** 输　入  : NONE
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
** 注  意  : 此函数仅针对于内核线程, 进程内的线程将不起作用, 进程内的线程共享进程内统一的相对目录.
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IoPrivateEnv (VOID)
{
             PLW_CLASS_TCB    ptcbCur;
    REGISTER PLW_IO_ENV       pioe;

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  在中断中调用                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
    if (ptcbCur->TCB_pvIoEnv) {
        return  (ERROR_NONE);                                           /*  已经处于私有 IO 环境        */
    }
    
    pioe = _IosEnvCreate();                                             /*  创建 io 环境                */
    if (pioe == LW_NULL) {
        return  (PX_ERROR);
    }
    
    _IosEnvInherit(pioe);                                               /*  必须先继承才能保存 IO 环境  */
    
    __KERNEL_MODE_PROC(
        ptcbCur->TCB_pvIoEnv = (PVOID)pioe;                             /*  保存当前 IO 环境            */
    );
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_IoDefPathSet
** 功能描述: 设定当前工作路径 (必须是绝对路径, 以 "/" 作为起始)
** 输　入  : pcName                        新默认目录
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IoDefPathSet (CPCHAR       pcName)
{
    PCHAR    pcTail = (PCHAR)pcName;
    
    if (iosDevFind(pcName, &pcTail) == LW_NULL) {
        return  (PX_ERROR);
    }
    
    if (pcTail == pcName) {                                             /*  不是一个有效的设备名称      */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
    
    if (lib_strnlen(pcName, MAX_FILENAME_LENGTH) >= MAX_FILENAME_LENGTH) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_IO_NAME_TOO_LONG);
        return  (PX_ERROR);
    }
    
    lib_strcpy(_PathGetDef(), pcName);
    
    MONITOR_EVT(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_CHDIR, pcName);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_IoDefPathCat
** 功能描述: concatenate to current default path
** 输　入  : pcName                        连接新的默认目录
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_IoDefPathCat (CPCHAR  pcName)
{
    CHAR    cNewPath[MAX_FILENAME_LENGTH];
    PCHAR   pcTail;
    
    /*
     *  pcName 如果是设备则 cNewPath 为该设备, 否则将链接到 _PathGetDef() 后面.
     */
    if (_PathCat(_PathGetDef(), pcName, cNewPath) != ERROR_NONE) {
        _ErrorHandle(ENAMETOOLONG);
        return  (PX_ERROR);
    }

    /*
     *  压缩目录, 去掉 . 和 ..
     */
    _PathCondense(cNewPath);

    /*
     *  检查要压缩目录后的合法性
     */
    iosDevFind(cNewPath, &pcTail);
    if (pcTail == cNewPath) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
    
    lib_strlcpy(_PathGetDef(), cNewPath, MAX_FILENAME_LENGTH);          /*  变为新的当前目录            */
    
    MONITOR_EVT(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_CHDIR, cNewPath);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_IoDefPathGet
** 功能描述: get the current default path
** 输　入  : pcName                        获得默认目录
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_IoDefPathGet (PCHAR  pcName)
{
    lib_strcpy(pcName, _PathGetDef());
}
/*********************************************************************************************************
** 函数名称: chdir
** 功能描述: 设定当前工作路径 (可以为相对路径)
** 输　入  : pcName                        新默认目录
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  chdir (CPCHAR  pcName)
{
    struct stat   statGet;
    
    if (stat(pcName, &statGet) < ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    if (!S_ISDIR(statGet.st_mode)) {                                    /*  非目录                      */
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }
    
    return  (API_IoDefPathCat(pcName));
}
/*********************************************************************************************************
** 函数名称: cd
** 功能描述: 设定当前工作路径 (可以为相对路径)
** 输　入  : pcName                        链接目录
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  cd (CPCHAR  pcName)
{
    struct stat   statGet;
    
    if (stat(pcName, &statGet) < ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    if (!S_ISDIR(statGet.st_mode)) {                                    /*  非目录                      */
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }
    
    return  (API_IoDefPathCat(pcName));
}
/*********************************************************************************************************
** 函数名称: fchdir
** 功能描述: 根据文件描述符将文件设定为当前工作路径
** 输　入  : iFd                           文件描述符
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  fchdir (INT  iFd)
{
    REGISTER PLW_FD_ENTRY  pfdentry;
             struct stat   statGet;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    if (pfdentry == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    if ((pfdentry->FDENTRY_iFlag & O_WRONLY) ||
        (pfdentry->FDENTRY_iFlag & O_RDWR)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file is not read only.\r\n");
        _ErrorHandle(EACCES);
        return  (PX_ERROR);
    }
    
    if (fstat(iFd, &statGet) < ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    if (!S_ISDIR(statGet.st_mode)) {                                    /*  非 dir 文件描述符           */
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }
    
    return  (API_IoDefPathSet(pfdentry->FDENTRY_pcName));               /*  设置当前工作目录            */
}
/*********************************************************************************************************
** 函数名称: chroot
** 功能描述: 改变根目录, SylixOS 暂时不支持.
** 输　入  : pcPath                        根目录路径
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  chroot (CPCHAR  pcPath)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: getcwd
** 功能描述: get the current default path (POSIX)
** 输　入  : 
**           pcBuffer                      获得默认目录
**           stByteSize                    缓冲区大小
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PCHAR  getcwd (PCHAR  pcBuffer, size_t  stByteSize)
{
    if (stByteSize == 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "stByteSize invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    if (stByteSize < lib_strlen(_PathGetDef()) + 1) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "stByteSize invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    if (!pcBuffer) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pcBuffer invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    lib_strncpy(pcBuffer, _PathGetDef(), stByteSize);                   /*  use strncpy                 */
    
    return  (pcBuffer);
}
/*********************************************************************************************************
** 函数名称: getwd
** 功能描述: get the current default path
** 输　入  : pcName                        获得默认目录
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PCHAR  getwd (PCHAR  pcName)
{
    if (!pcName) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    lib_strcpy(pcName, _PathGetDef());
    return  (pcName);
}
/*********************************************************************************************************
** 函数名称: API_IoDefDevGet
** 功能描述: get current default device
** 输　入  : pcDevName                     获得设备名
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID  API_IoDefDevGet (PCHAR  pcDevName)
{
    REGISTER PLW_DEV_HDR        pdevhdrHdr;
             PCHAR              pcTail;
             
    pdevhdrHdr = iosDevFind(_PathGetDef(), &pcTail);
    if (pdevhdrHdr == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "default device invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        *pcDevName = PX_EOS;
    
    } else {
        lib_strcpy(pcDevName, pdevhdrHdr->DEVHDR_pcName);
    }
}
/*********************************************************************************************************
** 函数名称: API_IoDefDirGet
** 功能描述: get current default directory
** 输　入  : pcDirName                     获得目录名
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID  API_IoDefDirGet (PCHAR  pcDirName)
{
    PCHAR        pcTail;
    
    if (iosDevFind(_PathGetDef(), &pcTail) == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "dir invalidate.\r\n");
        _ErrorHandle(ENOENT);
        *pcDirName = PX_EOS;
    
    } else {
        lib_strcpy(pcDirName, pcTail);
    }
}
/*********************************************************************************************************
** 函数名称: API_IoGlobalStdSet
** 功能描述: set the file descriptor for global standard input/output/error
** 输　入  : 
**           iStdFd                        索引号     STD_IN  STD_OUT  STD_ERR
**           iNewFd                        文件描述符
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID  API_IoGlobalStdSet (INT  iStdFd, INT  iNewFd)
{
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  非内核任务不支持            */
        _ErrorHandle(EINVAL);
        return;
    }

    if (STD_VALID(iStdFd)) {
        _G_iIoStdFd[iStdFd] = iNewFd;
    }
}
/*********************************************************************************************************
** 函数名称: API_IoGlobalStdGet
** 功能描述: get the file descriptor for global standard input/output/error
** 输　入  : 
**           iStdFd                        索引号    STD_IN  STD_OUT  STD_ERR
** 输　出  : 文件描述符
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_IoGlobalStdGet (INT  iStdFd)
{
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  内核任务                    */
        return  (iStdFd);                                               /*  直接返回文件描述符          */
    }

    return  ((STD_VALID(iStdFd) ? (_G_iIoStdFd[iStdFd]) : (PX_ERROR)));
}
/*********************************************************************************************************
** 函数名称: API_IoTaskStdSet
** 功能描述: set the file descriptor for task standard input/output/error
** 输　入  : 
**           ulId                          任务句柄
**           iStdFd                        索引号    STD_IN  STD_OUT  STD_ERR
**           iNewFd                        文件描述符
** 输　出  : 
** 全局变量: 
** 调用模块: 
** 注  意  : 只能用作内核任务, 进程内线程不支持.
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID  API_IoTaskStdSet (LW_OBJECT_HANDLE  ulId,
                        INT               iStdFd,
                        INT               iNewFd)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
    
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  非内核任务不支持            */
        _ErrorHandle(EINVAL);
        return;
    }
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return;
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        _ErrorHandle(ERROR_THREAD_NULL);
        return;
    }
#endif
    
    iregInterLevel = __KERNEL_ENTER_IRQ();
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return;
    }
    
    if (STD_VALID(iStdFd)) {
        ptcb = __GET_TCB_FROM_INDEX(usIndex);
        ptcb->TCB_iTaskStd[iStdFd] = iNewFd;
    }
    __KERNEL_EXIT_IRQ(iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: API_IoTaskStdGet
** 功能描述: get the file descriptor for task standard input/output/error
** 输　入  : 
**           ulId                          任务句柄
**           iStdFd                        索引号    STD_IN  STD_OUT  STD_ERR
** 输　出  : 文件描述符
** 全局变量: 
** 调用模块: 
** 注  意  : 在进程中可获取当前任务标准 IO 设置信息, 如果进程是有内核任务创建, 则继承内核任务 0 1 2 文件.

                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_IoTaskStdGet (LW_OBJECT_HANDLE  ulId,
                       INT               iStdFd)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
    
    REGISTER INT                   iTaskFd;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (PX_ERROR);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (PX_ERROR);
    }
#endif
    
    iregInterLevel = __KERNEL_ENTER_IRQ();
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (PX_ERROR);
    }
    
    if (STD_VALID(iStdFd)) {
        ptcb = __GET_TCB_FROM_INDEX(usIndex);
        iTaskFd = ptcb->TCB_iTaskStd[iStdFd];
        
        if (STD_VALID(iTaskFd)) {
            __KERNEL_EXIT_IRQ(iregInterLevel);
            return  (_G_iIoStdFd[iTaskFd]);
        
        } else {
            __KERNEL_EXIT_IRQ(iregInterLevel);
            return  (iTaskFd);
        }
    }
    __KERNEL_EXIT_IRQ(iregInterLevel);
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: isatty
** 功能描述: return whether the underlying driver is a tty device
** 输　入  : 
**           iFd                           文件描述符
** 输　出  : BOOL                          是否是tty设备
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
BOOL  isatty (INT  iFd)
{   
    REGISTER INT     iRet;
             BOOL    bIsTty;
    
    iRet = ioctl(iFd, FIOISATTY, (LONG)&bIsTty);
    
    if ((iRet < 0) || (bIsTty == LW_FALSE)) {
        return  (LW_FALSE);
    } else {
        return  (LW_TRUE);
    }
}
/*********************************************************************************************************
** 函数名称: ttyname
** 功能描述: 返回当前终端设备的名称.
** 输　入  : 
**           iFd                           文件描述符
** 输　出  : 终端的名称, 如果非终端, 或者文件描述符错误, 则返回 NULL.
**           errno = EBADF   表示文件非法
**           errno = ENOTTY  该文件描述符非终端设备描述符
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
PCHAR   ttyname (INT  iFd)
{
    REGISTER BOOL          bIsTty;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    bIsTty = isatty(iFd);
    if (bIsTty == LW_FALSE) {
        errno = ENOTTY;
        return  (LW_NULL);
    }
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    return  (pfdentry->FDENTRY_pdevhdrHdr->DEVHDR_pcName);              /*  使用设备名                  */
}
/*********************************************************************************************************
** 函数名称: ttyname_r
** 功能描述: 返回当前终端设备的名称 (可重入).
** 输　入  : 
**           iFd                           文件描述符
**           pcBuffer                      名字缓冲
**           stLen                         缓冲大小
** 输　出  : 终端的名称, 如果非终端, 或者文件描述符错误, 则返回 NULL.
**           errno = EBADF   表示文件非法
**           errno = EINVAL  参数错误
**           errno = ENOTTY  该文件描述符非终端设备描述符
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
PCHAR  ttyname_r (INT  iFd, PCHAR  pcBuffer, size_t  stLen)
{
    REGISTER BOOL          bIsTty;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    if ((pcBuffer == LW_NULL) ||
        (stLen    == 0)) {
        errno = EINVAL;
        return  (LW_NULL);
    }
    
    bIsTty = isatty(iFd);
    if (bIsTty == LW_FALSE) {
        errno = ENOTTY;
        return  (LW_NULL);
    }

    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    _IosLock();                                                         /*  进入 IO 临界区              */
    lib_strlcpy(pcBuffer, pfdentry->FDENTRY_pdevhdrHdr->DEVHDR_pcName, (INT)stLen);
    _IosUnlock();                                                       /*  退出 IO 临界区              */
    
    return  (pcBuffer);
}
/*********************************************************************************************************
** 函数名称: dup2kernel
** 功能描述: 从进程里复制一个文件描述符到内核里, 
**           并且共享同一个文件表项.
** 输　入  : iFd                           文件描述符
** 输　出  : >=0 表示正确的文件描述符
**           -1 表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  dup2kernel (INT  iFd)
{
    REGISTER PLW_FD_ENTRY  pfdentry;
             INT           iFdNew;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    _IosLock();                                                         /*  进入 IO 临界区              */
    __KERNEL_SPACE_ENTER();
    iFdNew = _IosFileDup(pfdentry, 0);
    __KERNEL_SPACE_EXIT();
    if (iFdNew < 0) {
        _IosUnlock();                                                   /*  退出 IO 临界区              */
        _ErrorHandle(EMFILE);
        return  (PX_ERROR);
    }
    __LW_FD_CREATE_HOOK(iFdNew, 0);                                     /*  内核文件描述符              */
    _IosUnlock();                                                       /*  退出 IO 临界区              */
    
    MONITOR_EVT_INT3(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_DUP, iFd, iFdNew, 1, LW_NULL);
    
    return  (iFdNew);
}
/*********************************************************************************************************
** 函数名称: dupminfd
** 功能描述: 复制一个文件描述符, 由dup函数返回的文件描述符一定是当前可用文件描述符中的最小数值
**           并且共享同一个文件表项.
** 输　入  : iFd                           文件描述符
**           iMinFd                        最小文件描述符
** 输　出  : >=0 表示正确的文件描述符
**           -1 表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  dupminfd (INT  iFd, INT  iMinFd)
{
    REGISTER PLW_FD_ENTRY  pfdentry;
             INT           iFdNew;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    _IosLock();                                                         /*  进入 IO 临界区              */
    iFdNew = _IosFileDup(pfdentry, iMinFd);
    if (iFdNew < 0) {
        _IosUnlock();                                                   /*  退出 IO 临界区              */
        _ErrorHandle(EMFILE);
        return  (PX_ERROR);
    }
    __LW_FD_CREATE_HOOK(iFdNew, __PROC_GET_PID_CUR());
    _IosUnlock();                                                       /*  退出 IO 临界区              */
    
    MONITOR_EVT_INT3(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_DUP, iFd, iFdNew, 0, LW_NULL);
    
    return  (iFdNew);
}
/*********************************************************************************************************
** 函数名称: dup
** 功能描述: 复制一个文件描述符, 由dup函数返回的文件描述符一定是当前可用文件描述符中的最小数值
**           并且共享同一个文件表项.
** 输　入  : iFd                           文件描述符
** 输　出  : >=0 表示正确的文件描述符
**           -1 表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  dup (INT  iFd)
{
    return  (dupminfd(iFd, 0));
}
/*********************************************************************************************************
** 函数名称: dup2
** 功能描述: POSIX dup2()
** 输　入  : iFd1                          文件描述符1
**           iFd2                          文件描述符2
** 输　出  : >=0 表示正确的文件描述符
**           -1 表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  dup2 (INT  iFd1, INT  iFd2)
{
    REGISTER PLW_FD_ENTRY  pfdentry1;
    REGISTER PLW_FD_ENTRY  pfdentry2;
    
    if (__PROC_GET_PID_CUR() == 0) {                                    /*  dup2 只在进程中被支持       */
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    if (iFd2 == iFd1) {                                                 /*  期望的文件描述符相同        */
        _ErrorHandle(ENOTSUP);                                          /*  不支持                      */
        return  (PX_ERROR);
    }
    
    pfdentry1 = _IosFileGet(iFd1, LW_FALSE);
    pfdentry2 = _IosFileGet(iFd2, LW_FALSE);
    
    if ((pfdentry1 == LW_NULL) || (pfdentry1->FDENTRY_ulCounter == 0)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    if (pfdentry1 == pfdentry2) {
        return  (iFd2);
    }
    
__re_check:                                                             /*  重新检查 FD2 是否被关闭     */
    _IosLock();                                                         /*  进入 IO 临界区              */
    pfdentry2 = _IosFileGet(iFd2, LW_FALSE);
    if (pfdentry2) {
        INT  iRef = _IosFileRefGet(iFd2);
        if (iRef > 1) {
            _IosUnlock();                                               /*  退出 IO 临界区              */
            _ErrorHandle(EBUSY);
            return  (PX_ERROR);
        }
        _IosUnlock();                                                   /*  退出 IO 临界区              */
        close(iFd2);                                                    /*  关闭 fd2                    */
        goto    __re_check;
    }
    iFd2 = _IosFileDup2(pfdentry1, iFd2);
    if (iFd2 < 0) {
        _IosUnlock();                                                   /*  退出 IO 临界区              */
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    __LW_FD_CREATE_HOOK(iFd2, __PROC_GET_PID_CUR());
    _IosUnlock();                                                       /*  退出 IO 临界区              */
    
    MONITOR_EVT_INT3(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_DUP, iFd1, iFd2, 0, LW_NULL);
    
    return  (iFd2);
}
/*********************************************************************************************************
  注意: 一下函数 sylixos 未使用, 仅限于兼容 linux 程序.
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: makedev
** 功能描述: sylixos 未使用, 仅限于兼容 linux 程序
** 输　入  : 
**           major      主设备号
**           minor      从设备号
** 输　出  : dev_t
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
dev_t   makedev (uint_t  major, uint_t minor)
{
    return  (dev_t)((major << 20) | minor);
}
/*********************************************************************************************************
** 函数名称: major
** 功能描述: sylixos 未使用, 仅限于兼容 linux 程序
** 输　入  : 
**           dev        设备号
** 输　出  : 主设备号
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
uint_t  major (dev_t dev)
{
    return  (uint_t)(dev >> 20);
}
/*********************************************************************************************************
** 函数名称: minor
** 功能描述: sylixos 未使用, 仅限于兼容 linux 程序
** 输　入  : 
**           dev        设备号
** 输　出  : 从设备号
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
uint_t  minor (dev_t dev)
{
    return  (uint_t)(dev & 0xfffff);
}
/*********************************************************************************************************
** 函数名称: realpath
** 功能描述: 此函数用来将参数 pcPath 所指的相对路径转换成绝对路径后存于参数 pcResolvedPath 
             所指的字符串数组或指针中 (暂时还不支持替换内部的符号链接)
** 输　入  : pcPath             输入路径
**           pcResolvedPath     获取绝对路径保存地址 (注意, 至少保证 PATH_MAX + 1 个字节)
** 输　出  : 绝对路径
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
PCHAR  realpath (CPCHAR  pcPath, PCHAR  pcResolvedPath)
{
    CHAR            cFullPathName[MAX_FILENAME_LENGTH];
    PLW_DEV_HDR     pdevhdrHdr;
    size_t          stDevNameLen;

    if (!pcPath || !pcResolvedPath) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    if (ioFullFileNameGet(pcPath, &pdevhdrHdr, cFullPathName) != ERROR_NONE) {
        return  (LW_NULL);
    }
    
    if (cFullPathName[0] == PX_EOS) {
        lib_strcpy(pcResolvedPath, pdevhdrHdr->DEVHDR_pcName);
    
    } else {
        stDevNameLen = lib_strlen(pdevhdrHdr->DEVHDR_pcName);
        if (stDevNameLen == 1) {                                        /*  根目录设备 "/"              */
            lib_strcpy(pcResolvedPath, cFullPathName);                  /*  保存绝对路径文件名          */
        
        } else {
            lib_strcpy(pcResolvedPath, pdevhdrHdr->DEVHDR_pcName);
            lib_strcpy(&pcResolvedPath[stDevNameLen], cFullPathName);
        }
    }
    
    return  (pcResolvedPath);
}
/*********************************************************************************************************
** 函数名称: flock
** 功能描述: 老式文件锁函数, 锁定整个文件, 如果需要文件字节锁, 则需要使用 fcntl 函数.
** 输　入  : iFd                文件描述符
**           iOperation         锁定参数 LOCK_SH / LOCK_EX / LOCK_UN ( | LOCK_NB 表示不阻塞)
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  flock (INT iFd, INT iOperation)
{
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    return  (_FdLockfProc(pfdentry, iOperation, __PROC_GET_PID_CUR()));
}
/*********************************************************************************************************
** 函数名称: lockf
** 功能描述: POSIX 提供的另一种文件锁
** 输　入  : iFd                文件描述符
**           iCmd               F_ULOCK     Unlock locked sections.
**                              F_LOCK      Lock a section for exclusive use.
**                              F_TLOCK     Test and lock a section for exclusive use.
**                              F_TEST      Test a section for locks by other processes.
**           oftLen             要锁定的资源从文件中当前偏移量开始，
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 这里没有判断是否拥有写权限
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  lockf (INT iFd, INT iCmd, off_t oftLen)
{
             INT           iIoCtlCmd;
             INT           iError;
      struct flock         fl;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    fl.l_whence = SEEK_CUR;
    fl.l_start  = 0;
    fl.l_len    = oftLen;
    fl.l_pid    = __PROC_GET_PID_CUR();
    
    switch (iCmd) {
    
    case F_ULOCK:
        fl.l_type = F_UNLCK;
        iIoCtlCmd = FIOSETLK;                                           /*  设置区域为解锁状态          */
        break;
        
    case F_LOCK:
        fl.l_type = F_WRLCK;
        iIoCtlCmd = FIOSETLKW;                                          /*  设置区域为锁定状态          */
        break;
        
    case F_TLOCK:
        fl.l_type = F_WRLCK;
        iIoCtlCmd = FIOSETLK;                                           /*  非阻塞的锁定                */
        break;
    
    case F_TEST:
        fl.l_type = F_WRLCK;
        iIoCtlCmd = FIOGETLK;                                           /*  获得指定区域的锁状态        */
        break;
    
    default:
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iError = _FdLockfIoctl(pfdentry, iIoCtlCmd, (struct flock *)&fl);
    if ((iError == ERROR_NONE) && (iCmd == F_TEST)) {
        if (fl.l_type == F_UNLCK) {                                     /*  该区域可以访问              */
            return  (ERROR_NONE);
        
        } else {                                                        /*  此区域不允许访问            */
            _ErrorHandle(EACCES);
            return  (PX_ERROR);
        }
    } else {
        return  (iError);
    }
}
/*********************************************************************************************************
** 函数名称: umask
** 功能描述: 设置文件创建屏蔽字
** 输　入  : modeMask            新的屏蔽字
** 输　出  : 原先的屏蔽字
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
mode_t  umask (mode_t modeMask)
{
    PLW_IO_ENV  pioeDef = _IosEnvGetDef();
    mode_t      modeOld;
    
    __KERNEL_ENTER();
    modeOld = pioeDef->IOE_modeUMask;
    pioeDef->IOE_modeUMask = modeMask & 0777;                           /*  只能操作 rwxrwxrwx 位       */
    __KERNEL_EXIT();
    
    return  (modeOld);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
