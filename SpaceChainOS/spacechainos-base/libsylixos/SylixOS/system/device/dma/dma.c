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
** 文   件   名: dma.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 01 月 06 日
**
** 描        述: 通用 DMA 设备管理组件. 主要由设备驱动程序使用, 不建议用户应用程序直接使用.

** BUG
2008.01.24  加入新型的内存管理函数.
2009.04.08  加入对 SMP 多核的支持.
2009.09.15  加入了任务队列同步功能.
2009.12.11  升级 DMA 硬件抽象结构, 支持系统同时存在多种异构 DMA 控制器.
2011.11.17  修正 __DMA_CHANNEL_STATUS() 参数错误.
2013.08.26  API_DmaJobAdd() 在中断中不等待.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_MAX_DMA_CHANNELS > 0) && (LW_CFG_DMA_EN > 0)
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static __DMA_CHANNEL        _G_dmacChannel[LW_CFG_MAX_DMA_CHANNELS];    /*  每一个 DMA 通道的控制块     */
static spinlock_t           _G_slDmaManage;                             /*  DMA 操作 锁                 */
/*********************************************************************************************************
  通道是否有效判断宏
*********************************************************************************************************/
#define __DMA_CHANNEL_INVALID(uiChannel)    (uiChannel >= LW_CFG_MAX_DMA_CHANNELS)
/*********************************************************************************************************
  获得通道控制块
*********************************************************************************************************/
#define __DMA_CHANNEL_GET(uiChannel)        &_G_dmacChannel[uiChannel]
/*********************************************************************************************************
  DMA 硬件操作
*********************************************************************************************************/
#define __DMA_CHANNEL_RESET(uiChannel)  do {                                                \
            if (_G_dmacChannel[uiChannel].DMAC_pdmafuncs &&                                 \
                _G_dmacChannel[uiChannel].DMAC_pdmafuncs->DMAF_pfuncReset) {                \
                _G_dmacChannel[uiChannel].DMAC_pdmafuncs->DMAF_pfuncReset(uiChannel,        \
                                            _G_dmacChannel[uiChannel].DMAC_pdmafuncs);      \
            }                                                                               \
        } while (0)

#define __DMA_CHANNEL_TRANS(uiChannel, pdmatMsg, iRet)  do {                                \
            if (_G_dmacChannel[uiChannel].DMAC_pdmafuncs &&                                 \
                _G_dmacChannel[uiChannel].DMAC_pdmafuncs->DMAF_pfuncTrans) {                \
                iRet =                                                                      \
                _G_dmacChannel[uiChannel].DMAC_pdmafuncs->DMAF_pfuncTrans(uiChannel,        \
                                            _G_dmacChannel[uiChannel].DMAC_pdmafuncs,       \
                                            pdmatMsg);                                      \
            }                                                                               \
        } while (0)
        
#define __DMA_CHANNEL_STATUS(uiChannel, iRet)  do {                                         \
            if (_G_dmacChannel[uiChannel].DMAC_pdmafuncs &&                                 \
                _G_dmacChannel[uiChannel].DMAC_pdmafuncs->DMAF_pfuncStatus) {               \
                iRet =                                                                      \
                _G_dmacChannel[uiChannel].DMAC_pdmafuncs->DMAF_pfuncStatus(uiChannel,       \
                                            _G_dmacChannel[uiChannel].DMAC_pdmafuncs);      \
            }                                                                               \
        } while (0)
