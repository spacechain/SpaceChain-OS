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
** 文   件   名: sdhci.h
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2011 年 01 月 17 日
**
** 描        述: SD 标准主控制器驱动源文件(SD Host Controller Simplified Specification Version 2.00).

** BUG:
2011.03.02  增加 主控传输模式查看\设置函数.允许动态改变传输模式(主控上不存在设备的情况下).
2011.04.07  增加 SDMA 传输功能.
2011.04.07  考虑到 SD 控制器在不同平台上其寄存器可能在 IO 空间,也可能在内存空间,所以读写寄存器
            的6个函数申明为外部函数,由具体平台的驱动实现.
2014.11.13  修改 一般传输模式为中断方式, 同时支持 SDIO 中断处理
2014.11.15  增加 SDHCI 注册到 SDM 的相关功能函数
2014.11.24  修正 SDIO 中断服务线程的一个 BUG, 在等到中断信号量后再获取 SDHCI 控制器对象.
2014.11.29  修正 SDHCI 中断服务函数, 增加防止 SDIO 中断异常丢失的处理.
2015.03.10  修正 ACMD12 功能的中断掩码设置.
            增加 QUIRK 相关代码.
2015.03.11  增加 卡写保护功能支持.
2015.03.14  增加 SDHCI 不能发出 SDIO 硬件中断的处理.
2015.11.20  增加 对不支持 ACMD12 的控制器处理.
            增加 对 SDHCI v3.0 的兼容性支持.
2016.12.16  修正 在传输过程中可能产生死锁的问题.
2017.03.15  修正 使用查询 SDIO 中断情况下, 应用线程禁止中断可能造成死锁的问题.
2017.07.10  修正 带忙应答的命令处理应考虑数据完成中断和命令完成中断的先后顺序.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_IO
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0)
#include "sdhci.h"
#include "../include/sddebug.h"
#include "../core/sddrvm.h"
#include "../core/sdstd.h"
#include "../core/sdiostd.h"
/*********************************************************************************************************
  内部宏
*********************************************************************************************************/
#define __SDHCI_HOST_DEVNUM_MAX   1                                     /*  一个控制器支持的槽数量      */
#define __SDHCI_MAX_BASE_CLK      102400000                             /*  主控允许最大基时钟102.4Mhz  */

#define __SDHCI_CMD_TOUT_SEC      4                                     /*  命令超时物理时间(经验值)    */
#define __SDHCI_CMD_RETRY         20000                                 /*  命令超时计数                */

#define __SDHCI_DMA_BOUND_LEN     (1 << (12 + __SDHCI_DMA_BOUND_NBITS))
#define __SDHCI_DMA_BOUND_NBITS   7                                     /*  系统连续内存边界位指示      */
                                                                        /*  当前设定值为 7 (最大):      */
                                                                        /*  1 << (12 + 7) = 512k        */
                                                                        /*  当数据搬移时超过512K边界,需 */
                                                                        /*  要更新DMA地址               */
#define __SDHCI_HOST_NAME(phs)    \
        ((phs)->SDHCIHS_psdadapter->SDADAPTER_busadapter.BUSADAPTER_cName)

#define __SDHCI_OCR_330           (SD_VDD_32_33 | SD_VDD_33_34)
#define __SDHCI_OCR_300           (SD_VDD_30_31 | SD_VDD_31_32)
#define __SDHCI_OCR_180           (SD_VDD_165_195)

#define __SDHCI_INT_EN_MASK       (SDHCI_INT_BUS_POWER | SDHCI_INT_DATA_END_BIT |\
                                   SDHCI_INT_DATA_CRC  | SDHCI_INT_DATA_TIMEOUT |\
                                   SDHCI_INT_INDEX     | SDHCI_INT_END_BIT      |\
                                   SDHCI_INT_CRC       | SDHCI_INT_TIMEOUT      |\
                                   SDHCI_INT_DATA_END  | SDHCI_INT_RESPONSE     |\
                                   SDHCI_INT_ACMD12ERR)
/*********************************************************************************************************
  传输事务线程相关
*********************************************************************************************************/
#define __SDHCI_SDIOINTSVR_PRIO   197
#define __SDHCI_SDIOINTSVR_STKSZ  (8 * 1024)
/*********************************************************************************************************
  传输事务线操作宏定义
*********************************************************************************************************/
#define __SDHCI_TRANS_LOCK(pt)    LW_SPIN_LOCK(&pt->SDHCITS_slLock)
#define __SDHCI_TRANS_UNLOCK(pt)  LW_SPIN_UNLOCK(&pt->SDHCITS_slLock)
/*********************************************************************************************************
  适用于控制器能发出 SDIO 硬件中断的情况
*********************************************************************************************************/
#define __SDHCI_SDIO_WAIT(pt)     API_SemaphoreCPend(pt->SDHCITS_hSdioIntSem, LW_OPTION_WAIT_INFINITE)
#define __SDHCI_SDIO_NOTIFY(pt)   API_SemaphoreCPost(pt->SDHCITS_hSdioIntSem)
/*********************************************************************************************************
  适用于控制器不能发出 SDIO 硬件中断, 使用查询方式处理 SDIO 中断的情况
*********************************************************************************************************/
#define __SDHCI_SDIO_DISABLE(pt)  API_SemaphoreBPend(pt->SDHCITS_hSdioIntSem, LW_OPTION_WAIT_INFINITE)
#define __SDHCI_SDIO_ENABLE(pt)   API_SemaphoreBPost(pt->SDHCITS_hSdioIntSem)
#define __SDHCI_SDIO_REQUEST(pt)  __SDHCI_SDIO_DISABLE(pt)
#define __SDHCI_SDIO_RELEASE(pt)  __SDHCI_SDIO_ENABLE(pt)

#define __SDHCI_SDIO_INT_MAX      4
#define __SDHCI_SDIO_DLY_MAX      3
/*********************************************************************************************************
  前置声明
*********************************************************************************************************/
struct __sdhci_trans;
struct __sdhci_host;
struct __sdhci_sdm_host;

typedef struct __sdhci_trans      __SDHCI_TRANS;
typedef struct __sdhci_host       __SDHCI_HOST;
typedef struct __sdhci_sdm_host   __SDHCI_SDM_HOST;
typedef struct __sdhci_trans     *__PSDHCI_TRANS;
typedef struct __sdhci_host      *__PSDHCI_HOST;
typedef struct __sdhci_sdm_host  *__PSDHCI_SDM_HOST;
/*********************************************************************************************************
  标准主控器功能描述结构
*********************************************************************************************************/
typedef struct __sdhci_capab {
    UINT32      SDHCICAP_uiBaseClkFreq;                                 /*  基时钟频率                  */
    UINT32      SDHCICAP_uiMaxBlkSize;                                  /*  支持的最大块长度            */
    UINT32      SDHCICAP_uiVoltage;                                     /*  电压支持情况                */

    BOOL        SDHCICAP_bCanSdma;                                      /*  是否支持 SDMA               */
    BOOL        SDHCICAP_bCanAdma;                                      /*  是否支持 ADMA               */
    BOOL        SDHCICAP_bCanHighSpeed;                                 /*  是否支持高速传输            */
    BOOL        SDHCICAP_bCanSusRes;                                    /*  是否支持挂起\恢复功能       */
}__SDHCI_CAPAB, *__PSDHCI_CAPAB;
/*********************************************************************************************************
  SD 标准控制器规范中, 其块大小最大为 2048(SDIO 设备最大块也是 2048), 最大块计数为 65535.
  同时, 在 DMA 传输中, 单次最大传输 512KB.
  1. 一般传输模式:
     该模式只用到 BlkSize 和 BlkCnt 寄存器. 用户请求的 BlkSize 一定是不超过 2048 的, 但是 BlkCnt 可能
     超过65535. 此时就需要对数据进行拆分.
     比如用户数据 BlkSize 为32, BlkCnt 为 80000, 则需要拆分为两次传输.

  2. DMA 传输:
     此时不仅要考虑 1 的情况, 还应该将用户数据拆分为 N 个 512KB 进行 DMA 传输.
     比如用户数据 BlkSize 为2048, BlkCnt 为256. 则需要拆分为 2 个 512 KB 的 DMA 传输.

  3. 在实际使用中, 上述情形及其少见(为了传输的稳定和减少出错). 因此, 当前版本不支持上面情况的特殊处理.

  标准主控器传输事务控制块
  这里将用户提交的单次请求(包括发送命令、数据读写、读取应答等一整套流程操作)称作一次事务.
*********************************************************************************************************/
struct __sdhci_trans {
    __PSDHCI_HOST         SDHCITS_psdhcihost;
#if LW_CFG_VMM_EN > 0
    UINT8                *SDHCITS_pucDmaBuffer;
#endif
    LW_OBJECT_HANDLE      SDHCITS_hFinishSync;                          /*  传输事务完成同步信号        */

    BOOL                  SDHCITS_bCmdFinish;
    BOOL                  SDHCITS_bDatFinish;
    INT                   SDHCITS_iCmdError;
    INT                   SDHCITS_iDatError;                            /*  事务状态控制                */

    UINT8                *SDHCITS_pucDatBuffCurr;
    UINT32                SDHCITS_uiBlkSize;
    UINT32                SDHCITS_uiBlkCntRemain;
    BOOL                  SDHCITS_bIsRead;                              /*  事务传输状态控制            */

    INT                   SDHCITS_iTransType;
#define __SDHIC_TRANS_NORMAL    0
#define __SDHIC_TRANS_SDMA      1
#define __SDHIC_TRANS_ADMA      2

    LW_OBJECT_HANDLE      SDHCITS_hSdioIntThread;                       /*  负责 SDIO 中断处理线程      */
    LW_OBJECT_HANDLE      SDHCITS_hSdioIntSem;                          /*  SDIO 中断同步信号           */

    UINT32                SDHCITS_uiIntSta;

    INT                   SDHCITS_iStage;
#define __SDHCI_TRANS_STAGE_START  0
#define __SDHCI_TRANS_STAGE_STOP   1
    LW_SD_COMMAND        *SDHCITS_psdcmd;
    LW_SD_COMMAND        *SDHCITS_psdcmdStop;
    LW_SD_DATA           *SDHCITS_psddat;                               /*  对用户请求的引用            */

    LW_SPINLOCK_DEFINE   (SDHCITS_slLock);
};
/*********************************************************************************************************
  标准主控器 HOST结构
*********************************************************************************************************/
struct __sdhci_host {
    LW_SD_FUNCS             SDHCIHS_sdfunc;                             /*  对应的驱动函数              */
    PLW_SD_ADAPTER          SDHCIHS_psdadapter;                         /*  对应的总线适配器            */
    LW_SDHCI_HOST_ATTR      SDHCIHS_sdhcihostattr;                      /*  主控属性                    */
    __SDHCI_CAPAB           SDHCIHS_sdhcicap;                           /*  主控功能                    */
    UINT32                  SDHCIHS_uiVersion;                          /*  硬件版本                    */
    atomic_t                SDHCIHS_atomicDevCnt;                       /*  设备计数                    */

    UINT32                  SDHCIHS_ucTransferMod;                      /*  主控使用的传输模式          */
    INT                   (*SDHCIHS_pfuncMasterXfer)                    /*  主控当前使用的传输函数      */
                          (
                          __PSDHCI_HOST        psdhcihost,
                          PLW_SD_DEVICE        psddev,
                          PLW_SD_MESSAGE       psdmsg,
                          INT                  iNum
                          );

    __PSDHCI_TRANS          SDHCIHS_psdhcitrans;                        /*  传输事务控制块对象          */
    __PSDHCI_SDM_HOST       SDHCIHS_psdhcisdmhost;                      /*  SDM 层 HOST 信息            */

    UINT32                  SDHCIHS_uiClkCurr;
    BOOL                    SDHCIHS_bClkEnable;
    BOOL                    SDHCIHS_bSdioIntEnable;
    UINT8                   SDHCIHS_ucDevType;                          /*  当前设备的类型              */
};
/*********************************************************************************************************
  标准主控器设备内部结构
*********************************************************************************************************/
typedef struct __sdhci_dev {
    PLW_SD_DEVICE       SDHCIDEV_psddevice;                             /*  对应的 SD 设备              */
    __PSDHCI_HOST       SDHCIDEV_psdhcihost;                            /*  对应的 HOST                 */
    CHAR                SDHCIDEV_pcDevName[LW_CFG_OBJECT_NAME_SIZE];    /*  设备名称(与对应 SD 设备相同)*/
} __SDHCI_DEVICE, *__PSDHCI_DEVICE;
/*********************************************************************************************************
  标准主控器注册到 SDM 模块的数据结构
*********************************************************************************************************/
struct __sdhci_sdm_host {
    SD_HOST         SDHCISDMH_sdhost;
    __PSDHCI_HOST   SDHCISDMH_psdhcihost;
    SD_CALLBACK     SDHCISDMH_callbackChkDev;
    PVOID           SDHCISDMH_pvCallBackArg;
    PVOID           SDHCISDMH_pvDevAttached;
    PVOID           SDHCISDMH_pvSdmHost;
};
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static SDHCI_DRV_FUNCS _G_sdhcidrvfuncTbl[2];
/*********************************************************************************************************
  私有函数申明
*********************************************************************************************************/
static VOID __sdhciHostCapDecode(PLW_SDHCI_HOST_ATTR psdhcihostattr, __PSDHCI_CAPAB psdhcicap);
/*********************************************************************************************************
  CALLBACK FOR SD BUS LAYER
*********************************************************************************************************/
static INT __sdhciTransfer(PLW_SD_ADAPTER      psdadapter,
                           PLW_SD_DEVICE       psddev,
                           PLW_SD_MESSAGE      psdmsg,
                           INT                 iNum);
static INT __sdhciIoCtl(PLW_SD_ADAPTER  psdadapter,
                        INT             iCmd,
                        LONG            lArg);
/*********************************************************************************************************
  FOR I\O CONTROL PRIVATE
*********************************************************************************************************/
static INT __sdhciClockSet(__PSDHCI_HOST     psdhcihost, UINT32 uiSetClk);
static INT __sdhciClockStop(__PSDHCI_HOST    psdhcihost);
static INT __sdhciBusWidthSet(__PSDHCI_HOST  psdhcihost, UINT32 uiBusWidth);
static INT __sdhciPowerOn(__PSDHCI_HOST      psdhcihost);
static INT __sdhciPowerOff(__PSDHCI_HOST     psdhcihost);
static INT __sdhciPowerSetVol(__PSDHCI_HOST  psdhcihost,
                              UINT8          ucVol,
                              BOOL           bIsOn);
static INT __sdhciHighSpeedEn(__PSDHCI_HOST  psdhcihost,  BOOL bEnable);
/*********************************************************************************************************
  FOR SDM LAYER
*********************************************************************************************************/
static __SDHCI_SDM_HOST *__sdhciSdmHostNew(__PSDHCI_HOST      psdhcihost);
static INT               __sdhciSdmHostDel(__SDHCI_SDM_HOST  *psdhcisdmhost);

static INT               __sdhciSdmCallBackInstall(SD_HOST          *psdhost,
                                                   INT               iCallbackType,
                                                   SD_CALLBACK       callback,
                                                   PVOID             pvCallbackArg);
static INT               __sdhciSdmCallBackUnInstall(SD_HOST          *psdhost,
                                                     INT               iCallbackType);
static VOID              __sdhciSdmSdioIntEn(SD_HOST *psdhost, BOOL bEnable);
static BOOL              __sdhciSdmIsCardWp(SD_HOST *psdhost);

static VOID              __sdhciSdmDevAttach(SD_HOST *psdhost, CPCHAR cpcDevName);
static VOID              __sdhciSdmDevDetach(SD_HOST *psdhost);
/*********************************************************************************************************
  FOR TRANSFER PRIVATE
*********************************************************************************************************/
static INT __sdhciTransferNorm(__PSDHCI_HOST       psdhcihost,
                               PLW_SD_DEVICE       psddev,
                               PLW_SD_MESSAGE      psdmsg,
                               INT                 iNum);
static INT __sdhciTransferSdma(__PSDHCI_HOST       psdhcihost,
                               PLW_SD_DEVICE       psddev,
                               PLW_SD_MESSAGE      psdmsg,
                               INT                 iNum);
static INT __sdhciTransferAdma(__PSDHCI_HOST       psdhcihost,
                               PLW_SD_DEVICE       psddev,
                               PLW_SD_MESSAGE      psdmsg,
                               INT                 iNum);
