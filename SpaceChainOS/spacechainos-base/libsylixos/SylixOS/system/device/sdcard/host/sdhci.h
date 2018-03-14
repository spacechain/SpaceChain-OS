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
** 文   件   名: sdhci.h
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2011 年 01 月 14 日
**
** 描        述: sd标准主控制器驱动头文件(SD Host Controller Simplified Specification Version 2.00).
**               注意所有寄存器定义都为相对偏移地址.
**               对于使用标准控制器的平台而言,使用该模块完全屏蔽了sd bus层的操作,移植工作更简单.

** BUG:
2011.03.02  增加主控传输模式查看\设置函数.允许动态改变传输模式(主控上不存在设备的情况下).
2011.04.12  主控属性里增加最大时钟频率这一项.
2011.05.10  考虑到SD控制器在不同平台上其寄存器可能在IO空间,也可能在内存空间,所以读写寄存器的6个函数申明
            为外部函数,由具体平台的驱动实现.
2011.06.01  访问寄存器的与硬件平台相关的函数采用回调方式,由驱动实现.
            今天是六一儿童节,回想起咱的童年,哎,一去不复返.祝福天下的小朋友们茁壮成长,健康快乐.
2014.11.14  为支持 SDIO 和加入 SDM 模块管理, 修改了相关数据结构. 同时删除了一些 API, 改为内部使用
2015.11.20  增加对特殊总线位宽的支持.
2015.12.17  增加对 MMC/eMMC 总线位宽的兼容性处理.
2017.02.28  增加 SDIO 外设额外电源设置和带外 SDIO 中断的 Quirk 操作.
*********************************************************************************************************/

#ifndef __SDHCI_H
#define __SDHCI_H

/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0)

/*********************************************************************************************************
   系统SDMA地址寄存器.
*********************************************************************************************************/

#define SDHCI_SYS_SDMA                  0x00                            /*  DMA传输时指向一个内存地址   */

/*********************************************************************************************************
    块大小寄存器.该寄存器用于描述DMA传输(bit 12 - 14)中的地址边界和一般传输中的块大小(bit 0 - 11)
*********************************************************************************************************/

#define SDHCI_BLOCK_SIZE                0x04
#define SDHCI_MAKE_BLKSZ(dma, blksz)    (((dma & 0x7) << 12) | (blksz & 0xfff))

/*********************************************************************************************************
    块计数寄存器.
*********************************************************************************************************/

#define SDHCI_BLOCK_COUNT               0x06

/*********************************************************************************************************
    参数寄存器.
*********************************************************************************************************/

#define SDHCI_ARGUMENT                  0x08

/*********************************************************************************************************
    传输模式寄存器.
*********************************************************************************************************/

#define SDHCI_TRANSFER_MODE             0x0c
#define SDHCI_TRNS_DMA                  0x01
#define SDHCI_TRNS_BLK_CNT_EN           0x02
#define SDHCI_TRNS_ACMD12               0x04
#define SDHCI_TRNS_ACMD23               0x08
#define SDHCI_TRNS_READ                 0x10
#define SDHCI_TRNS_MULTI                0x20

/*********************************************************************************************************
    命令寄存器.
*********************************************************************************************************/

#define SDHCI_COMMAND                   0x0e
#define SDHCI_CMD_CRC_CHK               0x08
#define SDHCI_CMD_INDEX_CHK             0x10
#define SDHCI_CMD_DATA                  0x20
#define SDHCI_CMD_RESP_TYPE_MASK        0x03
#define SDHCI_CMD_RESP_TYPE_NONE        0x00
#define SDHCI_CMD_RESP_TYPE_LONG        0x01
#define SDHCI_CMD_RESP_TYPE_SHORT       0x02
#define SDHCI_CMD_RESP_TYPE_SHORT_BUSY  0x03
#define SDHCI_CMD_TYPE_NORMAL           0x00
#define SDHCI_CMD_TYPE_SUSPEND          0x40
#define SDHCI_CMD_TYPE_RESUME           0x80
#define SDHCI_CMD_TYPE_ABORT            0xc0

#define SDHCI_MAKE_CMD(cmd, flg)        (((cmd & 0xff) << 8) | (flg & 0xff))

/*********************************************************************************************************
    响应寄存器.
*********************************************************************************************************/

