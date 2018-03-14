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
** 文   件   名: epoll.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 11 月 18 日
**
** 描        述: Linux epoll subsystem (有限支持 epoll 部分主要功能).
**
** 注        意: SylixOS epoll 兼容子系统是由 select 子系统模拟出来的, 所以效率没有 select 高.
*********************************************************************************************************/

#ifndef __SYS_EPOLL_H
#define __SYS_EPOLL_H

#include "unistd.h"

/*********************************************************************************************************
 epoll_ctl() commands
*********************************************************************************************************/

#ifndef EPOLL_CTL_ADD
#define EPOLL_CTL_ADD   1
#define EPOLL_CTL_DEL   2
#define EPOLL_CTL_MOD   3
#endif                                                                  /*  EPOLL_CTL_ADD               */

/*********************************************************************************************************
 events types (bit fields)
*********************************************************************************************************/

#ifndef EPOLLIN
#define EPOLLIN         1
#define EPOLLPRI        2                                               /*  not support now!            */
#define EPOLLOUT        4
#define EPOLLERR        8
#define EPOLLHUP        16
#define EPOLLONESHOT    (1 << 30)
#define EPOLLET         (1 << 31)                                       /*  edge-trigger not support!   */
#endif                                                                  /*  EPOLLIN                     */

/*********************************************************************************************************
 epoll_create1 flags
*********************************************************************************************************/

#define EPOLL_CLOEXEC   O_CLOEXEC
#define EPOLL_NONBLOCK  O_NONBLOCK

/*********************************************************************************************************
 epoll_event
*********************************************************************************************************/

typedef union epoll_data {
    void        *ptr;
    int          fd;
    uint32_t     u32;
    uint64_t     u64;
} epoll_data_t;

struct epoll_event {
    uint32_t     events;                                                /*  Epoll events                */
    epoll_data_t data;                                                  /*  User data variable          */
};

/*********************************************************************************************************
 epoll api
*********************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

int epoll_create(int size);
int epoll_create1(int flags);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
int epoll_pwait(int epfd, struct epoll_event *events, int maxevents, int timeout, 
                const sigset_t *sigmask);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __SYS_EPOLL_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
