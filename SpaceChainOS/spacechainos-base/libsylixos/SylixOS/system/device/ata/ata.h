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
** 文   件   名: ata.h
**
** 创   建   人: Hou.XiaoLong (侯小龙)
**
** 文件创建日期: 2010 年 02 月 23 日
**
** 描        述: ATA 设备驱动头文件.
*********************************************************************************************************/

#ifndef __ATA_H
#define __ATA_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_ATA_EN > 0)
/*********************************************************************************************************
  定义驱动安装回调函数时的命令
*********************************************************************************************************/
#define ATA_CALLBACK_WRITE_DATA             1                           /*  安装写操作时的回调          */
#define ATA_CALLBACK_CHECK_DEV              2                           /*  安装检测设备是否存在时的回调*/
/*********************************************************************************************************
  用户配置标志:传输模式, 几何结构
*********************************************************************************************************/
#define ATA_PIO_DEF_0                       0x00                        /*  PIO默认模式                 */
#define ATA_PIO_DEF_1                       0x01                        /*  PIO默认模式, 没有IORDY      */
#define ATA_PIO_0                           0x08                        /*  PIO mode 0                  */
#define ATA_PIO_1                           0x09                        /*  PIO mode 1                  */
#define ATA_PIO_2                           0x0A                        /*  PIO mode 2                  */
#define ATA_PIO_3                           0x0B                        /*  PIO mode 3                  */
#define ATA_PIO_4                           0x0C                        /*  PIO mode 4                  */
#define ATA_PIO_AUTO                        0x000D                      /*  自动检测最高PIO模式         */
#define ATA_DMA_0                           0x0002                      /*  DMA mode 0,暂未支持         */
#define ATA_DMA_1                           0x0003                      /*  DMA mode 1,暂未支持         */
#define ATA_DMA_2                           0x0004                      /*  DMA mode 2,暂未支持         */
#define ATA_DMA_AUTO                        0x0005                      /*  自动检测最高DMA模式         */

#define ATA_BITS_8                          0x0040                      /*  RW bits size,8  bits        */
#define ATA_BITS_16                         0x0080                      /*  RW bits size,16 bits        */
#define ATA_BITS_32                         0x00C0                      /*  RW bits size,32 bits暂不支持*/

#define ATA_PIO_SINGLE                      0x0010                      /*  RW PIO单扇区读写            */
#define ATA_PIO_MULTI                       0x0020                      /*  RW PIO连续扇区读写          */

#define ATA_DMA_SINGLE                      0x0400                      /*  RW DMA single word,暂不支持 */
#define ATA_DMA_MULTI                       0x0800                      /*  RW DMA multi word,暂不支持  */
#define ATA_DMA_ULTRA                       0x1000                      /*  RW DMA ultra word,暂不支持  */

#define ATA_GEO_FORCE                       0x0100                      /*  set geometry in the table   */
#define ATA_GEO_PHYSICAL                    0x0200                      /*  set physical geometry       */
#define ATA_GEO_CURRENT                     0x0300                      /*  set current geometry        */
/*********************************************************************************************************
  ATA 驱动函数结构
*********************************************************************************************************/
#ifdef  __cplusplus
typedef INT     (*ATA_CALLBACK)(...);
#else
typedef INT     (*ATA_CALLBACK)();
#endif

typedef struct __ata_drv_funcs              ATA_DRV_FUNCS;

typedef struct __ata_chan {
    ATA_DRV_FUNCS    *pDrvFuncs;
} ATA_CHAN;                                                             /*  ATA 驱动结构体              */

struct __ata_drv_funcs {
    INT               (*ioctl)
                      (
                      ATA_CHAN  *patachan,
                      INT        iCmd,
                      PVOID      pvArg
                      );

    INT               (*ioOutByte)
                      (
                      ATA_CHAN  *patachan,
                      ULONG      ulIoAddr,
                      UINT       uiData
                      );

    INT               (*ioInByte)
                      (
                      ATA_CHAN  *patachan,
                      ULONG      ulIoAddr
                      );

    INT               (*ioOutWordString)
                      (
                      ATA_CHAN  *patachan,
                      ULONG      ulIoAddr,
                      INT16     *psBuff,
                      INT        iWord
                      );

    INT               (*ioInWordString)
                      (
                      ATA_CHAN  *patachan,
                      ULONG      ulIoAddr,
                      INT16     *psBuff,
                      INT        iWord
                      );

    INT               (*sysReset)
                      (
                      ATA_CHAN  *patachan,
                      INT        iDrive
                      );

    INT               (*callbackInstall)
                      (
                      ATA_CHAN     *patachan,
                      INT           iCallbackType,
                      ATA_CALLBACK  callback,
                      PVOID         pvCallbackArg
                      );
};
/*********************************************************************************************************
  ATA 控制器寄存器地址标示
*********************************************************************************************************/
typedef struct ata_reg {
    addr_t      ATAREG_ulData;                                          /*  (RW) data register (16 bits)*/
    addr_t      ATAREG_ulError;                                         /*  (R)  error register         */
    addr_t      ATAREG_ulFeature;                                       /*  (W) feature or write-precomp*/
    addr_t      ATAREG_ulSeccnt;                                        /*  (RW) sector count           */
    addr_t      ATAREG_ulSector;                                        /*  (RW) first sector number    */
    addr_t      ATAREG_ulCylLo;                                         /*  (RW) cylinder low byte      */
    addr_t      ATAREG_ulCylHi;                                         /*  (RW) cylinder high byte     */
    addr_t      ATAREG_ulSdh;                                           /*  (RW) sector size/drive/head */
    addr_t      ATAREG_ulCommand;                                       /*  (W)  command register       */
    addr_t      ATAREG_ulStatus;                                        /*  (R)  immediate status       */
    addr_t      ATAREG_ulAStatus;                                       /*  (R)  alternate status       */
    addr_t      ATAREG_ulDControl;                                      /*  (W)  disk controller control*/
} ATA_REG;
/*********************************************************************************************************
  ATA 设备通道参数
*********************************************************************************************************/
typedef struct atachan_paparm {
    INT         ATACP_iCtrlNum;                                         /*  控制器号                    */
    INT         ATACP_iDrives;                                          /*  设备个数                    */
    INT         ATACP_iBytesPerSector;                                  /*  每扇区字节数                */
    INT         ATACP_iConfigType;                                      /*  配置标志                    */
    BOOL        ATACP_bIntEnable;                                       /*  系统中断使能标志            */
    BOOL        ATACP_bPreadBeSwap;                                     /*  大端 CPU Pread 是否需要翻转 */
    ULONG       ATACP_ulSyncSemTimeout;                                 /*  同步等待超时时间(系统时钟)  */
    ATA_REG     ATACP_atareg;                                           /*  ATA任务文件寄存器           */
} ATA_CHAN_PARAM;
typedef ATA_CHAN_PARAM *PATA_CHAN_PARAM;
/*********************************************************************************************************
  API
*********************************************************************************************************/
LW_API INT            API_AtaDrv(ATA_CHAN  *patachan, ATA_CHAN_PARAM *patacp);
LW_API PLW_BLK_DEV    API_AtaDevCreate(INT iCtrl,
                                       INT iDrive,
                                       INT iNBlocks,
                                       INT iBlkOffset);
LW_API INT            API_AtaDevDelete(PLW_BLK_DEV pblkdev);

#define ataDrv        API_AtaDrv
#define ataDevCreate  API_AtaDevCreate
#define ataDevDelete  API_AtaDevDelete

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_ATA_EN > 0)         */
#endif                                                                  /*  __ATA_H                     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
