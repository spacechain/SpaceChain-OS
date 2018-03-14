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
** 文   件   名: i2cBus.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 10 月 19 日
**
** 描        述: i2c 总线模型(修改后的 linux i2c 子系统简化版).
*********************************************************************************************************/

#ifndef __I2CBUS_H
#define __I2CBUS_H

/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

/*********************************************************************************************************
  I2C 总线传输控制消息
*********************************************************************************************************/

typedef struct lw_i2c_message {
    UINT16                   I2CMSG_usAddr;                             /*  器件地址                    */
    UINT16                   I2CMSG_usFlag;                             /*  传输控制参数                */
    
#define LW_I2C_M_TEN           0x0010                                   /*  使用 10bit 设备地址         */
#define LW_I2C_M_RD            0x0001                                   /*  为读操作, 否则为写          */
#define LW_I2C_M_NOSTART       0x4000                                   /*  不发送 start 标志           */
#define LW_I2C_M_REV_DIR_ADDR  0x2000                                   /*  读写标志位反转              */
#define LW_I2C_M_IGNORE_NAK    0x1000                                   /*  忽略 ACK NACK               */
#define LW_I2C_M_NO_RD_ACK     0x0800                                   /*  读操作时不发送 ACK          */
#define LW_I2C_M_RECV_LEN      0x0400                                   /*  !目前不支持!                */
    
    UINT16                   I2CMSG_usLen;                              /*  长度(缓冲区大小)            */
    UINT8                   *I2CMSG_pucBuffer;                          /*  缓冲区                      */
} LW_I2C_MESSAGE;
typedef LW_I2C_MESSAGE      *PLW_I2C_MESSAGE;

/*********************************************************************************************************
  I2C 总线适配器
*********************************************************************************************************/

struct lw_i2c_funcs;
typedef struct lw_i2c_adapter {
    LW_BUS_ADAPTER           I2CADAPTER_pbusadapter;                    /*  总线节点                    */
    struct lw_i2c_funcs     *I2CADAPTER_pi2cfunc;                       /*  总线适配器操作函数          */
    
    LW_OBJECT_HANDLE         I2CADAPTER_hBusLock;                       /*  总线操作锁                  */
    ULONG                    I2CADAPTER_ulTimeout;                      /*  操作超时时间                */
    INT                      I2CADAPTER_iRetry;                         /*  重试次数                    */
    
    LW_LIST_LINE_HEADER      I2CADAPTER_plineDevHeader;                 /*  设备链表                    */
} LW_I2C_ADAPTER;
typedef LW_I2C_ADAPTER      *PLW_I2C_ADAPTER;

/*********************************************************************************************************
  I2C 总线传输函数集
*********************************************************************************************************/

typedef struct lw_i2c_funcs {
    INT                    (*I2CFUNC_pfuncMasterXfer)(PLW_I2C_ADAPTER   pi2cadapter,
                                                      PLW_I2C_MESSAGE   pi2cmsg,
                                                      INT               iNum);
                                                                        /*  适配器数据传输              */
    INT                    (*I2CFUNC_pfuncMasterCtl)(PLW_I2C_ADAPTER   pi2cadapter,
                                                     INT               iCmd,
                                                     LONG              lArg);
                                                                        /*  适配器控制                  */
} LW_I2C_FUNCS;
typedef LW_I2C_FUNCS        *PLW_I2C_FUNCS;

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
#endif                                                                  /*  __I2CBUS_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
