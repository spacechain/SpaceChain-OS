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
** 文   件   名: tpsfs_sylixos.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: tpsfs SylixOS FS 接口.

** BUG:
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0
#include "tpsfs_type.h"
#include "tpsfs_error.h"
#include "tpsfs_port.h"
#include "tpsfs_super.h"
#include "tpsfs_trans.h"
#include "tpsfs_btree.h"
#include "tpsfs_inode.h"
#include "tpsfs_dir.h"
#include "tpsfs_super.h"
#include "tpsfs.h"
/*********************************************************************************************************
  TPS Struct
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR          TPSVOL_devhdrHdr;                               /*  设备头                      */
    TPS_DEV             TPSVOL_dev;
    BOOL                TPSVOL_bForceDelete;                            /*  是否允许强制卸载卷          */
    PTPS_SUPER_BLOCK    TPSVOL_tpsFsVol;                                /*  文件系统卷信息              */
    INT                 TPSVOL_iDrv;                                    /*  驱动表位置                  */
    BOOL                TPSVAL_bValid;                                  /*  有效性标志                  */
    LW_OBJECT_HANDLE    TPSVOL_hVolLock;                                /*  卷操作锁                    */
    LW_LIST_LINE_HEADER TPSVOL_plineFdNodeHeader;                       /*  fd_node 链表                */
    UINT64              TPSVOL_uiTime;                                  /*  卷创建时间 TPS 格式         */
    INT                 TPSVOL_iFlag;                                   /*  O_RDONLY or O_RDWR          */
    LW_HANDLE           TPSVOL_hThreadFlush;                            /*  回写线程句柄                */
    BOOL                TPSVOL_bNeedExit;                               /*  是否需要退出线程            */
} TPS_VOLUME;
typedef TPS_VOLUME     *PTPS_VOLUME;

typedef struct {
    PTPS_VOLUME         TPSFIL_ptpsvol;                                 /*  所在卷信息                  */
    PTPS_INODE          TPSFIL_pinode;                                  /*  文件inode                   */
    INT                 TPSFIL_iFileType;                               /*  文件类型                    */
    CHAR                TPSFIL_cName[1];                                /*  文件名                      */
} TPS_FILE;
typedef TPS_FILE       *PTPS_FILE;
/*********************************************************************************************************
  内部全局变量
*********************************************************************************************************/
static INT              _G_iTpsDrvNum = PX_ERROR;
/*********************************************************************************************************
  inode 回写间隔 (ms)
*********************************************************************************************************/
#define __TPS_INODE_FLUSH_INTERVAL    2000
/*********************************************************************************************************
  文件类型
*********************************************************************************************************/
#define __TPS_FILE_TYPE_NODE          0                                 /*  open 打开文件               */
#define __TPS_FILE_TYPE_DIR           1                                 /*  open 打开目录               */
#define __TPS_FILE_TYPE_DEV           2                                 /*  open 打开设备               */
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define __TPS_FILE_LOCK(ptpsfile)   API_SemaphoreMPend(ptpsfile->TPSFIL_ptpsvol->TPSVOL_hVolLock, \
                                    LW_OPTION_WAIT_INFINITE)
#define __TPS_FILE_UNLOCK(ptpsfile) API_SemaphoreMPost(ptpsfile->TPSFIL_ptpsvol->TPSVOL_hVolLock)
#define __TPS_VOL_LOCK(ptpsvol)     API_SemaphoreMPend(ptpsvol->TPSVOL_hVolLock, LW_OPTION_WAIT_INFINITE)
#define __TPS_VOL_UNLOCK(ptpsvol)   API_SemaphoreMPost(ptpsvol->TPSVOL_hVolLock)
/*********************************************************************************************************
  检测路径字串是否为根目录或者直接指向设备
*********************************************************************************************************/
#define __STR_IS_ROOT(pcName)       ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))
/*********************************************************************************************************
  打印inode信息
*********************************************************************************************************/
#ifdef __SYLIXOS_DEBUG
#define TPSFS_IOCTRL_INODE_PRINT     LW_OSIOD('f', 135, BOOL)
#endif
/*********************************************************************************************************
  驱动程序声明
*********************************************************************************************************/
static LONG     __tpsFsOpen(PTPS_VOLUME     ptpsvol,
                            PCHAR           pcName,
                            INT             iFlags,
                            INT             iMode);
static INT      __tpsFsRemove(PTPS_VOLUME     ptpsvol,
                              PCHAR           pcName);
static INT      __tpsFsClose(PLW_FD_ENTRY   pfdentry);
static ssize_t  __tpsFsRead(PLW_FD_ENTRY   pfdentry,
                            PCHAR          pcBuffer,
                            size_t         stMaxBytes);
static ssize_t  __tpsFsPRead(PLW_FD_ENTRY   pfdentry,
                             PCHAR          pcBuffer,
                             size_t         stMaxBytes,
                             off_t      oftPos);
static ssize_t  __tpsFsWrite(PLW_FD_ENTRY   pfdentry,
                             PCHAR          pcBuffer,
                             size_t         stNBytes);
static ssize_t  __tpsFsPWrite(PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer,
                              size_t        stNBytes,
                              off_t         oftPos);
static INT      __tpsFsLStat(PTPS_VOLUME   ptpsfs,
                             PCHAR         pcName,
                             struct stat  *pstat);
static INT      __tpsFsIoctl(PLW_FD_ENTRY   pfdentry,
                             INT            iRequest,
                             LONG           lArg);
static INT      __tpsFsSymlink(PTPS_VOLUME   ptpsvol,
                               PCHAR         pcName,
                               CPCHAR        pcLinkDst);
static ssize_t  __tpsFsReadlink(PTPS_VOLUME   ptpsvol,
                                PCHAR         pcName,
                                PCHAR         pcLinkDst,
                                size_t        stMaxSize);
static INT      __tpsFsSync(PLW_FD_ENTRY   pfdentry, BOOL  bFlushCache);
/*********************************************************************************************************
  文件系统创建函数
*********************************************************************************************************/
LW_API INT      API_TpsFsDevCreate(PCHAR   pcName, PLW_BLK_DEV  pblkd);
/*********************************************************************************************************
  block函数声明
*********************************************************************************************************/
INT             __blockIoDevCreate(PLW_BLK_DEV  pblkdNew);
VOID            __blockIoDevDelete(INT  iIndex);
PLW_BLK_DEV     __blockIoDevGet(INT  iIndex);
INT             __blockIoDevReset(INT  iIndex);
INT             __blockIoDevIoctl(INT  iIndex, INT  iCmd, LONG  lArg);
INT             __blockIoDevIsLogic(INT  iIndex);
INT             __blockIoDevFlag(INT     iIndex);
PLW_BLK_DEV     __blockIoDevGet(INT  iIndex);
INT             __blockIoDevRead(INT     iIndex,
                                 VOID   *pvBuffer,
                                 ULONG   ulStartSector,
                                 ULONG   ulSectorCount);
INT             __blockIoDevWrite(INT     iIndex,
                                  VOID   *pvBuffer,
                                  ULONG   ulStartSector,
                                  ULONG   ulSectorCount);
