/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: s_api.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 13 日
**
** 描        述: 这是系统综合 API 。

** BUG
2007.04.08  加入了对裁剪的宏支持
2007.11.21  加入 API_IosDevFileClose() 函数,用于自定义设备删除时使用.
2009.02.08  去掉与 stdio 冲突的 API
2012.12.21  去掉 API_IosDevFileClose() 必须使用 API_IosDevFileAbnormal().
*********************************************************************************************************/

#ifndef  __S_API_H
#define  __S_API_H

/*********************************************************************************************************
  DEVICE
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

#if LW_CFG_DEVICE_EN > 0
LW_API INT            API_IosDrvInstall(LONGFUNCPTR    pfuncCreate,
                                        FUNCPTR        pfuncDelete,
                                        LONGFUNCPTR    pfuncOpen,
                                        FUNCPTR        pfuncClose,
                                        SSIZETFUNCPTR  pfuncRead,
                                        SSIZETFUNCPTR  pfuncWrite,
                                        FUNCPTR        pfuncIoctl);     /*  安装驱动程序                */

LW_API INT            API_IosDrvInstallEx(struct file_operations  *pfileop);
                                                                        /*  安装驱动程序                */

LW_API INT            API_IosDrvInstallEx2(struct file_operations  *pfileop, INT  iType);
                                                                        /*  安装驱动程序                */

LW_API ULONG          API_IosDrvRemove(INT  iDrvNum, BOOL  bForceClose);/*  卸载驱动程序                */

LW_API ULONG          API_IosDrvGetType(INT  iDrvNum, INT  *piType);    /*  获取驱动程序类型            */

LW_API INT            API_IosDevFileAbnormal(PLW_DEV_HDR    pdevhdrHdr);/*  设备相关文件转为异常模式    */

LW_API ULONG          API_IosDevAdd(PLW_DEV_HDR    pdevhdrHdr,
                                    CPCHAR         pcName,
                                    INT            iDrvNum);            /*  安装设备                    */

LW_API ULONG          API_IosDevAddEx(PLW_DEV_HDR    pdevhdrHdr,
                                      CPCHAR         pcName,
                                      INT            iDrvNum,
                                      UCHAR          ucType);           /*  安装设备(指定设备 d_type)   */
                                                                        /*  详情见 dirent.h             */

LW_API VOID           API_IosDevDelete(PLW_DEV_HDR    pdevhdrHdr);      /*  删除设备                    */

LW_API PLW_DEV_HDR    API_IosDevFind(CPCHAR  pcName, 
                                     PCHAR  *ppcNameTail);              /*  查找设备                    */

LW_API INT            API_IoFullFileNameGet(CPCHAR         pcPathName,
                                            PLW_DEV_HDR   *ppdevhdr,
                                            PCHAR          pcFullFileName);
                                                                        /*  获得完整文件名              */
#endif

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  POSIX & ANSI
*********************************************************************************************************/

#if LW_CFG_DEVICE_EN > 0
LW_API INT            API_IoPathCondense(CPCHAR  pcPath, 
                                         PCHAR   pcPathCondense, 
                                         size_t  stSize);               /*  目录压缩                    */

LW_API INT            API_IoPrivateEnv(VOID);                           /*  线程进入私有 IO 环境        */
                                                                        /*  例如: 独立的相对路径        */

LW_API INT            API_IoDefPathSet(CPCHAR  pcName);                 /*  设置系统默认目录            */

LW_API VOID           API_IoDefPathGet(PCHAR  pcName);

LW_API INT            API_IoDefPathCat(CPCHAR  pcName);

LW_API VOID           API_IoDefDevGet(PCHAR  pcDevName);

LW_API VOID           API_IoDefDirGet(PCHAR  pcDirName);

LW_API VOID           API_IoGlobalStdSet(INT  iStdFd, INT  iNewFd);

LW_API INT            API_IoGlobalStdGet(INT  iStdFd);

LW_API VOID           API_IoTaskStdSet(LW_OBJECT_HANDLE  ulId,
                                       INT               iStdFd,
                                       INT               iNewFd);

LW_API INT            API_IoTaskStdGet(LW_OBJECT_HANDLE  ulId,
                                       INT               iStdFd);
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */

