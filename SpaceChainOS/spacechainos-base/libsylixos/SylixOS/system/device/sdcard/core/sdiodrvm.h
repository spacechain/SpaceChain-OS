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
** 文   件   名: sdiodrvm.h
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2014 年 10 月 27 日
**
** 描        述: sd drv manager layer for sdio

** BUG:
*********************************************************************************************************/

#ifndef __SDIODRVM_H
#define __SDIODRVM_H

#include "sdcore.h"

/*********************************************************************************************************
  前置声明
*********************************************************************************************************/

struct sdio_drv;
struct sdio_dev_id;
struct sdio_cccr;
struct sdio_func_tuple;
struct sdio_func;
struct sdio_init_data;

typedef struct sdio_drv         SDIO_DRV;
typedef struct sdio_dev_id      SDIO_DEV_ID;
typedef struct sdio_cccr        SDIO_CCCR;
typedef struct sdio_func_tuple  SDIO_FUNC_TUPLE;
typedef struct sdio_func        SDIO_FUNC;
typedef struct sdio_init_data   SDIO_INIT_DATA;

/*********************************************************************************************************
  sdio 设备 相关定义
*********************************************************************************************************/

struct sdio_dev_id {
    UINT8    DEVID_ucClass;                                         /*  Std interface or SDIO_ANY_ID    */
    UINT16   DEVID_usVendor;                                        /*  Vendor or SDIO_ANY_ID           */
    UINT16   DEVID_usDevice;                                        /*  Device ID or SDIO_ANY_ID        */
    VOID    *DEVID_pvDrvPriv;                                       /*  driver private data             */
};
#define SDIO_DEV_ID_ANY     (~0)

struct sdio_cccr {
    UINT32    CCCR_uiSdioVsn;
    UINT32    CCCR_uiSdVsn;
    BOOL      CCCR_bMulBlk;
    BOOL      CCCR_bLowSpeed;                                       /*  低速卡(400khz)                  */
    BOOL      CCCR_bWideBus;                                        /*  在低速卡情况下，是否支持4位传输 */
    BOOL      CCCR_bHighPwr;
    BOOL      CCCR_bHighSpeed;                                      /*  高速卡(50Mhz)                   */
    BOOL      CCCR_bDisableCd;                                      /*  for sdio drv config             */
};

struct sdio_func_tuple {
    SDIO_FUNC_TUPLE *TUPLE_ptupleNext;
    UINT8            TUPLE_ucCode;
    UINT8            TUPLE_ucSize;
    UINT8            TUPLE_pucData[1];
};

struct sdio_func {
    UINT32           FUNC_uiNum;
    UINT8            FUNC_ucClass;
    UINT16           FUNC_usVendor;
    UINT16           FUNC_usDevice;

    ULONG            FUNC_ulMaxBlkSize;
    ULONG            FUNC_ulCurBlkSize;

    UINT32           FUNC_uiEnableTimeout;                          /*  in milli second                 */

    UINT32           FUNC_uiMaxDtr;                                 /*  仅功能0使用                     */

    SDIO_FUNC_TUPLE *FUNC_ptupleListHeader;                         /*  不能解析的TUPLE链表             */
    const SDIO_CCCR *FUNC_cpsdiocccr;                               /*  指向同一个CCCR                  */
};

#define SDIO_FUNC_MAX   8
struct sdio_init_data {
    SDIO_FUNC          INIT_psdiofuncTbl[SDIO_FUNC_MAX];
    INT                INIT_iFuncCnt;                               /*  不包括Func0                     */
    PLW_SDCORE_DEVICE  INIT_psdcoredev;
    SDIO_CCCR          INIT_sdiocccr;
    const SDIO_DEV_ID *INIT_pdevidCurr[SDIO_FUNC_MAX];              /*  当前设备的所有匹配的功能ID      */
};

/*********************************************************************************************************
  sdio 驱动
*********************************************************************************************************/

struct sdio_drv {
    LW_LIST_LINE  SDIODRV_lineManage;                               /*  驱动挂载链                      */
    CPCHAR        SDIODRV_cpcName;

    INT         (*SDIODRV_pfuncDevCreate)(SDIO_DRV         *psdiodrv,
                                          SDIO_INIT_DATA   *pinitdata,
                                          VOID            **ppvDevPriv);
    INT         (*SDIODRV_pfuncDevDelete)(SDIO_DRV *psdiodrv, VOID *pvDevPriv);
    VOID        (*SDIODRV_pfuncIrqHandle)(SDIO_DRV *psdiodrv, VOID *pvDevPriv);

    SDIO_DEV_ID  *SDIODRV_pdevidTbl;
    INT           SDIODRV_iDevidCnt;
    VOID         *SDIODRV_pvSpec;

    atomic_t      SDIODRV_atomicDevCnt;
};

/*********************************************************************************************************
  API
*********************************************************************************************************/

LW_API INT   API_SdmSdioLibInit(VOID);
LW_API INT   API_SdmSdioDrvRegister(SDIO_DRV *psdiodrv);
LW_API INT   API_SdmSdioDrvUnRegister(SDIO_DRV *psdiodrv);

#endif                                                              /*  __SDIODRVM_H                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
