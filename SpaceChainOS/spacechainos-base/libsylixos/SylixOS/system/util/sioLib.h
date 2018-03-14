/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: sioLib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 03 月 07 日
**
** 描        述: 这是系统 SIO 库
*********************************************************************************************************/

#ifndef  __SIOLIB_H
#define  __SIOLIB_H

/*********************************************************************************************************
  serial device I/O controls
*********************************************************************************************************/

#define SIO_BAUD_SET                          LW_OSIOD('s', 0x1003, LONG)
#define SIO_BAUD_GET                          LW_OSIOR('s', 0x1004, LONG)

#define SIO_HW_OPTS_SET                       LW_OSIOD('s', 0x1005, INT)
#define SIO_HW_OPTS_GET                       LW_OSIOR('s', 0x1006, INT)

#define SIO_MODE_SET                          LW_OSIOD('s', 0x1007, INT)
#define SIO_MODE_GET                          LW_OSIOR('s', 0x1008, INT)
#define SIO_AVAIL_MODES_GET                   LW_OSIOR('s', 0x1009, INT)

/*********************************************************************************************************
  These are used to communicate open/close between layers
*********************************************************************************************************/

#define SIO_OPEN                              LW_OSIO('s', 0x100a)
#define SIO_HUP                               LW_OSIO('s', 0x100b)

/*********************************************************************************************************
  The ioctl commands listed below provide a way for reading and
  setting the modem lines. 
 
  * SIO_MSTAT_GET: returns status of all input and output modem signals.
  * SIO_MCTRL_BITS_SET: sets modem signals specified in argument.
  * SIO_MCTRL_BITS_CLR: clears modem signal(s) specified in argument.
  * SIO_MCTRL_ISIG_MASK: returns mask of all input modem signals.
  * SIO_MCTRL_OSIG_MASK: returns mask of all output(writable) modem signals.
*********************************************************************************************************/

#define SIO_MSTAT_GET                         LW_OSIO( 's', 0x100c)     /* get modem status lines       */
#define SIO_MCTRL_BITS_SET                    LW_OSIOD('s', 0x100d, INT)/* set selected modem lines     */
#define SIO_MCTRL_BITS_CLR                    LW_OSIOD('s', 0x100e, INT)/* clear selected modem lines   */
#define SIO_MCTRL_ISIG_MASK                   LW_OSIOR('s', 0x100f, INT)/* mask of lines that can be rd */
#define SIO_MCTRL_OSIG_MASK                   LW_OSIOR('s', 0x1010, INT)/* mask of lines that can be set*/

/*********************************************************************************************************
  Ioctl cmds for reading/setting keyboard mode
*********************************************************************************************************/

#define SIO_KYBD_MODE_SET                     LW_OSIOD('s', 0x1011, INT)
#define SIO_KYBD_MODE_GET                     LW_OSIOR('s', 0x1012, INT)

/*********************************************************************************************************
  options to SIO_KYBD_MODE_SET 
 
  * These macros are used as arguments by the keyboard ioctl comands.
  * When used with SIO_KYBD_MODE_SET they mean the following:
  * 
  * SIO_KYBD_MODE_SYLIXOS
  *    Requests the keyboard driver to return keyboard.h format.
  *
  * SIO_KYBD_MODE_RAW:
  *    Requests the keyboard driver to return raw key values as read
  *    by the keyboard controller.
  * 
  * SIO_KYBD_MODE_ASCII:
  *    Requests the keyboard driver to return ASCII codes read from a
  *    known table that maps raw key values to ASCII.
  *
  * SIO_KYBD_MODE_UNICODE:
  *    Requests the keyboard driver to return 16 bit UNICODE values for
  *    multiple languages support.
*********************************************************************************************************/

#define SIO_KYBD_MODE_SYLIXOS                 0
#define SIO_KYBD_MODE_RAW                     1
#define SIO_KYBD_MODE_ASCII                   2
#define SIO_KYBD_MODE_UNICODE                 3

/*********************************************************************************************************
  Ioctl cmds for reading/setting keyboard led state
*********************************************************************************************************/

#define SIO_KYBD_LED_SET                      LW_OSIOD('s', 0x1013, INT)
#define SIO_KYBD_LED_GET                      LW_OSIOR('s', 0x1014, INT)

/*********************************************************************************************************
  options to SIO_KYBD_LED_SET
 
  * These macros are used as arguments by the keyboard ioctl comands.
  * When used with SIO_KYBD_LED_SET they mean the following:
  * 
  * SIO_KYBD_LED_NUM:
  *    Sets the Num Lock LED on the keyboard - bit 1
  * 
  * SIO_KYBD_LED_CAP:
  *    Sets the Caps Lock LED on the keyboard - bit 2
  *
  * SIO_KYBD_LED_SRC:
  *    Sets the Scroll Lock LED on the keyboard - bit 4
*********************************************************************************************************/

#define SIO_KYBD_LED_NUM                      1
#define SIO_KYBD_LED_CAP                      2
#define SIO_KYBD_LED_SCR                      4

/*********************************************************************************************************
  callback type codes
*********************************************************************************************************/

#define SIO_CALLBACK_GET_TX_CHAR              1
#define SIO_CALLBACK_PUT_RCV_CHAR             2
#define SIO_CALLBACK_ERROR                    3

/*********************************************************************************************************
  Error code
*********************************************************************************************************/

