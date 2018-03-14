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
** 文   件   名: sem.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 03 月 13 日
**
** 描        述: XSI IPC semaphore
*********************************************************************************************************/

#ifndef __SYS_SEM_H
#define __SYS_SEM_H

#include "ipc.h"
#include "time.h"

/*********************************************************************************************************
 commands for semctl
*********************************************************************************************************/

#define GETNCNT         3                               /* Return the value of semncnt {READ}           */
#define GETPID          4                               /* Return the value of sempid {READ}            */
#define GETVAL          5                               /* Return the value of semval {READ}            */
#define GETALL          6                               /* Return semvals into arg.array {READ}         */
#define GETZCNT         7                               /* Return the value of semzcnt {READ}           */
#define SETVAL          8                               /* Set the value of semval to arg.val {ALTER}   */
#define SETALL          9                               /* Set semvals from arg.array {ALTER}           */

/*********************************************************************************************************
 Permissions
*********************************************************************************************************/

#define SEM_A           0200                            /* alter permission                             */
#define SEM_R           0400                            /* read permission                              */

/*********************************************************************************************************
 semid_ds
*********************************************************************************************************/

struct semid_ds {
    struct ipc_perm     sem_perm;                       /* operation permission structure               */
    u_short             sem_nsems;                      /* number of semaphores in set                  */
    time_t              sem_otime;                      /* last semop^) time                            */
    time_t              sem_ctime;                      /* last time changed by semctl()                */
    long                sem_pad[4];
};

/*********************************************************************************************************
 A semaphore; this is an anonymous structure, not for external use 
*********************************************************************************************************/

struct sem {
    unsigned short      semval;                         /* semaphore value                              */
    pid_t               sempid;                         /* pid of last operation                        */
    unsigned short      semncnt;                        /* # awaiting semval > cval                     */
    unsigned short      semzcnt;                        /* # awaiting semval == 0                       */
};

/*********************************************************************************************************
 semop's sops parameter structure
*********************************************************************************************************/

struct sembuf {
    u_short             sem_num;                        /* semaphore #                                  */
    short               sem_op;                         /* semaphore operation                          */
    short               sem_flg;                        /* operation flags                              */
};

/*********************************************************************************************************
 Possible flag values for sem_flg
*********************************************************************************************************/

#define SEM_UNDO        010000                          /* [XSI] Set up adjust on exit entry            */

/*********************************************************************************************************
 System imposed limit on the value of the third parameter to semop().
 This is arbitrary, and the standards unfortunately do not provide a
 way for user applications to retrieve this value (e.g. via sysconf()
 or from a manifest value in <unistd.h>).  The value shown here is
 informational, and subject to change in future revisions.
*********************************************************************************************************/

#define MAX_SOPS        5                               /* maximum # of sembuf's per semop call         */

/*********************************************************************************************************
 Union used as the fourth argument to semctl() in all cases.  Specific
 member values are used for different values of the third parameter:
 
 Command                                        Member
 -------------------------------------------    ------
 GETALL, SETALL                                 array
 SETVAL                                         val
 IPC_STAT, IPC_SET                              buf
 
 The union definition is intended to be defined by the user application
 in conforming applications; it is provided here for two reasons:
 
 1) Historical source compatability for non-conforming applications
    expecting this header to declare the union type on their behalf
 
 2) Documentation; specifically, 64 bit applications that do not pass
    this structure for 'val', or, alternately, a 64 bit type, will
    not function correctly
*********************************************************************************************************/

union semun {
    int                 val;                                            /* value for SETVAL             */
    struct semid_ds    *buf;                                            /* buffer for IPC_STAT & IPC_SET*/
    unsigned short     *array;                                          /* array for GETALL & SETALL    */
};
typedef union semun     semun_t;

/*********************************************************************************************************
  XSI IPC semaphore API
*********************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

int   semctl(int, int, int, ...);
int   semget(key_t, int, int);
int   semop(int, struct sembuf *, size_t);
int   semtimedop(int, struct sembuf *, size_t, const struct timespec *);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __SYS_SEM_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
