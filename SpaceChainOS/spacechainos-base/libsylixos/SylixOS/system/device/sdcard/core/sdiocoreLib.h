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
** 文   件   名: sdiocoreLib.h
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2014 年 10 月 28 日
**
** 描        述: sdio 卡特殊操作接口头文件

** BUG:
*********************************************************************************************************/

#ifndef __SDIOCORE_LIB_H
#define __SDIOCORE_LIB_H

/*********************************************************************************************************
  SDIO API
*********************************************************************************************************/

INT API_SdioCoreDevReset(PLW_SDCORE_DEVICE   psdcoredev);
INT API_SdioCoreDevSendIoOpCond(PLW_SDCORE_DEVICE   psdcoredev, UINT32 uiOcr, UINT32 *puiOcrOut);

INT API_SdioCoreDevReadFbr(PLW_SDCORE_DEVICE   psdcoredev, SDIO_FUNC *psdiofunc);
INT API_SdioCoreDevReadCCCR(PLW_SDCORE_DEVICE   psdcoredev, SDIO_CCCR *psdiocccr);

INT API_SdioCoreDevReadCis(PLW_SDCORE_DEVICE   psdcoredev, SDIO_FUNC *psdiofunc);
INT API_SdioCoreDevFuncClean(SDIO_FUNC *psdiofunc);

INT API_SdioCoreDevHighSpeedEn(PLW_SDCORE_DEVICE   psdcoredev, SDIO_CCCR *psdiocccr);
INT API_SdioCoreDevWideBusEn(PLW_SDCORE_DEVICE   psdcoredev, SDIO_CCCR *psdiocccr);

INT API_SdioCoreDevReadByte(PLW_SDCORE_DEVICE   psdcoredev,
                            UINT32              uiFn,
                            UINT32              uiAddr,
                            UINT8              *pucByte);
INT API_SdioCoreDevWriteByte(PLW_SDCORE_DEVICE   psdcoredev,
                             UINT32              uiFn,
                             UINT32              uiAddr,
                             UINT8               ucByte);
INT API_SdioCoreDevWriteThenReadByte(PLW_SDCORE_DEVICE   psdcoredev,
                                     UINT32              uiFn,
                                     UINT32              uiAddr,
                                     UINT8               ucWrByte,
                                     UINT8              *pucRdByte);

INT API_SdioCoreDevFuncEn(PLW_SDCORE_DEVICE   psdcoredev,
                          SDIO_FUNC          *psdiofunc);
INT API_SdioCoreDevFuncDis(PLW_SDCORE_DEVICE   psdcoredev,
                           SDIO_FUNC          *psdiofunc);

INT API_SdioCoreDevFuncIntEn(PLW_SDCORE_DEVICE   psdcoredev,
                             SDIO_FUNC          *psdiofunc);
INT API_SdioCoreDevFuncIntDis(PLW_SDCORE_DEVICE   psdcoredev,
                              SDIO_FUNC          *psdiofunc);

INT API_SdioCoreDevFuncBlkSzSet(PLW_SDCORE_DEVICE   psdcoredev,
                                SDIO_FUNC          *psdiofunc,
                                UINT32              uiBlkSz);

INT API_SdioCoreDevRwDirect(PLW_SDCORE_DEVICE   psdcoredev,
                            BOOL                bWrite,
                            UINT32              uiFn,
                            UINT32              uiAddr,
                            UINT8               ucWrData,
                            UINT8              *pucRdBack);

INT API_SdioCoreDevRwExtend(PLW_SDCORE_DEVICE   psdcoredev,
                            BOOL                bWrite,
                            UINT32              uiFn,
                            UINT32              uiAddr,
                            BOOL                bAddrInc,
                            UINT8              *pucBuf,
                            UINT32              uiBlkCnt,
                            UINT32              uiBlkSz);

INT API_SdioCoreDevRwExtendX(PLW_SDCORE_DEVICE   psdcoredev,
                             BOOL                bWrite,
                             UINT32              uiFn,
                             BOOL                bIsBlkMode,
                             UINT32              uiAddr,
                             BOOL                bAddrInc,
                             UINT8              *pucBuf,
                             UINT32              uiBlkCnt,
                             UINT32              uiBlkSz);

#endif                                                              /*  __SDIOCORE_LIB_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