#define SDHCI_RESPONSE0                 0x10
#define SDHCI_RESPONSE1                 0x14
#define SDHCI_RESPONSE2                 0x18
#define SDHCI_RESPONSE3                 0x1c

/*********************************************************************************************************
    数据缓冲寄存器.
*********************************************************************************************************/

#define SDHCI_BUFFER                    0x20

/*********************************************************************************************************
    当前状态寄存器.
*********************************************************************************************************/

#define SDHCI_PRESENT_STATE             0x24
#define SDHCI_PSTA_CMD_INHIBIT          0x00000001
#define SDHCI_PSTA_DATA_INHIBIT         0x00000002
#define SDHCI_PSTA_DATA_ACTIVE          0x00000004
#define SDHCI_PSTA_DOING_WRITE          0x00000100
#define SDHCI_PSTA_DOING_READ           0x00000200
#define SDHCI_PSTA_SPACE_AVAILABLE      0x00000400
#define SDHCI_PSTA_DATA_AVAILABLE       0x00000800
#define SDHCI_PSTA_CARD_PRESENT         0x00010000
#define SDHCI_PSTA_WRITE_PROTECT        0x00080000

/*********************************************************************************************************
    主控器控制寄存器.
*********************************************************************************************************/

#define SDHCI_HOST_CONTROL              0x28
#define SDHCI_HCTRL_LED                 0x01
#define SDHCI_HCTRL_4BITBUS             0x02
#define SDHCI_HCTRL_HISPD               0x04
#define SDHCI_HCTRL_8BITBUS             0x20                            /*  just in v3.0 spec           */
#define SDHCI_HCTRL_DMA_MASK            0x18
#define SDHCI_HCTRL_SDMA                0x00
#define SDHCI_HCTRL_ADMA1               0x08
#define SDHCI_HCTRL_ADMA32              0x10
#define SDHCI_HCTRL_ADMA64              0x18
#define SDHCI_HCTRL_MMC_BITS8           0x20

/*********************************************************************************************************
    电源控制寄存器.
*********************************************************************************************************/

#define SDHCI_POWER_CONTROL             0x29
#define SDHCI_POWCTL_ON                 0x01
#define SDHCI_POWCTL_180                0x0a
#define SDHCI_POWCTL_300                0x0c
#define SDHCI_POWCTL_330                0x0e

/*********************************************************************************************************
    块间隔寄存器.
*********************************************************************************************************/

#define SDHCI_BLOCK_GAP_CONTROL         0x2a
#define SDHCI_BLKGAP_STOP               0x01
#define SDHCI_BLKGAP_RESTART            0x02
#define SDHCI_BLKGAP_RWCTL_EN           0x04
#define SDHCI_BLKGAP_INT_EN             0x08

/*********************************************************************************************************
    唤醒控制寄存器.
*********************************************************************************************************/

#define SDHCI_WAKE_UP_CONTROL           0x2b
#define SDHCI_WAKUP_INT_EN              0x01
#define SDHCI_WAKUP_INSERT_EN           0x02
#define SDHCI_WAKUP_REMOVE_EN           0x04

/*********************************************************************************************************
    时钟控制寄存器.
*********************************************************************************************************/

#define SDHCI_CLOCK_CONTROL             0x2c
#define SDHCI_CLKCTL_DIVIDER_SHIFT      8
#define SDHCI_CLKCTL_CLOCK_EN           0x0004
#define SDHCI_CLKCTL_INTER_STABLE       0x0002
#define SDHCI_CLKCTL_INTER_EN           0x0001

/*********************************************************************************************************
   超时计数控制寄存器.
*********************************************************************************************************/

#define SDHCI_TIMEOUT_CONTROL           0x2e

/*********************************************************************************************************
    软件复位寄存器.
*********************************************************************************************************/

#define SDHCI_SOFTWARE_RESET            0x2f
#define SDHCI_SFRST_ALL                 0x01
#define SDHCI_SFRST_CMD                 0x02
#define SDHCI_SFRST_DATA                0x04

/*********************************************************************************************************
    一般中断状态寄存器.
*********************************************************************************************************/

