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
** 文件创建日期: 2008 年 11 月 26 日
**
** 描        述: 磁盘高速缓冲控制器内部操作库.

** BUG
2008.12.16  __diskCacheFlushInvalidate2() 函数指定的次数为回写的脏扇区次数.
2009.03.16  加入读写旁路的支持.
2009.03.18  FIOCANCEL 将 CACHE 停止, 没有回写的数据全部丢弃. 内存回到初始状态.
2009.07.22  增加对 FIOSYNC 的支持.
2009.11.03  FIOCANCEL 命令应该传往磁盘驱动, 使其放弃所有数据.
            FIODISKCHANGE 与 FIOCANCEL 操作相同.
2015.02.05  修正 __diskCacheNodeReadData() 对扇区判断错误.
2016.07.12  磁盘写操作加入一个文件系统层参数.
2016.07.13  提高 HASH 表工作效率.
2016.07.27  的使用专用的磁盘扇区拷贝函数.
2017.09.02  对 FIOTRIM、SYNC 相关命令与读写扇区的互斥放在 disk cache 里面完成.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "diskCacheLib.h"
#include "diskCachePipeline.h"
#include "diskCache.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)
/*********************************************************************************************************
  使用最优的 LRU 会增大运算量降低效率
*********************************************************************************************************/
#define __LW_DISKCACHE_OPTIMUM_LRU          0
#define __LW_DISKCACHE_FLUSH_META_MAX_HASH  32
/*********************************************************************************************************
  内部宏操作
*********************************************************************************************************/
#define __LW_DISKCACHE_DISK_IOCTL(pdiskcDiskCache)              \
        pdiskcDiskCache->DISKC_pblkdDisk->BLKD_pfuncBlkIoctl
#define __LW_DISKCACHE_DISK_RESET(pdiskcDiskCache)              \
        pdiskcDiskCache->DISKC_pblkdDisk->BLKD_pfuncBlkReset
#define __LW_DISKCACHE_DISK_STATUS(pdiskcDiskCache)             \
        pdiskcDiskCache->DISKC_pblkdDisk->BLKD_pfuncBlkStatusChk
