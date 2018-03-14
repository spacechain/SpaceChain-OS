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
** 文   件   名: sdstd.h
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2010 年 11 月 01 日
**
** 描        述: sd 卡规范 v3.01 标准协议.
**               包括标准命令、应用命令等.

** BUG:
*********************************************************************************************************/

#ifndef __SDSTD_H
#define __SDSTD_H

/*********************************************************************************************************
   标准SD命令 (v3.01)                                 右边注释格式:   |  命令类型   参数      应答类型  |
   命令类型说明:
   bc   -  无应答的广播命令(broadcast command)
   bcr  -  有应答的广播命令(broadcast command with response)
   ac   -  指定地址的无应答的命令(addressed command)
   adtc -  指定地址且伴随数据传输的命令(addressed data-transfer command)
*********************************************************************************************************/

/*********************************************************************************************************
   Class 0
*********************************************************************************************************/

#define SD_GO_IDLE_STATE         0                                      /*  bc                          */
#define SD_SEND_OP_COND          1                                      /*  bcr  [31:0] OCR         R3  */
#define SD_ALL_SEND_CID          2                                      /*  bcr                     R2  */
#define SD_SEND_RELATIVE_ADDR    3                                      /*  bcr  [31:16] RCA        R1  */
#define SD_SET_DSR               4                                      /*  bc   [31:16] RCA            */
#define SD_CMD5_RESERVED      	 5
#define SD_SELECT_CARD           7                                      /*  ac   [31:16] RCA        R1  */
#define SD_SEND_IF_COND          8                                      /*  bcr  [11:8]vhs[7:0]chk  R1  */
#define SD_SEND_CSD              9                                      /*  ac   [31:16] RCA        R2  */
#define SD_SEND_CID             10                                      /*  ac   [31:16] RCA        R2  */
#define SD_VOL_SWITCH			11                                      /*  ac   		            R1  */
#define SD_STOP_TRANSMISSION    12                                      /*  ac                      R1b */
#define SD_SEND_STATUS          13                                      /*  ac   [31:16] RCA  R1\R2(SPI)*/
#define SD_CMD14_RESERVED		14
#define SD_GO_INACTIVE_STATE    15                                      /*  ac   [31:16] RCA            */
#define SD_SPI_READ_OCR         58                                      /*  spi                  spi_R3 */
#define SD_SPI_CRC_ON_OFF       59                                      /*  spi  [0:0] flag      spi_R1 */

/*********************************************************************************************************
   Class 2
*********************************************************************************************************/

#define SD_SET_BLOCKLEN         16                                      /*  ac   [31:0] block len   R1  */
#define SD_READ_SINGLE_BLOCK    17                                      /*  adtc [31:0] data addr   R1  */
#define SD_READ_MULTIPLE_BLOCK  18                                      /*  adtc [31:0] data addr   R1  */
#define SD_SEND_TUNING_BLOCK	19                                      /*  adtc                    R1  */
#define SD_SPEED_CLASS_CONTROL	20                                      /*  ac   [31:28]scc         R1b */
#define SD_CMD21_RESERVED		21
#define SD_CMD22_RESERVED		22
#define SD_SET_BLOCK_COUNT		23                                      /*  ac   [31:0]blk count    R1  */

/*********************************************************************************************************
   Class 4
*********************************************************************************************************/

#define SD_WRITE_BLOCK          24                                      /*  adtc [31:0] data addr   R1  */
#define SD_WRITE_MULTIPLE_BLOCK 25                                      /*  adtc [31:0] data addr   R1  */
#define SD_CMD26_RESERVED       26
#define SD_PROGRAM_CSD          27                                      /*  adtc                    R1  */

/*********************************************************************************************************
   Class 6
*********************************************************************************************************/

#define SD_SET_WRITE_PROT       28                                      /*  ac   [31:0] data addr   R1b */
#define SD_CLR_WRITE_PROT       29                                      /*  ac   [31:0] data addr   R1b */
#define SD_SEND_WRITE_PROT      30                                      /*  adtc [31:0] wpdata addr R1  */
#define SD_CMD31_RESERVED		31

/*********************************************************************************************************
   Class 5
*********************************************************************************************************/

#define SD_ERASE_WR_BLK_START   32                                      /*  ac   [31:0] data addr   R1  */
#define SD_ERASE_WR_BLK_END     33                                      /*  ac   [31:0] data addr   R1  */
#define SD_ERASE                38                                      /*  ac                      R1b */
#define SD_CMD39_RESERVED       39

/*********************************************************************************************************
   Class 7
*********************************************************************************************************/

#define SD_LOCK_SET_BLK_LEN		16                                      /*  ac  [31:0] block len     R1 */
#define SD_CMD40_RESERVED		40
#define SD_LOCK_UNLOCK          42                                      /*  adtc                     R1 */

/*********************************************************************************************************
   Class 8
*********************************************************************************************************/

#define SD_APP_CMD              55                                      /*  ac   [31:16] RCA        R1  */
#define SD_GEN_CMD              56                                      /*  adtc [0] RD/WR          R1  */

/*********************************************************************************************************
   Class 10
*********************************************************************************************************/

#define SD_SWITCH_FUNC           6                                      /*  adtc argument See below R1b */
                                                                        /*  [31   ]mod 0:chk 1:swi func */
                                                                        /*  [30:24]reserved.all "0"     */
                                                                        /*  [23:20]reserved for f-grp6  */
                                                                        /*  [19:16]reserved for f-grp5  */
                                                                        /*  [15:12]f-grp4 for curr limit*/
                                                                        /*  [11:08]f-grp3 for drv streng*/
                                                                        /*  [07:04]f-grp2 for cmd system*/
                                                                        /*  [03:00]f-grp1 for access mod*/