#define SDHCI_INT_STATUS                0x30
#define SDHCI_INTSTA_CMD_END            0x0001
#define SDHCI_INTSTA_DATA_END           0x0002
#define SDHCI_INTSTA_BLKGAP_END         0x0004
#define SDHCI_INTSTA_DMA                0x0008
#define SDHCI_INTSTA_WRTBUF_RDY         0x0010
#define SDHCI_INTSTA_RDBUF_RDY          0x0020
#define SDHCI_INTSTA_CARD_INSERT        0x0040
#define SDHCI_INTSTA_CARD_REMOVE        0x0080
#define SDHCI_INTSTA_CARD_INT           0x0100
#define SDHCI_INTSTA_CCS_INT            0x0200
#define SDHCI_INTSTA_RDWAIT_INT         0x0400
#define SDHCI_INTSTA_FIFA0_INT          0x0800
#define SDHCI_INTSTA_FIFA1_INT          0x1000
#define SDHCI_INTSTA_FIFA2_INT          0x2000
#define SDHCI_INTSTA_FIFA3_INT          0x4000
#define SDHCI_INTSTA_INT_ERROR          0x8000

/*********************************************************************************************************
    错误中断状态寄存器.
*********************************************************************************************************/

#define SDHCI_ERRINT_STATUS             0x32
#define SDHCI_EINTSTA_CMD_TIMOUT        0x0001
#define SDHCI_EINTSTA_CMD_CRC           0x0002
#define SDHCI_EINTSTA_CMD_ENDBIT        0x0004
#define SDHCI_EINTSTA_CMD_INDEX         0x0008
#define SDHCI_EINTSTA_DATA_TIMOUT       0x0010
#define SDHCI_EINTSTA_DATA_CRC          0x0020
#define SDHCI_EINTSTA_DATA_ENDBIT       0x0040
#define SDHCI_EINTSTA_CMD_CURRLIMIT     0x0080
#define SDHCI_EINTSTA_CMD_ACMD12        0x0100
#define SDHCI_EINTSTA_CMD_ADMA          0x0200


/*********************************************************************************************************
   一般中断状态使能寄存器.
*********************************************************************************************************/

#define SDHCI_INTSTA_ENABLE             0x34
#define SDHCI_INTSTAEN_CMD              0x0001
#define SDHCI_INTSTAEN_DATA             0x0002
#define SDHCI_INTSTAEN_BLKGAP           0x0004
#define SDHCI_INTSTAEN_DMA              0x0008
#define SDHCI_INTSTAEN_WRTBUF           0x0010
#define SDHCI_INTSTAEN_RDBUF            0x0020
#define SDHCI_INTSTAEN_CARD_INSERT      0x0040
#define SDHCI_INTSTAEN_CARD_REMOVE      0x0080
#define SDHCI_INTSTAEN_CARD_INT         0x0100
#define SDHCI_INTSTAEN_CCS_INT          0x0200
#define SDHCI_INTSTAEN_RDWAIT           0x0400
#define SDHCI_INTSTAEN_FIFA0            0x0800
#define SDHCI_INTSTAEN_FIFA1            0x1000
#define SDHCI_INTSTAEN_FIFA2            0x2000
#define SDHCI_INTSTAEN_FIFA3            0x4000

/*********************************************************************************************************
   错误中断状态使能寄存器.
*********************************************************************************************************/

#define SDHCI_EINTSTA_ENABLE            0x36
#define SDHCI_EINTSTAEN_CMD_TIMEOUT     0x0001
#define SDHCI_EINTSTAEN_CMD_CRC         0x0002
#define SDHCI_EINTSTAEN_CMD_ENDBIT      0x0004
#define SDHCI_EINTSTAEN_CMD_INDEX       0x0008
#define SDHCI_EINTSTAEN_DATA_TIMEOUT    0x0010
#define SDHCI_EINTSTAEN_DATA_CRC        0x0020
#define SDHCI_EINTSTAEN_DATA_ENDBIT     0x0040
#define SDHCI_EINTSTAEN_CURR_LIMIT      0x0080
#define SDHCI_EINTSTAEN_ACMD12          0x0100
#define SDHCI_EINTSTAEN_ADMA            0x0200

/*********************************************************************************************************
  一般中断信号使能寄存器.
*********************************************************************************************************/

