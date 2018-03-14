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
** 文   件   名: dm9000.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 10 月 31 日
**
** 描        述: dm9000 网卡驱动支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "../SylixOS/config/driver/drv_cfg.h"
/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_NET_EN > 0) && (LW_CFG_DRV_NIC_DM9000 > 0)
#include "linux/compat.h"
#include "linux/crc32.h"
#include "dm9000.h"
/*********************************************************************************************************
  dm9000 register
*********************************************************************************************************/
#define DM9000_ID               0x90000a46
#define DM9000_PKT_MAX          1536                                    /*  Received packet max size    */
#define DM9000_PKT_RDY          0x01                                    /*  Packet ready to receive     */
#define DM9000_PKT_ERR          0x02

/*********************************************************************************************************
  although the registers are 16 bit, they are 32-bit aligned.
*********************************************************************************************************/
#define DM9000_NCR              0x00
#define DM9000_NSR              0x01
#define DM9000_TCR              0x02
#define DM9000_TSR1             0x03
#define DM9000_TSR2             0x04
#define DM9000_RCR              0x05
#define DM9000_RSR              0x06
#define DM9000_ROCR             0x07
#define DM9000_BPTR             0x08
#define DM9000_FCTR             0x09
#define DM9000_FCR              0x0a
#define DM9000_EPCR             0x0b
#define DM9000_EPAR             0x0c
#define DM9000_EPDRL            0x0d
#define DM9000_EPDRH            0x0e
#define DM9000_WCR              0x0f

#define DM9000_PAR              0x10
#define DM9000_MAR              0x16

#define DM9000_GPCR             0x1e
#define DM9000_GPR              0x1f
#define DM9000_TRPAL            0x22
#define DM9000_TRPAH            0x23
#define DM9000_RWPAL            0x24
#define DM9000_RWPAH            0x25

#define DM9000_VIDL             0x28
#define DM9000_VIDH             0x29
#define DM9000_PIDL             0x2a
#define DM9000_PIDH             0x2b

#define DM9000_CHIPR            0x2c
#define DM9000_SMCR             0x2f

#define DM9000_PHY              0x40                                    /*  PHY address 0x01            */

#define DM9000_MRCMDX           0xf0
#define DM9000_MRCMD            0xf2
#define DM9000_MRRL             0xf4
#define DM9000_MRRH             0xf5
#define DM9000_MWCMDX           0xf6
#define DM9000_MWCMD            0xf8
#define DM9000_MWRL             0xfa
#define DM9000_MWRH             0xfb
#define DM9000_TXPLL            0xfc
#define DM9000_TXPLH            0xfd
#define DM9000_ISR              0xfe
#define DM9000_IMR              0xff

#define NCR_EXT_PHY             (1 << 7)
#define NCR_WAKEEN              (1 << 6)
#define NCR_FCOL                (1 << 4)
#define NCR_FDX                 (1 << 3)
#define NCR_LBK                 (3 << 1)
#define NCR_LBK_INT_MAC         (1 << 1)
#define NCR_LBK_INT_PHY         (2 << 1)
#define NCR_RST                 (1 << 0)

#define NSR_SPEED               (1 << 7)
#define NSR_LINKST              (1 << 6)
#define NSR_WAKEST              (1 << 5)
#define NSR_TX2END              (1 << 3)
#define NSR_TX1END              (1 << 2)
#define NSR_RXOV                (1 << 1)

#define TCR_TJDIS               (1 << 6)
#define TCR_EXCECM              (1 << 5)
#define TCR_PAD_DIS2            (1 << 4)
#define TCR_CRC_DIS2            (1 << 3)
#define TCR_PAD_DIS1            (1 << 2)
#define TCR_CRC_DIS1            (1 << 1)
#define TCR_TXREQ               (1 << 0)

#define TSR_TJTO                (1 << 7)
#define TSR_LC                  (1 << 6)
#define TSR_NC                  (1 << 5)
#define TSR_LCOL                (1 << 4)
#define TSR_COL                 (1 << 3)
#define TSR_EC                  (1 << 2)

#define RCR_WTDIS               (1 << 6)
#define RCR_DIS_LONG            (1 << 5)
#define RCR_DIS_CRC             (1 << 4)
#define RCR_ALL                 (1 << 3)
#define RCR_RUNT                (1 << 2)
#define RCR_PRMSC               (1 << 1)
#define RCR_RXEN                (1 << 0)

#define RSR_RF                  (1 << 7)
#define RSR_MF                  (1 << 6)
#define RSR_LCS                 (1 << 5)
#define RSR_RWTO                (1 << 4)
#define RSR_PLE                 (1 << 3)
#define RSR_AE                  (1 << 2)
#define RSR_CE                  (1 << 1)
#define RSR_FOE                 (1 << 0)

#define EPCR_EPOS_PHY           (1 << 3)
#define EPCR_EPOS_EE            (0 << 3)
#define EPCR_ERPRR              (1 << 2)
#define EPCR_ERPRW              (1 << 1)
#define EPCR_ERRE               (1 << 0)

