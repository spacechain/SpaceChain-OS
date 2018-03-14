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
** 文   件   名: ioLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 13 日
**
** 描        述: 系统 IO 内部功能函数库，

** BUG
2007.09.12  加入对裁剪的支持
2007.09.24  加入 _DebugHandle() 功能。
2008.01.16  修改了 IO 系统锁信号量的名字.
2008.03.23  修改代码格式.
2008.10.17  在系统重新启动或者关闭时, 将移除所有的设备.
2009.02.13  修改代码以适应新的 I/O 系统结构组织.
2009.03.16  _IosDeleteAll() 需要将所有打开的文件回写磁盘.
2009.06.25  _IosDeleteAll() 需要对表头做出特殊判断.
2009.08.27  当前路径默认为 "/" (root fs)
2009.11.21  加入任务删除回调.
            加入 io 环境初始化.
2009.12.13  可以支持 VxWorks 式的单级设备目录管理和 SylixOS 新的分级目录管理.
2010.07.10  /dev/null 设备支持打开与关闭操作.
2012.10.25  加入 _IosEnvInherit() 函数, 这样可以选择性的继承系统当前目录或者当前进程目录.
2013.01.09  IO 环境中加入对 umask 的支持.
2013.01.21  加入操作权限.
2013.01.22  加入 zero 设备驱动.
2013.08.14  _IosThreadDelete() 中加入对阻塞文件锁信息的移除.
2013.09.29  _IosDeleteAll() 中加入对 PCI 设备的复位.
2013.11.20  _IosInit() 加入对 eventfd, timerfd, signalfd 的初始化.
2016.10.22  简化系统重启设计.
*********************************************************************************************************/
#define  __SYLIXOS_PCI_DRV
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#define  __SYSTEM_MAIN_FILE
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入对裁剪的支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
#include "limits.h"
/*********************************************************************************************************
  文件系统相关
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)
#include "../SylixOS/fs/include/fs_fs.h"
#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
/*********************************************************************************************************
  进程环境
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  是否需要新的分级目录管理
*********************************************************************************************************/
#if LW_CFG_PATH_VXWORKS == 0
#include "../SylixOS/fs/rootFs/rootFs.h"
#include "../SylixOS/fs/rootFs/rootFsLib.h"
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
/*********************************************************************************************************
  文件系统初始化声明
*********************************************************************************************************/
#if LW_CFG_OEMDISK_EN > 0
extern VOID    API_OemDiskMountInit(VOID);
#endif                                                                  /*  LW_CFG_OEMDISK_EN > 0       */
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_MOUNT_EN > 0)
extern VOID    API_MountInit(VOID);
#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_MOUNT_EN > 0         */
/*********************************************************************************************************
  内部全局变量
*********************************************************************************************************/
static  LW_DEV_HDR            _G_devhdrNull;                            /*  空闲设备头                  */
static  LW_DEV_HDR            _G_devhdrZero;                            /*  zero设备头                  */
static  LW_OBJECT_HANDLE      _G_hIosMSemaphore;                        /*  IO(s) 锁                    */
/*********************************************************************************************************
** 函数名称: _IosLock
** 功能描述: 进入IO(s)系统临界区
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _IosLock (VOID)
{
    API_SemaphoreMPend(_G_hIosMSemaphore, LW_OPTION_WAIT_INFINITE);
}
/*********************************************************************************************************
** 函数名称: _IosUnlock
** 功能描述: 退出IO(s)系统临界区
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _IosUnlock (VOID)
{
    API_SemaphoreMPost(_G_hIosMSemaphore);
}
/*********************************************************************************************************
** 函数名称: _IosCheckPermissions
** 功能描述: 判断访问权限
** 输　入  : iFlag            打开 flag
**           bExec            是否可执行
**           mode             文件 mode
**           uid              文件 uid
**           gid              文件 gid
** 输　出  : 是否拥有操作权限
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _IosCheckPermissions (INT  iFlag, BOOL  bExec, mode_t  mode, uid_t  uid, gid_t  gid)
{
    uid_t       uidExecCur = geteuid();
    gid_t       gidExecCur = getegid();
    mode_t      modeNeed = 0;
    
    INT         i;
    INT         iNum;
    gid_t       gidList[NGROUPS_MAX];
    
    if (uidExecCur == 0) {                                              /*  超级用户                    */
        return  (ERROR_NONE);
    }
    
    if (uidExecCur == uid) {
        mode = (mode & S_IRWXU) >> 6;                                   /*  owner permission            */
        
    } else if (gidExecCur == gid) {
        mode = (mode & S_IRWXG) >> 3;                                   /*  group permission            */
    
    } else {
        iNum = getgroups(NGROUPS_MAX, gidList);                         /*  查看附加组信息              */
        for (i = 0; i < iNum; i++) {
            if (gidList[i] == gid) {                                    /*  存在于相同附加组            */
                break;
            }
        }
        
        if (i < iNum) {
            mode = (mode & S_IRWXG) >> 3;                               /*  group permission            */
        
        } else {
            mode = (mode & S_IRWXO);                                    /*  other permission            */
        }
    }
    
    if (iFlag & (O_WRONLY | O_RDWR | O_TRUNC)) {                        /*  write                       */
        modeNeed |= S_IWOTH;
        
    } else {
        modeNeed |= S_IROTH;
    }
    
    if (bExec) {
        modeNeed |= S_IXOTH;
    }
    
    if ((modeNeed & mode) == modeNeed) {                                /*  检查是否满足权限            */
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: _IosNullOpen
** 功能描述: 系统虚拟首设备驱动打开函数
** 输　入  : pdevhdr       设备头
**           pcName        名字
**           iFlags        打开标志
**           iMode         mode_t
** 输　出  : 文件指针
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  _IosNullOpen (LW_DEV_HDR   *pdevhdr, 
                           PCHAR         pcName,   
                           INT           iFlags, 
                           INT           iMode)
{
    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    } else {
        return  ((LONG)pdevhdr);
    }
}
/*********************************************************************************************************
** 函数名称: _IosNullClose
** 功能描述: 系统虚拟首设备驱动关闭函数
** 输　入  : pdevhdr       设备头
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _IosNullClose (LW_DEV_HDR   *pdevhdr)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _IosNullWrite
** 功能描述: 系统虚拟首设备驱动写函数
** 输　入  : pdevhdr       设备头
**           pcBuffer      需要写入的数据
**           stNByte       需要写入的长度
** 输　出  : 写入长度
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  _IosNullWrite (LW_DEV_HDR  *pdevhdr, CPCHAR  pcBuffer, size_t  stNByte)
{
    (VOID)pdevhdr;
    (VOID)pcBuffer;
    
    return  ((ssize_t)stNByte);
}
/*********************************************************************************************************
** 函数名称: _IosNullIoctl
** 功能描述: 系统虚拟首设备驱动 ioctl 函数
** 输　入  : pdevhdr       设备头
**           iRequest      请求命令
**           lArg          命令参数
** 输　出  : 命令执行结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _IosNullIoctl (LW_DEV_HDR *pdevhdr,
                           INT         iRequest,
                           LONG        lArg)
{
    struct stat     *pstat = (struct stat *)lArg;

    if (iRequest == FIOFSTATGET) {
        if (!pstat) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        if (pdevhdr == &_G_devhdrNull) {
            pstat->st_dev     = (dev_t)pdevhdr;
            pstat->st_ino     = 0;
            pstat->st_mode    = S_IWUSR | S_IWGRP | S_IWOTH | S_IFCHR;  /*  默认属性                    */
            pstat->st_nlink   = 0;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 1;
            pstat->st_size    = 0;
            pstat->st_blksize = 0;
            pstat->st_blocks  = 0;
            pstat->st_atime   = API_RootFsTime(LW_NULL);                /*  默认使用 root fs 基准时间   */
            pstat->st_mtime   = API_RootFsTime(LW_NULL);
            pstat->st_ctime   = API_RootFsTime(LW_NULL);
        
        } else {
            pstat->st_dev     = (dev_t)pdevhdr;
            pstat->st_ino     = 0;
            pstat->st_mode    = S_IRUSR | S_IRGRP | S_IROTH | S_IFCHR;  /*  默认属性                    */
            pstat->st_nlink   = 0;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 1;
            pstat->st_size    = 0;
            pstat->st_blksize = 0;
            pstat->st_blocks  = 0;
            pstat->st_atime   = API_RootFsTime(LW_NULL);                /*  默认使用 root fs 基准时间   */
            pstat->st_mtime   = API_RootFsTime(LW_NULL);
            pstat->st_ctime   = API_RootFsTime(LW_NULL);
        }
    }

    _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: _IosZeroRead
