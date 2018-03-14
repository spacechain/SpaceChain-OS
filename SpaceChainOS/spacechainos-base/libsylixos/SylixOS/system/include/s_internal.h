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
** 文   件   名: s_internal.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 12 月 13 日
**
** 描        述: 这是系统内部函数声明。

** BUG
2007.04.08  加入了对裁剪的宏支持
2008.06.08  加入 PTHREAD_CANCEL 操作, 支持相关的 posix 规定.
2009.11.21  加入 io 环境操作函数.
2013.09.17  使用 POSIX 规定的取消点动作.
*********************************************************************************************************/

#ifndef  __S_INTERNAL_H
#define  __S_INTERNAL_H

/*********************************************************************************************************
  PTHREAD_CANCEL 相关
*********************************************************************************************************/

#if LW_CFG_THREAD_CANCEL_EN > 0
#define __THREAD_CANCEL_POINT()         API_ThreadTestCancel()
#else
#define __THREAD_CANCEL_POINT()
#endif                                                                  /*  LW_CFG_THREAD_CANCEL_EN     */

/*********************************************************************************************************
  IOLIB
*********************************************************************************************************/

#if LW_CFG_DEVICE_EN > 0

/*********************************************************************************************************
  IO 锁
*********************************************************************************************************/


VOID            _IosLock(VOID);
VOID            _IosUnlock(VOID);

/*********************************************************************************************************
  IO 权限检查
*********************************************************************************************************/

INT             _IosCheckPermissions(INT  iFlag, BOOL  bExec, mode_t  mode, uid_t  uid, gid_t  gid);

/*********************************************************************************************************
  IO 环境
*********************************************************************************************************/

PLW_IO_ENV      _IosEnvCreate(VOID);
VOID            _IosEnvDelete(PLW_IO_ENV  pioe);
PLW_IO_ENV      _IosEnvGetDef(VOID);
VOID            _IosEnvInherit(PLW_IO_ENV  pioe);

/*********************************************************************************************************
  IO File
*********************************************************************************************************/

VOID            _IosFileListRemoveReq(PLW_FD_ENTRY   pfdentry);
VOID            _IosFileListLock(VOID);
VOID            _IosFileListUnlock(VOID);
INT             _IosFileListIslock(VOID);
PLW_FD_ENTRY    _IosFileGetKernel(INT  iFd, BOOL  bIsIgnAbn);
PLW_FD_ENTRY    _IosFileGet(INT  iFd, BOOL  bIsIgnAbn);
INT             _IosFileDup(PLW_FD_ENTRY pfdentry, INT iMinFd);
INT             _IosFileDup2(PLW_FD_ENTRY pfdentry, INT  iNewFd);
INT             _IosFileRefInc(INT  iFd);
INT             _IosFileRefDec(INT  iFd);
INT             _IosFileRefGet(INT  iFd);
PLW_FD_ENTRY    _IosFileNew(PLW_DEV_HDR  pdevhdrHdr, CPCHAR  pcName);
VOID            _IosFileDelete(PLW_FD_ENTRY    pfdentry);
VOID            _IosFileSet(PLW_FD_ENTRY   pfdentry, PLW_DEV_HDR  pdevhdrHdr, LONG  lValue, INT  iFlag);
INT             _IosFileRealName(PLW_FD_ENTRY   pfdentry, CPCHAR  pcRealName);
INT             _IosFileClose(PLW_FD_ENTRY   pfdentry);
INT             _IosFileIoctl(PLW_FD_ENTRY   pfdentry, INT  iCmd, LONG  lArg);

/*********************************************************************************************************
  lock file
*********************************************************************************************************/

INT             _FdLockfIoctl(PLW_FD_ENTRY   pfdentry, INT  iCmd, struct flock *pfl);
INT             _FdLockfProc(PLW_FD_ENTRY   pfdentry, INT  iType, pid_t  pid);

INT             _FdLockfClearFdNode(PLW_FD_NODE  pfdnode);
INT             _FdLockfClearFdEntry(PLW_FD_ENTRY  pfdentry, pid_t  pid);