#define FCTR_HWOT(ot)           ((ot & 0xf) << 4)
#define FCTR_LWOT(ot)           (ot & 0xf)

#define BPTR_BPHW(x)            ((x) << 4)
#define BPTR_JPT_200US          (0x07)
#define BPTR_JPT_600US          (0x0f)

#define IMR_PAR                 (1 << 7)
#define IMR_ROOM                (1 << 3)
#define IMR_ROM                 (1 << 2)
#define IMR_PTM                 (1 << 1)
#define IMR_PRM                 (1 << 0)

#define ISR_ROOS                (1 << 3)
#define ISR_ROS                 (1 << 2)
#define ISR_PTS                 (1 << 1)
#define ISR_PRS                 (1 << 0)
#define ISR_CLR_STATUS          (ISR_ROOS | ISR_ROS | ISR_PTS | ISR_PRS)

#define GPCR_GPIO0_OUT          (1 << 0)

#define GPR_PHY_PWROFF          (1 << 0)
/*********************************************************************************************************
  private data
*********************************************************************************************************/
struct dm9000_priv {
    atomic_t tx_pkt_cnt;
    UINT16   queue_pkt_len;

    LW_OBJECT_HANDLE  lock;
    LW_OBJECT_HANDLE  tx_sync;

    struct dm9000_netdev *dm9000;