** 功能描述: 系统 zero 设备驱动读函数
** 输　入  : pdevhdr       设备头
**           pcBuffer      需要读入的数据缓冲
**           stSize        缓冲大小
** 输　出  : 读出长度
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  _IosZeroRead (LW_DEV_HDR  *pdevhdr, PCHAR  pcBuffer, size_t  stSize)
{
    if (!pcBuffer || !stSize) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    lib_bzero(pcBuffer, stSize);
    
    return  ((ssize_t)stSize);
}
/*********************************************************************************************************
** 函数名称: _IosEnvGlobalInit
** 功能描述: 初始化全局 IO 环境
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _IosEnvGlobalInit (VOID)
{
    _S_ioeIoGlobalEnv.IOE_modeUMask = 0;                                /*  umask 0                     */
    lib_strcpy(_S_ioeIoGlobalEnv.IOE_cDefPath, PX_STR_ROOT);            /*  初始化为根目录              */
}
/*********************************************************************************************************
** 函数名称: _IosEnvCreate
** 功能描述: 创建一个 IO 环境
** 输　入  : NONE
** 输　出  : 创建的 IO 环境控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_IO_ENV  _IosEnvCreate (VOID)
{
    PLW_IO_ENV      pioe;
    
    pioe = (PLW_IO_ENV)__SHEAP_ALLOC(sizeof(LW_IO_ENV));
    if (pioe == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    lib_bzero(pioe, sizeof(LW_IO_ENV));
    
    return  (pioe);
}
/*********************************************************************************************************
** 函数名称: _IosEnvDelete
** 功能描述: 释放一个 IO 环境
** 输　入  : pioe      IO 环境控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _IosEnvDelete (PLW_IO_ENV  pioe)
{
    if (pioe) {
        __SHEAP_FREE(pioe);
    }
}
/*********************************************************************************************************
** 函数名称: _IosEnvGetDefault
** 功能描述: 获得当前默认的 I/O 环境
** 输　入  : NONE
** 输　出  : 获得当前使用的 I/O 环境控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_IO_ENV  _IosEnvGetDef (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
#if LW_CFG_MODULELOADER_EN > 0
    PLW_IO_ENV      pioe;
#endif
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  在中断中调用                */
        return  (&_S_ioeIoGlobalEnv);                                   /*  使用全局 IO 环境            */
    
    } else {
        LW_TCB_GET_CUR_SAFE(ptcbCur);                                   /*  当前任务控制块              */
        
#if LW_CFG_MODULELOADER_EN > 0
        pioe = vprocIoEnvGet(ptcbCur);                                  /*  获得当前进程 IO 环境        */
        if (pioe) {
            return  (pioe);
        }
#endif
        
        if (ptcbCur->TCB_pvIoEnv) {
            return  ((PLW_IO_ENV)ptcbCur->TCB_pvIoEnv);
                                                                        /*  使用私有 IO 环境            */
        } else {
            return  (&_S_ioeIoGlobalEnv);
        }
    }
}
/*********************************************************************************************************
** 函数名称: _IosEnvInherit
** 功能描述: 继承当前的 I/O 环境
** 输　入  : pioe      I/O 环境控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _IosEnvInherit (PLW_IO_ENV  pioe)
{
    PLW_IO_ENV  pioeDef = _IosEnvGetDef();

    if (pioe) {
        pioe->IOE_modeUMask = pioeDef->IOE_modeUMask;
        lib_strcpy(pioe->IOE_cDefPath, pioeDef->IOE_cDefPath);
    }
}
/*********************************************************************************************************
** 函数名称: _IosDeleteAll
** 功能描述: 移除所有操作系统中的设备
** 输　入  : iRebootType   重启类型
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _IosDeleteAll (INT  iRebootType)
{
    REGISTER PLW_DEV_HDR    pdevhdr;
             PLW_LIST_LINE  plineTemp;

    _IosLock();
    for (plineTemp  = _S_plineDevHdrHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pdevhdr = _LIST_ENTRY(plineTemp, LW_DEV_HDR, DEVHDR_lineManage);
        _IosUnlock();
        
        API_IosDevFileAbnormal(pdevhdr);                                /*  对应文件强制关闭            */
        
        _IosLock();
    }
    _IosUnlock();
    
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)
    API_DiskCacheSync(LW_NULL);                                         /*  回写所有磁盘缓冲            */
