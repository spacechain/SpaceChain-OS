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
** 文   件   名: vmmMmap.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2015 年 05 月 26 日
**
** 描        述: 进程内存映射管理.
*********************************************************************************************************/

#ifndef __VMMMMAP_H
#define __VMMMMAP_H

/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

#ifdef __SYLIXOS_KERNEL
/*********************************************************************************************************
  进程虚拟内存空间管理表
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            MAPN_lineManage;                            /*  所有映射节点双向链表        */
    LW_LIST_LINE            MAPN_lineVproc;                             /*  进程内按照顺序排序管理      */
    
    PVOID                   MAPN_pvAddr;                                /*  起始地址                    */
    size_t                  MAPN_stLen;                                 /*  内存长度                    */
    ULONG                   MAPN_ulFlag;                                /*  内存属性                    */
    
    INT                     MAPN_iFd;                                   /*  关联文件                    */
    mode_t                  MAPN_mode;                                  /*  文件mode                    */
    off_t                   MAPN_off;                                   /*  文件映射偏移量              */
    off_t                   MAPN_offFSize;                              /*  文件大小                    */
    dev_t                   MAPN_dev;                                   /*  文件描述符对应 dev          */
    ino64_t                 MAPN_ino64;                                 /*  文件描述符对应 inode        */
    
    INT                     MAPN_iFlags;                                /*  SHARED / PRIVATE            */
    pid_t                   MAPN_pid;                                   /*  映射进程的进程号            */
} LW_VMM_MAP_NODE;
typedef LW_VMM_MAP_NODE    *PLW_VMM_MAP_NODE;

/*********************************************************************************************************
  内部函数
*********************************************************************************************************/

VOID            __vmmMapInit(VOID);                                     /*  初始化                      */

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  API
*********************************************************************************************************/

#define LW_VMM_MAP_FAILED   ((PVOID)PX_ERROR)

LW_API PVOID    API_VmmMmap(PVOID  pvAddr, size_t  stLen, INT  iFlags, 
                            ULONG  ulFlag, INT  iFd, off_t  off);
                            
LW_API PVOID    API_VmmMremap(PVOID  pvAddr, size_t stOldSize, size_t stNewSize, INT  iMoveEn);

LW_API INT      API_VmmMunmap(PVOID  pvAddr, size_t  stLen);

LW_API INT      API_VmmMsync(PVOID  pvAddr, size_t  stLen, INT  iInval);

LW_API VOID     API_VmmMmapShow(VOID);

LW_API INT      API_VmmMmapPCount(pid_t  pid, size_t  *pstPhySize);

#if LW_CFG_MODULELOADER_EN > 0
LW_API VOID     API_VmmMmapReclaim(pid_t  pid);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#endif                                                                  /*  __VMMMMAP_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
