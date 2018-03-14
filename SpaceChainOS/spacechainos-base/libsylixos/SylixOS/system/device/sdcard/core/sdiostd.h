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
** 文   件   名: sdiostd.h
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2014 年 10 月 27 日
**
** 描        述: sdio 卡规范 v2.00 标准协议.
**               <<SD Specifications Part E1 SDIO Simplified Specification Version 2.00 February 8, 2007>>

** BUG:
*********************************************************************************************************/

#ifndef __SDIOSTD_H
#define __SDIOSTD_H

/*********************************************************************************************************
   标准SDIO命令 (v2.00)                                右边注释格式:   |  命令类型   参数      应答类型  |
   命令类型说明:
   bc   -  无应答的广播命令(broadcast command)
   bcr  -  有应答的广播命令(broadcast command with response)
   ac   -  指定地址的无应答的命令(addressed command)
   adtc -  指定地址且伴随数据传输的命令(addressed data-transfer command)
*********************************************************************************************************/

#define SDIO_SEND_OP_COND        5                                  /*  bcr  [23:0] OCR         R4      */

/*********************************************************************************************************
   SDIO_RW_DIRECT argument format:
    [31]    R/W flag
    [30:28] Function number
    [27]    RAW flag
    [25:9]  Register address
    [7:0]   Data
*********************************************************************************************************/

#define SDIO_RW_DIRECT          52                                  /*  ac   [31:0]             R5      */

/*********************************************************************************************************
   SDIO_RW_EXTENDED argument format:
    [31]    R/W flag
    [30:28] Function number
    [27]    Block mode
    [26]    Increment address
    [25:9]  Register address
    [8:0]   Byte/block count
*********************************************************************************************************/

#define SDIO_RW_EXTENDED        53                                  /*  adtc [31:0]             R5      */

/*********************************************************************************************************
  SDIO status in R5
  Type
    e : error bit
    s : status bit
    r : detected and set for the actual command response
    x : detected and set during command execution. the host must poll
        the card by sending status command in order to read these bits.
  Clear condition
    a : according to the card state
    b : always related to the previous command. Reception of
            a valid command will clear it (with a delay of one command)
    c : clear by read
*********************************************************************************************************/

#define R5_COM_CRC_ERROR        (1 << 15)                           /* er, b                            */
#define R5_ILLEGAL_CMD          (1 << 14)                           /* er, b                            */
#define R5_ERROR                (1 << 11)                           /* erx, c                           */
#define R5_FUNCTION_NUM         (1 << 9)                            /* er, c                            */
#define R5_OUT_OF_RANGE         (1 << 8)                            /* er, c                            */
#define R5_STATUS(x)            (x & 0xCB00)
#define R5_IO_CURRENT_STATE(x)  ((x & 0x3000) >> 12)                /* s, b                             */

/*********************************************************************************************************
   Card Common Control Registers (CCCR)
*********************************************************************************************************/

#define SDIO_CCCR_CCCR          0x00
#define SDIO_CCCR_REV_1_00      0                                   /* CCCR/FBR Version 1.00            */
#define SDIO_CCCR_REV_1_10      1                                   /* CCCR/FBR Version 1.10            */
#define SDIO_CCCR_REV_1_20      2                                   /* CCCR/FBR Version 1.20            */
#define SDIO_SDIO_REV_1_00      0                                   /* SDIO Spec Version 1.00           */
#define SDIO_SDIO_REV_1_10      1                                   /* SDIO Spec Version 1.10           */
#define SDIO_SDIO_REV_1_20      2                                   /* SDIO Spec Version 1.20           */
#define SDIO_SDIO_REV_2_00      3                                   /* SDIO Spec Version 2.00           */

#define SDIO_CCCR_SD            0x01
#define SDIO_SD_REV_1_01        0                                   /* SD Physical Spec Ver 1.01        */
#define SDIO_SD_REV_1_10        1                                   /* SD Physical Spec Ver 1.10        */
#define SDIO_SD_REV_2_00        2                                   /* SD Physical Spec Ver 2.00        */