#define SDHCI_SIGNAL_ENABLE             0x38
#define SDHCI_SIGEN_CMD_END             0x0001
#define SDHCI_SIGEN_DATA_END            0x0002
#define SDHCI_SIGEN_BLKGAP              0x0004
#define SDHCI_SIGEN_DMA                 0x0008
#define SDHCI_SIGEN_WRTBUF              0x0010
#define SDHCI_SIGEN_RDBUF               0x0020
#define SDHCI_SIGEN_CARD_INSERT         0x0040
#define SDHCI_SIGEN_CARD_REMOVE         0x0080
#define SDHCI_SIGEN_CARD_INT            0x0100

/*********************************************************************************************************
  错误中断信号使能寄存器.
*********************************************************************************************************/

#define SDHCI_ERRSIGNAL_ENABLE          0x3a
#define SDHCI_ESIGEN_CMD_TIMEOUT        0x0001
#define SDHCI_ESIGEN_CMD_CRC            0x0002
#define SDHCI_ESIGEN_CMD_ENDBIT         0x0004
#define SDHCI_ESIGEN_CMD_INDEX          0x0008
#define SDHCI_ESIGEN_DATA_TIMEOUT       0x0010
#define SDHCI_ESIGEN_DATA_CRC           0x0020
#define SDHCI_ESIGEN_DATA_ENDBIT        0x0040
#define SDHCI_ESIGEN_CURR_LIMIT         0x0080
#define SDHCI_ESIGEN_ACMD12             0x0100
#define SDHCI_ESIGEN_ADMA               0x0200

/*********************************************************************************************************
  following is for the case of 32-bit registers operation.
*********************************************************************************************************/

#define SDHCI_INT_RESPONSE              0x00000001
#define SDHCI_INT_DATA_END              0x00000002
#define SDHCI_INT_DMA_END               0x00000008
#define SDHCI_INT_SPACE_AVAIL           0x00000010
#define SDHCI_INT_DATA_AVAIL            0x00000020
#define SDHCI_INT_CARD_INSERT           0x00000040
#define SDHCI_INT_CARD_REMOVE           0x00000080
#define SDHCI_INT_CARD_INT              0x00000100
#define SDHCI_INT_ERROR                 0x00008000                      /*  normal                      */

#define SDHCI_INT_TIMEOUT               0x00010000
#define SDHCI_INT_CRC                   0x00020000
#define SDHCI_INT_END_BIT               0x00040000
#define SDHCI_INT_INDEX                 0x00080000
#define SDHCI_INT_DATA_TIMEOUT          0x00100000
#define SDHCI_INT_DATA_CRC              0x00200000
#define SDHCI_INT_DATA_END_BIT          0x00400000
#define SDHCI_INT_BUS_POWER             0x00800000
#define SDHCI_INT_ACMD12ERR             0x01000000
#define SDHCI_INT_ADMA_ERROR            0x02000000                      /*  error                       */

#define SDHCI_INT_NORMAL_MASK           0x00007fff
#define SDHCI_INT_ERROR_MASK            0xffff8000

#define SDHCI_INT_CMD_MASK              (SDHCI_INT_RESPONSE     | \
                                         SDHCI_INT_TIMEOUT      | \
                                         SDHCI_INT_CRC          | \
                                         SDHCI_INT_END_BIT      | \
                                         SDHCI_INT_INDEX)
#define SDHCI_INT_DATA_MASK             (SDHCI_INT_DATA_END     | \
                                         SDHCI_INT_DMA_END      | \
                                         SDHCI_INT_DATA_AVAIL   | \
                                         SDHCI_INT_SPACE_AVAIL  | \
                                         SDHCI_INT_DATA_TIMEOUT | \
                                         SDHCI_INT_DATA_CRC     | \
                                         SDHCI_INT_DATA_END_BIT | \
                                         SDHCI_INT_ADMA_ERROR)
#define SDHCI_INT_ALL_MASK              (~SDHCI_INT_CARD_INT)

/*********************************************************************************************************
  自动 CMD12 错误状态寄存器.
*********************************************************************************************************/

#define SDHCI_ACMD12_ERR                0x3c
#define SDHCI_EACMD12_EXE               0x0001
#define SDHCI_EACMD12_TIMEOUT           0x0002
#define SDHCI_EACMD12_CRC               0x0004
#define SDHCI_EACMD12_ENDBIT            0x0008
#define SDHCI_EACMD12_INDEX             0x0010
#define SDHCI_EACMD12_CMDISSUE          0x0080

/*********************************************************************************************************
  主控功能寄存器.
*********************************************************************************************************/

