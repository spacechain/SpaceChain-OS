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
** 文   件   名: af_unix_msg.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 12 月 28 日
**
** 描        述: AF_UNIX 对 msg 的分析与处理.

** BUG:
2013.11.18  加入对 MSG_CMSG_CLOEXEC 支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_UNIX_EN > 0
#include "limits.h"
#include "sys/socket.h"
#include "sys/un.h"
#include "af_unix.h"
#include "af_unix_msg.h"
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
/*********************************************************************************************************
** 函数名称: __unix_dup
** 功能描述: 处理一个接收到的文件描述符
** 输　入  : pidSend   发送进程
**           iFdSend   发送进程的文件描述符
** 输　出  : dup 到本进程后的文件描述符
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __unix_dup (pid_t  pidSend, INT  iFdSend)
{
    INT     iDup;

#if LW_CFG_MODULELOADER_EN > 0
    if ((pidSend == 0) || (__PROC_GET_PID_CUR() == 0)) {                /*  不允许与内核交互文件描述符  */
        return  (PX_ERROR);
    }
    if (pidSend == __PROC_GET_PID_CUR()) {
        iDup = dup(iFdSend);                                            /*  同一个进程中 dup 一次       */
    } else {
        iDup = vprocIoFileDupFrom(pidSend, iFdSend);                    /*  从发送进程 dup 到本进程     */
        vprocIoFileRefDecByPid(pidSend, iFdSend);                       /*  减少发送进程对于此文件的引用*/
    }