INT             __blockIoDevStatus(INT     iIndex);
/*********************************************************************************************************
** 函数名称: __tpsFsFlushThread
** 功能描述: inode 回写线程
** 输　入  : ptpsvol    卷
** 输　出  :
** 全局变量:
** 调用模块:
** 注  意  : 这里在线程退出之前需要提前调用 detach 激活文件系统卸载任务, 因为卸载任务可能是 t_except 所以
             真正的线程删除操作可能得不到执行.
*********************************************************************************************************/
static void  __tpsFsFlushThread (PTPS_VOLUME  ptpsvol)
{
    while (!ptpsvol->TPSVOL_bNeedExit) {
        API_TimeMSleep(__TPS_INODE_FLUSH_INTERVAL);

        __TPS_VOL_LOCK(ptpsvol);
        tpsFsFlushInodes(ptpsvol->TPSVOL_tpsFsVol);
        __TPS_VOL_UNLOCK(ptpsvol);
    }
    
    API_ThreadDetach(API_ThreadIdSelf());
}
/*********************************************************************************************************
** 函数名称: __tpsFsDiskIndex
** 功能描述: 获取 block I/O index.
** 输　入  : pdev      设备文件
** 输　出  : block io index.
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_INLINE INT  __tpsFsDiskIndex (PTPS_DEV  pdev)
{
    PTPS_VOLUME  ptpsvol = _LIST_ENTRY(pdev, TPS_VOLUME, TPSVOL_dev);
    
    return  (ptpsvol->TPSVOL_iDrv);
}
/*********************************************************************************************************
** 函数名称: __tpsFsDiskSectorSize
** 功能描述: 获取扇区大小
**           pdev               设备文件
** 输　出  : 扇区数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT  __tpsFsDiskSectorSize (PTPS_DEV pdev)
{
    INT     iDrv      = __tpsFsDiskIndex(pdev);
    ULONG   ulSecSize = 0;

    __blockIoDevIoctl(iDrv, LW_BLKD_GET_SECSIZE, (LONG)&ulSecSize);

    return  ((UINT)ulSecSize);
}
/*********************************************************************************************************
** 函数名称: __tpsFsDiskSectorCnt
** 功能描述: 获取扇区数
**           pdev               设备文件
** 输　出  : 扇区数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT64  __tpsFsDiskSectorCnt (PTPS_DEV pdev)
{
    INT     iDrv     = __tpsFsDiskIndex(pdev);
    ULONG   ulSecCnt = 0;

    __blockIoDevIoctl(iDrv, LW_BLKD_GET_SECNUM, (LONG)&ulSecCnt);

    return  ((UINT64)ulSecCnt);
}
/*********************************************************************************************************
** 函数名称: __tpsFsDiskRead
** 功能描述: 读取数据到磁盘
**           pdev               设备文件
** 输　入  : ui64StartSector    起始扇区
**           uiSectorCnt        扇区数
** 输　出  : 0表示是成功，非0 表示失败
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsDiskRead (PTPS_DEV    pdev,
                             PUCHAR      pucBuf,
                             UINT64      ui64StartSector,
                             UINT64      uiSectorCnt)
{
    INT     iDrv = __tpsFsDiskIndex(pdev);

    return  (__blockIoDevRead(iDrv,
                              (PVOID)pucBuf,
                              (ULONG)ui64StartSector,
                              (ULONG)uiSectorCnt));
}
/*********************************************************************************************************
** 函数名称: __tpsFsDiskWrite
** 功能描述: 写数据到磁盘
**           pdev               设备文件
** 输　入  : ui64StartSector    起始扇区
**           uiSectorCnt        扇区数
**           bSync              是否需要同步
** 输　出  : 0表示是成功，非0 表示失败
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsDiskWrite (PTPS_DEV   pdev,
                              PUCHAR     pucBuf,
                              UINT64     ui64StartSector,
                              UINT64     uiSectorCnt,
                              BOOL       bSync)
{
    INT     iDrv = __tpsFsDiskIndex(pdev);
    INT     iRtn;

    iRtn = __blockIoDevWrite((INT)iDrv,
                             (PVOID)pucBuf,
                             (ULONG)ui64StartSector,
                             (ULONG)uiSectorCnt);
    if (iRtn != 0) {
        return  (iRtn);
    }

    if (bSync) {                                                        /* 同步写入                     */
        LW_BLK_RANGE blkrange;
        
        blkrange.BLKR_ulStartSector = (ULONG)ui64StartSector;
        blkrange.BLKR_ulEndSector   = (ULONG)(ui64StartSector + uiSectorCnt - 1);

        iRtn = __blockIoDevIoctl((INT)iDrv, FIOSYNCMETA, (LONG)&blkrange);
    }

    return  (iRtn);
}
/*********************************************************************************************************
** 函数名称: __tpsFsDiskSync
** 功能描述: 同步扇区数据到磁盘
**           pdev               设备文件
** 输　入  : ui64StartSector    起始扇区
**           uiSectorCnt        扇区数
** 输　出  : 0 别是成功， 非0 表示失败
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsDiskSync (PTPS_DEV pdev, UINT64 ui64StartSector, UINT64 uiSectorCnt)
{
    INT          iDrv = __tpsFsDiskIndex(pdev);
    LW_BLK_RANGE blkrange;
    
    blkrange.BLKR_ulStartSector = (ULONG)(ui64StartSector);
    blkrange.BLKR_ulEndSector   = (ULONG)(ui64StartSector + uiSectorCnt - 1);

    return  (__blockIoDevIoctl((INT)iDrv, FIOSYNCMETA, (LONG)&blkrange));
}
/*********************************************************************************************************
** 函数名称: __tpsFsDiskSync
** 功能描述: 同步扇区数据到磁盘
**           pdev               设备文件
** 输　入  : ui64StartSector    起始扇区
**           uiSectorCnt        扇区数
** 输　出  : 0 别是成功， 非0 表示失败
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsDiskTrim (PTPS_DEV pdev, UINT64 ui64StartSector, UINT64 uiSectorCnt)
{
    INT          iDrv = __tpsFsDiskIndex(pdev);
    LW_BLK_RANGE blkrange;

    blkrange.BLKR_ulStartSector = (ULONG)(ui64StartSector);
    blkrange.BLKR_ulEndSector   = (ULONG)(ui64StartSector + uiSectorCnt - 1);

    return  (__blockIoDevIoctl((INT)iDrv, FIOTRIM, (LONG)&blkrange));
}
/*********************************************************************************************************
** 函数名称: API_TpsFsDrvInstall
** 功能描述: 安装 TPS 文件系统驱动程序
** 输　入  :
** 输　出  : < 0 表示失败
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_TpsFsDrvInstall (VOID)
{
    struct file_operations     fileop;

    if (_G_iTpsDrvNum > 0) {
        return  (ERROR_NONE);
    }

    lib_bzero(&fileop, sizeof(struct file_operations));

    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __tpsFsOpen;
    fileop.fo_release  = __tpsFsRemove;
    fileop.fo_open     = __tpsFsOpen;
    fileop.fo_close    = __tpsFsClose;
    fileop.fo_read     = __tpsFsRead;
    fileop.fo_read_ex  = __tpsFsPRead;
    fileop.fo_write    = __tpsFsWrite;
    fileop.fo_write_ex = __tpsFsPWrite;
    fileop.fo_lstat    = __tpsFsLStat;
    fileop.fo_ioctl    = __tpsFsIoctl;
    fileop.fo_symlink  = __tpsFsSymlink;
    fileop.fo_readlink = __tpsFsReadlink;

    _G_iTpsDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);       /*  使用 NEW_1 型设备驱动       */

    DRIVER_LICENSE(_G_iTpsDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iTpsDrvNum,      "Jiang.Taijin");
    DRIVER_DESCRIPTION(_G_iTpsDrvNum, "tpsFs driver.");

    _DebugHandle(__LOGMESSAGE_LEVEL, "SylixOS tpsFs file system installed.\r\n");

    __fsRegister("tpsfs", API_TpsFsDevCreate, LW_NULL);                 /*  注册文件系统                */

    return  ((_G_iTpsDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** 函数名称: API_TpsFsDevCreate
** 功能描述: 创建一个 TPS 设备, 例如: API_TpsFsDevCreate("/ata0", ...);
**           与 sylixos 的 yaffs 不同, TPS 每一个卷都是独立的设备.
** 输　入  : pcName            设备名(设备挂接的节点地址)
**           pblkd             块设备驱动
** 输　出  : < 0 表示失败
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
INT  API_TpsFsDevCreate (PCHAR   pcName, PLW_BLK_DEV  pblkd)
{
    REGISTER PTPS_VOLUME            ptpsvol;
    REGISTER INT                    iBlkdIndex;
             INT                    iErrLevel      = 0;
             UINT                   uiMountFlag    = 0;
             errno_t                iErr           = ERROR_NONE;
             LW_CLASS_THREADATTR    threadattr;

    if (_G_iTpsDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "tps Driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    if (pblkd == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "block device invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    if ((pcName == LW_NULL) || __STR_IS_ROOT(pcName)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "volume name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }

    if ((pblkd->BLKD_iLogic == 0) && (pblkd->BLKD_uiLinkCounter)) {     /*  物理设备被引用后不允许加载  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "logic device has already mount.\r\n");
        _ErrorHandle(ERROR_IO_ACCESS_DENIED);
        return  (PX_ERROR);
    }

    iBlkdIndex = __blockIoDevCreate(pblkd);                             /*  加入块设备驱动表            */
    if (iBlkdIndex == -1) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "block device invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    } else if (iBlkdIndex == -2) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "block device table full.\r\n");
        _ErrorHandle(ERROR_IOS_DRIVER_GLUT);
        return  (PX_ERROR);
    }

    ptpsvol = (PTPS_VOLUME)__SHEAP_ALLOC(sizeof(TPS_VOLUME));
    if (ptpsvol == LW_NULL) {
        __blockIoDevDelete(iBlkdIndex);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(ptpsvol, sizeof(TPS_VOLUME));                             /*  清空卷控制块                */

    ptpsvol->TPSVOL_dev.DEV_SectorSize    = __tpsFsDiskSectorSize;
    ptpsvol->TPSVOL_dev.DEV_SectorCnt     = __tpsFsDiskSectorCnt;
    ptpsvol->TPSVOL_dev.DEV_ReadSector    = __tpsFsDiskRead;
    ptpsvol->TPSVOL_dev.DEV_WriteSector   = __tpsFsDiskWrite;
    ptpsvol->TPSVOL_dev.DEV_Sync          = __tpsFsDiskSync;
    ptpsvol->TPSVOL_dev.DEV_Trim          = __tpsFsDiskTrim;

    ptpsvol->TPSVOL_bForceDelete = LW_FALSE;                            /*  不允许强制卸载卷            */
    ptpsvol->TPSVOL_iDrv     = iBlkdIndex;                              /*  记录驱动位置                */
    ptpsvol->TPSVAL_bValid   = LW_TRUE;                                 /*  卷有效                      */
    ptpsvol->TPSVOL_hVolLock = API_SemaphoreMCreate("tpsvol_lock",
                               LW_PRIO_DEF_CEILING,
                               LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                               LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                               LW_NULL);
    if (!ptpsvol->TPSVOL_hVolLock) {                                    /*  无法创建卷锁                */
        iErrLevel = 2;
        goto    __error_handle;
    }
    ptpsvol->TPSVOL_plineFdNodeHeader = LW_NULL;                        /*  没有文件被打开              */
    ptpsvol->TPSVOL_uiTime = TPS_UTC_TIME();                            /*  获得当前时间                */

    if (pblkd->BLKD_iFlag == O_RDONLY) {                                /*  访问模式转换成tpsfs模式     */
        uiMountFlag = TPS_MOUNT_FLAG_READ;

    } else if (pblkd->BLKD_iFlag == O_WRONLY) {
        uiMountFlag = TPS_MOUNT_FLAG_WRITE;

    } else {
        uiMountFlag = TPS_MOUNT_FLAG_WRITE | TPS_MOUNT_FLAG_READ;
    }

    iErr = tpsFsMount(&ptpsvol->TPSVOL_dev, uiMountFlag, &ptpsvol->TPSVOL_tpsFsVol);
    if (iErr != ERROR_NONE) {                                           /*  挂载文件系统                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "mount tpsfs failed.\r\n");
        iErrLevel = 3;
        goto    __error_handle;
    }

    /*
     *  启动 inode 回写线程
     */
    threadattr = API_ThreadAttrGetDefault();
    threadattr.THREADATTR_pvArg           = (void *)ptpsvol;
    threadattr.THREADATTR_stStackByteSize = LW_CFG_THREAD_DISKCACHE_STK_SIZE;
    threadattr.THREADATTR_ucPriority      = LW_PRIO_T_SERVICE;
    threadattr.THREADATTR_ulOption       |= LW_OPTION_OBJECT_GLOBAL;

    ptpsvol->TPSVOL_bNeedExit    = LW_FALSE;
    ptpsvol->TPSVOL_hThreadFlush = API_ThreadCreate("t_tpsfs",
                                                    (PTHREAD_START_ROUTINE)__tpsFsFlushThread,
                                                    &threadattr, LW_NULL);
                                                                        /*  创建背景更新线程            */
    ptpsvol->TPSVOL_iFlag = pblkd->BLKD_iFlag;

    if (iosDevAddEx(&ptpsvol->TPSVOL_devhdrHdr, pcName, _G_iTpsDrvNum, DT_DIR)
        != ERROR_NONE) {                                                /*  安装文件系统设备            */
        iErr = (INT)API_GetLastError();
        iErrLevel = 4;
        goto    __error_handle;
    }
    __fsDiskLinkCounterAdd(pblkd);                                      /*  增加块设备链接              */

    __blockIoDevIoctl(iBlkdIndex, LW_BLKD_CTRL_POWER, LW_BLKD_POWER_ON);/*  打开电源                    */
    __blockIoDevReset(iBlkdIndex);                                      /*  复位磁盘接口                */
    __blockIoDevIoctl(iBlkdIndex, FIODISKINIT, 0);                      /*  初始化磁盘                  */

    _DebugFormat(__LOGMESSAGE_LEVEL, "disk \"%s\" mount ok.\r\n", pcName);

    return  (ERROR_NONE);

    /*
     *  ERROR OCCUR
     */
__error_handle:
    if (iErrLevel > 3) {
        ptpsvol->TPSVOL_bNeedExit = LW_TRUE;
        API_ThreadWakeup(ptpsvol->TPSVOL_hThreadFlush);
        API_ThreadJoin(ptpsvol->TPSVOL_hThreadFlush, LW_NULL);          /*  回写线程退出                */
        tpsFsUnmount(ptpsvol->TPSVOL_tpsFsVol);                         /*  卸载挂载的文件系统          */
    }
    if (iErrLevel > 2) {
        API_SemaphoreMDelete(&ptpsvol->TPSVOL_hVolLock);
    }
    if (iErrLevel > 1) {
        __blockIoDevDelete(iBlkdIndex);
    }

    __SHEAP_FREE(ptpsvol);

    _ErrorHandle(iErr);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_TpsFsDevDelete
** 功能描述: 删除一个 TPS 设备, 例如: API_TpsFsDevDelete("/mnt/ata0");
** 输　入  : pcName            设备名(设备挂接的节点地址)
** 输　出  : < 0 表示失败
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_TpsFsDevDelete (PCHAR   pcName)
{
    if (API_IosDevMatchFull(pcName)) {                                  /*  如果是设备, 这里就卸载设备  */
        return  (unlink(pcName));

    } else {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __tpsFsPathBuildLink
** 功能描述: 根据链接文件内容生成连接目标
** 输　入  : promfs           文件系统
**           promdnt          文件结构
**           pcDest           输出缓冲
**           stSize           输出缓冲大小
**           pcPrefix         前缀
**           pcTail           后缀
** 输　出  : < 0 错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsPathBuildLink (PTPS_FILE    ptpsfile,  PCHAR        pcDest,
                                  size_t       stSize,    PCHAR        pcPrefix,
                                  PCHAR        pcTail)
{
    CHAR        cLink[PATH_MAX + 1] = {0};
    TPS_SIZE_T  szRead;

    if (tpsFsRead(ptpsfile->TPSFIL_pinode, (PUCHAR)cLink,
                  0, PATH_MAX + 1, &szRead) == ERROR_NONE) {
        return  (_PathBuildLink(pcDest, stSize,
                                ptpsfile->TPSFIL_ptpsvol->TPSVOL_devhdrHdr.DEVHDR_pcName,
                                pcPrefix, cLink, pcTail));
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __tpsFsOpen
** 功能描述: TPS FS open 操作
** 输　入  : ptpsvol          卷控制块
**           pcName           文件名
**           iFlags           方式
**           iMode            方法
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LONG  __tpsFsOpen (PTPS_VOLUME     ptpsvol,
                          PCHAR           pcName,
                          INT             iFlags,
                          INT             iMode)
{
    REGISTER PTPS_FILE      ptpsfile;
    REGISTER ULONG          ulError;
             PLW_FD_NODE    pfdnode;
             BOOL           bIsNew;
             struct stat    statGet;
             PCHAR          pcTail = LW_NULL;
             errno_t        iErr   = ERROR_NONE;

    if (pcName == LW_NULL) {                                            /*  无文件名                    */
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }

    if (__blockIoDevFlag(ptpsvol->TPSVOL_iDrv) == O_RDONLY) {           /*  此卷写保护, 处于只读状态    */
        if (iFlags & (O_CREAT | O_TRUNC | O_RDWR | O_WRONLY)) {
            _ErrorHandle(ERROR_IO_WRITE_PROTECTED);
            return  (PX_ERROR);
        }
    }
    if (iFlags & O_CREAT) {
        if (ptpsvol->TPSVOL_iFlag == O_RDONLY) {
            _ErrorHandle(EROFS);                                        /*  只读文件系统                */
            return  (PX_ERROR);
        }

        if (__fsCheckFileName(pcName)) {
            _ErrorHandle(ENOENT);
            return  (PX_ERROR);
        }

        if (S_ISFIFO(iMode) ||
            S_ISBLK(iMode)  ||
            S_ISCHR(iMode)) {                                           /*  这里不过滤 socket 文件      */
            _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);                    /*  不支持以上这些格式          */
            return  (PX_ERROR);
        }
    }

    ptpsfile = (PTPS_FILE)__SHEAP_ALLOC(sizeof(TPS_FILE) +
                                        lib_strlen(pcName));            /*  创建文件内存                */
    if (ptpsfile == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_strcpy(ptpsfile->TPSFIL_cName, pcName);                         /*  记录文件名                  */

    ptpsfile->TPSFIL_ptpsvol = ptpsvol;                                 /*  记录卷信息                  */

    ulError = __TPS_FILE_LOCK(ptpsfile);
    if ((ptpsvol->TPSVAL_bValid == LW_FALSE) ||
        (ulError != ERROR_NONE)) {                                      /*  卷正在被卸载                */
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }

    if ((iMode & S_IFMT) == 0) {
        iMode |= S_IFREG;
    }

    iErr = tpsFsOpen(ptpsvol->TPSVOL_tpsFsVol, pcName, iFlags,
                     iMode, &pcTail, &ptpsfile->TPSFIL_pinode);
    if (iErr != ERROR_NONE) {
        if (__STR_IS_ROOT(pcName)) {                                    /*  未格式化设备根路径          */
            ptpsfile->TPSFIL_iFileType = __TPS_FILE_TYPE_DEV;           /*  仅仅打开设备, 未格式化      */
            tpsFsStat(ptpsvol->TPSVOL_tpsFsVol, ptpsfile->TPSFIL_pinode, &statGet);
            goto    __file_open_ok;
            
        } else {
            __TPS_FILE_UNLOCK(ptpsfile);
            __SHEAP_FREE(ptpsfile);
            _ErrorHandle(iErr);
            return  (PX_ERROR);
        }
    }

    tpsFsStat(ptpsvol->TPSVOL_tpsFsVol, ptpsfile->TPSFIL_pinode, &statGet);

    if (S_ISLNK(statGet.st_mode) && pcTail) {
        INT     iFollowLinkType;
        PCHAR   pcSymfile = pcTail;
        PCHAR   pcPrefix;

        if (pcSymfile != pcName) {
            pcSymfile--;
        }

        while (pcSymfile != pcName && *pcSymfile == PX_DIVIDER) {
            pcSymfile--;
        }

        while (pcSymfile != pcName && *pcSymfile != PX_DIVIDER) {
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

        if (__tpsFsPathBuildLink(ptpsfile, pcName, PATH_MAX + 1,
                                 pcPrefix, pcTail) == ERROR_NONE) {
                                                                        /*  构造链接目标                */
            tpsFsClose(ptpsfile->TPSFIL_pinode);
            __TPS_FILE_UNLOCK(ptpsfile);
            __SHEAP_FREE(ptpsfile);
            return  (iFollowLinkType);

        } else {                                                        /*  构造连接失败                */
            tpsFsClose(ptpsfile->TPSFIL_pinode);
            __TPS_FILE_UNLOCK(ptpsfile);
            __SHEAP_FREE(ptpsfile);
            _ErrorHandle(ERROR_IOS_FILE_NOT_SUP);
            return  (PX_ERROR);
        }

    } else if (S_ISDIR(statGet.st_mode)) {
        ptpsfile->TPSFIL_iFileType = __TPS_FILE_TYPE_DIR;

    } else {
        ptpsfile->TPSFIL_iFileType = __TPS_FILE_TYPE_NODE;
    }

__file_open_ok:
    pfdnode = API_IosFdNodeAdd(&ptpsvol->TPSVOL_plineFdNodeHeader,
                               statGet.st_dev,
                               statGet.st_ino,
                               iFlags, statGet.st_mode, 0,
                               0, statGet.st_size,
                               (PVOID)ptpsfile,
                               &bIsNew);                                /*  添加文件节点                */
    if (pfdnode == LW_NULL) {                                           /*  无法创建 fd_node 节点       */
        tpsFsClose(ptpsfile->TPSFIL_pinode);
        __TPS_FILE_UNLOCK(ptpsfile);
        __SHEAP_FREE(ptpsfile);
        return  (PX_ERROR);
    }

    LW_DEV_INC_USE_COUNT(&ptpsvol->TPSVOL_devhdrHdr);                   /*  更新计数器                  */

    if (bIsNew == LW_FALSE) {                                           /*  有重复打开                  */
        tpsFsClose(ptpsfile->TPSFIL_pinode);
        __TPS_FILE_UNLOCK(ptpsfile);
        __SHEAP_FREE(ptpsfile);
    
    } else {
        __TPS_FILE_UNLOCK(ptpsfile);
    }

    return  ((LONG)pfdnode);
}
/*********************************************************************************************************
** 函数名称: __tpsFsRemove
** 功能描述: TPS FS remove 操作
** 输　入  : ptpsvol          卷控制块
**           pcName           文件名
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsRemove (PTPS_VOLUME     ptpsvol,
                           PCHAR           pcName)
{
    PCHAR          pcTail  = LW_NULL;
    errno_t        iErr    = ERROR_NONE;
    PLW_BLK_DEV    pblkd;
    TPS_FILE       tpsfile;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }

    if (__STR_IS_ROOT(pcName)) {                                        /*  根目录或者设备文件          */
        /*
         *  因为回写线程里面会加锁，为防止和API_ThreadJoin产生死锁，退出回写线程在加锁之前执行
         */
        ptpsvol->TPSVOL_bNeedExit = LW_TRUE;
        API_ThreadWakeup(ptpsvol->TPSVOL_hThreadFlush);
        API_ThreadJoin(ptpsvol->TPSVOL_hThreadFlush, LW_NULL);          /*  回写线程退出                */

        if (__TPS_VOL_LOCK(ptpsvol) != ERROR_NONE) {
            _ErrorHandle(ENXIO);
            return  (PX_ERROR);                                         /*  正在被其他任务卸载          */
        }

        if (ptpsvol->TPSVAL_bValid == LW_FALSE) {
            __TPS_VOL_UNLOCK(ptpsvol);
            return  (ERROR_NONE);                                       /*  正在被其他任务卸载          */
        }

__re_umount_vol:
        if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)ptpsvol)) {              /*  检查是否有正在工作的文件    */
            if (!ptpsvol->TPSVOL_bForceDelete) {
                __TPS_VOL_UNLOCK(ptpsvol);
                _ErrorHandle(EBUSY);
                return  (PX_ERROR);                                     /*  有文件打开, 不能被卸载      */
            }

            ptpsvol->TPSVAL_bValid = LW_FALSE;                          /*  开始卸载卷, 文件再无法打开  */

            __TPS_VOL_UNLOCK(ptpsvol);

            _DebugHandle(__ERRORMESSAGE_LEVEL, "disk have open file.\r\n");
            iosDevFileAbnormal(&ptpsvol->TPSVOL_devhdrHdr);             /*  将所有相关文件设为异常状态  */

            __TPS_VOL_LOCK(ptpsvol);
            goto    __re_umount_vol;

        } else {
            ptpsvol->TPSVAL_bValid = LW_FALSE;                          /*  开始卸载卷, 文件再无法打开  */
        }

        /*
         *  不管出现什么物理错误, 必须可以卸载卷, 这里不再判断错误.
         */
        __blockIoDevIoctl(ptpsvol->TPSVOL_iDrv,
                          FIOFLUSH, 0);                                 /*  回写 DISK CACHE             */
        __blockIoDevIoctl(ptpsvol->TPSVOL_iDrv, LW_BLKD_CTRL_POWER,
                          LW_BLKD_POWER_OFF);                           /*  设备断电                    */
        __blockIoDevIoctl(ptpsvol->TPSVOL_iDrv, LW_BLKD_CTRL_EJECT,
                          0);                                           /*  将设备弹出                  */

        pblkd = __blockIoDevGet(ptpsvol->TPSVOL_iDrv);                  /*  获得块设备控制块            */
        if (pblkd) {
            __fsDiskLinkCounterDec(pblkd);                              /*  减少链接次数                */
        }

        iosDevDelete((LW_DEV_HDR *)ptpsvol);                            /*  IO 系统移除设备             */

        tpsFsUnmount(ptpsvol->TPSVOL_tpsFsVol);                         /*  卸载挂载的文件系统          */

        __blockIoDevIoctl(ptpsvol->TPSVOL_iDrv,
                          FIOUNMOUNT, 0);                               /*  执行底层脱离任务            */
        __blockIoDevDelete(ptpsvol->TPSVOL_iDrv);                       /*  在驱动表中移除              */

        API_SemaphoreMDelete(&ptpsvol->TPSVOL_hVolLock);                /*  删除卷锁                    */

        __SHEAP_FREE(ptpsvol);                                          /*  释放卷控制块                */

        _DebugHandle(__LOGMESSAGE_LEVEL, "disk unmount ok.\r\n");

        return  (ERROR_NONE);

    } else {                                                            /*  删除文件或目录              */
        if (__blockIoDevFlag(ptpsvol->TPSVOL_iDrv) == O_RDONLY) {       /*  此卷写保护, 处于只读状态    */
            _ErrorHandle(ERROR_IO_WRITE_PROTECTED);
            return  (PX_ERROR);
        }

        if (__fsCheckFileName(pcName) < 0) {                            /*  检查文件名是否合法          */
            _ErrorHandle(ENOENT);                                       /*  文件未找到                  */
            return  (PX_ERROR);
        }

        if (__TPS_VOL_LOCK(ptpsvol) != ERROR_NONE) {
            _ErrorHandle(ENXIO);
            return  (PX_ERROR);
        }

        if (ptpsvol->TPSVAL_bValid == LW_FALSE) {                       /*  卷正在被卸载                */
            __TPS_VOL_UNLOCK(ptpsvol);
            _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
            return  (PX_ERROR);
        }

        iErr = tpsFsOpen(ptpsvol->TPSVOL_tpsFsVol, pcName, 0,
                           0, &pcTail, &tpsfile.TPSFIL_pinode);
        if (iErr != ERROR_NONE) {
             __TPS_VOL_UNLOCK(ptpsvol);
             _ErrorHandle(iErr);
             return  (PX_ERROR);
        }
        tpsfile.TPSFIL_ptpsvol = ptpsvol;

        if (S_ISLNK(tpsFsGetmod(tpsfile.TPSFIL_pinode)) &&
            pcTail && lib_strlen(pcTail)) {                             /* 最后一级链接文件直接删除     */
            PCHAR   pcSymfile = pcTail;
            PCHAR   pcPrefix;

            if (pcSymfile != pcName) {
                pcSymfile--;
            }

            while (pcSymfile != pcName && *pcSymfile == PX_DIVIDER) {
                pcSymfile--;
            }

            while (pcSymfile != pcName && *pcSymfile != PX_DIVIDER) {
                pcSymfile--;
            }

            if (pcSymfile == pcName) {
                pcPrefix = LW_NULL;                                     /*  没有前缀                    */

            } else {
                pcPrefix = pcName;
                *pcSymfile = PX_EOS;
            }

            if (pcTail && lib_strlen(pcTail)) {
                if (__tpsFsPathBuildLink(&tpsfile, pcName, PATH_MAX + 1,
                                         pcPrefix, pcTail) == ERROR_NONE) {
                                                                        /*  构造链接目标                */
                    tpsFsClose(tpsfile.TPSFIL_pinode);
                    __TPS_VOL_UNLOCK(ptpsvol);
                    return  (FOLLOW_LINK_TAIL);
                }
            }
        }

        if (S_ISDIR(tpsFsGetmod(tpsfile.TPSFIL_pinode))) {              /* 目录非空不允许删除           */
            PTPS_ENTRY    pentry;

            iErr = tpsFsReadDir(tpsfile.TPSFIL_pinode, LW_TRUE, MAX_BLK_NUM, &pentry);
            if (iErr != ENOENT) {
                if (pentry) {
                    tpsFsEntryFree(pentry);
                    iErr = ENOTEMPTY;
                }
                
                tpsFsClose(tpsfile.TPSFIL_pinode);
                __TPS_VOL_UNLOCK(ptpsvol);
                _ErrorHandle(iErr);
                return  (PX_ERROR);
            }
        }

        tpsFsClose(tpsfile.TPSFIL_pinode);

        iErr = tpsFsRemove(ptpsvol->TPSVOL_tpsFsVol, pcName);           /*  执行删除操作                */

        __TPS_VOL_UNLOCK(ptpsvol);

        if (iErr != ERROR_NONE) {
            _ErrorHandle(iErr);
            return  (PX_ERROR);                                         /*  删除失败                    */
        
        } else {
            return  (ERROR_NONE);                                       /*  删除正确                    */
        }
    }
}
/*********************************************************************************************************
** 函数名称: __tpsFsClose
** 功能描述: TPS FS close 操作
** 输　入  : pfdentry         文件控制块
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsClose (PLW_FD_ENTRY    pfdentry)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE     ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    PTPS_VOLUME   ptpsvol  = ptpsfile->TPSFIL_ptpsvol;
    BOOL          bFree    = LW_FALSE;
    BOOL          bRemove  = LW_FALSE;

    if (ptpsfile) {
        if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
            _ErrorHandle(ENXIO);                                        /*  设备出错                    */
            return  (PX_ERROR);
        }

        if (API_IosFdNodeDec(&ptpsvol->TPSVOL_plineFdNodeHeader,
                             pfdnode, &bRemove) == 0) {                 /*  fd_node 是否完全释放        */
            if (ptpsfile->TPSFIL_pinode) {
                if (bRemove && S_ISDIR(tpsFsGetmod(ptpsfile->TPSFIL_pinode))) {
                    PTPS_ENTRY    pentry;
                    
                    if (tpsFsReadDir(ptpsfile->TPSFIL_pinode, LW_TRUE, MAX_BLK_NUM, &pentry) != ENOENT) {
                        if (pentry) {
                            tpsFsEntryFree(pentry);
                        }

                        bRemove = LW_FALSE;
                    }
                }
                tpsFsClose(ptpsfile->TPSFIL_pinode);
                bFree = LW_TRUE;
            }
        }

        LW_DEV_DEC_USE_COUNT(&ptpsvol->TPSVOL_devhdrHdr);               /*  更新计数器                  */

        if (bRemove) {
            tpsFsRemove(ptpsvol->TPSVOL_tpsFsVol, ptpsfile->TPSFIL_cName);
        }

        __TPS_FILE_UNLOCK(ptpsfile);

        if (bFree) {
            __SHEAP_FREE(ptpsfile);                                     /*  释放内存                    */
        }

        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __tpsFsRead
** 功能描述: TPS FS read 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __tpsFsRead (PLW_FD_ENTRY   pfdentry,
                             PCHAR          pcBuffer,
                             size_t         stMaxBytes)
{
    PLW_FD_NODE   pfdnode    = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE     ptpsfile   = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    TPS_SIZE_T    szReadNum  = 0;
    errno_t       iErr       = ERROR_NONE;

    if (!pcBuffer || !stMaxBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (ptpsfile->TPSFIL_iFileType != __TPS_FILE_TYPE_NODE) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    iErr = tpsFsRead(ptpsfile->TPSFIL_pinode, (PUCHAR)pcBuffer,
                     pfdentry->FDENTRY_oftPtr, stMaxBytes, &szReadNum);
    if ((iErr == ERROR_NONE) && (szReadNum > 0)) {
        pfdentry->FDENTRY_oftPtr += (off_t)szReadNum;                   /*  更新文件指针                */
    }

    __TPS_FILE_UNLOCK(ptpsfile);

    _ErrorHandle(iErr);
    return  ((ssize_t)szReadNum);
}
/*********************************************************************************************************
** 函数名称: __tpsFsPRead
** 功能描述: TPS FS pread 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
**           oftPos           位置
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __tpsFsPRead (PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer,
                              size_t        stMaxBytes,
                              off_t         oftPos)
{
    PLW_FD_NODE   pfdnode       = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE     ptpsfile      = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    TPS_SIZE_T    szReadNum     = 0;
    errno_t       iErr          = ERROR_NONE;

    if (!pcBuffer || !stMaxBytes || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }

    if (ptpsfile->TPSFIL_iFileType != __TPS_FILE_TYPE_NODE) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    iErr = tpsFsRead(ptpsfile->TPSFIL_pinode, (PUCHAR)pcBuffer,
                     oftPos, stMaxBytes, &szReadNum);

    __TPS_FILE_UNLOCK(ptpsfile);

    _ErrorHandle(iErr);
    return  ((ssize_t)szReadNum);
}
/*********************************************************************************************************
** 函数名称: __tpsFsWrite
** 功能描述: TPS FS write 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         缓冲区
**           stNBytes         需要写入的数据
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __tpsFsWrite (PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer,
                              size_t        stNBytes)
{
    PLW_FD_NODE   pfdnode       = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE     ptpsfile      = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    TPS_SIZE_T    szWriteNum    = 0;
    errno_t       iErr          = ERROR_NONE;

    if (!pcBuffer || !stNBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }

    if (ptpsfile->TPSFIL_iFileType != __TPS_FILE_TYPE_NODE) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (pfdentry->FDENTRY_iFlag & O_APPEND) {                           /*  追加模式                    */
        pfdentry->FDENTRY_oftPtr = pfdnode->FDNODE_oftSize;             /*  移动读写指针到末尾          */
    }

    if (tpsFsGetSize(ptpsfile->TPSFIL_pinode) < pfdentry->FDENTRY_oftPtr) {
        UINT uiBufSize = ptpsfile->TPSFIL_pinode->IND_psb->SB_uiBlkSize;/*  自动扩展文件                */
        size_t szWrite;
        PUCHAR pucZoreBuf = (PUCHAR)__SHEAP_ALLOC(uiBufSize);
        if (pucZoreBuf == LW_NULL) {
            __TPS_FILE_UNLOCK(ptpsfile);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }

        lib_bzero(pucZoreBuf, uiBufSize);
        while (tpsFsGetSize(ptpsfile->TPSFIL_pinode) < pfdentry->FDENTRY_oftPtr) {
            szWrite = (size_t)min(uiBufSize, pfdentry->FDENTRY_oftPtr - tpsFsGetSize(ptpsfile->TPSFIL_pinode));
            if (tpsFsWrite(ptpsfile->TPSFIL_pinode, pucZoreBuf,
                           tpsFsGetSize(ptpsfile->TPSFIL_pinode),
                           szWrite, &szWriteNum) != ERROR_NONE) {
                break;
            }
        }

        __SHEAP_FREE(pucZoreBuf);
        if (tpsFsGetSize(ptpsfile->TPSFIL_pinode) < pfdentry->FDENTRY_oftPtr) {
            __TPS_FILE_UNLOCK(ptpsfile);
            _ErrorHandle(EFBIG);
            return  (PX_ERROR);
        }
    }

    iErr = tpsFsWrite(ptpsfile->TPSFIL_pinode, (PUCHAR)pcBuffer,
                      pfdentry->FDENTRY_oftPtr, stNBytes, &szWriteNum);
    if ((iErr == ERROR_NONE) && (szWriteNum > 0)) {
        pfdentry->FDENTRY_oftPtr += (off_t)szWriteNum;                  /*  更新文件指针                */
        pfdnode->FDNODE_oftSize   = tpsFsGetSize(ptpsfile->TPSFIL_pinode);
    }

    __TPS_FILE_UNLOCK(ptpsfile);

    _ErrorHandle(iErr);
    return  ((ssize_t)szWriteNum);
}
/*********************************************************************************************************
** 函数名称: __tpsFsPWrite
** 功能描述: TPS FS pwrite 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         缓冲区
**           stNBytes         需要写入的数据
**           oftPos           位置
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __tpsFsPWrite (PLW_FD_ENTRY  pfdentry,
                               PCHAR         pcBuffer,
                               size_t        stNBytes,
                               off_t         oftPos)
{
    PLW_FD_NODE   pfdnode       = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE     ptpsfile      = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    TPS_SIZE_T    szWriteNum    = 0;
    errno_t       iErr          = ERROR_NONE;

    if (!pcBuffer || !stNBytes || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }

    if (ptpsfile->TPSFIL_iFileType != __TPS_FILE_TYPE_NODE) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    if (tpsFsGetSize(ptpsfile->TPSFIL_pinode) < oftPos) {
        UINT uiBufSize = ptpsfile->TPSFIL_pinode->IND_psb->SB_uiBlkSize;/*  自动扩展文件                */
        size_t szWrite;
        PUCHAR pucZoreBuf = (PUCHAR)__SHEAP_ALLOC(uiBufSize);
        if (pucZoreBuf == LW_NULL) {
            __TPS_FILE_UNLOCK(ptpsfile);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }

        lib_bzero(pucZoreBuf, uiBufSize);
        while (tpsFsGetSize(ptpsfile->TPSFIL_pinode) < oftPos) {
            szWrite = (size_t)min(uiBufSize, oftPos - tpsFsGetSize(ptpsfile->TPSFIL_pinode));
            if (tpsFsWrite(ptpsfile->TPSFIL_pinode, pucZoreBuf,
                           tpsFsGetSize(ptpsfile->TPSFIL_pinode),
                           szWrite, &szWriteNum) != ERROR_NONE) {
                break;
            }
        }

        __SHEAP_FREE(pucZoreBuf);
        if (tpsFsGetSize(ptpsfile->TPSFIL_pinode) < oftPos) {
            __TPS_FILE_UNLOCK(ptpsfile);
            _ErrorHandle(EFBIG);
            return  (PX_ERROR);
        }
    }

    iErr = tpsFsWrite(ptpsfile->TPSFIL_pinode, (PUCHAR)pcBuffer,
                      oftPos, stNBytes, &szWriteNum);
    if ((iErr == ERROR_NONE) && (szWriteNum > 0)) {
        pfdnode->FDNODE_oftSize = tpsFsGetSize(ptpsfile->TPSFIL_pinode);
    }

    __TPS_FILE_UNLOCK(ptpsfile);

    _ErrorHandle(iErr);
    return  ((ssize_t)szWriteNum);
}

/*********************************************************************************************************
** 函数名称: __tpsFsSeek
** 功能描述: TPS FS 文件定位
** 输　入  : pfdentry            文件控制块
**           oftPos              定位点
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsSeek (PLW_FD_ENTRY  pfdentry, off_t  oftPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE      ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (ptpsfile->TPSFIL_iFileType != __TPS_FILE_TYPE_NODE) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    pfdentry->FDENTRY_oftPtr = oftPos;

    __TPS_FILE_UNLOCK(ptpsfile);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsReadDir
** 功能描述: TPS FS 获得指定目录信息
** 输　入  : pfdentry            文件控制块
**           dir                 目录结构
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsReadDir (PLW_FD_ENTRY  pfdentry, DIR  *dir)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE      ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    PTPS_ENTRY     pentry;
    INT            iErr     = ERROR_NONE;
    TPS_OFF_T      off      = 0;
    BOOL           bHash    = LW_TRUE;

    if (!dir) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (ptpsfile->TPSFIL_iFileType == __TPS_FILE_TYPE_NODE) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }

    /*
     *  dir_pos转bHash + off
     */
    if (dir->dir_pos == 0) {
        off   = MAX_BLK_NUM;
        bHash = LW_TRUE;
    
    } else {
        bHash = (dir->dir_pos & 0x80000000) ? LW_TRUE : LW_FALSE;
        
#if LW_CFG_CPU_WORD_LENGHT == 64
        off = (TPS_OFF_T)DIR_RESV_DATA_PV0(dir);
#else
        off = (UINT)DIR_RESV_DATA_PV1(dir);
        off <<= 32;
        off += (UINT)DIR_RESV_DATA_PV0(dir);
#endif
    }

    tpsFsReadDir(ptpsfile->TPSFIL_pinode, bHash, off, &pentry);
    if (!pentry) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }

    /*
     *  bHash + off转dir_pos
     */
    dir->dir_pos++;
    dir->dir_pos &= 0x7FFFFFFF;
    if (pentry->ENTRY_bInHash) {
        dir->dir_pos |= 0x80000000;
        if (pentry->ENTRY_offset > 0) {
            off = pentry->ENTRY_offset - 1;
        }
    
    } else {
        off = pentry->ENTRY_offset + pentry->ENTRY_uiLen;
    }

#if LW_CFG_CPU_WORD_LENGHT == 64
    DIR_RESV_DATA_PV0(dir) = (PVOID)off;
#else
    DIR_RESV_DATA_PV0(dir) = (PVOID)(UINT)off;
    DIR_RESV_DATA_PV1(dir) = (PVOID)(UINT)(off >> 32);
#endif

    dir->dir_dirent.d_type = IFTODT(tpsFsGetmod(pentry->ENTRY_pinode));
    dir->dir_dirent.d_shortname[0] = PX_EOS;
    lib_strlcpy(dir->dir_dirent.d_name,
                pentry->ENTRY_pcName,
                sizeof(dir->dir_dirent.d_name));

    tpsFsEntryFree(pentry);
    __TPS_FILE_UNLOCK(ptpsfile);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ramFsLStat
** 功能描述: ramFs stat 操作
** 输　入  : ptpsfs           tpsfs 文件系统
**           pcName           文件名
**           pstat            文件状态
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsLStat (PTPS_VOLUME  ptpsfs, PCHAR  pcName, struct stat *pstat)
{
    PTPS_INODE          ptpsinode;
    PTPS_SUPER_BLOCK    psb     = ptpsfs->TPSVOL_tpsFsVol;
    errno_t             iErr    = ERROR_NONE;

    if (!pcName || !pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_VOL_LOCK(ptpsfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    iErr = tpsFsOpen(ptpsfs->TPSVOL_tpsFsVol, pcName,
                     0, 0, LW_NULL, &ptpsinode);
    if (iErr == ERROR_NONE) {
        tpsFsStat(LW_NULL, ptpsinode, pstat);

    } else if (__STR_IS_ROOT(pcName)) {
        tpsFsStat(psb, LW_NULL, pstat);

    } else {
        __TPS_VOL_UNLOCK(ptpsfs);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }

    tpsFsClose(ptpsinode);
    __TPS_VOL_UNLOCK(ptpsfs);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsStatGet
** 功能描述: TPS FS 获得文件状态及属性
** 输　入  : pfdentry            文件控制块
**           pstat               stat 结构
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsStatGet (PLW_FD_ENTRY  pfdentry, struct stat *pstat)
{
    PLW_FD_NODE         pfdnode   = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE           ptpsfile  = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    PTPS_INODE          ptpsinode = ptpsfile->TPSFIL_pinode;
    PTPS_SUPER_BLOCK    psb       = ptpsfile->TPSFIL_ptpsvol->TPSVOL_tpsFsVol;

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }

    if (__STR_IS_ROOT(ptpsfile->TPSFIL_cName)) {                        /*  为根目录                    */
        tpsFsStat(psb, LW_NULL, pstat);

    } else {
        tpsFsStat(LW_NULL, ptpsinode, pstat);
    }

    __TPS_FILE_UNLOCK(ptpsfile);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsFormat
** 功能描述: TPS FS 格式化媒质
** 输　入  : pfdentry            文件控制块
**           lArg                参数
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsFormat (PLW_FD_ENTRY  pfdentry, LONG  lArg)
{
    PLW_FD_NODE          pfdnode         = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE            ptpsfile        = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    PTPS_VOLUME          ptpsvol         = ptpsfile->TPSFIL_ptpsvol;
    UINT                 uiMountFlag     = 0;
    errno_t              iErr            = ERROR_NONE;
    PLW_BLK_DEV          pblkd;

    if (!__STR_IS_ROOT(ptpsfile->TPSFIL_cName)) {                       /*  检查是否为设备文件          */
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }

    if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)ptpsvol) > 1) {              /*  检查是否有正在工作的文件    */
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(ERROR_IOS_TOO_MANY_OPEN_FILES);                    /*  有其他文件打开              */
        return  (PX_ERROR);
    }
    
    if (ptpsvol->TPSVOL_tpsFsVol) {
        tpsFsUnmount(ptpsvol->TPSVOL_tpsFsVol);                         /*  卸载挂载的文件系统          */
        ptpsvol->TPSVOL_tpsFsVol = LW_NULL;
    }

    (VOID)__blockIoDevIoctl(ptpsvol->TPSVOL_iDrv, FIOCANCEL, 0);        /*  CACHE 停止 (不回写数据)     */

    iErr = __blockIoDevIoctl(ptpsvol->TPSVOL_iDrv,
                             FIODISKFORMAT,
                             lArg);                                     /*  底层格式化                  */
    if (iErr < 0) {
        if (__blockIoDevIsLogic(ptpsvol->TPSVOL_iDrv)) {
            iErr = tpsFsFormat(&ptpsvol->TPSVOL_dev, TPS_MIN_BLK_SIZE); /*  此磁盘为逻辑磁盘不需要分区表*/
        
        } else {
            iErr = ENXIO;
        }
    }
    
    if (iErr) {                                                         /*  格式化失败                  */
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }

    pblkd = __blockIoDevGet(ptpsvol->TPSVOL_iDrv);
    if (pblkd) {
        if (pblkd->BLKD_iFlag == O_RDONLY) {                            /*  访问模式转换成tpsfs模式     */
            uiMountFlag = TPS_MOUNT_FLAG_READ;

        } else if (pblkd->BLKD_iFlag == O_WRONLY) {
            uiMountFlag = TPS_MOUNT_FLAG_WRITE;

        } else {
            uiMountFlag = TPS_MOUNT_FLAG_WRITE | TPS_MOUNT_FLAG_READ;
        }

        iErr = tpsFsMount(&ptpsvol->TPSVOL_dev, uiMountFlag, &ptpsvol->TPSVOL_tpsFsVol);
        if (iErr != ERROR_NONE) {                                       /*  挂载文件系统                */
            __TPS_FILE_UNLOCK(ptpsfile);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "mount tpsfs failed.\r\n");
            _ErrorHandle(iErr);
            return  (PX_ERROR);
        }
    }

    __TPS_FILE_UNLOCK(ptpsfile);

    if (iErr) {                                                         /*  返回                        */
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __tpsFsRename
** 功能描述: TPS FS 更名
** 输　入  : pfdentry            文件控制块
**           pcNewName           新名字
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
** 注  意  : 为了减小内存分片的产生, 将文件名和控制结构分配在同一内存分段, 这时无法更改 ptpsfile 保存的
             文件名, 所以文件名完全错误, 这时, 此文件不能再次操作, 所以, 用户必须调用 rename() 函数来完成
             此操作, 不能独立调用 ioctl() 来操作.
*********************************************************************************************************/
static INT  __tpsFsRename (PLW_FD_ENTRY  pfdentry, PCHAR  pcNewName)
{
    errno_t                 iErr    = ERROR_NONE;

             CHAR           cNewPath[PATH_MAX + 1];
    REGISTER PCHAR          pcNewPath = &cNewPath[0];
             PLW_FD_NODE    pfdnode   = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
             PTPS_FILE      ptpsfile   = (PTPS_FILE)pfdnode->FDNODE_pvFile;
             PTPS_VOLUME    ptpsvolNew;

    if (__STR_IS_ROOT(ptpsfile->TPSFIL_cName)) {                        /*  检查是否为设备文件          */
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                         /*  不支持设备重命名            */
        return  (PX_ERROR);
    }
    if (pcNewName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    if (__STR_IS_ROOT(pcNewName)) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (ptpsfile->TPSFIL_iFileType == __TPS_FILE_TYPE_DEV) {            /*  设备本身不能改名            */
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    if (ioFullFileNameGet(pcNewName,
                          (LW_DEV_HDR **)&ptpsvolNew,
                          cNewPath) != ERROR_NONE) {                    /*  获得新目录路径              */
        __TPS_FILE_UNLOCK(ptpsfile);
        return  (PX_ERROR);
    }
    
    if (ptpsvolNew != ptpsfile->TPSFIL_ptpsvol) {                       /*  必须为同一个卷              */
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    
    if (cNewPath[0] == PX_DIVIDER) {                                    /*  tpsFs 文件系统 rename 的第  */
        pcNewPath++;                                                    /*  2 个参数不能以'/'为起始字符 */
    }
    
    if (ptpsfile->TPSFIL_iFileType == __TPS_FILE_TYPE_DIR) {
        if (_PathMoveCheck(ptpsfile->TPSFIL_cName, pcNewPath)) {
            __TPS_FILE_UNLOCK(ptpsfile);
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    }
    
    iErr = tpsFsMove(ptpsfile->TPSFIL_ptpsvol->TPSVOL_tpsFsVol,
                     ptpsfile->TPSFIL_cName, pcNewPath);
    if (iErr) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(iErr);                                             /*  重命名出错                  */
        return  (PX_ERROR);
    }
    
    __TPS_FILE_UNLOCK(ptpsfile);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsWhere
** 功能描述: TPS FS 获得文件当前读写指针位置 (使用参数作为返回值, 与 FIOWHERE 的要求稍有不同)
** 输　入  : pfdentry            文件控制块
**           poftPos             读写指针位置
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsWhere (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE      ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (ptpsfile->TPSFIL_iFileType == __TPS_FILE_TYPE_NODE) {
        *poftPos = pfdentry->FDENTRY_oftPtr;
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    
    __TPS_FILE_UNLOCK(ptpsfile);

    _ErrorHandle(ulError);
    return  (iError);
 }
/*********************************************************************************************************
** 函数名称: __tpsFsNRead
** 功能描述: TPS FS 获得文件剩余的信息量
** 输　入  : pfdentry            文件控制块
**           piPos               剩余数据量
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsNRead (PLW_FD_ENTRY  pfdentry, INT  *piPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE      ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;

    if (piPos == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (ptpsfile->TPSFIL_iFileType == __TPS_FILE_TYPE_NODE) {
        *piPos = (INT)(pfdnode->FDNODE_oftSize - pfdentry->FDENTRY_oftPtr);
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    
    __TPS_FILE_UNLOCK(ptpsfile);

    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __tpsFsNRead64
** 功能描述: TPS FS 获得文件剩余的信息量
** 输　入  : pfdentry            文件控制块
**           poftPos             剩余数据量
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsNRead64 (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE      ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }

    if (ptpsfile->TPSFIL_iFileType == __TPS_FILE_TYPE_NODE) {
        *poftPos = pfdnode->FDNODE_oftSize - pfdentry->FDENTRY_oftPtr;
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }

    __TPS_FILE_UNLOCK(ptpsfile);

    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __tpsFsTruncate
** 功能描述: TPS FS 设置文件大小
** 输　入  : pfdentry            文件控制块
**           oftSize             文件大小
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsTruncate (PLW_FD_ENTRY  pfdentry, off_t  oftSize)
{
    PLW_FD_NODE         pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE           ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    INT                 iError   = ERROR_NONE;
    errno_t             iErr     = ERROR_NONE;

    if (oftSize < 0) {                                                  /*  TPS 文件必须在 4GB 内       */
        _ErrorHandle(EOVERFLOW);
        return  (PX_ERROR);

    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (ptpsfile->TPSFIL_iFileType == __TPS_FILE_TYPE_NODE) {
        if (oftSize > tpsFsGetSize(ptpsfile->TPSFIL_pinode)) {          /*  自动扩展文件                */
            UINT        uiBufSize  = ptpsfile->TPSFIL_pinode->IND_psb->SB_uiBlkSize;
            size_t      szWrite;
            TPS_SIZE_T  szWriteNum;
            PUCHAR      pucZoreBuf = (PUCHAR)__SHEAP_ALLOC(uiBufSize);

            if (pucZoreBuf == LW_NULL) {
                __TPS_FILE_UNLOCK(ptpsfile);
                _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
                _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
                return  (PX_ERROR);
            }

            while (tpsFsGetSize(ptpsfile->TPSFIL_pinode) < oftSize) {
                szWrite = (size_t)min(uiBufSize, oftSize -
                                                 tpsFsGetSize(ptpsfile->TPSFIL_pinode));
                if (tpsFsWrite(ptpsfile->TPSFIL_pinode, pucZoreBuf,
                               tpsFsGetSize(ptpsfile->TPSFIL_pinode),
                               szWrite, &szWriteNum) != ERROR_NONE) {
                    break;
                }
            }

            __SHEAP_FREE(pucZoreBuf);
            if (tpsFsGetSize(ptpsfile->TPSFIL_pinode) < oftSize) {
                __TPS_FILE_UNLOCK(ptpsfile);
                _ErrorHandle(EFBIG);
                return  (PX_ERROR);
            }

            __TPS_FILE_UNLOCK(ptpsfile);
            return  (ERROR_NONE);
        }

        iErr = tpsFsTrunc(ptpsfile->TPSFIL_pinode, oftSize);
        pfdnode->FDNODE_oftSize = tpsFsGetSize(ptpsfile->TPSFIL_pinode);
        iError = (iErr == ERROR_NONE ? ERROR_NONE : PX_ERROR);
    } else {
        iErr    = EISDIR;
        iError  = PX_ERROR;
    }
    
    __TPS_FILE_UNLOCK(ptpsfile);

    _ErrorHandle(iErr);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __tpsFsSync
** 功能描述: TPS FS 将文件缓存写入磁盘
** 输　入  : pfdentry            文件控制块
**           bFlushCache         是否同时清空 CACHE
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsSync (PLW_FD_ENTRY  pfdentry, BOOL  bFlushCache)
{
    PLW_FD_NODE         pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE           ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    INT                 iError   = ERROR_NONE;
    errno_t             iErr     = ERROR_NONE;

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    /*
     *  只处理文件回写，回写整个分区由上层处理
     */
    if (ptpsfile->TPSFIL_iFileType == __TPS_FILE_TYPE_NODE) {
        iErr = tpsFsSync(ptpsfile->TPSFIL_pinode);
        if (iErr != ERROR_NONE) {
            __TPS_FILE_UNLOCK(ptpsfile);
            _ErrorHandle(iErr);                                         /*  同步文件系统出错            */
            return  (PX_ERROR);
        }
    }

    if (bFlushCache) {
        iError = __blockIoDevIoctl(ptpsfile->TPSFIL_ptpsvol->TPSVOL_iDrv,
                                   FIOSYNC, 0);                         /*  清除 CACHE 回写磁盘         */
        if (iError < 0) {
            iErr = ERROR_IO_DEVICE_ERROR;                               /*  设备出错, 无法清空          */
        }
    }
    __TPS_FILE_UNLOCK(ptpsfile);

    _ErrorHandle(iErr);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __tpsFsChmod
** 功能描述: TPS FS 设置文件属性
** 输　入  : pfdentry            文件控制块
**           iMode               新的 mode
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsChmod (PLW_FD_ENTRY  pfdentry, INT  iMode)
{
    PLW_FD_NODE         pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE           ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    errno_t             iErr     = ERROR_NONE;

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }

    iErr = tpsFsChmod(ptpsfile->TPSFIL_pinode, iMode);
    if (iErr != ERROR_NONE) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }

    __TPS_FILE_UNLOCK(ptpsfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __fatFsTimeset
** 功能描述: TPS FS 设置文件时间
** 输　入  : pfdentry            文件控制块
**           utim                utimbuf 结构
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
** 注  意  : 目前此函数仅设置修改时间.
*********************************************************************************************************/
static INT  __tpsFsTimeset (PLW_FD_ENTRY  pfdentry, struct utimbuf  *utim)
{
    PLW_FD_NODE         pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE           ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    errno_t             iErr     = ERROR_NONE;

    if (__STR_IS_ROOT(ptpsfile->TPSFIL_cName)) {                        /*  检查是否为设备文件          */
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                         /*  不支持设备重命名            */
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }

    iErr = tpsFsChtime(ptpsfile->TPSFIL_pinode, utim);
    if (iErr != ERROR_NONE) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }

    __TPS_FILE_UNLOCK(ptpsfile);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsChown
** 功能描述: ramfs chown 操作
** 输　入  : pfdentry            文件控制块
**           pusr                新的所属用户
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsChown (PLW_FD_ENTRY  pfdentry, LW_IO_USR  *pusr)
{
    PLW_FD_NODE         pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE           ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    errno_t             iErr     = ERROR_NONE;

    if (!pusr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }

    iErr = tpsFsChown(ptpsfile->TPSFIL_pinode, pusr->IOU_uid, pusr->IOU_gid);
    if (iErr != ERROR_NONE) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }

    __TPS_FILE_UNLOCK(ptpsfile);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsStatfsGet
** 功能描述: TPS FS 获得文件系统状态及属性
** 输　入  : pfdentry            文件控制块
**           pstatfs             statfs 结构
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsStatfsGet (PLW_FD_ENTRY  pfdentry, struct statfs *pstatfs)
{
    PLW_FD_NODE         pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE           ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    PTPS_SUPER_BLOCK    psb      = ptpsfile->TPSFIL_ptpsvol->TPSVOL_tpsFsVol;

    if (!pstatfs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }

    lib_bzero(pstatfs, sizeof(struct statfs));

    tpsFsStatfs(psb, pstatfs);

    if (ptpsfile->TPSFIL_ptpsvol->TPSVOL_iFlag == O_RDONLY) {
        pstatfs->f_flag |= ST_RDONLY;
    }

    __TPS_FILE_UNLOCK(ptpsfile);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsSymlink
** 功能描述: tpsFs 创建符号链接文件
** 输　入  : ptpsfs              tpsfs 文件系统
**           pcName              链接原始文件名
**           pcLinkDst           链接目标文件名
**           stMaxSize           缓冲大小
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsSymlink (PTPS_VOLUME   ptpsfs,
                            PCHAR         pcName,
                            CPCHAR        pcLinkDst)
{
    errno_t             iErr     = ERROR_NONE;
    PTPS_INODE          pinode;
    TPS_SIZE_T          szRead;

    if (!pcName || !pcLinkDst) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__fsCheckFileName(pcName)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_VOL_LOCK(ptpsfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    iErr = tpsFsOpen(ptpsfs->TPSVOL_tpsFsVol, pcName,
                     O_CREAT | O_EXCL, S_IFLNK | DEFAULT_SYMLINK_PERM,
                     LW_NULL, &pinode);
    if (iErr != ERROR_NONE) {
        __TPS_VOL_UNLOCK(ptpsfs);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }

    iErr = tpsFsWrite(pinode, (PUCHAR)pcLinkDst, 0, lib_strlen(pcLinkDst), &szRead);
    if ((iErr != ERROR_NONE) || (szRead < lib_strlen(pcLinkDst))) {
        __TPS_VOL_UNLOCK(ptpsfs);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }
    
    tpsFsClose(pinode);

    __TPS_VOL_UNLOCK(ptpsfs);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsReadlink
** 功能描述: tpsFs 读取符号链接文件内容
** 输　入  : ptpsfs              tpsfs 文件系统
**           pcName              链接原始文件名
**           pcLinkDst           链接目标文件名
**           stMaxSize           缓冲大小
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t __tpsFsReadlink (PTPS_VOLUME   ptpsfs,
                                PCHAR         pcName,
                                PCHAR         pcLinkDst,
                                size_t        stMaxSize)
{
    errno_t     iErr     = ERROR_NONE;
    PTPS_INODE  pinode;
    TPS_SIZE_T  szLen;

    if (!pcName || !pcLinkDst || !stMaxSize) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_VOL_LOCK(ptpsfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    iErr = tpsFsOpen(ptpsfs->TPSVOL_tpsFsVol, pcName, 0, 0, LW_NULL, &pinode);
    if ((iErr != ERROR_NONE) || !S_ISLNK(tpsFsGetmod(pinode))) {
        __TPS_VOL_UNLOCK(ptpsfs);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }

    iErr = tpsFsRead(pinode, (PUCHAR)pcLinkDst, 0, stMaxSize, &szLen);
    if (iErr != ERROR_NONE) {
        tpsFsClose(pinode);
        __TPS_VOL_UNLOCK(ptpsfs);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }

    tpsFsClose(pinode);

    __TPS_VOL_UNLOCK(ptpsfs);

    return  ((ssize_t)szLen);
}
/*********************************************************************************************************
** 函数名称: __tpsFsIoctl
** 功能描述: TPS FS ioctl 操作
** 输　入  : pfdentry            文件控制块
**           request,            命令
**           arg                 命令参数
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsIoctl (PLW_FD_ENTRY  pfdentry,
                          INT           iRequest,
                          LONG          lArg)
{
    PLW_FD_NODE    pfdnode      = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE      ptpsfile     = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    PTPS_VOLUME    ptpsvol      = ptpsfile->TPSFIL_ptpsvol;
    PTPS_INODE     ptpsinode    = ptpsfile->TPSFIL_pinode;
    off_t          oftTemp  = 0;                                        /*  临时变量                    */
    INT            iError;

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
        if (ptpsvol->TPSVOL_iFlag == O_RDONLY) {
            _ErrorHandle(EROFS);
            return  (PX_ERROR);
        }
    }

    switch (iRequest) {                                                 /*  只读文件系统判断            */

    case FIORENAME:
    case FIOTIMESET:
    case FIOCHMOD:
        if (tpsFsGetmod(ptpsinode) == O_RDONLY) {
            _ErrorHandle(EROFS);
            return  (PX_ERROR);
        }
    }

    switch (iRequest) {

    case FIODISKFORMAT:                                                 /*  卷格式化                    */
        return  (__tpsFsFormat(pfdentry, lArg));
    
    case FIODISKINIT:                                                   /*  磁盘初始化                  */
        return  (__blockIoDevIoctl(ptpsvol->TPSVOL_iDrv, FIODISKINIT, lArg));

    case FIORENAME:                                                     /*  文件重命名                  */
        return  (__tpsFsRename(pfdentry, (PCHAR)lArg));

    /*
     *  FIOSEEK, FIOWHERE is 64 bit operate.
     */
    case FIOSEEK:                                                       /*  文件重定位                  */
        oftTemp = *(off_t *)lArg;
        return  (__tpsFsSeek(pfdentry, oftTemp));

    case FIOWHERE:                                                      /*  获得文件当前读写指针        */
        iError = __tpsFsWhere(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }

    case FIONREAD:                                                      /*  获得文件剩余字节数          */
        return  (__tpsFsNRead(pfdentry, (INT *)lArg));

    case FIONREAD64:                                                    /*  获得文件剩余字节数          */
        iError = __tpsFsNRead64(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }

    case FIOFSTATGET:                                                   /*  获得文件状态                */
        return  (__tpsFsStatGet(pfdentry, (struct stat *)lArg));
    
    case FIOFSTATFSGET:                                                 /*  获得文件系统状态            */
        return  (__tpsFsStatfsGet(pfdentry, (struct statfs *)lArg));

    case FIOREADDIR:                                                    /*  获取一个目录信息            */
        return  (__tpsFsReadDir(pfdentry, (DIR *)lArg));

    case FIOTIMESET:                                                    /*  设置文件时间                */
        return  (__tpsFsTimeset(pfdentry, (struct utimbuf *)lArg));

    /*
     *  FIOTRUNC is 64 bit operate.
     */
    case FIOTRUNC:                                                      /*  改变文件大小                */
        oftTemp = *(off_t *)lArg;
        return  (__tpsFsTruncate(pfdentry, oftTemp));

    case FIOSYNC:                                                       /*  将文件缓存回写              */
    case FIODATASYNC:
        return  (__tpsFsSync(pfdentry, LW_TRUE /*  LW_FALSE  ???*/));

    case FIOFLUSH:
        return  (__tpsFsSync(pfdentry, LW_TRUE));                       /*  文件与缓存全部回写          */

    case FIOCHMOD:
        return  (__tpsFsChmod(pfdentry, (INT)lArg));                    /*  改变文件访问权限            */

    case FIOSETFL:                                                      /*  设置新的 flag               */
        if ((INT)lArg & O_NONBLOCK) {
            pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);

    case FIOCHOWN:                                                      /*  修改文件所属关系            */
        return  (__tpsFsChown(pfdentry, (LW_IO_USR *)lArg));

    case FIOFSTYPE:                                                     /*  获得文件系统类型            */
        *(PCHAR *)lArg = "True Power Safe FileSystem";
        return  (ERROR_NONE);

    case FIOGETFORCEDEL:                                                /*  强制卸载设备是否被允许      */
        *(BOOL *)lArg = ptpsvol->TPSVOL_bForceDelete;
        return  (ERROR_NONE);

    case FIOSETFORCEDEL:                                                /*  设置强制卸载使能            */
        ptpsvol->TPSVOL_bForceDelete = (BOOL)lArg;
        return  (ERROR_NONE);

#ifdef __SYLIXOS_DEBUG
    case TPSFS_IOCTRL_INODE_PRINT:                                      /*  打印inode信息，用于调试     */
        if ((BOOL)lArg) {
            tpsFsInodeDump(ptpsvol->TPSVOL_tpsFsVol->SB_pinodeSpaceMng);/*  打印磁盘空间管理节点信息    */
        
        } else {
            tpsFsInodeDump(ptpsinode);                                  /*  打印普通节点信息            */
        }
        return  (ERROR_NONE);
#endif                                                                  /*  __SYLIXOS_DEBUG             */

    default:                                                            /*  无法识别的命令              */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    return  (PX_ERROR);
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
