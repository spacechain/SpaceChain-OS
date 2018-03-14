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
** 文   件   名: nfs_sylixos.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 03 月 08 日
**
** 描        述: nfs 文件系统 sylixos 接口.

** BUG:
2012.03.13  系统 reboot 时, 首先关闭了所有网络文件描述符, 在卸载所有卷, 所以这里 reboot 时必须首先卸载卷,
            nfs dev 一定是晚于网络初始化, 所以, 这里建立的 reboot hook 一定早于关闭描述符执行. 
2012.03.21  修正了对打开文件名为 \0 的处理.
            支持 auth_unix 选项.
2012.04.01  今天是愚人节, 但是我要谨记, 诚实是中华民族的传统美德, 在这个道德缺失的年代, 我们跟需要坚守道德
            目前 NFS 还是仅支持 udp 模式.
2012.04.11  允许 force unlink nfs 设备.
2012.07.01  只读文件系统支持.
2012.08.16  支持 pread 与 pwrite
2012.09.01  加入不可强制卸载卷的保护.
2012.09.25  nfs 可以创建 socket 类型文件.
2012.11.09  __nfsReadlink() 返回有效字节数.
2012.12.14  加入了类似其他操作系统的卷操作锁.
2013.01.06  nfs 使用最新 NEW_1 设备驱动类型. 这样可以支持文件锁. (nfs 仅支持本地进程文件锁)
            重新设计文件结构, 需要把相关信息都记录下来, 减少网络数据包提高速度.
2013.01.18  所有对只读文件系统带有 O_CREAT 的操作被拒绝.
2013.01.22  为了兼容性 MNT 之前最好发送一次 NULL 命令.
            修复 symlink 返回值的 BUG.
            优化单次传输数据量, 经测试 TCP (近32K单次传输量) 速度要优于 UDP (8K单次传输量).
2013.04.01  修正 GCC 4.7.3 引发的新 warning.
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
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_NFS_EN > 0)
#include "kernrpc/rpc.h"
#include "nfs_mount.h"
#include "nfs.h"
#include "nfs_xdr.h"
/*********************************************************************************************************
  说明:
  RPC机制使得单个远程程序一次只能支持一个远程过程调用，当前远程
  过程调用完成之前会自动阻塞其他远程过程调用，程序员设计分布式程序时
  不需要考虑这种互斥。
*********************************************************************************************************/
#define __NFS_FORCE_UMOUNT              1                               /* 强制 unlink 设备             */
/*********************************************************************************************************
  常用参数
*********************************************************************************************************/
#define __NFS_READDIR_MAXLEN            512                             /* readdir 一次读取 entry 数量  */
#define __NFS_BLOCK_SIZE                4096                            /* General FileSystem block size*/
/*********************************************************************************************************
  内部结构
*********************************************************************************************************/
typedef struct {
    cookie3             NFSID_cookie;                                   /*  cookie                      */
    cookieverf3         NFSID_cookieverf;
    entry3             *NFSID_entry;
    bool_t              NFSID_bEof;
    READDIR3res         NFSID_res;
} NFS_IN_DIR;
typedef NFS_IN_DIR     *PNFS_IN_DIR;

typedef struct {
    LW_DEV_HDR          NFSFS_devhdrHdr;                                /*  nfs 文件系统设备头          */
    BOOL                NFSFS_bForceDelete;                             /*  是否允许强制卸载卷          */
    BOOL                NFSFS_bValid;                                   /*  文件系统是否有效            */
    
    LW_OBJECT_HANDLE    NFSFS_hVolLock;                                 /*  卷操作锁                    */
    LW_LIST_LINE_HEADER NFSFS_plineFdNodeHeader;                        /*  fd_node 链表                */
    LW_LIST_LINE        NFSFS_lineManage;                               /*  所有 NFS 设备管理链表       */
    
    INT                 NFSFS_iFlag;                                    /*  O_RDONLY or O_RDWR          */
    nfs_fh3             NFSFS_hRoot;                                    /*  root handle                 */
    struct stat         NFSFS_stat;                                     /*  根节点文件 stat             */
    
    CLIENT             *NFSFS_pclient;
    CLIENT             *NFSFS_pclientMount;
    
    size_t              NFSFS_stOneTrans;                               /*  单次传输数据大小            */
    
    CHAR                NFSFS_cHost[MAXHOSTNAMELEN + 1];
    CHAR                NFSFS_cPath[PATH_MAX + 1];
} NFS_FS;
typedef NFS_FS         *PNFS_FS;

typedef struct {
    NFS_FS             *NFSFIL_nfsfs;
    nfs_fh3             NFSFIL_handle;
    NFS_IN_DIR          NFSFIL_nfsid;                                   /*  仅 dir 文件类型使用         */
    struct stat         NFSFIL_stat;                                    /*  文件 stat                   */
    INT                 NFSFIL_iFileType;                               /*  文件类型                    */
    CHAR                NFSFIL_cName[1];                                /*  文件名                      */
} NFS_FILE;
typedef NFS_FILE       *PNFS_FILE;
/*********************************************************************************************************
  内部全局变量
*********************************************************************************************************/
static INT                              _G_iNfsDrvNum = PX_ERROR;
static LW_LIST_LINE_HEADER              _G_plineNfsDev;                 /*  所有 nfs dev 链表           */
static LW_OBJECT_HANDLE                 _G_ulNfsLock;                   /*  nfs 操作锁                  */
/*********************************************************************************************************
  NFS 操作锁
*********************************************************************************************************/
#define __NFS_LOCK()                    API_SemaphoreMPend(_G_ulNfsLock, LW_OPTION_WAIT_INFINITE)
#define __NFS_UNLOCK()                  API_SemaphoreMPost(_G_ulNfsLock)
/*********************************************************************************************************
  文件类型
*********************************************************************************************************/
#define __NFS_FILE_TYPE_NODE            0                               /*  open 打开文件               */
#define __NFS_FILE_TYPE_DIR             1                               /*  open 打开目录               */
#define __NFS_FILE_TYPE_DEV             2                               /*  open 打开设备               */
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define __NFS_FILE_LOCK(pnfsfile)   API_SemaphoreMPend(pnfsfile->NFSFIL_nfsfs->NFSFS_hVolLock, \
                                    LW_OPTION_WAIT_INFINITE)
#define __NFS_FILE_UNLOCK(pnfsfile) API_SemaphoreMPost(pnfsfile->NFSFIL_nfsfs->NFSFS_hVolLock)
#define __NFS_VOL_LOCK(pnfsfs)      API_SemaphoreMPend(pnfsfs->NFSFS_hVolLock, LW_OPTION_WAIT_INFINITE)
#define __NFS_VOL_UNLOCK(pnfsfs)    API_SemaphoreMPost(pnfsfs->NFSFS_hVolLock)
/*********************************************************************************************************
  nfs 单次传输数据量
*********************************************************************************************************/
#define __NFS_ONE_TRANS_SIZE(pnfsfile)  (pnfsfile->NFSFIL_nfsfs->NFSFS_stOneTrans)
/*********************************************************************************************************
  检测路径字串是否为根目录或者直接指向设备
*********************************************************************************************************/
#define __STR_IS_ROOT(pcName)           ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))
/*********************************************************************************************************
  内部函数
*********************************************************************************************************/
static LONG     __nfsOpen(PNFS_FS         pnfsfs,
                          PCHAR           pcName,
                          INT             iFlags,
                          INT             iMode);
static INT      __nfsRemove(PNFS_FS       pnfsfs,
                            PCHAR         pcName);
static INT      __nfsClose(PLW_FD_ENTRY   pfdentry);
static ssize_t  __nfsRead(PLW_FD_ENTRY    pfdentry,
                          PCHAR           pcBuffer,
                          size_t          stMaxBytes);
static ssize_t  __nfsPRead(PLW_FD_ENTRY    pfdentry,
                           PCHAR           pcBuffer,
                           size_t          stMaxBytes,
                           off_t           oftPos);
static ssize_t  __nfsWrite(PLW_FD_ENTRY  pfdentry,
                           PCHAR         pcBuffer,
                           size_t        stNBytes);
static ssize_t  __nfsPWrite(PLW_FD_ENTRY  pfdentry,
                            PCHAR         pcBuffer,
                            size_t        stNBytes,
                            off_t         oftPos);
static INT      __nfsSeek(PLW_FD_ENTRY  pfdentry,
                          off_t         oftOffset);
static INT      __nfsWhere(PLW_FD_ENTRY   pfdentry,
                           off_t         *poftPos);
static INT      __nfsStat(PLW_FD_ENTRY   pfdentry, 
                          struct stat   *pstat);
static INT      __nfsStat64(PLW_FD_ENTRY   pfdentry, 
                            struct stat64 *pstat64);
static INT      __nfsLStat(PNFS_FS       pnfsfs,
                           PCHAR         pcName,
                           struct stat  *pstat);
static INT      __nfsIoctl(PLW_FD_ENTRY  pfdentry,
                           INT           iRequest,
                           LONG          lArg);
static INT      __nfsSymlink(PNFS_FS       pnfsfs,
                             PCHAR         pcName,
                             CPCHAR        pcLinkDst);
static ssize_t  __nfsReadlink(PNFS_FS       pnfsfs,
                              PCHAR         pcName,
                              PCHAR         pcLinkDst,
                              size_t        stMaxSize);
