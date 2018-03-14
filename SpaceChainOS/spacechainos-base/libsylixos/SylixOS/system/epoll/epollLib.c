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
** 文   件   名: epollLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 11 月 18 日
**
** 描        述: Linux epoll 子系统 (有限支持 epoll 部分主要功能).
**
** 注        意: SylixOS epoll 兼容子系统是由 select 子系统模拟出来的, 所以效率没有 select 高.

** BUG:
2017.08.31  epoll_pwait() 返回精确的文件描述符个数.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0) && (LW_CFG_EPOLL_EN > 0)
#include "epollDev.h"
/*********************************************************************************************************
  安全的获取 epoll 文件结构
*********************************************************************************************************/
#define LW_EPOLL_FILE_GET(fd)   \
        pepollfil = (PLW_EPOLL_FILE)API_IosFdValue((fd));   \
        if (pepollfil == (PLW_EPOLL_FILE)PX_ERROR) {    \
            return  (PX_ERROR); \
        }   \
        if (pepollfil->EPF_uiMagic != LW_EPOLL_FILE_MAGIC) {    \
            _ErrorHandle(EBADF);    \
            return  (PX_ERROR); \
        }
/*********************************************************************************************************
** 函数名称: _EpollInit
** 功能描述: 初始化 epoll 子系统
** 输　入  : NONE
** 输　出  : 初始化结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _EpollInit (VOID)
{
    INT     iError;
    
    iError = _epollDrvInstall();
    if (iError == ERROR_NONE) {
        iError =  _epollDevCreate();
    }
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: epoll_create
** 功能描述: 创建一个 epoll 文件描述符
** 输　入  : size      暂时未使用
** 输　出  : epoll 文件描述符
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  epoll_create (int size)
{
    INT iFd;

    (VOID)size;
    
    iFd = open(LW_EPOLL_DEV_PATH, O_RDWR);
    
    return  (iFd);
}
/*********************************************************************************************************
** 函数名称: epoll_create1
** 功能描述: 创建一个 epoll 文件描述符
** 输　入  : flags 
** 输　出  : epoll 文件描述符
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  epoll_create1 (int flags)
{
    INT iFd;
    
    flags &= (EPOLL_CLOEXEC | EPOLL_NONBLOCK);
    
    iFd = open(LW_EPOLL_DEV_PATH, O_RDWR | flags);
    
    return  (iFd);
}
/*********************************************************************************************************
** 函数名称: epoll_ctl
** 功能描述: 控制一个 epoll 文件描述符
** 输　入  : epfd          文件描述符
**           op            操作类型 EPOLL_CTL_ADD / EPOLL_CTL_DEL / EPOLL_CTL_MOD
**           fd            目标文件描述符
**           event         事件
** 输　出  : 0 表示成功 -1 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  epoll_ctl (int epfd, int op, int fd, struct epoll_event *event)
{
    INT             iError;
    PLW_EPOLL_FILE  pepollfil;
    PLW_EPOLL_EVENT pepollevent;
    
    if (!event && (op != EPOLL_CTL_DEL)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    LW_EPOLL_FILE_GET(epfd);
    
    if (pepollfil == (PLW_EPOLL_FILE)API_IosFdValue(fd)) {              /*  不能操作自己                */
        _ErrorHandle(ELOOP);
        return  (PX_ERROR);
    }
    
    iError = PX_ERROR;
    
    LW_EPOLL_FILE_LOCK(pepollfil);
    pepollevent = _epollFindEvent(pepollfil, fd);
    
    switch (op) {
    
    case EPOLL_CTL_ADD:
        if (!pepollevent) {
            iError = _epollAddEvent(pepollfil, fd, event);
        } else {
            _ErrorHandle(EEXIST);
        }
        break;
        
    case EPOLL_CTL_DEL:
        if (pepollevent) {
            iError = _epollDelEvent(pepollfil, pepollevent);
        } else {
            _ErrorHandle(ENOENT);
        }
        break;
        
    case EPOLL_CTL_MOD:
        if (pepollevent) {
            iError = _epollModEvent(pepollfil, pepollevent, event);
        } else {
            _ErrorHandle(ENOENT);
        }
        break;
        
    default:
        _ErrorHandle(EINVAL);
        break;
    }
    LW_EPOLL_FILE_UNLOCK(pepollfil);
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: epoll_wait
** 功能描述: 等待指定 epoll 文件描述符事件有效
** 输　入  : epfd          文件描述符
**           event         有效事件缓冲区
**           maxevents     有效事件缓冲区大小
**           timeout       超时时间 (毫秒, -1 表示永久等待)
** 输　出  : 有效的事件个数
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  epoll_wait (int epfd, struct epoll_event *events, int maxevents, int timeout)
{
    return  (epoll_pwait(epfd, events, maxevents, timeout, LW_NULL));
}
/*********************************************************************************************************
** 函数名称: epoll_pwait
** 功能描述: 等待指定 epoll 文件描述符事件有效
** 输　入  : epfd          文件描述符
**           event         有效事件缓冲区
**           maxevents     有效事件缓冲区大小
**           timeout       超时时间 (毫秒, -1 表示永久等待)
**           sigmask       等待时阻塞的信号
** 输　出  : 有效的事件个数
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  epoll_pwait (int epfd, struct epoll_event *events, int maxevents, int timeout, 
                  const sigset_t *sigmask)
{
    struct timespec ts;
    fd_set          fdsetRead, fdsetWrite, fdsetExcept;
    PLW_EPOLL_FILE  pepollfil;
    INT             iWidth;
    INT             iNum;
    
    LW_EPOLL_FILE_GET(epfd);
    
    FD_ZERO(&fdsetRead);
    FD_ZERO(&fdsetWrite);
    FD_ZERO(&fdsetExcept);
    
    if (timeout >= 0) {
        ts.tv_sec  = (time_t)(timeout / 1000);
        ts.tv_nsec = (LONG)(timeout % 1000) * (1000 * 1000);
    }
    
    LW_EPOLL_FILE_LOCK(pepollfil);
    iWidth = _epollInitFdset(pepollfil, &fdsetRead, &fdsetWrite, &fdsetExcept);
    LW_EPOLL_FILE_UNLOCK(pepollfil);
    
    if (timeout < 0) {
        iNum = pselect(iWidth, &fdsetRead, &fdsetWrite, &fdsetExcept, LW_NULL, sigmask);
    
    } else {
        iNum = pselect(iWidth, &fdsetRead, &fdsetWrite, &fdsetExcept, &ts, sigmask);
    }
    
    if (iNum <= 0) {
        return  (iNum);
    }
    
    LW_EPOLL_FILE_GET(epfd);
    
    LW_EPOLL_FILE_LOCK(pepollfil);
    iNum = _epollFiniFdset(pepollfil, iWidth, &fdsetRead, &fdsetWrite, &fdsetExcept, events, maxevents);
    LW_EPOLL_FILE_UNLOCK(pepollfil);
    
    return  (iNum);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SELECT_EN > 0        */
                                                                        /*  LW_CFG_EPOLL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