#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
}
/*********************************************************************************************************
** 函数名称: _IosThreadDelete
** 功能描述: 线程删除回调
** 输　入  : ulId      线程 ID
**           pvRetVal  返回值
**           ptcbDel   线程控制块
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _IosThreadDelete (LW_OBJECT_HANDLE  ulId, PVOID  pvRetVal, PLW_CLASS_TCB  ptcbDel)
{
    extern VOID  __fdLockfCleanupHook(PLW_CLASS_TCB  ptcbDel);

    if (ptcbDel && ptcbDel->TCB_pvIoEnv) {
        _IosEnvDelete((PLW_IO_ENV)ptcbDel->TCB_pvIoEnv);                /*  删除私有 io 环境            */
        ptcbDel->TCB_pvIoEnv = LW_NULL;                                 /*  防止重启后造成误判          */
    }
    
    __fdLockfCleanupHook(ptcbDel);                                      /*  删除阻塞文件锁              */
}
/*********************************************************************************************************
** 函数名称: _IosInit
** 功能描述: 初始化 IO 系统
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _IosInit (VOID)
{
#if LW_CFG_DEVICE_EN > 0
    INT     iNullDrv;
    INT     iZeroDrv;
    
    _IosEnvGlobalInit();                                                /*  初始化 io 环境              */
    
    _S_iIoMaxLinkLevels = LINK_MAX;                                     /*  默认最多级联 LINK_MAX 级    */
    
    lib_bzero(_S_deventryTbl, sizeof(_S_deventryTbl));                  /*  清空驱动程序表              */
    
    _S_plineDevHdrHeader = LW_NULL;                                     /*  初始化设备头链表            */
    
    _G_hIosMSemaphore = API_SemaphoreMCreate("ios_mutex", LW_PRIO_DEF_CEILING, 
                                             LW_OPTION_WAIT_PRIORITY | 
                                             LW_OPTION_DELETE_SAFE | 
                                             LW_OPTION_INHERIT_PRIORITY |
                                             LW_OPTION_OBJECT_GLOBAL, 
                                             LW_NULL);
    if (!_G_hIosMSemaphore) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "io system mutex can not be create.\r\n");
        return  (PX_ERROR);
    }
    
    iNullDrv = API_IosDrvInstall(_IosNullOpen, 
                                 LW_NULL,
                                 _IosNullOpen,
                                 _IosNullClose,
                                 LW_NULL,
                                 _IosNullWrite,
                                 _IosNullIoctl);                        /*  首先创建 NULL 设备驱动      */
    
    DRIVER_LICENSE(iNullDrv,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(iNullDrv,      "Han.hui");
    DRIVER_DESCRIPTION(iNullDrv, "null device driver.");
    
    iZeroDrv = API_IosDrvInstall(_IosNullOpen, 
                                 LW_NULL,
                                 _IosNullOpen,
                                 _IosNullClose,
                                 _IosZeroRead,
                                 LW_NULL,
                                 _IosNullIoctl);                        /*  创建 ZERO 设备驱动          */
                                 
    DRIVER_LICENSE(iZeroDrv,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(iZeroDrv,      "Han.hui");
    DRIVER_DESCRIPTION(iZeroDrv, "zero device driver.");
                                 
#if LW_CFG_PATH_VXWORKS == 0                                            /*  是否分级目录管理            */
    rootFsDrv();                                                        /*  安装 rootFs 驱动            */
    rootFsDevCreate();                                                  /*  创建根设备                  */
    
    API_RootFsMakeNode("/dev", LW_ROOTFS_NODE_TYPE_DIR, LW_ROOTFS_NODE_OPT_ROOTFS_TIME, 
                       DEFAULT_DIR_PERM, LW_NULL);                      /*  设备目录                    */
    API_RootFsMakeNode("/dev/pty", LW_ROOTFS_NODE_TYPE_DIR, LW_ROOTFS_NODE_OPT_ROOTFS_TIME,
                       DEFAULT_DIR_PERM, LW_NULL);                      /*  虚拟终端设备                */
    API_RootFsMakeNode("/dev/pipe", LW_ROOTFS_NODE_TYPE_DIR, LW_ROOTFS_NODE_OPT_ROOTFS_TIME,
                       DEFAULT_DIR_PERM, LW_NULL);                      /*  管道设备                    */
    API_RootFsMakeNode("/dev/input", LW_ROOTFS_NODE_TYPE_DIR, LW_ROOTFS_NODE_OPT_ROOTFS_TIME,
                       DEFAULT_DIR_PERM, LW_NULL);                      /*  输入设备                    */
    API_RootFsMakeNode("/dev/blk", LW_ROOTFS_NODE_TYPE_DIR, LW_ROOTFS_NODE_OPT_ROOTFS_TIME,
                       DEFAULT_DIR_PERM, LW_NULL);                      /*  blk raw 设备                */
    API_RootFsMakeNode("/mnt", LW_ROOTFS_NODE_TYPE_DIR, LW_ROOTFS_NODE_OPT_ROOTFS_TIME, 
                       DEFAULT_DIR_PERM, LW_NULL);                      /*  文件系统挂载目录            */
    API_RootFsMakeNode("/media", LW_ROOTFS_NODE_TYPE_DIR, LW_ROOTFS_NODE_OPT_ROOTFS_TIME, 
                       DEFAULT_DIR_PERM, LW_NULL);                      /*  热插拔存储器挂载目录        */
                                                                        /*  unix 兼容 null 设备路径     */
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
    
    (VOID)API_IosDevAddEx(&_G_devhdrNull, "/dev/null", iNullDrv, DT_CHR); 
    (VOID)API_IosDevAddEx(&_G_devhdrZero, "/dev/zero", iZeroDrv, DT_CHR);
    
    (VOID)API_SystemHookAdd((LW_HOOK_FUNC)_IosThreadDelete, 
                            LW_OPTION_THREAD_DELETE_HOOK);              /*  线程删除时回调              */
    (VOID)API_SystemHookAdd((LW_HOOK_FUNC)_IosDeleteAll, 
                            LW_OPTION_KERNEL_REBOOT);                   /*  安装系统关闭时, 回调函数    */
    
#if LW_CFG_EVENTFD_EN > 0
    eventfdDrv();
    eventfdDevCreate();
#endif                                                                  /*  LW_CFG_EVENTFD_EN > 0       */
    
#if LW_CFG_PTIMER_EN > 0 && LW_CFG_TIMERFD_EN > 0
    timerfdDrv();
    timerfdDevCreate();
    hstimerfdDrv();
    hstimerfdDevCreate();
#endif                                                                  /*  LW_CFG_TIMERFD_EN > 0       */
    
#if LW_CFG_SIGNALFD_EN > 0
    signalfdDrv();
    signalfdDevCreate();
#endif                                                                  /*  LW_CFG_SIGNALFD_EN > 0      */
    
#if LW_CFG_GPIO_EN > 0
    gpiofdDrv();
    gpiofdDevCreate();
#endif                                                                  /*  LW_CFG_GPIO_EN > 0          */
    
#if LW_CFG_OEMDISK_EN > 0
    API_OemDiskMountInit();
#endif                                                                  /*  LW_CFG_OEMDISK_EN > 0       */
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_MOUNT_EN > 0)
    API_MountInit();
#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_MOUNT_EN > 0         */
    return  (ERROR_NONE);
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
