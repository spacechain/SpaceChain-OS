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
** 文   件   名: nandRCache.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 07 月 19 日
**
** 描        述: nand flash 读 CACHE. 

** BUG:
2009.10.23  修正注释.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_YAFFS_DRV
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "nandRCache.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_YAFFS_EN > 0)
/*********************************************************************************************************
  注意:
  flash 磁盘读操作不会导致磁盘损坏, 只有写入或擦除操作时才有可能导致磁盘损坏. 
  
  1: 当写一个扇区时, 应该将相关的扇区 CACHE 设为读不命中 (API_NandRCacheNodeFree)
  2: 当擦出一个块时, 应该将块内所有扇区 CACHE 设为读不命中 (API_NandRCacheBlockFree)
      
  这样, 此算法就不会影响写平衡软件对坏块的管理.
*********************************************************************************************************/
/*********************************************************************************************************
  相关参数表
*********************************************************************************************************/
static const INT    _G_iNandRCacheHashSize[][2] = {
/*********************************************************************************************************
    CACHE SIZE      HASH SIZE
     (page)          (entry)
*********************************************************************************************************/
{         16,            8,         },
{         32,           16,         },
{         64,           32,         },
{        128,           64,         },
{        256,          128,         },
{        512,          256,         },
{       1024,          512,         },
{       2048,         1024,         },
{       4096,         2048,         },
{          0,         4096,         }
};
/*********************************************************************************************************
** 函数名称: API_NandRCacheCreate
** 功能描述: 创建一个 NAND FLASH READ CACHE.
** 输　入  : pvNandRCacheMem       缓存内存地址
**           stMemSize             缓存内存大小
**           ulPageSize            每个页面数据区大小
**           ulSpareSize           每个页面扩展区大小
**           ulPagePerBlock        每一块页面的个数
**           ppnrcache             返回创建成功的控制块
** 输　出  : ERROR 控制块
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_NandRCacheCreate (PVOID              pvNandRCacheMem, 
                             size_t             stMemSize,
                             ULONG              ulPageSize,
                             ULONG              ulSpareSize,
                             ULONG              ulPagePerBlock,
                             PLW_NRCACHE_CB    *ppnrcache)
{
             INT                 i;
             INT                 iErrLevel = 0;
             
    REGISTER PLW_NRCACHE_CB      pnrcache;
             PLW_NRCACHE_NODE    pnrcachenTemp;
             caddr_t             pcTemp;
             
             ULONG               ulNCacheNode;
             INT                 iHashSize;

    if (!pvNandRCacheMem || !stMemSize || !ulPageSize || 
        !ulSpareSize || !ulPagePerBlock || !ppnrcache) {                /*  参数错误                    */
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    ulNCacheNode = stMemSize / (ulPageSize + ulSpareSize);              /*  可以创建的 CACHE NODE 数量  */
    
    /*
     *  确定 HASH 表的大小
     */
    for (i = 0; ; i++) {
        if (_G_iNandRCacheHashSize[i][0] == 0) {
            iHashSize = _G_iNandRCacheHashSize[i][1];                   /*  最大表大小                  */
            break;
        } else {
            if (ulNCacheNode >= _G_iNandRCacheHashSize[i][0]) {
                continue;
            } else {
                iHashSize = _G_iNandRCacheHashSize[i][1];               /*  确定                        */
                break;
            }
        } 
    }
    
    pnrcache = (PLW_NRCACHE_CB)__SHEAP_ALLOC(sizeof(LW_NRCACHE_CB));    /*  开辟控制块内存              */
    if (pnrcache == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (ERROR_SYSTEM_LOW_MEMORY);
    }
    lib_bzero(pnrcache, sizeof(LW_NRCACHE_CB));
    
    pnrcache->NRCACHE_ulPagePerBlock = ulPagePerBlock;
    pnrcache->NRCACHE_ulnCacheNode   = ulNCacheNode;
    pnrcache->NRCACHE_iHashSize      = iHashSize;
    pnrcache->NRCACHE_pringLRU       = LW_NULL;
    pnrcache->NRCACHE_plineFree      = LW_NULL;
    
    /*
     *  开辟 hash 表内存
     */
    pnrcache->NRCACHE_pplineHash = (PLW_LIST_LINE *)__SHEAP_ALLOC(sizeof(PVOID) * (size_t)iHashSize);
    if (pnrcache->NRCACHE_pplineHash == LW_NULL) {
        iErrLevel = 1;
        goto    __error_handle;
    }
    lib_bzero(pnrcache->NRCACHE_pplineHash, (size_t)(sizeof(PVOID) * iHashSize));
    
    /*
     *  开辟每一个控制块内存.
     */
    pnrcache->NRCACHE_pnrcachenBuffer = 
        (PLW_NRCACHE_NODE)__SHEAP_ALLOC(sizeof(LW_NRCACHE_NODE) * (size_t)ulNCacheNode);
    if (pnrcache->NRCACHE_pnrcachenBuffer == LW_NULL) {
        iErrLevel = 2;
        goto    __error_handle;
    }
    /*
     *  初始化每个节点内存使用情况
     */
    pnrcachenTemp = pnrcache->NRCACHE_pnrcachenBuffer;
    pcTemp        = (caddr_t)pvNandRCacheMem;
    for (i = 0; i < ulNCacheNode; i++) {
        _LIST_RING_INIT_IN_CODE(pnrcachenTemp->NRCACHEN_ringLRU);       /*  初始化 LRU                  */
        _List_Line_Add_Tail(&pnrcachenTemp->NRCACHEN_lineManage,
                            &pnrcache->NRCACHE_plineFree);              /*  加入空闲表                  */
        pnrcachenTemp->NRCACHEN_ulChunkNo = 0;
        pnrcachenTemp->NRCACHEN_pcChunk   = pcTemp;                     /*  链接页面缓存内存            */
        pnrcachenTemp->NRCACHEN_pcSpare   = (pcTemp + ulPageSize);      /*  链接扩展区缓存内存          */
        
        pcTemp += (ulPageSize + ulSpareSize);
        pnrcachenTemp++;                                                /*  下一个节点                  */
    }
    *ppnrcache = pnrcache;                                              /*  保存控制块                  */
    
    return  (ERROR_NONE);
    
