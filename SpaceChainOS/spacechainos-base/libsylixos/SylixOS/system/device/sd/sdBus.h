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
** 文   件   名: sdBus.h
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2010 年 11 月 22 日
**
** 描        述: sd 总线模型.

** BUG:
2011.01.10  增加SPI模式下的相关定义.
2014.11.16  增加两个适配器 ioctl 命令, 用户停止/启动时钟.
2015.09.22  增加8位和DDR总线设置的命令.
*********************************************************************************************************/

#ifndef __SDBUS_H
#define __SDBUS_H

/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

/*********************************************************************************************************
   SD应答类型位定义
*********************************************************************************************************/

#define SD_RSP_PRESENT  (1 << 0)                                        /*  有应答                      */
#define SD_RSP_136      (1 << 1)                                        /*  136 位的应答                */
#define SD_RSP_CRC      (1 << 2)                                        /*  应答中包含有效CRC           */
#define SD_RSP_BUSY     (1 << 3)                                        /*  命令引起的应答是忙信号      */
#define SD_RSP_OPCODE   (1 << 4)                                        /*  应答中包含了发送的命令      */

/*********************************************************************************************************
   SD命令类型位定义 (非SPI模式下)
*********************************************************************************************************/

#define SD_CMD_MASK     (3 << 5)                                        /*  非SPI命令类型掩码           */
#define SD_CMD_AC       (0 << 5)                                        /*  带有指定地址 的命令         */
#define SD_CMD_ADTC     (1 << 5)                                        /*  带有地址,并伴随数据传输命令 */
#define SD_CMD_BC       (2 << 5)                                        /*  广播命令,无应答             */
#define SD_CMD_BCR      (3 << 5)                                        /*  广播命令 有应答             */

/*********************************************************************************************************
   SD应答类型位定义 (SPI模式)
*********************************************************************************************************/

#define SD_RSP_SPI_S1   (1 << 7)                                        /*  包含一个状态字节            */
#define SD_RSP_SPI_S2   (1 << 8)                                        /*  有第二个字节                */
#define SD_RSP_SPI_B4   (1 << 9)                                        /*  有四个字节                  */
#define SD_RSP_SPI_BUSY (1 << 10)                                       /*  卡发送的忙信号              */
#define SD_RSP_SPI_MASK (0x0f << 7)
/*********************************************************************************************************
   SD标准规范应答类型定义 (标准V3.01)
*********************************************************************************************************/

#define SD_RSP_NONE     (0)                                             /*  0表示没有应答               */
#define SD_RSP_R1       (SD_RSP_PRESENT | SD_RSP_CRC | SD_RSP_OPCODE)
#define SD_RSP_R1B      (SD_RSP_PRESENT | SD_RSP_CRC | SD_RSP_OPCODE | SD_RSP_BUSY)
#define SD_RSP_R2       (SD_RSP_PRESENT | SD_RSP_136 | SD_RSP_CRC)
#define SD_RSP_R3       (SD_RSP_PRESENT)
#define SD_RSP_R4       (SD_RSP_PRESENT)
#define SD_RSP_R5       (SD_RSP_PRESENT | SD_RSP_CRC | SD_RSP_OPCODE)
#define SD_RSP_R6       (SD_RSP_PRESENT | SD_RSP_CRC | SD_RSP_OPCODE)
#define SD_RSP_R7       (SD_RSP_PRESENT | SD_RSP_CRC | SD_RSP_OPCODE)

#define SD_RSP_SPI_R1   (SD_RSP_SPI_S1)
#define SD_RSP_SPI_R1B  (SD_RSP_SPI_S1 | SD_RSP_SPI_BUSY)
#define SD_RSP_SPI_R2   (SD_RSP_SPI_S1 | SD_RSP_SPI_S2)
#define SD_RSP_SPI_R3   (SD_RSP_SPI_S1 | SD_RSP_SPI_B4)
#define SD_RSP_SPI_R4   (SD_RSP_SPI_S1 | SD_RSP_SPI_B4)
#define SD_RSP_SPI_R5   (SD_RSP_SPI_S1 | SD_RSP_SPI_S2)
#define SD_RSP_SPI_R7   (SD_RSP_SPI_S1 | SD_RSP_SPI_B4)