/*********************************************************************************************************
** 函数名称: __nfsRpcError2Errno
** 功能描述: RPC 错误返回值转换为 errno 并记录
** 输　入  : clntstat      rpc 错误
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __nfsRpcErrorHandle (enum clnt_stat  clntstat)
{
    switch (clntstat) {

    case RPC_CANTENCODEARGS:
    case RPC_CANTDECODERES:
        errno = EBADMSG;
        break;

    case RPC_CANTSEND:
        errno = ECOMM;
        break;

    case RPC_CANTRECV:
        errno = EREMOTEIO;
        break;

    case RPC_TIMEDOUT:
        errno = ETIMEDOUT;
        break;
        
    case RPC_AUTHERROR:
        errno = EPERM;
        break;

    case RPC_VERSMISMATCH:
    case RPC_PROGUNAVAIL:
    case RPC_PROGVERSMISMATCH:
    case RPC_PROCUNAVAIL:
    case RPC_CANTDECODEARGS:
    case RPC_SYSTEMERROR:
    case RPC_NOBROADCAST:
        errno = EREMOTEIO;
        break;

    case RPC_UNKNOWNHOST:
    case RPC_UNKNOWNADDR:
        errno = ENETUNREACH;
        break;

    case RPC_UNKNOWNPROTO:
        errno = EPROTONOSUPPORT;
        break;

    case RPC_RPCBFAILURE:
    case RPC_PROGNOTREGISTERED:
    case RPC_N2AXLATEFAILURE:
        errno = ENOPROTOOPT;
        break;

    case RPC_INPROGRESS:
        errno = EINPROGRESS;
        break;

    default: 
        errno = EIO;
        break;
    }
}
/*********************************************************************************************************
** 函数名称: __nfsHostStringParse
** 功能描述: 分析 host string 参数
** 输　入  : pcHostString     nfs 服务器地址参数
**           pcHost           服务器地址
**           stHostLen        服务器地址缓冲长度
**           pcPath           服务器内部路径
**           stPathLen        服务器内部路径长度
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsHostStringParse (CPCHAR  pcHostString,
                                  PCHAR   pcHost,
                                  size_t  stHostLen,
                                  PCHAR   pcPath,
                                  size_t  stPathLen)
{
    INT     iTemp;
    PCHAR   pcTemp;

    pcTemp = lib_index(pcHostString, ':');
    if (pcTemp == LW_NULL) {
        return  (PX_ERROR);
    }
    iTemp = pcTemp - pcHostString;

    if (iTemp >= stHostLen) {
        return  (PX_ERROR);
    }

    memcpy(pcHost, pcHostString, iTemp);
    pcHost[iTemp] = PX_EOS;

    pcTemp++;
    lib_strlcpy(pcPath, pcTemp, stPathLen);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsHandleCpy
** 功能描述: handle cpy
** 输　入  : phDest         nfs 目标句柄
**           phSrc          nfs 源句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __nfsHandleCpy (nfs_fh3 *phDest, const nfs_fh3 *phSrc)
{
    phDest->data.data_len = phSrc->data.data_len;
    phDest->data.data_val = (char *)sys_malloc(phDest->data.data_len);  /*  use nfs xdr sys_malloc      */
    if (phDest->data.data_val == LW_NULL) {
        phDest->data.data_len = 0;
        return;
    }
    lib_memcpy(phDest->data.data_val, phSrc->data.data_val, phDest->data.data_len);
}
/*********************************************************************************************************
** 函数名称: __nfsAttr2Stat
** 功能描述: 将 attr 转换为标准 stat
** 输　入  : pnfsfs           设备
**           pattr            nfs 属性
**           pstat            标准属性
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __nfsAttr2Stat (PNFS_FS  pnfsfs, fattr3  *pattr, struct stat *pstat)
{
    if (!pnfsfs || !pattr || !pstat) {
        return;
    }
    
    pstat->st_dev = (dev_t)pnfsfs;
    
    if (sizeof(fileid3) == sizeof(ino_t)) {
        pstat->st_ino = (ino_t)pattr->fileid;
    } else {
        if (pattr->fileid & (fileid3)(~(ino_t)0)) {
            pstat->st_ino = (ino_t)pattr->fileid;
        } else {
            pstat->st_ino = (ino_t)(pattr->fileid >> 32);
        }
    }
    
    pstat->st_resv2   = (void *)((ULONG)(pattr->fileid));
    pstat->st_resv3   = (void *)((ULONG)(pattr->fileid >> 32));         /*  记录完整的 fileid           */
    
    pstat->st_nlink   = pattr->nlink;
    pstat->st_uid     = pattr->uid;
    pstat->st_gid     = pattr->gid;
    pstat->st_rdev    = pattr->rdev.specdata1;
    pstat->st_size    = pattr->size;
    pstat->st_blksize = __NFS_BLOCK_SIZE;
    pstat->st_blocks  = (long)(pstat->st_size / __NFS_BLOCK_SIZE);
    if (pstat->st_size % __NFS_BLOCK_SIZE) {
        pstat->st_blocks++;
    }
    
    /*
     *  NFS 服务器直接使用 UTC 时间.
     */
    pstat->st_atime   = (time_t)pattr->atime.seconds;
    pstat->st_mtime   = (time_t)pattr->mtime.seconds;
    pstat->st_ctime   = (time_t)pattr->ctime.seconds;
    
    switch (pattr->type) {
    
    case NF3REG:
        pstat->st_mode = (pattr->mode | S_IFREG);
        break;
    
    case NF3DIR:
        pstat->st_mode = (pattr->mode | S_IFDIR);
        break;
    
    case NF3BLK:
        pstat->st_mode = (pattr->mode | S_IFBLK);
        break;
    
    case NF3CHR:
        pstat->st_mode = (pattr->mode | S_IFCHR);
        break;
    
    case NF3LNK:
        pstat->st_mode = (pattr->mode | S_IFLNK);
        break;
    
    case NF3SOCK:
        pstat->st_mode = (pattr->mode | S_IFSOCK);
        break;
    
    case NF3FIFO:
        pstat->st_mode = (pattr->mode | S_IFIFO);
        break;
        
    default:
        pstat->st_mode = (pattr->mode | S_IFDIR);
        break;
    }
}
/*********************************************************************************************************
** 函数名称: __nfsRootStat
** 功能描述: 获得 nfs 根目录的 stat
** 输　入  : pnfsfs           设备
**           pstat            标准属性
** 输　出  : ERROR or NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsRootStat (PNFS_FS  pnfsfs, struct stat *pstat)
{
    enum clnt_stat  clntstat;

    GETATTR3args    args;
    GETATTR3res     res;
    nfs_fh3         handle;
    
    handle = pnfsfs->NFSFS_hRoot;
    
    args.object = handle;

    lib_bzero(&res, sizeof(res));
    
    clntstat = nfsproc3_getattr_3(args, &res, pnfsfs->NFSFS_pclient);
    if (clntstat != RPC_SUCCESS) {
        __nfsRpcErrorHandle(clntstat);
        return  (PX_ERROR);
    
    } else if (res.status != NFS3_OK) {
        _ErrorHandle(res.status);
        xdr_free((xdrproc_t)xdr_GETATTR3res, (char *)&res);
        return  (PX_ERROR);
    }
    
    __nfsAttr2Stat(pnfsfs, &res.GETATTR3res_u.resok.obj_attributes, pstat);
    
    xdr_free((xdrproc_t)xdr_GETATTR3res, (char *)&res);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsRebootHook
** 功能描述: nfs reboot hook (需要卸载所有的卷)
** 输　入  : NONE
**           NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __nfsRebootHook (VOID)
{
    CHAR        cDevName[MAX_FILENAME_LENGTH];
    PNFS_FS     pnfsfs;
    
    __NFS_LOCK();
    while (_G_plineNfsDev) {
        pnfsfs = _LIST_ENTRY(_G_plineNfsDev, NFS_FS, NFSFS_lineManage);
        pnfsfs->NFSFS_bForceDelete = LW_TRUE;
        lib_strlcpy(cDevName, pnfsfs->NFSFS_devhdrHdr.DEVHDR_pcName, MAX_FILENAME_LENGTH);
        __NFS_UNLOCK();
        unlink(cDevName);
        __NFS_LOCK();
    }
    __NFS_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: API_NfsDrvInstall
** 功能描述: 安装 NFS 文件系统驱动程序
** 输　入  :
** 输　出  : < 0 表示失败
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_NfsDrvInstall (VOID)
{
    struct file_operations     fileop;
    
    if (_G_ulNfsLock == 0) {
        _G_ulNfsLock = API_SemaphoreMCreate("nfs_lock", LW_PRIO_DEF_CEILING, 
                                            LW_OPTION_WAIT_PRIORITY |
                                            LW_OPTION_INHERIT_PRIORITY | 
                                            LW_OPTION_DELETE_SAFE |
                                            LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }

    if (_G_iNfsDrvNum > 0) {
        return  (ERROR_NONE);
    }

    lib_bzero(&fileop, sizeof(struct file_operations));

    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __nfsOpen;
    fileop.fo_release  = __nfsRemove;
    fileop.fo_open     = __nfsOpen;
    fileop.fo_close    = __nfsClose;
    fileop.fo_read     = __nfsRead;
    fileop.fo_read_ex  = __nfsPRead;
    fileop.fo_write    = __nfsWrite;
    fileop.fo_write_ex = __nfsPWrite;
    fileop.fo_lstat    = __nfsLStat;
    fileop.fo_ioctl    = __nfsIoctl;
    fileop.fo_symlink  = __nfsSymlink;
    fileop.fo_readlink = __nfsReadlink;

    _G_iNfsDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);       /*  实用新型 NEW_1 驱动程序     */

    DRIVER_LICENSE(_G_iNfsDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_iNfsDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iNfsDrvNum, "NFSv3 driver.");

    _DebugHandle(__LOGMESSAGE_LEVEL, "nfs file system installed.\r\n");
    
    __fsRegister("nfs", API_NfsDevCreate, LW_NULL);                     /*  注册文件系统                */

    return  ((_G_iNfsDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** 函数名称: API_NfsDevCreate
** 功能描述: 创建 NFS 设备.
** 输　入  : pcName            设备名(设备挂接的节点地址)
**           pblkd             使用 pblkd->BLKD_pcName 作为 host 标示.
                               同时查看 pblkd->BLKD_iFlag 如果是 O_RDONLY 则使用只读方式
** 输　出  : < 0 表示失败
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_NfsDevCreate (PCHAR   pcName, PLW_BLK_DEV  pblkd)
{
#define __NFS_NULL_CALL_TIMES   1

    static BOOL     bHookAdd = LW_FALSE;
    
    INT             i;
    PCHAR           pcHostString;
    PNFS_FS         pnfsfs;
    mountres3       mountres;
    enum clnt_stat  clntstat;
    
    CHAR            cAuthMeth[16] = "\0";
    CHAR            cProto[4];

    if (_G_iNfsDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "nfs Driver invalidate.\r\n");
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
    pcHostString = pblkd->BLKD_pcName;

    pnfsfs = (PNFS_FS)__SHEAP_ALLOC(sizeof(NFS_FS));
    if (pnfsfs == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pnfsfs, sizeof(NFS_FS));
    
    pnfsfs->NFSFS_bValid = LW_TRUE;

    if (__nfsHostStringParse(pcHostString,
                             pnfsfs->NFSFS_cHost, MAXHOSTNAMELEN + 1,
                             pnfsfs->NFSFS_cPath, PATH_MAX + 1)) {
        __SHEAP_FREE(pnfsfs);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "host parse.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    pnfsfs->NFSFS_hVolLock = API_SemaphoreMCreate("nfsvol_lock", LW_PRIO_DEF_CEILING,
                               LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE | 
                               LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                               LW_NULL);
    if (!pnfsfs->NFSFS_hVolLock) {                                      /*  创建卷操作锁失败            */
        goto    __error_handle;
    }
    
    if (lib_getenv_r("NFS_CLIENT_PROTO", cProto, sizeof(cProto)) < ERROR_NONE) {
        lib_strcpy(cProto, "udp");                                      /*  默认使用 udp 协议           */
    }
    
    pnfsfs->NFSFS_pclient = clnt_create(pnfsfs->NFSFS_cHost, NFS3_PROGRAM, NFS_V3, cProto);
    if (pnfsfs->NFSFS_pclient == LW_NULL) {                             /*  NFS(100003) V:3             */
        _ErrorHandle(EIO);
        fprintf(stderr, "creat nfs client failed, NFS(100003) V:3.\n");
        goto    __error_handle;
    }
    
    pnfsfs->NFSFS_pclientMount = clnt_create(pnfsfs->NFSFS_cHost, MOUNT_PROGRAM, MOUNT_V3, cProto);
    if (pnfsfs->NFSFS_pclientMount == LW_NULL) {                        /*  MOUNT(100005) V:3           */
        _ErrorHandle(EIO);
        fprintf(stderr, "create mount client failed, MOUNT(100005) V:3.\n");
        goto    __error_handle;
    }
    
    for (i = 0; i < __NFS_NULL_CALL_TIMES; i++) {
        clntstat = mountproc3_null_3(LW_NULL, pnfsfs->NFSFS_pclientMount);
        if (clntstat != RPC_SUCCESS) {                                  /*  NFS NULL Call               */
            __nfsRpcErrorHandle(clntstat);
            fprintf(stderr, "nfs null call failed, MNT.\n");
            goto    __error_handle;
        }
    }
    
    lib_getenv_r("NFS_CLIENT_AUTH", cAuthMeth, sizeof(cAuthMeth));
    if ((lib_strcmp(cAuthMeth, "AUTH_UNIX") == 0) ||
        (lib_strcmp(cAuthMeth, "UNIX") == 0)) {
        pnfsfs->NFSFS_pclient->cl_auth = authunix_create_default();     /*  auth unix                   */
        pnfsfs->NFSFS_pclientMount->cl_auth = authunix_create_default();
    }
    
    lib_bzero(&mountres, sizeof(mountres));
    clntstat = mountproc3_mnt_3(pnfsfs->NFSFS_cPath, &mountres, pnfsfs->NFSFS_pclientMount);
    if (clntstat != RPC_SUCCESS) {                                      /*  NFS MNT Call                */
        __nfsRpcErrorHandle(clntstat);
        fprintf(stderr, "nfs mount failed, MNT.\n");
        goto    __error_handle;
    
    } else if (mountres.fhs_status != MNT3_OK) {
        _ErrorHandle(mountres.fhs_status);
        xdr_free((xdrproc_t)xdr_mountres3, (char *)&mountres);          /*  释放 mountres               */
        fprintf(stderr, "nfs mount failed, MNT.\n");
        goto    __error_handle;
    }
    
    __nfsHandleCpy(&pnfsfs->NFSFS_hRoot, (nfs_fh3 *)&mountres.mountres3_u.mountinfo.fhandle);
    xdr_free((xdrproc_t)xdr_mountres3, (char *)&mountres);              /*  释放 mountres               */
    
    if (__nfsRootStat(pnfsfs, &pnfsfs->NFSFS_stat) < ERROR_NONE) {      /*  获得根节点属性              */
        fprintf(stderr, "nfs can not get root status.\n");
        goto    __error_handle;
    }

    pnfsfs->NFSFS_iFlag = pblkd->BLKD_iFlag;
    
    if (strncmp(cProto, "udp", 3) == 0) {                               /*  确定单次传输数据量          */
        pnfsfs->NFSFS_stOneTrans = UDPMSGSIZE - 400;
    } else {
        pnfsfs->NFSFS_stOneTrans = TCPMSGSIZE - 400;                    /*  减 400, 是空出参数区大小    */
    }

    if (iosDevAddEx(&pnfsfs->NFSFS_devhdrHdr, pcName, _G_iNfsDrvNum, DT_DIR)
        != ERROR_NONE) {                                                /*  安装文件系统设备            */
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&pnfsfs->NFSFS_hRoot);
        fprintf(stderr, "device add error.\n");
        goto    __error_handle;
    }
    
    __NFS_LOCK();
    _List_Line_Add_Ahead(&pnfsfs->NFSFS_lineManage, &_G_plineNfsDev);   /*  加入 nfs 设备链表           */
    __NFS_UNLOCK();
    
    if (!bHookAdd) {
        bHookAdd = LW_TRUE;
        API_SystemHookAdd(__nfsRebootHook, LW_OPTION_KERNEL_REBOOT);    /*  加入 reboot hook            */
    }

    _DebugFormat(__LOGMESSAGE_LEVEL, "target \"%s\" mount ok.\r\n", pcName);

    return  (ERROR_NONE);

__error_handle:
    if (pnfsfs) {
        if (pnfsfs->NFSFS_pclientMount) {
            if(pnfsfs->NFSFS_pclientMount->cl_auth) {
                auth_destroy(pnfsfs->NFSFS_pclientMount->cl_auth);
            }
            clnt_destroy(pnfsfs->NFSFS_pclientMount);
        }
        
        if (pnfsfs->NFSFS_pclient) {
            if (pnfsfs->NFSFS_pclient->cl_auth) {
                auth_destroy(pnfsfs->NFSFS_pclient->cl_auth);
            }
            clnt_destroy(pnfsfs->NFSFS_pclient);
        }
        
        if (pnfsfs->NFSFS_hVolLock) {
            API_SemaphoreMDelete(&pnfsfs->NFSFS_hVolLock);
        }
        
        __SHEAP_FREE(pnfsfs);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_NfsDevDelete
** 功能描述: 删除一个 nfs 设备, 例如: API_NfsDevDelete("/mnt/ata0");
** 输　入  : pcName            设备名(设备挂接的节点地址)
** 输　出  : < 0 表示失败
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_NfsDevDelete (PCHAR   pcName)
{
    if (API_IosDevMatchFull(pcName)) {                                  /*  如果是设备, 这里就卸载设备  */
        return  (unlink(pcName));
    
    } else {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __nfsFindSymlink
** 功能描述: 检查路径中是否存在符号链接文件
** 输　入  : pnfsfs           设备
**           pcName           文件名
**           ppcTail          如果存在连接文件, 指向连接文件后的路径
**           ppcSymfile       连接文件
**           phandleSym       连接文件句柄
** 输　出  : TRUE or FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  __nfsFindSymlink (PNFS_FS  pnfsfs, 
                               PCHAR    pcName, 
                               PCHAR   *ppcTail, 
                               PCHAR   *ppcSymfile, 
                               nfs_fh3 *phandleSym)
{
    CHAR        cNameBuffer[PATH_MAX + 1];
    PCHAR       pcNameStart;
    PCHAR       pcPtr = cNameBuffer;
    
    nfs_fh3     handle;
    
    LOOKUP3args args;
    LOOKUP3res  res;
    
    fattr3     *pattributes;
    ftype3      type;
    
    phandleSym->data.data_len = 0;
    phandleSym->data.data_val = LW_NULL;
    
    __nfsHandleCpy(&handle, &pnfsfs->NFSFS_hRoot);
    
    lib_strlcpy(cNameBuffer, pcName, PATH_MAX + 1);
    if (*pcPtr) {
        pcPtr++;
    }
    
    while (*pcPtr) {
        pcNameStart = pcPtr;
        
        while (*pcPtr && *pcPtr != PX_DIVIDER) {
            pcPtr++;
        }
        if (*pcPtr != 0) {
            *pcPtr++ = 0;
        }
    
        __nfsHandleCpy(&args.what.dir, &handle);
        args.what.name = pcNameStart;
        
        lib_bzero(&res, sizeof(res));
        if (nfsproc3_lookup_3(args, &res, pnfsfs->NFSFS_pclient) != RPC_SUCCESS) {
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&args.what.dir);
            break;
        
        } else if (res.status != NFS3_OK) {
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&args.what.dir);
            xdr_free((xdrproc_t)xdr_LOOKUP3res, (char *)&res);
            break;
        }
        
        if (res.LOOKUP3res_u.resok.obj_attributes.attributes_follow) {
            pattributes = &res.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes;
        } else {
            pattributes = &res.LOOKUP3res_u.resok.dir_attributes.post_op_attr_u.attributes;
        }
        
        type = pattributes->type;
        
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&args.what.dir);
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handle);
        __nfsHandleCpy(&handle, &res.LOOKUP3res_u.resok.object);
        xdr_free((xdrproc_t)xdr_LOOKUP3res, (char *)&res);
        
        if (type == NF3LNK) {
            *ppcTail    = (char *)(pcName + (pcPtr - cNameBuffer));
            *ppcSymfile = (char *)(pcName + (pcNameStart - cNameBuffer));/* point to symlink file name  */
            *phandleSym = handle;
            return  (LW_TRUE);
        
        } else if (type != NF3DIR) {
            break;
        }
    }
    
    xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handle);
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: __nfsReadSymlink
** 功能描述: 获得一个符号文件的连接目标
** 输　入  : pnfsfs           设备
**           phandle          句柄
**           pcLink           连接目标缓冲
**           stSize           连接目标缓冲大小
** 输　出  : < 0 错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsReadSymlink (PNFS_FS  pnfsfs, nfs_fh3  *phandle, PCHAR  pcLink, size_t  stSize)
{
    enum clnt_stat  clntstat;
    READLINK3args   arg;
    READLINK3res    clnt_res;

    arg.symlink = *phandle;
    
    lib_bzero(&clnt_res, sizeof(clnt_res));
    clntstat = nfsproc3_readlink_3(arg, &clnt_res, pnfsfs->NFSFS_pclient);
    if (clntstat != RPC_SUCCESS) {
        __nfsRpcErrorHandle(clntstat);
        return  (PX_ERROR);
    
    } else if (clnt_res.status != NFS3_OK) {
        xdr_free((xdrproc_t)xdr_READLINK3res, (char *)&clnt_res);
        _ErrorHandle(clnt_res.status);
        return  (PX_ERROR);
    
    }
    
    lib_strncpy(pcLink, clnt_res.READLINK3res_u.resok.data, stSize);    /*  拷贝有效字节, 如超出不考'\0'*/
    xdr_free((xdrproc_t)xdr_READLINK3res, (char *)&clnt_res);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsPathBuildLink
** 功能描述: 根据链接文件内容生成连接目标
** 输　入  : pnfsfs           设备
**           phandle          句柄
**           pcDest           输出缓冲
**           stSize           输出缓冲大小
**           pcPrefix         前缀
**           pcTail           后缀
** 输　出  : < 0 错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsPathBuildLink (PNFS_FS  pnfsfs, nfs_fh3  *phandle, 
                                PCHAR    pcDest, size_t    stSize,
                                PCHAR    pcPrefix, PCHAR   pcTail)
{
    CHAR        cLink[PATH_MAX + 1];
    
    if (__nfsReadSymlink(pnfsfs, phandle, cLink, PATH_MAX + 1) == ERROR_NONE) {
        return  (_PathBuildLink(pcDest, stSize, pnfsfs->NFSFS_devhdrHdr.DEVHDR_pcName, pcPrefix, 
                                cLink, pcTail));
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __nfsGetHandle
** 功能描述: 获得一个句柄
** 输　入  : pnfsfs           设备
**           pcName           需要搜索的名字
**           phandle          句柄
**           pcTail           句柄搜索结尾
**           pbIsDir          句柄对应是否为文件夹
**           pstat            需要获取的 stat
** 输　出  : < 0 错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsGetHandle (PNFS_FS      pnfsfs, 
                            PCHAR        pcName, 
                            nfs_fh3     *phandle, 
                            PCHAR       *pcTail, 
                            BOOL        *pbIsDir,
                            struct stat *pstat)
{
    CHAR        cNameBuffer[PATH_MAX + 1];
    PCHAR       pcNameStart;
    PCHAR       pcPtr = cNameBuffer;
    
    LOOKUP3args args;
    LOOKUP3res  res;
    
    fattr3     *pattributes;
    
    __nfsHandleCpy(phandle, &pnfsfs->NFSFS_hRoot);
    
    lib_strlcpy(cNameBuffer, pcName, PATH_MAX + 1);
    if (*pcPtr) {
        pcPtr++;
    }
    
    if ((*pcPtr == PX_EOS) && pstat) {
        *pstat = pnfsfs->NFSFS_stat;
    }
    
    while (*pcPtr) {
        pcNameStart = pcPtr;
        
        while (*pcPtr && *pcPtr != PX_DIVIDER) {
            pcPtr++;
        }
        if (*pcPtr != 0) {
            *pcPtr++ = 0;
        }
        
        __nfsHandleCpy(&args.what.dir, phandle);
        args.what.name = pcNameStart;
        
        lib_bzero(&res, sizeof(res));
        if (nfsproc3_lookup_3(args, &res, pnfsfs->NFSFS_pclient) != RPC_SUCCESS) {
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&args.what.dir);
            pcPtr = pcNameStart;                                        /*  倒退回去                    */
            break;
        
        } else if (res.status != NFS3_OK) {
            _ErrorHandle(res.status);
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&args.what.dir);
            xdr_free((xdrproc_t)xdr_LOOKUP3res, (char *)&res);
            pcPtr = pcNameStart;                                        /*  倒退回去                    */
            break;
        }
        
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&args.what.dir);
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)phandle);
        
        if (res.LOOKUP3res_u.resok.obj_attributes.attributes_follow) {
            pattributes = &res.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes;
        } else {
            pattributes = &res.LOOKUP3res_u.resok.dir_attributes.post_op_attr_u.attributes;
        }
        
        if ((*pcPtr == PX_EOS) && pstat) {
            __nfsAttr2Stat(pnfsfs, pattributes, pstat);
        }
        
        __nfsHandleCpy(phandle, &res.LOOKUP3res_u.resok.object);
        xdr_free((xdrproc_t)xdr_LOOKUP3res, (char *)&res);
    }
    
    if (*pcPtr) {                                                       /*  没有查找到目标              */
        *pcTail = (char *)(pcName + (pcPtr - cNameBuffer));
    
    } else {
        *pcTail = LW_NULL;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsGetDir
** 功能描述: 获得一个句柄 (上一级目录)
** 输　入  : pnfsfs           设备
**           pcName           需要搜索的名字
**           phandle          句柄
**           pcTail           句柄搜索结尾
**           pstat            需要获取的 stat
** 输　出  : < 0 错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsGetDir (PNFS_FS      pnfsfs, 
                         PCHAR        pcName, 
                         nfs_fh3     *phandle, 
                         PCHAR       *pcTail, 
                         struct stat *pstat)
{
    CHAR        cNameBuffer[PATH_MAX + 1];
    PCHAR       pcNameStart;
    PCHAR       pcPtr = cNameBuffer;
    
    LOOKUP3args args;
    LOOKUP3res  res;
    
    fattr3     *pattributes;
    
    __nfsHandleCpy(phandle, &pnfsfs->NFSFS_hRoot);
    
    lib_strlcpy(cNameBuffer, pcName, PATH_MAX + 1);
    if (*pcPtr) {
        pcPtr++;
    }
    
    if ((*pcPtr == PX_EOS) && pstat) {
        *pstat = pnfsfs->NFSFS_stat;
    }
    
    while (*pcPtr) {
        pcNameStart = pcPtr;
        
        if (lib_index(pcPtr, PX_DIVIDER) == LW_NULL) {
            break;
        }
        
        while (*pcPtr && *pcPtr != PX_DIVIDER) {
            pcPtr++;
        }
        if (*pcPtr != 0) {
            *pcPtr++ = 0;
        }
        
        __nfsHandleCpy(&args.what.dir, phandle);
        args.what.name = pcNameStart;
        
        lib_bzero(&res, sizeof(res));
        if (nfsproc3_lookup_3(args, &res, pnfsfs->NFSFS_pclient) != RPC_SUCCESS) {
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&args.what.dir);
            pcPtr = pcNameStart;                                        /*  倒退回去                    */
            break;
        
        } else if (res.status != NFS3_OK) {
            _ErrorHandle(res.status);
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&args.what.dir);
            xdr_free((xdrproc_t)xdr_LOOKUP3res, (char *)&res);
            pcPtr = pcNameStart;                                        /*  倒退回去                    */
            break;
        }
        
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&args.what.dir);
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)phandle);
        
        if ((*pcPtr == PX_EOS) && pstat) {
            if (res.LOOKUP3res_u.resok.obj_attributes.attributes_follow) {
                pattributes = &res.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes;
            } else {
                pattributes = &res.LOOKUP3res_u.resok.dir_attributes.post_op_attr_u.attributes;
            }
            __nfsAttr2Stat(pnfsfs, pattributes, pstat);
        }
        
        __nfsHandleCpy(phandle, &res.LOOKUP3res_u.resok.object);
        xdr_free((xdrproc_t)xdr_LOOKUP3res, (char *)&res);
    }
    
    if (*pcPtr) {                                                       /*  没有查找到目标              */
        *pcTail = (char *)(pcName + (pcPtr - cNameBuffer));
    
    } else {
        *pcTail = LW_NULL;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsOpenFile
** 功能描述: 打开或者创建文件
** 输　入  : pnfsfs           设备
**           pcName           文件名
**           iFlags           方式
**           iMode            mode_t
**           phandle          返回句柄
**           pbIsDir          如果只是打开, 则返回是否是 dir
**           pstat            需要获取的 stat
** 输　出  : < 0 错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __nfsOpenFile (PNFS_FS        pnfsfs, 
                          PCHAR          pcName, 
                          INT            iFlag, 
                          INT            iMode, 
                          nfs_fh3       *phandle, 
                          BOOL          *pbIsDir,
                          struct stat   *pstat)
{
    CHAR        cNameBuffer[PATH_MAX + 1];
    PCHAR       pcNameStart;
    PCHAR       pcPtr = cNameBuffer;
    
    BOOL        bFindIsDir = LW_TRUE;                                   /*  root 为目录型               */
    BOOL        bCanWrite  = LW_FALSE;
    
    enum clnt_stat  clntstat;
    
    LOOKUP3args args;
    LOOKUP3res  res;
    
    CREATE3args creatargs;
    CREATE3res  creatres;
    
    fattr3     *pattributes;
    
    __nfsHandleCpy(phandle, &pnfsfs->NFSFS_hRoot);
    
    lib_strlcpy(cNameBuffer, pcName, PATH_MAX + 1);
    if (*pcPtr) {
        pcPtr++;
    }
    
    if ((*pcPtr == PX_EOS) && pstat) {
        *pstat = pnfsfs->NFSFS_stat;
    }
    
    while (*pcPtr) {
        pcNameStart = pcPtr;
        
        while (*pcPtr && *pcPtr != PX_DIVIDER) {
            pcPtr++;
        }
        if (*pcPtr != 0) {
            *pcPtr++ = 0;
        }
        
        __nfsHandleCpy(&args.what.dir, phandle);
        args.what.name = pcNameStart;
        
        lib_bzero(&res, sizeof(res));
        if (nfsproc3_lookup_3(args, &res, pnfsfs->NFSFS_pclient) != RPC_SUCCESS) {
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&args.what.dir);
            pcPtr = pcNameStart;                                        /*  倒退回去                    */
            break;
        
        } else if (res.status != NFS3_OK) {
            _ErrorHandle(res.status);
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&args.what.dir);
            xdr_free((xdrproc_t)xdr_LOOKUP3res, (char *)&res);
            pcPtr = pcNameStart;                                        /*  倒退回去                    */
            break;
        }
        
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&args.what.dir);
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)phandle);
        
        if (res.LOOKUP3res_u.resok.obj_attributes.attributes_follow) {
            pattributes = &res.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes;
        } else {
            pattributes = &res.LOOKUP3res_u.resok.dir_attributes.post_op_attr_u.attributes;
        }
        
        if (pattributes->type == NF3DIR) {
            bFindIsDir = LW_TRUE;
        } else {
            bFindIsDir = LW_FALSE;
            if (pattributes->mode & S_IWRITE) {
                bCanWrite = LW_TRUE;
            }
        }
        
        if ((*pcPtr == PX_EOS) && pstat) {
            __nfsAttr2Stat(pnfsfs, pattributes, pstat);
        }
        
        __nfsHandleCpy(phandle, &res.LOOKUP3res_u.resok.object);
        xdr_free((xdrproc_t)xdr_LOOKUP3res, (char *)&res);
    }
    
    if (*pcPtr) {                                                       /*  没有查找到目标              */
        if ((iFlag & O_CREAT) == 0) {
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)phandle);
            _ErrorHandle(ENOENT);
            return  (PX_ERROR);
        }
        
        lib_bzero(&creatargs, sizeof(creatargs));
        lib_bzero(&creatres, sizeof(creatres));
        
        __nfsHandleCpy(&creatargs.where.dir, phandle);
        creatargs.where.name = pcPtr;
        creatargs.how.mode = UNCHECKED;
        creatargs.how.createhow3_u.obj_attributes.mode.set_it = LW_TRUE;
        creatargs.how.createhow3_u.obj_attributes.mode.set_mode3_u.mode = iMode;
        creatargs.how.createhow3_u.obj_attributes.uid.set_it = LW_TRUE;
        creatargs.how.createhow3_u.obj_attributes.uid.set_uid3_u.uid = getuid();
        creatargs.how.createhow3_u.obj_attributes.gid.set_it = LW_TRUE;
        creatargs.how.createhow3_u.obj_attributes.gid.set_gid3_u.gid = getgid();
        creatargs.how.createhow3_u.obj_attributes.size.set_it  = LW_FALSE;
        creatargs.how.createhow3_u.obj_attributes.atime.set_it = SET_TO_SERVER_TIME;
        creatargs.how.createhow3_u.obj_attributes.mtime.set_it = SET_TO_SERVER_TIME;
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)phandle);
        
        clntstat = nfsproc3_create_3(creatargs, &creatres, pnfsfs->NFSFS_pclient);
        if (clntstat != RPC_SUCCESS) {
            __nfsRpcErrorHandle(clntstat);
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&creatargs.where.dir);
            return  (PX_ERROR);
            
        } else if (creatres.status != NFS3_OK) {
            _ErrorHandle(creatres.status);
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&creatargs.where.dir);
            xdr_free((xdrproc_t)xdr_CREATE3res, (char *)&creatres);
            return  (PX_ERROR);
        }
        
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&creatargs.where.dir);
        
        *pbIsDir = LW_FALSE;                                            /*  创建文件成功                */
        
        if (!creatres.CREATE3res_u.resok.obj.handle_follows) {
            _ErrorHandle(EINVAL);
            xdr_free((xdrproc_t)xdr_CREATE3res, (char *)&creatres);
            return  (PX_ERROR);
        }
        
        if (pstat) {
            __nfsAttr2Stat(pnfsfs, 
                           &creatres.CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes, 
                           pstat);
        }
        
        __nfsHandleCpy(phandle, &creatres.CREATE3res_u.resok.obj.post_op_fh3_u.handle);
        xdr_free((xdrproc_t)xdr_CREATE3res, (char *)&creatres);
        
        return  (ERROR_NONE);
        
    } else {                                                            /*  查找到目标                  */

        *pbIsDir = bFindIsDir;

        if ((iFlag & O_CREAT) && (iFlag & O_EXCL)) {
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)phandle);
            _ErrorHandle(EEXIST);
            return  (PX_ERROR);
        }
        
        if ((bCanWrite == LW_FALSE) && (bFindIsDir == LW_FALSE)) {      /*  普通只读文件                */
            if ((iFlag & O_TRUNC) || ((iFlag & O_ACCMODE) != O_RDONLY)) {
                xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)phandle);
                _ErrorHandle(EACCES);
                return  (PX_ERROR);
            }
        }
        
        if (iFlag & O_TRUNC) {                                          /*  需要将文件清零              */
            SETATTR3args    attrargs;
            SETATTR3res     attrres;
            
            lib_bzero(&attrargs, sizeof(attrargs));
            lib_bzero(&attrres, sizeof(attrres));
            
            __nfsHandleCpy(&attrargs.object, phandle);
            attrargs.new_attributes.size.set_it = LW_TRUE;
            attrargs.new_attributes.size.set_size3_u.size = 0;
            
            clntstat = nfsproc3_setattr_3(attrargs, &attrres, pnfsfs->NFSFS_pclient);
            if (clntstat != RPC_SUCCESS) {
                __nfsRpcErrorHandle(clntstat);
                xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&attrargs.object);
                xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)phandle);
                return  (PX_ERROR);
            
            } else if (attrres.status != NFS3_OK) {
                _ErrorHandle(attrres.status);
                xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&attrargs.object);
                xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)phandle);
                xdr_free((xdrproc_t)xdr_SETATTR3res, (char *)&attrres);
                return  (PX_ERROR);
            }
            
            pstat->st_size = 0;
            
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&attrargs.object);
            xdr_free((xdrproc_t)xdr_SETATTR3res, (char *)&attrres);
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsRemoveFile
** 功能描述: 删除一个文件或者文件夹
** 输　入  : pnfsfs           设备
**           pcName           文件名
**           iFlags           方式
**           iMode            mode_t
**           phandle          返回句柄
**           pbIsDir          如果只是打开, 则返回是否是 dir
** 输　出  : < 0 错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __nfsRemoveFile (PNFS_FS  pnfsfs, PCHAR  pcName)
{
    nfs_fh3     handle;
    nfs_fh3     handleDir;
    BOOL        bHandeDirNeedFree = LW_FALSE;
    PCHAR       pcRemove = LW_NULL;

    CHAR        cNameBuffer[PATH_MAX + 1];
    PCHAR       pcNameStart;
    PCHAR       pcPtr = cNameBuffer;
    
    BOOL        bFindIsDir = LW_FALSE;
    
    enum clnt_stat  clntstat;
    
    LOOKUP3args args;
    LOOKUP3res  res;
    
    fattr3     *pattributes;
    
    __nfsHandleCpy(&handle, &pnfsfs->NFSFS_hRoot);
    
    lib_strlcpy(cNameBuffer, pcName, PATH_MAX + 1);
    if (*pcPtr) {
        pcPtr++;
    }
    
    while (*pcPtr) {
        pcNameStart = pcPtr;
        
        while (*pcPtr && *pcPtr != PX_DIVIDER) {
            pcPtr++;
        }
        if (*pcPtr != 0) {
            *pcPtr++ = 0;
        }
        
        __nfsHandleCpy(&args.what.dir, &handle);
        args.what.name = pcNameStart;
        
        lib_bzero(&res, sizeof(res));
        
        clntstat = nfsproc3_lookup_3(args, &res, pnfsfs->NFSFS_pclient);
        if (clntstat != RPC_SUCCESS) {
            __nfsRpcErrorHandle(clntstat);
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&args.what.dir);
            pcPtr = pcNameStart;                                        /*  倒退回去                    */
            break;
        
        } else if (res.status != NFS3_OK) {
            _ErrorHandle(res.status);
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&args.what.dir);
            xdr_free((xdrproc_t)xdr_LOOKUP3res, (char *)&res);
            pcPtr = pcNameStart;                                        /*  倒退回去                    */
            break;
        }
        
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&args.what.dir);
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handle);
        
        if (res.LOOKUP3res_u.resok.obj_attributes.attributes_follow) {
            pattributes = &res.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes;
        } else {
            pattributes = &res.LOOKUP3res_u.resok.dir_attributes.post_op_attr_u.attributes;
        }
        
        if (pattributes->type == NF3DIR) {
            bFindIsDir = LW_TRUE;
        } else {
            bFindIsDir = LW_FALSE;
        }
        
        __nfsHandleCpy(&handle, &res.LOOKUP3res_u.resok.object);
        xdr_free((xdrproc_t)xdr_LOOKUP3res, (char *)&res);
        
        if (!bHandeDirNeedFree && 
            *pcPtr && (lib_index(pcPtr, PX_DIVIDER) == 0)) {            /*  最后一级目录                */
            __nfsHandleCpy(&handleDir, &handle);
            bHandeDirNeedFree = LW_TRUE;
            pcRemove = pcPtr;
        }
    }
    
    if (*pcPtr) {                                                       /*  没有查找到目标              */
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handle);
        if (bHandeDirNeedFree) {
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handleDir);
        }
        return  (PX_ERROR);
    }
    
    if (bHandeDirNeedFree == LW_FALSE) {
        handleDir = pnfsfs->NFSFS_hRoot;                                /*  从根目录开始                */
        pcRemove  = cNameBuffer + 1;
    }
    
    if (bFindIsDir) {
        RMDIR3args  rdargs;
        RMDIR3res   rdres;
        
        rdargs.object.dir  = handleDir;
        rdargs.object.name = pcRemove;
        
        lib_bzero(&rdres, sizeof(rdres));
        
        clntstat = nfsproc3_rmdir_3(rdargs, &rdres, pnfsfs->NFSFS_pclient);
        if (clntstat != RPC_SUCCESS) {
            __nfsRpcErrorHandle(clntstat);
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handle);
            if (bHandeDirNeedFree) {
                xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handleDir);
            }
            return  (PX_ERROR);
            
        } else if (rdres.status != NFS3_OK) {
            _ErrorHandle(rdres.status);
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handle);
            if (bHandeDirNeedFree) {
                xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handleDir);
            }
            xdr_free((xdrproc_t)xdr_RMDIR3res, (char *)&rdres);
            return  (PX_ERROR);
        }
        
        xdr_free((xdrproc_t)xdr_RMDIR3res, (char *)&rdres);
    
    } else {
        REMOVE3args rmargs;
        REMOVE3res  rmres;
        
        rmargs.object.dir  = handleDir;
        rmargs.object.name = pcRemove;
        
        lib_bzero(&rmres, sizeof(rmres));
        
        clntstat = nfsproc3_remove_3(rmargs, &rmres, pnfsfs->NFSFS_pclient);
        if (clntstat != RPC_SUCCESS) {
            __nfsRpcErrorHandle(clntstat);
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handle);
            if (bHandeDirNeedFree) {
                xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handleDir);
            }
            return  (PX_ERROR);
        
        } else if (rmres.status != NFS3_OK) {
            _ErrorHandle(rmres.status);
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handle);
            if (bHandeDirNeedFree) {
                xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handleDir);
            }
            xdr_free((xdrproc_t)xdr_REMOVE3res, (char *)&rmres);
            return  (PX_ERROR);
        }
        
        xdr_free((xdrproc_t)xdr_REMOVE3res, (char *)&rmres);
    }
    
    xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handle);
    if (bHandeDirNeedFree) {
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handleDir);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsMakeDir
** 功能描述: 创建一个目录
** 输　入  : pnfsfs           设备
**           pcName           文件名
**           iMode            mode_t
**           phandle          返回句柄
**           pstat            需要获取的 stat
** 输　出  : < 0 错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsMakeDir (PNFS_FS        pnfsfs, 
                          PCHAR          pcName, 
                          INT            iMode, 
                          nfs_fh3       *phandle,
                          struct stat   *pstat)
{
    CHAR        cNameBuffer[PATH_MAX + 1];
    PCHAR       pcNameStart;
    PCHAR       pcPtr = cNameBuffer;
    
