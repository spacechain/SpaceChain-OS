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
** 文   件   名: can.c
**
** 创   建   人: Wang.Feng (王锋)
**
** 文件创建日期: 2010 年 02 月 01 日
**
** 描        述: CAN 设备库.

** BUG
2010.02.01  初始版本
2010.03.11  加入 select 对 SELEXCEPT 的支持.
2010.05.13  修正了队列写和读时可能未开中断的bug, 增加了总线上的几种异常情况的处理，获取总线状态修改为类似
            读取芯片的状态寄存器，读取后会自动清除，去除了设置总线状态的命令。
2010.07.10  __canClose() 返回值应为 ERROR_NONE.
2010.07.29  驱动设置总线状态时, 如果总线正常则设置, 否则累加异常.
2010.09.11  创建设备时, 指定设备类型.
2010.10.28  can read() write() 第三个参数为字节数, 必须为 sizeof(CAN_FRAME) 的整数倍.
            FIONREAD 和 FIONWRITE 参数也为字节数.
2012.06.29  设置 st_dev st_ino.
2012.08.10  当总线出错时, 需要激活读写等待线程. 
            pend(..., LW_OPTION_NOT_WAIT) 改为 clear() 操作.
2012.10.31  修正一些不合理的命名.
2017.03.13  支持 CAN FD 标准与发送完成同步.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"                                /*  需要根文件系统时间          */
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_CAN_EN > 0)
/*********************************************************************************************************
  定义 CAN 缓冲队列结构体
*********************************************************************************************************/
typedef struct {
    UINT                CANQ_uiCounter;                                 /*  当前缓冲区数据帧个数        */
    UINT                CANQ_uiMaxFrame;                                /*  最大数据帧个数              */

    PCAN_FD_FRAME       CANQ_pcanframeIn;                               /*  入队指针                    */
    PCAN_FD_FRAME       CANQ_pcanframeOut;                              /*  出队指针                    */

    PCAN_FD_FRAME       CANQ_pcanframeEnd;                              /*  队尾                        */
    PCAN_FD_FRAME       CANQ_pcanframeBuffer;                           /*  队头,缓冲区的首地址         */
} __CAN_QUEUE;
typedef __CAN_QUEUE    *__PCAN_QUEUE;
/*********************************************************************************************************
  CAN_DEV_WR_STATE
*********************************************************************************************************/
typedef struct {
    BOOL                CANSTAT_bBufEmpty;                              /*  FIFO 空标志                 */
} __CAN_DEV_STATE;
/*********************************************************************************************************
  CAN 设备结构
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR          CAN_devhdr;                                     /*  设备头                      */
    UINT                CAN_uiChannel;                                  /*  can 通道号                  */

    __PCAN_QUEUE        CAN_pcanqRecvQueue;                             /*  接收队列                    */
    __PCAN_QUEUE        CAN_pcanqSendQueue;                             /*  发送队列                    */

    LW_HANDLE           CAN_ulRcvSemB;                                  /*  CAN 接收信号                */
    LW_HANDLE           CAN_ulSendSemB;                                 /*  CAN 发送信号                */
    LW_HANDLE           CAN_ulSyncSemB;                                 /*  CAN 发送完成                */
    LW_HANDLE           CAN_ulMutexSemM;                                /*  互斥信号                    */

    __CAN_DEV_STATE     CAN_canstatReadState;                           /*  读状态                      */
    __CAN_DEV_STATE     CAN_canstatWriteState;                          /*  写状态                      */
    UINT                CAN_uiBusState;                                 /*  总线状态                    */

    UINT                CAN_uiDevFrameType;                             /*  CAN or CAN FD               */
    UINT                CAN_uiFileFrameType;                            /*  CAN or CAN FD               */

    ULONG               CAN_ulSendTimeout;                              /*  发送超时时间                */
    ULONG               CAN_ulRecvTimeout;                              /*  接收超时时间                */

    LW_SEL_WAKEUPLIST   CAN_selwulList;                                 /*  select() 等待链             */

    LW_SPINLOCK_DEFINE (CAN_slLock);                                    /*  自旋锁                      */
} __CAN_DEV;

typedef struct {
    __CAN_DEV           CANPORT_can;                                    /*  can 设备                    */
    CAN_CHAN           *CANPORT_pcanchan;                               /*  can 设备通道                */
} __CAN_PORT;
typedef __CAN_PORT     *__PCAN_PORT;
/*********************************************************************************************************
  定义全局变量, 用于保存can驱动号
*********************************************************************************************************/
static INT _G_iCanDrvNum = PX_ERROR;
/*********************************************************************************************************
  CAN LOCK
*********************************************************************************************************/
#define CANPORT_LOCK(pcanport)  \
        API_SemaphoreMPend(pcanport->CANPORT_can.CAN_ulMutexSemM, LW_OPTION_WAIT_INFINITE)
#define CANPORT_UNLOCK(pcanport) \
        API_SemaphoreMPost(pcanport->CANPORT_can.CAN_ulMutexSemM)
        
#define CANDEV_LOCK(pcandev)   \
        API_SemaphoreMPend(pcandev->CAN_ulMutexSemM, LW_OPTION_WAIT_INFINITE)
#define CANDEV_UNLOCK(pcandev)  \
        API_SemaphoreMPost(pcandev->CAN_ulMutexSemM)
