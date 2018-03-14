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
** 文   件   名: procFs.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 03 日
**
** 描        述: proc 文件系统.

** BUG:
2009.12.11  在创建驱动程序时, 就应该加入 proc 锁的创建.
2009.12.29  增加注释.
2010.05.05  proc 文件系统的 f_type 为 PROC_SUPER_MAGIC.
2010.09.10  加入对 d_type 的支持.
2011.08.11  压缩目录不再由文件系统完成, 操作系统已处理了.
2012.03.10  __procFsReadDir() 当目录读取完毕后, errno = ENOENT.
2012.04.01  proc 文件系统加入试验性质的文件权限管理.
2012.08.16  加入了 pread 操作.
2012.08.26  加入了符号链接的功能.
2012.11.09  __procFsReadlink() 返回有效字节数.
            lstat() 链接文件时显示正确的长度.
2013.01.21  移除访问权限判断, 交由 IO 系统统一完成.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_type.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
#include "procFs.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0
/*********************************************************************************************************
  内部信息节点初始化
*********************************************************************************************************/
#if LW_CFG_PROCFS_KERNEL_INFO > 0
extern  VOID  __procFsKernelInfoInit(VOID);
#endif                                                                  /*  LW_CFG_PROCFS_KERNEL_INFO   */
#if LW_CFG_PROCFS_HOOK_INFO > 0
extern VOID   __procFsHookInit(VOID);
#endif                                                                  /*  LW_CFG_PROCFS_HOOK_INFO > 0 */
#if LW_CFG_PROCFS_BSP_INFO > 0
extern  VOID  __procFsBspInfoInit(VOID);
#endif                                                                  /*  LW_CFG_PROCFS_BSP_INFO      */
extern  VOID  __procFssupInit(VOID);
#if LW_CFG_POWERM_EN > 0
extern  VOID  __procFsPowerInit(VOID);
#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */
/*********************************************************************************************************
  procfs 文件控制块及时间表
*********************************************************************************************************/
static LW_DEV_HDR       _G_devhdrProc;                                  /*  procfs 设备                 */
static time_t           _G_timeProcFs;                                  /*  procfs 创建时间             */
/*********************************************************************************************************
  procfs 主设备号
*********************************************************************************************************/
static INT              _G_iProcDrvNum = PX_ERROR;
/*********************************************************************************************************
  内部函数
*********************************************************************************************************/
PLW_PROCFS_NODE  __procFsFindNode(CPCHAR            pcName, 
                                  PLW_PROCFS_NODE  *pp_pfsnFather, 
                                  BOOL             *pbRoot,
                                  BOOL             *pbLast,
                                  PCHAR            *ppcTail);
VOID             __procFsRemoveNode(PLW_PROCFS_NODE  p_pfsn);
/*********************************************************************************************************
  检测路径字串是否为根目录或者直接指向设备
*********************************************************************************************************/
#define __STR_IS_ROOT(pcName)       ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))
/*********************************************************************************************************
  驱动程序声明
*********************************************************************************************************/
static LONG     __procFsOpen(PLW_DEV_HDR  pdevhdr,
                          PCHAR           pcName,
                          INT             iFlags,
                          INT             iMode);
static INT      __procFsClose(PLW_PROCFS_NODE  p_pfsn);
static ssize_t  __procFsRead(PLW_PROCFS_NODE  p_pfsn, 
                             PCHAR            pcBuffer, 
                             size_t           stMaxBytes);
static ssize_t  __procFsPRead(PLW_PROCFS_NODE  p_pfsn, 
                              PCHAR            pcBuffer, 
                              size_t           stMaxBytes,
                              off_t            oftPos);
static ssize_t  __procFsWrite(PLW_PROCFS_NODE  p_pfsn, 
                              PCHAR            pcBuffer, 
                              size_t           stBytes);
static ssize_t  __procFsPWrite(PLW_PROCFS_NODE  p_pfsn, 
                               PCHAR            pcBuffer, 
                               size_t           stBytes,
                               off_t            oftPos);
static INT      __procFsLStatGet(PLW_DEV_HDR pdevhdr, 
                                 PCHAR  pcName, 
                                 struct stat *pstat);
