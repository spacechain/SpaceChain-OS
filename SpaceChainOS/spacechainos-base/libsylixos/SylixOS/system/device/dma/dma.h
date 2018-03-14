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
** 文   件   名: dma.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 01 月 06 日
**
** 描        述: 通用 DMA 设备管理组件. 主要由设备驱动程序使用, 不建议用户应用程序直接使用.
                 如果系统中使用了 CACHE 或者 MMU, 在设备驱动程序中, 还需要使用操作系统提供的相关操作接口.
                 
** BUG
2010.12.27  加入 LW_DMA_TRANSACTION 中加入 DMAT_pvTransParam 字段, 可以扩展更为复杂的传输模式与参数.
            dma 抽象层并不处理此字段, 完全交给驱动程序使用, 一般与 DMAT_iTransMode 相关选项配合使用.
*********************************************************************************************************/

#ifndef __DMA_H
#define __DMA_H

/*********************************************************************************************************
  DMA 功能裁剪控制
*********************************************************************************************************/
#if (LW_CFG_MAX_DMA_CHANNELS > 0) && (LW_CFG_DMA_EN > 0)
/*********************************************************************************************************
  DMA 通道定义
*********************************************************************************************************/

#define LW_DMA_CHANNEL0        0                                        /*  DMA 通道 0                  */
#define LW_DMA_CHANNEL1        1                                        /*  DMA 通道 1                  */
#define LW_DMA_CHANNEL2        2                                        /*  DMA 通道 2                  */
#define LW_DMA_CHANNEL3        3                                        /*  DMA 通道 3                  */
#define LW_DMA_CHANNEL4        4                                        /*  DMA 通道 4                  */
#define LW_DMA_CHANNEL5        5                                        /*  DMA 通道 5                  */
#define LW_DMA_CHANNEL6        6                                        /*  DMA 通道 6                  */
#define LW_DMA_CHANNEL7        7                                        /*  DMA 通道 7                  */
#define LW_DMA_CHANNEL8        8                                        /*  DMA 通道 8                  */
#define LW_DMA_CHANNEL9        9                                        /*  DMA 通道 9                  */
#define LW_DMA_CHANNEL10       10                                       /*  DMA 通道 10                 */
#define LW_DMA_CHANNEL11       11                                       /*  DMA 通道 11                 */
#define LW_DMA_CHANNEL12       12                                       /*  DMA 通道 12                 */
#define LW_DMA_CHANNEL13       13                                       /*  DMA 通道 13                 */
#define LW_DMA_CHANNEL14       14                                       /*  DMA 通道 14                 */
#define LW_DMA_CHANNEL15       15                                       /*  DMA 通道 15                 */
#define LW_DMA_CHANNEL16       16                                       /*  DMA 通道 16                 */
#define LW_DMA_CHANNEL17       17                                       /*  DMA 通道 17                 */
#define LW_DMA_CHANNEL18       18                                       /*  DMA 通道 18                 */
#define LW_DMA_CHANNEL19       19                                       /*  DMA 通道 19                 */
#define LW_DMA_CHANNEL20       20                                       /*  DMA 通道 20                 */
#define LW_DMA_CHANNEL21       21                                       /*  DMA 通道 21                 */
#define LW_DMA_CHANNEL22       22                                       /*  DMA 通道 22                 */
#define LW_DMA_CHANNEL23       23                                       /*  DMA 通道 23                 */
#define LW_DMA_CHANNEL24       24                                       /*  DMA 通道 24                 */
#define LW_DMA_CHANNEL25       25                                       /*  DMA 通道 25                 */
#define LW_DMA_CHANNEL26       26                                       /*  DMA 通道 26                 */
#define LW_DMA_CHANNEL27       27                                       /*  DMA 通道 27                 */
#define LW_DMA_CHANNEL28       28                                       /*  DMA 通道 28                 */
#define LW_DMA_CHANNEL29       29                                       /*  DMA 通道 29                 */
#define LW_DMA_CHANNEL30       30                                       /*  DMA 通道 30                 */
#define LW_DMA_CHANNEL31       31                                       /*  DMA 通道 31                 */
#define LW_DMA_CHANNEL(n)      (n)

/*********************************************************************************************************
  DMA 状态定义
*********************************************************************************************************/

#define LW_DMA_STATUS_IDLE     0                                        /*  DMA 处于空闲模式            */
#define LW_DMA_STATUS_BUSY     1                                        /*  DMA 处于正在工作            */
#define LW_DMA_STATUS_ERROR    2                                        /*  DMA 处于错误状态            */

/*********************************************************************************************************
  DMA 地址方向控制
*********************************************************************************************************/

#define LW_DMA_ADDR_INC        0                                        /*  地址增长方式                */
#define LW_DMA_ADDR_FIX        1                                        /*  地址不变                    */
#define LW_DMA_ADDR_DEC        2                                        /*  地址减少方式                */

/*********************************************************************************************************
  DMA 传输参数
  
  DMA 操作的为物理地址, 所以 Src 和 Dest 地址均为物理地址.
  有些系统 CPU 体系构架的 CACHE 是使用虚拟地址作为索引的, 有些是使用物理地址做索引的.
  所以 DMA 软件层不处理任何 CACHE 相关的操作, 将这些操作留给驱动程序或应用程序完成.
  
  注意: 回调函数将可能会在中断上下文中执行.
*********************************************************************************************************/