#define SD_SWITCH_CMD_SET       0x00
#define SD_SWITCH_SET_BITS      0x01
#define SD_SWITCH_CLEAR_BITS    0x02
#define SD_SWITCH_WRITE_BYTE    0x03

#define SD_SWITCH_CHECK         0
#define SD_SWITCH_SWITCH        1

#define SD_EXFUNC_34			34
#define SD_EXFUNC_35			35
#define SD_EXFUNC_36			36
#define SD_EXFUNC_37			37
#define SD_EXFUNC_50			50
#define SD_EXFUNC_57			57                                      /*  在CMD6后可能有效的扩展功能  */

/*********************************************************************************************************
   应用命令
*********************************************************************************************************/

#define APP_CMD1_RESERVED		 1
#define APP_CMD2_RESERVED		 2
#define APP_CMD3_RESERVED		 3
#define APP_CMD4_RESERVED		 4
#define APP_CMD5_RESERVED		 5
#define APP_SET_BUS_WIDTH		 6                                      /*  ac  [1:0]bus width      R1  */
#define APP_CMD7_RESERVED		 7
#define APP_CMD8_RESERVED		 8
#define APP_CMD9_RESERVED		 9
#define APP_CMD10_RESERVED      10
#define APP_CMD11_RESERVED      11
#define APP_CMD12_RESERVED      12
#define APP_SEND_STATUS	        13                                      /*  adtc   		            R1  */
#define APP_CMD14_RESERVED      14
#define APP_CMD15_RESERVED      15
#define APP_CMD16_RESERVED      16
#define APP_CMD17_RESERVED      17
#define APP_CMD18_RESERVED      18
#define APP_CMD19_RESERVED      19
#define APP_CMD20_RESERVED      20
#define APP_CMD21_RESERVED      21
#define APP_SEND_NUM_WR_BLK     22                                      /*  adtc			        R1  */
#define APP_SET_WRBLK_ERA_CNT   23                                      /*  adtc			        R1  */
#define APP_CMD24_RESERVED      24
#define APP_CMD25_RESERVED      25
#define APP_CMD26_RESERVED      26
#define APP_CMD27_RESERVED      27
#define APP_CMD28_RESERVED      28
#define APP_CMD29_RESERVED      29
#define APP_CMD30_RESERVED      30
#define APP_CMD31_RESERVED      31
#define APP_CMD32_RESERVED      32
#define APP_CMD33_RESERVED      33
#define APP_CMD34_RESERVED      34
#define APP_CMD35_RESERVED      35
#define APP_CMD36_RESERVED      36
#define APP_CMD37_RESERVED      37
#define APP_CMD38_RESERVED      38
#define APP_CMD39_RESERVED      39
#define APP_CMD40_RESERVED      40
#define APP_SD_SEND_OP_COND     41                                      /*  bcr 			        R3  */
#define APP_SET_CLR_CARD_DETECT 42                                      /*  ac  [0]SET_CD		    R1  */
#define APP_SEND_SCR	        51                                      /*  adtc 			        R1  */

/*********************************************************************************************************
  EXT CSD 域代码
*********************************************************************************************************/

#define EXT_CSD_BUS_WIDTH               183
#define EXT_CSD_HS_TIMING               185
#define EXT_CSD_CARD_TYPE               196
#define EXT_CSD_REV                     192
#define EXT_CSD_STRUCT                  194
#define EXT_CSD_SEC_CNT                 212
#define BOOT_SIZE_MULTI                 226

/*********************************************************************************************************
  EXT CSD 域定义
*********************************************************************************************************/
#define MMC_CMD_SEND_EXT_CSD            8

#define EXT_CSD_CMD_SET_NORMAL          (1 << 0)
#define EXT_CSD_CMD_SET_SECURE          (1 << 1)
#define EXT_CSD_CMD_SET_CPSECURE        (1 << 2)

#define EXT_CSD_CARD_TYPE_26            (1 << 0)
#define EXT_CSD_CARD_TYPE_52            (1 << 1)
#define EXT_CSD_CARD_TYPE_52_DDR_18_30  (1 << 2)
#define EXT_CSD_CARD_TYPE_52_DDR_12     (1 << 3)

#define EXT_CSD_BUS_WIDTH_1             0
#define EXT_CSD_BUS_WIDTH_4             1
#define EXT_CSD_BUS_WIDTH_8             2
#define EXT_CSD_BUS_WIDTH_4_DDR         5
#define EXT_CSD_BUS_WIDTH_8_DDR         6

#define MMC_HS_TIMING                   0x00000100
#define MMC_HS_52MHZ                    (0x1 << 1)
#define MMC_HS_52MHZ_1_8V_3V_IO         (0x1 << 2)
#define MMC_HS_52MHZ_1_2V_IO            (0x1 << 3)

#define MMC_MODE_HS                     SDHOST_CAP_HIGHSPEED            /*  这里与 SDM 层定义一致       */
#define MMC_MODE_HS_52MHz               0x00100000
#define MMC_MODE_HS_52MHz_DDR_18_3V     0x00200000
#define MMC_MODE_HS_52MHz_DDR_12V       0x00400000

#endif                                                                  /*  __SDSTD_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