/*********************************************************************************************************
  DMA 内部函数声明
*********************************************************************************************************/
VOID                _dmaInit(VOID);
__PDMA_WAITNODE     _dmaWaitnodeAlloc(VOID);
VOID                _dmaWaitnodeFree(__PDMA_WAITNODE         pdmanNode);
VOID                _dmaInsertToWaitList(__PDMA_CHANNEL      pdmacChannel, __PDMA_WAITNODE   pdmanNode);
VOID                _dmaDeleteFromWaitList(__PDMA_CHANNEL    pdmacChannel, __PDMA_WAITNODE   pdmanNode);
__PDMA_WAITNODE     _dmaGetFirstInWaitList(__PDMA_CHANNEL    pdmacChannel);
/*********************************************************************************************************
** 函数名称: API_DmaDrvInstall
** 功能描述: 安装通用 DMA 驱动程序
** 输　入  : uiChannel              通道
**           pdmafuncs              驱动函数集
**           stMaxDataBytes         每一次传输的最多字节数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT    API_DmaDrvInstall (UINT              uiChannel,
                          PLW_DMA_FUNCS     pdmafuncs,
                          size_t            stMaxDataBytes)
{
#define __DMA_CHANNEL_MAX_NODE              8                           /*  默认单通道最大任务节点数    */

    static   BOOL   bIsInit = LW_FALSE;
    
    if (bIsInit == LW_FALSE) {
        bIsInit =  LW_TRUE;
        LW_SPIN_INIT(&_G_slDmaManage);                                  /*  初始化自旋锁                */
        _dmaInit();                                                     /*  初始化相关结构              */
    }
    
    if (__DMA_CHANNEL_INVALID(uiChannel)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "dma channel invalid.\r\n");
        _ErrorHandle(ERROR_DMA_CHANNEL_INVALID);
        return  (PX_ERROR);
    }
    
    if ((pdmafuncs == LW_NULL) || (stMaxDataBytes == 0)) {              /*  检查参数                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    _G_dmacChannel[uiChannel].DMAC_pdmafuncs      = pdmafuncs;
    _G_dmacChannel[uiChannel].DMAC_stMaxDataBytes = stMaxDataBytes;
                                                                        /*  没有安装过驱动              */
    if (_G_dmacChannel[uiChannel].DMAC_ulJobSync == LW_OBJECT_HANDLE_INVALID) {
        _G_dmacChannel[uiChannel].DMAC_pringHead    = LW_NULL;
        _G_dmacChannel[uiChannel].DMAC_iNodeCounter = 0;
        _G_dmacChannel[uiChannel].DMAC_iMaxNode     = __DMA_CHANNEL_MAX_NODE;   
                                                                        /*  默认最多 8 个节点           */
        _G_dmacChannel[uiChannel].DMAC_ulJobSync    = API_SemaphoreBCreate("dma_jobsync", 
                                                                   LW_FALSE, 
                                                                   LW_OPTION_WAIT_FIFO | 
                                                                   LW_OPTION_OBJECT_GLOBAL, 
                                                                   LW_NULL);
                                                                        /*  创建同步工作队列            */
        if (!_G_dmacChannel[uiChannel].DMAC_ulJobSync) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create dma_jobsync.\r\n");
            return  (PX_ERROR);
        }
    }
    __DMA_CHANNEL_RESET(uiChannel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DmaReset
** 功能描述: 复位指定通道的 DMA 控制器 
** 输　入  : uiChannel      DMA 通道号
** 输　出  : PX_ERROR or ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT     API_DmaReset (UINT  uiChannel)
{
    if (__DMA_CHANNEL_INVALID(uiChannel)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "dma channel invalid.\r\n");
        _ErrorHandle(ERROR_DMA_CHANNEL_INVALID);
        return  (PX_ERROR);
    }
    
    __DMA_CHANNEL_RESET(uiChannel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DmaJobNodeNum
** 功能描述: 获得指定 DMA 通道当前等待队列的节点数
** 输　入  : uiChannel      DMA 通道号
**           piNodeNum      当前节点数量缓存
** 输　出  : PX_ERROR or ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT     API_DmaJobNodeNum (UINT   uiChannel, INT  *piNodeNum)
{
             INTREG             iregInterLevel;
    REGISTER __PDMA_CHANNEL     pdmacChannel;

    if (__DMA_CHANNEL_INVALID(uiChannel)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "dma channel invalid.\r\n");
        _ErrorHandle(ERROR_DMA_CHANNEL_INVALID);
        return  (PX_ERROR);
    }
    
    pdmacChannel = __DMA_CHANNEL_GET(uiChannel);
    
    if (piNodeNum) {
        LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);           /*  关闭中断同时锁住 spinlock   */
        *piNodeNum = pdmacChannel->DMAC_iNodeCounter;
        LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);          /*  打开中断同时解锁 spinlock   */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DmaMaxNodeNumGet
** 功能描述: 获得指定 DMA 通道当前等待队列的节点数
** 输　入  : uiChannel      DMA 通道号
**           piMaxNodeNum   允许最大节点缓冲
** 输　出  : PX_ERROR or ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT     API_DmaMaxNodeNumGet (UINT   uiChannel, INT  *piMaxNodeNum)
{
             INTREG             iregInterLevel;
    REGISTER __PDMA_CHANNEL     pdmacChannel;

    if (__DMA_CHANNEL_INVALID(uiChannel)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "dma channel invalid.\r\n");
        _ErrorHandle(ERROR_DMA_CHANNEL_INVALID);
        return  (PX_ERROR);
    }
    
    pdmacChannel = __DMA_CHANNEL_GET(uiChannel);
    
    if (piMaxNodeNum) {
        LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);           /*  关闭中断同时锁住 spinlock   */
        *piMaxNodeNum = pdmacChannel->DMAC_iMaxNode;
        LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);          /*  打开中断同时解锁 spinlock   */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DmaMaxNodeNumSet
