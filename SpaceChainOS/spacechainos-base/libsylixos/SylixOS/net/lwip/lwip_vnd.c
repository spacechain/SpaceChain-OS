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
** 文   件   名: lwip_vnd.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 24 日
**
** 描        述: 虚拟网络设备接口.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_VNETDEV_EN > 0
#include "lwip/tcpip.h"
#include "net/if_ether.h"
#include "net/if_vnd.h"
#include "netdev/netdev.h"
#include "netdev/vnetdev.h"
/*********************************************************************************************************
  设备与文件结构
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            VND_lineManage;                             /*  所有网络设备链表            */
    struct vnetdev          VND_vnetdev;                                /*  网络设备                    */
    BOOL                    VND_bBusy;                                  /*  是否在忙                    */
    LW_SEL_WAKEUPLIST       VND_selwulList;                             /*  等待链                      */
    LW_OBJECT_HANDLE        VND_ulReadSync;                             /*  读取同步信号量              */
} LW_VND_DEV;
typedef LW_VND_DEV         *PLW_VND_DEV;

typedef struct {
    LW_DEV_HDR              VNDM_devhdrHdr;                              /*  设备头                      */
    LW_LIST_LINE_HEADER     VNDM_plineDev;                               /*  网络设备链                  */
    LW_LIST_LINE_HEADER     VNDM_plineFile;                              /*  文件链表                    */
} LW_VND_MDEV;
typedef LW_VND_MDEV        *PLW_VND_MDEV;