/*********************************************************************************************************
  PATH
*********************************************************************************************************/

VOID            _PathCondense(PCHAR  pcPathName);
                        
ULONG           _PathCat(CPCHAR  pcDirName,
                         CPCHAR  pcFileName,
                         PCHAR   pcResult);
                      
PCHAR           _PathLastNamePtr(CPCHAR  pcPathName);

VOID            _PathLastName(CPCHAR  pcPathName, PCHAR  *ppcLastName);
                                   
PCHAR           _PathGetDef(VOID);

PCHAR           _PathGetFull(PCHAR  pcFullPath, size_t  stMaxSize, CPCHAR  pcFileName);

INT             _PathBuildLink(PCHAR      pcBuildName, 
                               size_t     stMaxSize, 
                               CPCHAR     pcDevName,
                               CPCHAR     pcPrefix,
                               CPCHAR     pcLinkDst,
                               CPCHAR     pcTail);

INT             _PathMoveCheck(CPCHAR  pcSrc, CPCHAR  pcDest);

/*********************************************************************************************************
  IOS (当前未引出的 API)
*********************************************************************************************************/

LW_API PLW_DEV_HDR  API_IosDevMatch(CPCHAR  pcName);
LW_API PLW_DEV_HDR  API_IosDevMatchFull(CPCHAR  pcName);
LW_API PLW_DEV_HDR  API_IosNextDevGet(PLW_DEV_HDR    pdevhdrHdr);

#define iosDevMatch                              API_IosDevMatch
#define iosDevMatchFull                          API_IosDevMatchFull
#define iosNextDevGet                            API_IosNextDevGet

LW_API LONG         API_IosFdValue(INT  iFd);
LW_API PLW_DEV_HDR  API_IosFdDevFind(INT  iFd);

#define iosFdValue                               API_IosFdValue
#define iosFdDevFind                             API_IosFdDevFind

LW_API VOID         API_IosFdFree(INT  iFd);
LW_API INT          API_IosFdSet(INT            iFd,
                                 PLW_DEV_HDR    pdevhdrHdr,
                                 LONG           lValue,
                                 INT            iFlag);
LW_API INT          API_IosFdNew(PLW_DEV_HDR    pdevhdrHdr,
                                 CPCHAR         pcName,
                                 LONG           lValue,
                                 INT            iFlag);
LW_API INT          API_IosFdRealName(INT  iFd, CPCHAR  pcRealName);
LW_API INT          API_IosFdLock(INT  iFd);
LW_API INT          API_IosFdGetType(INT  iFd, INT *piType);
LW_API INT          API_IosFdGetFlag(INT  iFd, INT *piFlag);

LW_API INT          API_IosFdSetCloExec(INT  iFd, INT  iCloExec);
LW_API INT          API_IosFdGetCloExec(INT  iFd, INT  *piCloExec);

#define iosFdFree                                API_IosFdFree
#define iosFdSet                                 API_IosFdSet
#define iosFdNew                                 API_IosFdNew
#define iosFdRealName                            API_IosFdRealName
#define iosFdLock                                API_IosFdLock
#define iosFdGetType                             API_IosFdGetType
#define iosFdGetFlag                             API_IosFdGetFlag

LW_API INT          API_IosFdRefInc(INT  iFd);
LW_API INT          API_IosFdRefDec(INT  iFd);

#define iosFdRefInc                              API_IosFdRefInc
#define iosFdRefDec                              API_IosFdRefDec

LW_API INT          API_IosFdUnlink(PLW_DEV_HDR  pdevhdrHdr, CPCHAR  pcName);

#define iosFdUnlink                              API_IosFdUnlink

LW_API LONG         API_IosCreate(PLW_DEV_HDR    pdevhdrHdr,
                                  PCHAR          pcName,
                                  INT            iFlag,
                                  INT            iMode);
LW_API INT          API_IosDelete(PLW_DEV_HDR    pdevhdrHdr,
                                  PCHAR          pcName);
                                  