static INT  __sdhciCmdSend(__PSDHCI_HOST   psdhcihost,
                           PLW_SD_COMMAND  psdcmd,
                           PLW_SD_DATA     psddat);

static VOID __sdhciTransferModSet(__PSDHCI_HOST   psdhcihost);
static VOID __sdhciDataPrepareNorm(__PSDHCI_HOST  psdhcihost);
static VOID __sdhciDataPrepareSdma(__PSDHCI_HOST  psdhcihost);
static VOID __sdhciDataPrepareAdma(__PSDHCI_HOST  psdhcihost);

static VOID __sdhciTransferIntSet(__PSDHCI_HOST  psdhcihost);
static VOID __sdhciIntDisAndEn(__PSDHCI_HOST     psdhcihost,
                               UINT32            uiDisMask,
                               UINT32            uiEnMask);
static VOID __sdhciSdioIntEn(__PSDHCI_HOST     psdhcihost, BOOL bEnable);
/*********************************************************************************************************
  FOR TRANSACTION
*********************************************************************************************************/
static __SDHCI_TRANS *__sdhciTransNew(__PSDHCI_HOST  psdhcihost);
static VOID           __sdhciTransDel(__SDHCI_TRANS *psdhcitrans);
static INT            __sdhciTransTaskInit(__SDHCI_TRANS *psdhcitrans);
static INT            __sdhciTransTaskDeInit(__SDHCI_TRANS *psdhcitrans);

static INT            __sdhciTransPrepare(__SDHCI_TRANS *psdhcitrans,
                                          LW_SD_MESSAGE *psdmsg,
                                          INT            iTransType);
static INT            __sdhciTransStart(__SDHCI_TRANS *psdhcitrans);
static INT            __sdhciTransFinishWait(__SDHCI_TRANS *psdhcitrans);
static VOID           __sdhciTransFinish(__SDHCI_TRANS *psdhcitrans);
static INT            __sdhciTransClean(__SDHCI_TRANS *psdhcitrans);

static irqreturn_t    __sdhciTransIrq(VOID *pvArg, ULONG ulVector);
static PVOID          __sdhciSdioIntSvr(VOID *pvArg);

static INT            __sdhciTransCmdHandle(__SDHCI_TRANS *psdhcitrans, UINT32 uiIntSta);
static INT            __sdhciTransDatHandle(__SDHCI_TRANS *psdhcitrans, UINT32 uiIntSta);

static INT            __sdhciTransCmdFinish(__SDHCI_TRANS *psdhcitrans);
static INT            __sdhciTransDatFinish(__SDHCI_TRANS *psdhcitrans);

static INT            __sdhciDataReadNorm(__PSDHCI_TRANS    psdhcitrans);
static INT            __sdhciDataWriteNorm(__PSDHCI_TRANS    psdhcitrans);
/*********************************************************************************************************
  FOR IO ACCESS DRV
*********************************************************************************************************/
static UINT32 __sdhciIoReadL(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr);
static UINT16 __sdhciIoReadW(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr);
static UINT8  __sdhciIoReadB(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr);

static VOID   __sdhciIoWriteL(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT32 uiLword);
static VOID   __sdhciIoWriteW(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT16 usWord);
static VOID   __sdhciIoWriteB(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT8 ucByte);

static UINT32 __sdhciMemReadL(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr);
static UINT16 __sdhciMemReadW(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr);
static UINT8  __sdhciMemReadB(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr);
static VOID   __sdhciMemWriteL(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT32 uiLword);
static VOID   __sdhciMemWriteW(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT16 usWord);
static VOID   __sdhciMemWriteB(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT8 ucByte);

static INT    __sdhciRegAccessDrvInit(PLW_SDHCI_HOST_ATTR  psdhcihostattr);
/*********************************************************************************************************
  FOR DEBUG
*********************************************************************************************************/
#ifdef __SYLIXOS_DEBUG
static VOID __sdhciPreStaShow(PLW_SDHCI_HOST_ATTR psdhcihostattr);
static VOID __sdhciIntStaShow(PLW_SDHCI_HOST_ATTR psdhcihostattr);
#else
#define     __sdhciPreStaShow(x)
#define     __sdhciIntStaShow(x)
#endif                                                                  /*  __SYLIXOS_DEBUG             */
/*********************************************************************************************************
  INLINE FUNCTIONS
*********************************************************************************************************/
static LW_INLINE INT __sdhciCmdRespType (PLW_SD_COMMAND psdcmd)
{
    UINT32  uiRespFlag = SD_RESP_TYPE(psdcmd);
    INT     iType      = 0;

    if (!(uiRespFlag & SD_RSP_PRESENT)) {
        iType = SDHCI_CMD_RESP_TYPE_NONE;
    } else if (uiRespFlag & SD_RSP_136) {
        iType = SDHCI_CMD_RESP_TYPE_LONG;
    } else if (uiRespFlag & SD_RSP_BUSY) {
        iType = SDHCI_CMD_RESP_TYPE_SHORT_BUSY;
    } else {
        iType = SDHCI_CMD_RESP_TYPE_SHORT;
    }

    return  (iType);
}

static LW_INLINE VOID __sdhciSdmaAddrUpdate (__PSDHCI_HOST psdhcihost, LONG lSysAddr)
{
    SDHCI_WRITEL(&psdhcihost->SDHCIHS_sdhcihostattr,
                 SDHCI_SYS_SDMA,
                 (UINT32)lSysAddr);
}

static LW_INLINE VOID __sdhciHostReset (__PSDHCI_HOST psdhcihost, UINT8 ucBitMask)
{
    INT     iTimeout = 1000;

    SDHCI_WRITEB(&psdhcihost->SDHCIHS_sdhcihostattr,
                 SDHCI_SOFTWARE_RESET,
                 ucBitMask);
    while ((SDHCI_READB(&psdhcihost->SDHCIHS_sdhcihostattr,
                        SDHCI_SOFTWARE_RESET) & ucBitMask) && iTimeout) {
        iTimeout--;
    }

    if (iTimeout <= 0) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "host rest timeout.\r\n");
    }
}

static LW_INLINE VOID __sdhciDmaSelect (__PSDHCI_HOST psdhcihost, UINT8 ucDmaType)
{
    UINT8 ucHostCtrl;

    ucHostCtrl = SDHCI_READB(&psdhcihost->SDHCIHS_sdhcihostattr,
                             SDHCI_HOST_CONTROL);
    ucHostCtrl = (ucHostCtrl & (~SDHCI_HCTRL_DMA_MASK)) | ucDmaType;
    SDHCI_WRITEB(&psdhcihost->SDHCIHS_sdhcihostattr,
                 SDHCI_HOST_CONTROL,
                 ucHostCtrl);
}

static LW_INLINE VOID __sdhciIntClear (__PSDHCI_HOST psdhcihost)
{
    SDHCI_WRITEL(&psdhcihost->SDHCIHS_sdhcihostattr, SDHCI_INT_STATUS, SDHCI_INT_ALL_MASK);
}

static LW_INLINE VOID __sdhciTimeoutSet (__PSDHCI_HOST psdhcihost)
{
    LW_SDHCI_HOST_ATTR *psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;

    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncTimeoutSet) {
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncTimeoutSet(psdhcihostattr);
    } else {
        SDHCI_WRITEB(psdhcihostattr, SDHCI_TIMEOUT_CONTROL, 0xe);
    }
}
/*********************************************************************************************************
** 函数名称: API_SdhciHostCreate
** 功能描述: 创建SD标准主控制器
** 输    入: psdAdapter     所对应的 SD 总线适配器名称
**           psdhcihostattr 主控器属性
** 输    出: 成功: 返回主控对象指针. 失败: 返回 NULL.
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API PVOID  API_SdhciHostCreate (CPCHAR               pcAdapterName,
                                   PLW_SDHCI_HOST_ATTR  psdhcihostattr)
{
    PLW_SD_ADAPTER    psdadapter    = LW_NULL;
    __PSDHCI_HOST     psdhcihost    = LW_NULL;
    __PSDHCI_TRANS    psdhcitrans   = LW_NULL;
    __PSDHCI_SDM_HOST psdhcisdmhost = LW_NULL;
    INT               iError;

    if (!pcAdapterName || !psdhcihostattr) {
        _ErrorHandle(EINVAL);
        return  ((PVOID)LW_NULL);
    }

    /*
     * SDHCI 标准控制器的时钟分频系数有最大限制, 因此输入时钟不能超过最大值
     * 但是如果使用驱动自定义的时钟设置方法, 则该限制忽略
     */
    if (psdhcihostattr->SDHCIHOST_uiMaxClock > __SDHCI_MAX_BASE_CLK) {
        if (!psdhcihostattr->SDHCIHOST_pquirkop ||
            !psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncClockSet) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "max clock must be less than 102.4Mhz.\r\n");
            _ErrorHandle(EINVAL);
            return  ((PVOID)LW_NULL);
        }
    }

    iError = __sdhciRegAccessDrvInit(psdhcihostattr);                   /*  寄存器访问驱动初始化        */
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "register access drv init failed.\r\n");
        return  ((PVOID)LW_NULL);
    }

    psdhcihost = (__PSDHCI_HOST)__SHEAP_ALLOC(sizeof(__SDHCI_HOST));   /*  分配HOST                     */
    if (!psdhcihost) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  ((PVOID)LW_NULL);
    }
    lib_bzero(psdhcihost, sizeof(__SDHCI_HOST));

    psdhcihost->SDHCIHS_uiVersion = SDHCI_READW(psdhcihostattr, SDHCI_HOST_VERSION);
    psdhcihost->SDHCIHS_uiVersion = (psdhcihost->SDHCIHS_uiVersion &
                                     SDHCI_HVER_SPEC_VER_MASK)     >>
                                     SDHCI_HVER_SPEC_VER_SHIFT;

    __sdhciHostCapDecode(psdhcihostattr, &psdhcihost->SDHCIHS_sdhcicap);/*  主控功能解析                */

    psdhcihost->SDHCIHS_sdfunc.SDFUNC_pfuncMasterXfer = __sdhciTransfer;
    psdhcihost->SDHCIHS_sdfunc.SDFUNC_pfuncMasterCtl  = __sdhciIoCtl;   /*  初始化驱动函数              */

    iError = API_SdAdapterCreate(pcAdapterName, &psdhcihost->SDHCIHS_sdfunc);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "create sd adapter error.\r\n");
        goto    __err0;
    }

    psdadapter = API_SdAdapterGet(pcAdapterName);                       /*  获得适配器                  */

    psdhcihost->SDHCIHS_psdadapter           = psdadapter;
    psdhcihost->SDHCIHS_atomicDevCnt.counter = 0;                       /*  初始设备数为0               */
    psdhcihost->SDHCIHS_pfuncMasterXfer      = __sdhciTransferNorm;
    psdhcihost->SDHCIHS_ucTransferMod        = SDHCIHOST_TMOD_SET_NORMAL;
    lib_memcpy(&psdhcihost->SDHCIHS_sdhcihostattr,
               psdhcihostattr, sizeof(LW_SDHCI_HOST_ATTR));
                                                                        /*  保存属性域                  */
    __sdhciIntDisAndEn(psdhcihost, SDHCI_INT_ALL_MASK, 0);              /*  禁止所有中断                */

    psdhcitrans = __sdhciTransNew(psdhcihost);
    if (!psdhcitrans) {
        goto    __err1;
    }
    psdhcihost->SDHCIHS_psdhcitrans = psdhcitrans;                      /*  创建传输事务                */

    psdhcisdmhost = __sdhciSdmHostNew(psdhcihost);
    if (!psdhcisdmhost) {
        goto    __err2;
    }
    psdhcihost->SDHCIHS_psdhcisdmhost = psdhcisdmhost;                  /*  创建 SDM 层 HOST 对象       */

    __sdhciHostReset(psdhcihost, SDHCI_SFRST_CMD | SDHCI_SFRST_DATA);

    if (!SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_DONOT_SET_VOLTAGE)) {
        UINT32 uiVoltage = psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_uiVoltage;
        if ((uiVoltage & __SDHCI_OCR_330) == __SDHCI_OCR_330) {
            __sdhciPowerSetVol(psdhcihost, SDHCI_POWCTL_330, LW_TRUE);
        } else if ((uiVoltage & __SDHCI_OCR_300) == __SDHCI_OCR_300) {
            __sdhciPowerSetVol(psdhcihost, SDHCI_POWCTL_300, LW_TRUE);
        } else if ((uiVoltage & __SDHCI_OCR_180) == __SDHCI_OCR_180) {
            __sdhciPowerSetVol(psdhcihost, SDHCI_POWCTL_180, LW_TRUE);
        } else {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "warning: host ocr info lost.\r\n");
        }
    }

    __sdhciTimeoutSet(psdhcihost);

    __sdhciIntDisAndEn(psdhcihost, SDHCI_INT_ALL_MASK | SDHCI_INT_CARD_INT, __SDHCI_INT_EN_MASK);

    return  ((PVOID)psdhcihost);

__err2:
    __sdhciTransDel(psdhcitrans);

__err1:
    API_SdAdapterDelete(pcAdapterName);

