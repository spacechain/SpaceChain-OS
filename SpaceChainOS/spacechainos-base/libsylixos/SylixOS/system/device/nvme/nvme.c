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
** 文   件   名: nvme.c
**
** 创   建   人: Hui.Kai (惠凯)
**
** 文件创建日期: 2017 年 7 月 17 日
**
** 描        述: NVMe 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_PCI_DRV
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_NVME_EN > 0)
#include "linux/compat.h"
#include "linux/bitops.h"
#include "nvme.h"
#include "nvmeLib.h"
#include "nvmeDrv.h"
#include "nvmeDev.h"
#include "nvmeCtrl.h"
/*********************************************************************************************************
  PRP 缓存大小配置
*********************************************************************************************************/
#define NVME_PRP_BLOCK_SIZE     (1 << 12)
#define NVME_PRP_BLOCK_MASK     (NVME_PRP_BLOCK_SIZE - 1)
/*********************************************************************************************************
  事件操作
*********************************************************************************************************/
#define NVME_QUEUE_LOCK(q)                  \
        API_SemaphoreMPend((q)->NVMEQUEUE_hTagMuteSem, LW_OPTION_WAIT_INFINITE)
#define NVME_QUEUE_UNLOCK(q)                \
        API_SemaphoreMPost((q)->NVMEQUEUE_hTagMuteSem)

#define NVME_QUEUE_WSYNC(q, cmdid, timeout) \
        API_SemaphoreBPend((q)->NVMEQUEUE_hSyncBSem[cmdid], timeout)
#define NVME_QUEUE_SSYNC(q, cmdid)          \
        API_SemaphoreBPost((q)->NVMEQUEUE_hSyncBSem[cmdid])
/*********************************************************************************************************
  监控线程声明
*********************************************************************************************************/
extern PVOID  __nvmeMonitorThread(PVOID  pvArg);
/*********************************************************************************************************
** 函数名称: __nvmeWaitReady
** 功能描述: 等待控制器 Ready
** 输　入  : hCtrl     控制器句柄
**           ullCap    控制器 Capability
**           bEnabled  控制器是否使能
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nvmeWaitReady (NVME_CTRL_HANDLE  hCtrl, UINT64  ullCap, BOOL  bEnabled)
{
    INT64   i64Timeout = ((NVME_CAP_TIMEOUT(ullCap) + 1) * 500 * 1000 / LW_TICK_HZ) + API_TimeGet64();
    UINT32  uiBit      = bEnabled ? NVME_CSTS_RDY : 0;

    /*
     *  控制器 Ready 需要等待一段时间
     */
    while ((NVME_CTRL_READ(hCtrl, NVME_REG_CSTS) & NVME_CSTS_RDY) != uiBit) {
        API_TimeSleep(1);
        if (API_TimeGet64() > i64Timeout) {
            NVME_LOG(NVME_LOG_ERR, "device not ready; aborting %s\n",
                     bEnabled ? "initialisation" : "reset");
            _ErrorHandle(ENODEV);
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nvmeCtrlDisable
** 功能描述: 禁能控制器
** 输　入  : hCtrl    控制器句柄
**           ullCap   控制器 Capability
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nvmeCtrlDisable (NVME_CTRL_HANDLE  hCtrl, UINT64  ullCap)
{
    hCtrl->NVMECTRL_uiCtrlConfig &= ~NVME_CC_SHN_MASK;
    hCtrl->NVMECTRL_uiCtrlConfig &= ~NVME_CC_ENABLE;

    NVME_CTRL_WRITE(hCtrl, NVME_REG_CC, hCtrl->NVMECTRL_uiCtrlConfig);

    /*
     *  控制器禁能操作后需要等待控制器恢复 Ready 状态
     */
    return  (__nvmeWaitReady(hCtrl, ullCap, LW_FALSE));
}
/*********************************************************************************************************
** 函数名称: __nvmeCtrlEnable
** 功能描述: 使能控制器
** 输　入  : hCtrl    控制器句柄
**           ullCap   控制器 Capability
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nvmeCtrlEnable (NVME_CTRL_HANDLE  hCtrl, UINT64  ullCap)
{
    UINT  uiPageMin   = NVME_CAP_MPSMIN(ullCap) + 12;
    UINT  uiPageShift = LW_CFG_VMM_PAGE_SHIFT;

    if (uiPageShift < uiPageMin) {
        NVME_LOG(NVME_LOG_ERR, "minimum device page size %u too large for host (%u)\n\r",
                 1 << uiPageMin, 1 << uiPageShift);
        _ErrorHandle(ENODEV);
        return  (PX_ERROR);
    }

    /*
     *  配置控制器参数
     */
    hCtrl->NVMECTRL_uiPageSize    = 1 << uiPageShift;
    hCtrl->NVMECTRL_uiCtrlConfig  = NVME_CC_CSS_NVM;
    hCtrl->NVMECTRL_uiCtrlConfig |= (uiPageShift - 12) << NVME_CC_MPS_SHIFT;
    hCtrl->NVMECTRL_uiCtrlConfig |= NVME_CC_ARB_RR | NVME_CC_SHN_NONE;
    hCtrl->NVMECTRL_uiCtrlConfig |= NVME_CC_IOSQES | NVME_CC_IOCQES;
    hCtrl->NVMECTRL_uiCtrlConfig |= NVME_CC_ENABLE;

    NVME_CTRL_WRITE(hCtrl, NVME_REG_CC, hCtrl->NVMECTRL_uiCtrlConfig);

    /*
     *  控制器使能操作后需要等待控制器恢复 Ready 状态
     */
    return  (__nvmeWaitReady(hCtrl, ullCap, LW_TRUE));
}
/*********************************************************************************************************
** 函数名称: __nvmeCmdInfo
** 功能描述: 获取CMD信息首地址
** 输　入  : hNvmeQueue    命令队列
** 输　出  : 命令信息地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static NVME_CMD_INFO_HANDLE  __nvmeCmdInfo (NVME_QUEUE_HANDLE  hNvmeQueue)
{
    return  ((NVME_CMD_INFO_HANDLE)&hNvmeQueue->NVMEQUEUE_ulCmdIdData[BITS_TO_LONGS(hNvmeQueue->NVMEQUEUE_usDepth)]);
}
/*********************************************************************************************************
** 函数名称: __specialCompletion
** 功能描述: 特殊同步处理函数
** 输　入  : hNvmeQueue    命令队列
**           pvCtx         命令状态
**           pCompletion   命令完成条目
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __specialCompletion (NVME_QUEUE_HANDLE       hNvmeQueue,
                                  PVOID                   pvCtx,
                                  NVME_COMPLETION_HANDLE  hCompletion)
{
    if ((pvCtx == CMD_CTX_CANCELLED) || 
        (pvCtx == CMD_CTX_FLUSH)) {
        return;
    
    } else if (pvCtx == CMD_CTX_COMPLETED) {
        NVME_LOG(NVME_LOG_ERR, "completed id %d twice on queue %d\n",
                 hCompletion->NVMECOMPLETION_usCmdId,
                 le16_to_cpu(hCompletion->NVMECOMPLETION_usSqid));
                 
    } else if (pvCtx == CMD_CTX_INVALID) {
        NVME_LOG(NVME_LOG_ERR, "invalid id %d completed on queue %d\n",
                 hCompletion->NVMECOMPLETION_usCmdId,
                 le16_to_cpu(hCompletion->NVMECOMPLETION_usSqid));
                 
    } else {
        NVME_LOG(NVME_LOG_ERR, "unknown special completion %p\n", pvCtx);
    }
}
/*********************************************************************************************************
** 函数名称: __syncCompletion
** 功能描述: 命令同步完成
** 输　入  : hNvmeQueue    命令队列
**           pvCtx         同步参数
**           hCompletion   命令完成队列
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void  __syncCompletion (NVME_QUEUE_HANDLE       hNvmeQueue,
                               PVOID                   pvCtx,
                               NVME_COMPLETION_HANDLE  hCompletion)
{
    SYNC_CMD_INFO_HANDLE   hCmdInfo = (SYNC_CMD_INFO_HANDLE)pvCtx;
    REGISTER UINT16        usCmdId  = hCompletion->NVMECOMPLETION_usCmdId;
    REGISTER UINT16        usStatus;

    /*
     *  获取命令完成队列中的返回状态信息
     */
    hCmdInfo->SYNCCMDINFO_uiResult = le32_to_cpu(hCompletion->NVMECOMPLETION_uiResult);
    usStatus                       = le16_to_cpu(hCompletion->NVMECOMPLETION_usStatus) >> 1;
    hCmdInfo->SYNCCMDINFO_iStatus  = usStatus;

    NVME_QUEUE_SSYNC(hNvmeQueue, usCmdId);
}
/*********************************************************************************************************
** 函数名称: __cmdIdFree
** 功能描述: 释放命令 Id
** 输　入  : hNvmeQueue     命令队列
**           iCmdId         命令 ID
**           ppfCompletion  命令完成队列条目
** 输　出  : 回调函数参数
** 全局变量:
** 调用模块
*********************************************************************************************************/
static PVOID  __cmdIdFree (NVME_QUEUE_HANDLE    hNvmeQueue,
                           INT                  iCmdId,
                           nvme_completion_fn  *ppfCompletion)
{
    PVOID                 pvCtx;
    NVME_CMD_INFO_HANDLE  hCmdInfo = __nvmeCmdInfo(hNvmeQueue);

    if (iCmdId >= hNvmeQueue->NVMEQUEUE_usDepth) {                      /*  cmdid 超过队列深度的特殊操作*/
        *ppfCompletion = __specialCompletion;
        return  (CMD_CTX_INVALID);
    }

    if (ppfCompletion) {                                                /*  获取命令处理完成回调函数    */
        *ppfCompletion = hCmdInfo[iCmdId].NVMECMDINFO_pfCompletion;
    }

    pvCtx = hCmdInfo[iCmdId].NVMECMDINFO_pvCtx;
    hCmdInfo[iCmdId].NVMECMDINFO_pfCompletion = __specialCompletion;
    hCmdInfo[iCmdId].NVMECMDINFO_pvCtx        = CMD_CTX_COMPLETED;

    /*
     *  回收本次命令传输使用的 cmdid
     */
    generic_clear_bit(iCmdId, hNvmeQueue->NVMEQUEUE_ulCmdIdData);

    return  (pvCtx);
}
/*********************************************************************************************************
** 函数名称: __cmdIdCancel
** 功能描述: 取消命令 Id
** 输　入  : hNvmeQueue     命令队列
**           iCmdId         命令 ID
**           ppfCompletion  命令完成队列条目
** 输　出  : 回调函数参数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PVOID  __cmdIdCancel (NVME_QUEUE_HANDLE    hNvmeQueue,
                             INT                  iCmdId,
                             nvme_completion_fn  *ppfCompletion)
{
    PVOID                 pvCtx;
    NVME_CMD_INFO_HANDLE  hCmdInfo = __nvmeCmdInfo(hNvmeQueue);

    if (ppfCompletion) {                                                /*  获取命令处理完成回调函数    */
        *ppfCompletion = hCmdInfo[iCmdId].NVMECMDINFO_pfCompletion;
    }

    pvCtx = hCmdInfo[iCmdId].NVMECMDINFO_pvCtx;
    hCmdInfo[iCmdId].NVMECMDINFO_pfCompletion = __specialCompletion;
    hCmdInfo[iCmdId].NVMECMDINFO_pvCtx        = CMD_CTX_CANCELLED;

    /*
     *  回收本次命令传输使用的cmdid
     */
    generic_clear_bit(iCmdId, hNvmeQueue->NVMEQUEUE_ulCmdIdData);

    return  (pvCtx);
}
/*********************************************************************************************************
** 函数名称: __cmdIdAlloc
** 功能描述: 申请一个命令ID
** 输　入  : hNvmeQueue    命令队列
**           pvCtx         回调函数参数
**           pfHandler     回调函数
**           uiTimeout     超时时间
** 输　出  : ERROR or CMDID
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __cmdIdAlloc (NVME_QUEUE_HANDLE      hNvmeQueue,
                          PVOID                  pvCtx,
                          nvme_completion_fn     pfHandler,
                          UINT                   uiTimeout)
{
    INT                   iCmdId;
    INTREG                iregInterLevel;
    NVME_CMD_INFO_HANDLE  hCmdInfo = __nvmeCmdInfo(hNvmeQueue);

    NVME_QUEUE_LOCK(hNvmeQueue);
    
    for (;;) {                                                          /*  从 ID 标签池中获取可用的标签*/
        LW_SPIN_LOCK_QUICK(&hNvmeQueue->NVMEQUEUE_QueueLock, &iregInterLevel);
        iCmdId = hNvmeQueue->NVMEQUEUE_uiNextTag;
        hNvmeQueue->NVMEQUEUE_uiNextTag++;
        if (hNvmeQueue->NVMEQUEUE_uiNextTag >= hNvmeQueue->NVMEQUEUE_usDepth) {
            hNvmeQueue->NVMEQUEUE_uiNextTag  = 0;
        }
        if (!generic_test_bit(iCmdId, hNvmeQueue->NVMEQUEUE_ulCmdIdData)) {
            generic_set_bit(iCmdId, hNvmeQueue->NVMEQUEUE_ulCmdIdData);
            LW_SPIN_UNLOCK_QUICK(&hNvmeQueue->NVMEQUEUE_QueueLock, iregInterLevel);
            break;
        
        } else {
            LW_SPIN_UNLOCK_QUICK(&hNvmeQueue->NVMEQUEUE_QueueLock, iregInterLevel);
        }
    }

    NVME_QUEUE_UNLOCK(hNvmeQueue);

    /*
     *  填充回调函数参数
     */
    hCmdInfo[iCmdId].NVMECMDINFO_pfCompletion = pfHandler;
    hCmdInfo[iCmdId].NVMECMDINFO_pvCtx        = pvCtx;
    