#define SIO_ERROR_NONE                        (-1)
#define SIO_ERROR_FRAMING                     0
#define SIO_ERROR_PARITY                      1
#define SIO_ERROR_OFLOW                       2
#define SIO_ERROR_UFLOW                       3
#define SIO_ERROR_CONNECT                     4
#define SIO_ERROR_DISCONNECT                  5
#define SIO_ERROR_NO_CLK                      6
#define SIO_ERROR_UNKNWN                      7

/*********************************************************************************************************
  options to SIO_MODE_SET
*********************************************************************************************************/

#define	SIO_MODE_POLL                         1
#define	SIO_MODE_INT                          2

/*********************************************************************************************************
  options to SIOBAUDSET
*********************************************************************************************************/

#define SIO_BAUD_110                          110l
#define SIO_BAUD_300                          300l
#define SIO_BAUD_600                          600l
#define SIO_BAUD_1200                         1200l
#define SIO_BAUD_2400                         2400l
#define SIO_BAUD_4800                         4800l
#define SIO_BAUD_9600                         9600l
#define SIO_BAUD_14400                        14400l
#define SIO_BAUD_19200                        19200l
#define SIO_BAUD_38400                        38400l
#define SIO_BAUD_56000                        56000l
#define SIO_BAUD_57600                        57600l
#define SIO_BAUD_115200                       115200l
#define SIO_BAUD_128000                       128000l
#define SIO_BAUD_256000                       256000l

/*********************************************************************************************************
  options to SIO_HW_OPTS_SET (ala POSIX), bitwise or'ed together
*********************************************************************************************************/

#define CLOCAL                                0x1                       /* ignore modem status lines    */
#define CREAD                                 0x2                       /* enable device reciever       */

#define CSIZE                                 0xc                       /* CSxx mask                    */
#define CS5                                   0x0                       /* 5 bits                       */
#define CS6                                   0x4                       /* 6 bits                       */
#define CS7                                   0x8                       /* 7 bits                       */
#define CS8                                   0xc                       /* 8 bits                       */

#define HUPCL                                 0x10                      /* hang up on last close        */
#define STOPB                                 0x20                      /* send two stop bits (else one)*/
#define PARENB                                0x40                      /* parity detection enabled     */
                                                                        /* else disabled                */
#define PARODD                                0x80                      /* odd parity (else even)       */

/*********************************************************************************************************
  Modem signals definitions
 
  * The following six macros define the different modem signals. They
  * are used as arguments to the modem ioctls commands to specify 
  * the signals to read(set) and also to parse the returned value. They
  * provide hardware independence, as modem signals bits vary from one 
  * chip to another.
*********************************************************************************************************/

#define SIO_MODEM_DTR                         0x01                      /* data terminal ready          */
#define SIO_MODEM_RTS                         0x02                      /* request to send              */
#define SIO_MODEM_CTS                         0x04                      /* clear to send                */
#define SIO_MODEM_CD                          0x08                      /* carrier detect               */
#define SIO_MODEM_RI                          0x10                      /* ring                         */
#define SIO_MODEM_DSR                         0x20                      /* data set ready               */

/*********************************************************************************************************
  CALL BACK
*********************************************************************************************************/

#ifdef  __cplusplus
typedef INT     (*VX_SIO_CALLBACK)(...);
#else
typedef INT     (*VX_SIO_CALLBACK)();
#endif

/*********************************************************************************************************
  VXWORKS DATA STRUCT 
*********************************************************************************************************/

typedef struct sio_drv_funcs                       SIO_DRV_FUNCS;

typedef struct sio_chan {                                               /*  a serial channel            */
    
    SIO_DRV_FUNCS    *pDrvFuncs;
    
} SIO_CHAN;

struct sio_drv_funcs {                                                  /*  driver functions            */
    
    INT               (*ioctl)
                      (
                      SIO_CHAN    *pSioChan,
                      INT          cmd,
                      PVOID        arg
                      );
                      
    INT               (*txStartup)
                      (
                      SIO_CHAN    *pSioChan
                      );
                      
    INT               (*callbackInstall)
                      (
                      SIO_CHAN          *pSioChan,
                      INT                callbackType,
                      VX_SIO_CALLBACK    callback,
                      PVOID              callbackArg
                      );
                      
    INT               (*pollInput)
                      (
                      SIO_CHAN    *pSioChan,
                      PCHAR        inChar
                      );
                      
    INT               (*pollOutput)
                      (
                      SIO_CHAN    *pSioChan,
                      CHAR         outChar
                      );
};

/*********************************************************************************************************
  in-line macros
*********************************************************************************************************/

#define sioIoctl(pSioChan, cmd, arg) \
        ((pSioChan)->pDrvFuncs->ioctl(pSioChan, cmd, arg))
        
#define sioTxStartup(pSioChan) \
        ((pSioChan)->pDrvFuncs->txStartup(pSioChan))
        
#define sioCallbackInstall(pSioChan, callbackType, callback, callbackArg) \
	    ((pSioChan)->pDrvFuncs->callbackInstall(pSioChan, callbackType, \
                                                callback, callbackArg))
			
#define sioPollInput(pSioChan, inChar) \
        ((pSioChan)->pDrvFuncs->pollInput(pSioChan, inChar))
        
#define sioPollOutput(pSioChan, thisChar) \
        ((pSioChan)->pDrvFuncs->pollOutput(pSioChan, thisChar))

#endif                                                                  /*  __SIOLIB_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
