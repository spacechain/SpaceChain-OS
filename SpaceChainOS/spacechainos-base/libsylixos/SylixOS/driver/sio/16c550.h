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
** 文   件   名: 16c550.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2015 年 08 月 27 日
**
** 描        述: 16c550 兼容串口驱动支持.
*********************************************************************************************************/

#ifndef __16C550_H
#define __16C550_H

#include "SylixOS.h"
#include "sys/ioctl.h"

/*********************************************************************************************************
  16c550 register offset
*********************************************************************************************************/

#define RBR     0x00                                            /* receiver buffer register             */
#define THR     0x00                                            /* transmit holding register            */
#define DLL     0x00                                            /* divisor latch                        */
#define IER     0x01                                            /* interrupt enable register            */
#define DLM     0x01                                            /* divisor latch(MS)                    */
#define IIR     0x02                                            /* interrupt identification register    */
#define FCR     0x02                                            /* FIFO control register                */
#define LCR     0x03                                            /* line control register                */
#define MCR     0x04                                            /* modem control register               */
#define LSR     0x05                                            /* line status register                 */
#define MSR     0x06                                            /* modem status register                */
#define SCR     0x07                                            /* scratch register                     */

#define BAUD_LO(uart_freq, baud)  ((uart_freq / (16 * baud)) & 0xff)
#define BAUD_HI(uart_freq, baud)  (((uart_freq / (16 * baud)) & 0xff00) >> 8)

/*********************************************************************************************************
  Line Control Register
*********************************************************************************************************/

#define CHAR_LEN_5  0x00                                        /* 5bits data size                      */
#define CHAR_LEN_6  0x01                                        /* 6bits data size                      */
#define CHAR_LEN_7  0x02                                        /* 7bits data size                      */
#define CHAR_LEN_8  0x03                                        /* 8bits data size                      */
#define LCR_STB     0x04                                        /* 2 stop bits                          */
#define ONE_STOP    0x00                                        /* one stop bit                         */
#define LCR_PEN     0x08                                        /* parity enable                        */
#define PARITY_NONE 0x00                                        /* parity disable                       */
#define LCR_EPS     0x10                                        /* even parity select                   */
#define LCR_SP      0x20                                        /* stick parity select                  */
#define LCR_SBRK    0x40                                        /* break control bit                    */
#define LCR_DLAB    0x80                                        /* divisor latch access enable          */
#define DLAB        LCR_DLAB

/*********************************************************************************************************
  Line Status Register
*********************************************************************************************************/

#define LSR_DR          0x01                                    /* data ready                           */
#define RxCHAR_AVAIL    LSR_DR                                  /* data ready                           */
#define LSR_OE          0x02                                    /* overrun error                        */
#define LSR_PE          0x04                                    /* parity error                         */
#define LSR_FE          0x08                                    /* framing error                        */
#define LSR_BI          0x10                                    /* break interrupt                      */
#define LSR_THRE        0x20                                    /* transmit holding register empty      */
#define LSR_TEMT        0x40                                    /* transmitter empty                    */
#define LSR_FERR        0x80                                    /* fifo mode, set when PE,FE or BI err  */

/*********************************************************************************************************
  Interrupt Identification Register
*********************************************************************************************************/

#define IIR_IP      0x01
#define IIR_ID      0x0e
#define IIR_RLS     0x06                                        /* received line status                 */
#define Rx_INT      IIR_RLS                                     /* received line status                 */
#define IIR_RDA     0x04                                        /* received data available              */
#define RxFIFO_INT  IIR_RDA                                     /* received data available              */
#define IIR_THRE    0x02                                        /* transmit holding register empty      */
#define TxFIFO_INT  IIR_THRE
#define IIR_MSTAT   0x00                                        /* modem status                         */
#define IIR_TIMEOUT 0x0c                                        /* char receive timeout                 */

/*********************************************************************************************************
  Interrupt Enable Register
*********************************************************************************************************/

#define IER_ERDAI   0x01                                        /* received data avail. & timeout int   */
#define RxFIFO_BIT  IER_ERDAI
#define IER_ETHREI  0x02                                        /* transmit holding register empty int  */
#define TxFIFO_BIT  IER_ETHREI
#define IER_ELSI    0x04                                        /* receiver line status int enable      */
#define Rx_BIT      IER_ELSI
#define IER_EMSI    0x08                                        /* modem status int enable              */

/*********************************************************************************************************
  Modem Control Register
*********************************************************************************************************/

#define MCR_DTR     0x01                                        /* dtr output                           */
#define DTR         MCR_DTR
#define MCR_RTS     0x02                                        /* rts output                           */
#define MCR_OUT1    0x04                                        /* output #1                            */
#define MCR_OUT2    0x08                                        /* output #2                            */
#define MCR_LOOP    0x10                                        /* loopback enable                      */

/*********************************************************************************************************
  Modem Status Register
*********************************************************************************************************/