typedef struct {
    UINT8                     *DMAT_pucSrcAddress;                      /*  源端缓冲区地址              */
    UINT8                     *DMAT_pucDestAddress;                     /*  目的端缓冲区地址            */
    
    size_t                     DMAT_stDataBytes;                        /*  传输的字节数                */
    
    INT                        DMAT_iSrcAddrCtl;                        /*  源端地址方向控制            */
    INT                        DMAT_iDestAddrCtl;                       /*  目的地址方向控制            */
    
    INT                        DMAT_iHwReqNum;                          /*  外设请求端编号              */
    BOOL                       DMAT_bHwReqEn;                           /*  是否为外设启动 DMA 传输     */
    BOOL                       DMAT_bHwHandshakeEn;                     /*  是否使用硬件握手            */
    
    INT                        DMAT_iTransMode;                         /*  传输模式, 自定义            */
    PVOID                      DMAT_pvTransParam;                       /*  传输参数, 自定义            */
    ULONG                      DMAT_ulOption;                           /*  体系结构相关参数            */
    
    PVOID                      DMAT_pvArgStart;                         /*  启动回调参数                */
    VOID                     (*DMAT_pfuncStart)(UINT     uiChannel,
                                                PVOID    pvArg);        /*  启动本次传输之前的回调函数  */
    
    PVOID                     *DMAT_pvArg;                              /*  回调函数参数                */
    VOID                     (*DMAT_pfuncCallback)(UINT     uiChannel,
                                                   PVOID    pvArg);     /*  本次传输完成后的回调函数    */
} LW_DMA_TRANSACTION;
typedef LW_DMA_TRANSACTION    *PLW_DMA_TRANSACTION;

/*********************************************************************************************************
  DMA 设备定义 (驱动程序接口)
*********************************************************************************************************/

typedef struct lw_dma_funcs {
    VOID                     (*DMAF_pfuncReset)(UINT     uiChannel,
                                                struct lw_dma_funcs *pdmafuncs);
                                                                        /*  复位 DMA 当前的操作         */
    INT                      (*DMAF_pfuncTrans)(UINT     uiChannel,
                                                struct lw_dma_funcs *pdmafuncs,
                                                PLW_DMA_TRANSACTION  pdmatMsg);
                                                                        /*  启动一次 DMA 传输           */
    INT                      (*DMAF_pfuncStatus)(UINT    uiChannel,
                                                 struct lw_dma_funcs *pdmafuncs);
                                                                        /*  获得 DMA 当前的工作状态     */
} LW_DMA_FUNCS;
typedef LW_DMA_FUNCS    *PLW_DMA_FUNCS;

/*********************************************************************************************************
  API 驱动安装函数
*********************************************************************************************************/

LW_API  INT     API_DmaDrvInstall(UINT              uiChannel,
                                  PLW_DMA_FUNCS     pdmafuncs,
                                  size_t            stMaxDataBytes);    /*  安装指定通道的 DMA 驱动程序 */
                                  
/*********************************************************************************************************
  API 接口函数
*********************************************************************************************************/

LW_API  INT     API_DmaReset(UINT   uiChannel);                         /*  复位指定的 DMA 通道         */

LW_API  INT     API_DmaJobNodeNum(UINT   uiChannel, 
                                  INT   *piNodeNum);                    /*  获得当前队列节点数          */

LW_API  INT     API_DmaMaxNodeNumGet(UINT   uiChannel, 
                                     INT   *piMaxNodeNum);              /*  获得最大队列节点数          */

LW_API  INT     API_DmaMaxNodeNumSet(UINT   uiChannel, 
                                     INT    iMaxNodeNum);               /*  设置最大队列节点数          */

LW_API  INT     API_DmaJobAdd(UINT                      uiChannel,
                              PLW_DMA_TRANSACTION       pdmatMsg);      /*  添加一个 DMA 传输请求       */
                              
LW_API  INT     API_DmaGetMaxDataBytes(UINT   uiChannel);               /*  获得一次可以传输的最大字节数*/

LW_API  INT     API_DmaFlush(UINT   uiChannel);                         /*  删除所有被延迟处理的传输请求*/

/*********************************************************************************************************
  API 中断服务函数
*********************************************************************************************************/

LW_API  INT     API_DmaContext(UINT   uiChannel);                       /*  DMA 传输完成后的中断处理函数*/


#define dmaDrv                  API_DmaDrvInstall
#define dmaReset                API_DmaReset
#define dmaMaxNodeNumGet        API_DmaMaxNodeNumGet
#define dmaMaxNodeNumSet        API_DmaMaxNodeNumSet
#define dmaJobAdd               API_DmaJobAdd
#define dmaGetMaxDataBytes      API_DmaGetMaxDataBytes
#define dmaFlush                API_DmaFlush

#endif                                                                  /*  LW_CFG_MAX_DMA_CHANNELS > 0 */
                                                                        /*  LW_CFG_DMA_EN   > 0         */
#endif                                                                  /*  __DMA_H                     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