#define SDHCI_CAPABILITIES              0x40
#define SDHCI_CAP_TIMEOUT_CLK_MASK      0x0000003f
#define SDHCI_CAP_TIMEOUT_CLK_SHIFT     0
#define SDHCI_CAP_TIMEOUT_CLK_UNIT      0x00000080
#define SDHCI_CAP_BASECLK_MASK          0x00003f00
#define SDHCI_CAP_BASECLK_SHIFT         8
#define SDHCI_CAP_MAXBLK_MASK           0x00030000
#define SDHCI_CAP_MAXBLK_SHIFT          16
#define SDHCI_CAP_CAN_DO_ADMA           0x00080000
#define SDHCI_CAP_CAN_DO_HISPD          0x00200000
#define SDHCI_CAP_CAN_DO_SDMA           0x00400000
#define SDHCI_CAP_CAN_DO_SUSRES         0x00800000
#define SDHCI_CAP_CAN_VDD_330           0x01000000
#define SDHCI_CAP_CAN_VDD_300           0x02000000
#define SDHCI_CAP_CAN_VDD_180           0x04000000
#define SDHCI_CAP_CAN_64BIT             0x10000000

/*********************************************************************************************************
  Maximum current capability register.
*********************************************************************************************************/

#define SDHCI_MAX_CURRENT               0x48                            /*  4c-4f reserved for more     */
                                                                        /*  for more max current        */

/*********************************************************************************************************
  设置(force set, 即主动产生中断)ACMD12错误寄存器.
*********************************************************************************************************/

#define SDHCI_SET_ACMD12_ERROR          0x50

/*********************************************************************************************************
  设置(force set, 即主动产生中断)中断错误寄存器.
*********************************************************************************************************/

#define SDHCI_SET_INT_ERROR             0x52

/*********************************************************************************************************
  ADMA 错误状态寄存器.
*********************************************************************************************************/

#define SDHCI_ADMA_ERROR                0x54

/*********************************************************************************************************
  ADMA 地址寄存器.
*********************************************************************************************************/

#define SDHCI_ADMA_ADDRESS              0x58

/*********************************************************************************************************
  插槽中断状态寄存器.
*********************************************************************************************************/

#define SDHCI_SLOT_INT_STATUS           0xfc
#define SDHCI_SLOTINT_MASK              0x00ff
#define SDHCI_CHK_SLOTINT(slotsta, n)   ((slotsta & 0x00ff) | (1 << (n)))

/*********************************************************************************************************
  主控制器版本寄存器.
*********************************************************************************************************/

#define SDHCI_HOST_VERSION              0xfe
#define SDHCI_HVER_VENDOR_VER_MASK      0xff00
#define SDHCI_HVER_VENDOR_VER_SHIFT     8
#define SDHCI_HVER_SPEC_VER_MASK        0x00ff
#define SDHCI_HVER_SPEC_VER_SHIFT       0
#define SDHCI_HVER_SPEC_100             0x0000
#define SDHCI_HVER_SPEC_200             0x0001

/*********************************************************************************************************
  其他宏定义.
*********************************************************************************************************/

#define SDHCI_DELAYMS(ms)                                   \
        do {                                                \
            ULONG   ulTimeout = LW_MSECOND_TO_TICK_1(ms);   \
            API_TimeSleep(ulTimeout);                       \
        } while (0)

/*********************************************************************************************************
  主控传输模式设置使用参数.
*********************************************************************************************************/

#define SDHCIHOST_TMOD_SET_NORMAL       0
#define SDHCIHOST_TMOD_SET_SDMA         1
#define SDHCIHOST_TMOD_SET_ADMA         2

/*********************************************************************************************************
  主控支持的传输模式组合位标定义.
*********************************************************************************************************/

#define SDHCIHOST_TMOD_CAN_NORMAL       (1 << 0)
#define SDHCIHOST_TMOD_CAN_SDMA         (1 << 1)
#define SDHCIHOST_TMOD_CAN_ADMA         (1 << 2)

/*********************************************************************************************************
  SD 标准主控制器属性结构
*********************************************************************************************************/

struct _sdhci_drv_funcs;                                                /*  标准主控驱动函数声明        */
typedef struct _sdhci_drv_funcs SDHCI_DRV_FUNCS;

