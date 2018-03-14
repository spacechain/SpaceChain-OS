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
** 文   件   名: epollDev.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 11 月 18 日
**
** 描        述: Linux epoll 子系统虚拟设备 (有限支持 epoll 部分主要功能).
**
** 注        意: SylixOS epoll 兼容子系统是由 select 子系统模拟出来的, 所以效率没有 select 高.
*********************************************************************************************************/

#ifndef __EPOLLDEV_H
#define __EPOLLDEV_H

/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0) && (LW_CFG_EPOLL_EN > 0)

#include "sys/epoll.h"

/*********************************************************************************************************
  设备路径
*********************************************************************************************************/

#define LW_EPOLL_DEV_PATH   "/dev/epollfd"

/*********************************************************************************************************
  设备与文件结构
*********************************************************************************************************/

typedef struct {
    LW_DEV_HDR          EPD_devhdrHdr;                                  /*  设备头                      */
} LW_EPOLL_DEV;
typedef LW_EPOLL_DEV   *PLW_EPOLL_DEV;

#define LW_EPOLL_HASH_SIZE  16
#define LW_EPOLL_HASH_MASK  (LW_EPOLL_HASH_SIZE - 1)

typedef struct {
#define LW_EPOLL_FILE_MAGIC 0x35ac796b
    UINT32              EPF_uiMagic;                                    /*  魔数                        */
    INT                 EPF_iFlag;                                      /*  打开文件的选项              */
    LW_OBJECT_HANDLE    EPF_ulMutex;                                    /*  互斥操作                    */
    LW_LIST_LINE_HEADER EPF_plineEvent[LW_EPOLL_HASH_SIZE];             /*  事件链表                    */
} LW_EPOLL_FILE;
typedef LW_EPOLL_FILE  *PLW_EPOLL_FILE;

/*********************************************************************************************************
  文件互斥访问
*********************************************************************************************************/

#define LW_EPOLL_FILE_LOCK(pepollfil)       \
        API_SemaphoreMPend(pepollfil->EPF_ulMutex, LW_OPTION_WAIT_INFINITE)
        
#define LW_EPOLL_FILE_UNLOCK(pepollfil)     \
        API_SemaphoreMPost(pepollfil->EPF_ulMutex)

/*********************************************************************************************************
  文件保存的事件结构
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE        EPE_lineManage;
    INT                 EPE_iFd;
    struct epoll_event  EPE_epEvent;
} LW_EPOLL_EVENT;
typedef LW_EPOLL_EVENT *PLW_EPOLL_EVENT;

/*********************************************************************************************************
  初始化操作
*********************************************************************************************************/

INT  _epollDrvInstall(VOID);
INT  _epollDevCreate(VOID);

/*********************************************************************************************************
  基本事件操作
*********************************************************************************************************/

PLW_EPOLL_EVENT _epollFindEvent(PLW_EPOLL_FILE pepollfil, INT  iFd);
INT             _epollAddEvent(PLW_EPOLL_FILE  pepollfil, INT  iFd, struct epoll_event *event);
INT             _epollDelEvent(PLW_EPOLL_FILE  pepollfil, PLW_EPOLL_EVENT pepollevent);
INT             _epollModEvent(PLW_EPOLL_FILE  pepollfil, PLW_EPOLL_EVENT pepollevent, 
                               struct epoll_event *event);

/*********************************************************************************************************
  select 事件集合操作
*********************************************************************************************************/

INT   _epollInitFdset(PLW_EPOLL_FILE  pepollfil, 
                      fd_set         *pfdsetRead,
                      fd_set         *pfdsetWrite,
                      fd_set         *pfdsetExcept);
INT   _epollFiniFdset(PLW_EPOLL_FILE      pepollfil, 
                      INT                 iWidth,
                      fd_set             *pfdsetRead,
                      fd_set             *pfdsetWrite,
                      fd_set             *pfdsetExcept,
                      struct epoll_event *events, 
                      int                 maxevents);

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SELECT_EN > 0        */
                                                                        /*  LW_CFG_EPOLL_EN > 0         */
#endif                                                                  /*  __EPOLLDEV_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