__err0:
    __SHEAP_FREE(psdhcihost);

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_SdhciSdmHostGet
** 功能描述: 获得 SDM Host 对象
** 输    入: pvHost    SDHCI 主控器指针
** 输    出: SDHCI 控制器对应的 SDM 控制器对象
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API PVOID   API_SdhciSdmHostGet (PVOID pvHost)
{
    __PSDHCI_HOST   psdhcihost;
    PVOID           pvSdmHost;

    if (!pvHost) {
        return  (LW_NULL);
    }

    psdhcihost = (__PSDHCI_HOST)pvHost;
    pvSdmHost  = psdhcihost->SDHCIHS_psdhcisdmhost->SDHCISDMH_pvSdmHost;

    return  (pvSdmHost);
}
/*********************************************************************************************************
** 函数名称: API_SdhciHostDelete
** 功能描述: 删除 SD 标准主控制器
** 输    入: pvHost    主控器指针
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdhciHostDelete (PVOID  pvHost)
{
    __PSDHCI_HOST    psdhcihost = LW_NULL;

    if (!pvHost) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdhcihost = (__PSDHCI_HOST)pvHost;
    if (API_AtomicGet(&psdhcihost->SDHCIHS_atomicDevCnt)) {
        _ErrorHandle(EBUSY);
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device always attached.\r\n");
        return  (PX_ERROR);
    }

   __sdhciSdmHostDel(psdhcihost->SDHCIHS_psdhcisdmhost);
   __sdhciTransDel(psdhcihost->SDHCIHS_psdhcitrans);

    API_SdAdapterDelete(__SDHCI_HOST_NAME(psdhcihost));
    __SHEAP_FREE(psdhcihost);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdhciHostTransferModGet
** 功能描述: 获得主控支持的传输模式
** 输    入: pvHost    主控器指针
** 输    出: 成功,返回传输模式支持情况; 失败, 返回 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdhciHostTransferModGet (PVOID    pvHost)
{
    __PSDHCI_HOST   psdhcihost = LW_NULL;
    INT             iTmodFlg   = SDHCIHOST_TMOD_CAN_NORMAL;             /*  总是支持一般传输            */

    if (!pvHost) {
        _ErrorHandle(EINVAL);
        return  (0);
    }

    psdhcihost = (__PSDHCI_HOST)pvHost;

    if (psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_bCanSdma) {
        iTmodFlg |= SDHCIHOST_TMOD_CAN_SDMA;
    }

    if (psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_bCanAdma) {
        iTmodFlg |= SDHCIHOST_TMOD_CAN_ADMA;
    }

    return  (iTmodFlg);
}
/*********************************************************************************************************
** 函数名称: API_SdhciHostTransferModSet
** 功能描述: 设置主控的传输模式
** 输    入: pvHost    主控器指针
**           iTmod     要设置的传输模式
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdhciHostTransferModSet (PVOID    pvHost, INT   iTmod)
{
    __PSDHCI_HOST    psdhcihost = LW_NULL;

    if (!pvHost) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdhcihost   = (__PSDHCI_HOST)pvHost;

    /*
     * 必须保证主控上没有设备, 才能更改主控传输模式
     */
    if (API_AtomicGet(&psdhcihost->SDHCIHS_atomicDevCnt)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device exist.\r\n");
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }

    switch (iTmod) {
    
    case SDHCIHOST_TMOD_SET_NORMAL:
        psdhcihost->SDHCIHS_ucTransferMod   = SDHCIHOST_TMOD_SET_NORMAL;
        psdhcihost->SDHCIHS_pfuncMasterXfer = __sdhciTransferNorm;
        break;

    case SDHCIHOST_TMOD_SET_SDMA:
        if (psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_bCanSdma) {
            psdhcihost->SDHCIHS_ucTransferMod   = SDHCIHOST_TMOD_SET_SDMA;
            psdhcihost->SDHCIHS_pfuncMasterXfer = __sdhciTransferSdma;
        } else {
            return  (PX_ERROR);
        }
        break;

    case SDHCIHOST_TMOD_SET_ADMA:
        if (psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_bCanAdma) {
            psdhcihost->SDHCIHS_ucTransferMod   = SDHCIHOST_TMOD_SET_ADMA;
            psdhcihost->SDHCIHS_pfuncMasterXfer = __sdhciTransferAdma;
        } else {
            return  (PX_ERROR);
        }
        break;

    default:
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciDeviceAdd
** 功能描述: 向SD标准主控制器上添加一个 SD 设备
** 输    入: pvHost      主控器指针
**           pcDevice    要添加的 SD 设备名称
** 输    出: 成功,返回设备指针.失败,返回 NULL.
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PVOID  __sdhciDeviceAdd (PVOID     pvHost,
                                CPCHAR    pcDeviceName)
{
    __PSDHCI_HOST   psdhcihost   = LW_NULL;
    __PSDHCI_DEVICE psdhcidevice = LW_NULL;
    PLW_SD_DEVICE   psddevice    = LW_NULL;

    if (!pvHost) {
        _ErrorHandle(EINVAL);
        return  ((PVOID)0);
    }

    psdhcihost = (__PSDHCI_HOST)pvHost;

    if (API_AtomicGet(&psdhcihost->SDHCIHS_atomicDevCnt) >= __SDHCI_HOST_DEVNUM_MAX) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "can not add more devices.\r\n");
        return  ((PVOID)0);
    }

    psddevice = API_SdDeviceGet(__SDHCI_HOST_NAME(psdhcihost), pcDeviceName);
    if (!psddevice) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "find sd device failed.\r\n");
       return  ((PVOID)0);
    }

    psdhcidevice = (__PSDHCI_DEVICE)__SHEAP_ALLOC(sizeof(__SDHCI_DEVICE));
    if (!psdhcidevice) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  ((PVOID)0);
    }

    lib_bzero(psdhcidevice, sizeof(__SDHCI_DEVICE));

    psdhcidevice->SDHCIDEV_psddevice  = psddevice;
    psdhcidevice->SDHCIDEV_psdhcihost = psdhcihost;

    psddevice->SDDEV_pvUsr            = psdhcidevice;                   /*  绑定用户数据                */
    psdhcihost->SDHCIHS_ucDevType     = psddevice->SDDEV_ucType;        /*  记录设备类型                */

    API_AtomicInc(&psdhcihost->SDHCIHS_atomicDevCnt);                   /*  设备++                      */

    return  ((PVOID)psdhcidevice);
}
/*********************************************************************************************************
** 函数名称: __sdhciDeviceRemove
** 功能描述: 从 SD 标准主控制器上移除一个 SD 设备
** 输    入: pvDevice  要移除的 SD 设备指针
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdhciDeviceRemove (PVOID pvDevice)
{
    __PSDHCI_HOST   psdhcihost   = LW_NULL;
    __PSDHCI_DEVICE psdhcidevice = LW_NULL;

    if (!pvDevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdhcidevice  = (__PSDHCI_DEVICE)pvDevice;
    psdhcihost    = psdhcidevice->SDHCIDEV_psdhcihost;

    psdhcidevice->SDHCIDEV_psddevice->SDDEV_pvUsr = LW_NULL;            /*  用户数据无效                */
    __SHEAP_FREE(psdhcidevice);
    API_AtomicDec(&psdhcihost->SDHCIHS_atomicDevCnt);                   /*  设备--                      */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdhciDeviceCheckNotify
** 功能描述: 用于通知 SDHCI HOST 进行一次设备检查
** 输    入: pvHost   控制器对象
**           iDevSta  设备状态
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API VOID  API_SdhciDeviceCheckNotify (PVOID pvHost, INT iDevSta)
{
    __PSDHCI_HOST     psdhcihost    = LW_NULL;
    __PSDHCI_SDM_HOST psdhcisdmhost = LW_NULL;

    if (!pvHost) {
        _ErrorHandle(EINVAL);
        return;
    }

    psdhcihost    = (__PSDHCI_HOST)pvHost;
    psdhcisdmhost = psdhcihost->SDHCIHS_psdhcisdmhost;

    if (psdhcisdmhost->SDHCISDMH_callbackChkDev) {
        psdhcisdmhost->SDHCISDMH_callbackChkDev(psdhcisdmhost->SDHCISDMH_pvCallBackArg, iDevSta);
    }
}
/*********************************************************************************************************
** 函数名称: API_SdhciDeviceUsageInc
** 功能描述: 控制器上的设备的使用计数增加一
** 输    入: pvHost   控制器对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdhciDeviceUsageInc (PVOID  pvHost)
{
    __PSDHCI_HOST   psdhcihost   = LW_NULL;
    __PSDHCI_DEVICE psdhcidevice = LW_NULL;
    INT             iError;

    if (!pvHost) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdhcihost   = (__PSDHCI_HOST)pvHost;
    psdhcidevice = (__PSDHCI_DEVICE)psdhcihost->SDHCIHS_psdhcisdmhost->SDHCISDMH_pvDevAttached;
    if (!psdhcidevice) {
        return  (PX_ERROR);
    }

    iError = API_SdDeviceUsageInc(psdhcidevice->SDHCIDEV_psddevice);

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_SdhciDeviceUsageDec
** 功能描述: 控制器上的设备的使用计数减一
** 输    入: pvHost   控制器对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdhciDeviceUsageDec (PVOID  pvHost)
{
    __PSDHCI_HOST   psdhcihost   = LW_NULL;
    __PSDHCI_DEVICE psdhcidevice = LW_NULL;
    INT             iError;

    if (!pvHost) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdhcihost   = (__PSDHCI_HOST)pvHost;
    psdhcidevice = (__PSDHCI_DEVICE)psdhcihost->SDHCIHS_psdhcisdmhost->SDHCISDMH_pvDevAttached;
    if (!psdhcidevice) {
        return  (PX_ERROR);
    }

    iError = API_SdDeviceUsageDec(psdhcidevice->SDHCIDEV_psddevice);

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_SdhciDeviceUsageGet
** 功能描述: 获得控制器上的设备的使用计数
** 输    入: pvHost   控制器对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdhciDeviceUsageGet (PVOID  pvHost)
{
    __PSDHCI_HOST   psdhcihost   = LW_NULL;
    __PSDHCI_DEVICE psdhcidevice = LW_NULL;
    INT             iError;

    if (!pvHost) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdhcihost   = (__PSDHCI_HOST)pvHost;
    psdhcidevice = (__PSDHCI_DEVICE)psdhcihost->SDHCIHS_psdhcisdmhost->SDHCISDMH_pvDevAttached;
    if (!psdhcidevice) {
        return  (PX_ERROR);
    }

    iError = API_SdDeviceUsageGet(psdhcidevice->SDHCIDEV_psddevice);

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __sdhciTransferNorm
** 功能描述: 一般传输
** 输    入: psdhcihost  SD 主控制器
**           psddev      SD 设备
**           psdmsg      SD 传输控制消息组
**           iNum        消息个数
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdhciTransferNorm (__PSDHCI_HOST       psdhcihost,
                                PLW_SD_DEVICE       psddev,
                                PLW_SD_MESSAGE      psdmsg,
                                INT                 iNum)
{
    __PSDHCI_TRANS  psdhcitrans;
    INT             iError;
    INT             i = 0;

    psdhcitrans = psdhcihost->SDHCIHS_psdhcitrans;

    while ((i < iNum) && (psdmsg != LW_NULL)) {
        __SDHCI_TRANS_LOCK(psdhcitrans);                                /*  锁定传输                    */
        iError = __sdhciTransPrepare(psdhcitrans, psdmsg, __SDHIC_TRANS_NORMAL);
        if (iError != ERROR_NONE) {
            __SDHCI_TRANS_UNLOCK(psdhcitrans);
            return  (PX_ERROR);
        }

        __SDHCI_TRANS_UNLOCK(psdhcitrans);

        iError = __sdhciTransStart(psdhcitrans);
        if (iError != ERROR_NONE) {
            return  (PX_ERROR);
        }

        iError = __sdhciTransFinishWait(psdhcitrans);                   /*  等待本次传输完成            */
        if (iError != ERROR_NONE) {
            return  (PX_ERROR);
        }

        __SDHCI_TRANS_LOCK(psdhcitrans);                                /*  锁定传输                    */
        __sdhciTransClean(psdhcitrans);                                 /*  清理本次传输                */
        if (psdhcitrans->SDHCITS_iCmdError != ERROR_NONE) {
            __SDHCI_TRANS_UNLOCK(psdhcitrans);
            return  (PX_ERROR);
        }

        if (psdhcitrans->SDHCITS_iDatError != ERROR_NONE) {
            __SDHCI_TRANS_UNLOCK(psdhcitrans);
            return  (PX_ERROR);
        }
        __SDHCI_TRANS_UNLOCK(psdhcitrans);                              /*  解锁传输                    */

        i++;
        psdmsg++;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciTransferSdma
** 功能描述: SDMA 传输
** 输    入: psdhcihost  SD 主控制器
**           psddev      SD 设备
**           psdmsg      SD 传输控制消息组
**           iNum        消息个数
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdhciTransferSdma (__PSDHCI_HOST       psdhcihost,
                                PLW_SD_DEVICE       psddev,
                                PLW_SD_MESSAGE      psdmsg,
                                INT                 iNum)
{
    __PSDHCI_TRANS  psdhcitrans;
    INT             iError;
    INT             i = 0;

    psdhcitrans = psdhcihost->SDHCIHS_psdhcitrans;

    while ((i < iNum) && (psdmsg != LW_NULL)) {
        __SDHCI_TRANS_LOCK(psdhcitrans);                                /*  锁定传输                    */
        iError = __sdhciTransPrepare(psdhcitrans, psdmsg, __SDHIC_TRANS_SDMA);
        if (iError != ERROR_NONE) {
            __SDHCI_TRANS_UNLOCK(psdhcitrans);
            return  (PX_ERROR);
        }
        iError = __sdhciTransStart(psdhcitrans);
        if (iError != ERROR_NONE) {
            __SDHCI_TRANS_UNLOCK(psdhcitrans);
            return  (PX_ERROR);
        }
        __SDHCI_TRANS_UNLOCK(psdhcitrans);                              /*  解锁传输                    */

        iError = __sdhciTransFinishWait(psdhcitrans);                   /*  等待本次传输完成            */
        if (iError != ERROR_NONE) {
            return  (PX_ERROR);
        }

        __SDHCI_TRANS_LOCK(psdhcitrans);                                /*  锁定传输                    */
        __sdhciTransClean(psdhcitrans);                                 /*  清理本次传输                */
        if (psdhcitrans->SDHCITS_iCmdError != ERROR_NONE) {
            __SDHCI_TRANS_UNLOCK(psdhcitrans);
            return  (PX_ERROR);
        }

        if (psdhcitrans->SDHCITS_iDatError != ERROR_NONE) {
            __SDHCI_TRANS_UNLOCK(psdhcitrans);
            return  (PX_ERROR);
        }
        __SDHCI_TRANS_UNLOCK(psdhcitrans);                              /*  解锁传输                    */

        i++;
        psdmsg++;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciTransferAdma
** 功能描述: ADMA传输
** 输    入: psdadapter  SD 主控制器
**           psddev      SD 设备
**           psdmsg      SD 传输控制消息组
**           iNum        消息个数
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdhciTransferAdma (__PSDHCI_HOST       psdhcihost,
                                PLW_SD_DEVICE       psddev,
                                PLW_SD_MESSAGE      psdmsg,
                                INT                 iNum)
{
    /*
     *  TODO: adma mode support.
     */
    __sdhciDataPrepareAdma(LW_NULL);
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __sdhciTransfer
** 功能描述: 总线传输函数
** 输    入: psdadapter  SD 总线适配器
**           iCmd        控制命令
**           lArg        参数
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdhciTransfer (PLW_SD_ADAPTER      psdadapter,
                            PLW_SD_DEVICE       psddev,
                            PLW_SD_MESSAGE      psdmsg,
                            INT                 iNum)
{
    __PSDHCI_HOST  psdhcihost = LW_NULL;
    INT            iError     = ERROR_NONE;

    if (!psdadapter) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdhcihost = (__PSDHCI_HOST)psdadapter->SDADAPTER_psdfunc;
    if (!psdhcihost) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "no sdhci host.\r\n");
        return  (PX_ERROR);
    }

    /*
     * 执行主控上对应的总线传输函数
     */
    iError = psdhcihost->SDHCIHS_pfuncMasterXfer(psdhcihost, psddev, psdmsg, iNum);

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __sdhciIoCtl
** 功能描述: SD 主控器IO控制函数
** 输    入: psdadapter  SD 总线适配器
**           iCmd        控制命令
**           lArg        参数
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdhciIoCtl (PLW_SD_ADAPTER  psdadapter,
                         INT             iCmd,
                         LONG            lArg)
{
    __PSDHCI_HOST  psdhcihost = LW_NULL;
    INT            iError     = ERROR_NONE;

    if (!psdadapter) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdhcihost = (__PSDHCI_HOST)psdadapter->SDADAPTER_psdfunc;
    if (!psdhcihost) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "no sdhci host.\r\n");
        return  (PX_ERROR);
    }

    switch (iCmd) {
    
    case SDBUS_CTRL_POWEROFF:
        iError = __sdhciPowerOff(psdhcihost);
        break;

    case SDBUS_CTRL_POWERUP:
    case SDBUS_CTRL_POWERON:
        iError = __sdhciPowerOn(psdhcihost);
        break;

    case SDBUS_CTRL_SETBUSWIDTH:
        iError = __sdhciBusWidthSet(psdhcihost, (UINT32)lArg);
        break;

    case SDBUS_CTRL_SETCLK:
        if (lArg == SDARG_SETCLK_MAX) {
            iError = __sdhciHighSpeedEn(psdhcihost, LW_TRUE);
            if (iError != ERROR_NONE) {
                break;
            }
        } else {
            __sdhciHighSpeedEn(psdhcihost, LW_FALSE);
        }
        iError = __sdhciClockSet(psdhcihost, (UINT32)lArg);
        break;

    case SDBUS_CTRL_STOPCLK:
        iError = __sdhciClockStop(psdhcihost);
        break;

    case SDBUS_CTRL_STARTCLK:
        iError = __sdhciClockSet(psdhcihost, psdhcihost->SDHCIHS_uiClkCurr);
        break;

    case SDBUS_CTRL_DELAYCLK:
        break;

    case SDBUS_CTRL_GETOCR:
        *(UINT32 *)lArg = psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_uiVoltage;
        iError = ERROR_NONE;
        break;

    default:
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "invalidate command.\r\n");
        iError = PX_ERROR;
        break;
    }

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __sdhciHostCapDecode
** 功能描述: 解码主控的功能寄存器
** 输    入: psdhcihostattr  主控属性
             psdhcicap       功能结构指针
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdhciHostCapDecode (PLW_SDHCI_HOST_ATTR psdhcihostattr, __PSDHCI_CAPAB psdhcicap)
{
    UINT32  uiCapReg = 0;
    UINT32  uiTmp    = 0;

    if (!psdhcicap) {
        _ErrorHandle(EINVAL);
        return;
    }

    uiCapReg = SDHCI_READL(psdhcihostattr, SDHCI_CAPABILITIES);

    psdhcicap->SDHCICAP_uiBaseClkFreq  = (uiCapReg & SDHCI_CAP_BASECLK_MASK) >> SDHCI_CAP_BASECLK_SHIFT;
    psdhcicap->SDHCICAP_uiMaxBlkSize   = (uiCapReg & SDHCI_CAP_MAXBLK_MASK) >> SDHCI_CAP_MAXBLK_SHIFT;
    psdhcicap->SDHCICAP_bCanSdma       = (uiCapReg & SDHCI_CAP_CAN_DO_SDMA) ? LW_TRUE : LW_FALSE;
    psdhcicap->SDHCICAP_bCanAdma       = (uiCapReg & SDHCI_CAP_CAN_DO_ADMA) ? LW_TRUE : LW_FALSE;
    psdhcicap->SDHCICAP_bCanHighSpeed  = (uiCapReg & SDHCI_CAP_CAN_DO_HISPD) ? LW_TRUE : LW_FALSE;
    psdhcicap->SDHCICAP_bCanSusRes     = (uiCapReg & SDHCI_CAP_CAN_DO_SUSRES) ? LW_TRUE : LW_FALSE;

    psdhcicap->SDHCICAP_uiBaseClkFreq *= 1000000;                       /*  转为实际频率                */
    psdhcicap->SDHCICAP_uiMaxBlkSize   = 512 << psdhcicap->SDHCICAP_uiMaxBlkSize;
                                                                        /*  转换为实际最大块大小        */
    /*
     * 如果在此寄存器里的时钟为0, 则说明主控器没有自己内部时钟,
     * 而是来源于其他时钟源. 因此采用另外的时钟源作为基时钟.
     */
    if (psdhcicap->SDHCICAP_uiBaseClkFreq == 0) {
        uiTmp = psdhcihostattr->SDHCIHOST_uiMaxClock;
        if (uiTmp == 0) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "sdhci without clock source .\r\n");
            return;
        }
        psdhcicap->SDHCICAP_uiBaseClkFreq = uiTmp;
    }

    uiTmp = 0;
    if ((uiCapReg & SDHCI_CAP_CAN_VDD_330) != 0) {
        uiTmp |= __SDHCI_OCR_330;
    }
    if ((uiCapReg & SDHCI_CAP_CAN_VDD_300) != 0) {
        uiTmp |= __SDHCI_OCR_300;
    }
    if ((uiCapReg & SDHCI_CAP_CAN_VDD_180) != 0) {
        uiTmp |= __SDHCI_OCR_180;
    }

    if (uiTmp == 0) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "sdhci without voltage information.\r\n");
        return;
    }
    psdhcicap->SDHCICAP_uiVoltage = uiTmp;