static INT      __procFsIoctl(PLW_PROCFS_NODE  p_pfsn, 
                          INT              iRequest,
                          LONG             lArg);
static ssize_t  __procFsReadlink(PLW_DEV_HDR    pdevhdrHdr,
                                 PCHAR          pcName,
                                 PCHAR          pcLinkDst,
                                 size_t         stMaxSize);
/*********************************************************************************************************
** 函数名称: API_ProcFsDrvInstall
** 功能描述: 安装 procfs 文件系统驱动程序
** 输　入  : 
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_ProcFsDrvInstall (VOID)
{
    struct file_operations     fileop;

    if (_G_iProcDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    if (_G_ulProcFsLock == LW_OBJECT_HANDLE_INVALID) {
        _G_ulProcFsLock = API_SemaphoreMCreate("proc_lock", LW_PRIO_DEF_CEILING, LW_OPTION_WAIT_PRIORITY |
                                               LW_OPTION_INHERIT_PRIORITY | LW_OPTION_DELETE_SAFE | 
                                               LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));
    
    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __procFsOpen;
    fileop.fo_release  = LW_NULL;
    fileop.fo_open     = __procFsOpen;
    fileop.fo_close    = __procFsClose;
    fileop.fo_read     = __procFsRead;
    fileop.fo_read_ex  = __procFsPRead;
    fileop.fo_write    = __procFsWrite;
    fileop.fo_write_ex = __procFsPWrite;
    fileop.fo_lstat    = __procFsLStatGet;
    fileop.fo_ioctl    = __procFsIoctl;
    fileop.fo_readlink = __procFsReadlink;
    
    _G_iProcDrvNum = iosDrvInstallEx(&fileop);
     
    DRIVER_LICENSE(_G_iProcDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iProcDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iProcDrvNum, "procfs driver.");
    
    return  ((_G_iProcDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** 函数名称: API_ProcFsDevCreate
** 功能描述: 创建 proc fs.
** 输　入  : NONE
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_ProcFsDevCreate (VOID)
{
    static BOOL     bIsInit = LW_FALSE;
    
    if (bIsInit) {
        return  (ERROR_NONE);
    }
    
    if (iosDevAddEx(&_G_devhdrProc, "/proc", _G_iProcDrvNum, DT_DIR) != ERROR_NONE) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    bIsInit = LW_TRUE;
    
#if LW_CFG_PROCFS_BSP_INFO > 0
    __procFsBspInfoInit();
#endif                                                                  /*  LW_CFG_PROCFS_BSP_INFO      */

#if LW_CFG_PROCFS_KERNEL_INFO > 0
    __procFsKernelInfoInit();                                           /*  建立内核信息节点            */
#endif                                                                  /*  LW_CFG_PROCFS_KERNEL_INFO   */

#if LW_CFG_PROCFS_HOOK_INFO > 0
    __procFsHookInit();
#endif                                                                  /*  LW_CFG_PROCFS_HOOK_INFO > 0 */

    __procFssupInit();
    
#if LW_CFG_POWERM_EN > 0
    __procFsPowerInit();
#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */

    lib_time(&_G_timeProcFs);                                           /*  以 UTC 作为时间基准         */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __procFsOpen