#define SDIO_CCCR_IOEX          0x02
#define SDIO_CCCR_IORX          0x03

#define SDIO_CCCR_IENX          0x04                                /* Func/Master Interrupt Enable     */
#define SDIO_CCCR_INTX          0x05                                /* Function Interrupt Pending       */

#define SDIO_CCCR_ABORT         0x06                                /* function abort/card reset        */

#define SDIO_CCCR_IF            0x07                                /* bus interface controls           */
#define SDIO_BUS_WIDTH_1BIT     0x00
#define SDIO_BUS_WIDTH_4BIT     0x02
#define SDIO_BUS_WIDTH_MASK     0x03
#define SDIO_BUS_ECSI           0x20                                /* Enable continuous SPI interrupt  */
#define SDIO_BUS_SCSI           0x40                                /* Support continuous SPI interrupt */
#define SDIO_BUS_ASYNC_INT      0x20
#define SDIO_BUS_CD_DISABLE     0x80                                /* disable pull-up on DAT3 (pin 1)  */

#define SDIO_CCCR_CAPS          0x08
#define SDIO_CCCR_CAP_SDC       0x01                                /* can do CMD52 while data transfer */
#define SDIO_CCCR_CAP_SMB       0x02                                /* can do multi-block xfers (CMD53) */
#define SDIO_CCCR_CAP_SRW       0x04                                /* supports read-wait protocol      */
#define SDIO_CCCR_CAP_SBS       0x08                                /* supports suspend/resume          */
#define SDIO_CCCR_CAP_S4MI      0x10                                /* interrupt during 4-bit CMD53     */
#define SDIO_CCCR_CAP_E4MI      0x20                                /* enable ints during 4-bit CMD53   */
#define SDIO_CCCR_CAP_LSC       0x40                                /* low speed card                   */
#define SDIO_CCCR_CAP_4BLS      0x80                                /* 4 bit low speed card             */

#define SDIO_CCCR_CIS           0x09                                /* common CIS pointer (3 bytes)     */

/*
 * Following 4 regs are valid only if SBS is set
 */
#define SDIO_CCCR_SUSPEND       0x0c
#define SDIO_CCCR_SELx          0x0d
#define SDIO_CCCR_EXECx         0x0e
#define SDIO_CCCR_READYx        0x0f

#define SDIO_CCCR_BLKSIZE       0x10

#define SDIO_CCCR_POWER         0x12
#define SDIO_POWER_SMPC         0x01                                /* Supports Master Power Control    */
#define SDIO_POWER_EMPC         0x02                                /* Enable Master Power Control      */

#define SDIO_CCCR_SPEED         0x13
#define SDIO_SPEED_SHS          0x01                                /* Supports High-Speed mode         */
#define SDIO_SPEED_EHS          0x02                                /* Enable High-Speed mode           */

/*********************************************************************************************************
   Function Base Register (FBR)
*********************************************************************************************************/

#define SDIO_FBR_BASE(fn)       ((fn) * 0x100)                      /* base of function f's FBRs        */

#define SDIO_FBR_STD_IF         0x00
#define SDIO_FBR_SUPPORTS_CSA   0x40                                /* supports Code Storage Area       */
#define SDIO_FBR_ENABLE_CSA     0x80                                /* enable Code Storage Area         */

#define SDIO_FBR_STD_IF_EXT     0x01

#define SDIO_FBR_POWER          0x02
#define SDIO_FBR_POWER_SPS      0x01                                /* Supports Power Selection         */
#define SDIO_FBR_POWER_EPS      0x02                                /* Enable (low) Power Selection     */

#define SDIO_FBR_CIS            0x09                                /* CIS pointer (3 bytes)            */
#define SDIO_CIS_TPL_NULL       0x00
#define SDIO_CIS_TPL_END        0xff

#define SDIO_FBR_CSA            0x0C                                /* CSA pointer (3 bytes)            */
#define SDIO_FBR_CSA_DATA       0x0F
#define SDIO_FBR_BLKSIZE        0x10                                /* block size (2 bytes)             */

#endif                                                              /*  __SDIOSTD_H                     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
