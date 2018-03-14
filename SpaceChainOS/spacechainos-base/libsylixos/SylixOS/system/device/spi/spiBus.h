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
** 文   件   名: spiBus.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 11 月 08 日
**
** 描        述: spi 总线模型.

** BUG:
2010.11.17  spi message 加入 spi 地址信息, 总线驱动程序应该通过 spi 地址信息选择不同的片选信号.
            这样就支持了 spi 单总线多设备的模型. 与 i2c 类型 spi 设备的驱动程序就可以在不同的平台上复用.
2011.04.02  spi 片选和传输配合的情况太多, 所以 spi 地址片选由设备驱动来完成.
*********************************************************************************************************/

#ifndef __SPIBUS_H
#define __SPIBUS_H

/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

/*********************************************************************************************************
  SPI 总线传输控制消息
*********************************************************************************************************/

typedef struct lw_spi_message {
    UINT16                   SPIMSG_usBitsPerOp;                        /*  操作单位bits数              */
    UINT16                   SPIMSG_usFlag;                             /*  传输控制参数                */
    
#define LW_SPI_M_CPOL_0      0x0000                                     /*  CPOL 配置                   */
#define LW_SPI_M_CPOL_1      0x0001
#define LW_SPI_M_CPHA_0      0x0000                                     /*  CPHA 配置                   */
#define LW_SPI_M_CPHA_1      0x0002

#define LW_SPI_M_CPOL_EN     0x0004                                     /*  是否设置新的 CPOL 配置      */
                                                                        /*  否则 CPOL 配置与上次传输相同*/
#define LW_SPI_M_CPHA_EN     0x0008                                     /*  是否设置新的 CPHA 配置      */
                                                                        /*  否则 CPHA 配置与上次传输相同*/

/*********************************************************************************************************
  LW_SPI_M_WRBUF_FIX, LW_SPI_M_RDBUF_FIX 主要用于半双工的 SPI 操作 (多数从机器件都是这种操作模式).
  
  1: 当 LW_SPI_M_WRBUF_FIX 有效时, 表明本次操作是读操作, 每次发送的数据都是 SPIMSG_pucWrBuffer 的第一个字
     符, 而读到的数据将依次存放在 SPIMSG_pucRdBuffer 中.
     所以: 读缓冲的大小必须可以存放 SPIMSG_usLen 个字符, 而写缓冲的大小可以仅是 1 个字符长度.
  2: 当 LW_SPI_M_RDBUF_FIX 有效时, 表明本次操作是写操作, 对读到什么数据并不感兴趣, 所以每次读到的数据都放
     在 SPIMSG_pucRdBuffer 的第一个字符处.
     所以: 写缓冲的大小必须可以存放 SPIMSG_usLen 个字符, 而读缓冲的大小可以仅是 1 个字符长度.
*********************************************************************************************************/

#define LW_SPI_M_WRBUF_FIX   0x0010                                     /*  发送缓冲区仅发送第一个字符  */
                                                                        /*  永远发送第一个字符          */
#define LW_SPI_M_RDBUF_FIX   0x0020                                     /*  接收缓冲区仅接收第一个字符  */
                                                                        /*  接收的数据永远发在第一个接收*/
                                                                        /*  缓冲指针的位置              */

#define LW_SPI_M_MSB         0x0040                                     /*  从高位到低位                */
#define LW_SPI_M_LSB         0x0080                                     /*  从低位到高位                */

    UINT32                   SPIMSG_uiLen;                              /*  长度(缓冲区大小)            */
                                                                        /*  长度为0, 只设置传输控制参数 */
    
    UINT8                   *SPIMSG_pucWrBuffer;                        /*  发送缓冲区                  */
    UINT8                   *SPIMSG_pucRdBuffer;                        /*  接收缓冲区                  */
    
    VOIDFUNCPTR              SPIMSG_pfuncComplete;                      /*  传输结束后的回调函数        */
    PVOID                    SPIMSG_pvContext;                          /*  回调函数参数                */
} LW_SPI_MESSAGE;
typedef LW_SPI_MESSAGE      *PLW_SPI_MESSAGE;

/*********************************************************************************************************
  SPI 总线适配器
*********************************************************************************************************/

struct lw_spi_funcs;
typedef struct lw_spi_adapter {
    LW_BUS_ADAPTER           SPIADAPTER_pbusadapter;                    /*  总线节点                    */
    struct lw_spi_funcs     *SPIADAPTER_pspifunc;                       /*  总线适配器操作函数          */
    
    LW_OBJECT_HANDLE         SPIADAPTER_hBusLock;                       /*  总线操作锁                  */
    
    LW_LIST_LINE_HEADER      SPIADAPTER_plineDevHeader;                 /*  设备链表                    */
} LW_SPI_ADAPTER;
typedef LW_SPI_ADAPTER      *PLW_SPI_ADAPTER;

/*********************************************************************************************************
  SPI 总线传输 master ctl 命令
*********************************************************************************************************/

#define LW_SPI_CTL_BAUDRATE  FIOBAUDRATE                                /*  设置 baudrate               */
#define LW_SPI_CTL_USERFUNC  FIOUSRFUNC                                 /*  用户自定义功能              */

/*********************************************************************************************************
  SPI 总线传输函数集
*********************************************************************************************************/

typedef struct lw_spi_funcs {
    INT                    (*SPIFUNC_pfuncMasterXfer)(PLW_SPI_ADAPTER   pspiadapter,
                                                      PLW_SPI_MESSAGE   pspimsg,
                                                      INT               iNum);
                                                                        /*  适配器数据传输              */
    INT                    (*SPIFUNC_pfuncMasterCtl)(PLW_SPI_ADAPTER   pspiadapter,
                                                     INT               iCmd,
                                                     LONG              lArg);
                                                                        /*  适配器控制                  */
} LW_SPI_FUNCS;
typedef LW_SPI_FUNCS        *PLW_SPI_FUNCS;

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
#endif                                                                  /*  __SPIBUS_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