/*********************************************************************************************************
** 函数名称: __diskCacheMemcpy
** 功能描述: 扇区拷贝
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
**           pvTo               目标
**           pvFrom             源
**           stSize             字节数 (必须是 512 字节倍数)
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_STATIC VOID  __diskCacheMemcpy (PVOID  pvTo, CPVOID  pvFrom, size_t  stSize)
{
    REGISTER INT      iMuti512;
    REGISTER INT      i;
    REGISTER UINT64  *pu64To   = (UINT64 *)pvTo;
    REGISTER UINT64  *pu64From = (UINT64 *)pvFrom;
    
    if (((addr_t)pvTo & 0x7) || ((addr_t)pvFrom & 0x7)) {
        lib_memcpy(pvTo, pvFrom, stSize);
        return;
    }
    
    iMuti512 = stSize >> 9;
    
    do {
        for (i = 0; i < 64; i += 16) {
            *pu64To++ = *pu64From++;
            *pu64To++ = *pu64From++;
            *pu64To++ = *pu64From++;
            *pu64To++ = *pu64From++;
            *pu64To++ = *pu64From++;
            *pu64To++ = *pu64From++;
            *pu64To++ = *pu64From++;
            *pu64To++ = *pu64From++;
            *pu64To++ = *pu64From++;
            *pu64To++ = *pu64From++;
            *pu64To++ = *pu64From++;
            *pu64To++ = *pu64From++;
            *pu64To++ = *pu64From++;
            *pu64To++ = *pu64From++;
            *pu64To++ = *pu64From++;
            *pu64To++ = *pu64From++;
        }
    } while (--iMuti512);
}
/*********************************************************************************************************
** 函数名称: __diskCacheMemReset
** 功能描述: 重新初始化磁盘 CACHE 缓冲区内存管理部分
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_STATIC INT  __diskCacheMemReset (PLW_DISKCACHE_CB   pdiskcDiskCache)
{
    REGISTER INT                    i;
    REGISTER PLW_DISKCACHE_NODE     pdiskn = (PLW_DISKCACHE_NODE)pdiskcDiskCache->DISKC_pcCacheNodeMem;
    REGISTER PCHAR                  pcData = (PCHAR)pdiskcDiskCache->DISKC_pcCacheMem;
    
    pdiskcDiskCache->DISKC_ulValidCounter = 0;                          /*  没有有效的扇区              */
    pdiskcDiskCache->DISKC_ulDirtyCounter = 0;                          /*  没有需要回写的扇区          */
    
    pdiskcDiskCache->DISKC_pringLruHeader = LW_NULL;                    /*  初始化 LRU 表               */
    lib_bzero(pdiskcDiskCache->DISKC_pplineHash,
              (sizeof(PVOID) * pdiskcDiskCache->DISKC_iHashSize));      /*  初始化 HASH 表              */
    
    for (i = 0; i < pdiskcDiskCache->DISKC_ulNCacheNode; i++) {
        pdiskn->DISKN_ulSectorNo = (ULONG)PX_ERROR;
        pdiskn->DISKN_iStatus    = 0;
        pdiskn->DISKN_pcData     = pcData;
        
        _List_Ring_Add_Last(&pdiskn->DISKN_ringLru, 
                            &pdiskcDiskCache->DISKC_pringLruHeader);    /*  插入 LRU 表                 */
        _LIST_LINE_INIT_IN_CODE(pdiskn->DISKN_lineHash);                /*  初始化 HASH 表              */
    
        pdiskn++;
        pcData += pdiskcDiskCache->DISKC_ulBytesPerSector;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __diskCacheMemInit
** 功能描述: 初始化磁盘 CACHE 缓冲区内存管理部分
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
**           pvDiskCacheMem     缓冲区
**           ulBytesPerSector   扇区大小
**           ulNSector          扇区数量
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __diskCacheMemInit (PLW_DISKCACHE_CB   pdiskcDiskCache,
                         VOID              *pvDiskCacheMem,
                         ULONG              ulBytesPerSector,
                         ULONG              ulNSector)
{
    REGISTER PCHAR                  pcData = (PCHAR)pvDiskCacheMem;

    pdiskcDiskCache->DISKC_ulNCacheNode = ulNSector;
    pdiskcDiskCache->DISKC_pcCacheMem   = (caddr_t)pcData;              /*  CACHE 缓冲区                */
    
    pdiskcDiskCache->DISKC_pcCacheNodeMem = (caddr_t)__SHEAP_ALLOC(sizeof(LW_DISKCACHE_NODE) * 
                                                                   (size_t)ulNSector);
    if (pdiskcDiskCache->DISKC_pcCacheNodeMem == LW_NULL) {
        return  (PX_ERROR);
    }
    
    return  (__diskCacheMemReset(pdiskcDiskCache));
}
/*********************************************************************************************************
** 函数名称: __diskCacheHashAdd
** 功能描述: 将指定 CACHE 节点加入到相关的 HASH 表中
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
**           pdiskn             需要插入的 CACHE 节点
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_STATIC VOID  __diskCacheHashAdd (PLW_DISKCACHE_CB   pdiskcDiskCache, PLW_DISKCACHE_NODE  pdiskn)
{
    REGISTER PLW_LIST_LINE      *pplineHashEntry;
    
    pplineHashEntry = &pdiskcDiskCache->DISKC_pplineHash[
                       pdiskn->DISKN_ulSectorNo &
                       (pdiskcDiskCache->DISKC_iHashSize - 1)];         /*  获得 HASH 表入口            */
                     
    _List_Line_Add_Ahead(&pdiskn->DISKN_lineHash, pplineHashEntry);     /*  插入 HASH 表                */

    pdiskcDiskCache->DISKC_ulValidCounter++;
}
/*********************************************************************************************************
** 函数名称: __diskCacheHashRemove
** 功能描述: 将指定 CACHE 节点加入到相关的 HASH 表中
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
**           pdiskn             需要插入的 CACHE 节点
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_STATIC VOID  __diskCacheHashRemove (PLW_DISKCACHE_CB   pdiskcDiskCache, PLW_DISKCACHE_NODE  pdiskn)
{
    REGISTER PLW_LIST_LINE      *pplineHashEntry;
    
    pplineHashEntry = &pdiskcDiskCache->DISKC_pplineHash[
                       pdiskn->DISKN_ulSectorNo &
                       (pdiskcDiskCache->DISKC_iHashSize - 1)];         /*  获得 HASH 表入口            */
                       
    _List_Line_Del(&pdiskn->DISKN_lineHash, pplineHashEntry);
    
    pdiskn->DISKN_ulSectorNo = (ULONG)PX_ERROR;
    pdiskn->DISKN_iStatus    = 0;                                       /*  无脏位与有效位              */

    pdiskcDiskCache->DISKC_ulValidCounter--;
}
/*********************************************************************************************************
** 函数名称: __diskCacheHashFind
** 功能描述: 从 HASH 表中寻找指定属性的扇区
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
**           ulSectorNo         扇区的编号
** 输　出  : 寻找到的节点
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_STATIC PLW_DISKCACHE_NODE  __diskCacheHashFind (PLW_DISKCACHE_CB   pdiskcDiskCache, ULONG  ulSectorNo)
{
    REGISTER PLW_LIST_LINE          plineHash;
    REGISTER PLW_DISKCACHE_NODE     pdiskn;
    
    plineHash = pdiskcDiskCache->DISKC_pplineHash[
                ulSectorNo & 
                (pdiskcDiskCache->DISKC_iHashSize - 1)];                /*  获得 HASH 表入口            */
                       
    for (; plineHash != LW_NULL; plineHash = _list_line_get_next(plineHash)) {
        pdiskn = _LIST_ENTRY(plineHash, LW_DISKCACHE_NODE, DISKN_lineHash);
        if (pdiskn->DISKN_ulSectorNo == ulSectorNo) {
            return  (pdiskn);                                           /*  寻找到                      */
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __diskCacheLruFind
** 功能描述: 从 LRU 表中寻找指定属性的扇区
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
**           ulSectorNo         扇区的编号
** 输　出  : 寻找到的节点
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if __LW_DISKCACHE_OPTIMUM_LRU > 0

LW_STATIC PLW_DISKCACHE_NODE  __diskCacheLruFind (PLW_DISKCACHE_CB   pdiskcDiskCache, ULONG  ulSectorNo)
{
             PLW_LIST_RING          pringLruHeader = pdiskcDiskCache->DISKC_pringLruHeader;
    REGISTER PLW_LIST_RING          pringTemp;
    REGISTER PLW_DISKCACHE_NODE     pdiskn;
    
    if (_LIST_RING_IS_EMPTY(pringLruHeader)) {
        return  (LW_NULL);
    }
    
    pdiskn = (PLW_DISKCACHE_NODE)pringLruHeader;
    if (pdiskn->DISKN_ulSectorNo == ulSectorNo) {
        return  (pdiskn);                                               /*  寻找到                      */
    }
    
    for (pringTemp  = _list_ring_get_next(pringLruHeader);
         pringTemp != pringLruHeader;
         pringTemp  = _list_ring_get_next(pringTemp)) {                 /*  遍历 LRU 表                 */
         
        pdiskn = (PLW_DISKCACHE_NODE)pringTemp;
        if (pdiskn->DISKN_ulSectorNo == ulSectorNo) {
            return  (pdiskn);                                           /*  寻找到                      */
        }
    }
    
    return  (LW_NULL);
}

#endif                                                                  /*  __LW_DISKCACHE_OPTIMUM_LRU  */
/*********************************************************************************************************
** 函数名称: __diskCacheSortListAdd
** 功能描述: 将指定 CACHE 节点加入到一个经过排序的链表中
** 输　入  : pringListHeader    链表表头
**           pdiskn             需要插入的 CACHE 节点
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_STATIC VOID  __diskCacheSortListAdd (PLW_LIST_RING  *ppringListHeader, PLW_DISKCACHE_NODE  pdiskn)
{
    REGISTER PLW_LIST_RING          pringTemp;
             PLW_LIST_RING          pringDummyHeader;
    REGISTER PLW_DISKCACHE_NODE     pdisknTemp;

    if (_LIST_RING_IS_EMPTY(*ppringListHeader)) {
        _List_Ring_Add_Ahead(&pdiskn->DISKN_ringLru, 
                             ppringListHeader);
        return;
    }
    
    pdisknTemp = (PLW_DISKCACHE_NODE)(*ppringListHeader);               /*  LRU 表为 CACHE 节点首个元素 */
    if (pdiskn->DISKN_ulSectorNo < pdisknTemp->DISKN_ulSectorNo) {
        _List_Ring_Add_Ahead(&pdiskn->DISKN_ringLru, 
                             ppringListHeader);                         /*  直接插入表头                */
        return;
    }
    
    for (pringTemp  = _list_ring_get_next(*ppringListHeader);
         pringTemp != *ppringListHeader;
         pringTemp  = _list_ring_get_next(pringTemp)) {                 /*  从第二个节点线性插入        */
        
        pdisknTemp = (PLW_DISKCACHE_NODE)pringTemp;
        if (pdiskn->DISKN_ulSectorNo < pdisknTemp->DISKN_ulSectorNo) {
            pringDummyHeader = pringTemp;
            _List_Ring_Add_Last(&pdiskn->DISKN_ringLru,
                                &pringDummyHeader);                     /*  插入到合适的位置            */
            return;
        }
    }
    
    _List_Ring_Add_Last(&pdiskn->DISKN_ringLru,
                        ppringListHeader);                              /*  插入到链表最后              */
}
/*********************************************************************************************************
** 函数名称: __diskCacheFsCallback
** 功能描述: 回写磁盘后文件系统回调函数
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
**           u64FsKey           文件系统 key
**           ulStartSector      起始扇区
**           ulSectorCount      总回写扇区数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_STATIC VOID  __diskCacheFsCallback (PLW_DISKCACHE_CB     pdiskcDiskCache, 
                                    UINT64               u64FsKey,
                                    ULONG                ulStartSector,
                                    ULONG                ulSectorCount)
{
    if (pdiskcDiskCache->DISKC_pfuncFsCallback) {
        pdiskcDiskCache->DISKC_pfuncFsCallback(pdiskcDiskCache->DISKC_pvFsArg,
                                               u64FsKey,
                                               ulStartSector,
                                               ulSectorCount);
    }
}
/*********************************************************************************************************
** 函数名称: __diskCacheFlushList
** 功能描述: 将指定链表内的 CACHE 节点扇区全部回写磁盘
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
**           pringFlushHeader   链表头
**           bMakeInvalidate    是否在回写后将相关节点设为无效状态
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_STATIC INT  __diskCacheFlushList (PLW_DISKCACHE_CB   pdiskcDiskCache, 
                                     PLW_LIST_RING      pringFlushHeader,
                                     BOOL               bMakeInvalidate)
{
    REGISTER INT                    i;

             PLW_DISKCACHE_NODE     pdiskn;
    REGISTER PLW_DISKCACHE_NODE     pdisknContinue;
             PLW_LIST_RING          pringTemp;
             
    REGISTER INT                    iBurstCount;
             PVOID                  pvBuffer;
             PCHAR                  pcBurstBuffer;
             INT                    iRetVal;
             BOOL                   bHasError = LW_FALSE;
    
    for (pringTemp  = pringFlushHeader;
         pringTemp != LW_NULL;
         pringTemp  = pringFlushHeader) {                               /*  直到链表为空为止            */
        
        pdiskn = (PLW_DISKCACHE_NODE)pringTemp;                         /*  DISKN_ringLru 是第一个元素  */
        pdisknContinue = pdiskn;
        
        for (iBurstCount = 1; 
             iBurstCount < pdiskcDiskCache->DISKC_iMaxWBurstSector;
             iBurstCount++) {
            
            pdisknContinue = 
                (PLW_DISKCACHE_NODE)_list_ring_get_next(&pdisknContinue->DISKN_ringLru);
            if (pdisknContinue->DISKN_ulSectorNo != 
                (pdiskn->DISKN_ulSectorNo + iBurstCount)) {             /*  判断是否为连续扇区          */
                break;
            }
        }
        
        pvBuffer = __diskCacheWpGetBuffer(&pdiskcDiskCache->DISKC_wpWrite, LW_FALSE);
                                          
        if (iBurstCount == 1) {                                         /*  不能进行猝发操作            */
            __diskCacheMemcpy(pvBuffer, pdiskn->DISKN_pcData,
                              (size_t)pdiskcDiskCache->DISKC_ulBytesPerSector);
        
        } else {                                                        /*  可以使用猝发写入            */
            pcBurstBuffer  = (PCHAR)pvBuffer;
            pdisknContinue = pdiskn;
            
            for (i = 0; i < iBurstCount; i++) {                         /*  装载猝发传输数据            */
                __diskCacheMemcpy(pcBurstBuffer, 
                       pdisknContinue->DISKN_pcData, 
                       (size_t)pdiskcDiskCache->DISKC_ulBytesPerSector);/*  拷贝数据                    */
                           
                pdisknContinue = 
                    (PLW_DISKCACHE_NODE)_list_ring_get_next(&pdisknContinue->DISKN_ringLru);
                pcBurstBuffer += pdiskcDiskCache->DISKC_ulBytesPerSector;
            }
        }
        
        iRetVal = __diskCacheWpWrite(pdiskcDiskCache, pdiskcDiskCache->DISKC_pblkdDisk,
                                     pvBuffer, pdiskn->DISKN_ulSectorNo,
                                     iBurstCount);                      /*  猝发写入扇区                */
        if (iRetVal < 0) {
            __diskCacheWpPutBuffer(&pdiskcDiskCache->DISKC_wpWrite, pvBuffer);
            bHasError       = LW_TRUE;
            bMakeInvalidate = LW_TRUE;                                  /*  回写失败, 需要无效设备      */
        
        } else {
            __diskCacheFsCallback(pdiskcDiskCache, 
                                  pdiskn->DISKN_u64FsKey,
                                  pdiskn->DISKN_ulSectorNo,
                                  iBurstCount);                         /*  执行文件系统回调            */
        }
        
        for (i = 0; i < iBurstCount; i++) {                             /*  开始处理这些回写的扇区记录  */
            __LW_DISKCACHE_CLR_DIRTY(pdiskn);
            
            if (bMakeInvalidate) {
                __diskCacheHashRemove(pdiskcDiskCache, pdiskn);         /*  从有效的 HASH 表中删除      */
            }
            
            pdisknContinue = 
                (PLW_DISKCACHE_NODE)_list_ring_get_next(&pdiskn->DISKN_ringLru);
                
            _List_Ring_Del(&pdiskn->DISKN_ringLru, 
                           &pringFlushHeader);                          /*  从临时回写链表中删除        */
            _List_Ring_Add_Last(&pdiskn->DISKN_ringLru, 
                           &pdiskcDiskCache->DISKC_pringLruHeader);     /*  加入到磁盘 CACHE LRU 表中   */
                                 
            pdiskn = pdisknContinue;                                    /*  下一个连续节点              */
        }
        
        pdiskcDiskCache->DISKC_ulDirtyCounter -= iBurstCount;           /*  重新计算需要回写的扇区数    */
    }
    
    if (bHasError) {
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __diskCacheFlushInvalidate
** 功能描述: 将指定磁盘 CACHE 的指定扇区范围回写并且按寻求设置为无效.
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
**           ulStartSector      起始扇区
**           ulEndSector        结束扇区
**           bMakeFlush         是否进行 FLUSH 操作
**           bMakeInvalidate    是否进行 INVALIDATE 操作
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_STATIC INT  __diskCacheFlushInvRange (PLW_DISKCACHE_CB   pdiskcDiskCache,
                                         ULONG              ulStartSector,
                                         ULONG              ulEndSector,
                                         BOOL               bMakeFlush,
                                         BOOL               bMakeInvalidate)
{
             INT                    i;
    REGISTER PLW_DISKCACHE_NODE     pdiskn;
    REGISTER PLW_DISKCACHE_NODE     pdisknPrev;
             PLW_LIST_RING          pringFlushHeader = LW_NULL;
                                                                        /*  从最近最久没有使用的开始    */
    pdiskn = (PLW_DISKCACHE_NODE)_list_ring_get_prev(pdiskcDiskCache->DISKC_pringLruHeader);
    
    for (i = 0; i < pdiskcDiskCache->DISKC_ulNCacheNode; i++) {
    
        pdisknPrev = (PLW_DISKCACHE_NODE)_list_ring_get_prev(&pdiskn->DISKN_ringLru);
        
        if ((pdiskn->DISKN_ulSectorNo > ulEndSector) ||
            (pdiskn->DISKN_ulSectorNo < ulStartSector)) {               /*  不在设定的范围内            */
            goto    __check_again;
        }
        
        if (__LW_DISKCACHE_IS_DIRTY(pdiskn)) {                          /*  检查是需要回写              */
            if (bMakeFlush) {
                _List_Ring_Del(&pdiskn->DISKN_ringLru, 
                               &pdiskcDiskCache->DISKC_pringLruHeader);
                __diskCacheSortListAdd(&pringFlushHeader, pdiskn);      /*  加入到排序等待回写队列      */
            }
            goto    __check_again;
        }
        
        if (__LW_DISKCACHE_IS_VALID(pdiskn)) {                          /*  检查是否有效                */
            if (bMakeInvalidate) {
                __diskCacheHashRemove(pdiskcDiskCache, pdiskn);         /*  从有效的 HASH 表中删除      */
            }
        }
        
__check_again:
        pdiskn = pdisknPrev;                                            /*  遍历 LRU 表                 */
    }
    
    if (!_LIST_RING_IS_EMPTY(pringFlushHeader)) {                       /*  是否需要回写                */
        return  (__diskCacheFlushList(pdiskcDiskCache, pringFlushHeader, bMakeInvalidate));
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __diskCacheFlushInv
** 功能描述: 将指定磁盘 CACHE 的指定关键区回写并且按寻求设置为无效.
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
**           ulStartSector      起始扇区
**           ulEndSector        结束扇区
**           bMakeFlush         是否进行 FLUSH 操作
**           bMakeInvalidate    是否进行 INVALIDATE 操作
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_STATIC INT  __diskCacheFlushInvMeta (PLW_DISKCACHE_CB   pdiskcDiskCache,
                                        ULONG              ulStartSector,
                                        ULONG              ulEndSector,
                                        BOOL               bMakeFlush,
                                        BOOL               bMakeInvalidate)
{
             ULONG                  i;
             ULONG                  ulCnt = ulEndSector - ulStartSector + 1;
    REGISTER PLW_DISKCACHE_NODE     pdiskn;
             PLW_LIST_RING          pringFlushHeader = LW_NULL;
    
    if (ulCnt > __LW_DISKCACHE_FLUSH_META_MAX_HASH) {                   /*  大于优化大小                */
        return  (__diskCacheFlushInvRange(pdiskcDiskCache, 
                                          ulStartSector, 
                                          ulEndSector, 
                                          bMakeFlush, 
                                          bMakeInvalidate));
    }
    
    for (i = 0; i < ulCnt; i++) {                                       /*  循环处理                    */
        pdiskn = __diskCacheHashFind(pdiskcDiskCache, ulEndSector);     /*  首先从有效的 HASH 表中查找  */
        if (pdiskn == LW_NULL) {
            ulEndSector--;
            continue;
        }
        
        if (__LW_DISKCACHE_IS_DIRTY(pdiskn)) {                          /*  检查是需要回写              */
            if (bMakeFlush) {
                _List_Ring_Del(&pdiskn->DISKN_ringLru, 
                               &pdiskcDiskCache->DISKC_pringLruHeader);
                __diskCacheSortListAdd(&pringFlushHeader, pdiskn);      /*  加入到排序等待回写队列      */
            }
            ulEndSector--;
            continue;
        }
        
        if (__LW_DISKCACHE_IS_VALID(pdiskn)) {                          /*  检查是否有效                */
            if (bMakeInvalidate) {
                __diskCacheHashRemove(pdiskcDiskCache, pdiskn);         /*  从有效的 HASH 表中删除      */
            }
        }
        
        ulEndSector--;                                                  /*  反向循环, 链表排序更快      */
    }
    
    if (!_LIST_RING_IS_EMPTY(pringFlushHeader)) {                       /*  是否需要回写                */
        return  (__diskCacheFlushList(pdiskcDiskCache, pringFlushHeader, bMakeInvalidate));
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __diskCacheFlushInvCnt
** 功能描述: 将指定磁盘 CACHE 的指定扇区数量回写并且按寻求设置为无效. (方法2)
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
**           iLruSectorNum      最久未使用的扇区数量
**           bMakeFlush         是否进行 FLUSH 操作
**           bMakeInvalidate    是否进行 INVALIDATE 操作
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_STATIC INT  __diskCacheFlushInvCnt (PLW_DISKCACHE_CB   pdiskcDiskCache,
                                       INT                iLruSectorNum,
                                       BOOL               bMakeFlush,
                                       BOOL               bMakeInvalidate)
{
             INT                    i = 0;
             
    REGISTER PLW_DISKCACHE_NODE     pdiskn;
    REGISTER PLW_DISKCACHE_NODE     pdisknPrev;
             PLW_LIST_RING          pringFlushHeader = LW_NULL;
             
    if (iLruSectorNum < 1) {
        return  (ERROR_NONE);                                           /*  不需要处理                  */
    }
    
    if (iLruSectorNum > (INT)pdiskcDiskCache->DISKC_ulDirtyCounter) {
        iLruSectorNum = (INT)pdiskcDiskCache->DISKC_ulDirtyCounter;     /*  最多处理 dirty count 个扇区 */
    }
                                                                        /*  从最近最久没有使用的开始    */
    pdiskn = (PLW_DISKCACHE_NODE)_list_ring_get_prev(pdiskcDiskCache->DISKC_pringLruHeader);
    
    for (i = 0; 
         (i < pdiskcDiskCache->DISKC_ulNCacheNode) && (iLruSectorNum > 0);
         i++) {
    
        pdisknPrev = (PLW_DISKCACHE_NODE)_list_ring_get_prev(&pdiskn->DISKN_ringLru);
        
        if (__LW_DISKCACHE_IS_DIRTY(pdiskn)) {                          /*  检查是需要回写              */
            if (bMakeFlush) {
                _List_Ring_Del(&pdiskn->DISKN_ringLru, 
                               &pdiskcDiskCache->DISKC_pringLruHeader);
                __diskCacheSortListAdd(&pringFlushHeader, pdiskn);      /*  加入到排序等待回写队列      */
            }
            iLruSectorNum--;
            goto    __check_again;
        }
        
        if (__LW_DISKCACHE_IS_VALID(pdiskn)) {                          /*  检查是否有效                */
            if (bMakeInvalidate) {
                __diskCacheHashRemove(pdiskcDiskCache, pdiskn);         /*  从有效的 HASH 表中删除      */
                iLruSectorNum--;
            }
        }
    
__check_again:
        pdiskn = pdisknPrev;                                            /*  遍历 LRU 表                 */
    }
    
    if (!_LIST_RING_IS_EMPTY(pringFlushHeader)) {                       /*  是否需要回写                */
        return  (__diskCacheFlushList(pdiskcDiskCache, pringFlushHeader, bMakeInvalidate));
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __diskCacheNodeFind
** 功能描述: 寻找一个指定的 CACHE 块
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
**           ulSectorNo         扇区号
** 输　出  : 寻找到的节点
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_STATIC PLW_DISKCACHE_NODE  __diskCacheNodeFind (PLW_DISKCACHE_CB   pdiskcDiskCache, ULONG  ulSectorNo)
{
    REGISTER PLW_DISKCACHE_NODE         pdiskn;
    
    pdiskn = __diskCacheHashFind(pdiskcDiskCache, ulSectorNo);          /*  首先从有效的 HASH 表中查找  */
    
#if __LW_DISKCACHE_OPTIMUM_LRU > 0
    if (pdiskn) {
        return  (pdiskn);                                               /*  HASH 表命中                 */
    }
    
    pdiskn = __diskCacheLruFind(pdiskcDiskCache, ulSectorNo);           /*  开始从 LRU 表中搜索         */
#endif                                                                  /*  __LW_DISKCACHE_OPTIMUM_LRU  */
    
    return  (pdiskn);
}
/*********************************************************************************************************
** 函数名称: __diskCacheNodeAlloc
** 功能描述: 创建一个 CACHE 块
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
**           ulSectorNo         扇区号
**           iExpectation       期望预开辟的多余的扇区数量 (未用, 使用 DISKC_iMaxBurstSector)
** 输　出  : 创建到的节点
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_STATIC PLW_DISKCACHE_NODE  __diskCacheNodeAlloc (PLW_DISKCACHE_CB   pdiskcDiskCache, 
                                                    ULONG              ulSectorNo,
                                                    INT                iExpectation)
{
    REGISTER PLW_LIST_RING          pringTemp;
    REGISTER PLW_DISKCACHE_NODE     pdiskn;
    
    if (pdiskcDiskCache->DISKC_ulNCacheNode == 
        pdiskcDiskCache->DISKC_ulDirtyCounter) {                        /*  所有 CACHE 全部是为脏       */
        pringTemp = pdiskcDiskCache->DISKC_pringLruHeader;
        goto    __try_flush;
    }
    
__check_again:                                                          /*  从最近最久没有使用的开始    */
    for (pringTemp  = _list_ring_get_prev(pdiskcDiskCache->DISKC_pringLruHeader);
         pringTemp != pdiskcDiskCache->DISKC_pringLruHeader;
         pringTemp  = _list_ring_get_prev(pringTemp)) {
        
        pdiskn = (PLW_DISKCACHE_NODE)pringTemp;
        if (__LW_DISKCACHE_IS_DIRTY(pdiskn) == 0) {                     /*  不需要回写的数据块          */
            break;
        }
    }
    
__try_flush:
    if (pringTemp == pdiskcDiskCache->DISKC_pringLruHeader) {           /*  没有合适的控制块            */
        pdiskn = (PLW_DISKCACHE_NODE)pringTemp;
        if (__LW_DISKCACHE_IS_DIRTY(pdiskn)) {                          /*  表头也不可使用              */
            __diskCacheFlushInvCnt(pdiskcDiskCache, 
                                   pdiskcDiskCache->DISKC_iMaxWBurstSector,
                                   LW_TRUE, LW_FALSE);                  /*  回写一些扇区的数据          */
            goto    __check_again;
        }
    }

    if (__LW_DISKCACHE_IS_VALID(pdiskn)) {                              /*  是否为有效扇区              */
        __diskCacheHashRemove(pdiskcDiskCache, pdiskn);                 /*  从有效表中删除              */
    }

    pdiskn->DISKN_ulSectorNo = ulSectorNo;
    __LW_DISKCACHE_SET_VALID(pdiskn);
    
    __diskCacheHashAdd(pdiskcDiskCache, pdiskn);
    
    return  (pdiskn);
}
/*********************************************************************************************************
** 函数名称: __diskCacheNodeRead
** 功能描述: 从磁盘中读取数据填写到 CACHE 块中.
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
**           pdiskn             CACHE 块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_STATIC INT  __diskCacheNodeRead (PLW_DISKCACHE_CB  pdiskcDiskCache, PLW_DISKCACHE_NODE  pdiskn)
{
    REGISTER INT        i;
    REGISTER INT        iNSector;
             INT        iRetVal;
             ULONG      ulStartSector = pdiskn->DISKN_ulSectorNo;
             PCHAR      pcData;
             PVOID      pvBuffer;
             
    iNSector = (INT)__MIN((ULONG)pdiskcDiskCache->DISKC_iMaxRBurstSector, 
                          (ULONG)((pdiskcDiskCache->DISKC_ulNCacheNode - 
                          pdiskcDiskCache->DISKC_ulDirtyCounter)));     /*  获得读扇区的个数            */
                      
    iNSector = (INT)__MIN((ULONG)iNSector,                              /*  进行无符号数比较            */
                          (ULONG)(pdiskcDiskCache->DISKC_ulEndStector - 
                          pdiskn->DISKN_ulSectorNo));                   /*  不能超越最后一个扇区        */
                      
    if (iNSector <= 0) {
        return  (PX_ERROR);
    }
    
    pvBuffer = __diskCacheWpGetBuffer(&pdiskcDiskCache->DISKC_wpWrite, LW_TRUE);

    iRetVal = __diskCacheWpRead(pdiskcDiskCache, pdiskcDiskCache->DISKC_pblkdDisk,
                                pvBuffer, pdiskn->DISKN_ulSectorNo, iNSector);
                                                                        /*  连续读取多个扇区的数据      */
    if (iRetVal < 0) {
        __diskCacheWpPutBuffer(&pdiskcDiskCache->DISKC_wpWrite, pvBuffer);
        return  (iRetVal);
    }
    
    i      = 0;
    pcData = (PCHAR)pvBuffer;
    do {
        __diskCacheMemcpy(pdiskn->DISKN_pcData, pcData, 
                       (UINT)pdiskcDiskCache->DISKC_ulBytesPerSector);  /*  拷贝数据                    */
        
        _List_Ring_Del(&pdiskn->DISKN_ringLru, 
                       &pdiskcDiskCache->DISKC_pringLruHeader);         /*  重新确定 LRU 表位置         */
        _List_Ring_Add_Ahead(&pdiskn->DISKN_ringLru, 
                       &pdiskcDiskCache->DISKC_pringLruHeader);
    
        i++;
        pcData += pdiskcDiskCache->DISKC_ulBytesPerSector;
        
        if (i < iNSector) {                                             /*  还需要复制数据              */
            pdiskn = __diskCacheNodeFind(pdiskcDiskCache, (ulStartSector + i));
            if (pdiskn) {                                               /*  下一个扇区数据有效          */
                break;
            }
            
            pdiskn = __diskCacheNodeAlloc(pdiskcDiskCache, 
                                          (ulStartSector + i), 
                                          (iNSector - i));              /*  重新开辟一个空的节点        */
            if (pdiskn == LW_NULL) {
                break;
            }
        }
    } while (i < iNSector);
    
    __diskCacheWpPutBuffer(&pdiskcDiskCache->DISKC_wpWrite, pvBuffer);  /*  read 需要释放缓存           */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __diskCacheNodeGet
** 功能描述: 获取一个经过指定处理的 CACHE 节点
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
**           ulSectorNo         扇区号
**           iExpectation       期望预开辟的多余的扇区数量
**           bIsRead            是否为读操作
** 输　出  : 节点处理过后的节点
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_STATIC PLW_DISKCACHE_NODE  __diskCacheNodeGet (PLW_DISKCACHE_CB   pdiskcDiskCache, 
                                                  ULONG              ulSectorNo,
                                                  INT                iExpectation,
                                                  BOOL               bIsRead)
{
    REGISTER PLW_DISKCACHE_NODE     pdiskn;
             BOOL                   bIsNewNode = LW_FALSE;
             INT                    iError;
    
    pdiskn = __diskCacheNodeFind(pdiskcDiskCache, ulSectorNo);
    if (pdiskn) {                                                       /*  直接命中                    */
        goto    __data_op;
    }
    
    pdiskn = __diskCacheNodeAlloc(pdiskcDiskCache, ulSectorNo, iExpectation);
    if (pdiskn == LW_NULL) {                                            /*  开辟新节点失败              */
        return  (LW_NULL);
    }
    
    bIsNewNode = LW_TRUE;
    
__data_op:
    if (bIsRead) {                                                      /*  读取数据                    */
        if (bIsNewNode) {                                               /*  需要从磁盘读取数据          */
            iError = __diskCacheNodeRead(pdiskcDiskCache, pdiskn);
            if (iError < 0) {                                           /*  读取错误                    */
                __diskCacheHashRemove(pdiskcDiskCache, pdiskn);
                return  (LW_NULL);
            }
        }
    
    } else {                                                            /*  写数据                      */
        if (__LW_DISKCACHE_IS_DIRTY(pdiskn) == 0) {
            __LW_DISKCACHE_SET_DIRTY(pdiskn);                           /*  设置脏位标志                */
            pdiskcDiskCache->DISKC_ulDirtyCounter++;
        }
    }
    
    pdiskcDiskCache->DISKC_disknLuck = pdiskn;                          /*  设置幸运节点                */
    
    _List_Ring_Del(&pdiskn->DISKN_ringLru, 
                   &pdiskcDiskCache->DISKC_pringLruHeader);             /*  重新确定 LRU 表位置         */
    _List_Ring_Add_Ahead(&pdiskn->DISKN_ringLru, 
                   &pdiskcDiskCache->DISKC_pringLruHeader);
                   
    return  (pdiskn);
}
/*********************************************************************************************************
** 函数名称: __diskCacheRead
** 功能描述: 通过磁盘 CACHE 读取磁盘的指定扇区
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
**           pvBuffer           缓冲区
**           ulStartSector      起始扇区
**           ulSectorCount      连续扇区数量
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __diskCacheRead (PLW_DISKCACHE_CB   pdiskcDiskCache,
                      VOID              *pvBuffer, 
                      ULONG              ulStartSector, 
                      ULONG              ulSectorCount)
{
             INT                    i;
             INT                    iError = ERROR_NONE;
    REGISTER PLW_DISKCACHE_NODE     pdiskn;
             PCHAR                  pcData = (PCHAR)pvBuffer;

    __LW_DISKCACHE_LOCK(pdiskcDiskCache);                               /*  互斥访问                    */
    for (i = 0; i < ulSectorCount; i++) {
        pdiskn = __diskCacheNodeGet(pdiskcDiskCache, 
                                    (ulStartSector + i), 
                                    (INT)(ulSectorCount - i),
                                    LW_TRUE);
        if (pdiskn == LW_NULL) {
            iError =  PX_ERROR;
            break;
        }
        
        __diskCacheMemcpy(pcData, pdiskn->DISKN_pcData,
                   (size_t)pdiskcDiskCache->DISKC_ulBytesPerSector);    /*  拷贝数据                    */
                   
        pcData += pdiskcDiskCache->DISKC_ulBytesPerSector;
    }
    __LW_DISKCACHE_UNLOCK(pdiskcDiskCache);                             /*  解锁                        */

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __diskCacheWrite
** 功能描述: 通过磁盘 CACHE 写入磁盘的指定扇区
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
**           pvBuffer           缓冲区
**           ulStartSector      起始扇区
**           ulSectorCount      连续扇区数量
**           u64FsKey           文件系统特定数据
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __diskCacheWrite (PLW_DISKCACHE_CB   pdiskcDiskCache,
                       VOID              *pvBuffer, 
                       ULONG              ulStartSector, 
                       ULONG              ulSectorCount,
                       UINT64             u64FsKey)
{
             INT                    i;
             INT                    iError = ERROR_NONE;
    REGISTER PLW_DISKCACHE_NODE     pdiskn;
             PCHAR                  pcData = (PCHAR)pvBuffer;
             
    __LW_DISKCACHE_LOCK(pdiskcDiskCache);                               /*  互斥访问                    */
    for (i = 0; i < ulSectorCount; i++) {
        pdiskn = __diskCacheNodeGet(pdiskcDiskCache, 
                                    (ulStartSector + i), 
                                    (INT)(ulSectorCount - i),
                                    LW_FALSE);
        if (pdiskn == LW_NULL) {
            iError =  PX_ERROR;
            break;
        }
        
        if (__LW_DISKCACHE_IS_DIRTY(pdiskn) == 0) {
            __LW_DISKCACHE_SET_DIRTY(pdiskn);                           /*  设置脏位标志                */
            pdiskcDiskCache->DISKC_ulDirtyCounter++;
        }
        
        __diskCacheMemcpy(pdiskn->DISKN_pcData, pcData, 
                   (size_t)pdiskcDiskCache->DISKC_ulBytesPerSector);    /*  写入数据                    */
                   
        pdiskn->DISKN_u64FsKey = u64FsKey;                              /*  保存文件系统 key            */
                   
        pcData += pdiskcDiskCache->DISKC_ulBytesPerSector;
    }
    __LW_DISKCACHE_UNLOCK(pdiskcDiskCache);                             /*  解锁                        */

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __diskCacheIoctl
** 功能描述: 通过磁盘 CACHE 控制一个磁盘
** 输　入  : pdiskcDiskCache   磁盘 CACHE 控制块
**           iCmd              控制命令
**           lArg              控制参数
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __diskCacheIoctl (PLW_DISKCACHE_CB   pdiskcDiskCache, INT  iCmd, LONG  lArg)
{
    REGISTER INT            iError;
    REGISTER BOOL           bMutex;
             PLW_BLK_RANGE  pblkrange;

    if (__LW_DISKCACHE_LOCK(pdiskcDiskCache)) {                         /*  互斥访问                    */
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    switch (iCmd) {
    
    case LW_BLKD_DISKCACHE_GET_OPT:
        *(INT *)lArg = pdiskcDiskCache->DISKC_iCacheOpt;
        __LW_DISKCACHE_UNLOCK(pdiskcDiskCache);                         /*  解锁                        */
        return  (ERROR_NONE);
        
    case LW_BLKD_DISKCACHE_SET_OPT:
        pdiskcDiskCache->DISKC_iCacheOpt = (INT)lArg;
        __LW_DISKCACHE_UNLOCK(pdiskcDiskCache);                         /*  解锁                        */
        return  (ERROR_NONE);
        
    case LW_BLKD_DISKCACHE_INVALID:                                     /*  全部不命中                  */
        __diskCacheFlushInvRange(pdiskcDiskCache,
                                 0, (ULONG)PX_ERROR, LW_TRUE, LW_TRUE);
        __LW_DISKCACHE_UNLOCK(pdiskcDiskCache);                         /*  解锁                        */
        return  (ERROR_NONE);
        
    case LW_BLKD_DISKCACHE_RAMFLUSH:                                    /*  随机回写                    */
        __diskCacheFlushInvCnt(pdiskcDiskCache, 
                               (INT)lArg, LW_TRUE, LW_FALSE);
        __LW_DISKCACHE_UNLOCK(pdiskcDiskCache);                         /*  解锁                        */
        return  (ERROR_NONE);
        
    case LW_BLKD_DISKCACHE_CALLBACKFUNC:                                /*  设置文件系统回调            */
        pdiskcDiskCache->DISKC_pfuncFsCallback = (VOIDFUNCPTR)lArg;
        __LW_DISKCACHE_UNLOCK(pdiskcDiskCache);                         /*  解锁                        */
        return  (ERROR_NONE);
        
    case LW_BLKD_DISKCACHE_CALLBACKARG:                                 /*  设置文件系统回调参数        */
        pdiskcDiskCache->DISKC_pvFsArg = (PVOID)lArg;
        __LW_DISKCACHE_UNLOCK(pdiskcDiskCache);                         /*  解锁                        */
        return  (ERROR_NONE);
        
    case FIOSYNC:                                                       /*  同步磁盘                    */
    case FIODATASYNC:
    case FIOFLUSH:                                                      /*  全部回写                    */
        iError = ERROR_NONE;
        bMutex = LW_TRUE;
        __diskCacheFlushInvRange(pdiskcDiskCache,
                                 0, (ULONG)PX_ERROR, LW_TRUE, LW_FALSE);
        __diskCacheWpSync(&pdiskcDiskCache->DISKC_wpWrite);             /*  等待写结束                  */
        break;
    
    case FIOTRIM:
    case FIOSYNCMETA:                                                   /*  TRIM, SYNCMETA 需要回写区域 */
        iError    = ERROR_NONE;
        bMutex    = LW_TRUE;
        pblkrange = (PLW_BLK_RANGE)lArg;
        __diskCacheFlushInvMeta(pdiskcDiskCache,
                                pblkrange->BLKR_ulStartSector, 
                                pblkrange->BLKR_ulEndSector,
                                LW_TRUE, LW_FALSE);                     /*  回写指定范围的数据          */
        __diskCacheWpSync(&pdiskcDiskCache->DISKC_wpWrite);             /*  等待写结束                  */
        break;
        
    case FIODISKCHANGE:                                                 /*  磁盘发生改变                */
        pdiskcDiskCache->DISKC_blkdCache.BLKD_bDiskChange = LW_TRUE;
    case FIOUNMOUNT:                                                    /*  卸载卷                      */
        iError = ERROR_NONE;
        bMutex = LW_FALSE;
        __diskCacheWpSync(&pdiskcDiskCache->DISKC_wpWrite);             /*  等待写结束                  */
        __diskCacheMemReset(pdiskcDiskCache);                           /*  重新初始化 CACHE 内存       */
        break;

    default:
        iError = PX_ERROR;
        bMutex = LW_FALSE;
        break;
    }
    
    if (bMutex) {                                                       /*  互斥执行命令不能并行        */
        __LW_DISKCACHE_DISK_IOCTL(pdiskcDiskCache)(pdiskcDiskCache->DISKC_pblkdDisk, iCmd, lArg);
        __LW_DISKCACHE_UNLOCK(pdiskcDiskCache);                         /*  解锁                        */
    
    } else {
        __LW_DISKCACHE_UNLOCK(pdiskcDiskCache);                         /*  解锁                        */
        if (iError == ERROR_NONE) {
            __LW_DISKCACHE_DISK_IOCTL(pdiskcDiskCache)(pdiskcDiskCache->DISKC_pblkdDisk, iCmd, lArg);
        
        } else {
            iError = __LW_DISKCACHE_DISK_IOCTL(pdiskcDiskCache)(pdiskcDiskCache->DISKC_pblkdDisk, iCmd, lArg);
        }
        
        if (iCmd == FIODISKINIT) {                                      /*  需要确定磁盘的最后一个扇区  */
            ULONG           ulNDiskSector = (ULONG)PX_ERROR;
            PLW_BLK_DEV     pblkdDisk     = pdiskcDiskCache->DISKC_pblkdDisk;

            if (pblkdDisk->BLKD_ulNSector) {
                ulNDiskSector = pblkdDisk->BLKD_ulNSector;

            } else {
                pblkdDisk->BLKD_pfuncBlkIoctl(pblkdDisk,
                                              LW_BLKD_GET_SECNUM,
                                              (LONG)&ulNDiskSector);
            }
            pdiskcDiskCache->DISKC_ulEndStector = ulNDiskSector - 1;    /*  获得最后一个扇区的编号      */
        }
    }

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __diskCacheReset
** 功能描述: 通过磁盘 CACHE 复位一个磁盘(初始化)
** 输　入  : pdiskcDiskCache   磁盘 CACHE 控制块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __diskCacheReset (PLW_DISKCACHE_CB   pdiskcDiskCache)
{
    __diskCacheIoctl(pdiskcDiskCache, FIOSYNC, 0);                      /*  CACHE 回写磁盘              */

    return  (__LW_DISKCACHE_DISK_RESET(pdiskcDiskCache)(pdiskcDiskCache->DISKC_pblkdDisk));
}
/*********************************************************************************************************
** 函数名称: __diskCacheStatusChk
** 功能描述: 通过磁盘 CACHE 检查一个磁盘
** 输　入  : pdiskcDiskCache   磁盘 CACHE 控制块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __diskCacheStatusChk (PLW_DISKCACHE_CB   pdiskcDiskCache)
{
    return  (__LW_DISKCACHE_DISK_STATUS(pdiskcDiskCache)(pdiskcDiskCache->DISKC_pblkdDisk));
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