    enum clnt_stat  clntstat;
    
    LOOKUP3args args;
    LOOKUP3res  res;
    
    MKDIR3args  dirargs;
    MKDIR3res   dirres;
    
    __nfsHandleCpy(phandle, &pnfsfs->NFSFS_hRoot);
    
    lib_strlcpy(cNameBuffer, pcName, PATH_MAX + 1);
    if (*pcPtr) {
        pcPtr++;
    }
    
    if ((*pcPtr == PX_EOS) && pstat) {
        *pstat = pnfsfs->NFSFS_stat;
    }
    
    while (*pcPtr) {
        pcNameStart = pcPtr;
        
        while (*pcPtr && *pcPtr != PX_DIVIDER) {
            pcPtr++;
        }
        if (*pcPtr != 0) {
            *pcPtr++ = 0;
        }
        
        __nfsHandleCpy(&args.what.dir, phandle);
        args.what.name = pcNameStart;
        
        lib_bzero(&res, sizeof(res));
        if (nfsproc3_lookup_3(args, &res, pnfsfs->NFSFS_pclient) != RPC_SUCCESS) {
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&args.what.dir);
            pcPtr = pcNameStart;                                        /*  倒退回去                    */
            break;
        
        } else if (res.status != NFS3_OK) {
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&args.what.dir);
            xdr_free((xdrproc_t)xdr_LOOKUP3res, (char *)&res);
            pcPtr = pcNameStart;                                        /*  倒退回去                    */
            break;
        }
        
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&args.what.dir);
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)phandle);
        __nfsHandleCpy(phandle, &res.LOOKUP3res_u.resok.object);
        xdr_free((xdrproc_t)xdr_LOOKUP3res, (char *)&res);
    }
    
    if (*pcPtr == PX_EOS) {
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)phandle);
        _ErrorHandle(EEXIST);
        return  (PX_ERROR);
    }
    
    lib_bzero(&dirargs, sizeof(dirargs));
    lib_bzero(&dirres, sizeof(dirres));
    
    __nfsHandleCpy(&dirargs.where.dir, phandle);
    dirargs.where.name = pcPtr;
    dirargs.attributes.mode.set_it = LW_TRUE;
    dirargs.attributes.mode.set_mode3_u.mode = iMode;
    dirargs.attributes.uid.set_it = LW_TRUE;
    dirargs.attributes.uid.set_uid3_u.uid = getuid();
    dirargs.attributes.gid.set_it = LW_TRUE;
    dirargs.attributes.gid.set_gid3_u.gid = getgid();
    dirargs.attributes.size.set_it  = LW_FALSE;
    dirargs.attributes.atime.set_it = SET_TO_SERVER_TIME;               /*  use server time             */
    dirargs.attributes.mtime.set_it = SET_TO_SERVER_TIME;
    xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)phandle);
    
    clntstat = nfsproc3_mkdir_3(dirargs, &dirres, pnfsfs->NFSFS_pclient);
    if (clntstat != RPC_SUCCESS) {
        __nfsRpcErrorHandle(clntstat);
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&dirargs.where.dir);
        return  (PX_ERROR);
    
    } else if (dirres.status != NFS3_OK) {
        _ErrorHandle(dirres.status);
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&dirargs.where.dir);
        xdr_free((xdrproc_t)xdr_MKDIR3res, (char *)&dirres);
        return  (PX_ERROR);
    }
    
    if (pstat) {
        __nfsAttr2Stat(pnfsfs, 
                       &dirres.MKDIR3res_u.resok.obj_attributes.post_op_attr_u.attributes, 
                       pstat);
    }
    
    xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&dirargs.where.dir);
    __nfsHandleCpy(phandle, &dirres.MKDIR3res_u.resok.obj.post_op_fh3_u.handle);
    xdr_free((xdrproc_t)xdr_MKDIR3res, (char *)&dirres);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsCloseFile
** 功能描述: nfs 内部关闭一个文件
** 输　入  : pnfsfile         nfs 文件控制块
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsCloseFile (PNFS_FILE    pnfsfile)
{
    if (pnfsfile) {
        if (pnfsfile->NFSFIL_iFileType == __NFS_FILE_TYPE_NODE) {
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&pnfsfile->NFSFIL_handle);
        
        } else if (pnfsfile->NFSFIL_iFileType == __NFS_FILE_TYPE_DIR) {
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&pnfsfile->NFSFIL_handle);
            xdr_free((xdrproc_t)xdr_READDIR3res, (char *)&pnfsfile->NFSFIL_nfsid.NFSID_res);
        }
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __nfsOpen
** 功能描述: nfs open 操作
** 输　入  : pnfsfs           设备
**           pcName           文件名
**           iFlags           方式
**           iMode            mode_t
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LONG  __nfsOpen (PNFS_FS         pnfsfs,
                        PCHAR           pcName,
                        INT             iFlags,
                        INT             iMode)
{
    INT             iError;
    PNFS_FILE       pnfsfile;
    PLW_FD_NODE     pfdnode;
    size_t          stSize;
    BOOL            bIsDir = LW_FALSE;
    BOOL            bIsNew;
    ino64_t         inode64;

    if (pcName == LW_NULL) {                                            /*  无文件名                    */
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (pnfsfs->NFSFS_pclient == LW_NULL) {
            _ErrorHandle(ENXIO);
            return  (PX_ERROR);
        }
        
        if (iFlags & O_CREAT) {
            if (pnfsfs->NFSFS_iFlag == O_RDONLY) {
                _ErrorHandle(EROFS);                                    /*  只读文件系统                */
                return  (PX_ERROR);
            }
            if (__fsCheckFileName(pcName)) {
                _ErrorHandle(ENOENT);
                return  (PX_ERROR);
            }
            if (S_ISFIFO(iMode) || 
                S_ISBLK(iMode)  ||
                S_ISCHR(iMode)) {
                _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);                /*  不支持以上这些格式          */
                return  (PX_ERROR);
            }
        }
        
        stSize   = sizeof(NFS_FILE) + lib_strlen(pcName);
        pnfsfile = (PNFS_FILE)__SHEAP_ALLOC(stSize);
        if (pnfsfile == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        lib_bzero(pnfsfile, stSize);
        lib_strcpy(pnfsfile->NFSFIL_cName, pcName);                     /*  记录文件名                  */
    
        pnfsfile->NFSFIL_nfsfs = pnfsfs;
        
        if (__NFS_FILE_LOCK(pnfsfile) != ERROR_NONE) {
            _ErrorHandle(ENXIO);
            return  (PX_ERROR);
        }
        
        if (pnfsfs->NFSFS_bValid == LW_FALSE) {                         /*  文件系统是否有效            */
            __NFS_FILE_UNLOCK(pnfsfile);
            __SHEAP_FREE(pnfsfile);
            _ErrorHandle(ENOENT);
            return  (PX_ERROR);
        }
        /*
         *  首先处理符号链接文件情况
         */
        {
            nfs_fh3 handle;
            INT     iFollowLinkType;
            PCHAR   pcTail, pcSymfile, pcPrefix;
            BOOL    bIsSym = __nfsFindSymlink(pnfsfs, pcName, &pcTail, &pcSymfile, &handle);
                                                                        /*  检查目录是否包含 symlink    */
            if (bIsSym) {                                               /*  目录中有 symlink            */
                pcSymfile--;                                            /*  从 / 开始                   */
                if (pcSymfile == pcName) {
                    pcPrefix = LW_NULL;                                 /*  没有前缀                    */
                } else {
                    pcPrefix = pcName;
                    *pcSymfile = PX_EOS;
                }
                if (pcTail && lib_strlen(pcTail)) {
                    iFollowLinkType = FOLLOW_LINK_TAIL;                 /*  连接目标内部文件            */
                } else {
                    iFollowLinkType = FOLLOW_LINK_FILE;                 /*  链接文件本身                */
                }
                
                if (__nfsPathBuildLink(pnfsfs, &handle, pcName, PATH_MAX + 1,
                                       pcPrefix, pcTail) == ERROR_NONE) {
                                                                        /*  构造链接目标                */
                    xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handle);
                    __NFS_FILE_UNLOCK(pnfsfile);
                    __SHEAP_FREE(pnfsfile);
                    return  (iFollowLinkType);
                
                } else {                                                /*  构造连接失败                */
                    xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handle);
                    __NFS_FILE_UNLOCK(pnfsfile);
                    __SHEAP_FREE(pnfsfile);
                    return  (PX_ERROR);
                }
            }
        }
        
        /*
         *  非符号链接文件.
         */
        if ((iFlags & O_CREAT) && S_ISDIR(iMode)) {                     /*  创建目录                    */
            iError = __nfsMakeDir(pnfsfs, pcName, iMode, 
                                  &pnfsfile->NFSFIL_handle, 
                                  &pnfsfile->NFSFIL_stat);
            pnfsfile->NFSFIL_iFileType = __NFS_FILE_TYPE_DIR;
            
        } else {
            iError = __nfsOpenFile(pnfsfs, pcName, iFlags, iMode, 
                                   &pnfsfile->NFSFIL_handle, &bIsDir, 
                                   &pnfsfile->NFSFIL_stat);
            if (bIsDir) {
                pnfsfile->NFSFIL_iFileType = __NFS_FILE_TYPE_DIR;
            } else {
                pnfsfile->NFSFIL_iFileType = __NFS_FILE_TYPE_NODE;
            }
        }
        
        if (iError < 0) {
            if (__STR_IS_ROOT(pnfsfile->NFSFIL_cName)) {
                pnfsfile->NFSFIL_stat = pnfsfs->NFSFS_stat;             /*  获得根节点属性              */
                pnfsfile->NFSFIL_iFileType = __NFS_FILE_TYPE_DEV;
                goto    __file_open_ok;                                 /*  设备打开正常                */
            }
            __NFS_FILE_UNLOCK(pnfsfile);
            __SHEAP_FREE(pnfsfile);
            return  (PX_ERROR);
        }
        