/*********************************************************************************************************
** 函数名称: __canInitQueue
** 功能描述: 缓冲初始化
** 输　入  : uiMaxFrame     缓冲区 can 帧个数
** 输　出  : 缓冲区指针
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static __PCAN_QUEUE  __canInitQueue (UINT   uiMaxFrame)
{
    size_t         stAllocSize;
    __CAN_QUEUE   *pcanq = LW_NULL;

    stAllocSize = (size_t)(sizeof(__CAN_QUEUE) + (uiMaxFrame * sizeof(CAN_FD_FRAME)));
    pcanq = (__CAN_QUEUE *)__SHEAP_ALLOC(stAllocSize);
    if (pcanq == LW_NULL) {
        return  (LW_NULL);
    }
    
    pcanq->CANQ_uiCounter       = 0;
    pcanq->CANQ_uiMaxFrame      = uiMaxFrame;
    pcanq->CANQ_pcanframeBuffer = (PCAN_FD_FRAME)(pcanq + 1);
    pcanq->CANQ_pcanframeIn     = pcanq->CANQ_pcanframeBuffer;
    pcanq->CANQ_pcanframeOut    = pcanq->CANQ_pcanframeBuffer;
    pcanq->CANQ_pcanframeEnd    = pcanq->CANQ_pcanframeBuffer + pcanq->CANQ_uiMaxFrame;

    return  (pcanq);
}
/*********************************************************************************************************
** 函数名称: __canCopyToQueue
** 功能描述: 向 QUEUE 中节点拷贝帧信息
** 输　入  : pcanfdframe       队列中的 CAN 帧
**           pvCanFrame        需要入队的 CAN 帧
**           uiFrameType       帧类型
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __canCopyToQueue (PCAN_FD_FRAME  pcanfdframe, PVOID  pvCanFrame, UINT  uiFrameType)
{
    PCAN_FRAME      pcanframeF;
    PCAN_FD_FRAME   pcanfdframeF;
    UINT8           ucLen;

    if (uiFrameType == CAN_STD_CAN) {
        pcanframeF = (PCAN_FRAME)pvCanFrame;
        ucLen      = (UINT8)((pcanframeF->CAN_ucLen > CAN_MAX_DATA) ? CAN_MAX_DATA : pcanframeF->CAN_ucLen);

        pcanfdframe->CAN_uiId         = pcanframeF->CAN_uiId;
        pcanfdframe->CAN_uiChannel    = pcanframeF->CAN_uiChannel;
        pcanfdframe->CAN_bExtId       = pcanframeF->CAN_bExtId;
        pcanfdframe->CAN_bRtr         = pcanframeF->CAN_bRtr;
        pcanfdframe->CAN_uiCanFdFlags = 0;
        pcanfdframe->CAN_ucLen        = ucLen;
        lib_memcpy(pcanfdframe->CAN_ucData, pcanframeF->CAN_ucData, ucLen);

    } else {
        pcanfdframeF = (PCAN_FD_FRAME)pvCanFrame;
        ucLen        = (UINT8)((pcanfdframeF->CAN_ucLen > CAN_FD_MAX_DATA) ? CAN_FD_MAX_DATA : pcanfdframeF->CAN_ucLen);

        pcanfdframe->CAN_uiId         = pcanfdframeF->CAN_uiId;
        pcanfdframe->CAN_uiChannel    = pcanfdframeF->CAN_uiChannel;
        pcanfdframe->CAN_bExtId       = pcanfdframeF->CAN_bExtId;
        pcanfdframe->CAN_bRtr         = pcanfdframeF->CAN_bRtr;
        pcanfdframe->CAN_uiCanFdFlags = pcanfdframeF->CAN_uiCanFdFlags;
        pcanfdframe->CAN_ucLen        = ucLen;
        lib_memcpy(pcanfdframe->CAN_ucData, pcanfdframeF->CAN_ucData, ucLen);
    }
}
/*********************************************************************************************************
** 函数名称: __canCopyFromQueue
** 功能描述: 从 QUEUE 中节点拷贝帧信息
** 输　入  : pcanfdframe       队列中的 CAN 帧
**           pvCanFrame        需要出队的 CAN 帧
**           uiFrameType       帧类型
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __canCopyFromQueue (PCAN_FD_FRAME  pcanfdframe, PVOID  pvCanFrame, UINT  uiFrameType)
{
    PCAN_FRAME      pcanframeT;
    PCAN_FD_FRAME   pcanfdframeT;
    UINT8           ucLen;

    if (uiFrameType == CAN_STD_CAN) {
        ucLen       = (UINT8)((pcanfdframe->CAN_ucLen > CAN_MAX_DATA) ? CAN_MAX_DATA : pcanfdframe->CAN_ucLen);
        pcanframeT  = (PCAN_FRAME)pvCanFrame;

        pcanframeT->CAN_uiId      = pcanfdframe->CAN_uiId;
        pcanframeT->CAN_uiChannel = pcanfdframe->CAN_uiChannel;
        pcanframeT->CAN_bExtId    = pcanfdframe->CAN_bExtId;
        pcanframeT->CAN_bRtr      = pcanfdframe->CAN_bRtr;
        pcanframeT->CAN_ucLen     = ucLen;
        lib_memcpy(pcanframeT->CAN_ucData, pcanfdframe->CAN_ucData, ucLen);

    } else {
        ucLen        = (UINT8)((pcanfdframe->CAN_ucLen > CAN_FD_MAX_DATA) ? CAN_FD_MAX_DATA : pcanfdframe->CAN_ucLen);
        pcanfdframeT = (PCAN_FD_FRAME)pvCanFrame;

        pcanfdframeT->CAN_uiId         = pcanfdframe->CAN_uiId;
        pcanfdframeT->CAN_uiChannel    = pcanfdframe->CAN_uiChannel;
        pcanfdframeT->CAN_bExtId       = pcanfdframe->CAN_bExtId;
        pcanfdframeT->CAN_bRtr         = pcanfdframe->CAN_bRtr;
        pcanfdframeT->CAN_uiCanFdFlags = pcanfdframe->CAN_uiCanFdFlags;
        pcanfdframeT->CAN_ucLen        = ucLen;
        lib_memcpy(pcanfdframeT->CAN_ucData, pcanfdframe->CAN_ucData, ucLen);
    }
}
/*********************************************************************************************************
** 函数名称: __canWriteQueue
** 功能描述: 向队列中写入数据
** 输　入  : pcanDev                  指向设备结构
**           pcanq                    队列指针
**           pvCanFrame               需要入队的 CAN 帧
**           uiFrameType              帧类型
**           iNumber                  要写入的个数
** 输　出  : 实际写入的个数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __canWriteQueue (__CAN_DEV     *pcanDev,
                             __PCAN_QUEUE   pcanq,
                             PVOID          pvCanFrame,
                             UINT           uiFrameType,
                             INT            iNumber)
{
    INT         i = 0;
    INTREG      iregInterLevel;

    while (iNumber) {
        LW_SPIN_LOCK_QUICK(&pcanDev->CAN_slLock, &iregInterLevel);
        if (pcanq->CANQ_uiCounter < pcanq->CANQ_uiMaxFrame) {           /*  更新接收队列                */
            pcanq->CANQ_uiCounter++;
            __canCopyToQueue(pcanq->CANQ_pcanframeIn, pvCanFrame, uiFrameType);
            pcanq->CANQ_pcanframeIn++;
            if (pcanq->CANQ_pcanframeIn >= pcanq->CANQ_pcanframeEnd) {
                pcanq->CANQ_pcanframeIn  = pcanq->CANQ_pcanframeBuffer;
            }
            iNumber--;
            if (uiFrameType == CAN_STD_CAN) {
                pvCanFrame = (PVOID)((addr_t)pvCanFrame + sizeof(CAN_FRAME));
            } else {
                pvCanFrame = (PVOID)((addr_t)pvCanFrame + sizeof(CAN_FD_FRAME));
            }
            i++;
        
        } else {
            LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
            break;
        }
        LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
    }

    return  (i);
}
/*********************************************************************************************************
** 函数名称: __canReadQueue
** 功能描述: 向队列中写入数据
** 输　入  : pcanDev                  指向设备结构
**           pcanq                    队列指针
**           pvCanFrame               需要出队的 CAN 帧
**           uiFrameType              帧类型
**           iNumber                  要读出的个数
** 输　出  : 实际读出的个数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __canReadQueue (__CAN_DEV    *pcanDev,
                           __PCAN_QUEUE  pcanq,
                           PVOID         pvCanFrame,
                           UINT          uiFrameType,
                           INT           iNumber)
{
    INT        i = 0;
    INTREG     iregInterLevel;

    while (iNumber) {
        LW_SPIN_LOCK_QUICK(&pcanDev->CAN_slLock, &iregInterLevel);
        if (pcanq->CANQ_uiCounter > 0) {
            pcanq->CANQ_uiCounter--;
            __canCopyFromQueue(pcanq->CANQ_pcanframeOut, pvCanFrame, uiFrameType);
            pcanq->CANQ_pcanframeOut++;
            if (pcanq->CANQ_pcanframeOut == pcanq->CANQ_pcanframeEnd) {
                pcanq->CANQ_pcanframeOut  = pcanq->CANQ_pcanframeBuffer;
            }
            iNumber--;
            if (uiFrameType == CAN_STD_CAN) {
                pvCanFrame = (PVOID)((addr_t)pvCanFrame + sizeof(CAN_FRAME));
            } else {
                pvCanFrame = (PVOID)((addr_t)pvCanFrame + sizeof(CAN_FD_FRAME));
            }
            i++;
        
        } else {
            LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
            break;
        }
        LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
    }

    return  (i);
}
/*********************************************************************************************************
** 函数名称: __canQFreeNum
** 功能描述: 获取队列中空闲数量
** 输　入  : pcanq                队列指针
** 输　出  : 空闲数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __canQFreeNum (__PCAN_QUEUE  pcanq)
{
    REGISTER INT   iNum;

    iNum = pcanq->CANQ_uiMaxFrame - pcanq->CANQ_uiCounter;

    return  (iNum);
}
/*********************************************************************************************************
** 函数名称: __canQCount
** 功能描述: 队列中消息的数量
** 输　入  : pcanq                队列指针
** 输　出  : 队列中有用数据的个数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __canQCount (__PCAN_QUEUE pcanq)
{
    return  (pcanq->CANQ_uiCounter);
}
/*********************************************************************************************************
** 函数名称: __canFlushQueue
** 功能描述: 清空队列
** 输　入  : pcanq                队列指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __canFlushQueue (__PCAN_QUEUE pcanq)
{
    pcanq->CANQ_uiCounter    = 0;
    pcanq->CANQ_pcanframeIn  = pcanq->CANQ_pcanframeBuffer;
    pcanq->CANQ_pcanframeOut = pcanq->CANQ_pcanframeBuffer;
}
/*********************************************************************************************************
** 函数名称: __canDeleteQueue
** 功能描述: 删除队列
** 输　入  : pcanq                队列指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __canDeleteQueue (__PCAN_QUEUE pcanq)
{
    __SHEAP_FREE((PVOID)pcanq);
}
/*********************************************************************************************************
** 函数名称: __canFlushRd
** 功能描述: 清除 CAN 设备读缓冲区
** 输　入  : pcanport           CAN 设备
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID   __canFlushRd (__CAN_PORT  *pcanport)
{
    INTREG  iregInterLevel;

    CANPORT_LOCK(pcanport);                                             /*  等待设备使用权              */

    LW_SPIN_LOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, &iregInterLevel);
    __canFlushQueue(pcanport->CANPORT_can.CAN_pcanqRecvQueue);          /*  清除缓冲区                  */
    LW_SPIN_UNLOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, iregInterLevel);

    API_SemaphoreBClear(pcanport->CANPORT_can.CAN_ulRcvSemB);           /*  清除读同步                  */

    CANPORT_UNLOCK(pcanport);                                           /*  释放设备使用权              */
}
/*********************************************************************************************************
** 函数名称: __canFlushWrt
** 功能描述: 清除 CAN 设备写缓冲区
** 输　入  : pcanport           CAN 设备
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID   __canFlushWrt (__CAN_PORT  *pcanport)
{
    INTREG  iregInterLevel;

    CANPORT_LOCK(pcanport);                                             /*  等待设备使用权              */

    LW_SPIN_LOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, &iregInterLevel);
    __canFlushQueue(pcanport->CANPORT_can.CAN_pcanqSendQueue);          /*  清除缓冲区                  */
    pcanport->CANPORT_can.CAN_canstatWriteState.CANSTAT_bBufEmpty = LW_TRUE;
                                                                        /*  发送队列空                  */
    LW_SPIN_UNLOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, iregInterLevel);

    API_SemaphoreBPost(pcanport->CANPORT_can.CAN_ulSendSemB);           /*  通知线程可写                */
    API_SemaphoreBPost(pcanport->CANPORT_can.CAN_ulSyncSemB);           /*  通知线程发送完毕            */

    CANPORT_UNLOCK(pcanport);                                           /*  释放设备使用权              */
    
    SEL_WAKE_UP_ALL(&pcanport->CANPORT_can.CAN_selwulList, SELWRITE);   /*  通知 select 线程可写        */
}
/*********************************************************************************************************
** 函数名称: __canITx
** 功能描述: 从发送缓冲区中读出一个数据
** 输　入  : pcanDev           CAN 设备
**           pvCanFrame        指向待读出的数据
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __canITx (__CAN_DEV  *pcanDev, PVOID  pvCanFrame)
{
    INTREG  iregInterLevel;
    INT     iTemp = 0;
    BOOL    bSync = LW_FALSE;

    if (!pcanDev || !pvCanFrame) {
        return  (PX_ERROR);
    }

    iTemp = __canReadQueue(pcanDev,
                           pcanDev->CAN_pcanqSendQueue,
                           pvCanFrame,
                           pcanDev->CAN_uiDevFrameType,
                           1);                                          /*  从发送队列中读取一帧数据    */

    LW_SPIN_LOCK_QUICK(&pcanDev->CAN_slLock, &iregInterLevel);
    if (iTemp <= 0) {
        pcanDev->CAN_canstatWriteState.CANSTAT_bBufEmpty = LW_TRUE;     /*  发送队列空                  */
        bSync = LW_TRUE;
    }
    LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);

    API_SemaphoreBPost(pcanDev->CAN_ulSendSemB);                        /*  释放信号量                  */
    if (bSync) {
        API_SemaphoreBPost(pcanDev->CAN_ulSyncSemB);
    }
    SEL_WAKE_UP_ALL(&pcanDev->CAN_selwulList, SELWRITE);                /*  释放所有等待写的线程        */

    return  ((iTemp) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** 函数名称: __canIRx
** 功能描述: 向接收缓冲区中写入一个数据
** 输　入  : pcanDev            CAN 设备
**           pvCanFrame         指向待写入的数据
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __canIRx (__CAN_DEV  *pcanDev, PVOID  pvCanFrame)
{
    INT     iTemp = 0;

    if (!pcanDev || !pvCanFrame) {
        return  (PX_ERROR);
    }

    iTemp = __canWriteQueue(pcanDev,
                            pcanDev->CAN_pcanqRecvQueue,
                            pvCanFrame,
                            pcanDev->CAN_uiDevFrameType,
                            1);                                         /*  往接收队列中写入一帧数据    */

    API_SemaphoreBPost(pcanDev->CAN_ulRcvSemB);                         /*  释放信号量                  */
    SEL_WAKE_UP_ALL(&pcanDev->CAN_selwulList, SELREAD);                 /*  select() 激活               */

    return  ((iTemp) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** 函数名称: __canTxStartup
** 功能描述: 启动发送函数
** 输　入  : pcanport           CAN 设备
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __canTxStartup (__CAN_PORT  *pcanport)
{
    INTREG    iregInterLevel;

    if (pcanport->CANPORT_can.CAN_canstatWriteState.CANSTAT_bBufEmpty == LW_TRUE) {
        LW_SPIN_LOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, &iregInterLevel);
                                                                        /*  关闭中断                    */
        if (pcanport->CANPORT_can.CAN_canstatWriteState.CANSTAT_bBufEmpty == LW_TRUE) {
            pcanport->CANPORT_can.CAN_canstatWriteState.CANSTAT_bBufEmpty = LW_FALSE;
            LW_SPIN_UNLOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, iregInterLevel);
                                                                        /*  打开中断                    */
                                                                        /*  启动发送                    */
            pcanport->CANPORT_pcanchan->pDrvFuncs->txStartup(pcanport->CANPORT_pcanchan);
            return;
        }                                                               /*  打开中断                    */
        LW_SPIN_UNLOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, iregInterLevel);
    }
}
/*********************************************************************************************************
** 函数名称: __canSetBusState
** 功能描述: 设置 CAN 设备的总线状态
** 输　入  : pcanDev            CAN 设备
**           iState             总线状态
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __canSetBusState (__CAN_DEV  *pcanDev, INT iState)
{
    INTREG       iregInterLevel;
    __CAN_PORT  *pcanport = (__CAN_PORT *)pcanDev;

    LW_SPIN_LOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, &iregInterLevel);
    if (iState) {
        pcanDev->CAN_uiBusState |= iState;
    } else {
        pcanDev->CAN_uiBusState = CAN_DEV_BUS_ERROR_NONE;
    }
    LW_SPIN_UNLOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, iregInterLevel);
    
    if (pcanDev->CAN_uiBusState != CAN_DEV_BUS_ERROR_NONE) {            /*  总线异常                    */
        API_SemaphoreBFlush(pcanDev->CAN_ulSendSemB, LW_NULL);          /*  激活写等待任务              */
        API_SemaphoreBFlush(pcanDev->CAN_ulRcvSemB, LW_NULL);           /*  激活读等待任务              */
        SEL_WAKE_UP_ALL(&pcanDev->CAN_selwulList, SELEXCEPT);           /*  select() 激活               */
    }
}
/*********************************************************************************************************
** 函数名称: __canDevInit
** 功能描述: 创建 CAN 设备
** 输　入  : pcanDev           CAN 设备
**           uiRdFrameSize     接收缓冲区大小
**           uiWrtFrameSize    发送缓冲区大小
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __canDevInit (__CAN_DEV *pcanDev,
                         UINT       uiRdFrameSize,
                         UINT       uiWrtFrameSize)
{
    REGISTER INT    iErrLevel = 0;

    pcanDev->CAN_ulSendTimeout = LW_OPTION_WAIT_INFINITE;               /*  初始化为永久等待            */
    pcanDev->CAN_ulRecvTimeout = LW_OPTION_WAIT_INFINITE;               /*  初始化为永久等待            */

    pcanDev->CAN_pcanqRecvQueue = __canInitQueue(uiRdFrameSize);        /*  创建读缓冲区                */
    if (pcanDev->CAN_pcanqRecvQueue == LW_NULL) {                       /*  创建失败                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    pcanDev->CAN_pcanqSendQueue = __canInitQueue(uiWrtFrameSize);       /*  创建写缓冲区                */
    if (pcanDev->CAN_pcanqSendQueue == LW_NULL) {                       /*  创建失败                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        iErrLevel = 1;
        goto    __error_handle;
    }
    pcanDev->CAN_canstatWriteState.CANSTAT_bBufEmpty = LW_TRUE;         /*  发送队列空                  */
    pcanDev->CAN_uiBusState = CAN_DEV_BUS_ERROR_NONE;
                                                                        /*  发送队列空                  */
    pcanDev->CAN_ulRcvSemB = API_SemaphoreBCreate("can_rsync",
                                                  LW_FALSE,
                                                  LW_OPTION_WAIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                  LW_NULL);             /*  读同步                      */
    if (!pcanDev->CAN_ulRcvSemB) {
        iErrLevel = 2;
        goto    __error_handle;
    }

    pcanDev->CAN_ulSendSemB = API_SemaphoreBCreate("can_wsync",
                                                   LW_TRUE,
                                                   LW_OPTION_WAIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                   LW_NULL);            /*  写同步                      */
    if (!pcanDev->CAN_ulSendSemB) {
        iErrLevel = 3;
        goto    __error_handle;
    }

    pcanDev->CAN_ulSyncSemB = API_SemaphoreBCreate("can_sync",
                                                   LW_FALSE,
                                                   LW_OPTION_WAIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                   LW_NULL);            /*  等待发送完成                */
    if (!pcanDev->CAN_ulSyncSemB) {
        iErrLevel = 4;
        goto    __error_handle;
    }

    pcanDev->CAN_ulMutexSemM = API_SemaphoreMCreate("can_lock",
                                                    LW_PRIO_DEF_CEILING,
                                                    LW_OPTION_WAIT_PRIORITY |
                                                    LW_OPTION_DELETE_SAFE |
                                                    LW_OPTION_INHERIT_PRIORITY |
                                                    LW_OPTION_OBJECT_GLOBAL,
                                                    LW_NULL);           /*  互斥访问控制信号量          */
    if (!pcanDev->CAN_ulMutexSemM) {
        iErrLevel = 5;
        goto    __error_handle;
    }

    SEL_WAKE_UP_LIST_INIT(&pcanDev->CAN_selwulList);                    /*  初始化 select 等待链        */

    LW_SPIN_INIT(&pcanDev->CAN_slLock);                                 /*  初始化自旋锁                */

    return  (ERROR_NONE);

__error_handle:
    if (iErrLevel > 4) {
        API_SemaphoreBDelete(&pcanDev->CAN_ulSyncSemB);                 /*  删除等待写完成              */
    }
    if (iErrLevel > 3) {
        API_SemaphoreBDelete(&pcanDev->CAN_ulSendSemB);                 /*  删除写同步                  */
    }
    if (iErrLevel > 2) {
        API_SemaphoreBDelete(&pcanDev->CAN_ulRcvSemB);                  /*  删除读同步                  */
    }
    if (iErrLevel > 1) {
        __canDeleteQueue(pcanDev->CAN_pcanqSendQueue);                  /*  删除读缓冲区                */
    }
    if (iErrLevel > 0) {
        __canDeleteQueue(pcanDev->CAN_pcanqRecvQueue);                  /*  删除写缓冲区                */
    }
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __canDevDelete
** 功能描述: 删除 CAN 设备资源
** 输　入  : pcanDev           CAN 设备
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __canDevDelete (__CAN_DEV *pcanDev)
{
    __canDeleteQueue(pcanDev->CAN_pcanqRecvQueue);
    __canDeleteQueue(pcanDev->CAN_pcanqSendQueue);

    API_SemaphoreBDelete(&pcanDev->CAN_ulRcvSemB);
    API_SemaphoreBDelete(&pcanDev->CAN_ulSendSemB);
    API_SemaphoreBDelete(&pcanDev->CAN_ulSyncSemB);
    API_SemaphoreMDelete(&pcanDev->CAN_ulMutexSemM);
}
/*********************************************************************************************************
** 函数名称: __canDrain
** 功能描述: CAN 设备等待发送完成
** 输　入  : pcanDev          CAN 设备
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __canDrain (__CAN_DEV  *pcanDev)
{
             INTREG     iregInterLevel;

    REGISTER ULONG      ulError;
             INT        iFree;

    do {
        LW_SPIN_LOCK_QUICK(&pcanDev->CAN_slLock, &iregInterLevel);
        iFree = __canQCount(pcanDev->CAN_pcanqSendQueue);
        LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);

        if (iFree == 0) {
            return  (ERROR_NONE);
        }

        ulError = API_SemaphoreBPend(pcanDev->CAN_ulSyncSemB, LW_OPTION_WAIT_INFINITE);
    } while (ulError == ERROR_NONE);

    _ErrorHandle(ERROR_IO_DEVICE_TIMEOUT);                              /*   超时                       */
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __canIoctl
** 功能描述: CAN 设备控制
** 输　入  : pcanDev          CAN 设备
**           iCmd             控制命令
**           lArg             参数
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __canIoctl (__CAN_DEV  *pcanDev, INT  iCmd, LONG  lArg)
{
    INTREG               iregInterLevel;
    INT                  iStatus = ERROR_NONE;
    struct stat         *pstat;
    PLW_SEL_WAKEUPNODE   pselwunNode;

    struct timeval      *timevalTemp;

    __CAN_PORT          *pcanport     = (__CAN_PORT *)pcanDev;
    CAN_DRV_FUNCS       *pCanDevFuncs = pcanport->CANPORT_pcanchan->pDrvFuncs;

    CANDEV_LOCK(pcanDev);                                               /*  等待设备使用权              */

    if (pCanDevFuncs->ioctl) {
        iStatus = pCanDevFuncs->ioctl(pcanport->CANPORT_pcanchan, iCmd, (PVOID)lArg);
    
    } else {
        iStatus = ENOSYS;
    }

    if ((iStatus == ENOSYS) ||
        ((iStatus == PX_ERROR) && (errno == ENOSYS))) {                 /*  驱动程序无法识别的命令      */

        iStatus = ERROR_NONE;                                           /*  清除驱动程序错误            */

        switch (iCmd) {

        case FIONREAD:                                                  /*  读缓冲区有效数据数量        */
            {
                LONG  lNFrame = __canQCount(pcanDev->CAN_pcanqRecvQueue);
                if (pcanDev->CAN_uiFileFrameType == CAN_STD_CAN) {
                    *((INT *)lArg) = (INT)(lNFrame * sizeof(CAN_FRAME));
                } else {
                    *((INT *)lArg) = (INT)(lNFrame * sizeof(CAN_FD_FRAME));
                }
            }
            break;

        case FIONWRITE:
            {
                LONG  lNFrame = __canQCount(pcanDev->CAN_pcanqSendQueue);
                if (pcanDev->CAN_uiFileFrameType == CAN_STD_CAN) {
                    *((INT *)lArg) = (INT)(lNFrame * sizeof(CAN_FRAME));
                } else {
                    *((INT *)lArg) = (INT)(lNFrame * sizeof(CAN_FD_FRAME));
                }
            }
            break;

        case FIOSYNC:                                                       /*  等待发送完成                */
        case FIODATASYNC:
            iStatus = __canDrain(pcanDev);
            break;

        case FIOFLUSH:                                                  /*  清空设备缓冲区              */
            __canFlushRd(pcanport);
            __canFlushWrt(pcanport);
            break;

        case FIOWFLUSH:
            __canFlushRd(pcanport);                                     /*  清空写缓冲区                */
            break;

        case FIORFLUSH:
            __canFlushWrt(pcanport);                                    /*  清空读缓冲区                */
            break;

        case FIOFSTATGET:                                               /*  获得文件属性                */
            pstat = (struct stat *)lArg;
            pstat->st_dev     = (dev_t)pcanDev;
            pstat->st_ino     = (ino_t)0;                               /*  相当于唯一节点              */
            pstat->st_mode    = 0666 | S_IFCHR;                         /*  默认属性                    */
            pstat->st_nlink   = 1;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 1;
            pstat->st_size    = 0;
            pstat->st_blksize = 0;
            pstat->st_blocks  = 0;
            pstat->st_atime   = API_RootFsTime(LW_NULL);                /*  默认使用 root fs 基准时间   */
            pstat->st_mtime   = API_RootFsTime(LW_NULL);
            pstat->st_ctime   = API_RootFsTime(LW_NULL);
            break;

        case FIOSELECT:
            pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
            SEL_WAKE_NODE_ADD(&pcanDev->CAN_selwulList, pselwunNode);

            switch (pselwunNode->SELWUN_seltypType) {

            case SELREAD:                                               /*  等待数据可读                */
                if (__canQCount(pcanDev->CAN_pcanqRecvQueue) > 0) {
                    SEL_WAKE_UP(pselwunNode);                           /*  唤醒节点                    */
                }
                break;

            case SELWRITE:
                if (__canQFreeNum(pcanDev->CAN_pcanqSendQueue) > 0) {
                    SEL_WAKE_UP(pselwunNode);                           /*  唤醒节点                    */
                }
                break;

            case SELEXCEPT:                                             /*  总线是否异常                */
                LW_SPIN_LOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, &iregInterLevel);
                if (pcanDev->CAN_uiBusState != CAN_DEV_BUS_ERROR_NONE) {
                    LW_SPIN_UNLOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, iregInterLevel);
                    SEL_WAKE_UP(pselwunNode);                           /*  唤醒节点                    */
                } else {
                    LW_SPIN_UNLOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, iregInterLevel);
                }
                break;
            }
            break;

        case FIOUNSELECT:
            SEL_WAKE_NODE_DELETE(&pcanDev->CAN_selwulList, (PLW_SEL_WAKEUPNODE)lArg);
            break;

        case CAN_DEV_GET_BUS_STATE:                                     /*  获取 CAN 控制器状态         */
            LW_SPIN_LOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, &iregInterLevel);
            *((LONG *)lArg)         = pcanDev->CAN_uiBusState;
            pcanDev->CAN_uiBusState = CAN_DEV_BUS_ERROR_NONE;           /* 读取后清除状态               */
            LW_SPIN_UNLOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, iregInterLevel);
            break;

        case FIOWTIMEOUT:
            if (lArg) {
                timevalTemp = (struct timeval *)lArg;
                pcanDev->CAN_ulSendTimeout = __timevalToTick(timevalTemp);
                                                                        /*  转换为系统时钟              */
            } else {
                pcanDev->CAN_ulSendTimeout = LW_OPTION_WAIT_INFINITE;
            }
            break;

        case FIORTIMEOUT:
            if (lArg) {
                timevalTemp = (struct timeval *)lArg;
                pcanDev->CAN_ulRecvTimeout = __timevalToTick(timevalTemp);
                                                                        /*  转换为系统时钟              */
            } else {
                pcanDev->CAN_ulRecvTimeout = LW_OPTION_WAIT_INFINITE;
            }
            break;

        case CAN_FIO_CAN_FD:                                            /*  设置 CAN 文件操作帧类型     */
            if (lArg == CAN_STD_CAN) {
                pcanDev->CAN_uiFileFrameType = CAN_STD_CAN;

            } else {
                pcanDev->CAN_uiFileFrameType = CAN_STD_CAN_FD;
            }
            break;

        default:
             _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
             iStatus = PX_ERROR;
             break;
        }
    }
    
    CANDEV_UNLOCK(pcanDev);                                             /*  释放设备使用权              */

    return  (iStatus);
}
/*********************************************************************************************************
** 函数名称: __canOpen
** 功能描述: CAN 设备打开
** 输　入  : pcanDev          CAN 设备
**           pcName           设备名称
**           iFlags           打开设备时使用的标志
**           iMode            打开的方式，保留
** 输　出  : CAN 设备指针
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LONG  __canOpen (__CAN_DEV  *pcanDev,
                        PCHAR       pcName,
                        INT         iFlags,
                        INT         iMode)
{
    __CAN_PORT      *pcanport = (__CAN_PORT *)pcanDev;

    if (LW_DEV_INC_USE_COUNT(&pcanDev->CAN_devhdr) == 1) {
        if (pcanport->CANPORT_pcanchan->pDrvFuncs->ioctl) {             /*  打开端口                    */
            pcanport->CANPORT_pcanchan->pDrvFuncs->ioctl(pcanport->CANPORT_pcanchan,
                                                         CAN_DEV_OPEN, LW_NULL);
        }
    }
    
    return  ((LONG)pcanDev);
}
/*********************************************************************************************************
** 函数名称: __canClose
** 功能描述: CAN 设备关闭
** 输　入  : pcanDev          CAN 设备
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __canClose (__CAN_DEV   *pcanDev)
{
    __CAN_PORT      *pcanport = (__CAN_PORT *)pcanDev;

    if (LW_DEV_GET_USE_COUNT(&pcanDev->CAN_devhdr)) {
        if (!LW_DEV_DEC_USE_COUNT(&pcanDev->CAN_devhdr)) {
            if (pcanport->CANPORT_pcanchan->pDrvFuncs->ioctl) {         /*  挂起端口                    */
                pcanport->CANPORT_pcanchan->pDrvFuncs->ioctl(pcanport->CANPORT_pcanchan,
                                                             CAN_DEV_CLOSE, LW_NULL);
            }
            SEL_WAKE_UP_ALL(&pcanDev->CAN_selwulList, SELEXCEPT);       /*  激活异常等待                */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __canWrite