typedef struct {
    LW_LIST_LINE            VNDFIL_lineManage;                          /*  文件链表                    */
    INT                     VNDFIL_iFlag;                               /*  打开文件的选项              */
    PLW_VND_DEV             VNDFIL_vndSel;                              /*  当前选择的网络设备          */
} LW_VND_FILE;
typedef LW_VND_FILE        *PLW_VND_FILE;
/*********************************************************************************************************
  驱动程序全局变量
*********************************************************************************************************/
static INT               _G_iVndDrvNum = PX_ERROR;
static LW_VND_MDEV       _G_vndmDev;
static LW_OBJECT_HANDLE  _G_hVndMutex;
static LW_OBJECT_HANDLE  _G_hVndSelMutex;
/*********************************************************************************************************
  锁操作
*********************************************************************************************************/
#define VND_LOCK()       LOCK_TCPIP_CORE()
#define VND_UNLOCK()     UNLOCK_TCPIP_CORE()
/*********************************************************************************************************
  驱动程序
*********************************************************************************************************/
static LONG     _vndOpen(PLW_VND_MDEV   pvnddev, PCHAR  pcName, INT  iFlags, INT  iMode);
static INT      _vndClose(PLW_VND_FILE  pvndfil);
static ssize_t  _vndRead(PLW_VND_FILE   pvndfil, PCHAR  pcBuffer, size_t  stMaxBytes);
static ssize_t  _vndWrite(PLW_VND_FILE  pvndfil, PCHAR  pcBuffer, size_t  stNBytes);
static INT      _vndIoctl(PLW_VND_FILE  pvndfil, INT    iRequest, LONG    lArg);
/*********************************************************************************************************
  shell 命令初始化
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
static VOID     __tshellVndInit(VOID);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** 函数名称: _netVndCreate
** 功能描述: 安装 vnd 控制设备
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _netVndCreate (VOID)
{
    if (_G_iVndDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    _G_hVndMutex = API_SemaphoreMCreate("vnd_lock", LW_PRIO_DEF_CEILING, 
                                        LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE | 
                                        LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (_G_hVndMutex == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }
    
    _G_hVndSelMutex = API_SemaphoreMCreate("vnd_sel", LW_PRIO_DEF_CEILING, 
                                           LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE | 
                                           LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (_G_hVndSelMutex == LW_OBJECT_HANDLE_INVALID) {
        API_SemaphoreMDelete(&_G_hVndMutex);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&_G_vndmDev.VNDM_devhdrHdr, IF_VND_PATH, 
                    _G_iVndDrvNum, DT_CHR) != ERROR_NONE) {
        API_SemaphoreMDelete(&_G_hVndMutex);
        API_SemaphoreMDelete(&_G_hVndSelMutex);
        return  (PX_ERROR);
    }
    
#if LW_CFG_SHELL_EN > 0
    __tshellVndInit();
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _netVndInit
** 功能描述: 初始化虚拟网络设备
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _netVndInit (VOID)
{
    if (_G_iVndDrvNum <= 0) {
        _G_iVndDrvNum  = iosDrvInstall(_vndOpen, LW_NULL, _vndOpen,
                                       _vndClose, _vndRead, _vndWrite, _vndIoctl);
        DRIVER_LICENSE(_G_iVndDrvNum,     "Dual GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iVndDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iVndDrvNum, "virtual net device driver.");
    }
    
    API_RootFsMakeNode("/dev/net", LW_ROOTFS_NODE_TYPE_DIR, LW_ROOTFS_NODE_OPT_ROOTFS_TIME, 
                       DEFAULT_DIR_PERM, LW_NULL);                      /*  创建 /dev/net 目录          */
                       
    if (_G_iVndDrvNum > 0) {
        return  (_netVndCreate());
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: _vndOpen
** 功能描述: 打开虚拟网络控制设备
** 输　入  : pvndmdev         虚拟网络控制设备
**           pcName           名称
**           iFlags           方式
**           iMode            方法
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  _vndOpen (PLW_VND_MDEV pvndmdev, PCHAR  pcName,INT  iFlags, INT  iMode)
{
    PLW_VND_FILE  pvndfil;
    
    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);                          /*  不能重复创建                */
            return  (PX_ERROR);
        }
        
        if (geteuid() != 0) {
            _ErrorHandle(EACCES);                                       /*  权限不够                    */
            return  (PX_ERROR);
        }
        
        pvndfil = (PLW_VND_FILE)__SHEAP_ALLOC(sizeof(LW_VND_FILE));
        if (!pvndfil) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        
        pvndfil->VNDFIL_iFlag  = iFlags;
        pvndfil->VNDFIL_vndSel = LW_NULL;
        
        VND_LOCK();
        _List_Line_Add_Tail(&pvndfil->VNDFIL_lineManage,
                            &_G_vndmDev.VNDM_plineFile);
        VND_UNLOCK();
        
        LW_DEV_INC_USE_COUNT(&_G_vndmDev.VNDM_devhdrHdr);
        
        return  ((LONG)pvndfil);
    }
}
/*********************************************************************************************************
** 函数名称: _vndClose
** 功能描述: 关闭虚拟网络控制设备
** 输　入  : pvndfil          网络控制设备文件
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _vndClose (PLW_VND_FILE  pvndfil)
{
    PLW_VND_DEV  pvnd;

    if (pvndfil) {
        VND_LOCK();
        _List_Line_Del(&pvndfil->VNDFIL_lineManage,
                       &_G_vndmDev.VNDM_plineFile);
        if (pvndfil->VNDFIL_vndSel) {
            pvnd = pvndfil->VNDFIL_vndSel;
            pvnd->VND_bBusy = LW_FALSE;
            vnetdev_linkup(&pvnd->VND_vnetdev, 0);
        }
        VND_UNLOCK();
        
        LW_DEV_DEC_USE_COUNT(&_G_vndmDev.VNDM_devhdrHdr);
        
        __SHEAP_FREE(pvndfil);
        
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: _vndRead
** 功能描述: 读网络控制设备文件
** 输　入  : pvndfil          网络控制设备文件
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  _vndRead (PLW_VND_FILE  pvndfil, PCHAR  pcBuffer, size_t  stMaxBytes)
{
    ULONG        ulErrCode;
    ULONG        ulTimeout;
    PLW_VND_DEV  pvnd;
    struct pbuf *p;
    ssize_t      sstRet;

    if (!pcBuffer || !stMaxBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pvndfil->VNDFIL_iFlag & O_NONBLOCK) {                           /*  非阻塞 IO                   */
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    }
    
    pvnd = pvndfil->VNDFIL_vndSel;                                      /*  没有选择网络设备            */
    if (pvnd == LW_NULL) {
        _ErrorHandle(EPIPE);
        return  (PX_ERROR);
    }
    
    for (;;) {
        VND_LOCK();
        p = vnetdev_get(&pvnd->VND_vnetdev);
        VND_UNLOCK();
        if (p) {
            break;                                                      /*  已接收到消息                */
        }

        ulErrCode = API_SemaphoreBPend(pvnd->VND_ulReadSync, ulTimeout);
        if (ulErrCode != ERROR_NONE) {                                  /*  超时                        */
            _ErrorHandle(EAGAIN);
            return  (0);
        }
    }
    
    sstRet = (ssize_t)((p->tot_len > stMaxBytes) ? stMaxBytes : p->tot_len);
    pbuf_copy_partial(p, pcBuffer, (u16_t)sstRet, 0);
    pbuf_free(p);
    
    return  (sstRet);
}
/*********************************************************************************************************
** 函数名称: _vndWrite
** 功能描述: 写网络控制设备文件
** 输　入  : pvndfil          网络控制设备文件
**           pcBuffer         将要写入的数据指针
**           stNBytes         写入数据大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  _vndWrite (PLW_VND_FILE  pvndfil, PCHAR  pcBuffer, size_t  stNBytes)
{
    PLW_VND_DEV  pvnd;
    struct pbuf *p;

    if (!pcBuffer || !stNBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pvnd = pvndfil->VNDFIL_vndSel;                                      /*  没有选择网络设备            */
    if (pvnd == LW_NULL) {
        _ErrorHandle(EPIPE);
        return  (PX_ERROR);
    }
    
    if (stNBytes > vnetdev_maxplen(&pvnd->VND_vnetdev)) {               /*  不能大于最大报文长度        */
        _ErrorHandle(EMSGSIZE);
        return  (PX_ERROR);
    }
    
    p = netdev_pbuf_alloc((u16_t)stNBytes);
    if (!p) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    
    pbuf_take(p, pcBuffer, (u16_t)stNBytes);                            /*  将数据填入 pbuf             */
    if (vnetdev_put(&pvnd->VND_vnetdev, p)) {
        netdev_pbuf_free(p);
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    return  ((ssize_t)stNBytes);
}
/*********************************************************************************************************
** 函数名称: _vndNotifyDev
** 功能描述: 有数据可读, 回调通知
** 输　入  : pvnetdev         虚拟网络设备结构
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _vndNotifyDev (struct vnetdev *pvnetdev)
{
    PLW_VND_DEV   pvnd = _LIST_ENTRY(pvnetdev, LW_VND_DEV, VND_vnetdev);
    
    API_SemaphoreBPost(pvnd->VND_ulReadSync);
    SEL_WAKE_UP_ALL(&pvnd->VND_selwulList, SELREAD);
}
/*********************************************************************************************************
** 函数名称: _vndAddDev
** 功能描述: 网络控制设备文件创建一个虚拟网络设备
** 输　入  : pifvnd            网络设备描述
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _vndAddDev (struct ifvnd *pifvnd)
{
    PLW_VND_DEV   pvndNew;
    PLW_LIST_LINE pline;
    
    if ((pifvnd->ifvnd_type != IF_VND_TYPE_RAW) &&
        (pifvnd->ifvnd_type != IF_VND_TYPE_ETHERNET)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pifvnd->ifvnd_bsize < VNETDEV_MTU_MAX) {                        /*  缓冲区至少是一个数据包      */
        _ErrorHandle(EMSGSIZE);
        return  (PX_ERROR);
    }
    
    pvndNew = (PLW_VND_DEV)__SHEAP_ALLOC(sizeof(LW_VND_DEV));
    if (!pvndNew) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pvndNew, sizeof(LW_VND_DEV));
    lib_memcpy(pvndNew->VND_vnetdev.netdev.hwaddr, pifvnd->ifvnd_hwaddr, ETH_ALEN);
    
    pvndNew->VND_selwulList.SELWUL_hListLock = _G_hVndSelMutex;
    pvndNew->VND_ulReadSync = API_SemaphoreBCreate("vnd_rd", LW_FALSE, 
                                                   LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pvndNew->VND_ulReadSync == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pvndNew);
        return  (PX_ERROR);
    }
    
    VND_LOCK();
    for (pline = _G_vndmDev.VNDM_plineDev; pline != LW_NULL; pline = _list_line_get_next(pline)) {
        PLW_VND_DEV  pvnd = _LIST_ENTRY(pline, LW_VND_DEV, VND_lineManage);
        if (pvnd->VND_vnetdev.id == pifvnd->ifvnd_id) {
            break;
        }
    }
    if (pline != LW_NULL) {
        VND_UNLOCK();
        API_SemaphoreBDelete(&pvndNew->VND_ulReadSync);
        __SHEAP_FREE(pvndNew);
        _ErrorHandle(EEXIST);
        return  (PX_ERROR);
    }
    pvndNew->VND_vnetdev.id = pifvnd->ifvnd_id;
    pvndNew->VND_bBusy      = LW_TRUE;
    _List_Line_Add_Ahead(&pvndNew->VND_lineManage, &_G_vndmDev.VNDM_plineDev);
    VND_UNLOCK();
    
    if (vnetdev_add(&pvndNew->VND_vnetdev, _vndNotifyDev, LW_CFG_NET_VNETDEV_DEF_BSIZE,
                    pifvnd->ifvnd_id, pifvnd->ifvnd_type, pvndNew)) {
        VND_LOCK();
        _List_Line_Del(&pvndNew->VND_lineManage, &_G_vndmDev.VNDM_plineDev);
        VND_UNLOCK();
        API_SemaphoreBDelete(&pvndNew->VND_ulReadSync);
        __SHEAP_FREE(pvndNew);
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }
    
    netdev_ifname(&pvndNew->VND_vnetdev.netdev, pifvnd->ifvnd_ifname);
    pvndNew->VND_bBusy = LW_FALSE;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _vndDelDev