#define MSR_DCTS    0x01                                        /* cts change                           */
#define MSR_DDSR    0x02                                        /* dsr change                           */
#define MSR_TERI    0x04                                        /* ring indicator change                */
#define MSR_DDCD    0x08                                        /* data carrier indicator change        */
#define MSR_CTS     0x10                                        /* complement of cts                    */
#define MSR_DSR     0x20                                        /* complement of dsr                    */
#define MSR_RI      0x40                                        /* complement of ring signal            */
#define MSR_DCD     0x80                                        /* complement of dcd                    */

/*********************************************************************************************************
  FIFO Control Register
*********************************************************************************************************/

#define FCR_EN          0x01                                    /* enable xmit and rcvr                 */
#define FIFO_ENABLE     FCR_EN
#define FCR_RXCLR       0x02                                    /* clears rcvr fifo                     */
#define RxCLEAR         FCR_RXCLR
#define FCR_TXCLR       0x04                                    /* clears xmit fifo                     */
#define TxCLEAR         FCR_TXCLR
#define FCR_DMA         0x08                                    /* dma                                  */
#define FCR_RXTRIG_L    0x40                                    /* rcvr fifo trigger lvl low            */
#define FCR_RXTRIG_H    0x80                                    /* rcvr fifo trigger lvl high           */

/*********************************************************************************************************
  sio channel
*********************************************************************************************************/

typedef struct sio16c550_chan SIO16C550_CHAN;

struct sio16c550_chan {
    SIO_DRV_FUNCS   *pdrvFuncs;
    
    LW_SPINLOCK_DEFINE  (slock);

    int (*pcbGetTxChar)();
    int (*pcbPutRcvChar)();

    void *getTxArg;
    void *putRcvArg;

    int channel_mode;                                           /* SIO_MODE_INT or SIO_MODE_POLL        */
    int switch_en;                                              /* RS-485 switch pin operate enable flag*/
    int hw_option;                                              /* hardware setup options               */

    UINT8   ier;                                                /* copy of interrupt enable register    */
    UINT8   lcr;                                                /* copy of line control register        */
    UINT8   mcr;                                                /* copy of modem control register       */
    UINT8   lsr;                                                /* copy of line status register         */
    
    BOOL    bdefer;
    
    ULONG   err_overrun;                                        /* err counter                          */
    ULONG   err_parity;
    ULONG   err_framing;
    ULONG   err_break;
    
    /*
     *  user MUST set following members before calling this module api.
     */
    PLW_JOB_QUEUE pdeferq;                                      /* ISR defer queue                      */
                                                                /* defer queue interrupt recommended!   */
    unsigned long xtal;                                         /* uart clock frequency                 */
    unsigned long baud;                                         /* init-baud rate                       */
    
    int fifo_len;                                               /* FIFO lenght                          */
    
    /*
     *  "Usually" Receiver FIFO Trigger Level and Tirgger bytes table
     *
     *  ***!!! YOU MUST CHECK DATASHEET !!!***
     *
     *  level  16 Bytes FIFO Trigger   32 Bytes FIFO Trigger  64 Bytes FIFO Trigger
     *    0              1                       8                    1
     *    1              4                      16                   16
     *    2              8                      24                   32
     *    3             14                      28                   56
     */
    int rx_trigger_level;                                       /* FIFO rx int trigger level (0 ~ 3)    */

    UINT16 iobase;                                              /* iobase address, only for x86 io space*/
                                                                /* (set 0 usually)                      */
    void (*setreg)(SIO16C550_CHAN *, int reg, UINT8 val);       /* set register value                   */
    UINT8 (*getreg)(SIO16C550_CHAN *, int reg);                 /* get register value                   */

    /*
     *  RS-485 mode
     */
    void (*delay)(SIO16C550_CHAN *);                            /* switch operate a little delay        */
    void (*send_start)(SIO16C550_CHAN *);                       /* tx start, switch direct to send mode */
    void (*send_end)(SIO16C550_CHAN *);                         /* tx end, switch direct pin to rx mode */

    void *priv;                                                 /* user can use this save some thing    */
};

/*********************************************************************************************************
  additional ioctl commands
  ioctl(fd, SIO_SWITCH_PIN_EN_SET, 1); RS-485 mode
  ioctl(fd, SIO_SWITCH_PIN_EN_SET, 0); RS-232/422 mode
*********************************************************************************************************/

#ifndef SIO_SWITCH_PIN_EN_SET
#define SIO_SWITCH_PIN_EN_SET   _IOW('s', 1, INT)
#define SIO_SWITCH_PIN_EN_GET   _IOR('s', 1, INT)
#endif

/*********************************************************************************************************
  16c550 driver functions
*********************************************************************************************************/

INT  sio16c550Init(SIO16C550_CHAN *psiochan);
VOID sio16c550Isr(SIO16C550_CHAN *psiochan);

#endif                                                          /*  __16C550_H                          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