struct _sdhci_quirk_op;
typedef struct _sdhci_quirk_op SDHCI_QUIRK_OP;

typedef struct lw_sdhci_host_attr {

    /*
     * SDHCIHOST_pdrvfuncs 为控制器寄存器访问驱动函数
     * 通常情况下驱动程序可不提供, 内部根据
     * SDHCIHOST_iRegAccessType 使用相应的默认驱动
     *
     * 但为了最大的适应性, 驱动程序也可使用自己的寄存器访问驱动
     */
    SDHCI_DRV_FUNCS *SDHCIHOST_pdrvfuncs;                               /*  标准主控驱动函数结构指针    */
    INT              SDHCIHOST_iRegAccessType;                          /*  寄存器访问类型              */
#define SDHCI_REGACCESS_TYPE_IO         0
#define SDHCI_REGACCESS_TYPE_MEM        1

    ULONG            SDHCIHOST_ulBasePoint;                             /*  槽基地址指针                */
    ULONG            SDHCIHOST_ulIntVector;                             /*  控制器在 CPU 中的中断向量   */
    UINT32           SDHCIHOST_uiMaxClock;                              /*  如果控制器没有内部时钟,用户 */
                                                                        /*  需要提供时钟源              */
    /*
     * 控制器怪异行为描述:
     * 有些控制器虽然按照 SDHCI 标准规范设计, 但在实现时存在一些
     * 不符合规范的地方(如某些寄存器的位定义, 某些功能不支持等).
     * SDHCIHOST_pquirkop:
     *  主要针对不同的寄存器位设置, 由驱动编写者自行实现.
     *  如果该域为 NULL, 或相关的操作方法为 NULL, 则内部会使用
     *  对应的标准操作.
     * SDHCIHOST_uiQuirkFlag:
     *  主要针对不同的特性开关, 如是否使用 ACMD12 等.
     *
     * SDHCI_QUIRK_FLG_SDIO_INT_OOB
     *  通常情况想, SDHCI 本身支持 SDIO 中断功能, 如果控制器确实不支持SDIO硬件中断, 则内部使用查询模式.
     *  但是, 有的 SDIO 设备自身通过额外的 GPIO 引脚产生 SDIO 中断信号, 即不由 SDHCI 控制器本身产生, 因此
     *  叫做带外中断(Out-of-Band). 使能此标志后, 将忽略 SDHCI_QUIRK_FLG_CANNOT_SDIO_INT 标志, 并且需要
     *  驱动自己实现带外中断服务.
     */
    SDHCI_QUIRK_OP  *SDHCIHOST_pquirkop;
    UINT32           SDHCIHOST_uiQuirkFlag;
#define SDHCI_QUIRK_FLG_DONOT_RESET_ON_EVERY_TRANSACTION      (1 << 0)  /*  每一次传输前不需要复位控制器*/
#define SDHCI_QUIRK_FLG_REENABLE_INTS_ON_EVERY_TRANSACTION    (1 << 1)  /*  传输后禁止, 传输前使能中断  */
#define SDHCI_QUIRK_FLG_DO_RESET_ON_TRANSACTION_ERROR         (1 << 2)  /*  传输错误时复位控制器        */
#define SDHCI_QUIRK_FLG_DONOT_CHECK_BUSY_BEFORE_CMD_SEND      (1 << 3)  /*  发送命令前不执行忙检查      */
#define SDHCI_QUIRK_FLG_DONOT_USE_ACMD12                      (1 << 4)  /*  不使用 Auto CMD12           */
#define SDHCI_QUIRK_FLG_DONOT_SET_POWER                       (1 << 5)  /*  不操作控制器电源的开/关     */
#define SDHCI_QUIRK_FLG_DO_RESET_AFTER_SET_POWER_ON           (1 << 6)  /*  当打开电源后执行控制器复位  */
#define SDHCI_QUIRK_FLG_DONOT_SET_VOLTAGE                     (1 << 7)  /*  不操作控制器的电压          */
#define SDHCI_QUIRK_FLG_CANNOT_SDIO_INT                       (1 << 8)  /*  控制器不能发出 SDIO 中断    */
#define SDHCI_QUIRK_FLG_RECHECK_INTS_AFTER_ISR                (1 << 9)  /*  中断服务后再次处理中断状态  */
#define SDHCI_QUIRK_FLG_CAN_DATA_8BIT                         (1 << 10) /*  支持8位数据传输             */
#define SDHCI_QUIRK_FLG_CAN_DATA_4BIT_DDR                     (1 << 11) /*  支持4位ddr数据传输          */
#define SDHCI_QUIRK_FLG_CAN_DATA_8BIT_DDR                     (1 << 12) /*  支持8位ddr数据传输          */
#define SDHCI_QUIRK_FLG_MMC_FORCE_1BIT                        (1 << 13) /*  MMC 卡强制使用1位总线       */
#define SDHCI_QUIRK_FLG_CANNOT_HIGHSPEED                      (1 << 14) /*  不支持高速传输              */
#define SDHCI_QUIRK_FLG_SDIO_INT_OOB                          (1 << 15) /*  SDIO OOB 中断               */
#define SDHCI_QUIRK_FLG_SDIO_FORCE_1BIT                       (1 << 16) /*  SDIO 卡强制使用1位总线      */
#define SDHCI_QUIRK_FLG_HAS_DATEND_IRQ_WHEN_NOT_BUSY          (1 << 17) /*  当卡不忙时会产生数据完成中断*/

    VOID            *SDHCIHOST_pvUsrSpec;                               /*  用户驱动特殊数据            */
} LW_SDHCI_HOST_ATTR, *PLW_SDHCI_HOST_ATTR;

