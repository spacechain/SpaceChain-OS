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
** 文   件   名: diskCacheLib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 10 月 10 日
**
** 描        述: 磁盘高速缓冲管理库. 
*********************************************************************************************************/

#ifndef __DISKCACHELIB_H
#define __DISKCACHELIB_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
extern LW_OBJECT_HANDLE     _G_ulDiskCacheListLock;                     /*  链表锁                      */
extern PLW_LIST_LINE        _G_plineDiskCacheHeader;                    /*  链表头                      */
/*********************************************************************************************************
  DISK CACHE NODE LOCK
*********************************************************************************************************/
#define __LW_DISKCACHE_LOCK(pdiskc)     \
        API_SemaphoreMPend(pdiskc->DISKC_hDiskCacheLock, LW_OPTION_WAIT_INFINITE)
#define __LW_DISKCACHE_UNLOCK(pdiskc)   \
        API_SemaphoreMPost(pdiskc->DISKC_hDiskCacheLock)
/*********************************************************************************************************
  DISK CACHE NODE OP
*********************************************************************************************************/
#define __LW_DISKCACHE_NODE_READ                0                       /*  节点读                      */
#define __LW_DISKCACHE_NODE_WRITE               1                       /*  节点写                      */
/*********************************************************************************************************
  DISK CACHE NODE MACRO
*********************************************************************************************************/
#define __LW_DISKCACHE_IS_VALID(pdiskn)         ((pdiskn)->DISKN_iStatus & 0x01)
#define __LW_DISKCACHE_IS_DIRTY(pdiskn)         ((pdiskn)->DISKN_iStatus & 0x02)

#define __LW_DISKCACHE_SET_VALID(pdiskn)        ((pdiskn)->DISKN_iStatus |= 0x01)
#define __LW_DISKCACHE_SET_DIRTY(pdiskn)        ((pdiskn)->DISKN_iStatus |= 0x02)

#define __LW_DISKCACHE_CLR_VALID(pdiskn)        ((pdiskn)->DISKN_iStatus &= ~0x01)
#define __LW_DISKCACHE_CLR_DIRTY(pdiskn)        ((pdiskn)->DISKN_iStatus &= ~0x02)
/*********************************************************************************************************
  DISK CACHE NODE
*********************************************************************************************************/
typedef struct {
    LW_LIST_RING            DISKN_ringLru;                              /*  LRU 表节点                  */
    LW_LIST_LINE            DISKN_lineHash;                             /*  HASH 表节点                 */
    
    ULONG                   DISKN_ulSectorNo;                           /*  缓冲的扇区号                */
    INT                     DISKN_iStatus;                              /*  节点状态                    */
    caddr_t                 DISKN_pcData;                               /*  扇区数据缓冲                */
    
    UINT64                  DISKN_u64FsKey;                             /*  文件系统自定义数据          */
} LW_DISKCACHE_NODE;
typedef LW_DISKCACHE_NODE  *PLW_DISKCACHE_NODE;
/*********************************************************************************************************
  DISK CACHE 并发写管线
*********************************************************************************************************/
typedef struct {
    ULONG                   DISKCWPM_ulStartSector;                     /*  起始扇区                    */
    ULONG                   DISKCWPM_ulNSector;                         /*  扇区数量                    */
    PVOID                   DISKCWPM_pvBuffer;                          /*  扇区缓冲                    */
} LW_DISKCACHE_WPMSG;
typedef LW_DISKCACHE_WPMSG *PLW_DISKCACHE_WPMSG;

