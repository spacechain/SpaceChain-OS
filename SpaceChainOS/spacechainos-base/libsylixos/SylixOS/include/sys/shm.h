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
** 文   件   名: shm.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 03 月 13 日
**
** 描        述: XSI IPC sharememory
*********************************************************************************************************/

#ifndef __SYS_SHM_H
#define __SYS_SHM_H

#include "ipc.h"

/*********************************************************************************************************
 * [XSI] The unsigned integer type used for the number of current attaches
 * that MUST be able to store values at least as large as a type unsigned
 * short.
*********************************************************************************************************/

typedef unsigned short  shmatt_t;

/*********************************************************************************************************
 * Possible flag values which may be OR'ed into the third argument to
 * shmat()
*********************************************************************************************************/

#define SHM_RDONLY      010000                          /* [XSI] Attach read-only (else read-write)     */
#define SHM_RND         020000                          /* [XSI] Round attach address to SHMLBA         */

/*********************************************************************************************************
 * lock sharememory in memory.
 * shmctl()
*********************************************************************************************************/

#define SHM_LOCK        11                              /* lock segment (root only)                     */
#define SHM_UNLOCK      12                              /* unlock segment (root only)                   */

/*********************************************************************************************************
 * This value is symbolic, and generally not expected to be sed by user
 * programs directly, although such ise is permitted by the standard.  Its
 * value in our implementation is equal to the number of bytes per page.
 *
 * NOTE:    We DO NOT obtain this value from the appropriate system
 *        headers at this time, to avoid the resulting namespace
 *        pollution, which is why we discourages its use.
*********************************************************************************************************/

#define SHMLBA          LW_CFG_VMM_PAGE_SIZE            /* [XSI] Segment low boundary address multiple  */

/*********************************************************************************************************
 "official" access mode definitions; somewhat braindead since you have
 to specify (SHM_* >> 3) for group and (SHM_* >> 6) for world permissions 
*********************************************************************************************************/

#define SHM_R           (IPC_R)
#define SHM_W           (IPC_W)

struct shmid_ds {
    struct ipc_perm     shm_perm;                       /* operation permission structure               */
    size_t              shm_segsz;                      /* size of segment in bytes                     */
    pid_t               shm_lpid;                       /* process ID of last shared memory op          */
    pid_t               shm_cpid;                       /* process ID of creator                        */
    shmatt_t            shm_nattch;                     /* number of current attaches                   */
    time_t              shm_atime;                      /* time of last shmat()                         */
    time_t              shm_dtime;                      /* time of last shmdt()                         */
    time_t              shm_ctime;                      /* time of last change by shmctl()              */
    void               *shm_internal;                   /* sysv stupidity                               */
};

#ifdef __SYLIXOS_KERNEL

/*********************************************************************************************************
 * System 5 style catch-all structure for shared memory constants that
 * might be of interest to user programs.  Do we really want/need this?
*********************************************************************************************************/

struct shminfo {
    int                 shmmax,                         /* max shared memory segment size (bytes)       */
                        shmmin,                         /* min shared memory segment size (bytes)       */
                        shmmni,                         /* max number of shared memory identifiers      */
                        shmseg,                         /* max shared memory segments per process       */
                        shmall;                         /* max amount of shared memory (pages)          */
};
extern struct shminfo    shminfo;

#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  XSI IPC sharememory API
*********************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

void *shmat(int, const void *, int);
int   shmctl(int, int, struct shmid_ds *);
int   shmdt(const void *);
int   shmget(key_t, size_t, int);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __SYS_SHM_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