/*********************************************************************************************************
   MMC status in R1, for native mode (SPI bits are different)
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

#define R1_OUT_OF_RANGE         (1 << 31)                               /*  er, c                       */
#define R1_ADDRESS_ERROR        (1 << 30)                               /*  erx, c                      */
#define R1_BLOCK_LEN_ERROR      (1 << 29)                               /*  er, c                       */
#define R1_ERASE_SEQ_ERROR      (1 << 28)                               /*  er, c                       */
#define R1_ERASE_PARAM          (1 << 27)                               /*  ex, c                       */
#define R1_WP_VIOLATION         (1 << 26)                               /*  erx, c                      */
#define R1_CARD_IS_LOCKED       (1 << 25)                               /*  sx, a                       */
#define R1_LOCK_UNLOCK_FAILED   (1 << 24)                               /*  erx, c                      */
#define R1_COM_CRC_ERROR        (1 << 23)                               /*  er, b                       */
#define R1_ILLEGAL_COMMAND      (1 << 22)                               /*  er, b                       */
#define R1_CARD_ECC_FAILED      (1 << 21)                               /*  ex, c                       */
#define R1_CC_ERROR             (1 << 20)                               /*  erx, c                      */
#define R1_ERROR                (1 << 19)                               /*  erx, c                      */
#define R1_UNDERRUN             (1 << 18)                               /*  ex, c                       */
#define R1_OVERRUN              (1 << 17)                               /*  ex, c                       */
#define R1_CID_CSD_OVERWRITE    (1 << 16)                               /*  erx, c, CID/CSD overwrite   */
#define R1_WP_ERASE_SKIP        (1 << 15)                               /*  sx, c                       */
#define R1_CARD_ECC_DISABLED    (1 << 14)                               /*  sx, a                       */
#define R1_ERASE_RESET          (1 << 13)                               /*  sr, c                       */
#define R1_STATUS(x)            ((x) & 0xffffe000)
#define R1_CURRENT_STATE(x)     (((x) & 0x00001e00) >> 9)               /*  sx, b (4 bits)              */
#define R1_READY_FOR_DATA       (1 << 8)                                /*  sx, a                       */
#define R1_SWITCH_ERROR         (1 << 7)                                /*  sx, c                       */
#define R1_APP_CMD              (1 << 5)                                /*  sr, c                       */

/*********************************************************************************************************
  MMC/SD in SPI mode reports R1 status always, and R2 for SEND_STATUS
  R1 is the low order byte; R2 is the next highest byte, when present.
*********************************************************************************************************/

#define R1_SPI_IDLE             (1 << 0)
#define R1_SPI_ERASE_RESET      (1 << 1)
#define R1_SPI_ILLEGAL_COMMAND  (1 << 2)
#define R1_SPI_COM_CRC          (1 << 3)
#define R1_SPI_ERASE_SEQ        (1 << 4)
#define R1_SPI_ADDRESS          (1 << 5)
#define R1_SPI_PARAMETER        (1 << 6)
#define P1_SPI_BIT7RESERVED     (1 << 7)                                /*  第七位总是0 , 保留          */
#define R2_SPI_CARD_LOCKED      (1 << 8)
#define R2_SPI_WP_ERASE_SKIP    (1 << 9)
#define R2_SPI_LOCK_UNLOCK_FAIL R2_SPI_WP_ERASE_SKIP                    /*  or lock/unlock fail         */
#define R2_SPI_ERROR            (1 << 10)
#define R2_SPI_CC_ERROR         (1 << 11)
#define R2_SPI_CARD_ECC_ERROR   (1 << 12)
#define R2_SPI_WP_VIOLATION     (1 << 13)
#define R2_SPI_ERASE_PARAM      (1 << 14)
#define R2_SPI_OUT_OF_RANGE     (1 << 15)                               /*  or CSD overwrite            */
#define R2_SPI_CSD_OVERWRITE    R2_SPI_OUT_OF_RANGE

/*********************************************************************************************************
  SPI模式下数据应答令牌.  其令牌格式为:
      7---reserved----3--STATUS---1   0
      ---------------------------------
      | X | X | X | 0 |   |   |   | 1 |
      ---------------------------------
*********************************************************************************************************/

#define SD_SPITOKEN_DATRSP_MASK        0x0f
#define SD_SPITOKEN_DATRSP_DATACCEPT   0x05                             /*  数据接受成功                */
#define SD_SPITOKEN_DATRSP_CRCERROR    0x0b                             /*  CRC错误                     */
#define SD_SPITOKEN_DATRSP_WRTERROR    0x0d                             /*  数据写入错误                */

/*********************************************************************************************************
  SPI模式下数据传输控制令牌.
  注意: SD_SPITOKEN_START_MULBLK 和 SD_SPITOKEN_STOP_MULBLK 只用于多块写操作. 对于多块读操作的开始操作,
        使用SD_SPITOKEN_START_SIGBLK, 而多块读的停止操作, 使用与SD模式下的相同命令(CMD12).
*********************************************************************************************************/