typedef struct {
    BOOL                    DISKCWP_bExit;                              /*  是否需要退出                */
    BOOL                    DISKCWP_bCacheCoherence;                    /*  CACHE 一致性标志            */
    BOOL                    DISKCWP_bParallel;                          /*  并行化读写支持              */
    
    INT                     DISKCWP_iPipeline;                          /*  写管线线程数                */
    INT                     DISKCWP_iMsgCount;                          /*  写消息缓冲个数              */
    
    PVOID                   DISKCWP_pvRBurstBuffer;                     /*  管线缓存                    */
    PVOID                   DISKCWP_pvWBurstBuffer;                     /*  管线缓存                    */
    
    LW_OBJECT_HANDLE        DISKCWP_hMsgQueue;                          /*  管线刷新队列                */
    LW_OBJECT_HANDLE        DISKCWP_hCounter;                           /*  计数信号量                  */
    LW_OBJECT_HANDLE        DISKCWP_hPart;                              /*  管线缓存管理                */
    LW_OBJECT_HANDLE        DISKCWP_hSync;                              /*  排空信号                    */
    LW_OBJECT_HANDLE        DISKCWP_hDev;                               /*  非并发设备锁                */
    LW_OBJECT_HANDLE        DISKCWP_hWThread[LW_CFG_DISKCACHE_MAX_PIPELINE];
                                                                        /*  管线写任务表                */
} LW_DISKCACHE_WP;
typedef LW_DISKCACHE_WP    *PLW_DISKCACHE_WP;
/*********************************************************************************************************
  DISK CACHE NODE
*********************************************************************************************************/
typedef struct {
    LW_BLK_DEV              DISKC_blkdCache;                            /*  DISK CACHE 的 BLK IO 控制块 */
    PLW_BLK_DEV             DISKC_pblkdDisk;                            /*  被缓冲 BLK IO 控制块地址    */
    LW_LIST_LINE            DISKC_lineManage;                           /*  背景线程管理链表            */
    
    LW_OBJECT_HANDLE        DISKC_hDiskCacheLock;                       /*  DISK CACHE 操作锁           */
    INT                     DISKC_iCacheOpt;                            /*  CACHE 工作选项              */
    
    ULONG                   DISKC_ulEndStector;                         /*  最后一个扇区的编号          */
    ULONG                   DISKC_ulBytesPerSector;                     /*  每一扇区字节数量            */
    
    ULONG                   DISKC_ulValidCounter;                       /*  有效的扇区数量              */
    ULONG                   DISKC_ulDirtyCounter;                       /*  需要回写的扇区数量          */
    
    INT                     DISKC_iMaxRBurstSector;                     /*  最大猝发读写扇区数量        */
    INT                     DISKC_iMaxWBurstSector;
    LW_DISKCACHE_WP         DISKC_wpWrite;                              /*  并发写管线                  */
    
    PLW_LIST_RING           DISKC_pringLruHeader;                       /*  LRU 表头                    */
    PLW_LIST_LINE          *DISKC_pplineHash;                           /*  HASH 表池                   */
    INT                     DISKC_iHashSize;                            /*  HASH 表大小                 */
    
    ULONG                   DISKC_ulNCacheNode;                         /*  CACHE 缓冲的节点数          */
    caddr_t                 DISKC_pcCacheNodeMem;                       /*  CACHE 节点链表首地址        */
    caddr_t                 DISKC_pcCacheMem;                           /*  CACHE 缓冲区                */
    PLW_DISKCACHE_NODE      DISKC_disknLuck;                            /*  幸运扇区节点                */
    
    VOIDFUNCPTR             DISKC_pfuncFsCallback;                      /*  文件系统回调函数            */
    PVOID                   DISKC_pvFsArg;                              /*  文件系统回调参数            */
} LW_DISKCACHE_CB;
typedef LW_DISKCACHE_CB    *PLW_DISKCACHE_CB;
/*********************************************************************************************************
  内部函数
*********************************************************************************************************/
PVOID   __diskCacheThread(PVOID  pvArg);
VOID    __diskCacheListAdd(PLW_DISKCACHE_CB   pdiskcDiskCache);
VOID    __diskCacheListDel(PLW_DISKCACHE_CB   pdiskcDiskCache);

#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_DISKCACHE_EN > 0     */
#endif                                                                  /*  __DISKCACHELIB_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