#else
    iDup = dup(iFdSend);                                                /*  内核中做一次 dup            */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */

    return  (iDup);
}
/*********************************************************************************************************
** 函数名称: __unix_rmsg_proc
** 功能描述: 处理一个接收到的 msg
** 输　入  : pvMsgEx   扩展消息
**           uiLenEx   扩展消息长度
**           pidSend   sender pid
**           flags     MSG_CMSG_CLOEXEC 支持.
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __unix_rmsg_proc (PVOID  pvMsgEx, socklen_t  uiLenEx, pid_t  pidSend, INT  flags)
{
    INT              i;
    struct cmsghdr  *pcmhdr;
    struct msghdr    msghdrBuf;

    if ((pvMsgEx == LW_NULL) || (uiLenEx < sizeof(struct cmsghdr))) {
        return;
    }
    
    lib_bzero(&msghdrBuf, sizeof(struct msghdr));
    
    msghdrBuf.msg_control    = pvMsgEx;
    msghdrBuf.msg_controllen = uiLenEx;
    
    pcmhdr = CMSG_FIRSTHDR(&msghdrBuf);
    while (pcmhdr) {
        if (pcmhdr->cmsg_level == SOL_SOCKET) {
            if (pcmhdr->cmsg_type == SCM_RIGHTS) {                      /*  传送文件描述符              */
                INT  *iFdArry = (INT *)CMSG_DATA(pcmhdr);
                INT   iNum = (pcmhdr->cmsg_len - CMSG_LEN(0)) / sizeof(INT);
                for (i = 0; i < iNum; i++) {                            
                    iFdArry[i] = __unix_dup(pidSend, iFdArry[i]);       /*  记录 dup 到本进程的 fd      */
                    if ((flags & MSG_CMSG_CLOEXEC) && (iFdArry[i] >= 0)) {
                        API_IosFdSetCloExec(iFdArry[i], FD_CLOEXEC);
                    }
                }
            }
        }
        pcmhdr = CMSG_NXTHDR(&msghdrBuf, pcmhdr);
    }
}
/*********************************************************************************************************
** 函数名称: __unix_flight 
** 功能描述: 使一个文件描述符在飞行
** 输　入  : pidSender sender pid
**           iFd       要发送的文件描述符
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static  VOID  __unix_flight (pid_t  pidSend, INT  iFd)
{
#if LW_CFG_MODULELOADER_EN > 0
    if (pidSend == 0) {
        return;
    }
    vprocIoFileRefIncByPid(pidSend, iFd);                               /*  增加发送进程对于此文件的引用*/
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
}
/*********************************************************************************************************
** 函数名称: __unix_flight 
** 功能描述: 使一个文件描述符降落
** 输　入  : pidSender sender pid
**           iFd       要发送的文件描述符
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static  VOID  __unix_unflight (pid_t  pidSend, INT  iFd)
{
#if LW_CFG_MODULELOADER_EN > 0
    if (pidSend == 0) {
        return;
    }
    vprocIoFileRefDecByPid(pidSend, iFd);                               /*  减少发送进程对于此文件的引用*/
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
}
/*********************************************************************************************************
** 函数名称: __unix_smsg_proc
** 功能描述: 处理一个发送的 msg
** 输　入  : pafunixRecver  unix 接收节点
**           pvMsgEx        扩展消息
**           uiLenEx        扩展消息长度
**           pidSend        sender pid
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __unix_smsg_proc (AF_UNIX_T  *pafunixRecver, PVOID  pvMsgEx, socklen_t  uiLenEx, pid_t  pidSend)
{
    INT              i;
    struct cmsghdr  *pcmhdr;
    struct msghdr    msghdrBuf;
    socklen_t        uiTotalLen = 0;

    if ((pvMsgEx == LW_NULL) || (uiLenEx < sizeof(struct cmsghdr))) {
        return  (PX_ERROR);
    }
    
    lib_bzero(&msghdrBuf, sizeof(struct msghdr));
    
    msghdrBuf.msg_control    = pvMsgEx;
    msghdrBuf.msg_controllen = uiLenEx;
    
    pcmhdr = CMSG_FIRSTHDR(&msghdrBuf);                                 /*  遍历消息确认发送消息的合法性*/
    while (pcmhdr) {
        uiTotalLen += pcmhdr->cmsg_len;
        if (uiTotalLen > uiLenEx) {                                     /*  cmsghdr->cmsg_len 设置有错  */
            _ErrorHandle(EMSGSIZE);                                     /*  扩展消息内部大小错误        */
            return  (PX_ERROR);
        }
        if (pcmhdr->cmsg_level == SOL_SOCKET) {
            if ((pcmhdr->cmsg_type == SCM_CREDENTIALS) ||               /*  Linux 进程凭证              */
                (pcmhdr->cmsg_type == SCM_CRED)) {                      /*  BSD 进程凭证                */
                if (pafunixRecver->UNIX_iPassCred == 0) {               /*  接收方没有开启对应的使能位  */
                    _ErrorHandle(EINVAL);
                    return  (PX_ERROR);
                }
            }
        }
        pcmhdr = CMSG_NXTHDR(&msghdrBuf, pcmhdr);
    }
    
    pcmhdr = CMSG_FIRSTHDR(&msghdrBuf);                                 /*  遍历消息预处理发送的消息    */
    while (pcmhdr) {
        if (pcmhdr->cmsg_level == SOL_SOCKET) {
            if (pcmhdr->cmsg_type == SCM_RIGHTS) {                      /*  传送文件描述符              */
                INT  *iFdArry = (INT *)CMSG_DATA(pcmhdr);
                INT   iNum = (pcmhdr->cmsg_len - CMSG_LEN(0)) / sizeof(INT);
                for (i = 0; i < iNum; i++) {
                    __unix_flight(pidSend, iFdArry[i]);
                }
            
            } else if (pcmhdr->cmsg_type == SCM_CREDENTIALS) {          /*  Linux 进程凭证              */
                struct ucred *pucred = (struct ucred *)CMSG_DATA(pcmhdr);
                pucred->pid = __PROC_GET_PID_CUR();
                if (geteuid() != 0) {                                   /*  非特权用户                  */
                    pucred->uid = geteuid();
                    if ((pucred->gid != getegid()) &&
                        (pucred->gid != getgid())) {                    /*  判断设置是否正确            */
                        pucred->gid = getegid();
                    }
                }
            
            } else if (pcmhdr->cmsg_type == SCM_CRED) {                 /*  BSD 进程凭证                */
                struct cmsgcred *pcmcred = (struct cmsgcred *)CMSG_DATA(pcmhdr);
                pcmcred->cmcred_pid  = __PROC_GET_PID_CUR();
                pcmcred->cmcred_uid  = getuid();
                pcmcred->cmcred_euid = geteuid();
                pcmcred->cmcred_gid  = getgid();
                pcmcred->cmcred_ngroups = (short)getgroups(CMGROUP_MAX, pcmcred->cmcred_groups);
            }
        }
        pcmhdr = CMSG_NXTHDR(&msghdrBuf, pcmhdr);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __unix_smsg_proc
** 功能描述: 发送的 msg 失败或者消息还没有被接受就被删除了
** 输　入  : pvMsgEx   扩展消息
**           uiLenEx   扩展消息长度
**           pidSend   sender pid
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __unix_smsg_unproc (PVOID  pvMsgEx, socklen_t  uiLenEx, pid_t  pidSend)
{
    INT              i;
    struct cmsghdr  *pcmhdr;
    struct msghdr    msghdrBuf;

    if ((pvMsgEx == LW_NULL) || (uiLenEx < sizeof(struct cmsghdr))) {
        return;
    }
    
    lib_bzero(&msghdrBuf, sizeof(struct msghdr));
    
    msghdrBuf.msg_control    = pvMsgEx;
    msghdrBuf.msg_controllen = uiLenEx;
    
    pcmhdr = CMSG_FIRSTHDR(&msghdrBuf);
    while (pcmhdr) {
        if (pcmhdr->cmsg_level == SOL_SOCKET) {
            if (pcmhdr->cmsg_type == SCM_RIGHTS) {                      /*  传送文件描述符              */
                INT  *iFdArry = (INT *)CMSG_DATA(pcmhdr);
                INT   iNum = (pcmhdr->cmsg_len - CMSG_LEN(0)) / sizeof(INT);
                for (i = 0; i < iNum; i++) {
                    __unix_unflight(pidSend, iFdArry[i]);
                }
            
            }
        }
        pcmhdr = CMSG_NXTHDR(&msghdrBuf, pcmhdr);
    }
}

#endif                                                                  /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_NET_UNIX_EN > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