** 功能描述: procfs 打开一个文件 (非创建)
** 输　入  : pdevhdr          procfs 设备
**           pcName           文件名
**           iFlags           方式
**           iMode            方法
** 输　出  : proc 节点 (LW_NULL 表示 proc 根节点)
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  __procFsOpen (PLW_DEV_HDR     pdevhdr,
                           PCHAR           pcName,
                           INT             iFlags,
                           INT             iMode)
{
    PLW_PROCFS_NODE  p_pfsn;
    BOOL             bIsRoot;
    PCHAR            pcTail = LW_NULL;
    
    if (pcName == LW_NULL) {                                            /*  无文件名                    */
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
    
    if ((iFlags & O_CREAT) || (iFlags & O_TRUNC)) {                     /*  不能创建                    */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    if ((iFlags & O_ACCMODE) != O_RDONLY) {                             /*  不允许写 (目前)             */
        _ErrorHandle(ERROR_IO_WRITE_PROTECTED);
        return  (PX_ERROR);
    }
    
    __LW_PROCFS_LOCK();                                                 /*  锁 procfs                   */
    p_pfsn = __procFsFindNode(pcName, LW_NULL, &bIsRoot, LW_NULL, &pcTail);
    if (p_pfsn == LW_NULL) {                                            /*  为找到节点                  */
        __LW_PROCFS_UNLOCK();                                           /*  解锁 procfs                 */
        if (bIsRoot) {
            LW_DEV_INC_USE_COUNT(&_G_devhdrProc);                       /*  更新计数器                  */
            return  ((LONG)LW_NULL);
        } else {
            _ErrorHandle(ENOENT);
            return  (PX_ERROR);
        }
    }
    if (p_pfsn->PFSN_bReqRemove) {
        __LW_PROCFS_UNLOCK();                                           /*  解锁 procfs                 */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    if (p_pfsn->PFSN_iOpenNum) {                                        /*  不支持重复打开,             */
                                                                        /*  文件指针只有一个            */
        __LW_PROCFS_UNLOCK();                                           /*  解锁 procfs                 */
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }
    p_pfsn->PFSN_pfsnmMessage.PFSNM_oftPtr = 0;                         /*  文件指针回原位              */
    p_pfsn->PFSN_iOpenNum++;
    __LW_PROCFS_UNLOCK();                                               /*  解锁 procfs                 */
    
    if (p_pfsn && S_ISLNK(p_pfsn->PFSN_mode)) {                         /*  链接文件处理                */
        INT     iError;
        INT     iFollowLinkType;
        PCHAR   pcSymfile = pcTail - lib_strlen(p_pfsn->PFSN_pcName) - 1;
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
                                LW_NULL, pcPrefix, 
                                (PCHAR)p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer, 
                                pcTail);
        
        __LW_PROCFS_LOCK();                                             /*  锁 procfs                   */
        p_pfsn->PFSN_iOpenNum--;
        __LW_PROCFS_UNLOCK();                                           /*  解锁 procfs                 */
        
        if (iError) {
            return  (PX_ERROR);                                         /*  无法建立被链接目标目录      */
        } else {
            return  (iFollowLinkType);
        }
    }
    
    p_pfsn->PFSN_time = lib_time(LW_NULL);                              /*  以 UTC 时间作为时间基准     */
    
    LW_DEV_INC_USE_COUNT(&_G_devhdrProc);                               /*  更新计数器                  */

    return  ((LONG)p_pfsn);
}
/*********************************************************************************************************
** 函数名称: __procFsClose
** 功能描述: procfs 关闭一个文件
** 输　入  : p_pfsn        节点控制块
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __procFsClose (PLW_PROCFS_NODE  p_pfsn)
{
    if (p_pfsn) {
        API_ProcFsFreeNodeBuffer(p_pfsn);                               /*  释放缓存                    */
        __LW_PROCFS_LOCK();                                             /*  锁 procfs                   */
        p_pfsn->PFSN_iOpenNum--;
        if (p_pfsn->PFSN_iOpenNum == 0) {
            if (p_pfsn->PFSN_bReqRemove) {                              /*  请求删除节点                */
                __procFsRemoveNode(p_pfsn);                             /*  删除节点                    */
            }
        }
        __LW_PROCFS_UNLOCK();                                           /*  解锁 procfs                 */
    }
    LW_DEV_DEC_USE_COUNT(&_G_devhdrProc);                               /*  更新计数器                  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __procFsRead
** 功能描述: procfs 读取一个文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
** 输　出  : 实际读取的个数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsRead (PLW_PROCFS_NODE  p_pfsn, 
                              PCHAR            pcBuffer, 
                              size_t           stMaxBytes)
{
             ssize_t    sstNum = 0;
    REGISTER size_t     stBufferSize;                                   /*  文件缓冲区大小              */

    if (!pcBuffer || !stMaxBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    /*
     *  这里不需要加锁, 因为文件不关闭, 节点不可能释放.
     */
    if (!p_pfsn || !S_ISREG(p_pfsn->PFSN_mode)) {                       /*  只能读取 reg 文件           */
        _ErrorHandle(EBADF);                                            /*  文件不能读                  */
        return  (PX_ERROR);
    }
    
    if (p_pfsn->PFSN_p_pfsnoFuncs &&
        p_pfsn->PFSN_p_pfsnoFuncs->PFSNO_pfuncRead) {
        stBufferSize = p_pfsn->PFSN_pfsnmMessage.PFSNM_stBufferSize;    /*  获得当前缓冲大小            */
        
        /*
         *  当 PFSNM_ulNeedSize 为 0 时表示, 文件预期大小不确定, 
         *  需要文件驱动程序自己根据情况分配文件缓冲内存.
         */
        if (stBufferSize < p_pfsn->PFSN_pfsnmMessage.PFSNM_stNeedSize) {/*  缓冲太小                    */
            if (API_ProcFsAllocNodeBuffer(p_pfsn, 
                    p_pfsn->PFSN_pfsnmMessage.PFSNM_stNeedSize)) {
                                                                        /*  创建文件缓冲                */
                _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
                return  (0);
            }
            p_pfsn->PFSN_pfsnmMessage.PFSNM_stRealSize = 0;             /*  没有足够的缓冲证明文件还没有*/
                                                                        /*  开始生成                    */
        }
        
        sstNum = p_pfsn->PFSN_p_pfsnoFuncs->PFSNO_pfuncRead(p_pfsn,
                                                pcBuffer,
                                                stMaxBytes,
                                                p_pfsn->PFSN_pfsnmMessage.PFSNM_oftPtr);
        if (sstNum > 0) {
            p_pfsn->PFSN_pfsnmMessage.PFSNM_oftPtr += (off_t)sstNum;    /*  更新文件指针                */
        }
    } else {
        _ErrorHandle(ENOSYS);
    }

    return  (sstNum);
}
/*********************************************************************************************************
** 函数名称: __procFsPRead
** 功能描述: procfs 读取一个文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oftPos        位置
** 输　出  : 实际读取的个数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsPRead (PLW_PROCFS_NODE  p_pfsn, 
                               PCHAR            pcBuffer, 
                               size_t           stMaxBytes,
                               off_t            oftPos)
{
             ssize_t    sstNum = 0;
    REGISTER size_t     stBufferSize;                                   /*  文件缓冲区大小              */

    if (!pcBuffer || !stMaxBytes || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    /*
     *  这里不需要加锁, 因为文件不关闭, 节点不可能释放.
     */
    if (!p_pfsn || !S_ISREG(p_pfsn->PFSN_mode)) {                       /*  只能读取 reg 文件           */
        _ErrorHandle(EISDIR);                                           /*  目录不能读                  */
        return  (PX_ERROR);
    }
    
    if (p_pfsn->PFSN_p_pfsnoFuncs &&
        p_pfsn->PFSN_p_pfsnoFuncs->PFSNO_pfuncRead) {
        stBufferSize = p_pfsn->PFSN_pfsnmMessage.PFSNM_stBufferSize;    /*  获得当前缓冲大小            */

        /*
         *  当 PFSNM_ulNeedSize 为 0 时表示, 文件预期大小不确定, 
         *  需要文件驱动程序自己根据情况分配文件缓冲内存.
         */
        if (stBufferSize < p_pfsn->PFSN_pfsnmMessage.PFSNM_stNeedSize) {/*  缓冲太小                    */
            if (API_ProcFsAllocNodeBuffer(p_pfsn, 
                    p_pfsn->PFSN_pfsnmMessage.PFSNM_stNeedSize)) {
                                                                        /*  创建文件缓冲                */
                _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
                return  (0);
            }
            p_pfsn->PFSN_pfsnmMessage.PFSNM_stRealSize = 0;             /*  没有足够的缓冲证明文件还没有*/
                                                                        /*  开始生成                    */
        }
        
        sstNum = p_pfsn->PFSN_p_pfsnoFuncs->PFSNO_pfuncRead(p_pfsn,
                                                pcBuffer,
                                                stMaxBytes,
                                                oftPos);
    } else {
        _ErrorHandle(ENOSYS);
    }

    return  (sstNum);
}
/*********************************************************************************************************
** 函数名称: __procFsWrite
** 功能描述: procfs 写入一个文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stBytes       需要写入的字节数
** 输　出  : 实际写入的个数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsWrite (PLW_PROCFS_NODE  p_pfsn, 
                               PCHAR            pcBuffer, 
                               size_t           stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (0);
}
/*********************************************************************************************************
** 函数名称: __procFsPWrite
** 功能描述: procfs 写入一个文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stBytes       需要写入的字节数
**           oftPos        位置
** 输　出  : 实际写入的个数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsPWrite (PLW_PROCFS_NODE  p_pfsn, 
                                PCHAR            pcBuffer, 
                                size_t           stBytes,
                                off_t            oftPos)
{
    _ErrorHandle(ENOSYS);
    return  (0);
}
/*********************************************************************************************************
** 函数名称: __procFsStatGet
** 功能描述: procfs 获取一个文件属性
** 输　入  : p_pfsn        节点控制块
**           pstat         回写的文件属性
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __procFsStatGet (PLW_PROCFS_NODE  p_pfsn, struct stat *pstat)
{
    if (p_pfsn == LW_NULL) {                                            /*  procfs 根节点               */
        pstat->st_dev     = (dev_t)&_G_devhdrProc;
        pstat->st_ino     = (ino_t)0;                                   /*  根目录                      */
        pstat->st_mode    = (0644 | S_IFDIR);                           /*  默认属性                    */
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;                                          /*  owner is root               */
        pstat->st_gid     = 0;
        pstat->st_rdev    = 1;
        pstat->st_size    = 0;
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        pstat->st_atime   = _G_timeProcFs;
        pstat->st_mtime   = _G_timeProcFs;
        pstat->st_ctime   = _G_timeProcFs;
    } else {                                                            /*  子节点                      */
        pstat->st_dev     = (dev_t)&_G_devhdrProc;
        pstat->st_ino     = (ino_t)p_pfsn;
        pstat->st_mode    = p_pfsn->PFSN_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = p_pfsn->PFSN_uid;
        pstat->st_gid     = p_pfsn->PFSN_gid;
        pstat->st_rdev    = 1;
        pstat->st_size    = 0;
        pstat->st_blksize = p_pfsn->PFSN_pfsnmMessage.PFSNM_stBufferSize;
        pstat->st_blocks  = 1;
        pstat->st_atime   = p_pfsn->PFSN_time;
        pstat->st_mtime   = p_pfsn->PFSN_time;
        pstat->st_ctime   = p_pfsn->PFSN_time;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __procFsLStatGet
** 功能描述: procfs 获取一个文件属性
** 输　入  : pdevhdr       procfs 设备
**           pcName        文件名
**           pstat         回写的文件属性
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __procFsLStatGet (PLW_DEV_HDR pdevhdr, PCHAR  pcName, struct stat *pstat)
{
    PLW_PROCFS_NODE  p_pfsn;
    BOOL             bIsRoot;
    PCHAR            pcTail = LW_NULL;
    
    if (pcName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    __LW_PROCFS_LOCK();                                                 /*  锁 procfs                   */
    p_pfsn = __procFsFindNode(pcName, LW_NULL, &bIsRoot, LW_NULL, &pcTail);
    if (p_pfsn == LW_NULL) {                                            /*  为找到节点                  */
        __LW_PROCFS_UNLOCK();                                           /*  解锁 procfs                 */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    if (p_pfsn->PFSN_bReqRemove) {
        __LW_PROCFS_UNLOCK();                                           /*  解锁 procfs                 */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    if (p_pfsn) {
        p_pfsn->PFSN_time = lib_time(LW_NULL);                          /*  当前 UTC 时间               */
    
        pstat->st_dev     = (dev_t)&_G_devhdrProc;
        pstat->st_ino     = (ino_t)p_pfsn;
        pstat->st_mode    = p_pfsn->PFSN_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = p_pfsn->PFSN_uid;
        pstat->st_gid     = p_pfsn->PFSN_gid;
        pstat->st_rdev    = 1;
        
        if (S_ISLNK(p_pfsn->PFSN_mode)) {
            pstat->st_size = lib_strlen((PCHAR)p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer);
        } else {
            pstat->st_size = 0;
        }
        
        pstat->st_blksize = p_pfsn->PFSN_pfsnmMessage.PFSNM_stBufferSize;
        pstat->st_blocks  = 1;
        pstat->st_atime   = p_pfsn->PFSN_time;
        pstat->st_mtime   = p_pfsn->PFSN_time;
        pstat->st_ctime   = p_pfsn->PFSN_time;
        
        __LW_PROCFS_UNLOCK();                                           /*  解锁 procfs                 */
        
        return  (ERROR_NONE);
    
    } else {
        __LW_PROCFS_UNLOCK();                                           /*  解锁 procfs                 */
        
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __procFsStatfsGet
** 功能描述: procfs 获取文件系统属性
** 输　入  : p_pfsn        节点控制块
**           pstatfs       回写的文件系统属性
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __procFsStatfsGet (PLW_PROCFS_NODE  p_pfsn, struct statfs *pstatfs)
{
    if (pstatfs) {
        pstatfs->f_type   = PROC_SUPER_MAGIC;
        pstatfs->f_bsize  = 0;
        pstatfs->f_blocks = 1;
        pstatfs->f_bfree  = 0;
        pstatfs->f_bavail = 1;
        
        pstatfs->f_files  = (long)_G_pfsrRoot.PFSR_ulFiles;
        pstatfs->f_ffree  = 0;
        
#if LW_CFG_CPU_WORD_LENGHT == 64
        pstatfs->f_fsid.val[0] = (int32_t)((addr_t)&_G_pfsrRoot >> 32);
        pstatfs->f_fsid.val[1] = (int32_t)((addr_t)&_G_pfsrRoot & 0xffffffff);
#else
        pstatfs->f_fsid.val[0] = (int32_t)&_G_pfsrRoot;
        pstatfs->f_fsid.val[1] = 0;
#endif
        
        pstatfs->f_flag    = ST_NOSUID;
        pstatfs->f_namelen = PATH_MAX;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __procFsReadDir
** 功能描述: procfs 读取目录项
** 输　入  : p_pfsn        节点控制块
**           pstatfs       回写的文件系统属性
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __procFsReadDir (PLW_PROCFS_NODE  p_pfsn, DIR  *dir)
{
             INT                i;
             INT                iError = ERROR_NONE;
             
    REGISTER LONG               iStart = dir->dir_pos;
             PLW_PROCFS_NODE    p_pfsnTemp;
    
             PLW_LIST_LINE      plineTemp;
             PLW_LIST_LINE      plineHeader;                            /*  当前目录头                  */
    
    __LW_PROCFS_LOCK();                                                 /*  锁 procfs                   */
    if (p_pfsn == LW_NULL) {                                            /*  procfs 根节点               */
        plineHeader = _G_pfsrRoot.PFSR_plineSon;                        /*  从根目录开始搜索            */
    } else {
        if (!S_ISDIR(p_pfsn->PFSN_mode)) {
            __LW_PROCFS_UNLOCK();                                       /*  解锁 procfs                 */
            _ErrorHandle(ENOTDIR);                                      /*  不是一个目录                */
            return  (PX_ERROR);
        }
        plineHeader = p_pfsn->PFSN_plineSon;                            /*  从第一个儿子开始寻找        */
    }
        
    for ((plineTemp  = plineHeader), (i = 0); 
         (plineTemp != LW_NULL) && (i < iStart); 
         (plineTemp  = _list_line_get_next(plineTemp)), (i++));         /*  忽略                        */
    
    if (plineTemp == LW_NULL) {
        _ErrorHandle(ENOENT);
        iError = PX_ERROR;                                              /*  没有多余的节点              */
    
    } else {
        p_pfsnTemp = _LIST_ENTRY(plineTemp, LW_PROCFS_NODE, PFSN_lineBrother);
        dir->dir_pos++;
        lib_strlcpy(dir->dir_dirent.d_name, 
                    p_pfsnTemp->PFSN_pcName, 
                    sizeof(dir->dir_dirent.d_name));                    /*  拷贝文件名                  */
        dir->dir_dirent.d_shortname[0] = PX_EOS;
        dir->dir_dirent.d_type = IFTODT(p_pfsnTemp->PFSN_mode);         /*  转换为 d_type               */
    }
    __LW_PROCFS_UNLOCK();                                               /*  解锁 procfs                 */

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __procFsIoctl
** 功能描述: procfs ioctl 操作
** 输　入  : p_pfsn             节点控制块
**           request,           命令
**           arg                命令参数
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __procFsIoctl (PLW_PROCFS_NODE  p_pfsn, 
                           INT              iRequest,
                           LONG             lArg)
{
    switch (iRequest) {
    
    /*
     *  FIOSEEK, FIOWHERE is 64 bit operate.
     */
    case FIOSEEK:                                                       /*  文件重定位                  */
        if (p_pfsn && lArg) {
            __LW_PROCFS_LOCK();                                         /*  锁 procfs                   */
            p_pfsn->PFSN_pfsnmMessage.PFSNM_oftPtr = *(off_t *)lArg;
            __LW_PROCFS_UNLOCK();                                       /*  解锁 procfs                 */
        }
        return  (ERROR_NONE);
    
    case FIOWHERE:                                                      /*  获得文件当前读写指针        */
        if (lArg) {
            *(off_t *)lArg = p_pfsn->PFSN_pfsnmMessage.PFSNM_oftPtr;
        }
        return  (ERROR_NONE);
    
    case FIOFSTATGET:                                                   /*  获得文件状态                */
        return  (__procFsStatGet(p_pfsn, (struct stat *)lArg));
        
    case FIOFSTATFSGET:                                                 /*  获得文件系统状态            */
        return  (__procFsStatfsGet(p_pfsn, (struct statfs *)lArg));
    
    case FIOREADDIR:                                                    /*  获取一个目录信息            */
        return  (__procFsReadDir(p_pfsn, (DIR *)lArg));
    
    case FIOSYNC:                                                       /*  将文件缓存回写              */
    case FIODATASYNC:
    case FIOFLUSH:
        return  (ERROR_NONE);
        
    case FIOFSTYPE:                                                     /*  获得文件系统类型            */
        *(PCHAR *)lArg = "PROC FileSystem";
        return  (ERROR_NONE);
        
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __procFsReadlink
** 功能描述: procfs read link 操作
** 输　入  : pdevhdr                       procfs 设备
**           pcName                        链接原始文件名
**           pcLinkDst                     链接目标文件名
**           stMaxSize                     缓冲大小
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsReadlink (PLW_DEV_HDR    pdevhdr,
                                  PCHAR          pcName,
                                  PCHAR          pcLinkDst,
                                  size_t         stMaxSize)
{
    PLW_PROCFS_NODE  p_pfsn;
    size_t           stLen;
    
    if (pcName == LW_NULL) {                                            /*  无文件名                    */
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
    
    __LW_PROCFS_LOCK();                                                 /*  锁 procfs                   */
    p_pfsn = __procFsFindNode(pcName, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    if (p_pfsn == LW_NULL) {                                            /*  为找到节点                  */
        __LW_PROCFS_UNLOCK();                                           /*  解锁 procfs                 */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    if (p_pfsn->PFSN_bReqRemove) {
        __LW_PROCFS_UNLOCK();                                           /*  解锁 procfs                 */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    if (!S_ISLNK(p_pfsn->PFSN_mode)) {
        __LW_PROCFS_UNLOCK();                                           /*  解锁 procfs                 */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    stLen = lib_strlen((PCHAR)p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer);
    lib_strncpy(pcLinkDst, (PCHAR)p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer, stMaxSize);
    if (stLen > stMaxSize) {                                            /*  拷贝链接内容                */
        stLen = stMaxSize;                                              /*  计算有效字节数              */
    }
    
    __LW_PROCFS_UNLOCK();                                               /*  解锁 procfs                 */
    
    return  ((ssize_t)stLen);
}

#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
