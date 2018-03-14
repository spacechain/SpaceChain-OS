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
** 文   件   名: lwip_socket.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 12 月 18 日
**
** 描        述: socket 接口. (使用 sylixos io 系统包裹 lwip 文件描述符)
*********************************************************************************************************/

#ifndef __LWIP_SOCKET_H
#define __LWIP_SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  close-on-exec and non-blocking
*********************************************************************************************************/

#define SOCK_CLOEXEC    0x10000000                                      /* Atomically close-on-exec     */
#define SOCK_NONBLOCK   0x20000000                                      /* Atomically non-blocking      */

/*********************************************************************************************************
  The maximum backlog queue length.
*********************************************************************************************************/

#define SOMAXCONN       255                                             /* lwip max backlog is 0xff     */

/*********************************************************************************************************
  AF_UNIX sockopt
*********************************************************************************************************/

#define SO_PASSCRED     16
#define SO_PEERCRED     17

/*********************************************************************************************************
  "Socket"-level control message types:
*********************************************************************************************************/

#define SCM_RIGHTS      0x01                                            /* access rights (array of int) */
#define SCM_CREDENTIALS 0x02                                            /* Linux certificate            */
#define SCM_CRED        0x03                                            /* BSD certificate              */

/*********************************************************************************************************
  AF_UNIX Linux certificate 
*********************************************************************************************************/

struct ucred {                                                          /* Linux                        */
    uint32_t    pid;                                                    /* Sender's process ID          */
    uint32_t    uid;                                                    /* Sender's user ID             */
    uint32_t    gid;                                                    /* Sender's group ID            */
};

/*********************************************************************************************************
  AF_UNIX BSD certificate 
*********************************************************************************************************/

#define CMGROUP_MAX 16

struct cmsgcred {                                                       /* FreeBSD                      */
    pid_t   cmcred_pid;                                                 /* PID of sending process       */
    uid_t   cmcred_uid;                                                 /* real UID of sending process  */
    uid_t   cmcred_euid;                                                /* effective UID of sending process */
    gid_t   cmcred_gid;                                                 /* real GID of sending process  */
    short   cmcred_ngroups;                                             /* number or groups             */
    gid_t   cmcred_groups[CMGROUP_MAX];                                 /* groups                       */
};

/*********************************************************************************************************
  socket api
*********************************************************************************************************/

LW_API int          socketpair(int domain, int type, int protocol, int sv[2]);
LW_API int          socket(int domain, int type, int protocol);
LW_API int          accept(int s, struct sockaddr *addr, socklen_t *addrlen);
LW_API int          accept4(int s, struct sockaddr *addr, socklen_t *addrlen, int flags);
LW_API int          bind(int s, const struct sockaddr *name, socklen_t namelen);
LW_API int          shutdown(int s, int how);
LW_API int          connect(int s, const struct sockaddr *name, socklen_t namelen);
LW_API int          getsockname(int s, struct sockaddr *name, socklen_t *namelen);
LW_API int          getpeername(int s, struct sockaddr *name, socklen_t *namelen);
LW_API int          setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);
LW_API int          getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen);
LW_API int          listen(int s, int backlog);
LW_API ssize_t      recv(int s, void *mem, size_t len, int flags);
LW_API ssize_t      recvfrom(int s, void *mem, size_t len, int flags,
                             struct sockaddr *from, socklen_t *fromlen);
LW_API ssize_t      recvmsg(int  s, struct msghdr *msg, int flags);
LW_API ssize_t      send(int s, const void *data, size_t size, int flags);
LW_API ssize_t      sendto(int s, const void *data, size_t size, int flags,
                           const struct sockaddr *to, socklen_t tolen);
LW_API ssize_t      sendmsg(int  s, const struct msghdr *msg, int flags);

/*********************************************************************************************************
  kernel function
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
VOID  __socketInit(VOID);
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __LWIP_SOCKET_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