#ifdef  __SYLIXOS_DEBUG
#ifndef printk
#define printk
#endif                                                                  /*  printk                      */
    printk("\nhost capablity >>\n");
    printk("uiCapReg       : %08x\n", uiCapReg);
    printk("base clock     : %u\n", psdhcicap->SDHCICAP_uiBaseClkFreq);
    printk("max block size : %u\n", psdhcicap->SDHCICAP_uiMaxBlkSize);
    printk("can sdma       : %s\n", psdhcicap->SDHCICAP_bCanSdma ? "yes" : "no");
    printk("can adma       : %s\n", psdhcicap->SDHCICAP_bCanAdma ? "yes" : "no");
    printk("can high speed : %s\n", psdhcicap->SDHCICAP_bCanHighSpeed ? "yes" : "no");
    printk("voltage support: %08x\n", psdhcicap->SDHCICAP_uiVoltage);
#endif                                                                  /*  __SYLIXOS_DEBUG             */
}
/*********************************************************************************************************
** 函数名称: __sdhciClockSet
** 功能描述: 时钟频率设置并使能
** 输    入: psdhcihost      SDHCI HOST 结构指针
**           uiSetClk        要设置的时钟频率
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdhciClockSet (__PSDHCI_HOST  psdhcihost, UINT32 uiSetClk)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    UINT32              uiBaseClk      = psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_uiBaseClkFreq;

    UINT16              usDivClk       = 0;
    UINT                uiTimeout      = 30;

    if ((uiSetClk == psdhcihost->SDHCIHS_uiClkCurr) &&
        (psdhcihost->SDHCIHS_bClkEnable)) {
        return  (ERROR_NONE);
    }

    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncClockSet) {
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncClockSet(psdhcihostattr, uiSetClk);
        goto __ret;
    }

    if (uiSetClk > uiBaseClk) {
        uiSetClk = uiBaseClk;
    }

    SDHCI_WRITEW(psdhcihostattr, SDHCI_CLOCK_CONTROL, 0);               /*  禁止时钟模块所有内部功能    */

    /*
     * 计算分频因子
     */
    if (uiSetClk < uiBaseClk) {
        usDivClk = (UINT16)((uiBaseClk + uiSetClk - 1) / uiSetClk);
        if (usDivClk <= 2) {
            usDivClk = 1 << 0;
        } else if (usDivClk <= 4) {
            usDivClk = 1 << 1;
        } else if (usDivClk <= 8) {
            usDivClk = 1 << 2;
        } else if (usDivClk <= 16) {
            usDivClk = 1 << 3;
        } else if (usDivClk <= 32) {
            usDivClk = 1 << 4;
        } else if (usDivClk <= 64) {
            usDivClk = 1 << 5;
        } else if (usDivClk <= 128) {
            usDivClk = 1 << 6;
        } else {
            usDivClk = 1 << 7;
        }
    }

#ifdef __SYLIXOS_DEBUG
#ifndef printk
#define printk
#endif                                                                  /*  printk                      */
        printk("sdhci current clock: %uHz\n", uiBaseClk / (usDivClk << 1));
#endif                                                                  /*  __SYLIXOS_DEBUG             */

    usDivClk <<= SDHCI_CLKCTL_DIVIDER_SHIFT;
    usDivClk  |= SDHCI_CLKCTL_INTER_EN;                                 /*  内部时钟使能                */
    SDHCI_WRITEW(psdhcihostattr, SDHCI_CLOCK_CONTROL, usDivClk);

    /*
     * 等待时钟稳定
     */
    while (1) {
        usDivClk = SDHCI_READW(psdhcihostattr, SDHCI_CLOCK_CONTROL);
        if (usDivClk & SDHCI_CLKCTL_INTER_STABLE) {
            break;
        }

        uiTimeout--;
        if (uiTimeout == 0) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "wait internal clock to be stable timeout.\r\n");
            return  (PX_ERROR);
        }
        SDHCI_DELAYMS(1);
    }

    usDivClk |= SDHCI_CLKCTL_CLOCK_EN;                                  /*  SDCLK 设备时钟使能          */
    SDHCI_WRITEW(psdhcihostattr, SDHCI_CLOCK_CONTROL, usDivClk);

__ret:
    psdhcihost->SDHCIHS_uiClkCurr  = uiSetClk;
    psdhcihost->SDHCIHS_bClkEnable = LW_TRUE;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciClockStop
** 功能描述: 停止时钟输出
** 输    入: psdhcihost      SDHCI HOST 结构指针
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdhciClockStop (__PSDHCI_HOST    psdhcihost)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    UINT32              uiData;

    if (!psdhcihost->SDHCIHS_bClkEnable) {
        return  (ERROR_NONE);
    }

    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncClockStop) {
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncClockStop(psdhcihostattr);
        goto __ret;
    }

    uiData = SDHCI_READW(psdhcihostattr, SDHCI_CLOCK_CONTROL) & (~SDHCI_CLKCTL_CLOCK_EN);
    SDHCI_WRITEW(psdhcihostattr, SDHCI_CLOCK_CONTROL, uiData);

__ret:
    psdhcihost->SDHCIHS_bClkEnable = LW_FALSE;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciBusWidthSet
** 功能描述: 主控总线位宽设置
** 输    入: psdhcihost      SDHCI HOST 结构指针
**           uiBusWidth      要设置的总线位宽
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdhciBusWidthSet (__PSDHCI_HOST  psdhcihost, UINT32 uiBusWidth)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    BOOL                bSdioIntEn     = psdhcihost->SDHCIHS_bSdioIntEnable;
    UINT8               ucCtl;

    if (!SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_CANNOT_SDIO_INT) &&
        !SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_SDIO_INT_OOB)) {
        __sdhciSdioIntEn(psdhcihost, LW_FALSE);                         /*  禁止卡中断                  */
    }

    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncBusWidthSet) {
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncBusWidthSet(psdhcihostattr, uiBusWidth);
        goto __ret;
    }

    ucCtl  = SDHCI_READB(psdhcihostattr, SDHCI_HOST_CONTROL);

    switch (uiBusWidth) {
    case SDARG_SETBUSWIDTH_1:
        ucCtl &= ~SDHCI_HCTRL_8BITBUS;
        ucCtl &= ~SDHCI_HCTRL_4BITBUS;
        break;

    case SDARG_SETBUSWIDTH_4:
        ucCtl &= ~SDHCI_HCTRL_8BITBUS;
        ucCtl |= SDHCI_HCTRL_4BITBUS;
        break;

    case SDARG_SETBUSWIDTH_8:
        if (psdhcihost->SDHCIHS_uiVersion > SDHCI_HVER_SPEC_200) {
            ucCtl &= ~SDHCI_HCTRL_4BITBUS;
            ucCtl |= SDHCI_HCTRL_8BITBUS;
        } else {
            SDCARD_DEBUG_MSG(__LOGMESSAGE_LEVEL, "sdhci host can't support 8-bit width.\r\n");
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        break;

    default:
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter of bus width error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    SDHCI_WRITEB(psdhcihostattr, SDHCI_HOST_CONTROL, ucCtl);

__ret:
    if (!SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_CANNOT_SDIO_INT) &&
        !SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_SDIO_INT_OOB)) {
        __sdhciSdioIntEn(psdhcihost, bSdioIntEn);                         /*  恢复之前的卡中断设置      */
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciPowerOn
** 功能描述: 打开电源
** 输    入: psdhcihost     SDHCI HOST 结构指针
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdhciPowerOn (__PSDHCI_HOST  psdhcihost)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    UINT8               ucPow;

    /*
     * 平台相关硬件复位
     */
    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncHwReset) {
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncHwReset(psdhcihostattr);
    }

    /*
     * 控制器标准复位操作
     */
    if (!SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_DONOT_SET_POWER)) {
        ucPow  = SDHCI_READB(psdhcihostattr, SDHCI_POWER_CONTROL);
        ucPow |= SDHCI_POWCTL_ON;
        SDHCI_WRITEB(psdhcihostattr, SDHCI_POWER_CONTROL, ucPow);

        if (SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_DO_RESET_AFTER_SET_POWER_ON)) {
            __sdhciHostReset(psdhcihost, SDHCI_SFRST_CMD);
            __sdhciHostReset(psdhcihost, SDHCI_SFRST_DATA);
        }
    }

    /*
     * 额外电源设置与控制器本身无关
     */
    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncExtraPowerSet) {
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncExtraPowerSet(psdhcihostattr, LW_TRUE);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciPowerff
** 功能描述: 关闭电源
** 输    入: psdhcihost       SDHCI HOST 结构指针
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdhciPowerOff (__PSDHCI_HOST  psdhcihost)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    UINT8               ucPow;

    if (!SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_DONOT_SET_POWER)) {
        ucPow  = SDHCI_READB(psdhcihostattr, SDHCI_POWER_CONTROL);
        ucPow &= ~SDHCI_POWCTL_ON;
        SDHCI_WRITEB(psdhcihostattr, SDHCI_POWER_CONTROL, ucPow);
    }

    /*
     * 额外电源设置与控制器本身无关
     */
    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncExtraPowerSet) {
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncExtraPowerSet(psdhcihostattr, LW_FALSE);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciPowerSetVol
** 功能描述: 电源设置电压
** 输    入: psdhcihost      SDHCI HOST 结构指针
**           ucVol           电压
**           bIsOn           设置之后电源是否开启
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdhciPowerSetVol (__PSDHCI_HOST  psdhcihost,
                               UINT8          ucVol,
                               BOOL           bIsOn)
{
    if (bIsOn) {
        ucVol |= SDHCI_POWCTL_ON;
    } else {
        ucVol &= ~SDHCI_POWCTL_ON;
    }

    __sdhciPowerOff(psdhcihost);

    SDHCI_WRITEB(&psdhcihost->SDHCIHS_sdhcihostattr, SDHCI_POWER_CONTROL, ucVol);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciHighSpeedEn
** 功能描述: 主控高速设置
** 输    入: psdhcihost      SDHCI HOST 结构指针
**           bEnable         是否使能高速
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdhciHighSpeedEn (__PSDHCI_HOST  psdhcihost,  BOOL bEnable)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    UINT8               ucDat;

    ucDat  = SDHCI_READB(psdhcihostattr, SDHCI_HOST_CONTROL);
    if (bEnable) {
        if (!psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_bCanHighSpeed) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "host don't suport highspeed mode.\r\n");
            return  (PX_ERROR);
        }
        ucDat |= SDHCI_HCTRL_HISPD;
    } else {
        ucDat &= ~SDHCI_HCTRL_HISPD;
    }

    SDHCI_WRITEB(psdhcihostattr, SDHCI_HOST_CONTROL, ucDat);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciSdmHostNew
** 功能描述: 创建并初始化一个 SDM 规定的控制器对象
** 输    入: psdhcihost    SDHCI 控制器内部结构对象
** 输    出: 对象指针
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static __SDHCI_SDM_HOST *__sdhciSdmHostNew (__PSDHCI_HOST   psdhcihost)
{
    __SDHCI_SDM_HOST *psdhcisdmhost;
    SD_HOST          *psdhost;
    INT               iCapablity;
    PVOID             pvSdmHost;

    if (!psdhcihost) {
        return  (LW_NULL);
    }

    psdhcisdmhost = (__SDHCI_SDM_HOST *)__SHEAP_ALLOC(sizeof(__SDHCI_SDM_HOST));
    if (!psdhcisdmhost) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        return  (LW_NULL);
    }
    lib_bzero(psdhcisdmhost, sizeof(__SDHCI_SDM_HOST));

    iCapablity = SDHOST_CAP_DATA_4BIT;

    if (SDHCI_QUIRK_FLG(&psdhcihost->SDHCIHS_sdhcihostattr,
                        SDHCI_QUIRK_FLG_CAN_DATA_8BIT)) {
        iCapablity |= SDHOST_CAP_DATA_8BIT;
    }
    if (SDHCI_QUIRK_FLG(&psdhcihost->SDHCIHS_sdhcihostattr,
                        SDHCI_QUIRK_FLG_CAN_DATA_4BIT_DDR)) {
        iCapablity |= SDHOST_CAP_DATA_4BIT_DDR;
    }
    if (SDHCI_QUIRK_FLG(&psdhcihost->SDHCIHS_sdhcihostattr,
                        SDHCI_QUIRK_FLG_CAN_DATA_8BIT_DDR)) {
        iCapablity |= SDHOST_CAP_DATA_8BIT_DDR;
    }
    if (SDHCI_QUIRK_FLG(&psdhcihost->SDHCIHS_sdhcihostattr,
                        SDHCI_QUIRK_FLG_MMC_FORCE_1BIT)) {
        iCapablity |= SDHOST_CAP_MMC_FORCE_1BIT;
    }
    if (psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_bCanHighSpeed) {
        iCapablity |= SDHOST_CAP_HIGHSPEED;
    }
    if (SDHCI_QUIRK_FLG(&psdhcihost->SDHCIHS_sdhcihostattr,
                        SDHCI_QUIRK_FLG_CANNOT_HIGHSPEED)) {
        iCapablity &= ~SDHOST_CAP_HIGHSPEED;
    }
    if (SDHCI_QUIRK_FLG(&psdhcihost->SDHCIHS_sdhcihostattr,
                        SDHCI_QUIRK_FLG_SDIO_FORCE_1BIT)) {
        iCapablity |= SDHOST_CAP_SDIO_FORCE_1BIT;
    }

    psdhost = &psdhcisdmhost->SDHCISDMH_sdhost;

    psdhost->SDHOST_cpcName                = __SDHCI_HOST_NAME(psdhcihost);
    psdhost->SDHOST_iType                  = SDHOST_TYPE_SD;
    psdhost->SDHOST_iCapbility             = iCapablity;
    psdhost->SDHOST_pfuncCallbackInstall   = __sdhciSdmCallBackInstall;
    psdhost->SDHOST_pfuncCallbackUnInstall = __sdhciSdmCallBackUnInstall;
    psdhost->SDHOST_pfuncSdioIntEn         = __sdhciSdmSdioIntEn;
    psdhost->SDHOST_pfuncIsCardWp          = __sdhciSdmIsCardWp;
    psdhost->SDHOST_pfuncDevAttach         = __sdhciSdmDevAttach;
    psdhost->SDHOST_pfuncDevDetach         = __sdhciSdmDevDetach;

    psdhcisdmhost->SDHCISDMH_psdhcihost    = psdhcihost;
    psdhcihost->SDHCIHS_psdhcisdmhost      = psdhcisdmhost;

    pvSdmHost = API_SdmHostRegister(psdhost);
    if (!pvSdmHost) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "can't register into sdm modules.\r\n");
        __SHEAP_FREE(psdhcisdmhost);
        return  (LW_NULL);

    }
    psdhcisdmhost->SDHCISDMH_pvSdmHost = pvSdmHost;

    return  (psdhcisdmhost);
}
/*********************************************************************************************************
** 函数名称: __sdhciSdmHostDel
** 功能描述: 清理 SDM 规定的控制器对象
** 输    入: psdhcisdmhost    SDM 层规定的 HOST 信息结构
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdhciSdmHostDel ( __SDHCI_SDM_HOST  *psdhcisdmhost)
{
    PVOID  pvSdmHost;

    pvSdmHost = psdhcisdmhost->SDHCISDMH_pvSdmHost;
    if (API_SdmHostUnRegister(pvSdmHost) != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "can't unregister from sdm modules.\r\n");
        return  (PX_ERROR);
    }

    __SHEAP_FREE(psdhcisdmhost);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciSdmCallBackInstall
** 功能描述: 安装回调函数
** 输    入: psdhost          SDM 层规定的 HOST 信息结构
**           iCallbackType    回调函数类型
**           callback         回调函数指针
**           pvCallbackArg    回调函数参数
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdhciSdmCallBackInstall (SD_HOST       *psdhost,
                                       INT            iCallbackType,
                                       SD_CALLBACK    callback,
                                       PVOID          pvCallbackArg)
{
    __SDHCI_SDM_HOST *psdhcisdmhost = (__SDHCI_SDM_HOST *)psdhost;
    if (!psdhcisdmhost) {
        return  (PX_ERROR);
    }

    if (iCallbackType == SDHOST_CALLBACK_CHECK_DEV) {
        psdhcisdmhost->SDHCISDMH_callbackChkDev = callback;
        psdhcisdmhost->SDHCISDMH_pvCallBackArg  = pvCallbackArg;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciSdmCallBackUnInstall
** 功能描述: 注销安装的回调函数
** 输    入: psdhost          SDM 层规定的 HOST 信息结构
**           iCallbackType    回调函数类型
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdhciSdmCallBackUnInstall (SD_HOST    *psdhost,
                                         INT         iCallbackType)
{
    __SDHCI_SDM_HOST *psdhcisdmhost = (__SDHCI_SDM_HOST *)psdhost;
    if (!psdhcisdmhost) {
        return  (PX_ERROR);
    }

    if (iCallbackType == SDHOST_CALLBACK_CHECK_DEV) {
        psdhcisdmhost->SDHCISDMH_callbackChkDev = LW_NULL;
        psdhcisdmhost->SDHCISDMH_pvCallBackArg  = LW_NULL;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciSdmIsCardWp
** 功能描述: 判断该 HOST 上对应的卡是否写保护
** 输    入: psdhost          SDM 层规定的 HOST 信息结构
** 输    出: LW_TRUE: 卡写保护    LW_FALSE: 卡未写保护
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL __sdhciSdmIsCardWp (SD_HOST *psdhost)
{
    __SDHCI_SDM_HOST   *psdhcisdmhost  = (__SDHCI_SDM_HOST *)psdhost;
    LW_SDHCI_HOST_ATTR *psdhcihostattr = LW_NULL;
    BOOL                bIsWp          = LW_FALSE;

    if (!psdhcisdmhost) {
        return  (LW_FALSE);
    }

    psdhcihostattr = &psdhcisdmhost->SDHCISDMH_psdhcihost->SDHCIHS_sdhcihostattr;
    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncIsCardWp) {
        bIsWp = psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncIsCardWp(psdhcihostattr);
    }

    return  (bIsWp);
}
/*********************************************************************************************************
** 函数名称: __sdhciSdmSdioIntEn
** 功能描述: 使能 SDIO 中断
** 输    入: psdhost          SDM 层规定的 HOST 信息结构
**           bEnable          是否使能
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdhciSdmSdioIntEn (SD_HOST *psdhost,  BOOL bEnable)
{
    __SDHCI_SDM_HOST    *psdhcisdmhost  = (__SDHCI_SDM_HOST *)psdhost;
    __SDHCI_HOST        *psdhcihost     = psdhcisdmhost->SDHCISDMH_psdhcihost;
    LW_SDHCI_HOST_ATTR  *psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;

    if (!psdhcisdmhost) {
        return;
    }

    psdhcihost     = psdhcisdmhost->SDHCISDMH_psdhcihost;
    psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;

    if(SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_SDIO_INT_OOB)) {
        if (psdhcihostattr->SDHCIHOST_pquirkop &&
            psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncOOBInterSet) {
            psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncOOBInterSet(psdhcihostattr, bEnable);
        }
        return;
    }

    if (SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_CANNOT_SDIO_INT)) {
        if (psdhcihost->SDHCIHS_bSdioIntEnable == bEnable) {
            return;
        }
        if (bEnable) {
            __SDHCI_SDIO_ENABLE(psdhcihost->SDHCIHS_psdhcitrans);
        } else {
            __SDHCI_SDIO_DISABLE(psdhcihost->SDHCIHS_psdhcitrans);
        }
        psdhcihost->SDHCIHS_bSdioIntEnable = bEnable;

    } else {
        __sdhciSdioIntEn(psdhcihost, bEnable);
    }
}
/*********************************************************************************************************
** 函数名称: __sdhciSdmDevAttach
** 功能描述: 添加设备
** 输    入: psdhost          SDM 层规定的 HOST 信息结构
**           cpcDevName       设备的名称
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __sdhciSdmDevAttach (SD_HOST *psdhost, CPCHAR cpcDevName)
{
    __SDHCI_SDM_HOST *psdhcisdmhost = (__SDHCI_SDM_HOST *)psdhost;
    PVOID             pvDevAttached;

    if (!psdhcisdmhost) {
        return;
    }

    pvDevAttached = __sdhciDeviceAdd(psdhcisdmhost->SDHCISDMH_psdhcihost, cpcDevName);
    if (!pvDevAttached) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device attached failed.\r\n");
        return;
    }

    psdhcisdmhost->SDHCISDMH_pvDevAttached = pvDevAttached;
}
/*********************************************************************************************************
** 函数名称: __sdhciSdmDevDetach
** 功能描述: 删除设备
** 输    入: psdhost          SDM 层规定的 HOST 信息结构
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __sdhciSdmDevDetach (SD_HOST *psdhost)
{
    __SDHCI_SDM_HOST *psdhcisdmhost = (__SDHCI_SDM_HOST *)psdhost;

    if (!psdhcisdmhost) {
        return;
    }

    __sdhciDeviceRemove(psdhcisdmhost->SDHCISDMH_pvDevAttached);
}
/*********************************************************************************************************
** 函数名称: __sdhciCmdSend
** 功能描述: 命令发送
** 输    入: psdhcihost            SDHCI HOST 结构指针
**           psdcmd                命令结构指针
**           psddat                数据传输控制结构
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdhciCmdSend (__PSDHCI_HOST   psdhcihost,
                            PLW_SD_COMMAND  psdcmd,
                            PLW_SD_DATA     psddat)
{
    PLW_SDHCI_HOST_ATTR   psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    __PSDHCI_TRANS        psdhcitrans    = psdhcihost->SDHCIHS_psdhcitrans;
    UINT32                uiMask;
    UINT                  uiTimeout;
    INT                   iCmdFlg;

    struct timespec       tvOld;
    struct timespec       tvNow;

    if (!SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_DONOT_CHECK_BUSY_BEFORE_CMD_SEND)) {
        uiMask = SDHCI_PSTA_CMD_INHIBIT;
        if (psddat || SD_CMD_TEST_RSP(psdcmd, SD_RSP_BUSY)) {
            uiMask |= SDHCI_PSTA_DATA_INHIBIT;
        }

        if (psdcmd == psdhcitrans->SDHCITS_psdcmdStop) {
            uiMask &= ~SDHCI_PSTA_DATA_INHIBIT;
        }

        /*
         * 等待命令(数据)线空闲
         */
        uiTimeout = 0;
        lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);
        
        while (SDHCI_READL(psdhcihostattr, SDHCI_PRESENT_STATE) & uiMask) {
            uiTimeout++;
            if (uiTimeout > __SDHCI_CMD_RETRY) {
                goto    __timeout;
            }

            lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
            if ((tvNow.tv_sec - tvOld.tv_sec) >= __SDHCI_CMD_TOUT_SEC) {
                goto    __timeout;
            }
        }
    }

    if ((psdcmd->SDCMD_uiFlag & SD_RSP_BUSY) || psddat) {
        __sdhciTimeoutSet(psdhcihost);
    }

    /*
     * 写参数寄存器
     */
    SDHCI_WRITEL(psdhcihostattr, SDHCI_ARGUMENT, psdcmd->SDCMD_uiArg);

    /*
     * 设置传输模式
     */
    if (psddat) {
        __sdhciTransferModSet(psdhcihost);
    } else {
        uiMask = SDHCI_READW(psdhcihostattr, SDHCI_TRANSFER_MODE);
        uiMask &=  ~(SDHCI_TRNS_ACMD12 | SDHCI_TRNS_ACMD23);
        SDHCI_WRITEW(psdhcihostattr, SDHCI_TRANSFER_MODE, uiMask);
    }

    if ((psdcmd->SDCMD_uiFlag & SD_RSP_136) && (psdcmd->SDCMD_uiFlag & SD_RSP_BUSY)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "response type unavailable.\r\n");
        psdhcitrans->SDHCITS_iCmdError = PX_ERROR;
        return  (PX_ERROR);
    }

    /*
     * 命令发送
     */
    iCmdFlg = __sdhciCmdRespType(psdcmd);                               /*  应答类型                    */
    if (SD_CMD_TEST_RSP(psdcmd, SD_RSP_CRC)) {
        iCmdFlg |= SDHCI_CMD_CRC_CHK;
    }
    if (SD_CMD_TEST_RSP(psdcmd, SD_RSP_OPCODE)) {
        iCmdFlg |= SDHCI_CMD_INDEX_CHK;
    }
    if (psddat) {
        iCmdFlg |= SDHCI_CMD_DATA;                                      /*  命令类型                    */
    }

    /*
     * 如果是 CMD12 或者是 CMD52 中的写I/O Abort 命令
     */
    if (psdcmd->SDCMD_uiOpcode == SD_STOP_TRANSMISSION) {
        iCmdFlg |= SDHCI_CMD_TYPE_ABORT;
    } else if (psdcmd->SDCMD_uiOpcode == SDIO_RW_DIRECT) {
        UINT32 uiArg = psdcmd->SDCMD_uiArg;
        if ((((uiArg >>  9) & 0x1ffff) == SDIO_CCCR_ABORT) &&           /*  CCCR abort reg addr         */
            (((uiArg >> 28) & 0x7)     == 0)               &&           /*  Function 0                  */
            (((uiArg >> 31) & 0x1)     != 0)) {                         /*  Write the Reg               */
            iCmdFlg |= SDHCI_CMD_TYPE_ABORT;
        }
    }

    SDHCI_WRITEW(psdhcihostattr, SDHCI_COMMAND, SDHCI_MAKE_CMD(psdcmd->SDCMD_uiOpcode, iCmdFlg));

    KN_IO_MB();
    return  (ERROR_NONE);

__timeout:
    SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL,"timeout error.\r\n");

    psdhcitrans->SDHCITS_iCmdError = PX_ERROR;
    psdhcitrans->SDHCITS_iDatError = PX_ERROR;
    __sdhciTransFinish(psdhcitrans);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __sdhciTransferModSet
** 功能描述: 设置传输模式
** 输    入: psdhcihost      SDHCI HOST 结构指针
**           psddat          数据传输控制机构
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdhciTransferModSet (__PSDHCI_HOST  psdhcihost)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    __PSDHCI_TRANS      psdhcitrans    = psdhcihost->SDHCIHS_psdhcitrans;
    UINT16              usTmod;

    if (!psdhcitrans->SDHCITS_pucDatBuffCurr) {
        return;
    }

    usTmod = SDHCI_TRNS_BLK_CNT_EN;                                     /*  使能块计数                  */

    if (psdhcitrans->SDHCITS_uiBlkCntRemain > 1) {
        usTmod |= SDHCI_TRNS_MULTI;
        if (!SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_DONOT_USE_ACMD12) &&
            (psdhcitrans->SDHCITS_psdcmd->SDCMD_uiOpcode != SDIO_RW_EXTENDED)) {
            usTmod |= SDHCI_TRNS_ACMD12;                                /*  使用ACMD12功能              */
        }
    }

    if (psdhcitrans->SDHCITS_bIsRead) {
        usTmod |= SDHCI_TRNS_READ;
    }

    if (psdhcihost->SDHCIHS_ucTransferMod != SDHCIHOST_TMOD_SET_NORMAL) {
        usTmod |= SDHCI_TRNS_DMA;
    }

    SDHCI_WRITEW(psdhcihostattr, SDHCI_TRANSFER_MODE, usTmod);
}
/*********************************************************************************************************
** 函数名称: __sdhciDataPrepareNorm
** 功能描述: 一般数据传输准备
** 输    入: psdhcihost      SDHCI HOST 结构指针
**           psddat          数据传输控制机构
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdhciDataPrepareNorm (__PSDHCI_HOST  psdhcihost)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    __PSDHCI_TRANS      psdhcitrans    = psdhcihost->SDHCIHS_psdhcitrans;

    if (!psdhcitrans->SDHCITS_pucDatBuffCurr) {
        return;
    }

    __sdhciTransferIntSet(psdhcihost);                                  /*  传输中断使能                */

    SDHCI_WRITEW(psdhcihostattr,
                 SDHCI_BLOCK_SIZE,
                 SDHCI_MAKE_BLKSZ(__SDHCI_DMA_BOUND_NBITS,
                                  psdhcitrans->SDHCITS_uiBlkSize));     /*  块大小寄存器                */

    SDHCI_WRITEW(psdhcihostattr,
                 SDHCI_BLOCK_COUNT,
                 psdhcitrans->SDHCITS_uiBlkCntRemain);                  /*  块计数寄存器                */
}
/*********************************************************************************************************
** 函数名称: __sdhciDataPrepareSdma
** 功能描述: DMA数据传输准备
** 输    入: psdhcihost      SDHCI HOST 结构指针
**           psdmsg          传输控制消息结构指针
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdhciDataPrepareSdma (__PSDHCI_HOST  psdhcihost)
{
    __PSDHCI_TRANS psdhcitrans = psdhcihost->SDHCIHS_psdhcitrans;
    UINT8         *pucBuf      = psdhcitrans->SDHCITS_pucDatBuffCurr;

    if (!pucBuf) {
        return;
    }

#if LW_CFG_VMM_EN > 0
    pucBuf = psdhcitrans->SDHCITS_pucDmaBuffer;
    if (!psdhcitrans->SDHCITS_bIsRead) {
        lib_memcpy(psdhcitrans->SDHCITS_pucDmaBuffer,
                   psdhcitrans->SDHCITS_pucDatBuffCurr,
                   psdhcitrans->SDHCITS_uiBlkCntRemain * psdhcitrans->SDHCITS_uiBlkSize);
    }
#else
    pucBuf = psdhcitrans->SDHCITS_pucDatBuffCurr;
#endif

    __sdhciSdmaAddrUpdate(psdhcihost, (LONG)pucBuf);                    /*  写入 DMA 系统地址寄存器     */
                                                                        /*  命令发送后才会启动 DMA 传输 */
    __sdhciDataPrepareNorm(psdhcihost);
    __sdhciDmaSelect(psdhcihost, SDHCI_HCTRL_SDMA);
}
/*********************************************************************************************************
** 函数名称: __sdhciDataPrepareAdma
** 功能描述: 高效 DMA 数据传输准备
** 输    入: psdhcihost      SDHCI HOST 结构指针
**           psddat          数据传输控制机构
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdhciDataPrepareAdma (__PSDHCI_HOST  psdhcihost)
{
}
/*********************************************************************************************************
** 函数名称: __sdhciDataReadNorm
** 功能描述: 一般数据传输(读数据)
** 输    入: psdhcitrans     传输控制块
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdhciDataReadNorm (__PSDHCI_TRANS    psdhcitrans)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr;
    UINT32              uiBlkSize;
    UINT32              uiChunk;
    UINT32              uiData;
    UINT8              *pucBuffer;

    psdhcihostattr = &psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr;
    uiBlkSize      = psdhcitrans->SDHCITS_uiBlkSize;
    pucBuffer      = psdhcitrans->SDHCITS_pucDatBuffCurr;
    uiChunk        = 0;
    uiData         = 0;

    if (((ULONG)pucBuffer) & 0x3) {
        while (uiBlkSize) {
            if (uiChunk == 0) {
                uiData = SDHCI_READL(psdhcihostattr, SDHCI_BUFFER);
                uiChunk = 4;
            }

            *pucBuffer = uiData & 0xFF;
            pucBuffer++;
            uiData >>= 8;
            uiChunk--;
            uiBlkSize--;
        }

    } else {
        UINT32 *puiBuffer = (UINT32 *)pucBuffer;

        while (uiBlkSize >= 4) {
            *puiBuffer++ = SDHCI_READL(psdhcihostattr, SDHCI_BUFFER);
            uiBlkSize   -= 4;
        }

        pucBuffer = (UINT8 *)puiBuffer;

        if (uiBlkSize > 0) {
            uiData = SDHCI_READL(psdhcihostattr, SDHCI_BUFFER);
            while (uiBlkSize-- > 0) {
                *pucBuffer++ = uiData & 0xFF;
                uiData >>= 8;
            }
        }
    }

    psdhcitrans->SDHCITS_uiBlkCntRemain -= 1;
    psdhcitrans->SDHCITS_pucDatBuffCurr  = pucBuffer;                   /*  记录下一次数据处理位置      */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciDataWriteNorm
** 功能描述: 一般数据传输(写数据)
** 输    入: psdhcitrans     传输控制块
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdhciDataWriteNorm (__PSDHCI_TRANS    psdhcitrans)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr;
    UINT32              uiBlkSize;
    UINT32              uiChunk;
    UINT32              uiData;
    UINT8              *pucBuffer;

    psdhcihostattr = &psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr;
    uiBlkSize      = psdhcitrans->SDHCITS_uiBlkSize;
    pucBuffer      = psdhcitrans->SDHCITS_pucDatBuffCurr;
    uiChunk        = 0;
    uiData         = 0;

    /*
     * 以下可以处理块大小不是 4 倍数的情况
     */
    if (((ULONG)pucBuffer) & 0x3) {
        while (uiBlkSize) {
            uiData |= (UINT32)(*pucBuffer) << (uiChunk << 3);

            pucBuffer++;
            uiChunk++;
            uiBlkSize--;
            if ((uiChunk == 4) || (uiBlkSize == 0)) {
                SDHCI_WRITEL(psdhcihostattr, SDHCI_BUFFER, uiData);
                uiChunk = 0;
                uiData  = 0;
            }
        }

    } else {
        UINT32 *puiBuffer = (UINT32 *)pucBuffer;

        while (uiBlkSize >= 4) {
            SDHCI_WRITEL(psdhcihostattr, SDHCI_BUFFER, *puiBuffer++);
            uiBlkSize -= 4;
        }

        pucBuffer = (UINT8 *)puiBuffer;

        if (uiBlkSize > 0) {
            while (uiBlkSize-- > 0) {
                uiData <<= 8;
                uiData |= *pucBuffer++;
            }
            SDHCI_WRITEL(psdhcihostattr, SDHCI_BUFFER, uiData);
        }
    }

    psdhcitrans->SDHCITS_uiBlkCntRemain -= 1;
    psdhcitrans->SDHCITS_pucDatBuffCurr  = pucBuffer;                   /*  记录下一次数据处理位置      */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciTransferIntSet
** 功能描述: 数据传输中断设置
** 输    入: psdhcihost       SDHCI HOST 结构指针
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdhciTransferIntSet (__PSDHCI_HOST  psdhcihost)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    UINT32              uiIntIoMsk;                                     /*  使用一般传输时的中断掩码    */
    UINT32              uiIntDmaMsk;                                    /*  使用 DMA 传输时的中断掩码   */
    UINT32              uiEnMask;                                       /*  最终使能掩码                */

    uiIntIoMsk  = SDHCI_INT_SPACE_AVAIL | SDHCI_INT_DATA_AVAIL;
    uiIntDmaMsk = SDHCI_INT_DMA_END     | SDHCI_INT_ADMA_ERROR;
    uiEnMask    = SDHCI_READL(psdhcihostattr, SDHCI_INTSTA_ENABLE);     /*  读取32位(包括错误中断)      */

    if (psdhcihost->SDHCIHS_ucTransferMod != SDHCIHOST_TMOD_SET_NORMAL) {
        uiEnMask &= ~uiIntIoMsk;
        uiEnMask |=  uiIntDmaMsk;
    } else {
        uiEnMask &= ~uiIntDmaMsk;
        uiEnMask |=  uiIntIoMsk;
    }

    uiEnMask |= SDHCI_INT_RESPONSE | SDHCI_INT_DATA_END;

    /*
     * 因为这两个寄存器的位标位置都完全相同,
     * 所以可以同时用一个掩码.
     */
    SDHCI_WRITEL(psdhcihostattr, SDHCI_INTSTA_ENABLE, uiEnMask);
    SDHCI_WRITEL(psdhcihostattr, SDHCI_SIGNAL_ENABLE, uiEnMask);
}
/*********************************************************************************************************
** 函数名称: __sdhciIntDisAndEn
** 功能描述: 中断设置(使能\禁能).设置中断状态(错误\一般状态)和中断信号(错误\一般信号)寄存器.
** 输    入: psdhcihost      SDHCI HOST 结构指针
**           uiDisMask       禁能掩码
**           uiEnMask        使能掩码
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdhciIntDisAndEn (__PSDHCI_HOST  psdhcihost,
                                UINT32         uiDisMask,
                                UINT32         uiEnMask)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    UINT32              uiMask;

    uiMask  =  SDHCI_READL(psdhcihostattr, SDHCI_INTSTA_ENABLE);
    uiMask &= ~uiDisMask;
    uiMask |=  uiEnMask;

    /*
     * 因为这两个寄存器的位标位置都完全相同,
     * 所以可以同时用一个掩码.
     */
    SDHCI_WRITEL(psdhcihostattr, SDHCI_INTSTA_ENABLE, uiMask);
    SDHCI_WRITEL(psdhcihostattr, SDHCI_SIGNAL_ENABLE, uiMask);
}
/*********************************************************************************************************
** 函数名称: __sdhciSdioIntEn
** 功能描述: 使能 SDIO 中断
** 输    入: psdhcihost      SDHCI HOST 结构指针
**           bEnable         是否使能
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdhciSdioIntEn (__PSDHCI_HOST psdhcihost, BOOL bEnable)
{
    PLW_SDHCI_HOST_ATTR  psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    UINT32               uiMask;

    if (psdhcihost->SDHCIHS_bSdioIntEnable == bEnable) {
        return;
    }

    uiMask = SDHCI_READL(psdhcihostattr, SDHCI_INTSTA_ENABLE);
    if (bEnable) {
        uiMask |= SDHCI_INT_CARD_INT;
    } else {
        uiMask &= ~SDHCI_INT_CARD_INT;
    }

    SDHCI_WRITEL(psdhcihostattr, SDHCI_INTSTA_ENABLE, uiMask);
    SDHCI_WRITEL(psdhcihostattr, SDHCI_SIGNAL_ENABLE, uiMask);
    psdhcihost->SDHCIHS_bSdioIntEnable = bEnable;
}
/*********************************************************************************************************
** 函数名称: __sdhciTransNew
** 功能描述: 新分配并初始化一个传输事务对象
** 输    入: psdhcihost   主控制器对象
** 输    出: 传输事务对象
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static __SDHCI_TRANS *__sdhciTransNew (__PSDHCI_HOST   psdhcihost)
{
    __SDHCI_TRANS    *psdhcitrans;
    LW_OBJECT_HANDLE  hSync;
    INT               iError;

#if LW_CFG_VMM_EN > 0
    VOID             *pvDmaBuf;
#endif

    if (!psdhcihost) {
        return  (LW_NULL);
    }

    psdhcitrans = (__SDHCI_TRANS *)__SHEAP_ALLOC(sizeof(__SDHCI_TRANS));
    if (!psdhcitrans) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        return  (LW_NULL);
    }
    lib_bzero(psdhcitrans, sizeof(__SDHCI_TRANS));

#if LW_CFG_VMM_EN > 0
    pvDmaBuf = API_VmmDmaAllocAlign(__SDHCI_DMA_BOUND_LEN, 4);
    if (!pvDmaBuf) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        goto    __err0;
    }
    psdhcitrans->SDHCITS_pucDmaBuffer = (UINT8 *)pvDmaBuf;
#endif

    hSync = API_SemaphoreBCreate("sdhcits_sync", LW_FALSE, LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (hSync == LW_OBJECT_HANDLE_INVALID) {
        goto    __err1;
    }

    LW_SPIN_INIT(&psdhcitrans->SDHCITS_slLock);

#if LW_CFG_VMM_EN > 0
    psdhcitrans->SDHCITS_pucDmaBuffer = pvDmaBuf;
#endif
    psdhcitrans->SDHCITS_hFinishSync  = hSync;
    psdhcitrans->SDHCITS_psdhcihost   = psdhcihost;
    psdhcihost->SDHCIHS_psdhcitrans   = psdhcitrans;

    iError = __sdhciTransTaskInit(psdhcitrans);
    if (iError != ERROR_NONE) {
        goto    __err2;
    }

    API_InterVectorConnect(psdhcihost->SDHCIHS_sdhcihostattr.SDHCIHOST_ulIntVector,
                           __sdhciTransIrq,
                           (VOID *)psdhcitrans,
                           "sdhci_isr");
    API_InterVectorEnable(psdhcihost->SDHCIHS_sdhcihostattr.SDHCIHOST_ulIntVector);

    return  (psdhcitrans);

__err2:
    API_SemaphoreBDelete(&hSync);

__err1:
#if LW_CFG_VMM_EN > 0
   API_VmmDmaFree(pvDmaBuf);
#endif

__err0:
    __SHEAP_FREE(psdhcitrans);

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __sdhciTransTaskInit
** 功能描述: 初始化传输事务线程相关
** 输    入: psdhcitrans  传输事务控制块
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdhciTransTaskInit (__SDHCI_TRANS *psdhcitrans)
{
    LW_OBJECT_HANDLE     hSdioIntSem;
    LW_OBJECT_HANDLE     hSdioIntThread;
    LW_CLASS_THREADATTR  threadattr;
    LW_SDHCI_HOST_ATTR  *psdhcihostattr;

    psdhcihostattr = &psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr;
    if (SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_SDIO_INT_OOB)) {
        return  (ERROR_NONE);
    }

    if (SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_CANNOT_SDIO_INT)) {
        hSdioIntSem = API_SemaphoreBCreate("sdhcisdio_sem",
                                           LW_FALSE,
                                           LW_OPTION_OBJECT_GLOBAL,
                                           LW_NULL);
    } else {
        hSdioIntSem = API_SemaphoreCCreate("sdhcisdio_sem",
                                           0,
                                           4096,
                                           LW_OPTION_OBJECT_GLOBAL,
                                           LW_NULL);
    }

    if (hSdioIntSem == LW_OBJECT_HANDLE_INVALID) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "create semaphore failed.\r\n");
        return  (PX_ERROR);
    }
    psdhcitrans->SDHCITS_hSdioIntSem = hSdioIntSem;

    threadattr = API_ThreadAttrGetDefault();
    threadattr.THREADATTR_pvArg            = (PVOID)psdhcitrans;
    threadattr.THREADATTR_ucPriority       = __SDHCI_SDIOINTSVR_PRIO;
    threadattr.THREADATTR_stStackByteSize  = __SDHCI_SDIOINTSVR_STKSZ;
    threadattr.THREADATTR_ulOption        |= LW_OPTION_OBJECT_GLOBAL;
    hSdioIntThread = API_ThreadCreate("t_sdhcisdio",
                                      __sdhciSdioIntSvr,
                                      &threadattr,
                                      LW_NULL);
    if (hSdioIntThread == LW_OBJECT_HANDLE_INVALID) {
        goto    __err;
    }
    psdhcitrans->SDHCITS_hSdioIntThread = hSdioIntThread;

    return  (ERROR_NONE);

__err:
    API_SemaphoreDelete(&hSdioIntSem);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __sdhciTransTaskDeInit
** 功能描述: 清理传输事务线程相关
** 输    入: psdhcitrans  传输事务控制块
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdhciTransTaskDeInit (__SDHCI_TRANS *psdhcitrans)
{
    LW_SDHCI_HOST_ATTR  *psdhcihostattr;

    psdhcihostattr = &psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr;

    if (SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_SDIO_INT_OOB)) {
        return  (ERROR_NONE);
    }

    API_ThreadDelete(&psdhcitrans->SDHCITS_hSdioIntThread, LW_NULL);
    API_SemaphoreDelete(&psdhcitrans->SDHCITS_hSdioIntSem);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciTransDel
** 功能描述: 清理并释放传输事务对象空间
** 输    入: psdhcitrans
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdhciTransDel (__SDHCI_TRANS *psdhcitrans)
{
    ULONG ulVector;

    if (!psdhcitrans) {
        return;
    }

    ulVector = psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr.SDHCIHOST_ulIntVector;

    API_InterVectorDisable(ulVector);
    API_InterVectorDisconnect(ulVector, __sdhciTransIrq, (VOID *)psdhcitrans);

    __sdhciTransTaskDeInit(psdhcitrans);

#if LW_CFG_VMM_EN > 0
    API_VmmDmaFree(psdhcitrans->SDHCITS_pucDmaBuffer);
#endif

    API_SemaphoreBDelete(&psdhcitrans->SDHCITS_hFinishSync);
    __SHEAP_FREE(psdhcitrans);
}
/*********************************************************************************************************
** 函数名称: __sdhciTransPrepare
** 功能描述: 准备传输事务
** 输    入: psdhcitrans      传输事务对象
**           psdmsg           用户请求的传输消息
**           iTransType       传输类型
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdhciTransPrepare (__SDHCI_TRANS *psdhcitrans,
                                 LW_SD_MESSAGE *psdmsg,
                                 INT            iTransType)
{
    LW_SD_DATA  *psddata;
    UINT32       uiMaxBlkSize;

    if (!psdhcitrans || !psdmsg || !psdmsg->SDMSG_psdcmdCmd) {
        return  (PX_ERROR);
    }

    uiMaxBlkSize = psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_uiMaxBlkSize;
    psddata      = psdmsg->SDMSG_psddata;
    if (psddata) {
        if ((psddata->SDDAT_uiBlkSize < 1) || (psddata->SDDAT_uiBlkSize > uiMaxBlkSize)) {
            SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL,
                              "blk size(%u) is out of range(1~%u)\r\n",
                              psddata->SDDAT_uiBlkSize, uiMaxBlkSize);
            return  (PX_ERROR);
        }
        if ((psddata->SDDAT_uiBlkNum < 1) || (psddata->SDDAT_uiBlkNum > 65535)) {
            SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL,
                              "blk num(%u) is out of range(1~65535)\r\n",
                              psddata->SDDAT_uiBlkNum);
            return  (PX_ERROR);
        }
        if ((psddata->SDDAT_uiBlkSize * psddata->SDDAT_uiBlkNum) > (512 * 1024)) {
            SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL,
                              "transfer data length(%u) is out of range(512KB)\r\n",
                              psddata->SDDAT_uiBlkSize * psddata->SDDAT_uiBlkNum);
            return  (PX_ERROR);
        }

        if (SD_DAT_IS_READ(psddata)) {
            psdhcitrans->SDHCITS_pucDatBuffCurr = psdmsg->SDMSG_pucRdBuffer;
            psdhcitrans->SDHCITS_bIsRead        = LW_TRUE;
        } else {
            psdhcitrans->SDHCITS_pucDatBuffCurr = psdmsg->SDMSG_pucWrtBuffer;
            psdhcitrans->SDHCITS_bIsRead        = LW_FALSE;
        }

        psdhcitrans->SDHCITS_uiBlkSize      = psddata->SDDAT_uiBlkSize;
        psdhcitrans->SDHCITS_uiBlkCntRemain = psddata->SDDAT_uiBlkNum;

    } else {
        psdhcitrans->SDHCITS_pucDatBuffCurr = LW_NULL;
        psdhcitrans->SDHCITS_uiBlkSize      = 0;
        psdhcitrans->SDHCITS_uiBlkCntRemain = 0;
    }

    psdhcitrans->SDHCITS_iTransType  = iTransType;
    psdhcitrans->SDHCITS_bCmdFinish  = LW_FALSE;
    psdhcitrans->SDHCITS_bDatFinish  = LW_FALSE;
    psdhcitrans->SDHCITS_iCmdError   = ERROR_NONE;
    psdhcitrans->SDHCITS_iDatError   = ERROR_NONE;

    psdhcitrans->SDHCITS_psdcmd      = psdmsg->SDMSG_psdcmdCmd;
    psdhcitrans->SDHCITS_psdcmdStop  = psdmsg->SDMSG_psdcmdStop;
    psdhcitrans->SDHCITS_psddat      = psdmsg->SDMSG_psddata;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciTransStart
** 功能描述: 启动一次传输事务
** 输    入: psdhcitrans      传输事务对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdhciTransStart (__SDHCI_TRANS *psdhcitrans)
{
    INT     iRet;

    if (!psdhcitrans) {
        return  (PX_ERROR);
    }

    if (!SDHCI_QUIRK_FLG(&psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr,
                         SDHCI_QUIRK_FLG_DONOT_RESET_ON_EVERY_TRANSACTION)) {
        __sdhciHostReset(psdhcitrans->SDHCITS_psdhcihost, SDHCI_SFRST_DATA | SDHCI_SFRST_CMD);
    }


    if (SDHCI_QUIRK_FLG(&psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr,
                         SDHCI_QUIRK_FLG_REENABLE_INTS_ON_EVERY_TRANSACTION)) {
        __sdhciIntClear(psdhcitrans->SDHCITS_psdhcihost);
        __sdhciIntDisAndEn(psdhcitrans->SDHCITS_psdhcihost, SDHCI_INT_ALL_MASK, __SDHCI_INT_EN_MASK);
    }

    if (psdhcitrans->SDHCITS_pucDatBuffCurr) {
        if (psdhcitrans->SDHCITS_iTransType == __SDHIC_TRANS_NORMAL) {
            __sdhciDataPrepareNorm(psdhcitrans->SDHCITS_psdhcihost);
        } else if (psdhcitrans->SDHCITS_iTransType == __SDHIC_TRANS_SDMA) {
            __sdhciDataPrepareSdma(psdhcitrans->SDHCITS_psdhcihost);
        } else {
            __sdhciDataPrepareAdma(psdhcitrans->SDHCITS_psdhcihost);
        }
    }

    psdhcitrans->SDHCITS_iStage = __SDHCI_TRANS_STAGE_START;
    iRet = __sdhciCmdSend(psdhcitrans->SDHCITS_psdhcihost,
                          psdhcitrans->SDHCITS_psdcmd,
                          psdhcitrans->SDHCITS_psddat);

    if (iRet != ERROR_NONE) {
        API_SemaphoreBClear(psdhcitrans->SDHCITS_hFinishSync);
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __sdhciTransFinishWait
** 功能描述: 等待本次传输事务完成
** 输    入: psdhcitrans      传输事务对象
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdhciTransFinishWait (__SDHCI_TRANS *psdhcitrans)
{
    INT iRet;
    iRet = API_SemaphoreBPend(psdhcitrans->SDHCITS_hFinishSync, LW_OPTION_WAIT_INFINITE);
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __sdhciTransFinish
** 功能描述: 完成传输事务
** 输    入: psdhcitrans      传输事务对象
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __sdhciTransFinish (__SDHCI_TRANS *psdhcitrans)
{
    API_SemaphoreBPost(psdhcitrans->SDHCITS_hFinishSync);
}
/*********************************************************************************************************
** 函数名称: __sdhciTransClean
** 功能描述: 清理本次传输事务
** 输    入: psdhcitrans      传输事务对象
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdhciTransClean (__SDHCI_TRANS *psdhcitrans)
{
    if (!psdhcitrans) {
        return  (PX_ERROR);
    }

    psdhcitrans->SDHCITS_psdcmd     = LW_NULL;
    psdhcitrans->SDHCITS_psdcmdStop = LW_NULL;
    psdhcitrans->SDHCITS_psddat     = LW_NULL;

    if (SDHCI_QUIRK_FLG(&psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr,
                         SDHCI_QUIRK_FLG_REENABLE_INTS_ON_EVERY_TRANSACTION)) {
        __sdhciIntDisAndEn(psdhcitrans->SDHCITS_psdhcihost, SDHCI_INT_ALL_MASK, 0);
        __sdhciIntClear(psdhcitrans->SDHCITS_psdhcihost);
    }

    API_SemaphoreBClear(psdhcitrans->SDHCITS_hFinishSync);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciTransIrq
** 功能描述: SDHCI 中断服务程序
** 输    入: pvArg      事务控制块
**           ulVector   中断向量
** 输    出: 中断处理结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static irqreturn_t   __sdhciTransIrq (VOID *pvArg, ULONG ulVector)
{
    __SDHCI_TRANS       *psdhcitrans    = (__SDHCI_TRANS *)pvArg;
    __SDHCI_HOST        *psdhcihost     = psdhcitrans->SDHCITS_psdhcihost;
    LW_SDHCI_HOST_ATTR  *psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    BOOL                 bSdioInt       = LW_FALSE;

    UINT32               uiIntSta;
    irqreturn_t          irqret;

    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncIsrEnterHook) {
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncIsrEnterHook(psdhcihostattr);
    }

    uiIntSta = SDHCI_READL(psdhcihostattr, SDHCI_INT_STATUS);

__redo:
    if (!uiIntSta || uiIntSta == 0xffffffff) {
        SDHCI_WRITEL(psdhcihostattr, SDHCI_INT_STATUS, uiIntSta);
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "unknown int signals\r\n");
        irqret = LW_IRQ_NONE;
        goto    __end;                                                  /*  无效或未知的中断            */
    }

    SDHCI_WRITEL(psdhcihostattr, SDHCI_INT_STATUS, uiIntSta);

    __SDHCI_TRANS_LOCK(psdhcitrans);

    psdhcitrans->SDHCITS_uiIntSta = uiIntSta;

    if (uiIntSta & SDHCI_INT_CMD_MASK) {
        SDHCI_WRITEL(psdhcihostattr, SDHCI_INT_STATUS, uiIntSta & SDHCI_INT_CMD_MASK);
        __sdhciTransCmdHandle(psdhcitrans, uiIntSta & SDHCI_INT_CMD_MASK);
    }

    if (uiIntSta & SDHCI_INT_DATA_MASK) {
        SDHCI_WRITEL(psdhcihostattr, SDHCI_INT_STATUS, uiIntSta & SDHCI_INT_DATA_MASK);
        __sdhciTransDatHandle(psdhcitrans, uiIntSta & SDHCI_INT_DATA_MASK);
    }

    /*
     * 经测试发现, 某些情况下, 在本次数据传输完成后, HOST 并不能产生 SDIO 中断.
     * 这可能和具体的 SDIO 应用驱动的实现有关. 在这里为了确保不会因此原因造成
     * 传输的异常中断, 进行特殊处理.
     */
    if ((psdhcitrans->SDHCITS_pucDatBuffCurr)  &&
        (!psdhcitrans->SDHCITS_uiBlkCntRemain) &&
        (uiIntSta & SDHCI_INT_DATA_END)        &&
        psdhcihost->SDHCIHS_bSdioIntEnable) {
        bSdioInt = LW_TRUE;
    }

    uiIntSta &= ~(SDHCI_INT_CMD_MASK | SDHCI_INT_DATA_MASK);
    uiIntSta &= ~(SDHCI_INT_ERROR);

    if (uiIntSta & SDHCI_INT_BUS_POWER) {
        SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL,
                          "sdhci(%s): card consumed too much power!\r\n",
                          __SDHCI_HOST_NAME(psdhcitrans->SDHCITS_psdhcihost));
        SDHCI_WRITEL(psdhcihostattr, SDHCI_INT_STATUS, SDHCI_INT_BUS_POWER);
        uiIntSta &= ~SDHCI_INT_BUS_POWER;
    }

    if (uiIntSta & SDHCI_INT_CARD_INT) {
        bSdioInt = LW_TRUE;
        uiIntSta &= ~SDHCI_INT_CARD_INT;
    }

    if (uiIntSta) {
        SDHCI_WRITEL(psdhcihostattr, SDHCI_INT_STATUS, uiIntSta);
    }

    irqret = LW_IRQ_HANDLED;

    KN_IO_MB();

    if (!SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_CANNOT_SDIO_INT) &&
        !SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_SDIO_INT_OOB)    &&
        bSdioInt) {
        __sdhciSdioIntEn(psdhcitrans->SDHCITS_psdhcihost, LW_FALSE);
        __SDHCI_SDIO_NOTIFY(psdhcitrans);
    }

    /*
     * 有的控制器在处理完当前的中断后, 可能会再次产生新的中断状态,
     * 该状态不一定能在之后以硬件中断的方式通知 CPU,
     * 该情况下需要再次查询直到处理完成.
     */
    if (SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_RECHECK_INTS_AFTER_ISR)) {
        uiIntSta = SDHCI_READL(psdhcihostattr, SDHCI_INT_STATUS);
        if (uiIntSta) {
            __SDHCI_TRANS_UNLOCK(psdhcitrans);
            bSdioInt = LW_FALSE;
            goto __redo;
        }
    }

    __SDHCI_TRANS_UNLOCK(psdhcitrans);