** 功能描述: 写 CAN 设备
** 输　入  : pcanDev          CAN 设备
**           pvCanFrame       写缓冲区指针
**           stNBytes         发送缓冲区字节数
** 输　出  : 返回实际写入的个数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t __canWrite (__CAN_DEV  *pcanDev, PVOID  pvCanFrame, size_t  stNBytes)
{
    INTREG         iregInterLevel;
    INT            iFrameput;
    INT            i = 0;
    ULONG          ulError;
    __CAN_PORT    *pcanport = (__CAN_PORT *)pcanDev;
    UINT           uiFrameType;
    size_t         stNumber;
    size_t         stUnit;

    if (pcanDev->CAN_uiFileFrameType == CAN_STD_CAN) {
        uiFrameType = CAN_STD_CAN;
        stUnit      = sizeof(CAN_FRAME);

    } else {
        uiFrameType = CAN_STD_CAN_FD;
        stUnit      = sizeof(CAN_FD_FRAME);
    }

    stNumber = stNBytes / stUnit;                                       /*  转换为数据包个数            */

    while (stNumber > 0) {
        ulError = API_SemaphoreBPend(pcanDev->CAN_ulSendSemB,
                                     pcanDev->CAN_ulSendTimeout);
        if (ulError) {
            _ErrorHandle(ERROR_IO_DEVICE_TIMEOUT);                      /*   超时                       */
            return  ((ssize_t)(i * stUnit));
        }

        CANDEV_LOCK(pcanDev);                                           /*  等待设备使用权              */
        
        LW_SPIN_LOCK_QUICK(&pcanDev->CAN_slLock, &iregInterLevel);
        if (pcanDev->CAN_uiBusState != CAN_DEV_BUS_ERROR_NONE) {        /*  总线错误                    */
            LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
            CANDEV_UNLOCK(pcanDev);
            _ErrorHandle(EIO);
            return  ((ssize_t)(i * stUnit));
        }
        LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);

        iFrameput = __canWriteQueue(pcanDev,
                                    pcanDev->CAN_pcanqSendQueue,
                                    pvCanFrame,
                                    uiFrameType,
                                    (INT)stNumber);

        __canTxStartup(pcanport);                                       /*  启动一次发送                */

        stNumber   -= (size_t)iFrameput;                                /*  剩余需要发送的数据          */
        i          += iFrameput;
        pvCanFrame  = (PVOID)((addr_t)pvCanFrame + (iFrameput * stUnit));

        LW_SPIN_LOCK_QUICK(&pcanDev->CAN_slLock, &iregInterLevel);
                                                                        /*  关闭中断                    */
        if (__canQFreeNum(pcanDev->CAN_pcanqSendQueue) > 0) {
            LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel); /*  打开中断                    */
            API_SemaphoreBPost(pcanDev->CAN_ulSendSemB);                /*  缓冲区还有空间              */
        
        } else {
            LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
                                                                        /*  打开中断                    */
        }

        CANDEV_UNLOCK(pcanDev);                                         /*  释放设备使用权              */
    }

    return  ((ssize_t)(i * stUnit));
}
/*********************************************************************************************************
** 函数名称: __canRead
** 功能描述: 读 CAN 设备
** 输　入  : pcanDev          CAN 设备
**           pvCanFrame       CAN发送缓冲区指针
**           stNBytes         读取缓冲区的字节数
** 输　出  : 返回实际读取的个数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t __canRead (__CAN_DEV  *pcanDev, PVOID  pvCanFrame, size_t  stNBytes)
{
             INTREG       iregInterLevel;
    REGISTER ssize_t      sstNRead;
             UINT         uiFrameType;
             size_t       stNumber;
             size_t       stUnit;
             ULONG        ulError;

    if (pcanDev->CAN_uiFileFrameType == CAN_STD_CAN) {
        uiFrameType = CAN_STD_CAN;
        stUnit      = sizeof(CAN_FRAME);

    } else {
        uiFrameType = CAN_STD_CAN_FD;
        stUnit      = sizeof(CAN_FD_FRAME);
    }

    stNumber = stNBytes / stUnit;                                       /*  转换为数据包个数            */

    for (;;) {
        ulError = API_SemaphoreBPend(pcanDev->CAN_ulRcvSemB,
                                     pcanDev->CAN_ulRecvTimeout);
        if (ulError) {
            _ErrorHandle(ERROR_IO_DEVICE_TIMEOUT);                      /*  超时                        */
            return  (0);
        }

        CANDEV_LOCK(pcanDev);                                           /*  等待设备使用权              */

        LW_SPIN_LOCK_QUICK(&pcanDev->CAN_slLock, &iregInterLevel);
                                                                        /*  关闭中断                    */
        if (__canQCount(pcanDev->CAN_pcanqRecvQueue)) {                 /*  检查是否有数据              */
            break;
        
        } else {
            if (pcanDev->CAN_uiBusState != CAN_DEV_BUS_ERROR_NONE) {    /*  总线错误                    */
                LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
                CANDEV_UNLOCK(pcanDev);
                _ErrorHandle(EIO);
                return  (0);
            }
        }
        LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
                                                                        /*  打开中断                    */
        CANDEV_UNLOCK(pcanDev);                                         /*  释放设备使用权              */
    }

    LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
    sstNRead = __canReadQueue(pcanDev,
                              pcanDev->CAN_pcanqRecvQueue,
                              pvCanFrame,
                              uiFrameType,
                              (INT)stNumber);
    LW_SPIN_LOCK_QUICK(&pcanDev->CAN_slLock, &iregInterLevel);

    if (__canQCount(pcanDev->CAN_pcanqRecvQueue)) {                     /*  是否还有数据                */
        LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
        API_SemaphoreBPost(pcanDev->CAN_ulRcvSemB);                     /*  通知其他等待读的线程        */
    
    } else {
        LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
    }

    CANDEV_UNLOCK(pcanDev);                                             /*  释放设备使用权              */

    return  (sstNRead * stUnit);
}
/*********************************************************************************************************
** 函数名称: API_CanDrvInstall
** 功能描述: 安装 can 驱动程序
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_CanDrvInstall (void)
{
    if (_G_iCanDrvNum > 0) {
        return  (ERROR_NONE);
    }

    _G_iCanDrvNum = iosDrvInstall(__canOpen, LW_NULL, __canOpen, __canClose,
                                  __canRead, __canWrite, __canIoctl);

    DRIVER_LICENSE(_G_iCanDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iCanDrvNum,      "Wang.feng");
    DRIVER_DESCRIPTION(_G_iCanDrvNum, "CAN Bus driver.");

    return  (_G_iCanDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_CanDevCreate
** 功能描述: 创建 can 设备
** 输　入  : pcName           设备名
**           pcanchan         通道
**           uiRdFrameSize    接收缓冲区大小
**           uiWrtFrameSize   发送缓冲区大小
** 输　出  : 是否成功
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_CanDevCreate (PCHAR     pcName,
                       CAN_CHAN *pcanchan,
                       UINT      uiRdFrameSize,
                       UINT      uiWrtFrameSize)
{
    __PCAN_PORT   pcanport;
    INT           iTemp;

    if ((!pcName) || (!pcanchan)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((uiRdFrameSize < 1) || (uiWrtFrameSize < 1)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((!pcanchan->pDrvFuncs->callbackInstall) ||
        (!pcanchan->pDrvFuncs->txStartup)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (_G_iCanDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can Driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }

    pcanport = (__PCAN_PORT)__SHEAP_ALLOC(sizeof(__CAN_PORT));
    if (!pcanport) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pcanport, sizeof(__CAN_PORT));

    iTemp = __canDevInit(&pcanport->CANPORT_can,
                         uiRdFrameSize,
                         uiWrtFrameSize);                               /*  初始化设备控制块            */

    if (iTemp != ERROR_NONE) {
        __SHEAP_FREE(pcanport);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    __canFlushRd(pcanport);
    __canFlushWrt(pcanport);

    pcanport->CANPORT_pcanchan = pcanchan;

    pcanchan->pDrvFuncs->callbackInstall(pcanchan, CAN_CALLBACK_GET_TX_DATA,
                                        (CAN_CALLBACK)__canITx, (PVOID)pcanport);
    pcanchan->pDrvFuncs->callbackInstall(pcanchan, CAN_CALLBACK_PUT_RCV_DATA,
                                        (CAN_CALLBACK)__canIRx, (PVOID)pcanport);
    pcanchan->pDrvFuncs->callbackInstall(pcanchan, CAN_CALLBACK_PUT_BUS_STATE,
                                        (CAN_CALLBACK)__canSetBusState, (PVOID)pcanport);

    if (pcanchan->pDrvFuncs->ioctl) {
        pcanchan->pDrvFuncs->ioctl(pcanchan, CAN_DEV_CAN_FD,
                                   &pcanport->CANPORT_can.CAN_uiDevFrameType);
    }

    iTemp = (INT)iosDevAddEx(&pcanport->CANPORT_can.CAN_devhdr, pcName,
                             _G_iCanDrvNum, DT_CHR);
    if (iTemp) {
        __canDevDelete(&pcanport->CANPORT_can);
        __SHEAP_FREE(pcanport);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "add device error.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: API_CanDevRemove
** 功能描述: 移除一个 CAN 设备
** 输　入  : pcName           需要移除的 CAN 设备
**           bForce           模式选择
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_CanDevRemove (PCHAR   pcName, BOOL  bForce)
{
    __PCAN_PORT     pDevHdr;
    PCHAR           pcTail = LW_NULL;

    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }

    if (_G_iCanDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }

    pDevHdr = (__PCAN_PORT)iosDevFind(pcName, &pcTail);
    if ((pDevHdr == LW_NULL) || (pcName == pcTail)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device not found.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }

    if (bForce == LW_FALSE) {
        if (LW_DEV_GET_USE_COUNT(&pDevHdr->CANPORT_can.CAN_devhdr)) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "too many open files.\r\n");
            _ErrorHandle(EBUSY);
            return  (PX_ERROR);
        }
        if (SEL_WAKE_UP_LIST_LEN(&pDevHdr->CANPORT_can.CAN_selwulList) > 0) {
            errno = EBUSY;
            return  (PX_ERROR);
       }
    }

    iosDevFileAbnormal(&pDevHdr->CANPORT_can.CAN_devhdr);
    iosDevDelete(&pDevHdr->CANPORT_can.CAN_devhdr);

    SEL_WAKE_UP_LIST_TERM(&pDevHdr->CANPORT_can.CAN_selwulList);

    __canDevDelete(&pDevHdr->CANPORT_can);

    __SHEAP_FREE((PVOID)pDevHdr);

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_CAN_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