** 功能描述: 设置指定 DMA 通道当前等待队列的节点数
** 输　入  : uiChannel      DMA 通道号
**           iMaxNodeNum    允许最大节点缓冲
** 输　出  : PX_ERROR or ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT     API_DmaMaxNodeNumSet (UINT   uiChannel, INT  iMaxNodeNum)
{
             INTREG             iregInterLevel;
    REGISTER __PDMA_CHANNEL     pdmacChannel;

    if (__DMA_CHANNEL_INVALID(uiChannel)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "dma channel invalid.\r\n");
        _ErrorHandle(ERROR_DMA_CHANNEL_INVALID);
        return  (PX_ERROR);
    }
    
    pdmacChannel = __DMA_CHANNEL_GET(uiChannel);
    
    /*
     *  注意, 这里并没有判断 iMaxNodeNum 有效性, 可能设置为 0 或者负数停止工作队列执行.
     */
    LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);               /*  关闭中断同时锁住 spinlock   */
    pdmacChannel->DMAC_iMaxNode = iMaxNodeNum;
    LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);              /*  打开中断同时解锁 spinlock   */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DmaJobAdd
** 功能描述: 添加一个 DMA 传输请求
** 输　入  : uiChannel      DMA 通道号
**           pdmatMsg       DMA 相关信息
** 输　出  : PX_ERROR or ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT     API_DmaJobAdd (UINT                 uiChannel,
                       PLW_DMA_TRANSACTION  pdmatMsg)
{
#define __SAFE()    if (!bInterContext) {   API_ThreadSafe();   }
#define __UNSAFE()  if (!bInterContext) {   API_ThreadUnsafe(); }

             INTREG             iregInterLevel;
    REGISTER __PDMA_CHANNEL     pdmacChannel;
    REGISTER __PDMA_WAITNODE    pdmanNodeNew;
             BOOL               bInterContext;
    
    if (__DMA_CHANNEL_INVALID(uiChannel)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "dma channel invalid.\r\n");
        _ErrorHandle(ERROR_DMA_CHANNEL_INVALID);
        return  (PX_ERROR);
    }
    
    if (!pdmatMsg) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pdmatMsg invalid.\r\n");
        _ErrorHandle(ERROR_DMA_TRANSMSG_INVALID);
        return  (PX_ERROR);
    }
    
    if (pdmatMsg->DMAT_stDataBytes > 
        _G_dmacChannel[uiChannel].DMAC_stMaxDataBytes) {                /*  数据量过大                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "data too large.\r\n");
        _ErrorHandle(ERROR_DMA_DATA_TOO_LARGE);
        return  (PX_ERROR);
    }
    
    bInterContext = API_InterContext();
    pdmacChannel  = __DMA_CHANNEL_GET(uiChannel);
    
    do {
        LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);           /*  关闭中断同时锁住 spinlock   */
        if (pdmacChannel->DMAC_iNodeCounter >= 
            pdmacChannel->DMAC_iMaxNode) {
            LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);      /*  打开中断同时解锁 spinlock   */
            
            if (bInterContext) {                                        /*  在中断中调用                */
                _ErrorHandle(ERROR_DMA_MAX_NODE);
                return  (PX_ERROR);
                
            } else if (API_SemaphoreBPend(pdmacChannel->DMAC_ulJobSync, 
                            LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {   /*  等待                        */
                return  (PX_ERROR);
            }
        } else {                                                        /*  满足插入节点条件            */
            LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);      /*  打开中断同时解锁 spinlock   */
            break;
        }
    } while (1);                                                        /*  循环等待                    */
    
    __SAFE();
    
    LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);               /*  关闭中断同时锁住 spinlock   */
    pdmanNodeNew = _dmaWaitnodeAlloc();                                 /*  使用快速分配节点            */
    LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);              /*  打开中断同时解锁 spinlock   */
    
    if (pdmanNodeNew) {                                                 /*  有可供快速使用的节点        */
        pdmanNodeNew->DMAN_bDontFree = LW_TRUE;                         /*  不需要释放操作              */
    } else {                                                            /*  需要进行动态内存分配        */
        
        if (bInterContext) {                                            /*  在中断中调用                */
            __UNSAFE();
            _ErrorHandle(ERROR_DMA_NO_FREE_NODE);                       /*  缺少空闲节点                */
            return  (PX_ERROR);
        }
        
        pdmanNodeNew = (__PDMA_WAITNODE)__SHEAP_ALLOC(sizeof(__DMA_WAITNODE));
        if (pdmanNodeNew) {
            pdmanNodeNew->DMAN_bDontFree = LW_FALSE;                    /*  需要释放操作                */
        
        } else {
            __UNSAFE();
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);                      /*  缺少内存                    */
            return  (PX_ERROR);
        }
    }
    
    pdmanNodeNew->DMAN_pdmatMsg = *pdmatMsg;                            /*  保存消息                    */
    
    LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);               /*  关闭中断同时锁住 spinlock   */
    _dmaInsertToWaitList(pdmacChannel, pdmanNodeNew);                   /*  插入 DMA 待处理队列         */
    if (pdmacChannel->DMAC_iNodeCounter == 1) {                         /*  只有唯一的一个节点          */
        if (pdmanNodeNew->DMAN_pdmatMsg.DMAT_pfuncStart) {
            pdmanNodeNew->DMAN_pdmatMsg.DMAT_pfuncStart(uiChannel,
            pdmanNodeNew->DMAN_pdmatMsg.DMAT_pvArgStart);               /*  执行启动回调                */
        }
        {
            INT     iRet = ERROR_NONE;
            __DMA_CHANNEL_TRANS(uiChannel, pdmatMsg, iRet);             /*  初始化传输诸元              */
            (VOID)iRet;                                                 /*  暂不处理错误                */
        }
    }
    LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);              /*  打开中断同时解锁 spinlock   */
    
    __UNSAFE();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DmaGetMaxDataBytes