#define SDHCI_QUIRK_FLG(pattr, flg)   ((pattr)->SDHCIHOST_uiQuirkFlag & (flg))

/*********************************************************************************************************
  SD 标准主控驱动结构体
*********************************************************************************************************/

struct _sdhci_drv_funcs {
    UINT32        (*sdhciReadL)
                  (
                  PLW_SDHCI_HOST_ATTR   psdhcihostattr,
                  ULONG                 ulReg
                  );
    UINT16        (*sdhciReadW)
                  (
                  PLW_SDHCI_HOST_ATTR   psdhcihostattr,
                  ULONG                 ulReg
                  );
    UINT8         (*sdhciReadB)
                  (
                  PLW_SDHCI_HOST_ATTR   psdhcihostattr,
                  ULONG                 ulReg
                  );
    VOID          (*sdhciWriteL)
                  (
                  PLW_SDHCI_HOST_ATTR   psdhcihostattr,
                  ULONG                 ulReg,
                  UINT32                uiLword
                  );
    VOID          (*sdhciWriteW)
                  (
                  PLW_SDHCI_HOST_ATTR   psdhcihostattr,
                  ULONG                 ulReg,
                  UINT16                usWord
                  );
    VOID          (*sdhciWriteB)
                  (
                  PLW_SDHCI_HOST_ATTR   psdhcihostattr,
                  ULONG                 ulReg,
                  UINT8                 ucByte
                  );
};

#define SDHCI_READL(pattr, lReg)           \
        ((pattr)->SDHCIHOST_pdrvfuncs->sdhciReadL)(pattr, lReg)
#define SDHCI_READW(pattr, lReg)           \
        ((pattr)->SDHCIHOST_pdrvfuncs->sdhciReadW)(pattr, lReg)
#define SDHCI_READB(pattr, lReg)           \
        ((pattr)->SDHCIHOST_pdrvfuncs->sdhciReadB)(pattr, lReg)

#define SDHCI_WRITEL(pattr, lReg, uiLword) \
        ((pattr)->SDHCIHOST_pdrvfuncs->sdhciWriteL)(pattr, lReg, uiLword)
#define SDHCI_WRITEW(pattr, lReg, usWord)  \
        ((pattr)->SDHCIHOST_pdrvfuncs->sdhciWriteW)(pattr, lReg, usWord)
#define SDHCI_WRITEB(pattr, lReg, ucByte)  \
        ((pattr)->SDHCIHOST_pdrvfuncs->sdhciWriteB)(pattr, lReg, ucByte)