/*********************************************************************************************************
  VxWorks SHOW
*********************************************************************************************************/

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
LW_API VOID           API_IoDrvShow(VOID);

LW_API VOID           API_IoDrvLicenseShow(VOID);

LW_API VOID           API_IoDevShow(INT  iShowType);

LW_API VOID           API_IoFdShow(VOID);

LW_API VOID           API_IoFdentryShow(VOID);
#endif

/*********************************************************************************************************
  Driver License
*********************************************************************************************************/

#if LW_CFG_DEVICE_EN > 0
LW_API INT                   API_IoSetDrvLicense(INT  iDrvNum, PCHAR  pcLicense);

LW_API PCHAR                 API_IoGetDrvLicense(INT  iDrvNum);

LW_API INT                   API_IoSetDrvAuthor(INT  iDrvNum, PCHAR  pcAuthor);

LW_API PCHAR                 API_IoGetDrvAuthor(INT  iDrvNum);

LW_API INT                   API_IoSetDrvDescription(INT  iDrvNum, PCHAR  pcDescription);

LW_API PCHAR                 API_IoGetDrvDescription(INT  iDrvNum);
#endif

/*********************************************************************************************************
  API
*********************************************************************************************************/

#define iosDrvInstall                            API_IosDrvInstall
#define iosDrvInstallEx                          API_IosDrvInstallEx
#define iosDrvInstallEx2                         API_IosDrvInstallEx2
#define iosDrvRemove                             API_IosDrvRemove
#define iosDrvGetType                            API_IosDrvGetType

#define iosDevAdd                                API_IosDevAdd
#define iosDevAddEx                              API_IosDevAddEx
#define iosDevDelete                             API_IosDevDelete
#define iosDevFileAbnormal                       API_IosDevFileAbnormal
#define iosDevFind                               API_IosDevFind

/*********************************************************************************************************
  File name
*********************************************************************************************************/

#if LW_CFG_DEVICE_EN > 0
LW_API INT                   API_IosFdGetName(INT  iFd, PCHAR  pcName, size_t  stSize);
LW_API INT                   API_IosFdGetRealName(INT  iFd, PCHAR  pcName, size_t  stSize);

#define iosFdGetName                             API_IosFdGetName
#define iosFdGetRealName                         API_IosFdGetRealName
#endif

/*********************************************************************************************************
  IO system kernel FILE
*********************************************************************************************************/

#define mkfs                                     diskformat
#define ioPrivateEnv                             API_IoPrivateEnv

#define ioFullFileNameGet                        API_IoFullFileNameGet
#define ioPathCondense                           API_IoPathCondense

#define ioDefPathSet                             API_IoDefPathSet
#define ioDefPathGet                             API_IoDefPathGet

#define ioDefPathCat                             API_IoDefPathCat
#define ioDefDevGet                              API_IoDefDevGet
#define ioDefDirGet                              API_IoDefDirGet

#define ioGlobalStdSet                           API_IoGlobalStdSet
#define ioGlobalStdGet                           API_IoGlobalStdGet

#define ioTaskStdSet                             API_IoTaskStdSet
#define ioTaskStdGet                             API_IoTaskStdGet

/*********************************************************************************************************
  POSIX API
*********************************************************************************************************/

#if LW_CFG_DEVICE_EN > 0
LW_API INT              mknod(CPCHAR  pcNodeName, mode_t  mode, dev_t dev);
LW_API INT              creat(CPCHAR  cpcName, INT  iMode);
LW_API INT              open(CPCHAR       cpcName,
                             INT          iFlag,
                             ...);
LW_API INT              open64(CPCHAR       cpcName,
                               INT          iFlag,
                               ...);
LW_API INT              unlink(CPCHAR       pcName);
LW_API INT              rename(CPCHAR       pcOldName, 
                               CPCHAR       pcNewName);
LW_API INT              close(INT  iFd);

LW_API ssize_t          read(INT     iFd,
                             PVOID   pvBuffer,
                             size_t  stMaxBytes);
LW_API ssize_t          pread(INT     iFd,
                              PVOID   pvBuffer,
                              size_t  stMaxBytes,
                              off_t   oftPos);