** 功能描述: 获得一个 DMA 传输请求的最大数据量
** 输　入  : uiChannel      DMA 通道号
** 输　出  : 最大字节数
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT     API_DmaGetMaxDataBytes (UINT   uiChannel)
{
    if (__DMA_CHANNEL_INVALID(uiChannel)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "dma channel invalid.\r\n");
        _ErrorHandle(ERROR_DMA_CHANNEL_INVALID);
        return  (PX_ERROR);
    }
    
    return  ((INT)_G_dmacChannel[uiChannel].DMAC_stMaxDataBytes);
}
/*********************************************************************************************************
** 函数名称: API_DmaFlush
** 功能描述: 删除所有被延迟处理的传输请求 (不调用回调函数)
** 输　入  : uiChannel      DMA 通道号
** 输　出  : PX_ERROR or ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT     API_DmaFlush (UINT   uiChannel)
{
             INTREG             iregInterLevel;
    REGISTER __PDMA_CHANNEL     pdmacChannel;
    REGISTER __PDMA_WAITNODE    pdmanNode;
    
    if (__DMA_CHANNEL_INVALID(uiChannel)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "dma channel invalid.\r\n");
        _ErrorHandle(ERROR_DMA_CHANNEL_INVALID);
        return  (PX_ERROR);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    pdmacChannel = __DMA_CHANNEL_GET(uiChannel);                        /*  获得通道控制块              */
    
    API_ThreadSafe();
    
    pdmacChannel->DMAC_bIsInFlush = LW_TRUE;                            /*  开始进行 FLUSH 操作         */
    for (;;) {
        LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);           /*  关闭中断同时锁住 spinlock   */
        pdmanNode = _dmaGetFirstInWaitList(pdmacChannel);               /*  获得最近的一个节点          */
        if (!pdmanNode) {                                               /*  没有节点了                  */
            __DMA_CHANNEL_RESET(uiChannel);                             /*  复位通道                    */
            LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);      /*  打开中断同时解锁 spinlock   */
            break;                                                      /*  跳出循环                    */
        }
        _dmaDeleteFromWaitList(pdmacChannel, pdmanNode);                /*  从等待队列删除这个节点      */
        if (pdmanNode->DMAN_bDontFree) {
            _dmaWaitnodeFree(pdmanNode);                                /*  加入到空闲队列              */
            LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);      /*  打开中断同时解锁 spinlock   */
        
        } else {
            LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);      /*  打开中断同时解锁 spinlock   */
            __SHEAP_FREE((PVOID)pdmanNode);                             /*  释放到内存堆中              */
        }
    }
    pdmacChannel->DMAC_bIsInFlush = LW_FALSE;                           /*  结束 FLUSH 操作             */
    
    API_SemaphoreBPost(pdmacChannel->DMAC_ulJobSync);                   /*  释放同步信号量              */
    
    API_ThreadUnsafe();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DmaContext
