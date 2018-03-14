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
** 文   件   名: sja1000.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2015 年 08 月 27 日
**
** 描        述: SJA1000 CAN 总线驱动支持.
*********************************************************************************************************/

#ifndef __SJA1000_H
#define __SJA1000_H

#include "SylixOS.h"

/*********************************************************************************************************
  use 16MHz standar baudrate
*********************************************************************************************************/

#define BTR_1000K       0x0014U
#define BTR_800K        0x0016U
#define BTR_666K        0x80B6U
#define BTR_500K        0x001CU
#define BTR_400K        0x80FAU
#define BTR_250K        0x011CU
#define BTR_200K        0x81FAU
#define BTR_125K        0x031CU
#define BTR_100K        0x041CU
#define BTR_80K         0x83FFU
#define BTR_50K         0x091CU
#define BTR_40K         0x87FFU
#define BTR_20K         0x181CU
#define BTR_10K         0x311CU
#define BTR_5K          0xBFFFU
#define BTR_30K         0x8D4CU
#define BTR_600K        0x40A8U                                 /* actually 615.4K                      */
#define BTR_700K        0x40A6U                                 /* actually 727  K                      */
#define BTR_900K        0x4014U                                 /* actually 1000 K                      */

/*********************************************************************************************************
  interrupte enable register
*********************************************************************************************************/

#define IER_RIE         (0x1u)                                  /* enable recv int                      */
#define IER_TIR         (0x1u << 1)                             /* enable transmit int                  */
#define IER_EIR         (0x1u << 2)                             /* enable error int                     */
#define IER_DOIE        (0x1u << 3)                             /* enable data overflow int             */
#define IER_WUIE        (0x1u << 4)                             /* enable wakeup int                    */
#define IER_EPIE        (0x1u << 5)                             /* enable error comfirm int             */
#define IER_ALIE        (0x1u << 6)                             /* enable arbitration loss int          */
#define IER_BEIE        (0x1u << 7)                             /* enable bus error int                 */
#define IER_ALL         (0x0FFu)                                /* enable all int                       */

/*********************************************************************************************************
  can mode
*********************************************************************************************************/

#define BAIS_CAN        0x00                                    /* basic can mode                       */
#define PELI_CAN        0x01                                    /* peli can mode                        */

/*********************************************************************************************************
  sja1000 register
*********************************************************************************************************/

#define MOD             0
#define CMR             1
#define SR              2
#define IR              3
#define IER             4
#define BTR0            6
#define BTR1            7
#define OCR             8
#define TEST            9
#define ALC             11
#define ECC             12
#define EWL             13
#define RXERR           14
#define TXERR           15
#define TXBUF           16
#define RXBUF           16
#define ACR0            16
#define AMR0            20
#define CDR             31

#define BASIC_ACR       4                                       /* BasicCAN                             */
#define BASIC_AMR       5

/*********************************************************************************************************
  mode register
*********************************************************************************************************/

#define MOD_RM          (0x1u)                                  /* reset mode 1:reset 0:normal          */
#define MOD_LOW         (0x1u << 1)                             /* listen only mode                     */
#define MOD_STM         (0x1u << 2)                             /* self test mode                       */
#define MOD_AFM         (0x1u << 3)                             /* acceptance filter mode 1:single 0:double */
#define MON_SM          (0x1u << 4)                             /* sleep mode                           */

#define MOD_BASIC_RIE   (0x01u)                                 /* BasicCAN                             */
#define MON_BASIC_TIE   (0x1u << 1)
#define M0N_BASIC_ELE   (0x1u << 2)
#define MON_BASIC_OLE   (0x1u << 3)

/*********************************************************************************************************
  command register
*********************************************************************************************************/

#define CMR_TR          (0x1u)                                  /* transmit command                     */
#define CMR_AT          (0x1u << 1)                             /* abort transmit                       */
#define CMR_RR          (0x1u << 2)                             /* release receive buffer               */
#define CMR_CDO         (0x1u << 3)                             /* clean data overflow                  */
#define CMR_SRR         (0x1u << 4)                             /* self receive require                 */