#define SD_SPITOKEN_START_SIGBLK       0xfe                             /*  单块传输开始令牌            */
#define SD_SPITOKEN_START_MULBLK       0xfc                             /*  多块传输开始令牌            */
#define SD_SPITOKEN_STOP_MULBLK        0xfd                             /*  多块传输停止令牌            */

/*********************************************************************************************************
  SPI模式下数据错误令牌.
  当产生一个数据通信错误时, SD卡会发送此令牌. 格式为:
      7---reserved----3                                           0
      -------------------------------------------------------------
      | 0 | 0 | 0 | 0 |out of range|card ecc failed|cc error|error|
      -------------------------------------------------------------
*********************************************************************************************************/

#define SD_SPITOKEN_DATERR_ERROR       (1 << 0)
#define SD_SPITOKEN_DATERR_CC_ERROR    (1 << 1)
#define SD_SPITOKEN_DATERR_ECC_FAILED  (1 << 2)
#define SD_SPITOKEN_DATERR_OUT_RANGE   (1 << 3)
/*********************************************************************************************************
   SD应答类型提取及其检测
*********************************************************************************************************/

#define SD_RESP_TYPE(pcmd) ((pcmd)->SDCMD_uiFlag &      \
                            (SD_RSP_PRESENT         |   \
                             SD_RSP_136             |   \
                             SD_RSP_CRC             |   \
                             SD_RSP_BUSY            |   \
                             SD_RSP_OPCODE))

#define SD_CMD_TEST_RSP(pcmd, type)       (((pcmd)->SDCMD_uiFlag & (type)) == type)
#define SD_MSG_TEST_RSP(pmsg, type)       SD_CMD_TEST_RSP((pmsg)->SDMSG_psdcmdCmd, type)
#define SD_CMD_TEST_CMD(pcmd, type)       (SD_CMD_TEST_RSP(pcmd, type) == type)
#define SD_MSG_TEST_CMD(pmsg, type)       SD_CMD_TEST_CMD((pmsg)->SDMSG_psdcmdCmd, type)

/*********************************************************************************************************
  SD 总线命令
*********************************************************************************************************/

typedef struct lw_sd_command {
    UINT32  SDCMD_uiOpcode;                                             /*  操作码(命令)                */
    UINT32  SDCMD_uiArg;                                                /*  参数                        */
    UINT32  SDCMD_uiResp[4];                                            /*  应答(有效位最多128位)       */
    UINT32  SDCMD_uiFlag;                                               /*  属性位标 (命令和应答属性)   */
    UINT32  SDCMD_uiRetry;
} LW_SD_COMMAND, *PLW_SD_COMMAND;
#define SD_CMD_FLG(pcmd)   ((pcmd)->SDCMD_uiFlag)
#define SD_CMD_ARG(pcmd)   ((pcmd)->SDCMD_uiArg)
#define SD_CMD_OPC(pcmd)   ((pcmd)->SDCMD_uiOpcode)

/*********************************************************************************************************
  SD 总线数据控制
*********************************************************************************************************/

typedef struct lw_sd_data {
    UINT32  SDDAT_uiBlkSize;
    UINT32  SDDAT_uiBlkNum;
    UINT32  SDDAT_uiFlags;
} LW_SD_DATA, *PLW_SD_DATA;
#define SD_DAT_WRITE   (1 << 8)
#define SD_DAT_READ    (1 << 9)
#define SD_DAT_STREAM  (1 << 10)
#define SD_DAT_IS_STREAM(pSdDat)    ((pSdDat)->SDDAT_uiFlags  & SD_DAT_STREAM)
#define SD_DAT_IS_WRITE(pSdDat)     (((pSdDat)->SDDAT_uiFlags & SD_DAT_WRITE) == SD_DAT_WRITE)
#define SD_DAT_IS_READ(pSdDat)      (((pSdDat)->SDDAT_uiFlags & SD_DAT_READ)  == SD_DAT_READ)
#define SD_DAT_IS_BOTHRW(pSdDat)    (((pSdDat)->SDDAT_uiFlags &        \
                                      (SD_DAT_WRITE | SD_DAT_READ)) == \
                                      (SD_DAT_WRITE | SD_DAT_READ))

/*********************************************************************************************************
  SD 总线传输控制结构
*********************************************************************************************************/

