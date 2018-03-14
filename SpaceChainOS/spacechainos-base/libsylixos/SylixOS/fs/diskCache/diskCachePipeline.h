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
** 文   件   名: diskCachePipeline.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 07 月 25 日
**
** 描        述: 磁盘高速缓冲并发写管线.
*********************************************************************************************************/

#ifndef __DISKCACHEPIPELINE_H
#define __DISKCACHEPIPELINE_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)

/*********************************************************************************************************
  内部函数
*********************************************************************************************************/
INT   __diskCacheWpCreate(PLW_DISKCACHE_CB  pdiskc,
                          PLW_DISKCACHE_WP  pwp, 
                          BOOL              bCacheCoherence,
                          BOOL              bParallel,
                          INT               iPipeline, 
                          INT               iMsgCount,
                          INT               iMaxRBurstSector,
                          INT               iMaxWBurstSector,
                          ULONG             ulBytesPerSector);
INT   __diskCacheWpDelete(PLW_DISKCACHE_WP  pwp);
PVOID __diskCacheWpGetBuffer(PLW_DISKCACHE_WP  pwp, BOOL bRead);
VOID  __diskCacheWpPutBuffer(PLW_DISKCACHE_WP  pwp, PVOID  pvBuffer);
INT   __diskCacheWpRead(PLW_DISKCACHE_CB  pdiskc,
                        PLW_BLK_DEV       pblkd,
                        PVOID             pvBuffer,
                        ULONG             ulStartSector,
                        ULONG             ulNSector);
INT   __diskCacheWpWrite(PLW_DISKCACHE_CB  pdiskc,
                         PLW_BLK_DEV       pblkdDisk,
                         PVOID             pvBuffer,
                         ULONG             ulStartSector,
                         ULONG             ulNSector);
VOID  __diskCacheWpSync(PLW_DISKCACHE_WP  pwp);

#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_DISKCACHE_EN > 0     */
#endif                                                                  /*  __DISKCACHEPIPELINE_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