#define CMR_BASIC_TR    (0x1u)                                  /* BasicCAN                             */
#define CMR_BASIC_AT    (0x1u << 1)
#define CMR_BASIC_RRB   (0x1u << 2)
#define CMR_BASIC_CDO   (0x1u << 3)
#define CMR_BASIC_GTS   (0x1u << 4)

/*********************************************************************************************************
  status register
*********************************************************************************************************/

#define SR_RRS          (0x01u)
#define SR_DOS          (0x1u << 1)
#define SR_TBS          (0x1u << 2)
#define SR_TCS          (0x1u << 3)
#define SR_RS           (0x1u << 4)
#define SR_TS           (0x1u << 5)
#define SR_ES           (0x1u << 6)
#define SR_BS           (0x1u << 7)

/*********************************************************************************************************
  interrupt status register
*********************************************************************************************************/

#define IR_RI           (0x01u)
#define IR_TI           (0x1u << 1)
#define IR_EI           (0x1u << 2)
#define IR_DOI          (0x1u << 3)
#define IR_WUI          (0x1u << 4)
#define IR_EPI          (0x1u << 5)
#define IR_ALI          (0x1u << 6)
#define IR_BEI          (0x1u << 7)

/*********************************************************************************************************
  output control register
*********************************************************************************************************/

#define OCR_MODE0       (0x1u)
#define OCR_MODE1       (0x1u << 1)
#define OCR_POL0        (0x1u << 2)
#define OCR_TN0         (0x1u << 3)
#define OCR_TP0         (0x1u << 4)
#define OCR_POL1        (0x1u << 5)
#define OCR_TN1         (0x1u << 6)
#define OCR_TP1         (0x1u << 7)

/*********************************************************************************************************
  clock divide register
*********************************************************************************************************/

#define CDR_CLKOFF      (0x1u << 3)
#define CDR_RXINTEN     (0x1u << 5)
#define CDR_CBP         (0x1u << 6)
#define CDR_CANMOD      (0x1u << 7)

/*********************************************************************************************************
  sja1000 filter
*********************************************************************************************************/

typedef struct sja1000_filter {
    UINT32    acr_code;
    UINT32    amr_code;
    int       mode;                                             /* 1:singel 0:double                    */
} SJA1000_FILTER;

/*********************************************************************************************************
  sja1000 can channel
*********************************************************************************************************/

typedef struct sja1000_chan SJA1000_CHAN;

struct sja1000_chan {
    CAN_DRV_FUNCS *pDrvFuncs;

    LW_SPINLOCK_DEFINE  (slock);

    INT (*pcbSetBusState)();                                    /* bus status callback                  */
    INT (*pcbGetTx)();                                          /* int callback                         */
    INT (*pcbPutRcv)();

    PVOID   pvSetBusStateArg;
    PVOID   pvGetTxArg;
    PVOID   pvPutRcvArg;

    unsigned long canmode;                                      /* BAIS_CAN or PELI_CAN                 */
    unsigned long baud;

    SJA1000_FILTER  filter;

    /*
     *  user MUST set following members before calling this module api.
     */
    unsigned long channel;

    void (*setreg)(SJA1000_CHAN *, int reg, UINT8 val);         /* set register value                   */
    UINT8 (*getreg)(SJA1000_CHAN *, int reg);                   /* get register value                   */

    /*
     *  you MUST clean cpu interrupt pending bit in reset()
     */
    void (*reset)(SJA1000_CHAN *);                              /* hardware reset sja1000               */
    void (*open)(SJA1000_CHAN *);
    void (*close)(SJA1000_CHAN *);

    void *priv;                                                 /* user can use this save some thing    */
};

/*********************************************************************************************************
  sja1000 driver functions
*********************************************************************************************************/

INT  sja1000Init(SJA1000_CHAN *pcanchan);
VOID sja1000Isr(SJA1000_CHAN *pcanchan);

#endif                                                          /*  __SJA1000_H                         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
