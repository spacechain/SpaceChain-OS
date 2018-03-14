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
** 文   件   名: msg.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 03 月 13 日
**
** 描        述: XSI IPC message
*********************************************************************************************************/

#ifndef __SYS_MSG_H
#define __SYS_MSG_H

#include "ipc.h"

/*********************************************************************************************************
 [XSI] Used for the number of messages in the message queue 
*********************************************************************************************************/

typedef unsigned long       msgqnum_t;

/*********************************************************************************************************
 [XSI] Used for the number of bytes allowed in a message queue 
*********************************************************************************************************/

typedef unsigned long       msglen_t;

/*********************************************************************************************************
 Possible values for the fifth parameter to msgrcv(), in addition to the
 IPC_NOWAIT flag, which is permitted.
*********************************************************************************************************/

#define MSG_NOERROR         010000                      /* [XSI] No error if big message                */

struct msqid_ds {
    struct ipc_perm         msg_perm;                   /* msg queue permission bits                    */
    msgqnum_t               msg_qnum;                   /* number of msgs in the queue                  */
    msglen_t                msg_qbytes;                 /* max # of bytes on the queue                  */
    pid_t                   msg_lspid;                  /* pid of last msgsnd()                         */
    pid_t                   msg_lrpid;                  /* pid of last msgrcv()                         */
    time_t                  msg_stime;                  /* time of last msgsnd()                        */
    time_t                  msg_rtime;                  /* time of last msgrcv()                        */
    time_t                  msg_ctime;                  /* time of last msgctl()                        */
    long                    msg_pad[4];
};

/*********************************************************************************************************
 Example structure describing a message whose address is to be passed as
 the second argument to the functions msgrcv() and msgsnd().  The only
 actual hard requirement is that the first field be of type long, and
 contain the message type.  The user is encouraged to define their own
 application specific structure; this definition is included solely for
 backward compatability with existing source code.
*********************************************************************************************************/

struct mymsg {
    long                    mtype;                      /* message type (+ve integer)                   */
    char                    mtext[1];                   /* message body                                 */
};

/*********************************************************************************************************
 Based on the configuration parameters described in an SVR2 (yes, two)
 config(1m) man page.
 
 Each message is broken up and stored in segments that are msgssz bytes
 long.  For efficiency reasons, this should be a power of two.  Also,
 it doesn't make sense if it is less than 8 or greater than about 256.
 Consequently, msginit in kern/sysv_msg.c checks that msgssz is a power of
 two between 8 and 1024 inclusive (and panic's if it isn't).
*********************************************************************************************************/

struct msginfo {
    int                     msgmax,                     /* max chars in a message                       */
                            msgmni,                     /* max message queue identifiers                */
                            msgmnb,                     /* max chars in a queue                         */
                            msgtql,                     /* max messages in system                       */
                            msgssz,                     /* size of a message segment (see notes above)  */
                            msgseg;                     /* number of message segments                   */
};

#ifdef __SYLIXOS_KERNEL

extern struct msginfo       msginfo;

#ifndef MSGSSZ
#define MSGSSZ              8                           /* Each segment must be 2^N long                */
#endif
#ifndef MSGSEG
#define MSGSEG              2048                        /* must be less than 32767                      */
#endif
#define MSGMAX              (MSGSSZ * MSGSEG)
#ifndef MSGMNB
#define MSGMNB              2048                        /* max # of bytes in a queue                    */
#endif
#ifndef MSGMNI
#define MSGMNI              40
#endif
#ifndef MSGTQL
#define MSGTQL              40
#endif

/*********************************************************************************************************
 macros to convert between msqid_ds's and msqid's.
 (specific to this implementation)
*********************************************************************************************************/

#define MSQID(ix,ds)        ((ix) & 0xffff | (((ds).msg_perm.seq << 16) & 0xffff0000))
#define MSQID_IX(id)        ((id) & 0xffff)
#define MSQID_SEQ(id)       (((id) >> 16) & 0xffff)

#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  XSI IPC message API
*********************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

int     msgsys(int, ...);
int     msgctl(int, int, struct msqid_ds *);
int     msgget(key_t, int);
int     msgsnd(int, const void *, size_t, int);
ssize_t msgrcv(int, void *, size_t, long, int);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __SYS_MSG_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