__error_handle:
    if (iErrLevel > 1) {
        __SHEAP_FREE(pnrcache->NRCACHE_pplineHash);
    }
    if (iErrLevel > 0) {
        __SHEAP_FREE(pnrcache);
    }
    _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
    _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
    return  (ERROR_SYSTEM_LOW_MEMORY);
}
/*********************************************************************************************************
** 函数名称: API_NandRCacheDelete
** 功能描述: 删除一个 NAND FLASH READ CACHE.
** 输　入  : pnrcache              控制块
** 输　出  : ERROR 控制块
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_NandRCacheDelete (PLW_NRCACHE_CB    pnrcache)
{
    if (!pnrcache) {
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    
    __SHEAP_FREE(pnrcache->NRCACHE_pnrcachenBuffer);
    __SHEAP_FREE(pnrcache->NRCACHE_pplineHash);
    __SHEAP_FREE(pnrcache);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_NandRCacheNodeGet
** 功能描述: 获得指定的扇区控制块, 如果没有命中, 返回 NULL.
**           (如果命中, 同时将此节点挂入 LRU 头部)
** 输　入  : pnrcache              控制块
**           ulChunkNo             扇区号
** 输　出  : 查询到的缓冲块
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
PLW_NRCACHE_NODE  API_NandRCacheNodeGet (PLW_NRCACHE_CB  pnrcache, ULONG  ulChunkNo)
{
    REGISTER PLW_LIST_LINE      plineHash;
    REGISTER PLW_NRCACHE_NODE   pnrcachen;
    
    if (pnrcache == LW_NULL) {
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (LW_NULL);
    }
                                                                        /*  获得 hash 表入口            */
    plineHash = pnrcache->NRCACHE_pplineHash[ulChunkNo & (pnrcache->NRCACHE_iHashSize - 1)];
    
    for (; plineHash != LW_NULL; plineHash = _list_line_get_next(plineHash)) {
        pnrcachen = _LIST_ENTRY(plineHash, LW_NRCACHE_NODE, NRCACHEN_lineManage);
        if (pnrcachen->NRCACHEN_ulChunkNo == ulChunkNo) {
            
            if (pnrcache->NRCACHE_pringLRU != &pnrcachen->NRCACHEN_ringLRU) {
                _List_Ring_Del(&pnrcachen->NRCACHEN_ringLRU, 
                               &pnrcache->NRCACHE_pringLRU);
                _List_Ring_Add_Ahead(&pnrcachen->NRCACHEN_ringLRU, 
                                     &pnrcache->NRCACHE_pringLRU);      /*  需要将此节点插入 LRU 表头   */
            }
            return  (pnrcachen);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_NandRCacheNodeFree
** 功能描述: 将一个有效的节点释放 (读不命中)
** 输　入  : pnrcache              控制块
**           ulChunkNo             扇区号
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID  API_NandRCacheNodeFree (PLW_NRCACHE_CB  pnrcache, ULONG  ulChunkNo)
{
    REGISTER PLW_LIST_LINE      plineHash;
    REGISTER PLW_LIST_LINE     *pplineHash;
    REGISTER PLW_NRCACHE_NODE   pnrcachen;
    
    if (pnrcache == LW_NULL) {
        return;
    }
    
    pplineHash = &pnrcache->NRCACHE_pplineHash[ulChunkNo & (pnrcache->NRCACHE_iHashSize - 1)];
    plineHash  = *pplineHash;
    
    for (; plineHash != LW_NULL; plineHash = _list_line_get_next(plineHash)) {
        pnrcachen = _LIST_ENTRY(plineHash, LW_NRCACHE_NODE, NRCACHEN_lineManage);
        if (pnrcachen->NRCACHEN_ulChunkNo == ulChunkNo) {
            
            _List_Ring_Del(&pnrcachen->NRCACHEN_ringLRU, 
                           &pnrcache->NRCACHE_pringLRU);                /*  从 LRU 中删除               */
            _List_Line_Del(&pnrcachen->NRCACHEN_lineManage,
                           pplineHash);                                 /*  从 hash 中删除              */
            _List_Line_Add_Tail(&pnrcachen->NRCACHEN_lineManage,
                                &pnrcache->NRCACHE_plineFree);          /*  从新加入空闲表              */
        
            return;
        }
    }
}
/*********************************************************************************************************
** 函数名称: API_NandRCacheNodeAlloc
** 功能描述: 从空闲 node 池中获取一个节点, 若不存在空闲节点, 则将淘汰一个最久未使用的节点.
**           (同时将此节点挂入 LRU 头部)
** 输　入  : pnrcache              控制块
**           ulChunkNo             扇区号
** 输　出  : 分配的缓冲块
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
PLW_NRCACHE_NODE  API_NandRCacheNodeAlloc (PLW_NRCACHE_CB  pnrcache, ULONG  ulChunkNo)
{
    REGISTER PLW_LIST_LINE     *pplineHash;
    REGISTER PLW_NRCACHE_NODE   pnrcachen;
    
    if (pnrcache == LW_NULL) {
        return  (LW_NULL);
    }
    
    if (pnrcache->NRCACHE_plineFree) {                                  /*  是否存在空闲节点            */
                                                                        /*  获得 hash 表入口            */
        pplineHash = &pnrcache->NRCACHE_pplineHash[ulChunkNo & (pnrcache->NRCACHE_iHashSize - 1)];
        pnrcachen  = _LIST_ENTRY(pnrcache->NRCACHE_plineFree, 
                                 LW_NRCACHE_NODE, 
                                 NRCACHEN_lineManage);                  /*  获得一个空闲节点            */
        
        _List_Line_Del(&pnrcachen->NRCACHEN_lineManage,
                       &pnrcache->NRCACHE_plineFree);                   /*  从空闲表中删除              */
        _List_Line_Add_Ahead(&pnrcachen->NRCACHEN_lineManage,
                             pplineHash);                               /*  插入 hash 表                */
        _List_Ring_Add_Ahead(&pnrcachen->NRCACHEN_ringLRU, 
                             &pnrcache->NRCACHE_pringLRU);              /*  需要将此节点插入 LRU 表头   */
    
    } else {                                                            /*  需要淘汰一个节点            */
                                                                        /*  最久未使用的节点            */
        PLW_LIST_RING   pringLast = _list_ring_get_prev(pnrcache->NRCACHE_pringLRU);
        
        pnrcachen = _LIST_ENTRY(pringLast, 
                                LW_NRCACHE_NODE, 
                                NRCACHEN_ringLRU);
    
        pplineHash = &pnrcache->NRCACHE_pplineHash[pnrcachen->NRCACHEN_ulChunkNo & 
                                                   (pnrcache->NRCACHE_iHashSize - 1)];
        _List_Line_Del(&pnrcachen->NRCACHEN_lineManage,
                       pplineHash);                                     /*  从原先的 hash 表中删除      */
        
        pplineHash = &pnrcache->NRCACHE_pplineHash[ulChunkNo & (pnrcache->NRCACHE_iHashSize - 1)];
        _List_Line_Add_Ahead(&pnrcachen->NRCACHEN_lineManage,
                             pplineHash);                               /*  插入 hash 表                */
                             
        _List_Ring_Del(&pnrcachen->NRCACHEN_ringLRU, 
                       &pnrcache->NRCACHE_pringLRU);                    /*  从 LRU 中删除               */
        _List_Ring_Add_Ahead(&pnrcachen->NRCACHEN_ringLRU, 
                             &pnrcache->NRCACHE_pringLRU);              /*  需要将此节点插入 LRU 表头   */
    }
    
    pnrcachen->NRCACHEN_ulChunkNo = ulChunkNo;                          /*  记录扇区号                  */

    return  (pnrcachen);
}
/*********************************************************************************************************
** 函数名称: API_NandRCacheBlockFree
** 功能描述: 由于一个 block 的删除, 释放所有与相关 block 有关的扇区.
** 输　入  : pnrcache              控制块
**           ulBlockNo             块号 (非扇区号! 块内的第一个扇区号为: 块号 X 每一块中扇区的个数)
** 输　出  : 分配的缓冲块
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID  API_NandRCacheBlockFree (PLW_NRCACHE_CB  pnrcache, ULONG  ulBlockNo)
{
    REGISTER ULONG      ulStartChunk;
             INT        i;

    if (pnrcache == LW_NULL) {
        return;
    }
    
    ulStartChunk = (ulBlockNo * pnrcache->NRCACHE_ulPagePerBlock);
    for (i = 0; i < pnrcache->NRCACHE_ulPagePerBlock; i++) {
        API_NandRCacheNodeFree(pnrcache, (ULONG)(ulStartChunk + i));
    }
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_YAFFS_EN > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