__file_open_ok:
        if (sizeof(fileid3) == sizeof(ino_t)) {
            inode64 = pnfsfile->NFSFIL_stat.st_ino;
        } else {
            ULONG   ulLow  = (ULONG)pnfsfile->NFSFIL_stat.st_resv2;
            ULONG   ulHigh = (ULONG)pnfsfile->NFSFIL_stat.st_resv3;
            inode64 = (ino64_t)(((ino64_t)ulLow) + ((ino64_t)ulHigh << 32));
        }
        pfdnode = API_IosFdNodeAdd(&pnfsfs->NFSFS_plineFdNodeHeader,
                                    pnfsfile->NFSFIL_stat.st_dev,
                                    inode64,                            /*  NFS 使用 64bit inode        */
                                    iFlags,
                                    iMode,
                                    pnfsfile->NFSFIL_stat.st_uid,
                                    pnfsfile->NFSFIL_stat.st_gid,
                                    pnfsfile->NFSFIL_stat.st_size,
                                    (PVOID)pnfsfile,
                                    &bIsNew);                           /*  添加文件节点                */
        if (pfdnode == LW_NULL) {                                       /*  无法创建 fd_node 节点       */
            __NFS_FILE_UNLOCK(pnfsfile);
            __nfsCloseFile(pnfsfile);
            __SHEAP_FREE(pnfsfile);
            return  (PX_ERROR);
        }
        
        LW_DEV_INC_USE_COUNT(&pnfsfs->NFSFS_devhdrHdr);                 /*  更新计数器                  */
        
        if (bIsNew == LW_FALSE) {                                       /*  有重复打开                  */
            __nfsCloseFile(pnfsfile);
            __NFS_FILE_UNLOCK(pnfsfile);
            __SHEAP_FREE(pnfsfile);
        
        } else {
            __NFS_FILE_UNLOCK(pnfsfile);
        }
        
        return  ((LONG)pfdnode);                                        /*  返回fd_node                 */
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __nfsRemove
** 功能描述: nfs remove 操作
** 输　入  : pnfsfs           设备
**           pcName           文件名
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsRemove (PNFS_FS         pnfsfs,
                         PCHAR           pcName)
{
    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }

    if (__STR_IS_ROOT(pcName)) {                                        /*  根目录或者 nfs 设备文件     */
        if (__NFS_VOL_LOCK(pnfsfs) != ERROR_NONE) {
            _ErrorHandle(ENXIO);
            return  (PX_ERROR);
        }
        
        if (pnfsfs->NFSFS_bValid == LW_FALSE) {
            __NFS_VOL_UNLOCK(pnfsfs);
            return  (ERROR_NONE);                                       /*  正在被其他任务卸载          */
        }
        
        pnfsfs->NFSFS_bValid = LW_FALSE;
        
__re_umount_vol:
        if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)pnfsfs)) {               /*  检查是否有正在工作的文件    */
            if (!pnfsfs->NFSFS_bForceDelete) {
                pnfsfs->NFSFS_bValid = LW_TRUE;
                __NFS_VOL_UNLOCK(pnfsfs);
                _ErrorHandle(EBUSY);
                return  (PX_ERROR);
            }
            
            __NFS_VOL_UNLOCK(pnfsfs);
            
            _DebugHandle(__ERRORMESSAGE_LEVEL, "disk have open file.\r\n");
            iosDevFileAbnormal(&pnfsfs->NFSFS_devhdrHdr);               /*  将所有相关文件设为异常模式  */
        
            __NFS_VOL_LOCK(pnfsfs);
            goto    __re_umount_vol;
        }

        if (pnfsfs->NFSFS_pclientMount &&
            (mountproc3_umnt_3((char *)pnfsfs->NFSFS_cPath, LW_NULL, 
                               pnfsfs->NFSFS_pclientMount) != RPC_SUCCESS)) {
#if __NFS_FORCE_UMOUNT == 0
            __NFS_VOL_UNLOCK(pnfsfs);
            fprintf(stderr, "nfs umount failed.\n");
            return  (PX_ERROR);
#endif                                                                  /*  !__NFS_FORCE_UMOUNT         */
        }
        
        __NFS_LOCK();
        _List_Line_Del(&pnfsfs->NFSFS_lineManage, &_G_plineNfsDev);     /*  从 nfs 设备链表删除         */
        __NFS_UNLOCK();

        if (pnfsfs->NFSFS_pclientMount) {
            if(pnfsfs->NFSFS_pclientMount->cl_auth) {
                auth_destroy(pnfsfs->NFSFS_pclientMount->cl_auth);
                pnfsfs->NFSFS_pclientMount->cl_auth = LW_NULL;
            }
            clnt_destroy(pnfsfs->NFSFS_pclientMount);
            pnfsfs->NFSFS_pclientMount = LW_NULL;
        }

        if (pnfsfs->NFSFS_pclient) {
            if (pnfsfs->NFSFS_pclient->cl_auth) {
                auth_destroy(pnfsfs->NFSFS_pclient->cl_auth);
                pnfsfs->NFSFS_pclient->cl_auth = LW_NULL;
            }
            clnt_destroy(pnfsfs->NFSFS_pclient);
            pnfsfs->NFSFS_pclient = LW_NULL;
        }
        
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&pnfsfs->NFSFS_hRoot);

        iosDevDelete((LW_DEV_HDR *)pnfsfs);                             /*  IO 系统移除设备             */

        API_SemaphoreMDelete(&pnfsfs->NFSFS_hVolLock);                  /*  删除卷锁                    */
        __SHEAP_FREE(pnfsfs);

        _DebugHandle(__LOGMESSAGE_LEVEL, "nfs unmount ok.\r\n");
        
        return  (ERROR_NONE);
    
    } else {
        struct stat statDel;
        
        if (__NFS_VOL_LOCK(pnfsfs) != ERROR_NONE) {
            _ErrorHandle(ENXIO);
            return  (PX_ERROR);
        }
        
        if (__nfsLStat(pnfsfs, pcName, &statDel) == ERROR_NONE) {
            INT     iError;
            
            if (pnfsfs->NFSFS_iFlag == O_RDONLY) {
                __NFS_VOL_UNLOCK(pnfsfs);
                _ErrorHandle(EROFS);                                    /*  只读文件系统                */
                return  (PX_ERROR);
            }
            
            iError = __nfsRemoveFile(pnfsfs, pcName);                   /*  删除                        */
            __NFS_VOL_UNLOCK(pnfsfs);
            return  (iError);
        
        } else {
            PCHAR       pcTail, pcSymfile, pcPrefix;
            nfs_fh3     handle;
                                                                        /*  rpc 调用会使 errno 发生变化 */ 
            if (__nfsFindSymlink(pnfsfs, pcName, &pcTail, &pcSymfile, &handle)) {
                                                                        /*  目录中有 symlink            */
                pcSymfile--;                                            /*  从 / 开始                   */
                if (pcSymfile == pcName) {
                    pcPrefix = LW_NULL;                                 /*  没有前缀                    */
                } else {
                    pcPrefix = pcName;
                    *pcSymfile = PX_EOS;
                }
                if (__nfsPathBuildLink(pnfsfs, &handle, pcName, PATH_MAX + 1,
                                       pcPrefix, pcTail) == ERROR_NONE) {
                                                                        /*  构造链接目标                */
                    xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handle);
                    
                    __NFS_VOL_UNLOCK(pnfsfs);
                    return  (FOLLOW_LINK_TAIL);                         /*  一定不是连接文件本身        */
                }
            
            } else {
                _ErrorHandle(ENOENT);                                   /*  没有找打文件                */
            }
        }
        
        __NFS_VOL_UNLOCK(pnfsfs);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __nfsClose