#define iosCreate                                API_IosCreate
#define iosDelete                                API_IosDelete
                                  
LW_API LONG         API_IosOpen(PLW_DEV_HDR    pdevhdrHdr,
                                PCHAR          pcName,
                                INT            iFlag,
                                INT            iMode);
LW_API INT          API_IosClose(INT  iFd);

#define iosOpen                                  API_IosOpen
#define iosClose                                 API_IosClose

LW_API ssize_t      API_IosRead(INT      iFd,
                                PCHAR    pcBuffer,
                                size_t   stMaxByte);
LW_API ssize_t      API_IosPRead(INT      iFd,
                                 PCHAR    pcBuffer,
                                 size_t   stMaxByte,
                                 off_t    oftPos);
LW_API ssize_t      API_IosWrite(INT      iFd,
                                 CPCHAR   pcBuffer,
                                 size_t   stNBytes);
LW_API ssize_t      API_IosPWrite(INT      iFd,
                                  CPCHAR   pcBuffer,
                                  size_t   stNBytes,
                                  off_t    oftPos);
                                  
#define iosRead                                  API_IosRead
#define iosPRead                                 API_IosPRead
#define iosWrite                                 API_IosWrite
#define iosPWrite                                API_IosPWrite
                                  
LW_API INT          API_IosIoctl(INT    iFd,
                                 INT    iCmd,
                                 LONG   lArg); 
LW_API off_t        API_IosLseek(INT    iFd,
                                 off_t  oftOffset,
                                 INT    iWhence);
LW_API INT          API_IosFstat(INT    iFd, struct stat *pstat);

LW_API INT          API_IosLstat(PLW_DEV_HDR    pdevhdrHdr,
                                 PCHAR          pcName, 
                                 struct stat   *pstat);

#define iosIoctl                                 API_IosIoctl
#define iosLseek                                 API_IosLseek
#define iosFstat                                 API_IosFstat
#define iosLstat                                 API_IosLstat

LW_API INT          API_IosSymlink(PLW_DEV_HDR    pdevhdrHdr,
                                   PCHAR          pcName,
                                   CPCHAR         pcLinkDst);
LW_API ssize_t      API_IosReadlink(PLW_DEV_HDR    pdevhdrHdr,
                                    PCHAR          pcName,
                                    PCHAR          pcLinkDst,
                                    size_t         stMaxSize);

#define iosSymlink                               API_IosSymlink
#define iosReadlink                              API_IosReadlink

LW_API INT          API_IosMmap(INT   iFd, PLW_DEV_MMAP_AREA  pdmap);
LW_API INT          API_IosUnmap(INT   iFd, PLW_DEV_MMAP_AREA  pdmap);

#define iosMmap                                  API_IosMmap
#define iosUnmap                                 API_IosUnmap

/*********************************************************************************************************
  fd_entry
*********************************************************************************************************/

LW_API INT          API_IosFdEntryReclaim(PLW_FD_ENTRY  pfdentry, ULONG  ulRefDec, pid_t  pid);

/*********************************************************************************************************
  fd_node
*********************************************************************************************************/

LW_API PLW_FD_NODE  API_IosFdNodeAdd(LW_LIST_LINE_HEADER  *pplineHeader,
                                     dev_t                 dev,
                                     ino64_t               inode,
                                     INT                   iFlags,
                                     mode_t                mode,
                                     uid_t                 uid,
                                     gid_t                 gid,
                                     off_t                 oftSize,
                                     PVOID                 pvFile,
                                     BOOL                 *pbIsNew);
                                     
LW_API INT          API_IosFdNodeDec(LW_LIST_LINE_HEADER  *pplineHeader,
                                     PLW_FD_NODE           pfdnode,
                                     BOOL                 *bRemove);
                                     
LW_API PLW_FD_NODE  API_IosFdNodeFind(LW_LIST_LINE_HEADER  plineHeader, dev_t  dev, ino64_t  inode);

LW_API INT          API_IosFdNodeLock(PLW_FD_NODE  pfdnode);

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
#endif                                                                  /*  __S_INTERNAL_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
