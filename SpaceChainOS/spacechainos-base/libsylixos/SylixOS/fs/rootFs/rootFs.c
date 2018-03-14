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
** 文   件   名: rootFs.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 08 月 26 日
**
** 描        述: 根目录文件系统.

** BUG:
2009.09.15  修正时区错误.
2009.10.22  read write 返回值为 ssize_t.
2009.12.14  加入分级根目录管理机制.
2009.12.15  修正 remove 删除链接文件的错误.
2010.09.10  支持 d_type.
2011.05.16  修正符号连接创建目录错误.
2011.06.11  加入 __rootFsTimeSet() 允许第一次同步 rtc 时间时, 设置 root fs 时间.
2011.07.11  创建根文件系统时, 如果时间没有设置过, 将会记录当前创建时间.
2011.08.07  加入对 lstat() 的支持.
2011.08.11  压缩目录不再由文件系统完成, 操作系统已处理了.
2011.12.13  使用 LW_CFG_SPIPE_DEFAULT_SIZE 作为创建的 fifo 大小.
2012.03.10  __rootFsReadDir 当目录读取完毕后, 返回 errno == ENOENT.
2012.06.29  __rootFsLStatGet 为链接文件时, 大小不正确.
2012.09.25  加入对 AF_UNIX 类型 sock 文件的支持. mknod
2012.12.27  修正一些命名.
2013.01.22  根文件系统加入 chmod 支持.
2013.03.16  加入对 reg 文件的支持.
2013.03.30  修正根文件系统在符号连接内创建节点的错误.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
#include "rootFsLib.h"
#include "rootFs.h"
/*********************************************************************************************************
  rootfs 文件控制块及时间表 (这里默认所有的设备使用 root fs 创建时的时间作为自己的时间基准)
*********************************************************************************************************/
#if LW_CFG_PATH_VXWORKS == 0
       LW_DEV_HDR       _G_devhdrRoot;                                  /*  rootfs 设备                 */
#else
static LW_DEV_HDR       _G_devhdrRoot;                                  /*  rootfs 设备                 */
#endif                                                                  /*  LW_CFG_PATH_VXWORKS         */
static time_t           _G_timeRootFs = 0;
/*********************************************************************************************************
  rootfs 主设备号
*********************************************************************************************************/
static INT              _G_iRootDrvNum = PX_ERROR;
/*********************************************************************************************************
  驱动程序声明
*********************************************************************************************************/
static LONG     __rootFsOpen(PLW_DEV_HDR     pdevhdr,
                             PCHAR           pcName,
                             INT             iFlags,
                             INT             iMode);
static INT      __rootFsRemove(PLW_DEV_HDR     pdevhdr,
                               PCHAR           pcName);
static INT      __rootFsClose(LW_DEV_HDR    *pdevhdr);
static ssize_t  __rootFsRead(LW_DEV_HDR     *pdevhdr,
                             PCHAR           pcBuffer, 
                             size_t          stMaxBytes);
static ssize_t  __rootFsWrite(LW_DEV_HDR *pdevhdr,
                              PCHAR       pcBuffer, 
                              size_t      stNBytes);
static INT      __rootFsLStatGet(LW_DEV_HDR *pdevhdr, 
                                 PCHAR       pcName, 
                                 struct stat *pstat);
static INT      __rootFsIoctl(LW_DEV_HDR *pdevhdr,
                              INT         iRequest,
                              LONG        lArg);
static INT      __rootFsSymlink(PLW_DEV_HDR     pdevhdr,
                                PCHAR           pcName,
                                CPCHAR          pcLinkDst);
static ssize_t  __rootFsReadlink(PLW_DEV_HDR    pdevhdr,
                                 PCHAR          pcName,
                                 PCHAR          pcLinkDst,
                                 size_t         stMaxSize);