** 功能描述: nfs close 操作
** 输　入  : pfdentry         文件控制块
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsClose (PLW_FD_ENTRY    pfdentry)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PNFS_FILE     pnfsfile = (PNFS_FILE)pfdnode->FDNODE_pvFile;
    PNFS_FS       pnfsfs   = pnfsfile->NFSFIL_nfsfs;
    BOOL          bFree    = LW_FALSE;
    BOOL          bRemove  = LW_FALSE;

    if (pnfsfile) {
        if (__NFS_FILE_LOCK(pnfsfile) != ERROR_NONE) {
            _ErrorHandle(ENXIO);                                        /*  设备出错                    */
            return  (PX_ERROR);
        }
        if (API_IosFdNodeDec(&pnfsfs->NFSFS_plineFdNodeHeader,
                             pfdnode, &bRemove) == 0) {                 /*  fd_node 是否完全释放        */
            __nfsCloseFile(pnfsfile);
            bFree = LW_TRUE;
        }
        
        LW_DEV_DEC_USE_COUNT(&pnfsfile->NFSFIL_nfsfs->NFSFS_devhdrHdr);
        
        if (bRemove) {
            __nfsRemoveFile(pnfsfs, pnfsfile->NFSFIL_cName);
        }
        
        __NFS_FILE_UNLOCK(pnfsfile);
        
        if (bFree) {
            __SHEAP_FREE(pnfsfile);
        }
        
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __nfsRead
** 功能描述: nfs read 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __nfsRead (PLW_FD_ENTRY   pfdentry,
                           PCHAR          pcBuffer,
                           size_t         stMaxBytes)
{
    enum clnt_stat  clntstat;
    
    READ3args   args;
    READ3res    res;

    PLW_FD_NODE pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PNFS_FILE   pnfsfile = (PNFS_FILE)pfdnode->FDNODE_pvFile;
    size_t      stTotal  = 0;
    count3      countOnce;
    BOOL        bIsEof   = LW_FALSE;                                    /*  是否到文件尾部              */

    if (!pcBuffer || !stMaxBytes) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    if (__NFS_FILE_LOCK(pnfsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (pnfsfile->NFSFIL_iFileType != __NFS_FILE_TYPE_NODE) {
        __NFS_FILE_UNLOCK(pnfsfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    while (stTotal < stMaxBytes) {                                      /*  没有读取完全                */
        if (bIsEof) {
            __NFS_FILE_UNLOCK(pnfsfile);
            return  ((ssize_t)stTotal);
        }
    
        countOnce = (count3)(stMaxBytes - stTotal);
        if (countOnce > __NFS_ONE_TRANS_SIZE(pnfsfile)) {
            countOnce = __NFS_ONE_TRANS_SIZE(pnfsfile);                 /*  单次最大传输量              */
        }
    
        args.file   = pnfsfile->NFSFIL_handle;
        args.offset = (offset3)(pfdentry->FDENTRY_oftPtr);
        args.count  = countOnce;
        
        lib_bzero(&res, sizeof(res));
        
        clntstat = nfsproc3_read_3(args, &res, pnfsfile->NFSFIL_nfsfs->NFSFS_pclient);
        if (clntstat != RPC_SUCCESS) {
            if (stTotal == 0) {
                __nfsRpcErrorHandle(clntstat);
            }
            __NFS_FILE_UNLOCK(pnfsfile);
            return  ((ssize_t)stTotal);
        
        } else if (res.status != NFS3_OK) {
            if (stTotal == 0) {
                _ErrorHandle(res.status);
            }
            xdr_free((xdrproc_t)xdr_READ3res, (char *)&res);
            __NFS_FILE_UNLOCK(pnfsfile);
            return  ((ssize_t)stTotal);
        }
        
        if (res.READ3res_u.resok.eof) {                                 /*  已读取到文件结尾            */
            bIsEof = LW_TRUE;
        }
        
        lib_memcpy(pcBuffer + stTotal, 
                   res.READ3res_u.resok.data.data_val, 
                   (UINT)res.READ3res_u.resok.count);                   /*  拷贝                        */
        
        pfdentry->FDENTRY_oftPtr += (off_t)res.READ3res_u.resok.count;  /*  更新文件指针                */
        stTotal += (size_t)res.READ3res_u.resok.count;                  /*  已读取数据量                */
        
        xdr_free((xdrproc_t)xdr_READ3res, (char *)&res);
    }

    __NFS_FILE_UNLOCK(pnfsfile);
    
    return  ((ssize_t)stTotal);
}
/*********************************************************************************************************
** 函数名称: __nfsPRead
** 功能描述: nfs pread 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
**           oftPos           位置
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __nfsPRead (PLW_FD_ENTRY   pfdentry,
                            PCHAR          pcBuffer,
                            size_t         stMaxBytes,
                            off_t          oftPos)
{
    enum clnt_stat  clntstat;
    
    READ3args   args;
    READ3res    res;

    PLW_FD_NODE pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PNFS_FILE   pnfsfile = (PNFS_FILE)pfdnode->FDNODE_pvFile;
    size_t      stTotal  = 0;
    count3      countOnce;
    BOOL        bIsEof   = LW_FALSE;                                    /*  是否到文件尾部              */

    if (!pcBuffer || !stMaxBytes || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    if (__NFS_FILE_LOCK(pnfsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (pnfsfile->NFSFIL_iFileType != __NFS_FILE_TYPE_NODE) {
        __NFS_FILE_UNLOCK(pnfsfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    while (stTotal < stMaxBytes) {                                      /*  没有读取完全                */
        if (bIsEof) {
            __NFS_FILE_UNLOCK(pnfsfile);
            return  ((ssize_t)stTotal);
        }
    
        countOnce = (count3)(stMaxBytes - stTotal);
        if (countOnce > __NFS_ONE_TRANS_SIZE(pnfsfile)) {
            countOnce = __NFS_ONE_TRANS_SIZE(pnfsfile);                 /*  单次最大传输量              */
        }
    
        args.file   = pnfsfile->NFSFIL_handle;
        args.offset = (offset3)oftPos;
        args.count  = countOnce;
        
        lib_bzero(&res, sizeof(res));
        
        clntstat = nfsproc3_read_3(args, &res, pnfsfile->NFSFIL_nfsfs->NFSFS_pclient);
        if (clntstat != RPC_SUCCESS) {
            if (stTotal == 0) {
                __nfsRpcErrorHandle(clntstat);
            }
            __NFS_FILE_UNLOCK(pnfsfile);
            return  ((ssize_t)stTotal);
        
        } else if (res.status != NFS3_OK) {
            if (stTotal == 0) {
                _ErrorHandle(res.status);
            }
            xdr_free((xdrproc_t)xdr_READ3res, (char *)&res);
            __NFS_FILE_UNLOCK(pnfsfile);
            return  ((ssize_t)stTotal);
        }
        
        if (res.READ3res_u.resok.eof) {                                 /*  已读取到文件结尾            */
            bIsEof = LW_TRUE;
        }
        
        lib_memcpy(pcBuffer + stTotal, 
                   res.READ3res_u.resok.data.data_val, 
                   (UINT)res.READ3res_u.resok.count);                   /*  拷贝                        */
        
        oftPos  += (off_t)res.READ3res_u.resok.count;                   /*  计算指针                    */
        stTotal += (size_t)res.READ3res_u.resok.count;                  /*  已读取数据量                */
                   
        xdr_free((xdrproc_t)xdr_READ3res, (char *)&res);
    }

    __NFS_FILE_UNLOCK(pnfsfile);
    
    return  ((ssize_t)stTotal);
}
/*********************************************************************************************************
** 函数名称: __nfsWrite
** 功能描述: nfs write 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         缓冲区
**           stNBytes         需要写入的数据
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __nfsWrite (PLW_FD_ENTRY  pfdentry,
                            PCHAR         pcBuffer,
                            size_t        stNBytes)
{
    enum clnt_stat  clntstat;

    WRITE3args  args;
    WRITE3res   res;

    PLW_FD_NODE pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PNFS_FILE   pnfsfile = (PNFS_FILE)pfdnode->FDNODE_pvFile;
    size_t      stTotal  = 0;
    count3      countOnce;

    if (!pcBuffer || !stNBytes) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    if (__NFS_FILE_LOCK(pnfsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (pnfsfile->NFSFIL_nfsfs->NFSFS_iFlag == O_RDONLY) {
        __NFS_FILE_UNLOCK(pnfsfile);
        _ErrorHandle(EROFS);
        return  (PX_ERROR);
    }
    
    if (pnfsfile->NFSFIL_iFileType != __NFS_FILE_TYPE_NODE) {
        __NFS_FILE_UNLOCK(pnfsfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (pfdentry->FDENTRY_iFlag & O_APPEND) {                           /*  追加模式                    */
        pfdentry->FDENTRY_oftPtr = pfdnode->FDNODE_oftSize;             /*  移动读写指针到末尾          */
    }
    
    while (stTotal < stNBytes) {                                        /*  没有写完全                  */
        countOnce = (count3)(stNBytes - stTotal);
        if (countOnce > __NFS_ONE_TRANS_SIZE(pnfsfile)) {
            countOnce = __NFS_ONE_TRANS_SIZE(pnfsfile);                 /*  单次最大传输量              */
            args.stable = UNSTABLE;
        } else {
            args.stable = FILE_SYNC;
        }
        
        args.file   = pnfsfile->NFSFIL_handle;
        args.offset = (offset3)(pfdentry->FDENTRY_oftPtr);
        args.count  = countOnce;
        
        args.data.data_val = pcBuffer + stTotal;
        args.data.data_len = (u_int)args.count;
        
        lib_bzero(&res, sizeof(res));
        
        clntstat = nfsproc3_write_3(args, &res, pnfsfile->NFSFIL_nfsfs->NFSFS_pclient);
        if (clntstat != RPC_SUCCESS) {
            if (stTotal == 0) {
                __nfsRpcErrorHandle(clntstat);
            }
            __NFS_FILE_UNLOCK(pnfsfile);
            return  ((ssize_t)stTotal);
            
        } else if (res.status != NFS3_OK) {
            if (stTotal == 0) {
                _ErrorHandle(res.status);
            }
            xdr_free((xdrproc_t)xdr_WRITE3res, (char *)&res);
            __NFS_FILE_UNLOCK(pnfsfile);
            return  ((ssize_t)stTotal);
        }
        
        pfdentry->FDENTRY_oftPtr += (off_t)res.WRITE3res_u.resok.count; /*  更新文件指针                */
        stTotal += (size_t)res.WRITE3res_u.resok.count;                 /*  已写入数据量                */
        
        __nfsAttr2Stat(pnfsfile->NFSFIL_nfsfs, 
            &res.WRITE3res_u.resok.file_wcc.after.post_op_attr_u.attributes, 
            &pnfsfile->NFSFIL_stat);                                    /*  更新 stat                   */
            
        pfdnode->FDNODE_oftSize = pnfsfile->NFSFIL_stat.st_size;        /*  更新文件大小                */
        
        xdr_free((xdrproc_t)xdr_WRITE3res, (char *)&res);
    }
    
    __NFS_FILE_UNLOCK(pnfsfile);
    
    return  ((ssize_t)stTotal);
}
/*********************************************************************************************************
** 函数名称: __nfsPWrite
** 功能描述: nfs pwrite 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         缓冲区
**           stNBytes         需要写入的数据
**           oftPos           位置
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __nfsPWrite (PLW_FD_ENTRY  pfdentry,
                             PCHAR         pcBuffer,
                             size_t        stNBytes,
                             off_t         oftPos)
{
    enum clnt_stat  clntstat;

    WRITE3args  args;
    WRITE3res   res;

    PLW_FD_NODE pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PNFS_FILE   pnfsfile = (PNFS_FILE)pfdnode->FDNODE_pvFile;
    size_t      stTotal  = 0;
    count3      countOnce;

    if (!pcBuffer || !stNBytes || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    if (__NFS_FILE_LOCK(pnfsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (pnfsfile->NFSFIL_nfsfs->NFSFS_iFlag == O_RDONLY) {
        __NFS_FILE_UNLOCK(pnfsfile);
        _ErrorHandle(EROFS);
        return  (PX_ERROR);
    }
    
    if (pnfsfile->NFSFIL_iFileType != __NFS_FILE_TYPE_NODE) {
        __NFS_FILE_UNLOCK(pnfsfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    while (stTotal < stNBytes) {                                        /*  没有写完全                  */
        countOnce = (count3)(stNBytes - stTotal);
        if (countOnce > __NFS_ONE_TRANS_SIZE(pnfsfile)) {
            countOnce = __NFS_ONE_TRANS_SIZE(pnfsfile);                 /*  单次最大传输量              */
            args.stable = UNSTABLE;
        } else {
            args.stable = FILE_SYNC;
        }
        
        args.file   = pnfsfile->NFSFIL_handle;
        args.offset = (offset3)oftPos;
        args.count  = countOnce;
        
        args.data.data_val = pcBuffer + stTotal;
        args.data.data_len = (u_int)args.count;
        
        lib_bzero(&res, sizeof(res));
        
        clntstat = nfsproc3_write_3(args, &res, pnfsfile->NFSFIL_nfsfs->NFSFS_pclient);
        if (clntstat != RPC_SUCCESS) {
            if (stTotal == 0) {
                __nfsRpcErrorHandle(clntstat);
            }
            __NFS_FILE_UNLOCK(pnfsfile);
            return  ((ssize_t)stTotal);
            
        } else if (res.status != NFS3_OK) {
            if (stTotal == 0) {
                _ErrorHandle(res.status);
            }
            xdr_free((xdrproc_t)xdr_WRITE3res, (char *)&res);
            __NFS_FILE_UNLOCK(pnfsfile);
            return  ((ssize_t)stTotal);
        }
        
        oftPos  += (off_t)res.WRITE3res_u.resok.count;                  /*  计算指针                    */
        stTotal += (size_t)res.WRITE3res_u.resok.count;                 /*  已写入数据量                */
        
        __nfsAttr2Stat(pnfsfile->NFSFIL_nfsfs, 
            &res.WRITE3res_u.resok.file_wcc.after.post_op_attr_u.attributes, 
            &pnfsfile->NFSFIL_stat);                                    /*  更新 stat                   */
            
        pfdnode->FDNODE_oftSize = pnfsfile->NFSFIL_stat.st_size;        /*  更新文件大小                */
        
        xdr_free((xdrproc_t)xdr_WRITE3res, (char *)&res);
    }
    
    __NFS_FILE_UNLOCK(pnfsfile);
    
    return  ((ssize_t)stTotal);
}
/*********************************************************************************************************
** 函数名称: __nfsNRead
** 功能描述: nfs nread 操作
** 输　入  : pfdentry         文件控制块
**           piNRead          剩余数据量
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsNRead (PLW_FD_ENTRY  pfdentry, INT  *piNRead)
{
    PLW_FD_NODE     pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PNFS_FILE       pnfsfile = (PNFS_FILE)pfdnode->FDNODE_pvFile;

    if (piNRead == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__NFS_FILE_LOCK(pnfsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    *piNRead = (INT)(pfdnode->FDNODE_oftSize - pfdentry->FDENTRY_oftPtr);
    
    __NFS_FILE_UNLOCK(pnfsfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsNRead64
** 功能描述: nfs nread 操作
** 输　入  : pfdentry         文件控制块
**           poftNRead        剩余数据量
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsNRead64 (PLW_FD_ENTRY  pfdentry, off_t  *poftNRead)
{
    PLW_FD_NODE     pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PNFS_FILE       pnfsfile = (PNFS_FILE)pfdnode->FDNODE_pvFile;
    
    if (__NFS_FILE_LOCK(pnfsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    *poftNRead = (pfdnode->FDNODE_oftSize - pfdentry->FDENTRY_oftPtr);
    
    __NFS_FILE_UNLOCK(pnfsfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsSeek
** 功能描述: nfs seek 操作
** 输　入  : pfdentry         文件控制块
**           oftOffset        偏移量
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsSeek (PLW_FD_ENTRY  pfdentry,
                       off_t         oftOffset)
{
    PLW_FD_NODE     pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PNFS_FILE       pnfsfile = (PNFS_FILE)pfdnode->FDNODE_pvFile;
    
    if (__NFS_FILE_LOCK(pnfsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    if (pnfsfile->NFSFIL_nfsfs->NFSFS_iFlag == O_RDONLY) {
        if (oftOffset > pfdnode->FDNODE_oftSize) {                      /*  只读文件系统不能扩大文件    */
            __NFS_FILE_UNLOCK(pnfsfile);
            _ErrorHandle(EROFS);
            return  (PX_ERROR);
        }
    }
    if (pnfsfile->NFSFIL_iFileType != __NFS_FILE_TYPE_NODE) {
        __NFS_FILE_UNLOCK(pnfsfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    pfdentry->FDENTRY_oftPtr = oftOffset;
    
    __NFS_FILE_UNLOCK(pnfsfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsWhere
** 功能描述: nfs 获得文件当前读写指针位置 (使用参数作为返回值, 与 FIOWHERE 的要求稍有不同)
** 输　入  : pfdentry            文件控制块
**           poftPos             读写指针位置
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsWhere (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    PLW_FD_NODE     pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PNFS_FILE       pnfsfile = (PNFS_FILE)pfdnode->FDNODE_pvFile;

    if (__NFS_FILE_LOCK(pnfsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (poftPos) {
        *poftPos = pfdentry->FDENTRY_oftPtr;
        __NFS_FILE_UNLOCK(pnfsfile);
        return  (ERROR_NONE);
    }

    __NFS_FILE_UNLOCK(pnfsfile);
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __nfsRename
** 功能描述: nfs rename 操作
** 输　入  : pfdentry         文件控制块
**           pcNewName        新文件名
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsRename (PLW_FD_ENTRY  pfdentry, PCHAR  pcNewName)
{
             CHAR           cNewPath[PATH_MAX + 1];
    REGISTER PCHAR          pcNewPath = &cNewPath[0];
             NFS_FS        *pnfsfsNew;

             enum clnt_stat  clntstat;

             RENAME3args    args;
             RENAME3res     res;
             
             nfs_fh3        handleFromDir;
             nfs_fh3        handleToDir;
             
             PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
             PNFS_FILE      pnfsfile = (PNFS_FILE)pfdnode->FDNODE_pvFile;
             
             PCHAR          pcFromFile;
             PCHAR          pcToFile;

    if (__STR_IS_ROOT(pnfsfile->NFSFIL_cName)) {                        /*  检查是否为设备文件          */
        _ErrorHandle(ENOSYS);                                           /*  不支持设备重命名            */
        return  (PX_ERROR);
    }
    if ((pcNewName == LW_NULL) || __STR_IS_ROOT(pcNewName)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (__NFS_FILE_LOCK(pnfsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if ((pnfsfile->NFSFIL_iFileType == __NFS_FILE_TYPE_NODE) ||
        (pnfsfile->NFSFIL_iFileType == __NFS_FILE_TYPE_DIR)) {          /*  open 创建的普通文件或目录   */
        if (ioFullFileNameGet(pcNewName, 
                              (LW_DEV_HDR **)&pnfsfsNew, 
                              cNewPath) != ERROR_NONE) {                /*  获得新目录路径              */
            __NFS_FILE_UNLOCK(pnfsfile);
            return  (PX_ERROR);
        }
        if (pnfsfsNew != pnfsfile->NFSFIL_nfsfs) {                      /*  必须为同一个 nfs 设备节点   */
            __NFS_FILE_UNLOCK(pnfsfile);
            _ErrorHandle(EXDEV);
            return  (PX_ERROR);
        }
        
        if (cNewPath[0] == PX_DIVIDER) {
            pcNewPath++;
        }
        
        if (pnfsfile->NFSFIL_iFileType == __NFS_FILE_TYPE_DIR) {
            if (_PathMoveCheck(pnfsfile->NFSFIL_cName, pcNewPath)) {    /*  如果时目录项则检查          */
                __NFS_FILE_UNLOCK(pnfsfile);
                _ErrorHandle(EINVAL);
                return  (PX_ERROR);
            }
        }

        if (__nfsGetDir(pnfsfile->NFSFIL_nfsfs, pnfsfile->NFSFIL_cName, 
                        &handleFromDir, &pcFromFile, LW_NULL)) {
            __NFS_FILE_UNLOCK(pnfsfile);
            return  (PX_ERROR);
        }
        if (__nfsGetDir(pnfsfile->NFSFIL_nfsfs, cNewPath, &handleToDir, 
                        &pcToFile, LW_NULL)) {
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handleFromDir);
            __NFS_FILE_UNLOCK(pnfsfile);
            return  (PX_ERROR);
        }
        if (!pcFromFile || !pcToFile) {
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handleFromDir);
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handleToDir);
            __NFS_FILE_UNLOCK(pnfsfile);
            return  (PX_ERROR);
        }
        
        args.from.dir  = handleFromDir;
        args.from.name = pcFromFile;
        args.to.dir  = handleToDir;
        args.to.name = pcToFile;
        
        lib_bzero(&res, sizeof(res));
        
        clntstat = nfsproc3_rename_3(args, &res, pnfsfile->NFSFIL_nfsfs->NFSFS_pclient);
        if (clntstat != RPC_SUCCESS) {
            __nfsRpcErrorHandle(clntstat);
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handleFromDir);
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handleToDir);
            __NFS_FILE_UNLOCK(pnfsfile);
            return  (PX_ERROR);
        
        } else if (res.status != NFS3_OK) {
            _ErrorHandle(res.status);
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handleFromDir);
            xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handleToDir);
            xdr_free((xdrproc_t)xdr_RENAME3res, (char *)&res);
            __NFS_FILE_UNLOCK(pnfsfile);
            return  (PX_ERROR);
        }
        
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handleFromDir);
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handleToDir);
        xdr_free((xdrproc_t)xdr_RENAME3res, (char *)&res);
        __NFS_FILE_UNLOCK(pnfsfile);
        return  (ERROR_NONE);
    }
    
    __NFS_FILE_UNLOCK(pnfsfile);
    
    _ErrorHandle(EINVAL);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __nfsStatGet
** 功能描述: nfs stat 操作
** 输　入  : pfdentry         文件控制块
**           pstat            文件状态
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsStat (PLW_FD_ENTRY  pfdentry, struct stat *pstat)
{
    PLW_FD_NODE     pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PNFS_FILE       pnfsfile = (PNFS_FILE)pfdnode->FDNODE_pvFile;
    
    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (__NFS_FILE_LOCK(pnfsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    *pstat = pnfsfile->NFSFIL_stat;
    
    __NFS_FILE_UNLOCK(pnfsfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsStatGet64
** 功能描述: nfs stat 操作
** 输　入  : pfdentry         文件控制块
**           pstat64          文件状态
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsStat64 (PLW_FD_ENTRY  pfdentry, struct stat64 *pstat64)
{
    PLW_FD_NODE     pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PNFS_FILE       pnfsfile = (PNFS_FILE)pfdnode->FDNODE_pvFile;
    
    if (!pstat64) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (__NFS_FILE_LOCK(pnfsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    pstat64->st_dev     = pnfsfile->NFSFIL_stat.st_dev;
    pstat64->st_ino     = (ino64_t)pnfsfile->NFSFIL_stat.st_ino;
    pstat64->st_mode    = pnfsfile->NFSFIL_stat.st_mode;
    pstat64->st_nlink   = pnfsfile->NFSFIL_stat.st_nlink;
    pstat64->st_uid     = pnfsfile->NFSFIL_stat.st_uid;
    pstat64->st_gid     = pnfsfile->NFSFIL_stat.st_gid;
    pstat64->st_rdev    = pnfsfile->NFSFIL_stat.st_rdev;
    pstat64->st_size    = pnfsfile->NFSFIL_stat.st_size;
    pstat64->st_atime   = pnfsfile->NFSFIL_stat.st_atime;
    pstat64->st_mtime   = pnfsfile->NFSFIL_stat.st_mtime;
    pstat64->st_ctime   = pnfsfile->NFSFIL_stat.st_ctime;
    pstat64->st_blksize = pnfsfile->NFSFIL_stat.st_blksize;
    pstat64->st_blocks  = (blkcnt64_t)pnfsfile->NFSFIL_stat.st_dev;
    
    if (sizeof(ino64_t) != sizeof(ino_t)) {
        ULONG   ulLow   = (ULONG)pnfsfile->NFSFIL_stat.st_resv2;
        ULONG   ulHigh  = (ULONG)pnfsfile->NFSFIL_stat.st_resv3;
        pstat64->st_ino = (ino64_t)(((ino64_t)ulLow) + ((ino64_t)ulHigh << 32));
    }

    __NFS_FILE_UNLOCK(pnfsfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsLStat
** 功能描述: nfs stat 操作
** 输　入  : pnfsfs           设备
**           pcName           文件名
**           pstat            文件状态
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsLStat (PNFS_FS  pnfsfs, PCHAR  pcName, struct stat *pstat)
{
    BOOL        bIsDir;
    nfs_fh3     handle;
    
    if (!pcName || !pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__NFS_VOL_LOCK(pnfsfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (__nfsOpenFile(pnfsfs, pcName, O_RDONLY, 0, &handle, &bIsDir, pstat)) {
        __NFS_VOL_UNLOCK(pnfsfs);
        return  (PX_ERROR);
    }
    
    xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handle);                  /*  等于同时释放了 handle       */
    
    __NFS_VOL_UNLOCK(pnfsfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsStatfs
** 功能描述: nfs statfs 操作
** 输　入  : pfdentry         文件控制块
**           pstatfs          文件系统状态
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsStatfs (PLW_FD_ENTRY  pfdentry, struct statfs *pstatfs)
{
    enum clnt_stat  clntstat;

    FSSTAT3args     args;
    FSSTAT3res      res;
    
    PLW_FD_NODE     pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PNFS_FILE       pnfsfile = (PNFS_FILE)pfdnode->FDNODE_pvFile;
    
    if (!pstatfs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__NFS_FILE_LOCK(pnfsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    args.fsroot = pnfsfile->NFSFIL_handle;                              /*  以当前路径获取文件系统信息  */

    lib_bzero(&res, sizeof(res));
    
    clntstat = nfsproc3_fsstat_3(args, &res, pnfsfile->NFSFIL_nfsfs->NFSFS_pclient);
    if (clntstat != RPC_SUCCESS) {
        __nfsRpcErrorHandle(clntstat);
        __NFS_FILE_UNLOCK(pnfsfile);
        return  (PX_ERROR);
    
    } else if (res.status != NFS3_OK) {
        _ErrorHandle(res.status);
        xdr_free((xdrproc_t)xdr_FSSTAT3res, (char *)&res);
        __NFS_FILE_UNLOCK(pnfsfile);
        return  (PX_ERROR);
    }

    pstatfs->f_type  = NFS_SUPER_MAGIC;                                 /* type of info, zero for now   */
    pstatfs->f_bsize = __NFS_BLOCK_SIZE;
    
    pstatfs->f_blocks = (long)(res.FSSTAT3res_u.resok.tbytes / __NFS_BLOCK_SIZE);
    pstatfs->f_bfree  = (long)(res.FSSTAT3res_u.resok.fbytes / __NFS_BLOCK_SIZE);
    pstatfs->f_bavail = (long)(res.FSSTAT3res_u.resok.abytes / __NFS_BLOCK_SIZE);
    
    pstatfs->f_files  = (long)res.FSSTAT3res_u.resok.tfiles;            /* total file nodes in fs       */
    pstatfs->f_ffree  = (long)res.FSSTAT3res_u.resok.ffiles;            /* free file nodes in fs        */
    
#if LW_CFG_CPU_WORD_LENGHT == 64
    pstatfs->f_fsid.val[0] = (int32_t)((addr_t)pnfsfile->NFSFIL_nfsfs >> 32);
    pstatfs->f_fsid.val[1] = (int32_t)((addr_t)pnfsfile->NFSFIL_nfsfs & 0xffffffff);
#else
    pstatfs->f_fsid.val[0] = (int32_t)pnfsfile->NFSFIL_nfsfs;
    pstatfs->f_fsid.val[1] = 0;
#endif
    
    pstatfs->f_flag = ST_NOSUID;
    if (pnfsfile->NFSFIL_nfsfs->NFSFS_iFlag == O_RDONLY) {
        pstatfs->f_flag |= ST_RDONLY;
    }
    pstatfs->f_namelen = PATH_MAX;
    
    xdr_free((xdrproc_t)xdr_FSSTAT3res, (char *)&res);

    __NFS_FILE_UNLOCK(pnfsfile);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsChmod
** 功能描述: nfs chmod 操作
** 输　入  : pfdentry            文件控制块
**           iMode               新的 mode
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsChmod (PLW_FD_ENTRY  pfdentry, INT  iMode)
{
    enum clnt_stat  clntstat;
    
    SETATTR3args    attrargs;
    SETATTR3res     attrres;
    
    PLW_FD_NODE     pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PNFS_FILE       pnfsfile = (PNFS_FILE)pfdnode->FDNODE_pvFile;
    
    iMode |= S_IRUSR;
    
    lib_bzero(&attrargs, sizeof(attrargs));
    lib_bzero(&attrres, sizeof(attrres));
    
    if (__NFS_FILE_LOCK(pnfsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (pnfsfile->NFSFIL_stat.st_mode == iMode) {                       /*  没有变化                    */
        __NFS_FILE_UNLOCK(pnfsfile);
        return  (ERROR_NONE);
    }
    
    attrargs.object = pnfsfile->NFSFIL_handle;
    attrargs.new_attributes.mode.set_it = LW_TRUE;
    attrargs.new_attributes.mode.set_mode3_u.mode = iMode;
    
    clntstat = nfsproc3_setattr_3(attrargs, &attrres, pnfsfile->NFSFIL_nfsfs->NFSFS_pclient);
    if (clntstat != RPC_SUCCESS) {
        __nfsRpcErrorHandle(clntstat);
        __NFS_FILE_UNLOCK(pnfsfile);
        return  (PX_ERROR);
    
    } else if (attrres.status != NFS3_OK) {
        _ErrorHandle(attrres.status);
        xdr_free((xdrproc_t)xdr_SETATTR3res, (char *)&attrres);
        __NFS_FILE_UNLOCK(pnfsfile);
        return  (PX_ERROR);
    }
    xdr_free((xdrproc_t)xdr_SETATTR3res, (char *)&attrres);
    
    pnfsfile->NFSFIL_stat.st_mode = iMode;

    __NFS_FILE_UNLOCK(pnfsfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsReadDir
** 功能描述: nfs 获得指定目录信息
** 输　入  : pfdentry            文件控制块
**           dir                 目录结构
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsReadDir (PLW_FD_ENTRY  pfdentry, DIR  *dir)
{
    enum clnt_stat  clntstat;
    
    PLW_FD_NODE     pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PNFS_FILE       pnfsfile = (PNFS_FILE)pfdnode->FDNODE_pvFile;
    
    NFS_IN_DIR     *pnfsid;
    
    if (!dir) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__NFS_FILE_LOCK(pnfsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (pnfsfile->NFSFIL_iFileType == __NFS_FILE_TYPE_NODE) {
        __NFS_FILE_UNLOCK(pnfsfile);
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }
    
    pnfsid = &pnfsfile->NFSFIL_nfsid;
    
    if (dir->dir_pos == 0) {
        pnfsid->NFSID_entry = LW_NULL;
        lib_bzero(&pnfsid->NFSID_cookie, sizeof(cookie3));
        lib_bzero(&pnfsid->NFSID_cookieverf, sizeof(cookieverf3));
        
        xdr_free((xdrproc_t)xdr_READDIR3res, (char *)&pnfsid->NFSID_res);
        lib_bzero(&pnfsid->NFSID_res, sizeof(pnfsid->NFSID_res));
    }
    
    if (pnfsid->NFSID_entry == LW_NULL) {                               /*  从头开始, 或者继续读取      */
        nfs_fh3      handle;
        READDIR3args args;
        
        if (pnfsfile->NFSFIL_iFileType == __NFS_FILE_TYPE_DEV) {
            handle = pnfsfile->NFSFIL_nfsfs->NFSFS_hRoot;
        } else {
            handle = pnfsfile->NFSFIL_handle;
        }
        xdr_free((xdrproc_t)xdr_READDIR3res, (char *)&pnfsid->NFSID_res);
        lib_bzero(&pnfsid->NFSID_res, sizeof(pnfsid->NFSID_res));
        
        args.dir = handle;
        args.cookie = pnfsid->NFSID_cookie;
        lib_memcpy(&args.cookieverf, &pnfsid->NFSID_cookieverf, sizeof(cookieverf3));
        /*
         * For reasons yet unknown to me, the HP server expects the value of
         * count to be AT LEAST 512 in order to successfully read directory files
         * mounted from HP. Sun server does not have this limitation.
         */
        args.count = __NFS_READDIR_MAXLEN;
        
        clntstat = nfsproc3_readdir_3(args, &pnfsid->NFSID_res, pnfsfile->NFSFIL_nfsfs->NFSFS_pclient);
        if (clntstat != RPC_SUCCESS) {
            __nfsRpcErrorHandle(clntstat);
            __NFS_FILE_UNLOCK(pnfsfile);
            return  (PX_ERROR);
        
        } else if (pnfsid->NFSID_res.status != NFS3_OK) {
            _ErrorHandle(pnfsid->NFSID_res.status);
            __NFS_FILE_UNLOCK(pnfsfile);
            return  (PX_ERROR);
        }
        
        memcpy(&pnfsid->NFSID_cookieverf, 
               &pnfsid->NFSID_res.READDIR3res_u.resok.cookieverf, 
               sizeof(cookieverf3));
        pnfsid->NFSID_bEof  = pnfsid->NFSID_res.READDIR3res_u.resok.reply.eof;
        pnfsid->NFSID_entry = pnfsid->NFSID_res.READDIR3res_u.resok.reply.entries;
    }
    
    /*
     *  NFSID_bEof 表示 NFSID_entry 链表是否包含了, 整个目录的情况, 
     *  如果没有, 读取完 entry 链表后则需要继续再次 nfsproc3_readdir_3 操作.
     *  NFSID_cookie 则保存了上一个目录的信息, 他将帮助 nfsproc3_readdir_3 正确获取下一次的读取.
     *  每一次的 nfsproc3_readdir_3 操作, 都将获取一个 cookieverf 用于下次的 nfsproc3_readdir_3
     */
__get_one_entry:
    if ((pnfsid->NFSID_bEof) && (pnfsid->NFSID_entry == LW_NULL)) {     /*  全部读取结束                */
        __NFS_FILE_UNLOCK(pnfsfile);
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    if (pnfsid->NFSID_entry) {
        if ((lib_strcmp(pnfsid->NFSID_entry->name, ".") == 0) ||
            (lib_strcmp(pnfsid->NFSID_entry->name, "..") == 0)) {
            pnfsid->NFSID_cookie = pnfsid->NFSID_entry->cookie;
            pnfsid->NFSID_entry = pnfsid->NFSID_entry->nextentry;
            goto    __get_one_entry;
        }
    
        strlcpy(dir->dir_dirent.d_name, pnfsid->NFSID_entry->name, PATH_MAX + 1);
        dir->dir_dirent.d_shortname[0] = PX_EOS;
        dir->dir_dirent.d_type = DT_UNKNOWN;
        dir->dir_pos++;
        
        pnfsid->NFSID_cookie = pnfsid->NFSID_entry->cookie;
        pnfsid->NFSID_entry = pnfsid->NFSID_entry->nextentry;
    }
    
    __NFS_FILE_UNLOCK(pnfsfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsTimeset
** 功能描述: nfs 设置文件时间
** 输　入  : pfdentry            文件控制块
**           utim                utimbuf 结构
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
** 注  意  : 目前此函数不进行任何处理.
*********************************************************************************************************/
static INT  __nfsTimeset (PLW_FD_ENTRY  pfdentry, struct utimbuf  *utim)
{
    enum clnt_stat  clntstat;

    SETATTR3args    attrargs;
    SETATTR3res     attrres;
    
    PLW_FD_NODE     pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PNFS_FILE       pnfsfile = (PNFS_FILE)pfdnode->FDNODE_pvFile;
    
    if (!utim) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    lib_bzero(&attrargs, sizeof(attrargs));
    lib_bzero(&attrres, sizeof(attrres));
    
    if (__NFS_FILE_LOCK(pnfsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    attrargs.object = pnfsfile->NFSFIL_handle;
    attrargs.new_attributes.atime.set_it = SET_TO_CLIENT_TIME;
    attrargs.new_attributes.atime.set_atime_u.atime.seconds = (uint32)utim->actime;
    attrargs.new_attributes.mtime.set_it = SET_TO_CLIENT_TIME;
    attrargs.new_attributes.mtime.set_mtime_u.mtime.seconds = (uint32)utim->modtime;
    
    clntstat = nfsproc3_setattr_3(attrargs, &attrres, pnfsfile->NFSFIL_nfsfs->NFSFS_pclient);
    if (clntstat != RPC_SUCCESS) {
        __nfsRpcErrorHandle(clntstat);
        __NFS_FILE_UNLOCK(pnfsfile);
        return  (PX_ERROR);
    
    } else if (attrres.status != NFS3_OK) {
        _ErrorHandle(attrres.status);
        xdr_free((xdrproc_t)xdr_SETATTR3res, (char *)&attrres);
        __NFS_FILE_UNLOCK(pnfsfile);
        return  (PX_ERROR);
    }
    xdr_free((xdrproc_t)xdr_SETATTR3res, (char *)&attrres);
    
    __NFS_FILE_UNLOCK(pnfsfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsTruncate
** 功能描述: nfs 设置文件大小
** 输　入  : pfdentry            文件控制块
**           oftSize             文件大小
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsTruncate (PLW_FD_ENTRY  pfdentry, off_t  oftSize)
{
    enum clnt_stat  clntstat;
    
    SETATTR3args    attrargs;
    SETATTR3res     attrres;
    
    PLW_FD_NODE     pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PNFS_FILE       pnfsfile = (PNFS_FILE)pfdnode->FDNODE_pvFile;
    
    lib_bzero(&attrargs, sizeof(attrargs));
    lib_bzero(&attrres, sizeof(attrres));
    
    if (__NFS_FILE_LOCK(pnfsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (pnfsfile->NFSFIL_iFileType != __NFS_FILE_TYPE_NODE) {
        __NFS_FILE_UNLOCK(pnfsfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    attrargs.object = pnfsfile->NFSFIL_handle;
    attrargs.new_attributes.size.set_it = LW_TRUE;
    attrargs.new_attributes.size.set_size3_u.size = oftSize;
    
    clntstat = nfsproc3_setattr_3(attrargs, &attrres, pnfsfile->NFSFIL_nfsfs->NFSFS_pclient);
    if (clntstat != RPC_SUCCESS) {
        __nfsRpcErrorHandle(clntstat);
        __NFS_FILE_UNLOCK(pnfsfile);
        return  (PX_ERROR);
    
    } else if (attrres.status != NFS3_OK) {
        _ErrorHandle(attrres.status);
        xdr_free((xdrproc_t)xdr_SETATTR3res, (char *)&attrres);
        __NFS_FILE_UNLOCK(pnfsfile);
        return  (PX_ERROR);
    }
    xdr_free((xdrproc_t)xdr_SETATTR3res, (char *)&attrres);
    
    pnfsfile->NFSFIL_stat.st_size = oftSize;
    
    __NFS_FILE_UNLOCK(pnfsfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsFlush
** 功能描述: nfs 文件回写服务器
** 输　入  : pfdentry            文件控制块
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
** 注  意  : 目前此函数不进行任何处理.
*********************************************************************************************************/
static INT  __nfsFlush (PLW_FD_ENTRY  pfdentry)
{
    enum clnt_stat  clntstat;
    
    GETATTR3args    args;
    GETATTR3res     res;
    
    COMMIT3args     commitargs;
    COMMIT3res      commitres;
    
    PLW_FD_NODE     pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PNFS_FILE       pnfsfile = (PNFS_FILE)pfdnode->FDNODE_pvFile;
    
    if (__NFS_FILE_LOCK(pnfsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (pnfsfile->NFSFIL_iFileType != __NFS_FILE_TYPE_NODE) {
        __NFS_FILE_UNLOCK(pnfsfile);
        return  (ERROR_NONE);
    }
    
    args.object = pnfsfile->NFSFIL_handle;
    
    lib_bzero(&res, sizeof(res));
    
    clntstat = nfsproc3_getattr_3(args, &res, pnfsfile->NFSFIL_nfsfs->NFSFS_pclient);
    if (clntstat != RPC_SUCCESS) {
        __nfsRpcErrorHandle(clntstat);
        __NFS_FILE_UNLOCK(pnfsfile);
        return  (PX_ERROR);
    
    } else if (res.status != NFS3_OK) {
        _ErrorHandle(res.status);
        xdr_free((xdrproc_t)xdr_GETATTR3res, (char *)&res);
        __NFS_FILE_UNLOCK(pnfsfile);
        return  (PX_ERROR);
    }
    xdr_free((xdrproc_t)xdr_GETATTR3res, (char *)&res);
    
    commitargs.file   = pnfsfile->NFSFIL_handle;
    commitargs.offset = 0;
    commitargs.count  = (count3)res.GETATTR3res_u.resok.obj_attributes.size;

    lib_bzero(&commitres, sizeof(commitres));
    
    clntstat = nfsproc3_commit_3(commitargs, &commitres, pnfsfile->NFSFIL_nfsfs->NFSFS_pclient);
    if (clntstat != RPC_SUCCESS) {
        __nfsRpcErrorHandle(clntstat);
        __NFS_FILE_UNLOCK(pnfsfile);
        return  (PX_ERROR);
    }
    xdr_free((xdrproc_t)xdr_COMMIT3res, (char *)&commitres);

    __NFS_FILE_UNLOCK(pnfsfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsSymlink
** 功能描述: nfs 创建符号链接文件
** 输　入  : pnfsfs              设备
**           pcName              创建的连接文件
**           pcLinkDst           链接目标
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __nfsSymlink (PNFS_FS  pnfsfs, PCHAR  pcName, CPCHAR  pcLinkDst)
{
    nfs_fh3  handle;
    PCHAR    pcTail = LW_NULL;
    BOOL     bIsDir;
    
    enum clnt_stat  clntstat;
    
    SYMLINK3args    args;
    SYMLINK3res     res;
    
    if (!pcName || !pcLinkDst) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__fsCheckFileName(pcName)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__NFS_VOL_LOCK(pnfsfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (pnfsfs->NFSFS_iFlag == O_RDONLY) {
        __NFS_VOL_UNLOCK(pnfsfs);
        _ErrorHandle(EROFS);
        return  (PX_ERROR);
    }
    
    __nfsGetHandle(pnfsfs, pcName, &handle, &pcTail, &bIsDir, LW_NULL);
    
    if (!pcTail || (*pcTail == PX_EOS)) {                               /*  存在文件                    */
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handle);
        __NFS_VOL_UNLOCK(pnfsfs);
        _ErrorHandle(EEXIST);
        return  (PX_ERROR);
    }
    
    lib_bzero(&args, sizeof(args));
    lib_bzero(&res, sizeof(res));
    
    args.where.dir  = handle;
    args.where.name = pcTail;
    
    args.symlink.symlink_attributes.mode.set_it = LW_TRUE;
    args.symlink.symlink_attributes.mode.set_mode3_u.mode = DEFAULT_SYMLINK_PERM;
    args.symlink.symlink_attributes.uid.set_it = LW_TRUE;
    args.symlink.symlink_attributes.uid.set_uid3_u.uid = getuid();
    args.symlink.symlink_attributes.gid.set_it = LW_TRUE;
    args.symlink.symlink_attributes.gid.set_gid3_u.gid = getgid();
    args.symlink.symlink_attributes.size.set_it  = LW_FALSE;
    args.symlink.symlink_attributes.atime.set_it = SET_TO_SERVER_TIME;
    args.symlink.symlink_attributes.mtime.set_it = SET_TO_SERVER_TIME;
    
    args.symlink.symlink_data = (PCHAR)pcLinkDst;
    
    clntstat = nfsproc3_symlink_3(args, &res, pnfsfs->NFSFS_pclient);
    if (clntstat != RPC_SUCCESS) {
        __nfsRpcErrorHandle(clntstat);
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handle);
        __NFS_VOL_UNLOCK(pnfsfs);
        return  (PX_ERROR);
        
    } else if (res.status != NFS3_OK) {
        _ErrorHandle(res.status);
        xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handle);
        xdr_free((xdrproc_t)xdr_SYMLINK3res, (char *)&res);
        __NFS_VOL_UNLOCK(pnfsfs);
        return  (PX_ERROR);
    }
    xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handle);
    xdr_free((xdrproc_t)xdr_SYMLINK3res, (char *)&res);

    __NFS_VOL_UNLOCK(pnfsfs);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nfsReadlink
** 功能描述: nfs 读取符号链接文件内容
** 输　入  : pnfsfs              设备
**           pcName              链接原始文件名
**           pcLinkDst           链接目标文件名
**           stMaxSize           缓冲大小
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t __nfsReadlink (PNFS_FS       pnfsfs,
                              PCHAR         pcName,
                              PCHAR         pcLinkDst,
                              size_t        stMaxSize)
{
    nfs_fh3  handle;
    PCHAR    pcTail = LW_NULL;
    BOOL     bIsDir;
    INT      iError = PX_ERROR;
    
    if (__NFS_VOL_LOCK(pnfsfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    __nfsGetHandle(pnfsfs, pcName, &handle, &pcTail, &bIsDir, LW_NULL);
    
    if (!pcTail || (*pcTail == PX_EOS)) {                               /*  存在文件                    */
        iError = __nfsReadSymlink(pnfsfs, &handle, pcLinkDst, stMaxSize);
    }
    
    xdr_free((xdrproc_t)xdr_nfs_fh3, (char *)&handle);
    
    __NFS_VOL_UNLOCK(pnfsfs);
    
    if (iError == 0) {
        return  ((ssize_t)lib_strnlen(pcLinkDst, stMaxSize));
    } else {
        return  ((ssize_t)iError);
    }
}
/*********************************************************************************************************
** 函数名称: __nfsIoctl
** 功能描述: nfs ioctl 操作
** 输　入  : pnfsfile           nfs 文件控制块
**           request,           命令
**           arg                命令参数
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nfsIoctl (PLW_FD_ENTRY  pfdentry,
                        INT           iRequest,
                        LONG          lArg)
{
    PLW_FD_NODE     pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PNFS_FILE       pnfsfile = (PNFS_FILE)pfdnode->FDNODE_pvFile;
    
    off_t           oftSize;
    off_t           oftTemp;
    INT             iError;

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
        if (pnfsfile->NFSFIL_nfsfs->NFSFS_iFlag == O_RDONLY) {
            _ErrorHandle(EROFS);
            return  (PX_ERROR);
        }
    }
    
    switch (iRequest) {                                                 /*  只读文件系统判断            */
    
    case FIORENAME:
    case FIOTIMESET:
    case FIOCHMOD:
        if (pnfsfile->NFSFIL_nfsfs->NFSFS_iFlag == O_RDONLY) {
            _ErrorHandle(EROFS);
            return  (PX_ERROR);
        }
    }

    switch (iRequest) {

    case FIODISKINIT:                                                   /*  磁盘初始化                  */
        return  (ERROR_NONE);

    case FIORENAME:                                                     /*  文件重命名                  */
        return  (__nfsRename(pfdentry, (PCHAR)lArg));

    /*
     *  FIOSEEK, FIOWHERE is 64 bit operate.
     */
    case FIOSEEK:                                                       /*  文件重定位                  */
        oftTemp = *(off_t *)lArg;
        return  (__nfsSeek(pfdentry, oftTemp));

    case FIOWHERE:                                                      /*  获得文件当前读写指针        */
        iError = __nfsWhere(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }

    case FIONREAD:                                                      /*  获得文件剩余字节数          */
        return  (__nfsNRead(pfdentry, (INT *)lArg));
        
    case FIONREAD64:                                                    /*  获得文件剩余字节数          */
        iError = __nfsNRead64(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }

    case FIOLABELGET:                                                   /*  获取卷标                    */
    case FIOLABELSET:                                                   /*  设置卷标                    */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);

    case FIOFSTATGET:                                                   /*  获得文件状态                */
        return  (__nfsStat(pfdentry, (struct stat *)lArg));

    case FIOFSTATGET64:
        return  (__nfsStat64(pfdentry, (struct stat64 *)lArg));         /*  获得文件状态 64bit          */

    case FIOFSTATFSGET:                                                 /*  获得文件系统状态            */
        return  (__nfsStatfs(pfdentry, (struct statfs *)lArg));

    case FIOREADDIR:                                                    /*  获取一个目录信息            */
        return  (__nfsReadDir(pfdentry, (DIR *)lArg));

    case FIOTIMESET:                                                    /*  设置文件时间                */
        return  (__nfsTimeset(pfdentry, (struct utimbuf *)lArg));

    /*
     *  FIOTRUNC is 64 bit operate.
     */
    case FIOTRUNC:                                                      /*  改变文件大小                */
        oftSize = *(off_t *)lArg;
        return  (__nfsTruncate(pfdentry, oftSize));

    case FIOSYNC:                                                       /*  将文件缓存回写              */
    case FIODATASYNC:
    case FIOFLUSH:
        return  (__nfsFlush(pfdentry));

    case FIOCHMOD:
        return  (__nfsChmod(pfdentry, (INT)lArg));                      /*  改变文件访问权限            */

    case FIOSETFL:                                                      /*  设置新的 flag               */
        if ((INT)lArg & O_NONBLOCK) {
            pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);

    case FIOFSTYPE:                                                     /*  获得文件系统类型            */
        *(PCHAR *)lArg = "NFSv3 FileSystem";
        return  (ERROR_NONE);
        
    case FIOGETFORCEDEL:                                                /*  强制卸载设备是否被允许      */
        *(BOOL *)lArg = pnfsfile->NFSFIL_nfsfs->NFSFS_bForceDelete;
        return  (ERROR_NONE);
    
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_NFS_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