LW_API ssize_t          pread64(INT     iFd,
                                PVOID   pvBuffer,
                                size_t  stMaxBytes,
                                off64_t oftPos);
LW_API ssize_t          write(INT     iFd,
                              CPVOID  pvBuffer,
                              size_t  stNBytes);
LW_API ssize_t          pwrite(INT     iFd,
                               CPVOID  pvBuffer,
                               size_t  stNBytes,
                               off_t   oftPos);
LW_API ssize_t          pwrite64(INT     iFd,
                                 CPVOID  pvBuffer,
                                 size_t  stNBytes,
                                 off64_t oftPos);
LW_API INT              ioctl(INT   iFd,
                              INT   iFunction,
                              ...);
LW_API off_t            lseek(INT      iFd,
                              off_t    oftOffset,
                              INT      iWhence);
LW_API off64_t          lseek64(INT        iFd,
                                off64_t    oftOffset,
                                INT        iWhence);
LW_API off64_t          llseek(INT        iFd,
                               off64_t    oftOffset,
                               INT        iWhence);
                              
LW_API ssize_t          readv(INT             iFd,
                              struct iovec    *piovec,
                              INT             iIovcnt);
LW_API ssize_t          writev(INT                    iFd,
                               const struct iovec    *piovec,
                               INT                    iIovcnt);

LW_API INT              chdir(CPCHAR  pcName);
LW_API INT              cd(CPCHAR  pcName);
LW_API INT              fchdir(INT  iFd);
LW_API INT              chroot(CPCHAR  pcPath);

LW_API PCHAR            getcwd(PCHAR  pcBuffer, size_t  stByteSize);
LW_API PCHAR            getwd(PCHAR  pcName);

LW_API BOOL             isatty(INT  iFd);
LW_API PCHAR            ttyname(INT  iFd);
LW_API PCHAR            ttyname_r(INT  iFd, PCHAR  pcBuffer, size_t  stLen);

LW_API INT              dup2kernel(INT  iFd);
#ifdef __SYLIXOS_KERNEL
LW_API INT              dupminfd(INT  iFd, INT  iMinFd);
#endif
LW_API INT              dup(INT  iFd);
LW_API INT              dup2(INT  iFd1, INT  iFd2);

LW_API INT              diskformat(CPCHAR  pcDevName);
LW_API INT              diskinit(CPCHAR  pcDevName);

LW_API INT              utime(CPCHAR  pcFile, const struct utimbuf *utimbNew);

LW_API INT              access(CPCHAR  pcName, INT  iMode);

LW_API INT              fstat(INT  iFd, struct stat *pstat);
LW_API INT              stat(CPCHAR pcName, struct stat *pstat);
LW_API INT              lstat(CPCHAR  pcName, struct stat *pstat);
LW_API INT              fstat64(INT  iFd, struct stat64 *pstat64);
LW_API INT              stat64(CPCHAR pcName, struct stat64 *pstat64);
LW_API INT              lstat64(CPCHAR  pcName, struct stat64 *pstat64);

LW_API INT              fstatfs(INT  iFd, struct statfs *pstatfs);
LW_API INT              statfs(CPCHAR pcName, struct statfs *pstatfs);

LW_API INT              ftruncate(INT  iFd, off_t  oftLength);
LW_API INT              truncate(CPCHAR  pcName, off_t  oftLength);
LW_API INT              ftruncate64(INT  iFd, off64_t  oftLength);
LW_API INT              truncate64(CPCHAR  pcName, off64_t  oftLength);

LW_API VOID             sync(VOID);
LW_API INT              fsync(INT  iFd);
LW_API INT              fdatasync(INT  iFd);

LW_API INT              fchmod(INT  iFd, INT  iMode);
LW_API INT              chmod(CPCHAR  pcName, INT  iMode);

LW_API INT              fchown(INT  iFd, uid_t uid, gid_t gid);
LW_API INT              chown(CPCHAR  pcName, uid_t uid, gid_t gid);
LW_API INT              lchown(CPCHAR  pcName, uid_t uid, gid_t gid);

LW_API INT              mkdir(CPCHAR  dirname, mode_t  mode);
LW_API INT              rmdir(CPCHAR  pathname); 

