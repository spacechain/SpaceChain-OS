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
** 文   件   名: nandRCache.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 07 月 19 日
**
** 描        述: nand flash 读 CACHE. 
*********************************************************************************************************/

#ifndef __NANDRCACHE_H
#define __NANDRCACHE_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_YAFFS_EN > 0)

typedef struct {
    LW_LIST_LINE             NRCACHEN_lineManage;                       /*  空闲链表 或 hash            */
    LW_LIST_RING             NRCACHEN_ringLRU;                          /*  LRU 链                      */
    ULONG                    NRCACHEN_ulChunkNo;                        /*  扇区号                      */
    caddr_t                  NRCACHEN_pcChunk;                          /*  数据区缓冲                  */
    caddr_t                  NRCACHEN_pcSpare;                          /*  扩展区缓冲                  */
} LW_NRCACHE_NODE;
typedef LW_NRCACHE_NODE     *PLW_NRCACHE_NODE;                          /*  缓冲节点                    */

typedef struct {
    PLW_NRCACHE_NODE         NRCACHE_pnrcachenBuffer;                   /*  nand flash 所有的缓冲       */
    ULONG                    NRCACHE_ulPagePerBlock;                    /*  每一个块的页面个数          */
    ULONG                    NRCACHE_ulnCacheNode;                      /*  CACHE 中包含的节点个数      */
    INT                      NRCACHE_iHashSize;                         /*  hash 表大小                 */
    PLW_LIST_LINE           *NRCACHE_pplineHash;                        /*  搜索 hash 表                */
    PLW_LIST_RING            NRCACHE_pringLRU;                          /*  有效点中最近最少使用表      */
    PLW_LIST_LINE            NRCACHE_plineFree;                         /*  空闲表头                    */
} LW_NRCACHE_CB;
typedef LW_NRCACHE_CB       *PLW_NRCACHE_CB;                            /*  nand read cache 控制块      */

/*********************************************************************************************************
  NAND CACHE API
*********************************************************************************************************/

LW_API ULONG                 API_NandRCacheCreate(PVOID              pvNandRCacheMem, 
                                                  size_t             stMemSize,
                                                  ULONG              ulPageSize,
                                                  ULONG              ulSpareSize,
                                                  ULONG              ulPagePerBlock,
                                                  PLW_NRCACHE_CB    *ppnrcache);

LW_API ULONG                 API_NandRCacheDelete(PLW_NRCACHE_CB    pnrcache);

LW_API PLW_NRCACHE_NODE      API_NandRCacheNodeGet(PLW_NRCACHE_CB  pnrcache, ULONG  ulChunkNo);
 
LW_API VOID                  API_NandRCacheNodeFree(PLW_NRCACHE_CB  pnrcache, ULONG  ulChunkNo);
 
LW_API PLW_NRCACHE_NODE      API_NandRCacheNodeAlloc(PLW_NRCACHE_CB  pnrcache, ULONG  ulChunkNo);

LW_API VOID                  API_NandRCacheBlockFree(PLW_NRCACHE_CB  pnrcache, ULONG  ulBlockNo);

#define nandRCacheCreate     API_NandRCacheCreate
#define nandRCacheDelete     API_NandRCacheDelete
#define nandRCacheNodeGet    API_NandRCacheNodeGet
#define nandRCacheNodeFree   API_NandRCacheNodeFree
#define nandRCacheNodeAlloc  API_NandRCacheNodeAlloc
#define nandRCacheBlockFree  API_NandRCacheBlockFree

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_YAFFS_EN > 0)       */
#endif                                                                  /*  __NANDRCACHE_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
