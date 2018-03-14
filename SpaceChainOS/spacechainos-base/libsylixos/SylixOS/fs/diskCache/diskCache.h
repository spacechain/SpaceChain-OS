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
** 文   件   名: diskCache.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 11 月 26 日
**
** 描        述: 磁盘高速缓冲控制器.
*********************************************************************************************************/

#ifndef __DISKCACHE_H
#define __DISKCACHE_H

/*********************************************************************************************************
  注意:
  
  如果使用 CACHE, 请保证与物理设备的一一对应, (多个逻辑分区将共享一个 CACHE)
  
  DISK CACHE 的使用:

  使用 DISK CACHE 的最大目的就是提高慢速块设备 IO 的访问效率, 利用系统内存作为数据的缓冲, 
  使用 DISK CACHE 的最大问题就在于磁盘数据与缓冲数据的不同步性, 同步数据可以调用 ioctl(..., FIOFLUSH, ...)
  实现.
  
  pblkDev = xxxBlkDevCreate(...);
  diskCacheCreate(pblkDev, ..., &pCacheBlk);
  ...(如果存在多磁盘分区, 详见: diskPartition.h)
  fatFsDevCreate(pVolName, pCacheBlk);
  
  ...操作设备
  
  umount(pVolName);
  ...(如果存在多磁盘分区, 详见: diskPartition.h)
  diskCacheDelete(pCacheBlk);
  xxxBlkDevDelete(pblkDev);
  
  推荐使用 oem 磁盘操作库.
*********************************************************************************************************/

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)

/*********************************************************************************************************
  ioctl 附加命令
*********************************************************************************************************/

#define LW_BLKD_DISKCACHE_GET_OPT       LW_OSIOR('b', 150, INT)         /*  获取 CACHE 选项             */
#define LW_BLKD_DISKCACHE_SET_OPT       LW_OSIOD('b', 151, INT)         /*  设置 CACHE 选项             */
#define LW_BLKD_DISKCACHE_INVALID       LW_OSIO( 'b', 152)              /*  使 CACHE 回写并全部不命中   */
#define LW_BLKD_DISKCACHE_RAMFLUSH      LW_OSIOD('b', 153, ULONG)       /*  随机回写一些脏扇区          */
#define LW_BLKD_DISKCACHE_CALLBACKFUNC  LW_OSIOD('b', 154, FUNCPTR)     /*  文件系统回调函数            */
#define LW_BLKD_DISKCACHE_CALLBACKARG   LW_OSIOD('b', 155, PVOID)       /*  回调函数参数                */

/*********************************************************************************************************
  DISK CACHE 描述
  
  注意: 
    
    1. 当设备支持并发读写操作时, DCATTR_bParallel = LW_TRUE, 否则为 LW_FALSE
    2. DCATTR_iPipeline >= 0 && < LW_CFG_DISKCACHE_MAX_PIPELINE.
       DCATTR_iPipeline == 0 表示不使用管线并发技术.
    3. DCATTR_iMsgCount 最小为 DCATTR_iPipeline, 可以为 DCATTR_iPipeline 2 ~ 8 倍.
*********************************************************************************************************/

typedef struct {
    PVOID           DCATTR_pvCacheMem;                                  /*  扇区缓存地址                */
    size_t          DCATTR_stMemSize;                                   /*  扇区缓存大小                */
    BOOL            DCATTR_bCacheCoherence;                             /*  缓冲区需要 CACHE 一致性保障 */
    INT             DCATTR_iMaxRBurstSector;                            /*  磁盘猝发读的最大扇区数      */
    INT             DCATTR_iMaxWBurstSector;                            /*  磁盘猝发写的最大扇区数      */
    INT             DCATTR_iMsgCount;                                   /*  管线消息队列缓存个数        */
    INT             DCATTR_iPipeline;                                   /*  处理管线线程数量            */
    BOOL            DCATTR_bParallel;                                   /*  是否支持并行读写            */
    ULONG           DCATTR_ulReserved[8];                               /*  保留                        */
} LW_DISKCACHE_ATTR;
typedef LW_DISKCACHE_ATTR  *PLW_DISKCACHE_ATTR;

/*********************************************************************************************************
  API
*********************************************************************************************************/

LW_API ULONG  API_DiskCacheCreate(PLW_BLK_DEV   pblkdDisk, 
                                  PVOID         pvDiskCacheMem, 
                                  size_t        stMemSize, 
                                  INT           iMaxBurstSector,
                                  PLW_BLK_DEV  *ppblkDiskCache);

LW_API ULONG  API_DiskCacheCreateEx(PLW_BLK_DEV   pblkdDisk, 
                                    PVOID         pvDiskCacheMem, 
                                    size_t        stMemSize, 
                                    INT           iMaxRBurstSector,
                                    INT           iMaxWBurstSector,
                                    PLW_BLK_DEV  *ppblkDiskCache);

LW_API ULONG  API_DiskCacheCreateEx2(PLW_BLK_DEV          pblkdDisk, 
                                     PLW_DISKCACHE_ATTR   pdcattrl,
                                     PLW_BLK_DEV         *ppblkDiskCache);

LW_API INT    API_DiskCacheDelete(PLW_BLK_DEV   pblkdDiskCache);

LW_API INT    API_DiskCacheSync(PLW_BLK_DEV   pblkdDiskCache);

#define diskCacheCreate     API_DiskCacheCreate
#define diskCacheCreateEx   API_DiskCacheCreateEx
#define diskCacheCreateEx2  API_DiskCacheCreateEx2
#define diskCacheDelete     API_DiskCacheDelete
#define diskCacheSync       API_DiskCacheSync

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
#endif                                                                  /*  __DISKCACHE_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