/*********************************************************************************************************
** 函数名称: API_RootFsDrvInstall
** 功能描述: 安装 rootfs 文件系统驱动程序
** 输　入  : 
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_RootFsDrvInstall (VOID)
{
    struct file_operations     fileop;

    if (_G_iRootDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));
    
    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __rootFsOpen;
    fileop.fo_release  = __rootFsRemove;
    fileop.fo_open     = __rootFsOpen;
    fileop.fo_close    = __rootFsClose;
    fileop.fo_read     = __rootFsRead;
    fileop.fo_write    = __rootFsWrite;
    fileop.fo_lstat    = __rootFsLStatGet;
    fileop.fo_ioctl    = __rootFsIoctl;

    /*
     *  仅当系统使用新的 SylixOS 分级目录管理时, 才提供链接文件支持
     */
    fileop.fo_symlink  = __rootFsSymlink;
    fileop.fo_readlink = __rootFsReadlink;
    
    _G_iRootDrvNum = iosDrvInstallEx(&fileop);
     
    DRIVER_LICENSE(_G_iRootDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iRootDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iRootDrvNum, "rootfs driver.");
    
    return  ((_G_iRootDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** 函数名称: API_RootFsDevCreate
** 功能描述: 创建 root fs.
** 输　入  : NONE
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_RootFsDevCreate (VOID)
{
    static BOOL     bIsInit = LW_FALSE;
    
    if (bIsInit) {
        return  (ERROR_NONE);
    }
    
    if (iosDevAddEx(&_G_devhdrRoot, PX_STR_ROOT, _G_iRootDrvNum, DT_DIR) != ERROR_NONE) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    bIsInit = LW_TRUE;
    
    if (_G_timeRootFs == 0) {                                           /*  没有设置过                  */
        lib_time(&_G_timeRootFs);                                       /*  以 UTC 时间作为时间基准     */
    }
    
#if LW_CFG_PATH_VXWORKS == 0
    _G_rfsrRoot.RFSR_time      = _G_timeRootFs;
    _G_rfsrRoot.RFSR_stMemUsed = 0;
    _G_rfsrRoot.RFSR_ulFiles   = 0;
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_RootFsTime
** 功能描述: 获得 root fs 时间基准
** 输　入  : time      时间指针
** 输　出  : 时间
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
time_t  API_RootFsTime (time_t  *time)
{
    if (time) {
        *time = _G_timeRootFs;
    }
    
    return  (_G_timeRootFs);
}
/*********************************************************************************************************
** 函数名称: __rootFsTimeSet
** 功能描述: 设置 root fs 时间基准 (只允许在系统启动时调用)
** 输　入  : time      时间指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
VOID  __rootFsTimeSet (time_t  *time)
{
    if (time) {
        _G_timeRootFs = *time;
    }
}
/*********************************************************************************************************
** 函数名称: __rootFsFixName
** 功能描述: root fs 修正文件名
** 输　入  : pcName           文件名
** 输　出  : 修正后的长度
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_PATH_VXWORKS == 0                                            /*  需要提供分级目录管理        */

INT  __rootFsFixName (PCHAR  pcName)
{
    size_t  stLen;
    
    /*
     *  由于 iosDevFind() 函数对根设备特殊的过滤, 这里需要修正根目录符号
     */
    stLen = lib_strnlen(pcName, (PATH_MAX - 1));
    if (stLen == 0) {                                                   /*  match 设备时滤掉了 /        */
        pcName[0] = PX_ROOT;
        pcName[1] = PX_EOS;
        stLen     = 1;
    
    } else if (pcName[0] != PX_ROOT) {
        /*
         *  SylixOS lib_memcpy() 带有缓冲区防止卷绕的能力.
         */
        lib_memcpy(&pcName[1], &pcName[0], stLen + 1);                  /*  向后推移一个字节            */
        pcName[0]  = PX_ROOT;                                           /*  加入根符号                  */
        stLen     += 1;
    }
    
    return  (stLen);
}

#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
/*********************************************************************************************************
** 函数名称: __rootFsOpen
** 功能描述: root fs open 操作
** 输　入  : pdevhdr          root fs 设备
**           pcName           文件名
**           iFlags           方式
**           iMode            方法
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  __rootFsOpen (PLW_DEV_HDR     pdevhdr,
                           PCHAR           pcName,
                           INT             iFlags,
                           INT             iMode)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  需要提供分级目录管理        */
    /*
     *  文件扩展数据为节点指针
     */
    PLW_ROOTFS_NODE    prfsnFather;
    PLW_ROOTFS_NODE    prfsn;
    BOOL               bIsRoot;
    BOOL               bIsLast;
    PCHAR              pcTail = LW_NULL;
    INT                iError;
    

    if (pcName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    __rootFsFixName(pcName);                                            /*  修正文件名                  */
    
__re_find:
    __LW_ROOTFS_LOCK();                                                 /*  锁定 rootfs                 */
    prfsn = __rootFsFindNode(pcName, &prfsnFather, &bIsRoot, &bIsLast, &pcTail);
                                                                        /*  查询设备                    */
    if (prfsn) {
        prfsn->RFSN_iOpenNum++;
        if (prfsn->RFSN_iNodeType != LW_ROOTFS_NODE_TYPE_LNK) {         /*  不是链接文件                */
            if ((iFlags & O_CREAT) && (iFlags & O_EXCL)) {              /*  排他创建文件                */
                prfsn->RFSN_iOpenNum--;
                __LW_ROOTFS_UNLOCK();                                   /*  解锁 rootfs                 */
                _ErrorHandle(EEXIST);                                   /*  已经存在文件                */
                return  (PX_ERROR);
            
            } else {
                __LW_ROOTFS_UNLOCK();                                   /*  解锁 rootfs                 */
                LW_DEV_INC_USE_COUNT(&_G_devhdrRoot);                   /*  更新计数器                  */
                return  ((LONG)prfsn);
            }
        }
    
    } else if ((iFlags & O_CREAT) && bIsLast) {                         /*  需要创建节点                */
        __LW_ROOTFS_UNLOCK();                                           /*  解锁 rootfs                 */
        
#if LW_CFG_MAX_VOLUMES > 0
        if (__fsCheckFileName(pcName)) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
        
        if (S_ISDIR(iMode)) {
            iError = rootFsMakeDir(pcName, iMode);
        
        } else if (S_ISSOCK(iMode)) {
            iError = rootFsMakeSock(pcName, iMode);
        
        } else if (S_ISREG(iMode) || !(iMode & S_IFMT)) {
            iError = rootFsMakeReg(pcName, iMode);
        
#if LW_CFG_SPIPE_EN > 0
        } else if (S_ISFIFO(iMode)) {
            spipeDrv();
            iError = spipeDevCreate(pcName, LW_CFG_SPIPE_DEFAULT_SIZE);
#endif                                                                  /*  LW_CFG_SPIPE_EN > 0         */
        } else {
            _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                     /*  不支持创建这种类型的文件    */
            return  (PX_ERROR);
        }
        
        if ((iError < 0) && (iFlags & O_EXCL)) {                        /*  创建失败, 已经写入 errno    */
            return  (PX_ERROR);
        }
        
        iFlags &= ~O_CREAT;                                             /*  避免 O_CREAT O_EXCL 判断错误*/
        
        goto    __re_find;
    }
    __LW_ROOTFS_UNLOCK();                                               /*  解锁 rootfs                 */
    
    if (prfsn) {                                                        /*  链接文件处理                */
        INT     iError;
        INT     iFollowLinkType;
        PCHAR   pcSymfile = pcTail - lib_strlen(prfsn->RFSN_rfsnv.RFSNV_pcName) - 1;
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
                                LW_NULL, pcPrefix, prfsn->RFSN_pcLink, pcTail);
        
        __LW_ROOTFS_LOCK();                                             /*  锁定 rootfs                 */
        prfsn->RFSN_iOpenNum--;
        __LW_ROOTFS_UNLOCK();                                           /*  解锁 rootfs                 */
        
        if (iError) {
            return  (PX_ERROR);                                         /*  无法建立被链接目标目录      */
        } else {
            return  (iFollowLinkType);
        }
    }
    
    if (bIsRoot) {
        LW_DEV_INC_USE_COUNT(&_G_devhdrRoot);                           /*  更新计数器                  */
        return  ((LONG)LW_NULL);                                        /*  根目录                      */
    
    } else {
        _ErrorHandle(ENOENT);                                           /*  没有找到文件                */
        return  (PX_ERROR);
    }
    
#else                                                                   /*  VxWorks 兼容目录管理        */
    /*
     *  文件扩展数据为根设备指针
     */
    if (lib_strcmp(pcName, PX_STR_ROOT) && (pcName[0] != PX_EOS)) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    if ((iFlags & O_CREAT) || (iFlags & O_TRUNC)) {                     /*  不能创建                    */
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
    
    if ((iFlags & O_ACCMODE) != O_RDONLY) {                             /*  不允许写                    */
        _ErrorHandle(ERROR_IO_WRITE_PROTECTED);
        return  (PX_ERROR);
    }
    
    LW_DEV_INC_USE_COUNT(&_G_devhdrRoot);                               /*  更新计数器                  */
        
    return  ((LONG)&_G_devhdrRoot);
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
}
/*********************************************************************************************************
** 函数名称: __rootFsRemove
** 功能描述: root fs remove 操作
** 输　入  : pdevhdr
**           pcName           文件名
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __rootFsRemove (PLW_DEV_HDR     pdevhdr,
                            PCHAR           pcName)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  需要提供分级目录管理        */
    /*
     *  驱动中的 delete 函数一定要判断 cFullFileName 参数, 如果为链接文件则需要做两个判断
     *  cFullFileName 指向被链接对象的内部文件或目录时, 应该返回 FOLLOW_LINK_TAIL 用来删除
     *  被链接目标(真实目标)中的文件或目录, 当 cFullFileName 指向的是连接文件本身, 那么
     *  应该直接删除链接文件即可, 不应该返回 FOLLOW_LINK_TAIL 删除链接目标! 切记! 切记!
     */
    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
        
    } else {
        PLW_ROOTFS_NODE    prfsn;
        BOOL               bIsRoot;
        PCHAR              pcTail = LW_NULL;
        
        __rootFsFixName(pcName);                                        /*  修正文件名                  */
        
        __LW_ROOTFS_LOCK();                                             /*  锁定 rootfs                 */
        prfsn = __rootFsFindNode(pcName, LW_NULL, &bIsRoot, LW_NULL, &pcTail);
                                                                        /*  查询设备                    */
        if (prfsn) {
            if (prfsn->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_DEV) {     /*  不能在这里卸载设备          */
                __LW_ROOTFS_UNLOCK();                                   /*  解锁 rootfs                 */
                _ErrorHandle(EBUSY);
                return  (PX_ERROR);
            
            } else if (prfsn->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_LNK) {
                size_t  stLenTail = 0;
                
                if (pcTail) {
                    stLenTail = lib_strlen(pcTail);                     /*  确定 tail 长度              */
                }
                /*
                 *  没有尾部, 表明需要删除链接文件本身, 即仅删除链接文件即可.
                 *  那么直接使用 API_RootFsRemoveNode 删除即可.
                 */
                if (stLenTail) {
                    PCHAR   pcSymfile = pcTail - lib_strlen(prfsn->RFSN_rfsnv.RFSNV_pcName) - 1;
                    PCHAR   pcPrefix;
                    
                    if (*pcSymfile != PX_DIVIDER) {
                        pcSymfile--;
                    }
                    if (pcSymfile == pcName) {
                        pcPrefix = LW_NULL;                             /*  没有前缀                    */
                    } else {
                        pcPrefix = pcName;
                        *pcSymfile = PX_EOS;
                    }
                    
                    if (_PathBuildLink(pcName, MAX_FILENAME_LENGTH, 
                                       LW_NULL, pcPrefix, prfsn->RFSN_pcLink, pcTail) < ERROR_NONE) {
                        __LW_ROOTFS_UNLOCK();                           /*  解锁 rootfs                 */
                        return  (PX_ERROR);                             /*  无法建立被链接目标目录      */
                    
                    } else {
                        __LW_ROOTFS_UNLOCK();                           /*  解锁 rootfs                 */
                        return  (FOLLOW_LINK_TAIL);                     /*  连接文件内部文件            */
                    }
                }
            }
        }
        __LW_ROOTFS_UNLOCK();                                           /*  解锁 rootfs                 */
        
        return  (API_RootFsRemoveNode(pcName));
    }
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __rootFsClose
** 功能描述: root fs close 操作
** 输　入  : pdevhdr
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __rootFsClose (LW_DEV_HDR     *pdevhdr)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  需要提供分级目录管理        */
    /*
     *  参数为节点指针
     */
    PLW_ROOTFS_NODE    prfsn = (PLW_ROOTFS_NODE)pdevhdr;
    
    __LW_ROOTFS_LOCK();                                                 /*  锁定 rootfs                 */
    if (prfsn) {
        prfsn->RFSN_iOpenNum--;
    }
    __LW_ROOTFS_UNLOCK();                                               /*  解锁 rootfs                 */
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */

    LW_DEV_DEC_USE_COUNT(&_G_devhdrRoot);                               /*  更新计数器                  */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __rootFsRead
** 功能描述: root fs read 操作
** 输　入  : pdevhdr
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __rootFsRead (LW_DEV_HDR *pdevhdr,
                              PCHAR       pcBuffer, 
                              size_t      stMaxBytes)
{
    _ErrorHandle(ENOSYS);                                               /*  此文件不支持                */
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __rootFsWrite
** 功能描述: root fs write 操作
** 输　入  : pdevhdr
**           pcBuffer         缓冲区
**           stNBytes         需要写入的数据
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __rootFsWrite (LW_DEV_HDR *pdevhdr,
                               PCHAR       pcBuffer, 
                               size_t      stNBytes)
{
    _ErrorHandle(ENOSYS);                                               /*  此文件不支持                */
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __rootFsStatGet
** 功能描述: root fs 获得文件状态及属性
** 输　入  : pdevhdr
**           pstat               stat 结构
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __rootFsStatGet (LW_DEV_HDR *pdevhdr, struct stat *pstat)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  需要提供分级目录管理        */
    /*
     *  参数为节点指针
     */
    PLW_ROOTFS_NODE    prfsn = (PLW_ROOTFS_NODE)pdevhdr;
    
    if (pstat == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (prfsn) {
        pstat->st_dev     = (dev_t)&_G_devhdrRoot;
        pstat->st_ino     = (ino_t)prfsn;
        pstat->st_mode    = prfsn->RFSN_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = prfsn->RFSN_uid;
        pstat->st_gid     = prfsn->RFSN_gid;
        pstat->st_rdev    = 1;
        if (prfsn->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_LNK) {
            pstat->st_size = lib_strlen(prfsn->RFSN_pcLink);
        } else {
            pstat->st_size = 0;
        }
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        if (prfsn->RFSN_time == (time_t)(PX_ERROR)) {
            pstat->st_atime = API_RootFsTime(LW_NULL);
            pstat->st_mtime = API_RootFsTime(LW_NULL);
            pstat->st_ctime = API_RootFsTime(LW_NULL);
        } else {
            pstat->st_atime = prfsn->RFSN_time;                         /*  节点创建基准时间            */
            pstat->st_mtime = prfsn->RFSN_time;
            pstat->st_ctime = prfsn->RFSN_time;
        }
    } else {
        pstat->st_dev     = (dev_t)&_G_devhdrRoot;
        pstat->st_ino     = (ino_t)0;                                   /*  根目录                      */
        pstat->st_mode    = (DEFAULT_FILE_PERM | S_IFDIR);              /*  默认属性                    */
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;
        pstat->st_gid     = 0;
        pstat->st_rdev    = 1;
        pstat->st_size    = 0;
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        pstat->st_atime   = API_RootFsTime(LW_NULL);                    /*  默认使用 root fs 基准时间   */
        pstat->st_mtime   = API_RootFsTime(LW_NULL);
        pstat->st_ctime   = API_RootFsTime(LW_NULL);
    }

#else
    if (pstat == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pstat->st_dev     = (dev_t)&_G_devhdrRoot;
    pstat->st_ino     = (ino_t)0;                                       /*  根目录                      */
    pstat->st_mode    = (0444 | S_IFDIR);                               /*  默认属性                    */
    pstat->st_nlink   = 1;
    pstat->st_uid     = 0;
    pstat->st_gid     = 0;
    pstat->st_rdev    = 1;
    pstat->st_size    = 0;
    pstat->st_blksize = 0;
    pstat->st_blocks  = 0;
    pstat->st_atime   = API_RootFsTime(LW_NULL);                        /*  默认使用 root fs 基准时间   */
    pstat->st_mtime   = API_RootFsTime(LW_NULL);
    pstat->st_ctime   = API_RootFsTime(LW_NULL);
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __rootFsLStatGet
** 功能描述: root fs 获得文件状态及属性 (如果是链接文件则获取连接文件的属性)
** 输　入  : pdevhdr             设备头
**           pstat               stat 结构
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __rootFsLStatGet (LW_DEV_HDR *pdevhdr, PCHAR  pcName, struct stat *pstat)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  需要提供分级目录管理        */
    /*
     *  文件扩展数据为节点指针
     */
    PLW_ROOTFS_NODE    prfsnFather;
    PLW_ROOTFS_NODE    prfsn;
    BOOL               bIsRoot;
    PCHAR              pcTail = LW_NULL;
    

    if (pcName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    __rootFsFixName(pcName);                                            /*  修正文件名                  */
    
    __LW_ROOTFS_LOCK();                                                 /*  锁定 rootfs                 */
    prfsn = __rootFsFindNode(pcName, &prfsnFather, &bIsRoot, LW_NULL, &pcTail);
                                                                        /*  查询设备                    */
    if (prfsn) {                                                        /*  一定不是 FOLLOW_LINK_TAIL   */
        pstat->st_dev     = (dev_t)&_G_devhdrRoot;
        pstat->st_ino     = (ino_t)prfsn;
        pstat->st_mode    = prfsn->RFSN_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = prfsn->RFSN_uid;
        pstat->st_gid     = prfsn->RFSN_gid;
        pstat->st_rdev    = 1;
        if (prfsn->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_LNK) {
            pstat->st_size = lib_strlen(prfsn->RFSN_pcLink);
        } else {
            pstat->st_size = 0;
        }
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        if (prfsn->RFSN_time == (time_t)(PX_ERROR)) {
            pstat->st_atime = API_RootFsTime(LW_NULL);
            pstat->st_mtime = API_RootFsTime(LW_NULL);
            pstat->st_ctime = API_RootFsTime(LW_NULL);
        } else {
            pstat->st_atime = prfsn->RFSN_time;                         /*  节点创建基准时间            */
            pstat->st_mtime = prfsn->RFSN_time;
            pstat->st_ctime = prfsn->RFSN_time;
        }
        __LW_ROOTFS_UNLOCK();                                           /*  解锁 rootfs                 */
    
    } else {
        __LW_ROOTFS_UNLOCK();                                           /*  解锁 rootfs                 */
        
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
#else
    if (pstat == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pstat->st_dev     = (dev_t)&_G_devhdrRoot;
    pstat->st_ino     = (dev_t)0;                                       /*  根目录                      */
    pstat->st_mode    = (0666 | S_IFDIR);                               /*  默认属性                    */
    pstat->st_nlink   = 1;
    pstat->st_uid     = 0;
    pstat->st_gid     = 0;
    pstat->st_rdev    = 1;
    pstat->st_size    = 0;
    pstat->st_blksize = 0;
    pstat->st_blocks  = 0;
    pstat->st_atime   = API_RootFsTime(LW_NULL);                        /*  默认使用 root fs 基准时间   */
    pstat->st_mtime   = API_RootFsTime(LW_NULL);
    pstat->st_ctime   = API_RootFsTime(LW_NULL);
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __rootFsStatfsGet
** 功能描述: root fs 获得文件系统状态及属性
** 输　入  : pdevhdr
**           pstatfs             statfs 结构
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __rootFsStatfsGet (LW_DEV_HDR *pdevhdr, struct statfs *pstatfs)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  需要提供分级目录管理        */
    if (pstatfs) {
        pstatfs->f_type   = 0;
        pstatfs->f_bsize  = (long)_G_rfsrRoot.RFSR_stMemUsed;           /*  根文件系统内存使用量        */
        pstatfs->f_blocks = 1;
        pstatfs->f_bfree  = 0;
        pstatfs->f_bavail = 1;
        
        pstatfs->f_files  = (long)_G_rfsrRoot.RFSR_ulFiles;             /*  文件总数                    */
        pstatfs->f_ffree  = 0;
        
#if LW_CFG_CPU_WORD_LENGHT == 64
        pstatfs->f_fsid.val[0] = (int32_t)((addr_t)pdevhdr >> 32);
        pstatfs->f_fsid.val[1] = (int32_t)((addr_t)pdevhdr & 0xffffffff);
#else
        pstatfs->f_fsid.val[0] = (int32_t)pdevhdr;
        pstatfs->f_fsid.val[1] = 0;
#endif
        
        pstatfs->f_flag    = 0;
        pstatfs->f_namelen = PATH_MAX;
    }
#else
    REGISTER PLW_DEV_HDR            pdevhdrTemp;
             PLW_LIST_LINE          plineTemp;
             long                   lSize  = 0;
             INT                    iCount = 0;

    _IosLock();
    for (plineTemp  = _S_plineDevHdrHeader; 
         plineTemp != LW_NULL; 
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pdevhdrTemp = _LIST_ENTRY(plineTemp, LW_DEV_HDR, DEVHDR_lineManage);
        
        lSize += (long)lib_strlen(pdevhdr->DEVHDR_pcName);
        iCount++;
    }
    _IosUnlock();
    
    lSize += (long)(iCount * sizeof(LW_DEV_HDR));
    
    if (pstatfs) {
        pstatfs->f_type   = 0;
        pstatfs->f_bsize  = lSize;
        pstatfs->f_blocks = 1;
        pstatfs->f_bfree  = 0;
        pstatfs->f_bavail = 1;
        
        pstatfs->f_files  = 0;
        pstatfs->f_ffree  = 0;
        
#if LW_CFG_CPU_WORD_LENGHT == 64
        pstatfs->f_fsid.val[0] = (int32_t)((addr_t)pdevhdr >> 32);
        pstatfs->f_fsid.val[1] = (int32_t)((addr_t)pdevhdr & 0xffffffff);
#else
        pstatfs->f_fsid.val[0] = (int32_t)pdevhdr;
        pstatfs->f_fsid.val[1] = 0;
#endif
        
        pstatfs->f_flag    = 0;
        pstatfs->f_namelen = PATH_MAX;
    }
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __rootFsReadDir
** 功能描述: root fs 获得指定目录信息
** 输　入  : pdevhdr
**           dir                 目录结构
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __rootFsReadDir (LW_DEV_HDR *pdevhdr, DIR  *dir)
{
             INT                i;
             INT                iError = ERROR_NONE;
    REGISTER LONG               iStart = dir->dir_pos;
             PLW_LIST_LINE      plineTemp;

#if LW_CFG_PATH_VXWORKS == 0                                            /*  需要提供分级目录管理        */
             PLW_LIST_LINE      plineHeader;
             PLW_ROOTFS_NODE    prfsn = (PLW_ROOTFS_NODE)pdevhdr;
             PLW_ROOTFS_NODE    prfsnTemp;
    
    __LW_ROOTFS_LOCK();                                                 /*  锁定 rootfs                 */
    if (prfsn == LW_NULL) {
        plineHeader = _G_rfsrRoot.RFSR_plineSon;
    } else {
        if (prfsn->RFSN_iNodeType != LW_ROOTFS_NODE_TYPE_DIR) {
            __LW_ROOTFS_UNLOCK();                                       /*  解锁 rootfs                 */
            _ErrorHandle(ENOTDIR);
            return  (PX_ERROR);
        }
        plineHeader = prfsn->RFSN_plineSon;
    }
    
    for ((plineTemp  = plineHeader), (i = 0); 
         (plineTemp != LW_NULL) && (i < iStart); 
         (plineTemp  = _list_line_get_next(plineTemp)), (i++));         /*  忽略                        */
    
    if (plineTemp == LW_NULL) {
        _ErrorHandle(ENOENT);
        iError = PX_ERROR;                                              /*  没有多余的节点              */
    
    } else {
        prfsnTemp = _LIST_ENTRY(plineTemp, LW_ROOTFS_NODE, RFSN_lineBrother);
        dir->dir_pos++;
        /*
         *  拷贝文件名
         */
        if (prfsnTemp->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_DEV) {     /*  设备文件                    */
            /*
             *  注意, 设备名不包含目录部分
             */
            PCHAR       pcDevName = lib_rindex(prfsnTemp->RFSN_rfsnv.RFSNV_pdevhdr->DEVHDR_pcName, 
                                               PX_DIVIDER);
            if (pcDevName == LW_NULL) {
                pcDevName =  prfsnTemp->RFSN_rfsnv.RFSNV_pdevhdr->DEVHDR_pcName;
            } else {
                pcDevName++;                                            /*  过滤 /                      */
            }
            
            lib_strlcpy(dir->dir_dirent.d_name, 
                        pcDevName, 
                        sizeof(dir->dir_dirent.d_name));                /*  拷贝文件名                  */
            
            dir->dir_dirent.d_type = 
                prfsnTemp->RFSN_rfsnv.RFSNV_pdevhdr->DEVHDR_ucType;     /*  设备 d_type                 */
        
        } else {
            /*
             *  目录文件或链接文件.
             */
            lib_strlcpy(dir->dir_dirent.d_name, 
                        prfsnTemp->RFSN_rfsnv.RFSNV_pcName, 
                        sizeof(dir->dir_dirent.d_name));                /*  拷贝文件名                  */
        
            dir->dir_dirent.d_type = IFTODT(prfsnTemp->RFSN_mode);      /*  rootfs 节点 type            */
        }
        dir->dir_dirent.d_shortname[0] = PX_EOS;
    }
    __LW_ROOTFS_UNLOCK();                                               /*  解锁 rootfs                 */
    
#else
    REGISTER PLW_DEV_HDR        pdevhdrTemp;
             
    _IosLock();
    for ((plineTemp  = _S_plineDevHdrHeader), (i = 0); 
         (plineTemp != LW_NULL) && (i < iStart); 
         (plineTemp  = _list_line_get_next(plineTemp)), (i++));         /*  忽略                        */

    if (plineTemp == LW_NULL) {
        iError = PX_ERROR;                                              /*  没有多余的节点              */
    
    } else {
        pdevhdrTemp = _LIST_ENTRY(plineTemp, LW_DEV_HDR, DEVHDR_lineManage);
        if (lib_strcmp(pdevhdrTemp->DEVHDR_pcName, PX_STR_ROOT) == 0) { /*  忽略根目录设备              */
            plineTemp  = _list_line_get_next(plineTemp);                /*  检查下一个节点              */
            dir->dir_pos++;
            if (plineTemp == LW_NULL) {
                _ErrorHandle(ENOENT);
                iError = PX_ERROR;                                      /*  没有多余的节点              */
                goto    __scan_over;
            } else {
                pdevhdrTemp = _LIST_ENTRY(plineTemp, LW_DEV_HDR, DEVHDR_lineManage);
            }
        }
        
        dir->dir_pos++;
        lib_strlcpy(dir->dir_dirent.d_name, 
                    &pdevhdrTemp->DEVHDR_pcName[1], 
                    sizeof(dir->dir_dirent.d_name));                    /*  拷贝文件名                  */
        dir->dir_dirent.d_type = pdevhdrTemp->DEVHDR_ucType;
        dir->dir_dirent.d_shortname[0] = PX_EOS;
    }
__scan_over:
    _IosUnlock();
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __rootFsChmod
** 功能描述: root fs chmod 操作
** 输　入  : pdevhdr
**           iMode               mode_t
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __rootFsChmod (LW_DEV_HDR *pdevhdr, INT  iMode)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  需要提供分级目录管理        */
    PLW_ROOTFS_NODE    prfsn = (PLW_ROOTFS_NODE)pdevhdr;
    
    if (iMode & S_IFMT) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iMode |= S_IRUSR;
    
    prfsn->RFSN_mode = iMode | (prfsn->RFSN_mode & S_IFMT);
    
    return  (ERROR_NONE);
    
#else
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
}
/*********************************************************************************************************
** 函数名称: __rootFsIoctl
** 功能描述: root fs ioctl 操作
** 输　入  : pdevhdr            
**           request,           命令
**           arg                命令参数
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __rootFsIoctl (LW_DEV_HDR *pdevhdr,
                           INT        iRequest,
                           LONG       lArg)
{
    switch (iRequest) {
    
    case FIOFSTATGET:                                                   /*  获得文件状态                */
        return  (__rootFsStatGet(pdevhdr, (struct stat *)lArg));
        
    case FIOFSTATFSGET:                                                 /*  获得文件系统状态            */
        return  (__rootFsStatfsGet(pdevhdr, (struct statfs *)lArg));
    
    case FIOREADDIR:                                                    /*  获取一个目录信息            */
        return  (__rootFsReadDir(pdevhdr, (DIR *)lArg));
    
    case FIOSYNC:                                                       /*  将文件缓存回写              */
    case FIODATASYNC:
    case FIOFLUSH:
        return  (ERROR_NONE);
        
    case FIOCHMOD:
        return  (__rootFsChmod(pdevhdr, (INT)lArg));                    /*  改变文件访问权限            */
        
    case FIOFSTYPE:                                                     /*  获得文件系统类型            */
        *(PCHAR *)lArg = "ROOT FileSystem";
        return  (ERROR_NONE);
        
    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __rootFsSymlink
** 功能描述: root fs symlink 操作
** 输　入  : pdevhdr            
**           pcName             创建的连接文件
**           pcLinkDst          链接目标
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __rootFsSymlink (PLW_DEV_HDR     pdevhdr,
                             PCHAR           pcName,
                             CPCHAR          pcLinkDst)
{
    REGISTER INT        iError;

#if LW_CFG_PATH_VXWORKS == 0                                            /*  需要提供分级目录管理        */

    if ((pcName == LW_NULL) || (pcLinkDst == LW_NULL)) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
    
#if LW_CFG_MAX_VOLUMES > 0
    if (__fsCheckFileName(pcName)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
    
    __rootFsFixName(pcName);                                            /*  修正文件名                  */
    
    return  (API_RootFsMakeNode(pcName, 
                                LW_ROOTFS_NODE_TYPE_LNK, 
                                LW_ROOTFS_NODE_OPT_NONE, 
                                DEFAULT_SYMLINK_PERM,
                                (PVOID)(pcLinkDst)));
#else
    _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __rootFsReadlink
** 功能描述: root fs read link 操作
** 输　入  : pdevhdr                       设备头
**           pcName                        链接原始文件名
**           pcLinkDst                     链接目标文件名
**           stMaxSize                     缓冲大小
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __rootFsReadlink (PLW_DEV_HDR    pdevhdr,
                                  PCHAR          pcName,
                                  PCHAR          pcLinkDst,
                                  size_t         stMaxSize)
{
    REGISTER INT        iError;

#if LW_CFG_PATH_VXWORKS == 0                                            /*  需要提供分级目录管理        */
    
    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
    
    __rootFsFixName(pcName);                                            /*  修正文件名                  */
    
    return  (__rootFsReadNode(pcName, pcLinkDst, stMaxSize));
    
#else
    _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */

    return  (iError);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