    void (*outblk)(struct dm9000_netdev *dm9000, const void *data, int len);
    void (*inblk)(struct dm9000_netdev *dm9000, void *data, int len);
    void (*dummy_inblk)(struct dm9000_netdev *dm9000, int len);
    void (*rx_status)(struct dm9000_netdev *dm9000, UINT16 *status, UINT16 *len);
};
/*********************************************************************************************************
** 函数名称: dm9000_outblk_8
** 功能描述: dm9000 out block 8bits
** 输　入  : dm9000      netdev
**           data        data buffer
**           len         len in bytes
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void  dm9000_outblk_8 (struct dm9000_netdev *dm9000, const void *data, int len)
{
    writes8(dm9000->data, data, len);
}
/*********************************************************************************************************
** 函数名称: dm9000_outblk_16
** 功能描述: dm9000 out block 16bits
** 输　入  : dm9000      netdev
**           data        data buffer
**           len         len in bytes
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void  dm9000_outblk_16 (struct dm9000_netdev *dm9000, const void *data, int len)
{
    writes16(dm9000->data, data, (((len) + 1) >> 1));
}
/*********************************************************************************************************
** 函数名称: dm9000_outblk_32
** 功能描述: dm9000 out block 32bits
** 输　入  : dm9000      netdev
**           data        data buffer
**           len         len in bytes
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void  dm9000_outblk_32 (struct dm9000_netdev *dm9000, const void *data, int len)
{
    writes32(dm9000->data, data, (((len) + 3) >> 2));
}
/*********************************************************************************************************
** 函数名称: dm9000_inblk_8
** 功能描述: dm9000 in block 8bits
** 输　入  : dm9000      netdev
**           data        data buffer
**           len         len in bytes
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void  dm9000_inblk_8 (struct dm9000_netdev *dm9000, void *data, int len)
{
    reads8(dm9000->data, data, len);
}
/*********************************************************************************************************
** 函数名称: dm9000_inblk_16
** 功能描述: dm9000 in block 16bits
** 输　入  : dm9000      netdev
**           data        data buffer
**           len         len in bytes
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void  dm9000_inblk_16 (struct dm9000_netdev *dm9000, void *data, int len)
{
    reads16(dm9000->data, data, (((len) + 1) >> 1));
}
/*********************************************************************************************************
** 函数名称: dm9000_inblk_32
** 功能描述: dm9000 in block 32bits
** 输　入  : dm9000      netdev
**           data        data buffer
**           len         len in bytes
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void  dm9000_inblk_32 (struct dm9000_netdev *dm9000, void *data, int len)
{
    reads32(dm9000->data, data, (((len) + 3) >> 2));
}
/*********************************************************************************************************
** 函数名称: dm9000_inblk_8
** 功能描述: dm9000 dummy in block 8bits
** 输　入  : dm9000      netdev
**           len         len in bytes
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void  dm9000_dummy_inblk_8 (struct dm9000_netdev *dm9000, int len)
{
    int i;

    for (i = 0; i < len; i++) {
        read8(dm9000->data);
    }
}
/*********************************************************************************************************
** 函数名称: dm9000_inblk_16
** 功能描述: dm9000 dummy in block 16bits
** 输　入  : dm9000      netdev
**           len         len in bytes
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void  dm9000_dummy_inblk_16 (struct dm9000_netdev *dm9000, int len)
{
    int i;

    len = (((len) + 1) >> 1);

    for (i = 0; i < len; i++) {
        read16(dm9000->data);
    }
}
/*********************************************************************************************************
** 函数名称: dm9000_inblk_32
** 功能描述: dm9000 dummy in block 32bits
** 输　入  : dm9000      netdev
**           len         len in bytes
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void  dm9000_dummy_inblk_32 (struct dm9000_netdev *dm9000, int len)
{
    int i;

    len = (((len) + 3) >> 2);

    for (i = 0; i < len; i++) {
        read32(dm9000->data);
    }
}
/*********************************************************************************************************
** 函数名称: dm9000_rx_status_8
** 功能描述: dm9000 read rx status 8bits
** 输　入  : dm9000      netdev
**           status      read buffer
**           len         len in bytes
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void  dm9000_rx_status_8 (struct dm9000_netdev *dm9000, UINT16 *status, UINT16 *len)
{
    write8(DM9000_MRCMD, dm9000->io);
    *status = le16_to_cpu(read8(dm9000->data) + (read8(dm9000->data) << 8));
    *len    = le16_to_cpu(read8(dm9000->data) + (read8(dm9000->data) << 8));
}
/*********************************************************************************************************
** 函数名称: dm9000_rx_status_16
** 功能描述: dm9000 read rx status 16bits
** 输　入  : dm9000      netdev
**           status      read buffer
**           len         len in bytes
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void  dm9000_rx_status_16 (struct dm9000_netdev *dm9000, UINT16 *status, UINT16 *len)
{
    write8(DM9000_MRCMD, dm9000->io);
    *status = le16_to_cpu(read16(dm9000->data));
    *len    = le16_to_cpu(read16(dm9000->data));
}
/*********************************************************************************************************
** 函数名称: dm9000_rx_status_32
** 功能描述: dm9000 read rx status 32bits
** 输　入  : dm9000      netdev
**           status      read buffer
**           len         len in bytes
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void  dm9000_rx_status_32 (struct dm9000_netdev *dm9000, UINT16 *status, UINT16 *len)
{
    UINT32 tmp;

    write8(DM9000_MRCMD, dm9000->io);
    tmp = read32(dm9000->data);
    *status = le16_to_cpu(tmp);
    *len    = le16_to_cpu(tmp >> 16);
}
/*********************************************************************************************************
** 函数名称: dm9000_io_read
** 功能描述: dm9000 io read
** 输　入  : dm9000      netdev
**           reg         register
** 输　出  : value
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static inline UINT8 dm9000_io_read (struct dm9000_netdev *dm9000, UINT8 reg)
{
    write8(reg, dm9000->io);
    return  (read8(dm9000->data));
}
/*********************************************************************************************************
** 函数名称: dm9000_io_write
** 功能描述: dm9000 io write
** 输　入  : dm9000      netdev
**           reg         register
**           value       value
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static inline void  dm9000_io_write (struct dm9000_netdev *dm9000, UINT8 reg, UINT8 value)
{
    write8(reg, dm9000->io);
    write8(value, dm9000->data);
}
/*********************************************************************************************************
** 函数名称: dm9000_phy_read
** 功能描述: dm9000 phy read
** 输　入  : dm9000      netdev
**           reg         register
** 输　出  : value
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static inline UINT16 dm9000_phy_read (struct dm9000_netdev *dm9000, UINT8 reg)
{
    UINT16  value;

    dm9000_io_write(dm9000, DM9000_EPAR, DM9000_PHY | reg);

    dm9000_io_write(dm9000, DM9000_EPCR, 0x0c);                         /*  Issue phyxcer read command  */
    bspDelayUs(100);                                                    /*  Wait read complete          */
    dm9000_io_write(dm9000, DM9000_EPCR, 0x00);                         /*  Clear phyxcer read command  */

    value = (dm9000_io_read(dm9000, DM9000_EPDRH) << 8) |
             dm9000_io_read(dm9000, DM9000_EPDRL);

    return  (value);
}
/*********************************************************************************************************
** 函数名称: dm9000_phy_write
** 功能描述: dm9000 phy write
** 输　入  : dm9000      netdev
**           reg         register
**           value       value
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static inline void  dm9000_phy_write (struct dm9000_netdev *dm9000, UINT8  reg, UINT16  value)
{
    dm9000_io_write(dm9000, DM9000_EPAR, DM9000_PHY | reg);

    dm9000_io_write(dm9000, DM9000_EPDRL, (value & 0xff));
    dm9000_io_write(dm9000, DM9000_EPDRH, ((value >> 8) & 0xff));

    dm9000_io_write(dm9000, DM9000_EPCR, 0x0A);                         /*  Issue phyxcer write command */
    bspDelayUs(500);                                                    /*  Wait write complete         */
    dm9000_io_write(dm9000, DM9000_EPCR, 0x00);                         /*  Clear phyxcer write command */
}
/*********************************************************************************************************
** 函数名称: dm9000_probe
** 功能描述: dm9000 probe
** 输　入  : dm9000      netdev
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int  dm9000_probe (struct dm9000_netdev *dm9000)
{
    UINT32  id;

    id  = dm9000_io_read(dm9000, DM9000_VIDL);
    id |= dm9000_io_read(dm9000, DM9000_VIDH) << 8;
    id |= dm9000_io_read(dm9000, DM9000_PIDL) << 16;
    id |= dm9000_io_read(dm9000, DM9000_PIDH) << 24;

    if (id == DM9000_ID) {
        printk(KERN_INFO "dm9000_probe: found at i/o: 0x%lx, id: 0x%x\n", dm9000->base, id);
        return  (0);

    } else {
        printk(KERN_ERR "dm9000_probe: Not found at i/o: 0x%lx, id: 0x%x\n", dm9000->base, id);
        return  (-1);
    }
}
/*********************************************************************************************************
** 函数名称: dm9000_reset
** 功能描述: dm9000 reset
** 输　入  : dm9000      netdev
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int  dm9000_reset (struct dm9000_netdev *dm9000)
{
    /*
     * Make all GPIO0 outputs, all others inputs.
     */
    dm9000_io_write(dm9000, DM9000_GPCR, 0x0f);

    /*
     * Step 1: Power internal PHY by writing 0 to GPIO0 pin
     */
    dm9000_io_write(dm9000, DM9000_GPR, 0x00);

    /*
     * Step 2: Software reset
     */
    dm9000_io_write(dm9000, DM9000_NCR, (NCR_LBK_INT_MAC | NCR_RST));

    do {
        bspDelayUs(20);                                                 /*  Wait at least 20 us         */
    } while (dm9000_io_read(dm9000, DM9000_NCR) & 1);

    dm9000_io_write(dm9000, DM9000_NCR, 0x00);
    dm9000_io_write(dm9000, DM9000_NCR, (NCR_LBK_INT_MAC | NCR_RST));   /*  Issue a second reset        */

    do {
        bspDelayUs(20);                                                 /*  Wait at least 20 us         */
    } while (dm9000_io_read(dm9000, DM9000_NCR) & 1);

    /*
     * Check whether the ethernet controller is present
     */
    if ((dm9000_io_read(dm9000, DM9000_PIDL) != 0x00) ||
        (dm9000_io_read(dm9000, DM9000_PIDH) != 0x90)) {
        printk(KERN_ERR "dm9000_reset: no respond\n");
        return  (-1);
    }

    return  (0);
}
/*********************************************************************************************************
** 函数名称: dm9000_init
** 功能描述: dm9000 init
** 输　入  : netdev      netdev
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int  dm9000_init (struct netdev *netdev)
{
    struct dm9000_priv   *priv = netdev->priv;
    struct dm9000_netdev *dm9000 = (struct dm9000_netdev *)netdev;

    UINT8  mode, oft;
    int    ret;
    int    i;
    UINT16 hash_table[4];

    ret = dm9000_reset(dm9000);
    if (ret < 0) {
        return  (-1);
    }

    ret = dm9000_probe(dm9000);
    if (ret < 0) {
        return  (-1);
    }

    /*
     * Auto-detect 8/16/32 bit mode, ISR Bit 6+7 indicate bus width
     */
    mode = dm9000_io_read(dm9000, DM9000_ISR) >> 6;
    switch (mode) {

    case 0x00:                                                          /* 16-bit mode                  */
        printk(KERN_INFO "dm9000_init: running in 16 bit mode\n");
        priv->outblk      = dm9000_outblk_16;
        priv->inblk       = dm9000_inblk_16;
        priv->dummy_inblk = dm9000_dummy_inblk_16;
        priv->rx_status   = dm9000_rx_status_16;
        break;

    case 0x01:                                                          /* 32-bit mode                  */
        printk(KERN_INFO "dm9000_init: running in 32 bit mode\n");
        priv->outblk      = dm9000_outblk_32;
        priv->inblk       = dm9000_inblk_32;
        priv->dummy_inblk = dm9000_dummy_inblk_32;
        priv->rx_status   = dm9000_rx_status_32;
        break;

    case 0x02:                                                          /* 8 bit mode                   */
        printk(KERN_INFO "dm9000_init: running in 8 bit mode\n");
        priv->outblk      = dm9000_outblk_8;
        priv->inblk       = dm9000_inblk_8;
        priv->dummy_inblk = dm9000_dummy_inblk_8;
        priv->rx_status   = dm9000_rx_status_8;
        break;

    default:                                                            /* Assume 8 bit mode            */
        printk(KERN_INFO "dm9000_init: running in 0x%02x bit mode\n", mode);
        priv->outblk      = dm9000_outblk_8;
        priv->inblk       = dm9000_inblk_8;
        priv->dummy_inblk = dm9000_dummy_inblk_8;
        priv->rx_status   = dm9000_rx_status_8;
        break;                                                          /* will probably not work anyway*/
    }

    /*
     * Program operating register, only internal phy supported
     */
    dm9000_io_write(dm9000, DM9000_NCR, 0x00);

    /*
     * TX Polling clear
     */
    dm9000_io_write(dm9000, DM9000_TCR, 0x00);

    /*
     * Less 3Kb, 200us
     */
    dm9000_io_write(dm9000, DM9000_BPTR, BPTR_BPHW(3) | BPTR_JPT_600US);

    /*
     * Flow Control: High/Low Water
     */
    dm9000_io_write(dm9000, DM9000_FCTR, FCTR_HWOT(3) | FCTR_LWOT(8));

    /*
     * TODO: This looks strange! Flow Control
     */
    dm9000_io_write(dm9000, DM9000_FCR, 0xff);

    /*
     * Special Mode
     */
    dm9000_io_write(dm9000, DM9000_SMCR, 0x00);

    /*
     * Clear TX status
     */
    dm9000_io_write(dm9000, DM9000_NSR, NSR_WAKEST | NSR_TX2END | NSR_TX1END);

    /*
     * Clear interrupt status
     */
    dm9000_io_write(dm9000, DM9000_ISR, ISR_CLR_STATUS);

    /*
     * Fill device MAC address registers
     */
    for (i = 0, oft = DM9000_PAR; i < 6; i++, oft++) {
        dm9000_io_write(dm9000, oft, dm9000->netdev.hwaddr[i]);
    }

    /*
     * Clear Hash Table
     */
    for (i = 0; i < 4; i++) {
        hash_table[i] = 0x0;
    }

    /*
     * broadcast address
     */
    hash_table[3] = 0x8000;

    for (i = 0, oft = DM9000_MAR; i < 4; i++) {
        dm9000_io_write(dm9000, oft++, hash_table[i]);
        dm9000_io_write(dm9000, oft++, hash_table[i] >> 8);
    }

    /*
     * Read back MAC address
     */
    printk(KERN_INFO "dm9000_init: MAC=");
    printk(KERN_INFO "%02x:", dm9000_io_read(dm9000, DM9000_PAR + 0));
    printk(KERN_INFO "%02x:", dm9000_io_read(dm9000, DM9000_PAR + 1));
    printk(KERN_INFO "%02x:", dm9000_io_read(dm9000, DM9000_PAR + 2));
    printk(KERN_INFO "%02x:", dm9000_io_read(dm9000, DM9000_PAR + 3));
    printk(KERN_INFO "%02x:", dm9000_io_read(dm9000, DM9000_PAR + 4));
    printk(KERN_INFO "%02x\n",dm9000_io_read(dm9000, DM9000_PAR + 5));

    /*
     * Activate DM9000
     */
    /*
     * RX enable
     */
    dm9000_io_write(dm9000, DM9000_RCR, RCR_DIS_LONG | RCR_DIS_CRC | RCR_RXEN);

    /*
     * Enable TX/RX interrupt mask
     */
    dm9000_io_write(dm9000, DM9000_IMR, IMR_PAR | IMR_ROOM | IMR_ROM | IMR_PTM | IMR_PRM);

    API_InterVectorEnable(dm9000->irq);

    return  (0);
}
/*********************************************************************************************************
** 函数名称: dm9000_drop
** 功能描述: dm9000 drop input packet
** 输　入  : dm9000      netdev
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void dm9000_drop (struct dm9000_netdev *dm9000)
{
    struct dm9000_priv  *priv = dm9000->netdev.priv;
    UINT8    rx_byte;
    UINT16   rx_len;
    UINT16   rx_status;

    do {
        dm9000_io_read(dm9000, DM9000_MRCMDX);
        rx_byte = read8(dm9000->data) & 0x03;
        if (rx_byte & DM9000_PKT_ERR) {
            netdev_linkinfo_err_inc(&dm9000->netdev);
            return;
        }

        if (!(rx_byte & DM9000_PKT_RDY)) {
            netdev_linkinfo_err_inc(&dm9000->netdev);
            return;
        }

        (priv->rx_status)(dm9000, &rx_status, &rx_len);
        if (rx_len < 0x40) {
            netdev_linkinfo_lenerr_inc(&dm9000->netdev);
        }

        if (rx_len > DM9000_PKT_MAX) {
            netdev_linkinfo_lenerr_inc(&dm9000->netdev);
        }

        rx_status >>= 8;
        if (rx_status & (RSR_FOE | RSR_CE | RSR_AE |
                         RSR_PLE | RSR_RWTO |
                         RSR_LCS | RSR_RF)) {
            if (rx_status & RSR_FOE) {
                netdev_linkinfo_err_inc(&dm9000->netdev);
            }

            if (rx_status & RSR_CE) {
                netdev_linkinfo_chkerr_inc(&dm9000->netdev);
            }

            if (rx_status & RSR_RF) {
                netdev_linkinfo_lenerr_inc(&dm9000->netdev);
            }
        }

        priv->dummy_inblk(dm9000, rx_len);
        netdev_linkinfo_drop_inc(&dm9000->netdev);
        netdev_statinfo_discards_inc(&dm9000->netdev, LINK_INPUT);
    } while (rx_byte & DM9000_PKT_RDY);
}
/*********************************************************************************************************
** 函数名称: dm9000_rxmode
** 功能描述: dm9000 set rx mode
** 输　入  : dm9000      netdev
**           flags       new flags
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int  dm9000_rxmode (struct netdev *netdev, int flags)
{
    struct dm9000_priv   *priv = netdev->priv;
    struct dm9000_netdev *dm9000 = (struct dm9000_netdev *)netdev;
    struct netdev_mac    *ha;
    int i, oft;
    UINT32 hash_val;
    UINT16 hash_table[4] = { 0, 0, 0, 0x8000 };                         /* broadcast address            */
    UINT8  rcr = RCR_DIS_LONG | RCR_DIS_CRC | RCR_RXEN;

    API_SemaphoreMPend(priv->lock, LW_OPTION_WAIT_INFINITE);
    API_InterVectorDisable(dm9000->irq);

    if (flags & IFF_PROMISC) {
        rcr |= RCR_PRMSC;
    }

    if (flags & IFF_ALLMULTI) {
        rcr |= RCR_ALL;
    }

    /*
     * the multicast address in Hash Table : 64 bits
     */
    NETDEV_MACFILTER_FOREACH(netdev, ha) {
        hash_val = ether_crc_le(6, ha->hwaddr) & 0x3f;
        hash_table[hash_val / 16] |= (UINT16) 1 << (hash_val % 16);
    }

    /*
     * Write the hash table to MAC MD table
     */
    for (i = 0, oft = DM9000_MAR; i < 4; i++) {
        dm9000_io_write(dm9000, oft++, hash_table[i]);
        dm9000_io_write(dm9000, oft++, hash_table[i] >> 8);
    }

    dm9000_io_write(dm9000, DM9000_RCR, rcr);

    API_InterVectorEnable(dm9000->irq);
    API_SemaphoreMPost(priv->lock);

    return  (0);
}
/*********************************************************************************************************
** 函数名称: dm9000_receive
** 功能描述: dm9000 receive packet
** 输　入  : netdev      netdev
**           input       system input function
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void  dm9000_receive (struct netdev *netdev, int (*input)(struct netdev *, struct pbuf *))
{
    struct dm9000_priv   *priv = netdev->priv;
    struct dm9000_netdev *dm9000 = (struct dm9000_netdev *)netdev;

    struct pbuf *p, *q;
    UINT8  reg;
    UINT8  rx_byte;
    UINT16 rx_len;
    UINT16 rx_status;
    int    good_pkt;
    int    err;

    API_SemaphoreMPend(priv->lock, LW_OPTION_WAIT_INFINITE);
    API_InterVectorDisable(dm9000->irq);

    do {
        dm9000_io_read(dm9000, DM9000_MRCMDX);
        rx_byte = read8(dm9000->data) & 0x03;
        if (rx_byte & DM9000_PKT_ERR) {
            netdev_linkinfo_err_inc(netdev);
            goto    __recv_over;
        }

        if (!(rx_byte & DM9000_PKT_RDY)) {
            netdev_linkinfo_err_inc(netdev);
            goto    __recv_over;
        }

        good_pkt = TRUE;

        (priv->rx_status)(dm9000, &rx_status, &rx_len);
        if (rx_len < 0x40) {
            netdev_linkinfo_lenerr_inc(netdev);
            good_pkt = FALSE;
        }

        if (rx_len > DM9000_PKT_MAX) {
            netdev_linkinfo_lenerr_inc(netdev);
            good_pkt = FALSE;
        }

        rx_status >>= 8;
        if (rx_status & (RSR_FOE | RSR_CE | RSR_AE |
                         RSR_PLE | RSR_RWTO |
                         RSR_LCS | RSR_RF)) {
            if (rx_status & RSR_FOE) {
                netdev_linkinfo_err_inc(netdev);
            }

            if (rx_status & RSR_CE) {
                netdev_linkinfo_chkerr_inc(netdev);
            }

            if (rx_status & RSR_RF) {
                netdev_linkinfo_lenerr_inc(netdev);
            }
            good_pkt = FALSE;
        }

        if (good_pkt) {
            if ((p = netdev_pbuf_alloc(rx_len)) != NULL) {
                for (q = p; q != NULL; q = q->next) {
                    (priv->inblk)(dm9000, q->payload, q->len);
                }

                err = input(netdev, p);
                if (err) {
                    netdev_pbuf_free(p);
                    p = NULL;

                    netdev_linkinfo_drop_inc(netdev);
                    netdev_statinfo_discards_inc(netdev, LINK_INPUT);

                } else {
                    netdev_linkinfo_recv_inc(netdev);
                    netdev_statinfo_total_add(netdev, LINK_INPUT, rx_len);
                    if (((UINT8 *)p->payload)[0] & 1) {
                        netdev_statinfo_mcasts_inc(netdev, LINK_INPUT);
                    } else {
                        netdev_statinfo_ucasts_inc(netdev, LINK_INPUT);
                    }
                }

            } else {
                priv->dummy_inblk(dm9000, rx_len);

                netdev_linkinfo_memerr_inc(netdev);
                netdev_linkinfo_drop_inc(netdev);
                netdev_statinfo_discards_inc(netdev, LINK_INPUT);
            }

        } else {
            priv->dummy_inblk(dm9000, rx_len);

            netdev_linkinfo_drop_inc(netdev);
            netdev_statinfo_discards_inc(netdev, LINK_INPUT);
        }
    } while (rx_byte == DM9000_PKT_RDY);

__recv_over:
    reg  = dm9000_io_read(dm9000, DM9000_IMR);
    reg |= IMR_PRM;
    dm9000_io_write(dm9000, DM9000_IMR, reg);

    API_InterVectorEnable(dm9000->irq);
    API_SemaphoreMPost(priv->lock);
}
/*********************************************************************************************************
** 函数名称: dm9000_transmit
** 功能描述: dm9000 transmit packet
** 输　入  : netdev      netdev
**           p           packet to transmit
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int  dm9000_transmit (struct netdev *netdev, struct pbuf *p)
{
    struct dm9000_priv   *priv = netdev->priv;
    struct dm9000_netdev *dm9000 = (struct dm9000_netdev *)netdev;

    struct pbuf *q;
    UINT16 tx_len;

    API_SemaphoreMPend(priv->lock, LW_OPTION_WAIT_INFINITE);
    API_InterVectorDisable(dm9000->irq);

    while (API_AtomicGet(&priv->tx_pkt_cnt) > 1) {
        API_InterVectorEnable(dm9000->irq);
        API_SemaphoreMPost(priv->lock);

        if (API_SemaphoreBPend(priv->tx_sync,
                               LW_MSECOND_TO_TICK_1(2000)) == ERROR_THREAD_WAIT_TIMEOUT) {
            netdev_linkinfo_drop_inc(netdev);
            netdev_statinfo_discards_inc(netdev, LINK_OUTPUT);
            return  (-1);

        } else {
            API_SemaphoreMPend(priv->lock, LW_OPTION_WAIT_INFINITE);
            API_InterVectorDisable(dm9000->irq);
        }
    }

    tx_len = p->tot_len;
    write8(DM9000_MWCMD, dm9000->io);

    for (q = p; q != NULL; q = q->next) {
        (priv->outblk)(dm9000, q->payload, q->len);
    }

    priv->queue_pkt_len = tx_len;

    if (API_AtomicInc(&priv->tx_pkt_cnt) == 1) {
        dm9000_io_write(dm9000, DM9000_TXPLL, (tx_len & 0xFF));
        dm9000_io_write(dm9000, DM9000_TXPLH, (tx_len >> 8) & 0xFF);
        dm9000_io_write(dm9000, DM9000_TCR, TCR_TXREQ);
    }

    API_InterVectorEnable(dm9000->irq);
    API_SemaphoreMPost(priv->lock);

    netdev_linkinfo_xmit_inc(netdev);
    netdev_statinfo_total_add(netdev, LINK_OUTPUT, tx_len);
    if (((UINT8 *)p->payload)[0] & 1) {
        netdev_statinfo_mcasts_inc(netdev, LINK_OUTPUT);
    } else {
        netdev_statinfo_ucasts_inc(netdev, LINK_OUTPUT);
    }

    return  (0);
}
/*********************************************************************************************************
** 函数名称: dm9000_watchdog
** 功能描述: dm9000 watchdog
** 输　入  : netdev      netdev
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void  dm9000_watchdog (struct netdev *netdev)
{
    struct dm9000_netdev *dm9000 = (struct dm9000_netdev *)netdev;
    UINT8  nsr, ncr;
    int    linkup;

    API_InterVectorDisable(dm9000->irq);

    if ((dm9000_phy_read(dm9000, 1) & 0x20)) {
        netdev_get_linkup(netdev, &linkup);
        if (!linkup || (netdev->speed == 0)) {
            nsr = dm9000_io_read(dm9000, DM9000_NSR);
            ncr = dm9000_io_read(dm9000, DM9000_NCR);
            netdev_set_linkup(netdev, 1, (nsr & NSR_SPEED) ? 100000000 : 10000000);
            printk(KERN_INFO "dm9000_watchdog: operating at %dM %s duplex mode\n",
                   (nsr & NSR_SPEED) ? 10 : 100, (ncr & NCR_FDX) ? "full" : "half");
        }

    } else {
        netdev_get_linkup(netdev, &linkup);
        if (linkup) {
            netdev_set_linkup(netdev, 0, 0);
            printk(KERN_INFO "dm9000_watchdog: can not establish link\n");
        }
    }

    API_InterVectorEnable(dm9000->irq);
}
/*********************************************************************************************************
** 函数名称: dm9000_txend
** 功能描述: dm9000 send end
** 输　入  : dm9000      netdev
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void  dm9000_txend (struct dm9000_netdev *dm9000)
{
    struct dm9000_priv  *priv = dm9000->netdev.priv;
    UINT8  tx_status;

    tx_status = dm9000_io_read(dm9000, DM9000_NSR);                     /*  Get TX status               */
    if (tx_status & (NSR_TX2END | NSR_TX1END)) {
        if (API_AtomicGet(&priv->tx_pkt_cnt) > 0) {                     /*  Queue packet check & send   */
            if (API_AtomicDec(&priv->tx_pkt_cnt) > 0) {
                dm9000_io_write(dm9000, DM9000_TXPLL, (priv->queue_pkt_len & 0xff));
                dm9000_io_write(dm9000, DM9000_TXPLH, (priv->queue_pkt_len >> 8) & 0xff);
                dm9000_io_write(dm9000, DM9000_TCR, TCR_TXREQ);
            }
            API_SemaphoreBFlush(priv->tx_sync, NULL);
        }
    }
}
/*********************************************************************************************************
** 函数名称: dm9000Isr
** 功能描述: dm9000 isr
** 输　入  : dm9000      netdev
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  dm9000Isr (struct dm9000_netdev *dm9000)
{
    UINT8  int_status;
    UINT8  last_io;
    UINT8  reg;

    last_io = read8(dm9000->io);                                        /*  Save previous io register   */

    reg = dm9000_io_read(dm9000, DM9000_IMR);                           /*  Disable all interrupts      */
    dm9000_io_write(dm9000, DM9000_IMR, IMR_PAR);

    int_status = dm9000_io_read(dm9000, DM9000_ISR);                    /*  Get ISR status              */
    dm9000_io_write(dm9000, DM9000_ISR, int_status);                    /*  Clear ISR status            */

    if (int_status & ISR_ROS) {
        printk(KERN_ERR "dm9000_isr: rx overflow\n");
        netdev_linkinfo_err_inc(&dm9000->netdev);
    }

    if (int_status & ISR_ROOS) {
        printk(KERN_ERR "dm9000_isr: rx overflow counter overflow\n");
        netdev_linkinfo_err_inc(&dm9000->netdev);
    }

    if (int_status & ISR_PRS) {                                         /*  Received the coming packet  */
        if (netdev_notify(&dm9000->netdev, LINK_INPUT, 1) == 0) {
            reg &= ~(IMR_PRM);                                          /*  disable recv interrupt      */

        } else {
            dm9000_drop(dm9000);
        }
    }

    if (int_status & ISR_PTS) {                                         /*  Transmit Interrupt check    */
        dm9000_txend(dm9000);
    }

    dm9000_io_write(dm9000, DM9000_IMR, reg);                           /*  Re-enable interrupt mask    */

    write8(last_io, dm9000->io);                                        /*  Restore previous io register*/
}
/*********************************************************************************************************
** 函数名称: dm9000Init
** 功能描述: dm9000 init
** 输　入  : dm9000      netdev
**           ip          default ip address string
**           netmask     default netmask address string
**           gw          default gw address string
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  dm9000Init (struct dm9000_netdev *dm9000, const char *ip, const char *netmask, const char *gw)
{
    static struct netdev_funcs  dm9000_drv = {
        .init     = dm9000_init,
        .rxmode   = dm9000_rxmode,
        .transmit = dm9000_transmit,
        .receive  = dm9000_receive,
    };

    struct dm9000_priv *priv;
    LW_OBJECT_HANDLE handle;
    int i;

    if (dm9000 == NULL) {
        return  (-1);
    }

    priv = (struct dm9000_priv *)sys_malloc(sizeof(struct dm9000_priv));
    if (priv == NULL) {
        printk(KERN_ERR "dm9000_netif_init: out of memory\n");
        return  (-1);
    }

    priv->queue_pkt_len = 0;
    API_AtomicSet(0, &priv->tx_pkt_cnt);

    handle = API_SemaphoreMCreate("dm9000_lock", 0,
                                  LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                  &priv->lock);
    if (handle == LW_HANDLE_INVALID) {
        sys_free(priv);
        return  (-1);
    }

    handle = API_SemaphoreBCreate("dm9000_txsync", 0,
                                  LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                  &priv->tx_sync);
    if (handle == LW_HANDLE_INVALID) {
        API_SemaphoreMDelete(&priv->lock);
        sys_free(priv);
        return  (-1);
    }

    dm9000->netdev.magic_no = NETDEV_MAGIC;

    if (dm9000->netdev.dev_name[0] == '\0') {
        lib_strcpy(dm9000->netdev.dev_name, "dm9000");
    }
    if (dm9000->netdev.if_name[0] == '\0') {
        lib_strcpy(dm9000->netdev.if_name,  "en");
    }

    dm9000->netdev.if_hostname = "SylixOS";
    dm9000->netdev.init_flags = NETDEV_INIT_LOAD_PARAM
                              | NETDEV_INIT_LOAD_DNS
                              | NETDEV_INIT_IPV6_AUTOCFG
                              | NETDEV_INIT_AS_DEFAULT;

    dm9000->netdev.chksum_flags = NETDEV_CHKSUM_ENABLE_ALL;
    dm9000->netdev.net_type = NETDEV_TYPE_ETHERNET;
    dm9000->netdev.speed = 0;
    dm9000->netdev.mtu = 1500;
    dm9000->netdev.hwaddr_len = 6;

    for (i = 0; i < 6; i++) {
        dm9000->netdev.hwaddr[i] = dm9000->mac[i];
    }

    dm9000->netdev.drv = &dm9000_drv;
    dm9000->netdev.priv = priv;

    if (dm9000->init) {
        dm9000->init(dm9000);
    }

    if (netdev_add(&dm9000->netdev, ip, netmask, gw,
                   IFF_UP | IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST) == 0) {
        netdev_linkup_wd_add(&dm9000->netdev, dm9000_watchdog);
        return  (0);

    } else {
        API_SemaphoreMDelete(&priv->lock);
        API_SemaphoreBDelete(&priv->tx_sync);
        sys_free(priv);
        return  (-1);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
                                                                        /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_DRV_NIC_DM9000       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