__end:
    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncIsrExitHook) {
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncIsrExitHook(psdhcihostattr);
    }

    return  (irqret);
}
/*********************************************************************************************************
** 函数名称: __sdhciSdioIntSvr
** 功能描述: SDHCI SDIO 中断服务线程
** 输    入: pvArg      事务控制块
** 输    出: 线程返回值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PVOID  __sdhciSdioIntSvr (VOID *pvArg)
{
    __SDHCI_TRANS       *psdhcitrans     = (__SDHCI_TRANS *)pvArg;
    __SDHCI_HOST        *psdhcihost      = psdhcitrans->SDHCITS_psdhcihost;
    LW_SDHCI_HOST_ATTR  *psdhcihostattr  = &psdhcihost->SDHCIHS_sdhcihostattr;
    INT                  iError          = ERROR_NONE;

    if (SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_CANNOT_SDIO_INT)) {
        INT     iTickDly  = __SDHCI_SDIO_DLY_MAX;
        INT     iDoIntCnt = 1;
        INT     i;

        while (1) {
            __SDHCI_SDIO_REQUEST(psdhcitrans);
            __SDHCI_SDIO_RELEASE(psdhcitrans);                          /*  立即释放允许其他服务禁止中断*/

            for (i = 0; i < iDoIntCnt; i++) {
                iError = API_SdmEventNotify(psdhcihost->SDHCIHS_psdhcisdmhost->SDHCISDMH_pvSdmHost,
                                            SDM_EVENT_SDIO_INTERRUPT);
                if (iError != ERROR_NONE) {
                    /*
                     * 如果本次确实没有 SDIO 中断, 这里不立即终止查询
                     * 因为有可能在下一次会产生, 这样不至于影响传输速度
                     */
                }
            }

            /*
             * 动态调整查询间隔
             */
            if (iError != ERROR_NONE) {
                /*
                 * 缓慢降低查询频率
                 */
                if (iTickDly < __SDHCI_SDIO_DLY_MAX) {
                    iTickDly++;
                }
                if (iDoIntCnt > 1) {
                    if (iTickDly >= __SDHCI_SDIO_DLY_MAX) {
                        iDoIntCnt--;
                    }
                }

            } else {
                /*
                 * 立即提升查询频率到最大
                 */
                iTickDly  = 1;
                iDoIntCnt = __SDHCI_SDIO_INT_MAX;
            }

            API_TimeSleep(iTickDly);
        }
    } else {
        while (1) {
            __SDHCI_SDIO_WAIT(psdhcitrans);
            API_SdmEventNotify(psdhcihost->SDHCIHS_psdhcisdmhost->SDHCISDMH_pvSdmHost,
                               SDM_EVENT_SDIO_INTERRUPT);
            __sdhciSdioIntEn(psdhcihost, LW_TRUE);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __sdhciTransCmdHandle
** 功能描述: 处理命令相关的事务
** 输    入: psdhcitrans      传输事务对象
**           uiIntSta         命令相关的中断状态
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdhciTransCmdHandle (__SDHCI_TRANS *psdhcitrans, UINT32 uiIntSta)
{
    if (!psdhcitrans->SDHCITS_psdcmd) {
        SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL, "unexpected cmd interrupt: %08x.\r\n", uiIntSta);

        do {
            SDHCI_WRITEL(&psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr,
                         SDHCI_INT_STATUS, uiIntSta);
            uiIntSta = SDHCI_READL(&psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr,
                                   SDHCI_INT_STATUS);
        } while (uiIntSta);

        return  (PX_ERROR);
    }

    if (uiIntSta &
        (SDHCI_INT_TIMEOUT |
         SDHCI_INT_CRC     |
         SDHCI_INT_END_BIT |
         SDHCI_INT_INDEX   |
         SDHCI_INT_ACMD12ERR)) {

        SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL, "cmd error interrupt: %08x.\r\n", uiIntSta);

        if (SDHCI_QUIRK_FLG(&psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr,
                            SDHCI_QUIRK_FLG_DO_RESET_ON_TRANSACTION_ERROR)) {
            if (uiIntSta & (SDHCI_INT_TIMEOUT | SDHCI_INT_CRC)) {
                if (!psdhcitrans->SDHCITS_bCmdFinish) {
                    __sdhciHostReset(psdhcitrans->SDHCITS_psdhcihost, SDHCI_SFRST_CMD);
                }

                if ((!psdhcitrans->SDHCITS_bDatFinish) ||
                    (SD_CMD_TEST_RSP(psdhcitrans->SDHCITS_psdcmd, SD_RSP_BUSY))) {
                    __sdhciHostReset(psdhcitrans->SDHCITS_psdhcihost, SDHCI_SFRST_DATA);
                }
            }
        }

        psdhcitrans->SDHCITS_iCmdError = PX_ERROR;
        __sdhciTransFinish(psdhcitrans);                                /*  结束本次传输                */
        return  (PX_ERROR);
    }

    if (uiIntSta & SDHCI_INT_RESPONSE) {
        __sdhciTransCmdFinish(psdhcitrans);                             /*  命令完成处理                */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciTransDatHandle
** 功能描述: 处理数据相关的事务
** 输    入: psdhcitrans      传输事务对象
**           uiIntSta         数据相关的中断状态
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdhciTransDatHandle (__SDHCI_TRANS *psdhcitrans, UINT32 uiIntSta)
{
    LW_SDHCI_HOST_ATTR  *psdhcihostattr = &psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr;

    /*
     * 当没有数据传输时, 如果命令包含了 忙等待信号, 则也会产生数据完成中断
     * 详见 SDHCI 规范 2.2.17(page64)
     */
    if (!psdhcitrans->SDHCITS_pucDatBuffCurr) {
        if (SD_CMD_TEST_RSP(psdhcitrans->SDHCITS_psdcmd, SD_RSP_BUSY)) {
            if (uiIntSta & SDHCI_INT_DATA_TIMEOUT) {
                psdhcitrans->SDHCITS_iDatError = PX_ERROR;
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "timeout on busy irq.\r\n");
                __sdhciTransFinish(psdhcitrans);
                return  (PX_ERROR);
            }

            if (uiIntSta & SDHCI_INT_DATA_END) {
                psdhcitrans->SDHCITS_iDatError  = ERROR_NONE;
                psdhcitrans->SDHCITS_bDatFinish = LW_TRUE;

                /*
                 * 如果命令未完成,需待命令中断产生后结束事务
                 */
                if (psdhcitrans->SDHCITS_bCmdFinish) {
                    __sdhciTransFinish(psdhcitrans);
                }

                return  (ERROR_NONE);
            }
        }
    }

    if (uiIntSta                &
        (SDHCI_INT_DATA_TIMEOUT |
         SDHCI_INT_DATA_CRC     |
         SDHCI_INT_DATA_END_BIT |
         SDHCI_INT_ADMA_ERROR)) {
        SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL, "data error interrupt: %08x.\r\n", uiIntSta);

        if (SDHCI_QUIRK_FLG(&psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr,
                            SDHCI_QUIRK_FLG_DO_RESET_ON_TRANSACTION_ERROR)) {
            if (uiIntSta & (SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_DATA_CRC)) {
                if ((!psdhcitrans->SDHCITS_bDatFinish) ||
                    (SD_CMD_TEST_RSP(psdhcitrans->SDHCITS_psdcmd, SD_RSP_BUSY))) {
                    __sdhciHostReset(psdhcitrans->SDHCITS_psdhcihost, SDHCI_SFRST_DATA);
                }
            }
        }

        psdhcitrans->SDHCITS_iDatError = PX_ERROR;
        psdhcitrans->SDHCITS_iCmdError = PX_ERROR;
        __sdhciTransDatFinish(psdhcitrans);
        return  (PX_ERROR);
    }

    /*
     * 数据可 读/写 中断
     */
    if ((uiIntSta & (SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL)) &&
        (psdhcitrans->SDHCITS_uiBlkCntRemain > 0)) {
        if (psdhcitrans->SDHCITS_bIsRead) {
            while (SDHCI_READL(psdhcihostattr, SDHCI_PRESENT_STATE) & SDHCI_PSTA_DATA_AVAILABLE) {
                __sdhciDataReadNorm(psdhcitrans);
                if (psdhcitrans->SDHCITS_uiBlkCntRemain == 0) {
                    /*
                     * 到这里, 数据已经处理完了. 但是这里不直接返回
                     * 因为一定会伴随一个 数据完成 中断, 由之后处理
                     */
                    break;
                }
            }
        } else {
            while (SDHCI_READL(psdhcihostattr, SDHCI_PRESENT_STATE) & SDHCI_PSTA_SPACE_AVAILABLE) {
                __sdhciDataWriteNorm(psdhcitrans);
                if (psdhcitrans->SDHCITS_uiBlkCntRemain == 0) {
                    /*
                     * 到这里, 数据已经处理完了. 但是这里不直接返回
                     * 因为一定会伴随一个 数据完成 中断, 由之后处理
                     */
                    break;
                }
            }
        }
    }

    /*
     * 当前未考虑 DMA 边界中断的情况. 但是
     * 如果确实产生了, 仅仅重启 DMA 传输即可
     */
    if (uiIntSta & SDHCI_INT_DMA_END) {
        UINT32  uiDmaAddr;
        uiDmaAddr = SDHCI_READL(psdhcihostattr, SDHCI_SYS_SDMA);
        SDHCI_WRITEL(psdhcihostattr, SDHCI_SYS_SDMA, uiDmaAddr);
    }

    if (uiIntSta & SDHCI_INT_DATA_END) {
        psdhcitrans->SDHCITS_bDatFinish = LW_TRUE;
        if (!psdhcitrans->SDHCITS_bCmdFinish) {
            /*
             * 数据中断和命令中断的顺序不确定
             * 如果命令中断还未产生, 这里仅仅标记数据传输完成
             * 等待稍后的命令中断进行后续处理
             */
            psdhcitrans->SDHCITS_bDatFinish = LW_TRUE;
        } else {
            __sdhciTransDatFinish(psdhcitrans);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciTransCmdFinish
** 功能描述: 命令正确完成后的处理
** 输    入: psdhcitrans      传输事务对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdhciTransCmdFinish (__SDHCI_TRANS *psdhcitrans)
{
    LW_SDHCI_HOST_ATTR  *psdhcihostattr = &psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr;
    LW_SD_COMMAND       *psdcmd         = psdhcitrans->SDHCITS_psdcmd;
    UINT32               uiRespFlag;

    if (psdhcitrans->SDHCITS_iStage == __SDHCI_TRANS_STAGE_START) {
        psdcmd = psdhcitrans->SDHCITS_psdcmd;
    } else {
        psdcmd = psdhcitrans->SDHCITS_psdcmdStop;
    }

    uiRespFlag = SD_RESP_TYPE(psdcmd);
    if (uiRespFlag & SD_RSP_PRESENT) {
        UINT32  *puiResp = psdcmd->SDCMD_uiResp;

        if (psdhcihostattr->SDHCIHOST_pquirkop &&
            psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncResponseGet) {
            psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncResponseGet(psdhcihostattr,
                                                                          uiRespFlag,
                                                                          puiResp);
            goto __ret;
        }

        /*
         * 长应答,应答寄存器中去除了 CRC 这一个字节,因此作移位处理.
         * 其应答字节序与上层规定的相反,作相应的转换.
         */
        if (uiRespFlag & SD_RSP_136) {
            UINT32   uiRsp0;
            UINT32   uiRsp1;
            UINT32   uiRsp2;
            UINT32   uiRsp3;

            uiRsp3 = SDHCI_READL(psdhcihostattr, SDHCI_RESPONSE3);
            uiRsp2 = SDHCI_READL(psdhcihostattr, SDHCI_RESPONSE2);
            uiRsp1 = SDHCI_READL(psdhcihostattr, SDHCI_RESPONSE1);
            uiRsp0 = SDHCI_READL(psdhcihostattr, SDHCI_RESPONSE0);

            puiResp[0] = (uiRsp3 << 8) | ( uiRsp2 >> 24);
            puiResp[1] = (uiRsp2 << 8) | ( uiRsp1 >> 24);
            puiResp[2] = (uiRsp1 << 8) | ( uiRsp0 >> 24);
            puiResp[3] = (uiRsp0 << 8);
        } else {
            puiResp[0] = SDHCI_READL(psdhcihostattr, SDHCI_RESPONSE0);
        }
    }

__ret:
    psdhcitrans->SDHCITS_iCmdError  = ERROR_NONE;
    psdhcitrans->SDHCITS_bCmdFinish = LW_TRUE;

    if (psdhcitrans->SDHCITS_iStage == __SDHCI_TRANS_STAGE_STOP) {
        __sdhciTransFinish(psdhcitrans);                                /*  本次事务结束                */
        return  (ERROR_NONE);
    }

    if (psdhcitrans->SDHCITS_pucDatBuffCurr &&
        psdhcitrans->SDHCITS_bDatFinish) {
        __sdhciTransDatFinish(psdhcitrans);                             /*  处理数据完成                */
    
	} else if (!psdhcitrans->SDHCITS_pucDatBuffCurr) {
        /*
         * 带忙等待的命令会产生数据完成中断
         * 此种情况下需要等待中断产生再结束事务
         */
        if(SD_CMD_TEST_RSP(psdhcitrans->SDHCITS_psdcmd, SD_RSP_BUSY) &&
           !psdhcitrans->SDHCITS_bDatFinish                          &&
           SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_HAS_DATEND_IRQ_WHEN_NOT_BUSY)) {
            return  (ERROR_NONE);
        }

        __sdhciTransFinish(psdhcitrans);                            /*  本次事务结束                */

    } else {
        /*
         * 这里表示数据还没处理完成
         * 则在后面的数据中断里处理
         */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciTransDatFinish
** 功能描述: 数据正确完成后的处理
** 输    入: psdhcitrans      传输事务对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdhciTransDatFinish (__SDHCI_TRANS *psdhcitrans)
{
    LW_SDHCI_HOST_ATTR  *psdhcihostattr = &psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr;

#if LW_CFG_VMM_EN > 0
    if (psdhcitrans->SDHCITS_iDatError == ERROR_NONE) {
        if (psdhcitrans->SDHCITS_bIsRead &&
            (psdhcitrans->SDHCITS_iTransType != __SDHIC_TRANS_NORMAL)) {

            lib_memcpy(psdhcitrans->SDHCITS_pucDatBuffCurr,
                       psdhcitrans->SDHCITS_pucDmaBuffer,
                       psdhcitrans->SDHCITS_uiBlkSize * psdhcitrans->SDHCITS_uiBlkCntRemain);
        }
    }
#endif

    if (SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_DONOT_USE_ACMD12)) {
        if (psdhcitrans->SDHCITS_psdcmdStop) {
            psdhcitrans->SDHCITS_iStage = __SDHCI_TRANS_STAGE_STOP;
            __sdhciCmdSend(psdhcitrans->SDHCITS_psdhcihost, psdhcitrans->SDHCITS_psdcmdStop, LW_NULL);
            return  (ERROR_NONE);
        }
    }

    __sdhciTransFinish(psdhcitrans);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciRegAccessDrvInit
** 功能描述: 寄存器空间访问驱动初始化
** 输    入: psdhcihostattr 控制器属性描述对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdhciRegAccessDrvInit (PLW_SDHCI_HOST_ATTR  psdhcihostattr)
{
    static BOOL bInit = LW_FALSE;
    INT         iType;

    if (!psdhcihostattr) {
        return  (PX_ERROR);
    }

    iType = psdhcihostattr->SDHCIHOST_iRegAccessType;
    if ((iType != SDHCI_REGACCESS_TYPE_IO)  &&
        (iType != SDHCI_REGACCESS_TYPE_MEM) &&
        !(psdhcihostattr->SDHCIHOST_pdrvfuncs)) {
        return  (PX_ERROR);
    }

    if (!bInit) {
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_IO].sdhciReadB   = __sdhciIoReadB;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_IO].sdhciReadW   = __sdhciIoReadW;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_IO].sdhciReadL   = __sdhciIoReadL;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_IO].sdhciWriteB  = __sdhciIoWriteB;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_IO].sdhciWriteW  = __sdhciIoWriteW;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_IO].sdhciWriteL  = __sdhciIoWriteL;

        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_MEM].sdhciReadB  = __sdhciMemReadB;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_MEM].sdhciReadW  = __sdhciMemReadW;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_MEM].sdhciReadL  = __sdhciMemReadL;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_MEM].sdhciWriteB = __sdhciMemWriteB;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_MEM].sdhciWriteW = __sdhciMemWriteW;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_MEM].sdhciWriteL = __sdhciMemWriteL;

        bInit = LW_TRUE;
    }

    /*
     * 如果提供了自己的寄存器访问驱动则不使用默认的方法
     */
    if (!psdhcihostattr->SDHCIHOST_pdrvfuncs) {
        psdhcihostattr->SDHCIHOST_pdrvfuncs = &_G_sdhcidrvfuncTbl[iType];
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdhciIoReadL
** 功能描述: IO 空间读取32位长度的数据
** 输    入: uiAddr   IO 空间地址
** 输    出: 数据
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT32 __sdhciIoReadL (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr)
{
    return  in32(psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** 函数名称: __sdhciIoReadW
** 功能描述: IO 空间读取16位长度的数据
** 输    入: uiAddr   IO 空间地址
** 输    出: 数据
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT16 __sdhciIoReadW (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr)
{
    return  in16(psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** 函数名称: __sdhciIoReadB
** 功能描述: IO 空间读取8位长度的数据
** 输    入: uiAddr   IO 空间地址
** 输    出: 数据
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT8 __sdhciIoReadB (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr)
{
    return  in8(psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** 函数名称: __sdhciIoWriteL
** 功能描述: IO 空间写入32位长度的数据
** 输    入: uiAddr     IO 空间地址
**           uiLword    写入的数据
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdhciIoWriteL (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT32 uiLword)
{
    out32(uiLword, psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** 函数名称: __sdhciIoWriteW
** 功能描述: IO 空间写入16位长度的数据
** 输    入: uiAddr     IO 空间地址
**           usWord     写入的数据
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdhciIoWriteW (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT16 usWord)
{
    out16(usWord, psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** 函数名称:
** 功能描述: IO 空间写入8位长度的数据
** 输    入: uiAddr     IO 空间地址
**           ucByte     写入的数据
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdhciIoWriteB (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT8 ucByte)
{
    out8(ucByte, psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** 函数名称: __sdhciMemReadL
** 功能描述: 内存空间读取32位长度的数据
** 输    入: ulAddr   内存空间地址
** 输    出: 数据
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT32 __sdhciMemReadL (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr)
{
    return  read32(psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** 函数名称: __sdhciMemReadW
** 功能描述: 内存空间读取16位长度的数据
** 输    入: ulAddr   内存空间地址
** 输    出: 数据
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT16 __sdhciMemReadW (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr)
{
    return  read16(psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** 函数名称: __sdhciMemReadB
** 功能描述: 内存空间读取8位长度的数据
** 输    入: ulAddr   内存空间地址
** 输    出: 数据
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT8 __sdhciMemReadB (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr)
{
    return  read8(psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** 函数名称: __sdhciMemWriteL
** 功能描述: 内存空间写入32位长度的数据
** 输    入: ulAddr     内存空间地址
**           uiLword    写入的数据
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdhciMemWriteL (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT32 uiLword)
{
    write32(uiLword, psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** 函数名称: __sdhciMemWriteW
** 功能描述: 内存空间写入16位长度的数据
** 输    入: ulAddr     内存空间地址
**           usWord     写入的数据
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdhciMemWriteW (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT16 usWord)
{
    write16(usWord, psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** 函数名称: __sdhciMemWriteB
** 功能描述: 内存空间写入8位长度的数据
** 输    入: ulAddr     内存空间地址
**           ucByte     写入的数据
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdhciMemWriteB (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT8 ucByte)
{
    write8(ucByte, psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** 函数名称: __sdhciPreStaShow
** 功能描述: 显示当前所有状态
** 输    入: psdhcihostattr
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#ifdef  __SYLIXOS_DEBUG
static VOID __sdhciPreStaShow (PLW_SDHCI_HOST_ATTR psdhcihostattr)
{
#ifndef printk
#define printk
#endif                                                                  /*  printk                      */

    UINT32  uiSta;

    uiSta = SDHCI_READL(psdhcihostattr, SDHCI_PRESENT_STATE);
    printk("\nhost present status >>\n");
    printk("cmd line(cmd) : %s\n", uiSta & SDHCI_PSTA_CMD_INHIBIT ? "busy" : "free");
    printk("cmd line(dat) : %s\n", uiSta & SDHCI_PSTA_DATA_INHIBIT ? "busy" : "free");
    printk("dat line      : %s\n", uiSta & SDHCI_PSTA_DATA_ACTIVE ? "busy" : "free");
    printk("write active  : %s\n", uiSta & SDHCI_PSTA_DOING_WRITE ? "active" : "inactive");
    printk("read active   : %s\n", uiSta & SDHCI_PSTA_DOING_READ ? "active" : "inactive");
    printk("write buffer  : %s\n", uiSta & SDHCI_PSTA_SPACE_AVAILABLE ? "ready" : "not ready");
    printk("read  buffer  : %s\n", uiSta & SDHCI_PSTA_DATA_AVAILABLE ? "ready" : "not ready");
    printk("card insert   : %s\n", uiSta & SDHCI_PSTA_CARD_PRESENT ? "insert" : "not insert");
}
/*********************************************************************************************************
** 函数名称: __sdhciIntStaShow
** 功能描述: 显示中断状态
** 输    入: psdhcihostattr
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdhciIntStaShow (PLW_SDHCI_HOST_ATTR psdhcihostattr)
{
#ifndef printk
#define printk
#endif                                                                  /*  printk                      */

    UINT32  uiSta;

    uiSta = SDHCI_READL(psdhcihostattr, SDHCI_INT_STATUS);
    printk("\nhost int status >>\n");
    printk("cmd finish  : %s\n", uiSta & SDHCI_INT_RESPONSE ? "yes" : "no");
    printk("dat finish  : %s\n", uiSta & SDHCI_INT_DATA_END ? "yes" : "no");
    printk("dma finish  : %s\n", uiSta & SDHCI_INT_DMA_END ? "yes" : "no");
    printk("space avail : %s\n", uiSta & SDHCI_INT_SPACE_AVAIL ? "yes" : "no");
    printk("data avail  : %s\n", uiSta & SDHCI_INT_DATA_AVAIL ? "yes" : "no");
    printk("card insert : %s\n", uiSta & SDHCI_INT_CARD_INSERT ? "insert" : "not insert");
    printk("card remove : %s\n", uiSta & SDHCI_INT_CARD_REMOVE ? "remove" : "not remove");
}

#endif                                                                  /*  __SYLIXOS_DEBUG             */
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