** 功能描述: 指定通道 DMA 中断服务函数.这里不判断通道有效性,使用时请小心!
** 输　入  : uiChannel      DMA 通道号
** 输　出  : PX_ERROR or ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT     API_DmaContext (UINT   uiChannel)
{
             INTREG             iregInterLevel;
    REGISTER __PDMA_CHANNEL     pdmacChannel;
    REGISTER __PDMA_WAITNODE    pdmanNode;
    REGISTER __PDMA_WAITNODE    pdmanNodeNew;                           /*  最新需要处理的节点          */
    
    pdmacChannel = __DMA_CHANNEL_GET(uiChannel);                        /*  获得通道控制块              */
    
    if (pdmacChannel->DMAC_bIsInFlush) {                                /*  在执行 FLUSH 操作           */
        return  (ERROR_NONE);
    }
    
    LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);               /*  关闭中断同时锁住 spinlock   */
    pdmanNode = _dmaGetFirstInWaitList(pdmacChannel);                   /*  获得最近的一个节点          */
    if (!pdmanNode) {                                                   /*  没有节点了                  */
        LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);          /*  打开中断同时解锁 spinlock   */
        return  (ERROR_NONE);
    }
    _dmaDeleteFromWaitList(pdmacChannel, pdmanNode);                    /*  从等待队列删除这个节点      */
    LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);              /*  打开中断同时解锁 spinlock   */
    
    /*
     *  以最快方式插入新节点, 防止音频播放断续.
     */
    LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);               /*  关闭中断同时锁住 spinlock   */
    pdmanNodeNew = _dmaGetFirstInWaitList(pdmacChannel);                /*  获得最近的一个节点          */
    if (pdmanNodeNew) {                                                 /*  存在新节点                  */
        if (pdmanNodeNew->DMAN_pdmatMsg.DMAT_pfuncStart) {
            pdmanNodeNew->DMAN_pdmatMsg.DMAT_pfuncStart(uiChannel,
            pdmanNodeNew->DMAN_pdmatMsg.DMAT_pvArgStart);               /*  执行启动回调                */
        }
        {
            INT     iRet = ERROR_NONE;
            __DMA_CHANNEL_TRANS(uiChannel,
                                &pdmanNodeNew->DMAN_pdmatMsg,
                                iRet);                                  /*  初始化传输诸元              */
            (VOID)iRet;                                                 /*  暂不处理错误                */
        }
    }
    LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);              /*  打开中断同时解锁 spinlock   */
    
    if (pdmanNode->DMAN_pdmatMsg.DMAT_pfuncCallback) {
        pdmanNode->DMAN_pdmatMsg.DMAT_pfuncCallback(uiChannel,
        pdmanNode->DMAN_pdmatMsg.DMAT_pvArg);                           /*  调用回调函数                */
    }
    
    /*
     *  释放原有节点
     */
    if (pdmanNode->DMAN_bDontFree) {
        LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);           /*  关闭中断同时锁住 spinlock   */
        _dmaWaitnodeFree(pdmanNode);                                    /*  加入到空闲队列              */
        LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);          /*  打开中断同时解锁 spinlock   */
    
    } else {
        _excJobAdd((VOIDFUNCPTR)_HeapFree, 
                   (PVOID)_K_pheapSystem, 
                   (PVOID)pdmanNode,                                    /*  释放节点                    */
                   (PVOID)LW_FALSE,
                   0, 0, 0);                                            /*  添加到延迟作业队列处理      */
    }
    
    /*
     *  检查是否需要释放同步信号量.
     */
    LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);               /*  关闭中断同时锁住 spinlock   */
    if ((pdmacChannel->DMAC_iMaxNode - pdmacChannel->DMAC_iNodeCounter) == 1) {
        LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);          /*  打开中断同时解锁 spinlock   */
        API_SemaphoreBPost(pdmacChannel->DMAC_ulJobSync);               /*  释放同步信号量              */
    
    } else {
        LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);          /*  打开中断同时解锁 spinlock   */
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MAX_DMA_CHANNELS > 0 */
                                                                        /*  LW_CFG_DMA_EN   > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