#if NVME_ID_DELAYED_RECOVERY > 0
    hCmdInfo[iCmdId].NVMECMDINFO_i64Timeout = API_TimeGet64() + uiTimeout;
#endif
                                                                        
    return  (iCmdId);
}
/*********************************************************************************************************
** 函数名称: __cmdIdAllocKillable
** 功能描述: 阻塞申请 cmdid，能够被唤醒
** 输　入  : hNvmeQueue    命令队列
**           pvCtx         回调函数参数
**           pfHandler     回调函数
**           uiTimeout     超时时间
** 输　出  : ERROR or iCmdId
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __cmdIdAllocKillable (NVME_QUEUE_HANDLE      hNvmeQueue,
                                  PVOID                  pvCtx,
                                  nvme_completion_fn     pfHandler,
                                  UINT                   uiTimeout)
{
    INT   iCmdId;

    iCmdId = __cmdIdAlloc(hNvmeQueue, pvCtx, pfHandler, uiTimeout);

    return  ((iCmdId < 0) ? PX_ERROR : iCmdId);
}
/*********************************************************************************************************
** 函数名称: __nvmeCmdSubmit
** 功能描述: 发送命令
** 输　入  : hNvmeQueue    命令队列
**           hCmd          需要发送的命令
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __nvmeCmdSubmit (NVME_QUEUE_HANDLE  hNvmeQueue, NVME_COMMAND_HANDLE  hCmd)
{
   UINT16   usTail;
   INTREG   iregInterLevel;

   LW_SPIN_LOCK_QUICK(&hNvmeQueue->NVMEQUEUE_QueueLock, &iregInterLevel);
   usTail = hNvmeQueue->NVMEQUEUE_usSqTail;
   lib_memcpy(&hNvmeQueue->NVMEQUEUE_hSubmissionQueue[usTail], hCmd, sizeof(NVME_COMMAND_CB));
   
   /*
    *  若当前 SQ Tail 达到命令队列深度，则回到队列头
    */
   if (++usTail == hNvmeQueue->NVMEQUEUE_usDepth) {
       usTail = 0;
   }
                                                                        
   write32(htole32(usTail), (addr_t)hNvmeQueue->NVMEQUEUE_puiDoorBell); /*  通知 NVMe 控制器有新命令产生*/
   hNvmeQueue->NVMEQUEUE_usSqTail = usTail;                             /*  更新当前 SQ 的Tail          */
   LW_SPIN_UNLOCK_QUICK(&hNvmeQueue->NVMEQUEUE_QueueLock, iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: __nvmeCommandAbort
** 功能描述: 终止命令
** 输　入  : hNvmeQueue    命令队列
**           iCmdId        命令 ID
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __nvmeCommandAbort (NVME_QUEUE_HANDLE  hNvmeQueue, INT  iCmdId)
{
    INTREG   iregInterLevel;

    LW_SPIN_LOCK_QUICK(&hNvmeQueue->NVMEQUEUE_QueueLock, &iregInterLevel);
    __cmdIdCancel(hNvmeQueue, iCmdId, LW_NULL);
    LW_SPIN_UNLOCK_QUICK(&hNvmeQueue->NVMEQUEUE_QueueLock, iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: __nvmeSyncCmdSubmit
** 功能描述: 发送同步命令
** 输　入  : hNvmeQueue    命令队列
**           hCmd          需要发送的命令
**           puiResult     命令返回
**           uiTimeout     命令超时时间
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __nvmeSyncCmdSubmit (NVME_QUEUE_HANDLE     hNvmeQueue,
                                NVME_COMMAND_HANDLE   hCmd,
                                UINT32               *puiResult,
                                UINT                  uiTimeout)
{
    INT                 iCmdId;
    SYNC_CMD_INFO_CB    cmdInfo;

    /*
     *  申请空闲的 cmdid，并注册命令完成回调函数
     */
    cmdInfo.SYNCCMDINFO_iStatus = -EINTR;
    iCmdId = __cmdIdAllocKillable(hNvmeQueue, &cmdInfo, __syncCompletion, uiTimeout);
    if (iCmdId < 0) {
        return  (iCmdId);
    }

    hCmd->tCommonCmd.NVMECOMMONCMD_usCmdId = (UINT16)iCmdId;            /*  填充命令对应的 cmdid        */

    __nvmeCmdSubmit(hNvmeQueue, hCmd);

    NVME_QUEUE_WSYNC(hNvmeQueue, iCmdId, uiTimeout);                    /*  等待命令完成同步信号        */

    if (cmdInfo.SYNCCMDINFO_iStatus == -EINTR) {                        /*  命令返回状态错误处理        */
        NVME_LOG(NVME_LOG_ERR, "nvme cmd %d timeout.\r\n", hCmd->tCommonCmd.NVMECOMMONCMD_ucOpcode);
        __nvmeCommandAbort(hNvmeQueue, iCmdId);
        return   (PX_ERROR);
    }

    if (puiResult) {                                                    /* 获取回调函数参数中的返回结果 */
        *puiResult = cmdInfo.SYNCCMDINFO_uiResult;
    }

    return  (cmdInfo.SYNCCMDINFO_iStatus);
}
/*********************************************************************************************************
** 函数名称: __nvmeAdminCmdSubmit
** 功能描述: 发送 Admin 命令
** 输　入  : hCtrl         控制器句柄
**           hCmd          需要发送的命令
**           puiResult     命令返回值
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nvmeAdminCmdSubmit (NVME_CTRL_HANDLE  hCtrl, NVME_COMMAND_HANDLE  hCmd, UINT32  *puiResult)
{
    return  (__nvmeSyncCmdSubmit(hCtrl->NVMECTRL_hQueues[0], hCmd, 
                                 puiResult, (UINT)NVME_ADMIN_TIMEOUT));
}
/*********************************************************************************************************
** 函数名称: __adapterQueueDelete
** 功能描述: 删除命令队列
** 输　入  : hCtrl         控制器句柄
**           ucOpcode      命令码
**           usId          队列 ID
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __adapterQueueDelete (NVME_CTRL_HANDLE  hCtrl, UINT8  ucOpcode, UINT16  usId)
{
    INT                  iStatus;
    NVME_COMMAND_CB      tCmd;

    lib_bzero(&tCmd, sizeof(tCmd));
    tCmd.tDeleteQueue.NVMEDELETEQUEUE_ucOpcode = ucOpcode;
    tCmd.tDeleteQueue.NVMEDELETEQUEUE_usQid    = cpu_to_le16(usId);

    iStatus = __nvmeAdminCmdSubmit(hCtrl, &tCmd, LW_NULL);
    if (iStatus) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __adapterCqAlloc
** 功能描述: 创建命令完成队列
** 输　入  : hCtrl         控制器句柄
**           usQid         队列 ID
**           hNvmeQueue    命令队列
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __adapterCqAlloc (NVME_CTRL_HANDLE  hCtrl, UINT16  usQid, NVME_QUEUE_HANDLE  hNvmeQueue)
{
    INT                  iStatus;
    INT                  iFlags = NVME_QUEUE_PHYS_CONTIG | NVME_CQ_IRQ_ENABLED;
    NVME_COMMAND_CB      tCmd;

    lib_bzero(&tCmd, sizeof(tCmd));
    tCmd.tCreateCq.NVMECREATECQ_ucOpcode    = NVME_ADMIN_CREATE_CQ;
    tCmd.tCreateCq.NVMECREATECQ_ullPrp1     = cpu_to_le64((ULONG)hNvmeQueue->NVMEQUEUE_hCompletionQueue);
    tCmd.tCreateCq.NVMECREATECQ_usCqid      = cpu_to_le16(usQid);
    tCmd.tCreateCq.NVMECREATECQ_usQsize     = cpu_to_le16(hNvmeQueue->NVMEQUEUE_usDepth - 1);
    tCmd.tCreateCq.NVMECREATECQ_usCqFlags   = cpu_to_le16(iFlags);
    tCmd.tCreateCq.NVMECREATECQ_usIrqVector = cpu_to_le16(hNvmeQueue->NVMEQUEUE_usCqVector);

    iStatus = __nvmeAdminCmdSubmit(hCtrl, &tCmd, LW_NULL);
    if (iStatus) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __adapterSqAlloc
** 功能描述: 创建命令发送队列
** 输　入  : hCtrl         控制器句柄
**           usQid         队列 ID
**           hNvmeQueue    命令队列
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __adapterSqAlloc (NVME_CTRL_HANDLE  hCtrl, UINT16  usQid, NVME_QUEUE_HANDLE  hNvmeQueue)
{
    INT                  iStatus;
    INT                  iFlags = NVME_QUEUE_PHYS_CONTIG | NVME_SQ_PRIO_MEDIUM;
    NVME_COMMAND_CB      tCmd;

    lib_bzero(&tCmd, sizeof(tCmd));
    tCmd.tCreateSq.NVMECREATESQ_ucOpcode  = NVME_ADMIN_CREATE_SQ;
    tCmd.tCreateSq.NVMECREATESQ_ullPrp1   = cpu_to_le64((ULONG)hNvmeQueue->NVMEQUEUE_hSubmissionQueue);
    tCmd.tCreateSq.NVMECREATESQ_usSqid    = cpu_to_le16(usQid);
    tCmd.tCreateSq.NVMECREATESQ_usQsize   = cpu_to_le16(hNvmeQueue->NVMEQUEUE_usDepth - 1);
    tCmd.tCreateSq.NVMECREATESQ_usSqFlags = cpu_to_le16(iFlags);
    tCmd.tCreateSq.NVMECREATESQ_usCqid    = cpu_to_le16(usQid);

    iStatus = __nvmeAdminCmdSubmit(hCtrl, &tCmd, LW_NULL);
    if (iStatus) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __adapterCqDelete
** 功能描述: 删除命令完成队列
** 输　入  : hCtrl         控制器句柄
**           usCqId        队列 ID
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __adapterCqDelete (NVME_CTRL_HANDLE  hCtrl, UINT16  usCqId)
{
    return  (__adapterQueueDelete(hCtrl, NVME_ADMIN_DELETE_CQ, usCqId));
}
/*********************************************************************************************************
** 函数名称: __adapterSqDelete
** 功能描述: 删除命令发送队列
** 输　入  : hCtrl         控制器句柄
**           usSqId        队列 ID
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __adapterSqDelete (NVME_CTRL_HANDLE  hCtrl, UINT16  usSqId)
{
    return  (__adapterQueueDelete(hCtrl, NVME_ADMIN_DELETE_SQ, usSqId));
}
/*********************************************************************************************************
** 函数名称: __nvmeCqCmdsAlloc
** 功能描述: 申请完成发送队列
** 输　入  : hCtrl         控制器句柄
**           hNvmeQueue    命令队列
**           iQueueId      队列 ID
**           iQCmdDepth    队列命令深度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nvmeCqCmdsAlloc (NVME_CTRL_HANDLE     hCtrl,
                               NVME_QUEUE_HANDLE    hNvmeQueue,
                               INT                  iQueueId,
                               INT                  iQCmdDepth)
{
    PVOID  pvBuff;

    pvBuff = API_CacheDmaMallocAlign(NVME_CQ_SIZE(iQCmdDepth), LW_CFG_VMM_PAGE_SIZE);
    hNvmeQueue->NVMEQUEUE_hCompletionQueue = (NVME_COMPLETION_HANDLE)pvBuff;
    if (hNvmeQueue->NVMEQUEUE_hCompletionQueue == LW_NULL) {
        NVME_LOG(NVME_LOG_ERR, "alloc aligned vmm dma buffer failed ctrl %d.\r\n",
                 hCtrl->NVMECTRL_uiIndex);
        return  (PX_ERROR);
    }
    API_CacheDmaFlush((PVOID)hNvmeQueue->NVMEQUEUE_hCompletionQueue, NVME_CQ_SIZE(iQCmdDepth));
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nvmeSqCmdsAlloc
** 功能描述: 申请命令发送队列
** 输　入  : hCtrl         控制器句柄
**           hNvmeQueue    命令队列
**           iQueueId      队列 ID
**           iQCmdDepth    队列命令深度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nvmeSqCmdsAlloc (NVME_CTRL_HANDLE     hCtrl,
                               NVME_QUEUE_HANDLE    hNvmeQueue,
                               INT                  iQueueId,
                               INT                  iQCmdDepth)
{
    PVOID  pvBuff;

    pvBuff = API_CacheDmaMallocAlign(NVME_SQ_SIZE(iQCmdDepth), LW_CFG_VMM_PAGE_SIZE);
    hNvmeQueue->NVMEQUEUE_hSubmissionQueue = (NVME_COMMAND_HANDLE)pvBuff;
    if (hNvmeQueue->NVMEQUEUE_hSubmissionQueue == LW_NULL) {
        NVME_LOG(NVME_LOG_ERR, "alloc aligned vmm dma buffer failed ctrl %d.\r\n",
                 hCtrl->NVMECTRL_uiIndex);
        return  (PX_ERROR);
    }
    API_CacheDmaFlush((PVOID)hNvmeQueue->NVMEQUEUE_hSubmissionQueue, NVME_SQ_SIZE(iQCmdDepth));

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nvmeIdentify
** 功能描述: 发送 Identify 命令
** 输　入  : hCtrl         控制器句柄
**           uiNsid        命名空间ID (Namespace Identifier)
**           uiCns         指定命令返回信息，0:命名空间结构体、1:控制器结构体...
**           ulDmaAddr     返回信息的缓冲地址
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nvmeIdentify (NVME_CTRL_HANDLE  hCtrl, UINT  uiNsid, UINT  uiCns, dma_addr_t  ulDmaAddr)
{
    NVME_COMMAND_CB      tCmd;

    lib_bzero(&tCmd, sizeof(tCmd));
    tCmd.tIdentify.NVMEIDENTIFY_ucOpcode  = NVME_ADMIN_IDENTIFY;
    tCmd.tIdentify.NVMEIDENTIFY_uiNsid    = cpu_to_le32(uiNsid);
    tCmd.tIdentify.NVMEIDENTIFY_ullPrp1   = cpu_to_le64(ulDmaAddr);
    tCmd.tIdentify.NVMEIDENTIFY_uiCns     = cpu_to_le32(uiCns);

    return  (__nvmeAdminCmdSubmit(hCtrl, &tCmd, LW_NULL));
}
/*********************************************************************************************************
** 函数名称: __nvmeFeaturesSet
** 功能描述: 设置控制器特性 features
** 输　入  : hCtrl         控制器句柄
**           uiFid         Feature Identifiers
**           uiDword11     命令 Dword11
**           ulDmaAddr     返回信息的缓冲地址
**           uiResult      命令返回值
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nvmeFeaturesSet (NVME_CTRL_HANDLE  hCtrl,
                               UINT              uiFid,
                               UINT              uiDword11,
                               dma_addr_t        ulDmaAddr,
                               UINT32           *uiResult)
{
    NVME_COMMAND_CB      tCmd;

    lib_bzero(&tCmd, sizeof(tCmd));
    tCmd.tFeatures.NVMEFEATURE_ucOpcode  = NVME_ADMIN_SET_FEATURES;
    tCmd.tFeatures.NVMEFEATURE_ullPrp1   = cpu_to_le64(ulDmaAddr);
    tCmd.tFeatures.NVMEFEATURE_uiFid     = cpu_to_le32(uiFid);
    tCmd.tFeatures.NVMEFEATURE_uiDword11 = cpu_to_le32(uiDword11);

    return  (__nvmeAdminCmdSubmit(hCtrl, &tCmd, uiResult));
}
/*********************************************************************************************************
** 函数名称: __nvmePrpsSetup
** 功能描述: 设置队列PRP
** 输　入  : hNvmeQueue     命令队列
**           hRwCmd         读写命令
**           iCmdId         命令 ID
**           ulDmaAddr      DMA传输起始地址
**           iTotalLen      DMA传输长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nvmePrpsSetup (NVME_QUEUE_HANDLE        hNvmeQueue,
                             NVME_RW_CMD_HANDLE       hRwCmd,
                             INT                      iCmdId,
                             dma_addr_t               ulDmaAddr,
                             INT                      iTotalLen)
{
    INT        i       = 0;
    INT        iLength = iTotalLen;
    UINT64     ullAddr = ulDmaAddr;
    INT        iOffset = ullAddr & NVME_PRP_BLOCK_MASK;
    UINT64    *pPrpList;

    hRwCmd->NVMERWCMD_ullPrp1 = cpu_to_le64(ullAddr);

    iLength -= (NVME_PRP_BLOCK_SIZE - iOffset);
    if (iLength <= 0) {
        return  (iTotalLen);
    }

    ullAddr += (NVME_PRP_BLOCK_SIZE - iOffset);
    if (iLength <= NVME_PRP_BLOCK_SIZE) {
        hRwCmd->NVMERWCMD_ullPrp2 = cpu_to_le64(ullAddr);
        return  (iTotalLen);
    }

    pPrpList = (UINT64 *)((addr_t)hNvmeQueue->NVMEQUEUE_pvPrpBuf + iCmdId * NVME_PRP_BLOCK_SIZE);
    hRwCmd->NVMERWCMD_ullPrp2 = cpu_to_le64((dma_addr_t)pPrpList);

    do {
        pPrpList[i++] = cpu_to_le64(ullAddr);
        ullAddr      += NVME_PRP_BLOCK_SIZE;
        iLength      -= NVME_PRP_BLOCK_SIZE;
    } while (iLength > 0);

    return  (iTotalLen);
}
/*********************************************************************************************************
** 函数名称: __nvmeUserCmdSubmit
** 功能描述: 发送命令
** 输　入  : hDev           设备句柄
**           ulDmaAddr      缓冲区地址
**           ulBlkStart     起始扇区
**           ulBlkCount     扇区数量
**           uiDirection    读写模式
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nvmeUserCmdSubmit (NVME_DEV_HANDLE   hDev,
                                 dma_addr_t        ulDmaAddr,
                                 ULONG             ulStartBlk,
                                 ULONG             ulBlkCount,
                                 UINT              uiDirection)
{
    INT                      iCmdId;
    SYNC_CMD_INFO_CB         cmdInfo;
    NVME_RW_CMD_CB           hRwCmd;
    NVME_CTRL_HANDLE         hCtrl = hDev->NVMEDEV_hCtrl;
    NVME_QUEUE_HANDLE        hNvmeQueue;
    
#if LW_CFG_SMP_EN > 0
    REGISTER ULONG           ulCPUId = LW_CPU_GET_CUR_ID();
    REGISTER INT             iIndex;
    
    if (hCtrl->NVMECTRL_uiQueueCount > LW_NCPUS) {
        hNvmeQueue = hCtrl->NVMECTRL_hQueues[ulCPUId + 1];
    
    } else {
        iIndex     = ulCPUId % (hCtrl->NVMECTRL_uiQueueCount - 1);
        hNvmeQueue = hCtrl->NVMECTRL_hQueues[iIndex + 1];
    }

#else
    hNvmeQueue = hCtrl->NVMECTRL_hQueues[1];
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    lib_bzero(&hRwCmd, sizeof(hRwCmd));
    if (uiDirection == O_WRONLY) {
        hRwCmd.NVMERWCMD_ucOpcode = NVME_CMD_WRITE;
    } else {
        hRwCmd.NVMERWCMD_ucOpcode = NVME_CMD_READ;
    }

    hRwCmd.NVMERWCMD_uiNsid    = cpu_to_le32(hDev->NVMEDEV_uiNameSpaceId);
    hRwCmd.NVMERWCMD_ullSlba   = cpu_to_le64(ulStartBlk);
    hRwCmd.NVMERWCMD_usLength  = cpu_to_le16(ulBlkCount - 1);

    cmdInfo.SYNCCMDINFO_iStatus = -EINTR;
    iCmdId = __cmdIdAllocKillable(hNvmeQueue, &cmdInfo, __syncCompletion, (UINT)NVME_IO_TIMEOUT);
    if (iCmdId < 0) {
        return  (iCmdId);
    }

    hRwCmd.NVMERWCMD_usCmdId = (UINT16)iCmdId;

    /*
     *  设置 I/O 队列传输数据使用的 PRP
     */
    __nvmePrpsSetup(hNvmeQueue, &hRwCmd, iCmdId, ulDmaAddr, 
                    (INT)(ulBlkCount * (1 << hDev->NVMEDEV_uiSectorShift)));
    
    __nvmeCmdSubmit(hNvmeQueue, (NVME_COMMAND_HANDLE)&hRwCmd);
    
    NVME_QUEUE_WSYNC(hNvmeQueue, iCmdId, NVME_IO_TIMEOUT);

    if (cmdInfo.SYNCCMDINFO_iStatus == -EINTR) {
        NVME_LOG(NVME_LOG_ERR, "nvme cmd %d timeout, iCmdId %d.\r\n", hRwCmd.NVMERWCMD_ucOpcode, iCmdId);
        __nvmeCommandAbort(hNvmeQueue, iCmdId);
        return  (PX_ERROR);
    }

    return  (cmdInfo.SYNCCMDINFO_iStatus);
}
/*********************************************************************************************************
** 函数名称: __nvmeDsmCmdSubmit
** 功能描述: 发送 DSM 命令
** 输　入  : hDev           设备句柄
**           ulStartSector  起始扇区
**           ulEndSector    结束扇区
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if NVME_TRIM_EN > 0

static INT  __nvmeDsmCmdSubmit (NVME_DEV_HANDLE   hDev,
                                ULONG             ulStartSector,
                                ULONG             ulEndSector)
{
    NVME_DSM_RANGE_HANDLE    hNvmeDsmRange;
    NVME_COMMAND_CB          tCmd;
    NVME_CTRL_HANDLE         hCtrl = hDev->NVMEDEV_hCtrl;
    NVME_QUEUE_HANDLE        hNvmeQueue;
    
#if LW_CFG_SMP_EN > 0
    REGISTER ULONG           ulCPUId = LW_CPU_GET_CUR_ID();
    REGISTER INT             iIndex;
    
    if (hCtrl->NVMECTRL_uiQueueCount > LW_NCPUS) {
        hNvmeQueue = hCtrl->NVMECTRL_hQueues[ulCPUId + 1];
    
    } else {
        iIndex     = ulCPUId % (hCtrl->NVMECTRL_uiQueueCount - 1);
        hNvmeQueue = hCtrl->NVMECTRL_hQueues[iIndex + 1];
    }

#else
    hNvmeQueue = hCtrl->NVMECTRL_hQueues[1];
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    /*
     *  填充 DSM Range 控制块
     */
    hNvmeDsmRange = hDev->NVMEDEV_hDsmRange;
    hNvmeDsmRange->NVMEDSMRANGE_uiCattr = cpu_to_le32(0);
    hNvmeDsmRange->NVMEDSMRANGE_uiNlb   = cpu_to_le32(ulEndSector - ulStartSector);
    hNvmeDsmRange->NVMEDSMRANGE_uiSlba  = cpu_to_le64(ulStartSector);

    lib_bzero(&tCmd, sizeof(tCmd));
    tCmd.tDsm.NVMEDSMCMD_ucOpcode       = NVME_CMD_DSM;
    tCmd.tDsm.NVMEDSMCMD_uiNsid         = cpu_to_le32(hDev->NVMEDEV_uiNameSpaceId);
    tCmd.tDsm.NVMEDSMCMD_ullPrp1        = cpu_to_le64((ULONG)hNvmeDsmRange);
    tCmd.tDsm.NVMEDSMCMD_uiNr           = 0;
    tCmd.tDsm.NVMEDSMCMD_uiAttributes   = cpu_to_le32(NVME_DSMGMT_AD);

    return  (__nvmeSyncCmdSubmit(hNvmeQueue, &tCmd, LW_NULL, (UINT)NVME_IO_TIMEOUT));
}

#endif                                                                  /*  NVME_TRIM_EN > 0            */
/*********************************************************************************************************
** 函数名称: __nvmeFlushCmdSubmit
** 功能描述: 发送 FLUSH 命令
** 输　入  : hDev           设备句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if NVME_CACHE_EN > 0

static INT  __nvmeFlushCmdSubmit (NVME_DEV_HANDLE   hDev)
{
    NVME_COMMAND_CB          tCmd;
    NVME_CTRL_HANDLE         hCtrl = hDev->NVMEDEV_hCtrl;
    NVME_QUEUE_HANDLE        hNvmeQueue;

#if LW_CFG_SMP_EN > 0
    REGISTER ULONG           ulCPUId = LW_CPU_GET_CUR_ID();
    REGISTER INT             iIndex;

    if (hCtrl->NVMECTRL_uiQueueCount > LW_NCPUS) {
        hNvmeQueue = hCtrl->NVMECTRL_hQueues[ulCPUId + 1];

    } else {
        iIndex     = ulCPUId % (hCtrl->NVMECTRL_uiQueueCount - 1);
        hNvmeQueue = hCtrl->NVMECTRL_hQueues[iIndex + 1];
    }

#else
    hNvmeQueue = hCtrl->NVMECTRL_hQueues[1];
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    lib_bzero(&tCmd, sizeof(tCmd));
    tCmd.tCommonCmd.NVMECOMMONCMD_ucOpcode = NVME_CMD_FLUSH;
    tCmd.tCommonCmd.NVMECOMMONCMD_uiNsid   = cpu_to_le32(hDev->NVMEDEV_uiNameSpaceId);

    return  (__nvmeSyncCmdSubmit(hNvmeQueue, &tCmd, LW_NULL, (UINT)NVME_IO_TIMEOUT));
}

#endif                                                                  /*  NVME_CACHE_EN > 0           */
/*********************************************************************************************************
** 函数名称: __nvmeBlkRd
** 功能描述: 块设备读操作
** 输　入  : hDev           设备句柄
**           pvWrtBuffer    缓冲区地址
**           ulBlkStart     起始扇区
**           ulBlkCount     扇区数量
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : pvRdBuffer 来自于 disk cache 属于内核内存范畴, 所以物理地址与虚拟地址相同.
*********************************************************************************************************/
static INT  __nvmeBlkRd (NVME_DEV_HANDLE   hDev,
                         VOID             *pvRdBuffer,
                         ULONG             ulStartBlk,
                         ULONG             ulBlkCount)
{
    INT                 i;
    ULONG               ulSecNum;
    ULONG               ulBytes;
    dma_addr_t          ulHandle;
    INT                 iStatus = PX_ERROR;

    ulBytes  = hDev->NVMEDEV_tBlkDev.BLKD_ulBytesPerSector;             /*  获取扇区大小                */
    ulHandle = (dma_addr_t)pvRdBuffer;                                  /*  物理地址与虚拟地址相同      */

    /*
     *  根据读取扇区数，进行分解传输，每次最大不超过控制器最大支持扇区数量
     */
    for (i = 0; i < ulBlkCount; i += ulSecNum) {
        ulSecNum = __MIN(ulBlkCount - i, hDev->NVMEDEV_uiMaxHwSectors);
        if (__nvmeUserCmdSubmit(hDev, ulHandle, ulStartBlk, ulSecNum, O_RDONLY)) {
            goto    __done;
        }
        
        ulStartBlk += ulSecNum;
        ulHandle    = ulHandle + (ulBytes * ulSecNum);
    }

    iStatus = ERROR_NONE;

__done:
    return  (iStatus);
}
/*********************************************************************************************************
** 函数名称: __nvmeBlkWrt
** 功能描述: 块设备写操作
** 输　入  : hDev           设备句柄
**           pvBuffer       缓冲区地址
**           ulBlkStart     起始扇区
**           ulBlkCount     扇区数量
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : pvWrtBuffer 来自于 disk cache 属于内核内存范畴, 所以物理地址与虚拟地址相同.
*********************************************************************************************************/
static INT  __nvmeBlkWrt (NVME_DEV_HANDLE   hDev,
                          VOID             *pvWrtBuffer,
                          ULONG             ulStartBlk,
                          ULONG             ulBlkCount)
{
    INT                 i;
    ULONG               ulSecNum;
    ULONG               ulBytes;
    dma_addr_t          ulHandle;
    INT                 iStatus = PX_ERROR;

    ulBytes  = hDev->NVMEDEV_tBlkDev.BLKD_ulBytesPerSector;             /*  获取扇区大小                */
    ulHandle = (dma_addr_t)pvWrtBuffer;                                 /*  物理地址与虚拟地址相同      */

    /*
     *  根据写入扇区数，进行分解传输，每次最大不超过控制器最大支持扇区数量
     */
    for (i = 0; i < ulBlkCount; i += ulSecNum) {
        ulSecNum = __MIN(ulBlkCount - i, hDev->NVMEDEV_uiMaxHwSectors);
        if (__nvmeUserCmdSubmit(hDev, ulHandle, ulStartBlk, ulSecNum, O_WRONLY)) {
            goto    __done;
        }

        ulStartBlk += ulSecNum;
        ulHandle    = ulHandle + (ulBytes * ulSecNum);
    }

    iStatus = ERROR_NONE;

__done:
    return  (iStatus);
}
/*********************************************************************************************************
** 函数名称: __nvmeBlkIoctl
** 功能描述: 块设备 ioctl
** 输　入  : hDev          设备句柄
**           iCmd          控制命令
**           lArg          控制参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : FIOTRIM 命令与读写命令互斥已经在 disk cache 里面得到保证.
*********************************************************************************************************/
static INT  __nvmeBlkIoctl (NVME_DEV_HANDLE    hDev,
                            INT                iCmd,
                            LONG               lArg)
{
    NVME_DEV_HANDLE     hNvmeDev;
    NVME_CTRL_HANDLE    hCtrl;                                          /* 控制器句柄                   */
    PLW_BLK_INFO        hBlkInfo;                                       /* 设备信息                     */

    if (!hDev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    hNvmeDev = (NVME_DEV_HANDLE)hDev;

    switch (iCmd) {

    case FIOSYNC:
    case FIODATASYNC:
    case FIOFLUSH:                                                      /*  将缓存回写到磁盘            */
    case FIOSYNCMETA:
#if NVME_CACHE_EN > 0
    {
        INT  iRet;

        iRet = __nvmeFlushCmdSubmit(hNvmeDev);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }
#endif                                                                  /*  NVME_CACHE_EN > 0           */
        break;

    case FIOUNMOUNT:                                                    /*  卸载卷                      */
    case FIODISKINIT:                                                   /*  初始化设备                  */
    case FIODISKCHANGE:                                                 /*  磁盘媒质发生变化            */
        break;

    case FIOTRIM:                                                       /*  TRIM 回收机制（TPSFS支持）  */
#if NVME_TRIM_EN > 0                                                    /*  NVME_TRIM_EN                */
    {
        INT              iRet;
        PLW_BLK_RANGE    hBlkRange;

        hBlkRange = (PLW_BLK_RANGE)lArg;
        iRet = __nvmeDsmCmdSubmit(hNvmeDev, hBlkRange->BLKR_ulStartSector, hBlkRange->BLKR_ulEndSector);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }
#endif                                                                  /*  NVME_TRIM_EN                */
        break;

    case LW_BLKD_GET_SECSIZE:
    case LW_BLKD_GET_BLKSIZE:                                           /*  获取扇区大小                */
        *((ULONG *)lArg) = (1 << hDev->NVMEDEV_uiSectorShift);
        break;

    case LW_BLKD_GET_SECNUM:                                            /*  获取扇区数量                */
        *((ULONG *)lArg) = hDev->NVMEDEV_ulBlkCount;
        break;

    case LW_BLKD_CTRL_INFO:                                            /*  获取扇区数量                */
        hCtrl    = hDev->NVMEDEV_hCtrl;
        hBlkInfo = (PLW_BLK_INFO)lArg;
        if (!hBlkInfo) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }

        lib_bzero(hBlkInfo, sizeof(LW_BLK_INFO));
        hBlkInfo->BLKI_uiType = LW_BLKD_CTRL_INFO_TYPE_NVME;
        lib_memcpy(hBlkInfo->BLKI_cSerial, hCtrl->NVMECTRL_cSerial, 
                   __MIN(sizeof(hCtrl->NVMECTRL_cSerial), LW_BLKD_CTRL_INFO_STR_SZ));
        lib_memcpy(hBlkInfo->BLKI_cProduct, hCtrl->NVMECTRL_cModel, 
                   __MIN(sizeof(hCtrl->NVMECTRL_cModel), LW_BLKD_CTRL_INFO_STR_SZ));
        lib_memcpy(hBlkInfo->BLKI_cFirmware, hCtrl->NVMECTRL_cFirmWareRev, 
                   __MIN(sizeof(hCtrl->NVMECTRL_cFirmWareRev), LW_BLKD_CTRL_INFO_STR_SZ));
        break;

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nvmeBlkStatusChk
** 功能描述: 块设备检测
** 输　入  : hDev          设备句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nvmeBlkStatusChk (NVME_DEV_HANDLE  pDev)
{
    NVME_CTRL_HANDLE   hNvmeCtrl = pDev->NVMEDEV_hCtrl;

    if (hNvmeCtrl->NVMECTRL_bInstalled == LW_FALSE) {                   /*  控制器未安装                */
        NVME_LOG(NVME_LOG_ERR, "alloc ctrl %s unit %d failed.\r\n",
                hNvmeCtrl->NVMECTRL_cCtrlName, hNvmeCtrl->NVMECTRL_uiUnitIndex);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nvmeBlkReset
** 功能描述: 块设备复位
** 输　入  : hDev  设备句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nvmeBlkReset (NVME_DEV_HANDLE  pDev)
{
    NVME_CTRL_HANDLE   hNvmeCtrl = pDev->NVMEDEV_hCtrl;

    if (hNvmeCtrl->NVMECTRL_bInstalled == LW_FALSE) {                   /*  控制器未安装                */
        NVME_LOG(NVME_LOG_ERR, "alloc ctrl %s unit %d failed.\r\n",
                hNvmeCtrl->NVMECTRL_cCtrlName, hNvmeCtrl->NVMECTRL_uiUnitIndex);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nvmeNamespacesAlloc
** 功能描述: 申请命名空间
** 输　入  : hCtrl         控制器句柄
**           uiNsid        命名空间 ID
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __nvmeNamespacesAlloc (NVME_CTRL_HANDLE      hCtrl,
                                    UINT                  uiNsid)
{
    INT                 iRet;
    ULONG               ulPl          = NVME_CACHE_PL;                  /* 并发操作线程数量             */
    ULONG               ulCacheSize   = NVME_CACHE_SIZE;
    ULONG               ulBurstSizeRd = NVME_CACHE_BURST_RD;            /* 猝发读大小                   */
    ULONG               ulBurstSizeWr = NVME_CACHE_BURST_WR;            /* 猝发写大小                   */
    ULONG               ulDcMsgCount  = 0;
    NVME_DEV_HANDLE     hDev          = LW_NULL;
    PLW_BLK_DEV         hBlkDev       = LW_NULL;
    LW_DISKCACHE_ATTR   dcattrl;
    NVME_ID_NS_HANDLE   hIdNs;
    NVME_DRV_HANDLE     hDrv;

    hDev = (NVME_DEV_HANDLE)__SHEAP_ZALLOC(sizeof(NVME_DEV_CB));
    if (!hDev) {
        NVME_LOG(NVME_LOG_ERR, "alloc ctrl %s unit %d dev %d failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex, uiNsid);
        return;
    }
    
#if NVME_TRIM_EN > 0
    hDev->NVMEDEV_hDsmRange = (NVME_DSM_RANGE_HANDLE)API_VmmDmaAlloc(sizeof(NVME_DSM_RANGE_CB));
    if (!hDev->NVMEDEV_hDsmRange) {
        __SHEAP_FREE(hDev);
        NVME_LOG(NVME_LOG_ERR, "alloc ctrl %s unit %d dev %d failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex, uiNsid);
        return;
    }
#endif                                                                  /* NVME_TRIM_EN > 0             */

    hIdNs = (NVME_ID_NS_HANDLE)API_VmmDmaAlloc(sizeof(NVME_ID_NS_CB));
    if (!hIdNs) {
#if NVME_TRIM_EN > 0
        API_VmmDmaFree(hDev->NVMEDEV_hDsmRange);
#endif                                                                  /* NVME_TRIM_EN > 0             */
        __SHEAP_FREE(hDev);
        NVME_LOG(NVME_LOG_ERR, "alloc ctrl %s unit %d dev %d failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex, uiNsid);
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return;
    }
    
    iRet = __nvmeIdentify(hCtrl, uiNsid, 0, (dma_addr_t)hIdNs);
    if (iRet) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d dev %d identify namespace failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex, uiNsid);
        goto    __free;
    }

    hDrv = hCtrl->NVMECTRL_hDrv;                                        /* 获得驱动句柄                 */

    hDev->NVMEDEV_hCtrl             = hCtrl;
    hDev->NVMEDEV_uiCtrl            = hCtrl->NVMECTRL_uiIndex;
    hDev->NVMEDEV_uiNameSpaceId     = uiNsid;                           /*  块设备namespace id          */
    hDev->NVMEDEV_uiSectorShift     = hIdNs->NVMEIDNS_tLbaf[hIdNs->NVMEIDNS_ucFlbas & 0xf].NVMELBAF_Ds;
    hDev->NVMEDEV_ulBlkCount        = (ULONG)le64_to_cpu(hIdNs->NVMEIDNS_ullNcap);
    hDev->NVMEDEV_uiMaxHwSectors    = hCtrl->NVMECTRL_uiMaxHwSize >> hDev->NVMEDEV_uiSectorShift;
    hBlkDev                         = &hDev->NVMEDEV_tBlkDev;

    /*
     *  配置块设备参数
     */
    hBlkDev->BLKD_pcName            = NVME_NAME;                        /*  块设备名                    */
    hBlkDev->BLKD_ulNSector         = hDev->NVMEDEV_ulBlkCount;
    hBlkDev->BLKD_ulBytesPerSector  = 1 << hDev->NVMEDEV_uiSectorShift;
    hBlkDev->BLKD_ulBytesPerBlock   = 1 << hDev->NVMEDEV_uiSectorShift;
    hBlkDev->BLKD_bRemovable        = LW_TRUE;
    hBlkDev->BLKD_iRetry            = 1;
    hBlkDev->BLKD_iFlag             = O_RDWR;
    hBlkDev->BLKD_bDiskChange       = LW_FALSE;
    hBlkDev->BLKD_pfuncBlkRd        = __nvmeBlkRd;                      /*  块设备读操作                */
    hBlkDev->BLKD_pfuncBlkWrt       = __nvmeBlkWrt;                     /*  块设备写操作                */
    hBlkDev->BLKD_pfuncBlkIoctl     = __nvmeBlkIoctl;                   /*  块设备IO控制                */
    hBlkDev->BLKD_pfuncBlkReset     = __nvmeBlkReset;                   /*  块设备复位                  */
    hBlkDev->BLKD_pfuncBlkStatusChk = __nvmeBlkStatusChk;               /*  块设备状态检查              */

    hBlkDev->BLKD_iLogic            = 0;                                /*  物理设备                    */
    hBlkDev->BLKD_uiLinkCounter     = 0;
    hBlkDev->BLKD_pvLink            = LW_NULL;
    hBlkDev->BLKD_uiPowerCounter    = 0;
    hBlkDev->BLKD_uiInitCounter     = 0;

    /*
     *  获取驱动器块设备参数信息
     */
    iRet = hDrv->NVMEDRV_pfuncOptCtrl(hCtrl, uiNsid, NVME_OPT_CMD_CACHE_PL_GET,
                                      (LONG)((ULONG *)&ulPl));
    if (iRet != ERROR_NONE) {
        ulPl = NVME_CACHE_PL;
    }
    iRet = hDrv->NVMEDRV_pfuncOptCtrl(hCtrl, uiNsid, NVME_OPT_CMD_CACHE_SIZE_GET,
                                      (LONG)((ULONG *)&ulCacheSize));
    if (iRet != ERROR_NONE) {
        ulCacheSize = NVME_CACHE_SIZE;
    }
    iRet = hDrv->NVMEDRV_pfuncOptCtrl(hCtrl, uiNsid, NVME_OPT_CMD_CACHE_BURST_RD_GET,
                                      (LONG)((ULONG *)&ulBurstSizeRd));
    if (iRet != ERROR_NONE) {
        ulBurstSizeRd = NVME_CACHE_BURST_RD;
    }
    iRet = hDrv->NVMEDRV_pfuncOptCtrl(hCtrl, uiNsid, NVME_OPT_CMD_CACHE_BURST_WR_GET,
                                      (LONG)((ULONG *)&ulBurstSizeWr));
    if (iRet != ERROR_NONE) {
        ulBurstSizeWr = NVME_CACHE_BURST_WR;
    }
    iRet = hDrv->NVMEDRV_pfuncOptCtrl(hCtrl, uiNsid, NVME_OPT_CMD_DC_MSG_COUNT_GET,
                                      (LONG)((ULONG *)&ulDcMsgCount));
    if (iRet != ERROR_NONE) {
        ulDcMsgCount = NVME_DRIVE_DISKCACHE_MSG_COUNT;
    }

    dcattrl.DCATTR_pvCacheMem       = LW_NULL;
    dcattrl.DCATTR_stMemSize        = (size_t)ulCacheSize;              /* 配置为0，不使用cache         */
    dcattrl.DCATTR_bCacheCoherence  = LW_TRUE;                          /* 保证 CACHE 一致性            */
    dcattrl.DCATTR_iMaxRBurstSector = (INT)__MIN(ulBurstSizeRd, hDev->NVMEDEV_uiMaxHwSectors);
    dcattrl.DCATTR_iMaxWBurstSector = (INT)__MIN(ulBurstSizeWr, hDev->NVMEDEV_uiMaxHwSectors);
    dcattrl.DCATTR_iMsgCount        = (INT)ulDcMsgCount;                /* 管线消息队列缓存个数         */
    dcattrl.DCATTR_bParallel        = LW_TRUE;                          /* 可支持并行操作               */
    dcattrl.DCATTR_iPipeline        = (INT)((ulPl > LW_NCPUS) ? LW_NCPUS : ulPl);

    if (!hDev->NVMEDEV_pvOemdisk) {                                     /* 挂载设备                     */
        hDev->NVMEDEV_pvOemdisk = (PVOID)oemDiskMount2(NVME_MEDIA_NAME,
                                                       hBlkDev,
                                                       &dcattrl);
    }

    if (!hDev->NVMEDEV_pvOemdisk) {                                     /* 挂载失败                     */
        NVME_LOG(NVME_LOG_ERR, "oem disk mount failed ctrl %d namespace id %d.\r\n",
                 hCtrl->NVMECTRL_uiIndex, uiNsid);
        goto    __free;
    }

    API_NvmeDevAdd(hDev);                                               /* 块设备管理操作               */
    API_VmmDmaFree(hIdNs);
    return;

__free:
    API_VmmDmaFree(hIdNs);
#if NVME_TRIM_EN > 0
    API_VmmDmaFree(hDev->NVMEDEV_hDsmRange);
#endif                                                                  /* NVME_TRIM_EN > 0             */
    __SHEAP_FREE(hDev);
}
/*********************************************************************************************************
** 函数名称: __nvmeNamespacesScan
** 功能描述: 扫描命名空间
** 输　入  : hCtrl             控制器句柄
**           uiNameSpaceNum    命名空间数量
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __nvmeNamespacesScan (NVME_CTRL_HANDLE  hCtrl, UINT  uiNameSpaceNum)
{
    INT  i;

    for (i = 1; i <= uiNameSpaceNum; i++) {
        __nvmeNamespacesAlloc(hCtrl, i);
    }
}
/*********************************************************************************************************
** 函数名称: __nvmeQueueScan
** 功能描述: 扫描队列
** 输　入  : hCtrl         控制器句柄
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nvmeQueueScan (NVME_CTRL_HANDLE  hCtrl)
{
    NVME_ID_CTRL_HANDLE    hIdCtrl;
    UINT32                *puiNsList;
    UINT64                 ullCap;
    INT                    iRet;
    INT                    iPageShift;
    dma_addr_t             ulDmaAddr;
    UINT                   uiNameSpaceNum;
    UINT                   uiListsNum;
    UINT                   uiNsId;
    UINT                   i;
    UINT                   j;

    hCtrl->NVMECTRL_uiVersion = NVME_CTRL_READ(hCtrl, NVME_REG_VS);     /*  获取控制器版本              */
    ullCap                    = NVME_CTRL_READQ(hCtrl, NVME_REG_CAP);   /*  获取控制器Capability        */
    iPageShift                = NVME_CAP_MPSMIN(ullCap) + 12;           /*  获取控制器最小页大小偏移    */

    ulDmaAddr = (dma_addr_t)API_VmmDmaAlloc(sizeof(NVME_ID_CTRL_CB));
    if (!ulDmaAddr) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    iRet = __nvmeIdentify(hCtrl, 0, 1, ulDmaAddr);                      /*  获取控制器数据结构体        */
    if (iRet != ERROR_NONE) {
        API_VmmDmaFree((PVOID)ulDmaAddr);
        NVME_LOG(NVME_LOG_ERR, "identify controller failed (%d).\r\n", iRet);
        return  (PX_ERROR);
    }

    hIdCtrl = (NVME_ID_CTRL_HANDLE)ulDmaAddr;

    /*
     *  解析控制器命名空间参数
     */
    uiNameSpaceNum         = le32_to_cpu(hIdCtrl->NVMEIDCTRL_uiNn);     /*  命名空间个数                */
    hCtrl->NVMECTRL_usOncs = le16_to_cpu(hIdCtrl->NVMEIDCTRL_usOncs);   /*  可选的 NVMe 命令支持        */
    hCtrl->NVMECTRL_ucVwc  = hIdCtrl->NVMEIDCTRL_ucVwc;                 /* Volatile Write Cache         */
    lib_memcpy(hCtrl->NVMECTRL_cSerial, hIdCtrl->NVMEIDCTRL_ucSn, sizeof(hCtrl->NVMECTRL_cSerial));
    lib_memcpy(hCtrl->NVMECTRL_cModel, hIdCtrl->NVMEIDCTRL_ucMn, sizeof(hCtrl->NVMECTRL_cModel));
    lib_memcpy(hCtrl->NVMECTRL_cFirmWareRev, hIdCtrl->NVMEIDCTRL_ucFr, sizeof(hCtrl->NVMECTRL_cFirmWareRev));

    /*
     *  根据最大传输数据获得每次能够传输的最大扇区数量，默认一个扇区 512 字节
     */
    if (hIdCtrl->NVMEIDCTRL_ucMdts) {
        hCtrl->NVMECTRL_uiMaxHwSize = 1 << (hIdCtrl->NVMEIDCTRL_ucMdts + iPageShift);
    } else {
        hCtrl->NVMECTRL_uiMaxHwSize = UINT_MAX;
    }

    if ((hCtrl->NVMECTRL_ulQuirks & NVME_QUIRK_STRIPE_SIZE) && hIdCtrl->NVMEIDCTRL_ucVs[3]) {
        UINT   uiMaxHwSize;

        hCtrl->NVMECTRL_uiStripeSize = 1 << (hIdCtrl->NVMEIDCTRL_ucVs[3] + iPageShift);
        uiMaxHwSize = hCtrl->NVMECTRL_uiStripeSize >> (iPageShift);
        if (hCtrl->NVMECTRL_uiMaxHwSize) {
            hCtrl->NVMECTRL_uiMaxHwSize = __MIN(uiMaxHwSize, hCtrl->NVMECTRL_uiMaxHwSize);
        } else {
            hCtrl->NVMECTRL_uiMaxHwSize = uiMaxHwSize;
        }
    }

    /*
     *  扫描控制器命名空间
     */
    if (hCtrl->NVMECTRL_uiVersion >= NVME_VS(1, 1) &&
        !(hCtrl->NVMECTRL_ulQuirks & NVME_QUIRK_IDENTIFY_CNS)) {
        uiListsNum = DIV_ROUND_UP(uiNameSpaceNum, 1024);                /* 每次list放回1024个NSID       */
        puiNsList  = (UINT32 *)ulDmaAddr;

        for (i = 0; i < uiListsNum; i++) {
            iRet = __nvmeIdentify(hCtrl, i, 2, ulDmaAddr);
            if (iRet) {
                goto    __scan_ns;
            }

            for (j = 0; j < __MIN(uiNameSpaceNum, 1024U); j++) {        /* 每次list放回1024个NSID       */
                uiNsId = le32_to_cpu(puiNsList[j]);
                if (!uiNsId) {
                    goto    __done;
                }
                __nvmeNamespacesAlloc(hCtrl, uiNsId);                   /* 为命名空间创建设备           */
            }
            uiNameSpaceNum -= j;
        }

        if (!iRet) {
            goto    __done;
        }
    }

__scan_ns:
    __nvmeNamespacesScan(hCtrl, uiNameSpaceNum);

__done:
    API_VmmDmaFree((PVOID)ulDmaAddr);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nvmeAdminQueueConfigure
** 功能描述: 获取 NVMe 队列需要的额外空间
** 输　入  : iDepth    队列深度
** 输　出  : 额外空间大小
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT  __nvmeQueueExtra (INT  iDepth)
{
    return  (DIV_ROUND_UP(iDepth, 8) + (iDepth * sizeof(struct nvme_cmd_info_cb)));
}
/*********************************************************************************************************
** 函数名称: __nvmeCqValid
** 功能描述: 处理命令完成队列是否有效
** 输　入  : hQueue          命令队列
**           usHead          完成队列头
**           usPhase         完成队列相位
** 输　出  : 是否有效
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_INLINE BOOL  __nvmeCqValid (NVME_QUEUE_HANDLE  hQueue, UINT16  usHead, UINT16  usPhase)
{
    return  ((le16_to_cpu(hQueue->NVMEQUEUE_hCompletionQueue[usHead].NVMECOMPLETION_usStatus) 
              & 1) == usPhase);
}
/*********************************************************************************************************
** 函数名称: __nvmeCqProcess
** 功能描述: 处理命令完成队列
** 输　入  : hQueue          命令队列
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __nvmeCqProcess (NVME_QUEUE_HANDLE  hQueue)
{
    UINT16       usHead;
    UINT16       usPhase;

    usHead  = hQueue->NVMEQUEUE_usCqHead;                               /*  完成队列头                  */
    usPhase = hQueue->NVMEQUEUE_ucCqPhase;                              /*  完成队列相位                */

    while (__nvmeCqValid(hQueue, usHead, usPhase)) {
        PVOID                pvCtx;
        nvme_completion_fn   pfCompletion;
        NVME_COMPLETION_CB   tCqe = hQueue->NVMEQUEUE_hCompletionQueue[usHead];

        if (++usHead == hQueue->NVMEQUEUE_usDepth) {                    /*  本轮循环结束，相位取反      */
            usHead  = 0;
            usPhase = !usPhase;
        }

        pvCtx = __cmdIdFree(hQueue, tCqe.NVMECOMPLETION_usCmdId, &pfCompletion);
        pfCompletion(hQueue, pvCtx, &tCqe);
    }

    if ((usHead  == hQueue->NVMEQUEUE_usCqHead) && 
        (usPhase == hQueue->NVMEQUEUE_ucCqPhase)) {
        return;
    }
                                                                        /*  写DB通知控制器，清除中断    */
    write32(htole32(usHead), (addr_t)(hQueue->NVMEQUEUE_puiDoorBell + 
                                      hQueue->NVMEQUEUE_hCtrl->NVMECTRL_uiDBStride));

    hQueue->NVMEQUEUE_usCqHead  = usHead;
    hQueue->NVMEQUEUE_ucCqPhase = usPhase;
    hQueue->NVMEQUEUE_ucCqSeen  = 1;
}
/*********************************************************************************************************
** 函数名称: __nvmeIrq
** 功能描述: NVMe 中断服务
** 输　入  : hQueue          命令队列
**           ulVector        中断向量号
** 输　出  : 中断返回值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static irqreturn_t  __nvmeIrq (PVOID  pvArg, ULONG  ulVector)
{
    irqreturn_t         irqResult;
    INTREG              iregInterLevel;
    NVME_QUEUE_HANDLE   hQueue = (NVME_QUEUE_HANDLE)pvArg;

    LW_SPIN_LOCK_QUICK(&hQueue->NVMEQUEUE_QueueLock, &iregInterLevel);
    __nvmeCqProcess(hQueue);
    irqResult = hQueue->NVMEQUEUE_ucCqSeen ? LW_IRQ_HANDLED : LW_IRQ_NONE;
    hQueue->NVMEQUEUE_ucCqSeen = 0;
    LW_SPIN_UNLOCK_QUICK(&hQueue->NVMEQUEUE_QueueLock, iregInterLevel);

    return  (irqResult);
}
/*********************************************************************************************************
** 函数名称: __nvmeQueuesFree
** 功能描述: 释放队列
** 输　入  : hCtrl    控制器句柄
**           iLowest  释放的最小队列 ID
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __nvmeQueueFree (NVME_QUEUE_HANDLE   hNvmeQueue)
{
    INT   i = 0;

    if (hNvmeQueue->NVMEQUEUE_hSubmissionQueue) {                       /* 释放 SQ 队列资源             */
        API_CacheDmaFree(hNvmeQueue->NVMEQUEUE_hSubmissionQueue);
    }

    if (hNvmeQueue->NVMEQUEUE_hCompletionQueue) {                       /* 释放 CQ 队列资源             */
        API_CacheDmaFree(hNvmeQueue->NVMEQUEUE_hCompletionQueue);
    }

    for (i = 0; i < hNvmeQueue->NVMEQUEUE_usDepth; i++) {
        API_SemaphoreBDelete(&hNvmeQueue->NVMEQUEUE_hSyncBSem[i]);      /* 释放同步命令信号量           */
    }

    API_SemaphoreMDelete(&hNvmeQueue->NVMEQUEUE_hTagMuteSem);           /* 删除申请 cmdid 互斥信号量    */

    API_VmmDmaFree(hNvmeQueue->NVMEQUEUE_pvPrpBuf);                     /* 释放队列 PRP 资源            */

    __SHEAP_FREE(hNvmeQueue);                                           /* 释放队列控制块资源           */
}
/*********************************************************************************************************
** 函数名称: __nvmeQueuesFree
** 功能描述: 释放队列资源
** 输　入  : hCtrl         控制器句柄
**           iLowest       释放的最小队列 ID
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __nvmeQueuesFree (NVME_CTRL_HANDLE  hCtrl, INT  iLowest)
{
    INT                 i;
    NVME_QUEUE_HANDLE   hNvmeQueue;

    for (i = hCtrl->NVMECTRL_uiQueueCount - 1; i >= iLowest; i--) {     /* 释放当前所有的队列资源       */
        hNvmeQueue = hCtrl->NVMECTRL_hQueues[i];
        hCtrl->NVMECTRL_uiQueueCount--;
        hCtrl->NVMECTRL_hQueues[i] = LW_NULL;
        __nvmeQueueFree(hNvmeQueue);
    }
}
/*********************************************************************************************************
** 函数名称: __nvmeQueueAlloc
** 功能描述: 申请队列
** 输　入  : hCtrl            控制器句柄
**           iQueueId         队列 ID
**           iQCmdDepth       队列命令深度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static NVME_QUEUE_HANDLE __nvmeQueueAlloc (NVME_CTRL_HANDLE hCtrl, INT  iQueueId,  INT  iQCmdDepth)
{
    REGISTER UINT      i;
    INT                iRet;
    UINT               uiExtra = __nvmeQueueExtra(iQCmdDepth);
    NVME_QUEUE_HANDLE  hNvmeQueue;

    hNvmeQueue = (NVME_QUEUE_HANDLE)__SHEAP_ZALLOC(sizeof(NVME_QUEUE_CB) + uiExtra);
    if (!hNvmeQueue) {
        NVME_LOG(NVME_LOG_ERR, "alloc ctrl %s unit %d queue failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    
    iRet = __nvmeCqCmdsAlloc(hCtrl, hNvmeQueue, iQueueId, iQCmdDepth);  /* NVME_COMPLETION_CB pool      */
    if (iRet != ERROR_NONE) {
        goto   __free_nvmeq;
    }
    
    iRet = __nvmeSqCmdsAlloc(hCtrl, hNvmeQueue, iQueueId, iQCmdDepth);  /* NVME_COMMAND_CB pool         */
    if (iRet != ERROR_NONE) {
        goto   __free_cqdma;
    }

    for (i = 0; i < iQCmdDepth; i++) {                                  /* 创建发送命令同步信号量       */
        hNvmeQueue->NVMEQUEUE_hSyncBSem[i] = API_SemaphoreBCreate("nvme_sync",
                                                                  LW_FALSE,
                                                                  (LW_OPTION_WAIT_FIFO |
                                                                   LW_OPTION_OBJECT_GLOBAL),
                                                                  LW_NULL);
    }

    hNvmeQueue->NVMEQUEUE_hTagMuteSem = API_SemaphoreMCreate("nvme_tag",
                                                             LW_PRIO_DEF_CEILING,
                                                             (LW_OPTION_WAIT_PRIORITY    |
                                                              LW_OPTION_DELETE_SAFE      |
                                                              LW_OPTION_INHERIT_PRIORITY |
                                                              LW_OPTION_OBJECT_GLOBAL),
                                                             LW_NULL);  /* 创建 cmdid 申请互斥信号量    */

    LW_SPIN_INIT(&hNvmeQueue->NVMEQUEUE_QueueLock);
    snprintf(hNvmeQueue->NVMEQUEUE_cIrqName, NVME_CTRL_IRQ_NAME_MAX, "nvme%d_q%d",
             hCtrl->NVMECTRL_uiIndex, iQueueId);
    hNvmeQueue->NVMEQUEUE_uiNextTag   = 0;
    hNvmeQueue->NVMEQUEUE_hCtrl       = hCtrl;
    hNvmeQueue->NVMEQUEUE_pvPrpBuf    = API_VmmDmaAllocAlign((size_t)(NVME_PRP_BLOCK_SIZE * iQCmdDepth), 
                                                             NVME_PRP_BLOCK_SIZE);
    _BugHandle(!hNvmeQueue->NVMEQUEUE_pvPrpBuf, LW_TRUE, "NVMe queue prp buffer allocate fail.\r\n");

    hNvmeQueue->NVMEQUEUE_usCqHead    = 0;
    hNvmeQueue->NVMEQUEUE_ucCqPhase   = 1;
    hNvmeQueue->NVMEQUEUE_puiDoorBell = &hCtrl->NVMECTRL_puiDoorBells[iQueueId * 2 * hCtrl->NVMECTRL_uiDBStride];
    hNvmeQueue->NVMEQUEUE_usDepth     = (UINT16)iQCmdDepth;
    hNvmeQueue->NVMEQUEUE_usQueueId   = (UINT16)iQueueId;
    hNvmeQueue->NVMEQUEUE_usCqVector  = __ARCH_USHRT_MAX;
    hCtrl->NVMECTRL_hQueues[iQueueId] = hNvmeQueue;
    KN_SMP_WMB();

    hCtrl->NVMECTRL_uiQueueCount++;

    return  (hNvmeQueue);

__free_cqdma:
    API_CacheDmaFree((PVOID)hNvmeQueue->NVMEQUEUE_hCompletionQueue);

__free_nvmeq:
    __SHEAP_FREE(hNvmeQueue);

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __nvmeQueueInit
** 功能描述: 初始化 NVMe 队列
** 输　入  : hNvmeQueue    命令队列
**           usQueueId     队列 ID
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __nvmeQueueInit (NVME_QUEUE_HANDLE  hNvmeQueue, UINT16  usQueueId)
{
    NVME_CTRL_HANDLE  hCtrl    = hNvmeQueue->NVMEQUEUE_hCtrl;
    UINT              uiExtra  = __nvmeQueueExtra(hNvmeQueue->NVMEQUEUE_usDepth);
    INTREG            iregInterLevel;

    LW_SPIN_LOCK_IRQ(&hNvmeQueue->NVMEQUEUE_QueueLock, &iregInterLevel);
    hNvmeQueue->NVMEQUEUE_usSqTail    = 0;
    hNvmeQueue->NVMEQUEUE_usCqHead    = 0;
    hNvmeQueue->NVMEQUEUE_ucCqPhase   = 1;
    hNvmeQueue->NVMEQUEUE_puiDoorBell = &hCtrl->NVMECTRL_puiDoorBells[usQueueId * 2 * hCtrl->NVMECTRL_uiDBStride];
    lib_bzero(hNvmeQueue->NVMEQUEUE_ulCmdIdData, uiExtra);
    lib_bzero((PVOID)hNvmeQueue->NVMEQUEUE_hCompletionQueue, NVME_CQ_SIZE(hNvmeQueue->NVMEQUEUE_usDepth));
    hCtrl->NVMECTRL_uiQueuesOnline++;
    LW_SPIN_UNLOCK_IRQ(&hNvmeQueue->NVMEQUEUE_QueueLock, iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: __nvmeAdminQueueConfigure
** 功能描述: 配置 NVMe Admin 队列
** 输　入  : hCtrl    控制器句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nvmeAdminQueueConfigure (NVME_CTRL_HANDLE  hCtrl)
{
    INT                     iRet;
    UINT32                  uiAga;
    UINT64                  ullCap;
    NVME_QUEUE_HANDLE       hQueue;

    ullCap = NVME_CTRL_READQ(hCtrl, NVME_REG_CAP);
    hCtrl->NVMECTRL_bSubSystem = NVME_CTRL_READ(hCtrl, NVME_REG_VS) >= NVME_VS(1, 1) ?
                                 NVME_CAP_NSSRC(ullCap)                              :
                                 0;

    if (hCtrl->NVMECTRL_bSubSystem && (NVME_CTRL_READ(hCtrl, NVME_REG_CSTS) & NVME_CSTS_NSSRO)) {
        NVME_CTRL_WRITE(hCtrl, NVME_REG_CSTS, NVME_CSTS_NSSRO);
    }

    /*
     *  配置 admin queue 前，需先禁能控制器
     */
    iRet = __nvmeCtrlDisable(hCtrl, ullCap);
    if (iRet != ERROR_NONE) {
         NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d disable failed.\r\n",
                  hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
         return  (PX_ERROR);
    }

    /*
     *  分配 admin queue
     */
    hQueue = hCtrl->NVMECTRL_hQueues[0];
    if (!hQueue) {
        hQueue = __nvmeQueueAlloc(hCtrl, 0, (INT)hCtrl->NVMECTRL_uiQCmdDepth);
        if (!hQueue) {
            NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d alooc queue failed.\r\n",
                     hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
            return  (PX_ERROR);
        }
    }

    uiAga  = hQueue->NVMEQUEUE_usDepth - 1;
    uiAga |= uiAga << 16;

    NVME_CTRL_WRITE(hCtrl,  NVME_REG_AQA, uiAga);
    NVME_CTRL_WRITEQ(hCtrl, NVME_REG_ASQ, (dma_addr_t)hQueue->NVMEQUEUE_hSubmissionQueue);
    NVME_CTRL_WRITEQ(hCtrl, NVME_REG_ACQ, (dma_addr_t)hQueue->NVMEQUEUE_hCompletionQueue);

    /*
     *  配置 admin queue 完成后，使能控制器
     */
    iRet = __nvmeCtrlEnable(hCtrl, ullCap);
    if (iRet != ERROR_NONE) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d enable failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        goto    __free_nvmeq;
    }

    hQueue->NVMEQUEUE_usCqVector = 0;

    iRet = API_NvmeCtrlIntConnect(hCtrl, hQueue, __nvmeIrq, hQueue->NVMEQUEUE_cIrqName);
    if (iRet != ERROR_NONE) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d request irq failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        hQueue->NVMEQUEUE_usCqVector = __ARCH_USHRT_MAX;
        goto    __free_nvmeq;
    }
    
    __nvmeQueueInit(hQueue, 0);                                         /*  Admin队列参数初始化         */

    return  (iRet);

__free_nvmeq:
    __nvmeQueuesFree(hCtrl, 0);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __nvmeIoQueueCountSet
** 功能描述: 设置 IO 队列个数
** 输　入  : hCtrl         控制器句柄
**           piIoQCount    IO 队列个数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : NVME_FEAT_NUM_QUEUES 命令获取的队列数量不包括 Admin 队列, 同时计数值从 0 开始 (1 个).
*********************************************************************************************************/
static INT  __nvmeIoQueueCountSet (NVME_CTRL_HANDLE  hCtrl, INT  *piIoQCount)
{
    INT     iStatus;
    INT     iIoQueuesNum;
    UINT32  uiRet;
    UINT32  uiQueueCount = (*piIoQCount - 1) | ((*piIoQCount - 1) << 16);

    /*
     *  设置 I/O 队列个数
     */
    iStatus = __nvmeFeaturesSet(hCtrl, NVME_FEAT_NUM_QUEUES, uiQueueCount, 0,  &uiRet);
    if (iStatus) {
        return  (iStatus);
    }

    iIoQueuesNum = __MIN(uiRet & 0xffff, uiRet >> 16) + 1;
    *piIoQCount  = __MIN(*piIoQCount, iIoQueuesNum);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nvmeIoQueueCreate
** 功能描述: 创建 NVMe 命令队列
** 输　入  : hNvmeQueue    命令队列
**           usQueueId     队列 ID
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nvmeIoQueueCreate (NVME_QUEUE_HANDLE  hNvmeQueue, UINT16  usQueueId)
{
    INT               iRet;
    NVME_CTRL_HANDLE  hCtrl   = hNvmeQueue->NVMEQUEUE_hCtrl;
    PCI_DEV_HANDLE    hPciDev = (PCI_DEV_HANDLE)hCtrl->NVMECTRL_pvArg;

    if (hCtrl->NVMECTRL_bMsix || hPciDev->PCIDEV_iDevIrqMsiEn) {
        hNvmeQueue->NVMEQUEUE_usCqVector = usQueueId - 1;
    } else {
        hNvmeQueue->NVMEQUEUE_usCqVector = 0;
    }

    iRet = __adapterCqAlloc(hCtrl, usQueueId, hNvmeQueue);              /*  创建命令完成队列            */
    if (iRet < 0) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d creat sq failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        return  (iRet);
    }

    iRet = __adapterSqAlloc(hCtrl, usQueueId, hNvmeQueue);              /*  创建命令发送队列            */
    if (iRet < 0) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d creat cq failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        goto    __release_cq;
    }

    iRet =  API_NvmeCtrlIntConnect(hCtrl, hNvmeQueue, __nvmeIrq, hNvmeQueue->NVMEQUEUE_cIrqName);
    if (iRet < 0) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d irq request failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        goto    __release_sq;
    }

    __nvmeQueueInit(hNvmeQueue, usQueueId);                             /*  初始化队列参数              */

    return  (iRet);

__release_sq:
    __adapterSqDelete(hCtrl, usQueueId);                                /*  删除命令发送队列            */

__release_cq:
    __adapterCqDelete(hCtrl, usQueueId);                                /*  删除命令完成队列            */

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __nvmeIoQueuesConfigure
** 功能描述: 配置控制器 I/O 队列
** 输　入  : hCtrl    控制器句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nvmeIoQueuesConfigure (NVME_CTRL_HANDLE  hCtrl)
{
    INT               i;
    INT               iIoQueuesNum = LW_NCPUS;
    INT               iRet         = PX_ERROR;
    PCI_DEV_HANDLE    hPciDev      = (PCI_DEV_HANDLE)hCtrl->NVMECTRL_pvArg;

    iRet = __nvmeIoQueueCountSet(hCtrl, &iIoQueuesNum);                 /* 设置I/O队列数量              */
    if (iRet < 0) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d set queues count failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        goto    __free_queues;
    }

    if (hCtrl->NVMECTRL_bMsix || hPciDev->PCIDEV_iDevIrqMsiEn) {
        if (iIoQueuesNum > hCtrl->NVMECTRL_uiIntNum) {
            iIoQueuesNum = hCtrl->NVMECTRL_uiIntNum;                    /* 建立与中断个数匹配的I/O队列  */
        }
    }

    /*
     *  申请 I/O 队列资源
     */
    for (i = hCtrl->NVMECTRL_uiQueueCount; i <= iIoQueuesNum; i++) {
        if (!__nvmeQueueAlloc(hCtrl, i, (INT)hCtrl->NVMECTRL_uiQCmdDepth)) {
            iRet = PX_ERROR;
            break;
        }
    }

    /*
     *  创建 I/O 队列
     */
    for (i = hCtrl->NVMECTRL_uiQueuesOnline; i <= hCtrl->NVMECTRL_uiQueueCount - 1; i++) {
        iRet = __nvmeIoQueueCreate(hCtrl->NVMECTRL_hQueues[i], (UINT16)i);
        if (iRet) {
            __nvmeQueuesFree(hCtrl, i);
            break;
        }
    }

    return  (iRet >= 0 ? 0 : iRet);

__free_queues:
    __nvmeQueuesFree(hCtrl, 0);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __nvmeCtrlConfig
** 功能描述: 配置控制器
** 输　入  : hCtrl         控制器句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nvmeCtrlConfig (NVME_CTRL_HANDLE  hCtrl)
{
    UINT64   ullCap;
    UINT     uiCapMqes; 

    ullCap    = NVME_CTRL_READQ(hCtrl, NVME_REG_CAP);
    uiCapMqes = NVME_CAP_MQES((UINT)ullCap) + 1;
                                                                        /*  队列命令深度                */
    hCtrl->NVMECTRL_uiQCmdDepth  = (UINT)__MIN(uiCapMqes, NVME_CMD_DEPTH_MAX);
    hCtrl->NVMECTRL_uiDBStride   = (UINT)(1 << NVME_CAP_STRIDE(ullCap));
    hCtrl->NVMECTRL_puiDoorBells = (UINT32 *)((addr_t)hCtrl->NVMECTRL_pvRegAddr + NVME_REG_SQ0TDBL);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __nvmeMonitorProcess
** 功能描述: NVMe 监测线程回调函数
** 输　入  : hNvmeDev    NVMe 设备
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PVOID  __nvmeMonitorProcess (NVME_DEV_HANDLE  hNvmeDev)
{
    INT                   i;
    NVME_QUEUE_HANDLE     hNvmeQueue;
    INTREG                iregInterLevel;

    for (i = 0; i < hNvmeDev->NVMEDEV_hCtrl->NVMECTRL_uiQueueCount; i++) {
        hNvmeQueue = hNvmeDev->NVMEDEV_hCtrl->NVMECTRL_hQueues[i];
        if (!hNvmeQueue) {
            continue;
        }
        LW_SPIN_LOCK_QUICK(&hNvmeQueue->NVMEQUEUE_QueueLock, &iregInterLevel);
        __nvmeCqProcess(hNvmeQueue);
        LW_SPIN_UNLOCK_QUICK(&hNvmeQueue->NVMEQUEUE_QueueLock, iregInterLevel);
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __nvmeDevMonitor
** 功能描述: 创建设备检测线程
** 输　入  : hCtrl      控制器句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nvmeDevMonitor (NVME_CTRL_HANDLE hCtrl)
{
    static LW_OBJECT_HANDLE     ulMonThread = 0ul;
    LW_CLASS_THREADATTR         threadattr;

    if (!ulMonThread) {
        API_ThreadAttrBuild(&threadattr,
                            NVME_MONITOR_STK_SIZE,
                            NVME_MONITOR_PRIORITY,
                            LW_OPTION_THREAD_STK_CHK | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL,
                            (PVOID)__nvmeMonitorProcess);
        
        ulMonThread = API_ThreadCreate("t_nvme",
                                       (PTHREAD_START_ROUTINE)__nvmeMonitorThread,
                                       (PLW_CLASS_THREADATTR)&threadattr,
                                       LW_NULL);
        if (LW_OBJECT_HANDLE_INVALID == ulMonThread) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_NvmeCtrlInit
** 功能描述: 控制器初始化
** 输　入  : hCtrl    控制器句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __nvmeInit (NVME_CTRL_HANDLE  hCtrl)
{
    INT   iRet;

    NVME_LOG(NVME_LOG_PRT, "init ctrl %d name %s uint index %d reg addr 0x%llx.\r\n",
             hCtrl->NVMECTRL_uiIndex, hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex,
             hCtrl->NVMECTRL_pvRegAddr);

    iRet = __nvmeCtrlConfig(hCtrl);                                     /*  配置控制参数                */
    if (iRet != ERROR_NONE) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d driver enable failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        return  (PX_ERROR);
    }

    iRet = __nvmeAdminQueueConfigure(hCtrl);                            /*  配置Admin队列               */
    if (iRet != ERROR_NONE) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d admin queue configure failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        return  (PX_ERROR);
    }

    iRet = __nvmeIoQueuesConfigure(hCtrl);                              /*  配置IO队列                  */
    if (iRet != ERROR_NONE) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d setup io queues failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        return  (PX_ERROR);
    }

    iRet = __nvmeDevMonitor(hCtrl);                                     /*  初始化管理线程              */
    if (iRet != ERROR_NONE) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d add dev list failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        return  (PX_ERROR);
    }

    /*
     *  如果没有可用的 I/O 队列，则释放 Admin 队列
     */
    if (hCtrl->NVMECTRL_uiQueuesOnline < 2) {
        NVME_LOG(NVME_LOG_ERR, "IO queues not created, online %u\n", 
                 hCtrl->NVMECTRL_uiQueuesOnline);
        __nvmeQueuesFree(hCtrl, 0);
        return  (PX_ERROR);
    
    } else {
        __nvmeQueueScan(hCtrl);                                         /*  扫描 NVMe 设备              */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_NvmeCtrlFree
** 功能描述: 释放一个 NVMe 控制器
** 输　入  : hCtrl     控制器句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_NvmeCtrlFree (NVME_CTRL_HANDLE  hCtrl)
{
    if (hCtrl != LW_NULL) {
        API_NvmeCtrlDelete(hCtrl);                                      /* 删除控制器                   */
        if (hCtrl->NVMECTRL_hQueues != LW_NULL) {
            __SHEAP_FREE(hCtrl->NVMECTRL_hQueues);                      /* 释放命令队列                 */
        }
        __SHEAP_FREE(hCtrl);                                            /* 释放控制器                   */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_NvmeCtrlCreate
** 功能描述: 创建 NVMe 控制器
** 输　入  : pcName     控制器名称
**           uiUnit     本类控制器索引
**           pvArg      扩展参数
**           ulData     设备异常行为
** 输　出  : NVMe 控制器句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
NVME_CTRL_HANDLE  API_NvmeCtrlCreate (CPCHAR  pcName, UINT  uiUnit, PVOID  pvArg, ULONG  ulData)
{
    INT                 iRet  = PX_ERROR;                               /* 操作结果                     */
    NVME_CTRL_HANDLE    hCtrl = LW_NULL;                                /* 控制器句柄                   */
    NVME_DRV_HANDLE     hDrv  = LW_NULL;                                /* 驱动句柄                     */

    if (!pcName) {                                                      /* 控制器参数错误               */
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    hCtrl = (NVME_CTRL_HANDLE)__SHEAP_ZALLOC(sizeof(NVME_CTRL_CB));     /* 分配控制器控制块             */
    if (!hCtrl) {                                                       /* 分配控制块失败               */
        NVME_LOG(NVME_LOG_ERR, "alloc ctrl %s unit %d tcb failed.\r\n", pcName, uiUnit);
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }

    hDrv = API_NvmeDrvHandleGet(pcName);                                /* 通过名字获得驱动句柄         */
    if (!hDrv) {                                                        /* 驱动未注册                   */
        NVME_LOG(NVME_LOG_ERR, "nvme driver %s not register.\r\n", pcName);
        goto    __error_handle;
    }

    hDrv->NVMEDRV_hCtrl         = hCtrl;                                /* 更新驱动对应的控制器         */
    hCtrl->NVMECTRL_hDrv        = hDrv;                                 /* 控制器驱动                   */
    lib_strlcpy(&hCtrl->NVMECTRL_cCtrlName[0], &hDrv->NVMEDRV_cDrvName[0], NVME_CTRL_NAME_MAX);
    hCtrl->NVMECTRL_uiCoreVer   = NVME_CTRL_DRV_VER_NUM;                /* 控制器核心版本               */
    hCtrl->NVMECTRL_uiUnitIndex = uiUnit;                               /* 本类控制器索引               */
    hCtrl->NVMECTRL_uiIndex     = API_NvmeCtrlIndexGet();               /* 控制器索引                   */
    hCtrl->NVMECTRL_pvArg       = pvArg;                                /* 控制器参数                   */
    hCtrl->NVMECTRL_ulQuirks    = ulData;                               /* 控制器异常行为               */
    API_NvmeCtrlAdd(hCtrl);                                             /* 添加控制器                   */

    if (hDrv->NVMEDRV_pfuncVendorCtrlReadyWork) {
        iRet = hDrv->NVMEDRV_pfuncVendorCtrlReadyWork(hCtrl, (UINT)LW_NCPUS);
    } else {
        iRet = ERROR_NONE;
    }

    if (iRet != ERROR_NONE) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d vendor ready work failed.\r\n", pcName, uiUnit);
        goto  __error_handle;
    }

    if (!hCtrl->NVMECTRL_pvRegAddr) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d reg addr null.\r\n",
                 hCtrl->NVMECTRL_uiIndex, uiUnit);
        goto  __error_handle;
    }

    hCtrl->NVMECTRL_bInstalled   = LW_TRUE;
    hCtrl->NVMECTRL_ulSemTimeout = NVME_SEM_TIMEOUT_DEF;
                                                                        /* 分配命令队列                 */
    hCtrl->NVMECTRL_hQueues = (NVME_QUEUE_HANDLE *)__SHEAP_ZALLOC(sizeof(PVOID) * (LW_NCPUS + 1));
    if (!hCtrl->NVMECTRL_hQueues) {                                     /* 分配命令队列失败             */
        NVME_LOG(NVME_LOG_ERR, "alloc ctrl %s unit %d queue failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        goto    __error_handle;
    }

    iRet = __nvmeInit(hCtrl);                                           /* 驱动初始化                   */
    if (iRet != ERROR_NONE) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d driver reset failed.\r\n", pcName, uiUnit);
        goto    __error_handle;
    }

    return  (hCtrl);                                                    /* 返回控制器句柄               */

__error_handle:                                                         /* 错误处理                     */
    API_NvmeCtrlFree(hCtrl);                                            /* 释放控制器                   */
    
    return  (LW_NULL);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_NVME_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