typedef struct lw_sd_message {
    LW_SD_COMMAND  *SDMSG_psdcmdCmd;                                    /*  发送命令                    */
    LW_SD_DATA     *SDMSG_psddata;                                      /*  数据传输属性                */
    LW_SD_COMMAND  *SDMSG_psdcmdStop;                                   /*  停止命令                    */
    UINT8          *SDMSG_pucRdBuffer;                                  /*  请求缓冲(读缓冲)            */
    UINT8          *SDMSG_pucWrtBuffer;                                 /*  请求缓冲(写缓冲)            */
} LW_SD_MESSAGE, *PLW_SD_MESSAGE;
#define SD_MSG_OPC(pmsg)  SDCMD_OPC((pmsg)->SDMSG_psdcmdCmd)
#define SD_MSG_ARG(pmsg)  SDCMD_ARG((pmsg)->SDMSG_psdcmdCmd)
#define SD_MSG_FLG(pmsg)  SDCMD_FLG((pmsg)->SDMSG_psdcmdCmd)

/*********************************************************************************************************
  SD 总线适配器
*********************************************************************************************************/

struct lw_sd_funcs;
typedef struct lw_sd_adapter {
    LW_BUS_ADAPTER          SDADAPTER_busadapter;                       /*  总线节点                    */
    struct lw_sd_funcs     *SDADAPTER_psdfunc;                          /*  总线适配器操作函数          */

    LW_OBJECT_HANDLE        SDADAPTER_hBusLock;                         /*  总线操作锁                  */
    INT                     SDADAPTER_iBusWidth;                        /*  总线位宽                    */
#define SDBUS_WIDTH_1       0
#define SDBUS_WIDTH_4       2
#define SDBUS_WIDTH_8       4
#define SDBUS_WIDTH_4_DDR   8
#define SDBUS_WIDTH_8_DDR   16

    LW_LIST_LINE_HEADER     SDADAPTER_plineDevHeader;                   /*  设备链表                    */
} LW_SD_ADAPTER, *PLW_SD_ADAPTER;

/*********************************************************************************************************
  SD 总线传输函数集
*********************************************************************************************************/

struct lw_sd_device;
typedef struct lw_sd_funcs {
    INT (*SDFUNC_pfuncMasterXfer)(PLW_SD_ADAPTER          psdadapter,
                                  struct lw_sd_device    *psddevice,
                                  PLW_SD_MESSAGE          psdmsg,
                                  INT                     iNum);
    INT (*SDFUNC_pfuncMasterCtl)(PLW_SD_ADAPTER          psdadapter,
                                 INT                     iCmd,
                                 LONG                    lArg);
} LW_SD_FUNCS, *PLW_SD_FUNCS;
#define SDADP_TXF(pAda)                             (pAda)->SDADAPTER_psdfunc->SDFUNC_pfuncMasterXfer
#define SDADP_CTRL(pAda)                            (pAda)->SDADAPTER_psdfunc->SDFUNC_pfuncMasterCtl
#define SDBUS_TRANSFER(pAda, pDev, pMsg, iNum)      SDADP_TXF(pAda)(pAda, pDev, pMsg, iNum)
#define SDBUS_IOCTRL(pAda, iCmd, lArg)              SDADP_CTRL(pAda)(pAda, iCmd, lArg)

#define SDBUS_CTRL_POWEROFF       0
#define SDBUS_CTRL_POWERUP        1
#define SDBUS_CTRL_POWERON        2
#define SDBUS_CTRL_SETBUSWIDTH    3                                     /*  硬件级(I\O)总线设置         */
#define SDBUS_CTRL_SETCLK         4
#define SDBUS_CTRL_STOPCLK        5
#define SDBUS_CTRL_STARTCLK       6
#define SDBUS_CTRL_DELAYCLK       7
#define SDBUS_CTRL_GETOCR         8                                     /*  获得适配器的电压情况        */

#define SDARG_SETCLK_LOW          400000
#define SDARG_SETCLK_NORMAL       25000000
#define SDARG_SETCLK_MAX          50000000

#define SDARG_SETBUSWIDTH_1       SDBUS_WIDTH_1
#define SDARG_SETBUSWIDTH_4       SDBUS_WIDTH_4
#define SDARG_SETBUSWIDTH_8       SDBUS_WIDTH_8
#define SDARG_SETBUSWIDTH_4_DDR   SDBUS_WIDTH_4_DDR
#define SDARG_SETBUSWIDTH_8_DDR   SDBUS_WIDTH_8_DDR

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
#endif                                                                  /*  __SDBUS_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