/*********************************************************************************************************
  SD 标准主控为兼容一些不符合标准的怪异(quirk)行为 或 有些由硬件自行定义的特性 操作
  注意: 所有函数的第一个参数是 使用 API_SdhciHostCreate() 时用户驱动传入的 psdhcihostattr 的<一份拷贝>,
        而不是原始的那个参数对象, 因此, 如果下面的方法实现确实依赖于更多的内部数据, 则必须将这些内
        部数据保存到 SDHCIHOST_pvUsrSpec 这个结构成员里, 而不是使用扩展结构体的方式.

  SDHCIQOP_pfuncExtraPowerSet :
        通常情况下, SDHCI 通过内部寄存器可控制电源的开关. 但不排除有的硬件设计上, 连接的 SDIO 设备还有
        额外电源开关. 实现此方法后, 则内部会同时调用标准SDHCI电源设置和此方法.

  SDHCIQOP_pfuncOOBInterSet :
        如果设置了 SDHCI_QUIRK_FLG_SDIO_INT_OOB 标志, 则驱动可以实现此方法供上层调用. 当然也可根据实际
        情况, 不实现此方法.
*********************************************************************************************************/

struct _sdhci_quirk_op {
    INT     (*SDHCIQOP_pfuncClockSet)                                   /*  设置并打开时钟              */
            (
            PLW_SDHCI_HOST_ATTR  psdhcihostattr,
            UINT32               uiClock
            );
    INT     (*SDHCIQOP_pfuncClockStop)                                  /*  停止时钟                    */
            (
            PLW_SDHCI_HOST_ATTR  psdhcihostattr
            );
    INT     (*SDHCIQOP_pfuncBusWidthSet)                                /*  设置总线位宽                */
            (
            PLW_SDHCI_HOST_ATTR  psdhcihostattr,
            UINT32               uiBusWidth
            );
    INT     (*SDHCIQOP_pfuncResponseGet)                                /*  获取应答                    */
            (
            PLW_SDHCI_HOST_ATTR  psdhcihostattr,
            UINT32               uiRespFlag,
            UINT32              *puiRespOut
            );
    VOID    (*SDHCIQOP_pfuncIsrEnterHook)                               /*  SD 普通中断进入 HOOK 调用   */
            (
            PLW_SDHCI_HOST_ATTR  psdhcihostattr
            );
    VOID    (*SDHCIQOP_pfuncIsrExitHook)                                /*  SD 普通中断退出 HOOK 调用   */
            (
            PLW_SDHCI_HOST_ATTR  psdhcihostattr
            );
    BOOL    (*SDHCIQOP_pfuncIsCardWp)                                   /*  获得卡写保护状态            */
            (
            PLW_SDHCI_HOST_ATTR  psdhcihostattr
            );
    VOID    (*SDHCIQOP_pfuncExtraPowerSet)                              /*  设置额外的电源              */
            (
            PLW_SDHCI_HOST_ATTR  psdhcihostattr,
            BOOL                 bPowerOn
            );
    VOID    (*SDHCIQOP_pfuncOOBInterSet)                                /*  设置 OOB 中断               */
            (
            PLW_SDHCI_HOST_ATTR  psdhcihostattr,
            BOOL                 bEnable
            );
    VOID    (*SDHCIQOP_pfuncTimeoutSet)                                 /*  设置 超时值(仅数据传输时用) */
            (
            PLW_SDHCI_HOST_ATTR  psdhcihostattr
            );
    VOID    (*SDHCIQOP_pfuncHwReset)                                    /*  平台相关硬件复位            */
            (
            PLW_SDHCI_HOST_ATTR  psdhcihostattr
            );
};

/*********************************************************************************************************
  SD 标准主控制器操作
*********************************************************************************************************/

LW_API PVOID      API_SdhciHostCreate(CPCHAR               pcAdapterName,
                                      PLW_SDHCI_HOST_ATTR  psdhcihostattr);
LW_API INT        API_SdhciHostDelete(PVOID    pvHost);
LW_API PVOID      API_SdhciSdmHostGet(PVOID    pvHost);

LW_API INT        API_SdhciHostTransferModGet(PVOID    pvHost);
LW_API INT        API_SdhciHostTransferModSet(PVOID    pvHost, INT   iTmod);

/*********************************************************************************************************
  SD 标准主控制器设备操作
*********************************************************************************************************/

LW_API VOID       API_SdhciDeviceCheckNotify(PVOID  pvHost, INT iDevSta);

LW_API INT        API_SdhciDeviceUsageInc(PVOID     pvHost);
LW_API INT        API_SdhciDeviceUsageDec(PVOID     pvHost);
LW_API INT        API_SdhciDeviceUsageGet(PVOID     pvHost);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
#endif                                                                  /*  __SDHCI_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
