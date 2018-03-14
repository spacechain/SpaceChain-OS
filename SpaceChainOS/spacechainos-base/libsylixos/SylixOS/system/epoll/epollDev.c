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
** 文   件   名: epollDev.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 11 月 18 日
**
** 描        述: Linux epoll 子系统虚拟设备 (有限支持 epoll 部分主要功能).
**
** 注        意: SylixOS epoll 兼容子系统是由 select 子系统模拟出来的, 所以效率没有 select 高.

** BUG:
2017.08.31  _epollFiniFdset() 返回精确的文件描述符个数.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0) && (LW_CFG_EPOLL_EN > 0)
#include "epollDev.h"
/*********************************************************************************************************
  驱动程序全局变量
*********************************************************************************************************/
static INT              _G_iEpollDrvNum = PX_ERROR;
static LW_EPOLL_DEV     _G_epolldev;
/*********************************************************************************************************
  驱动程序
*********************************************************************************************************/
static LONG     _epollOpen(PLW_EPOLL_DEV    pepolldev, PCHAR  pcName, INT  iFlags, INT  iMode);
static INT      _epollClose(PLW_EPOLL_FILE  pepollfil);
static INT      _epollIoctl(PLW_EPOLL_FILE  pepollfil, INT  iRequest, LONG  lArg);
/*********************************************************************************************************
** 函数名称: _epollDrvInstall
** 功能描述: 安装 epoll 设备驱动程序
** 输　入  : NONE
** 输　出  : 驱动是否安装成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _epollDrvInstall (VOID)
{
    if (_G_iEpollDrvNum <= 0) {
        _G_iEpollDrvNum  = iosDrvInstall(LW_NULL,
                                         LW_NULL,
                                         _epollOpen,
                                         _epollClose,
                                         LW_NULL,
                                         LW_NULL,
                                         _epollIoctl);
        DRIVER_LICENSE(_G_iEpollDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iEpollDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iEpollDrvNum, "epoll driver.");
    }
    
    return  ((_G_iEpollDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** 函数名称: _epollDevCreate
** 功能描述: 安装 epoll 设备
** 输　入  : NONE
** 输　出  : 设备是否创建成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
INT  _epollDevCreate (VOID)
{
    if (_G_iEpollDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&_G_epolldev.EPD_devhdrHdr, LW_EPOLL_DEV_PATH, 
                    _G_iEpollDrvNum, DT_CHR) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _epollOpen
** 功能描述: 打开 epoll 设备
** 输　入  : pepolldev        epoll 设备
**           pcName           名称
**           iFlags           方式
**           iMode            方法
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  _epollOpen (PLW_EPOLL_DEV pepolldev, 
                         PCHAR         pcName,
                         INT           iFlags, 
                         INT           iMode)
{
    PLW_EPOLL_FILE  pepollfil;

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);
            return  (PX_ERROR);
        }
        
        pepollfil = (PLW_EPOLL_FILE)__SHEAP_ALLOC(sizeof(LW_EPOLL_FILE));
        if (!pepollfil) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        lib_bzero(pepollfil, sizeof(LW_EPOLL_FILE));
        
        pepollfil->EPF_uiMagic = LW_EPOLL_FILE_MAGIC;
        pepollfil->EPF_iFlag   = iFlags;
        pepollfil->EPF_ulMutex = API_SemaphoreMCreate("epoll_mutex", LW_PRIO_DEF_CEILING,
                                                      LW_OPTION_WAIT_PRIORITY |
                                                      LW_OPTION_DELETE_SAFE | 
                                                      LW_OPTION_INHERIT_PRIORITY |
                                                      LW_OPTION_OBJECT_GLOBAL,
                                                      LW_NULL);
        if (pepollfil->EPF_ulMutex == LW_OBJECT_HANDLE_INVALID) {
            __SHEAP_FREE(pepollfil);
            return  (PX_ERROR);
        }
        
        LW_DEV_INC_USE_COUNT(&_G_epolldev.EPD_devhdrHdr);
        
        return  ((LONG)pepollfil);
    }
}
/*********************************************************************************************************
** 函数名称: _epollClose
** 功能描述: 关闭 epoll 文件
** 输　入  : pepollfil         epoll 文件
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _epollClose (PLW_EPOLL_FILE  pepollfil)
{
    INT             i;
    PLW_EPOLL_EVENT pepollevent;

    if (pepollfil) {
        LW_EPOLL_FILE_LOCK(pepollfil);
        for (i = 0; i < LW_EPOLL_HASH_SIZE; i++) {
            while (pepollfil->EPF_plineEvent[i]) {
                pepollevent = _LIST_ENTRY(pepollfil->EPF_plineEvent[i], 
                                          LW_EPOLL_EVENT, EPE_lineManage);
                _List_Line_Del(&pepollevent->EPE_lineManage, &pepollfil->EPF_plineEvent[i]);
                __SHEAP_FREE(pepollevent);
            }
        }
        LW_EPOLL_FILE_UNLOCK(pepollfil);
        
        LW_DEV_DEC_USE_COUNT(&_G_epolldev.EPD_devhdrHdr);
        
        API_SemaphoreMDelete(&pepollfil->EPF_ulMutex);
        __SHEAP_FREE(pepollfil);
        
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: _epollIoctl
** 功能描述: 控制 epoll 文件
** 输　入  : pepollfil        epoll 文件
**           iRequest         功能
**           lArg             参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _epollIoctl (PLW_EPOLL_FILE  pepollfil, 
                         INT             iRequest, 
                         LONG            lArg)
{
    struct stat *pstatGet;
    
    switch (iRequest) {
    
    case FIONBIO:
        LW_EPOLL_FILE_LOCK(pepollfil);
        if (*(INT *)lArg) {
            pepollfil->EPF_iFlag |= O_NONBLOCK;
        } else {
            pepollfil->EPF_iFlag &= ~O_NONBLOCK;
        }
        LW_EPOLL_FILE_UNLOCK(pepollfil);
        break;
        
    case FIOFSTATGET:
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev     = (dev_t)&_G_epolldev;
            pstatGet->st_ino     = (ino_t)0;                            /*  相当于唯一节点              */
            pstatGet->st_mode    = 0666 | S_IFCHR;
            pstatGet->st_nlink   = 1;
            pstatGet->st_uid     = 0;
            pstatGet->st_gid     = 0;
            pstatGet->st_rdev    = 1;
            pstatGet->st_size    = 0;
            pstatGet->st_blksize = 0;
            pstatGet->st_blocks  = 0;
            pstatGet->st_atime   = API_RootFsTime(LW_NULL);
            pstatGet->st_mtime   = API_RootFsTime(LW_NULL);
            pstatGet->st_ctime   = API_RootFsTime(LW_NULL);
        } else {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        break;
        
    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _epollFindEvent
** 功能描述: epoll 文件寻找一个匹配的事件
** 输　入  : pepollfil        epoll 文件
**           iFd              文件描述符
** 输　出  : 寻找到的事件
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_EPOLL_EVENT _epollFindEvent (PLW_EPOLL_FILE pepollfil, INT  iFd)
{
    INT             iHash = (iFd & LW_EPOLL_HASH_MASK);
    PLW_LIST_LINE   plineTemp;
    PLW_EPOLL_EVENT pepollevent;
    
    for (plineTemp  = pepollfil->EPF_plineEvent[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pepollevent = _LIST_ENTRY(plineTemp, LW_EPOLL_EVENT, EPE_lineManage);
        if (pepollevent->EPE_iFd == iFd) {
            return  (pepollevent);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: _epollAddEvent
** 功能描述: epoll 文件添加一个事件
** 输　入  : pepollfil        epoll 文件
**           iFd              文件描述符
**           event            事件
** 输　出  : 是否成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _epollAddEvent (PLW_EPOLL_FILE pepollfil, INT  iFd, struct epoll_event *event)
{
    INT             iHash = (iFd & LW_EPOLL_HASH_MASK);
    PLW_EPOLL_EVENT pepollevent;
    
    pepollevent = (PLW_EPOLL_EVENT)__SHEAP_ALLOC(sizeof(LW_EPOLL_EVENT));
    if (pepollevent == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    pepollevent->EPE_iFd             = iFd;
    pepollevent->EPE_epEvent         = *event;
    pepollevent->EPE_epEvent.events |= EPOLLERR | EPOLLHUP;             /*  Linux 有这样的操作          */
    
    _List_Line_Add_Tail(&pepollevent->EPE_lineManage, &pepollfil->EPF_plineEvent[iHash]);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _epollDelEvent
** 功能描述: epoll 文件删除一个事件
** 输　入  : pepollfil        epoll 文件
**           pepollevent      epoll 事件
** 输　出  : 是否成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _epollDelEvent (PLW_EPOLL_FILE  pepollfil, PLW_EPOLL_EVENT pepollevent)
{
    INT  iHash = (pepollevent->EPE_iFd & LW_EPOLL_HASH_MASK);

    _List_Line_Del(&pepollevent->EPE_lineManage, &pepollfil->EPF_plineEvent[iHash]);
    
    __SHEAP_FREE(pepollevent);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _epollModEvent
** 功能描述: epoll 文件修改一个事件
** 输　入  : pepollfil        epoll 文件
**           pepollevent      epoll 事件
**           event            新事件
** 输　出  : 是否成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _epollModEvent (PLW_EPOLL_FILE  pepollfil, PLW_EPOLL_EVENT pepollevent, struct epoll_event *event)
{
    pepollevent->EPE_epEvent         = *event;
    pepollevent->EPE_epEvent.events |= EPOLLERR | EPOLLHUP;             /*  Linux 有这样的操作          */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _epollInitFdset
** 功能描述: epoll 文件根据事件, 设置 select 文件描述符集合
** 输　入  : pepollfil        epoll 文件
**           pfdsetRead       读文件描述符集合
**           pfdsetWrite      写文件描述符集合
**           pfdsetExcept     异常文件描述符集合
** 输　出  : 最大文件描述符 + 1
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _epollInitFdset (PLW_EPOLL_FILE  pepollfil, 
                      fd_set         *pfdsetRead,
                      fd_set         *pfdsetWrite,
                      fd_set         *pfdsetExcept)
{
#define LW_EPOLL_INMASK    (EPOLLIN)
#define LW_EPOLL_OUTMASK   (EPOLLOUT)
#define LW_EPOLL_EXCMASK   (EPOLLERR | EPOLLHUP)
    
    INT             i;
    INT             iWidth = 0;
    PLW_LIST_LINE   plineTemp;
    PLW_EPOLL_EVENT pepollevent;
    
    for (i = 0; i < LW_EPOLL_HASH_SIZE; i++) {
        for (plineTemp  = pepollfil->EPF_plineEvent[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            pepollevent = _LIST_ENTRY(plineTemp, LW_EPOLL_EVENT, EPE_lineManage);
            if (pepollevent->EPE_epEvent.events & LW_EPOLL_INMASK) {
                FD_SET(pepollevent->EPE_iFd, pfdsetRead);
            }
            
            if (pepollevent->EPE_epEvent.events & LW_EPOLL_OUTMASK) {
                FD_SET(pepollevent->EPE_iFd, pfdsetWrite);
            }
            
            if (pepollevent->EPE_epEvent.events & LW_EPOLL_EXCMASK) {
                FD_SET(pepollevent->EPE_iFd, pfdsetExcept);
            }
            
            if (iWidth < pepollevent->EPE_iFd) {
                iWidth = pepollevent->EPE_iFd;
            }
        }
    }
    
    return  (iWidth + 1);
}
/*********************************************************************************************************
** 函数名称: _epollInitFdset
** 功能描述: epoll 文件根据 select 结果, 回写相应的事件.
** 输　入  : pepollfil        epoll 文件
**           iWidth           最大文件号 + 1
**           pfdsetRead       读文件描述符集合
**           pfdsetWrite      写文件描述符集合
**           pfdsetExcept     异常文件描述符集合
**           events           回写事件缓冲
**           maxevents        回写事件缓冲大小
** 输　出  : 有效 fd 的数量.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _epollFiniFdset (PLW_EPOLL_FILE      pepollfil, 
                      INT                 iWidth,
                      fd_set             *pfdsetRead,
                      fd_set             *pfdsetWrite,
                      fd_set             *pfdsetExcept,
                      struct epoll_event *events, 
                      int                 maxevents)
{
    BOOL                bOne;
    INT                 i, iCnt = 0;
    UINT32              uiEvents;
    PLW_LIST_LINE       plineTemp;
    PLW_EPOLL_EVENT     pepollevent;
    
    for (i = 0; i < LW_EPOLL_HASH_SIZE; i++) {
        for (plineTemp  = pepollfil->EPF_plineEvent[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {

            bOne        = LW_FALSE;
            uiEvents    = 0;
            pepollevent = _LIST_ENTRY(plineTemp, LW_EPOLL_EVENT, EPE_lineManage);
            if (pepollevent->EPE_epEvent.events & LW_EPOLL_INMASK) {
                if (FD_ISSET(pepollevent->EPE_iFd, pfdsetRead)) {
                    bOne      = LW_TRUE;
                    uiEvents |= LW_EPOLL_INMASK;
                    if (pepollevent->EPE_epEvent.events & EPOLLONESHOT) {
                        pepollevent->EPE_epEvent.events = 0;
                    }
                }
            }
            
            if (pepollevent->EPE_epEvent.events & LW_EPOLL_OUTMASK) {
                if (FD_ISSET(pepollevent->EPE_iFd, pfdsetWrite)) {
                    bOne      = LW_TRUE;
                    uiEvents |= LW_EPOLL_OUTMASK;
                    if (pepollevent->EPE_epEvent.events & EPOLLONESHOT) {
                        pepollevent->EPE_epEvent.events = 0;
                    }
                }
            }
            
            if (pepollevent->EPE_epEvent.events & LW_EPOLL_EXCMASK) {
                if (FD_ISSET(pepollevent->EPE_iFd, pfdsetExcept)) {
                    bOne      = LW_TRUE;
                    uiEvents |= LW_EPOLL_EXCMASK;
                    if (pepollevent->EPE_epEvent.events & EPOLLONESHOT) {
                        pepollevent->EPE_epEvent.events = 0;
                    }
                }
            }
            
            if (bOne) {
                if (iCnt < maxevents) {
                    events[iCnt].events = uiEvents;
                    events[iCnt].data   = pepollevent->EPE_epEvent.data;
                }
                iCnt++;
            }
        }
    }
    
    return  (iCnt);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SELECT_EN > 0        */
                                                                        /*  LW_CFG_EPOLL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