LW_API INT              dirfd(DIR  *pdir);
LW_API DIR             *opendir(CPCHAR   pathname);  
LW_API INT              closedir(DIR    *dir);
LW_API struct dirent   *readdir(DIR     *dir);
LW_API INT              readdir_r(DIR             *pdir, 
                                  struct dirent   *pdirentEntry,
                                  struct dirent  **ppdirentResult);
LW_API INT              rewinddir(DIR   *dir);

LW_API INT              mkfifo(CPCHAR  pcFifoName, mode_t  mode);
LW_API INT              pipe(INT  iFd[2]);
LW_API INT              pipe2(INT  iFd[2], INT  iFlag);

LW_API INT              link(CPCHAR  pcLinkDst, CPCHAR  pcSymPath);
LW_API INT              symlink(CPCHAR   pcActualPath, CPCHAR   pcSymPath);
LW_API ssize_t          readlink(CPCHAR   pcSymPath, PCHAR  pcBuffer, size_t  iSize);

LW_API PCHAR            realpath(CPCHAR  pcPath, PCHAR  pcResolvedPath);

LW_API INT              flock(INT iFd, INT iOperation);
LW_API INT              lockf(INT iFd, INT iCmd, off_t oftLen);

LW_API mode_t           umask(mode_t modeMask);
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */

/*********************************************************************************************************
  VxWorks SHOW
*********************************************************************************************************/

#define iosDrvShow                               API_IoDrvShow
#define iosDrvLicenseShow                        API_IoDrvLicenseShow
#define iosDevShow                               API_IoDevShow
#define iosFdShow                                API_IoFdShow

/*********************************************************************************************************
  Linux2.6 Driver License
*********************************************************************************************************/

#define DRIVER_LICENSE                           API_IoSetDrvLicense
#define DRIVER_AUTHOR                            API_IoSetDrvAuthor
#define DRIVER_DESCRIPTION                       API_IoSetDrvDescription

#define iosSetDrvLicense                         API_IoGetDrvLicense
#define iosSetDrvAuthor                          API_IoGetDrvAuthor
#define iosSetDrvDescrption                      API_IoGetDrvDescription

/*********************************************************************************************************
  线程池
*********************************************************************************************************/

#if LW_CFG_THREAD_POOL_EN > 0 && LW_CFG_MAX_THREAD_POOLS > 0
LW_API LW_OBJECT_HANDLE  API_ThreadPoolCreate(PCHAR                    pcName,
                                              PTHREAD_START_ROUTINE    pfuncThread,
                                              PLW_CLASS_THREADATTR     pthreadattr,
                                              UINT16                   usMaxThreadCounter,
                                              LW_OBJECT_ID            *pulId);

LW_API ULONG             API_ThreadPoolDelete(LW_OBJECT_HANDLE   *pulId);

LW_API ULONG             API_ThreadPoolAddThread(LW_OBJECT_HANDLE   ulId, PVOID  pvArg);

LW_API ULONG             API_ThreadPoolDeleteThread(LW_OBJECT_HANDLE   ulId);

LW_API ULONG             API_ThreadPoolSetAttr(LW_OBJECT_HANDLE  ulId, 
                                               PLW_CLASS_THREADATTR  pthreadattr);

LW_API ULONG             API_ThreadPoolGetAttr(LW_OBJECT_HANDLE  ulId, 
                                               PLW_CLASS_THREADATTR  pthreadattr);

LW_API ULONG             API_ThreadPoolStatus(LW_OBJECT_HANDLE         ulId,
                                              PTHREAD_START_ROUTINE   *ppthreadStartAddr,
                                              UINT16                  *pusMaxThreadCounter,
                                              UINT16                  *pusThreadCounter);
#endif

/*********************************************************************************************************
  SYSTEM HOOK LIST (must use in kernel mode)
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
LW_API ULONG             API_SystemHookAdd(LW_HOOK_FUNC  hookfunc, ULONG  ulOpt);

LW_API ULONG             API_SystemHookDelete(LW_HOOK_FUNC  hookfunc, ULONG  ulOpt);
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#endif                                                                  /*  __S_API_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