** 功能描述: 网络控制设备文件删除一个虚拟网络设备
** 输　入  : pifvnd            网络设备描述
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _vndDelDev (struct ifvnd *pifvnd)
{
    PLW_VND_DEV   pvnd;
    PLW_LIST_LINE pline;
    
    VND_LOCK();
    for (pline = _G_vndmDev.VNDM_plineDev; pline != LW_NULL; pline = _list_line_get_next(pline)) {
        pvnd = _LIST_ENTRY(pline, LW_VND_DEV, VND_lineManage);
        if (pvnd->VND_vnetdev.id == pifvnd->ifvnd_id) {
            break;
        }
    }
    if (pline != LW_NULL) {
        if (pvnd->VND_bBusy) {
            VND_UNLOCK();
            _ErrorHandle(EBUSY);
            return  (PX_ERROR);
        }
        _List_Line_Del(&pvnd->VND_lineManage, &_G_vndmDev.VNDM_plineDev);
        netdev_ifname(&pvnd->VND_vnetdev.netdev, pifvnd->ifvnd_ifname);
        pifvnd->ifvnd_type = pvnd->VND_vnetdev.type;

    } else {
        VND_UNLOCK();
        _ErrorHandle(ENODEV);
        return  (PX_ERROR);
    }
    VND_UNLOCK();
    
    vnetdev_delete(&pvnd->VND_vnetdev);
    API_SemaphoreBDelete(&pvnd->VND_ulReadSync);
    __SHEAP_FREE(pvnd);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _vndSelDev
** 功能描述: 网络控制设备文件选择一个虚拟网络设备
** 输　入  : pvndfil          网络控制设备文件
**           pifvnd           网络设备描述
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _vndSelDev (PLW_VND_FILE  pvndfil, struct ifvnd *pifvnd)
{
    PLW_VND_DEV   pvnd;
    PLW_LIST_LINE pline;
    
    if (pvndfil->VNDFIL_vndSel) {
        _ErrorHandle(ENOTEMPTY);
        return  (PX_ERROR);
    }
    
    VND_LOCK();
    for (pline = _G_vndmDev.VNDM_plineDev; pline != LW_NULL; pline = _list_line_get_next(pline)) {
        pvnd = _LIST_ENTRY(pline, LW_VND_DEV, VND_lineManage);
        if (pvnd->VND_vnetdev.id == pifvnd->ifvnd_id) {
            break;
        }
    }
    if (pline != LW_NULL) {
        if (pvnd->VND_bBusy) {
            VND_UNLOCK();
            _ErrorHandle(EBUSY);
            return  (PX_ERROR);
        }
        pvndfil->VNDFIL_vndSel = pvnd;
        pvnd->VND_bBusy        = LW_TRUE;
        vnetdev_linkup(&pvnd->VND_vnetdev, 1);
        netdev_ifname(&pvnd->VND_vnetdev.netdev, pifvnd->ifvnd_ifname);
        pifvnd->ifvnd_type = pvnd->VND_vnetdev.type;

    } else {
        VND_UNLOCK();
        _ErrorHandle(ENODEV);
        return  (PX_ERROR);
    }
    VND_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _vndIoctl
** 功能描述: 控制网络控制设备文件
** 输　入  : pvndfil          网络控制设备文件
**           iRequest         功能
**           lArg             参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _vndIoctl (PLW_VND_FILE  pvndfil, INT  iRequest, LONG  lArg)
{
    PLW_SEL_WAKEUPNODE   pselwunNode;
    struct ifvnd        *pifvnd;
    struct stat         *pstatGet;
    PLW_VND_DEV          pvnd = pvndfil->VNDFIL_vndSel;
    INT                  iRet = PX_ERROR;
    
    switch (iRequest) {
    
    case FIONREAD:
        if (pvnd) {
            VND_LOCK();
            *(INT *)lArg = vnetdev_nread(&pvnd->VND_vnetdev);
            VND_UNLOCK();
            iRet = ERROR_NONE;
        
        } else {
            _ErrorHandle(EPIPE);
        }
        break;
        
    case FIONBIO:
        VND_LOCK();
        if (*(INT *)lArg) {
            pvndfil->VNDFIL_iFlag |= O_NONBLOCK;
        } else {
            pvndfil->VNDFIL_iFlag &= ~O_NONBLOCK;
        }
        VND_UNLOCK();
        iRet = ERROR_NONE;
        break;
        
    case FIORFLUSH:
    case FIOFLUSH:
        if (pvnd) {
            VND_LOCK();
            vnetdev_flush(&pvnd->VND_vnetdev);
            VND_UNLOCK();
            iRet = ERROR_NONE;
            
        } else {
            _ErrorHandle(EPIPE);
        }
        break;
        
    case FIORBUFSET:
        if (pvnd) {
            VND_LOCK();
            iRet = vnetdev_bufsize(&pvnd->VND_vnetdev, (size_t)lArg);
            VND_UNLOCK();
            if (iRet) {
                _ErrorHandle(EMSGSIZE);
            }
            
        } else {
            _ErrorHandle(EPIPE);
        }
        break;
        
    case FIOFSTATGET:
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev   = (dev_t)&_G_vndmDev;
            pstatGet->st_ino   = (ino_t)0;
            pstatGet->st_mode  = 0444 | S_IFCHR;
            pstatGet->st_nlink = 1;
            pstatGet->st_uid   = 0;
            pstatGet->st_gid   = 0;
            pstatGet->st_rdev  = 1;
            if (pvnd) {
                VND_LOCK();
                pstatGet->st_size = (off_t)vnetdev_nread(&pvnd->VND_vnetdev);
                VND_UNLOCK();
            
            } else {
                pstatGet->st_size = 0;
            }
            pstatGet->st_blksize = 0;
            pstatGet->st_blocks  = 0;
            pstatGet->st_atime   = API_RootFsTime(LW_NULL);
            pstatGet->st_mtime   = API_RootFsTime(LW_NULL);
            pstatGet->st_ctime   = API_RootFsTime(LW_NULL);
            iRet = ERROR_NONE;
        
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
        
    case FIOSELECT:
        if (pvnd) {
            pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
            SEL_WAKE_NODE_ADD(&pvnd->VND_selwulList, pselwunNode);
            
            switch (pselwunNode->SELWUN_seltypType) {
            
            case SELREAD:
                if (vnetdev_nread(&pvnd->VND_vnetdev)) {
                    SEL_WAKE_UP(pselwunNode);
                }
                break;
                
            case SELWRITE:
                SEL_WAKE_UP(pselwunNode);
                break;
                
            case SELEXCEPT:
                break;
            }
            iRet = ERROR_NONE;
        
        } else {
            _ErrorHandle(EPIPE);
        }
        break;
        
    case FIOUNSELECT:
        if (pvnd) {
            SEL_WAKE_NODE_DELETE(&pvnd->VND_selwulList, (PLW_SEL_WAKEUPNODE)lArg);
            iRet = ERROR_NONE;
            
        } else {
            _ErrorHandle(EPIPE);
        }
        break;
        
    case SIOCVNDADD:
        pifvnd = (struct ifvnd *)lArg;
        if (pifvnd) {
            iRet = _vndAddDev(pifvnd);
            
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
        
    case SIOCVNDDEL:
        pifvnd = (struct ifvnd *)lArg;
        if (pifvnd) {
            iRet = _vndDelDev(pifvnd);
            
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
        
    case SIOCVNDSEL:
        pifvnd = (struct ifvnd *)lArg;
        if (pifvnd) {
            iRet = _vndSelDev(pvndfil, pifvnd);
            
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
        
    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __tshellVnd
** 功能描述: 系统命令 "vnd"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellVnd (INT  iArgC, PCHAR  *ppcArgV)
{
    INT             i, iFd, iMac[6];
    struct ifvnd    ifvnd;
    
    if (iArgC < 3) {
        goto    __arg_error;
    }
    
    if (!lib_strcmp(ppcArgV[1], "add")) {
        ifvnd.ifvnd_id    = lib_atoi(ppcArgV[2]);
        ifvnd.ifvnd_type  = IF_VND_TYPE_ETHERNET;
        ifvnd.ifvnd_bsize = LW_CFG_NET_VNETDEV_DEF_BSIZE;
        
        if (iArgC > 3) {
            if (sscanf(ppcArgV[3], "%x:%x:%x:%x:%x:%x",
                       &iMac[0], &iMac[1], &iMac[2], &iMac[3], &iMac[4], &iMac[5]) != 6) {
                goto    __arg_error;
            }
            for (i = 0; i < 6; i++) {
                ifvnd.ifvnd_hwaddr[i] = (UINT8)iMac[i];
            }
        
        } else {
            for (i = 0; i < 6; i++) {
                ifvnd.ifvnd_hwaddr[i] = 0;
            }
        }
        
        iFd = open(IF_VND_PATH, O_RDWR);
        if (iFd < 0) {
            fprintf(stderr, "can not open %s error: %s!\n", IF_VND_PATH, lib_strerror(errno));
            return  (PX_ERROR);
        }
        
        if (ioctl(iFd, SIOCVNDADD, &ifvnd)) {
            fprintf(stderr, "command 'SIOCVNDADD' error: %s!\n", lib_strerror(errno));
            close(iFd);
            return  (PX_ERROR);
        }
        close(iFd);
        
        printf("virtual net device %s added.\n", ifvnd.ifvnd_ifname);
        return  (ERROR_NONE);
        
    } else if (!lib_strcmp(ppcArgV[1], "del")) {
        ifvnd.ifvnd_id = lib_atoi(ppcArgV[2]);
        
        iFd = open(IF_VND_PATH, O_RDWR);
        if (iFd < 0) {
            fprintf(stderr, "can not open %s error: %s!\n", IF_VND_PATH, lib_strerror(errno));
            return  (PX_ERROR);
        }
        
        if (ioctl(iFd, SIOCVNDDEL, &ifvnd)) {
            fprintf(stderr, "command 'SIOCVNDDEL' error: %s!\n", lib_strerror(errno));
            close(iFd);
            return  (PX_ERROR);
        }
        close(iFd);
        
        printf("virtual net device %s delete.\n", ifvnd.ifvnd_ifname);
        return  (ERROR_NONE);
    }
    
__arg_error:
    fprintf(stderr, "arguments error!\n");
    return  (-ERROR_TSHELL_EPARAM);
}
/*********************************************************************************************************
** 函数名称: __tshellVndInit
** 功能描述: 虚拟网络设备控制设备 shell 命令初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID __tshellVndInit (VOID)
{
    API_TShellKeywordAdd("vnd", __tshellVnd);
    API_TShellFormatAdd("vnd", " [add | del] [id] [hwaddr]");
    API_TShellHelpAdd("vnd",   "add or delete a virtual net device (ethernet).\n"
                               "eg. vnd add 10 10:ae:13:b2:5c:f8\n"
                               "    vnd add 11\n"
                               "    vnd del 10\n");
}

#endif                                                                  /*  LW_CFG_SHELL_EN             */
#endif                                                                  /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_NET_VNETDEV_EN       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
