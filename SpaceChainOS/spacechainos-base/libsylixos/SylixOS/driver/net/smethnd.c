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
** 文   件   名: smethnd.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 07 月 24 日
**
** 描        述: 共享内存虚拟网卡驱动支持.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "../SylixOS/config/driver/drv_cfg.h"
/*********************************************************************************************************
  依赖于 SPIN LOCK
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_NET_EN > 0) && (LW_CFG_DRV_NIC_SMETHND > 0)
#include "net/if_ether.h"
#include "smethnd.h"
/*********************************************************************************************************
  smethnd config macro
*********************************************************************************************************/
#define SMETHND_CONFIG_SIZE     128                                     /*  config size                 */

#define SMETHND_CONFIG_LOCK(config)                 \
        do {                                        \
            __ARCH_SPIN_LOCK_RAW(&config->lock);    \
            KN_SMP_MB();                            \
        } while (0)
#define SMETHND_CONFIG_UNLOCK(config)               \
        do {                                        \
            KN_SMP_MB();                            \
            __ARCH_SPIN_UNLOCK_RAW(&config->lock);  \
        } while (0)
/*********************************************************************************************************
  smethnd config
*********************************************************************************************************/
struct smethnd_config {
    UINT8           hwaddr[6];                                          /*  6 bytes hwaddr              */
    UINT16          flag;
#define SMETHND_FLAG_UP         0x01
#define SMETHND_FLAG_BROADCAST  0x02

    UINT32          packet_base_hi;
    UINT32          packet_base_lo;

    UINT32          block_msg;
    UINT32          block_cnt;

    UINT32          packet_in;
    UINT32          packet_out;

    spinlock_t      lock;
    UINT32          reserved[8];
} __attribute__((packed));
/*********************************************************************************************************
  smethnd packet macro
*********************************************************************************************************/
#define SMETHND_PACKET_TOTAL_SHIFT      (12)                            /*  packet buffer total 2 ^ 12  */
#define SMETHND_PACKET_TOTAL_SIZE       (4096)                          /*  packet buffer total size    */
#define SMETHND_PACKET_LEN_SIZE         (4)                             /*  packet lenght size          */
#define SMETHND_PACKET_PAYLOAD_SIZE     (4096 - 4)                      /*  packet payload size         */
/*********************************************************************************************************
  smethnd packet
*********************************************************************************************************/
struct smethnd_packet {
    UINT32          len;
    UINT8           packet[SMETHND_PACKET_PAYLOAD_SIZE];
} __attribute__((packed));
/*********************************************************************************************************
** 函数名称: smethnd_up
** 功能描述: share memory ethernet device up
** 输　入  : netdev      netdev
** 输　出  : -1 or 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int  smethnd_up (struct netdev *netdev)
{
    struct smethnd_netdev *smethnd = (struct smethnd_netdev *)netdev->priv;
    struct smethnd_config *config  = (struct smethnd_config *)smethnd->mem.config_base;

    SMETHND_CONFIG_LOCK(config);
    config->flag |= SMETHND_FLAG_UP;
    SMETHND_CONFIG_UNLOCK(config);

    return  (0);
}
/*********************************************************************************************************
** 函数名称: smethnd_down
** 功能描述: share memory ethernet device down
** 输　入  : netdev      netdev
** 输　出  : -1 or 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int  smethnd_down (struct netdev *netdev)
{
    struct smethnd_netdev *smethnd = (struct smethnd_netdev *)netdev->priv;
    struct smethnd_config *config  = (struct smethnd_config *)smethnd->mem.config_base;

    SMETHND_CONFIG_LOCK(config);
    config->flag &= ~SMETHND_FLAG_UP;
    SMETHND_CONFIG_UNLOCK(config);

    return  (0);
}
/*********************************************************************************************************
** 函数名称: smethnd_sendto
** 功能描述: share memory ethernet device send a pakcet to remote
** 输　入  : smethnd     share memory ethernet device
**           config      remote ethernet device config
**           remoteno    remoteno
**           p           packet to transmit
** 输　出  : -1 or 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int  smethnd_sendto (struct smethnd_netdev *smethnd,
                            struct smethnd_config *config,
                            int                    remoteno,
                            struct pbuf           *p)
{
    int                    ret;
    struct netdev         *netdev = &smethnd->netdev;
    struct smethnd_packet *packet;
    addr_t                 packet_base;
    size_t                 offset = 0;
    struct pbuf           *q;

    SMETHND_CONFIG_LOCK(config);
    if ((config->flag & SMETHND_FLAG_UP) &&
        (config->block_msg < config->block_cnt)) {
#if LW_CFG_CPU_WORD_LENGHT == 32
        packet_base = config->packet_base_lo;
#else
        packet_base = ((addr_t)config->packet_base_hi << 32) + config->packet_base_lo;
#endif
        packet = (struct smethnd_packet *)(packet_base
               + (config->packet_out << SMETHND_PACKET_TOTAL_SHIFT));

        for (q = p; q != NULL; q = q->next) {
            MEMCPY(&packet->packet[offset], q->payload, q->len);
            offset += q->len;
        }

        packet->len = p->tot_len;

        config->block_msg++;
        config->packet_out++;
        if (config->packet_out >= config->block_cnt) {
            config->packet_out  = 0;
        }
        ret = 0;

    } else {
        ret = -1;
    }
    SMETHND_CONFIG_UNLOCK(config);

    if (ret == 0) {
        netdev_linkinfo_xmit_inc(netdev);
        netdev_statinfo_total_add(netdev, LINK_OUTPUT, p->tot_len);
        if (((UINT8 *)p->payload)[0] & 1) {
            netdev_statinfo_mcasts_inc(netdev, LINK_OUTPUT);
        } else {
            netdev_statinfo_ucasts_inc(netdev, LINK_OUTPUT);
        }
        smethnd->notify(smethnd, remoteno);

    } else {
        netdev_linkinfo_drop_inc(netdev);
        netdev_statinfo_discards_inc(netdev, LINK_OUTPUT);
    }

    return  (ret);
}
/*********************************************************************************************************
** 函数名称: smethnd_transmit
** 功能描述: share memory ethernet device transmit
** 输　入  : netdev      netdev
**           p           packet to transmit
** 输　出  : -1 or 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int  smethnd_transmit (struct netdev *netdev, struct pbuf *p)
{
    struct smethnd_netdev *smethnd = (struct smethnd_netdev *)netdev->priv;
    struct smethnd_mem     mem;
    struct smethnd_config *config;
    struct ethhdr         *ethhdr;
    int                    i, remoteno;

    if (p->len <= SIZEOF_ETH_HDR) {
        netdev_linkinfo_memerr_inc(netdev);
        netdev_linkinfo_drop_inc(netdev);
        netdev_statinfo_discards_inc(netdev, LINK_OUTPUT);
        return  (-1);
    }

    ethhdr = (struct ethhdr *)p->payload;
    if (ethhdr->h_dest[0] & 1) {
        for (i = 0; i < smethnd->totalnum; i++) {
            if (i != smethnd->localno) {
                smethnd->meminfo(smethnd, i, &mem);
                config = (struct smethnd_config *)mem.config_base;
                if (config->flag & SMETHND_FLAG_BROADCAST) {
                    smethnd_sendto(smethnd, config, i, p);
                }
            }
        }

    } else {
        remoteno = ((UINT8 *)p->payload)[5];
        if (remoteno < smethnd->totalnum) {
            smethnd->meminfo(smethnd, remoteno, &mem);
            config = (struct smethnd_config *)mem.config_base;
            if (smethnd_sendto(smethnd, config, remoteno, p)) {
                return  (-1);
            }
        }
    }

    return  (0);
}
/*********************************************************************************************************
** 函数名称: smethnd_receive
** 功能描述: share memory ethernet device receive
** 输　入  : netdev      netdev
**           input       system input function
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void  smethnd_receive (struct netdev *netdev, int (*input)(struct netdev *, struct pbuf *))
{
    struct smethnd_netdev *smethnd = (struct smethnd_netdev *)netdev->priv;
    struct smethnd_config *config  = (struct smethnd_config *)smethnd->mem.config_base;
    struct smethnd_packet *packet;
    addr_t                 packet_base;
    size_t                 offset;
    struct pbuf           *p, *q;

    if (!config->block_msg) {
        return;
    }

#if LW_CFG_CPU_WORD_LENGHT == 32
    packet_base = config->packet_base_lo;
#else
    packet_base = ((addr_t)config->packet_base_hi << 32) + (config->packet_base_lo);
#endif
    packet = (struct smethnd_packet *)(packet_base
           + (config->packet_in << SMETHND_PACKET_TOTAL_SHIFT));

    do {
        if (packet->len) {
            p = netdev_pbuf_alloc((UINT16)packet->len);
            if (p) {
                offset = 0;
                for (q = p; q != NULL; q = q->next) {
                    MEMCPY(q->payload, &packet->packet[offset], q->len);
                    offset += q->len;
                }

                if (input(netdev, p)) {
                    netdev_pbuf_free(p);
                    p = NULL;

                    netdev_linkinfo_drop_inc(netdev);
                    netdev_statinfo_discards_inc(netdev, LINK_INPUT);

                } else {
                    netdev_linkinfo_recv_inc(netdev);
                    netdev_statinfo_total_add(netdev, LINK_INPUT, packet->len);
                    if (((UINT8 *)p->payload)[0] & 1) {
                        netdev_statinfo_mcasts_inc(netdev, LINK_INPUT);
                    } else {
                        netdev_statinfo_ucasts_inc(netdev, LINK_INPUT);
                    }
                }

            } else {
                netdev_linkinfo_memerr_inc(netdev);
                netdev_linkinfo_drop_inc(netdev);
                netdev_statinfo_discards_inc(netdev, LINK_INPUT);
            }

        } else {
            netdev_linkinfo_lenerr_inc(netdev);
            netdev_linkinfo_drop_inc(netdev);
            netdev_statinfo_discards_inc(netdev, LINK_INPUT);
        }

        SMETHND_CONFIG_LOCK(config);
        config->block_msg--;
        config->packet_in++;
        if (config->packet_in >= config->block_cnt) {
            config->packet_in =  0;
            packet            =  (struct smethnd_packet *)(packet_base);
        } else {
            packet++;
        }
        SMETHND_CONFIG_UNLOCK(config);

    } while (config->block_msg);
}
/*********************************************************************************************************
** 函数名称: smethndInit
** 功能描述: share memory ethernet device init
** 输　入  : smethnd     share memory ethernet device
**           ip          default ip address string
**           netmask     default netmask address string
**           gw          default gw address string
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  smethndInit (struct smethnd_netdev *smethnd, const char *ip, const char *netmask, const char *gw)
{
    static struct netdev_funcs  smethnd_drv = {
        .up       = smethnd_up,
        .down     = smethnd_down,
        .transmit = smethnd_transmit,
        .receive  = smethnd_receive,
    };

    struct netdev         *netdev = &smethnd->netdev;
    struct smethnd_mem    *mem    = &smethnd->mem;
    struct smethnd_config *config = (struct smethnd_config *)mem->config_base;

    if (mem->config_size < SMETHND_CONFIG_SIZE) {
        return  (-1);
    }

    if (mem->packet_size < (64 * LW_CFG_KB_SIZE)) {
        return  (-1);
    }

    if ((smethnd->localno < 0) || (smethnd->localno > 127)) {
        return  (-1);
    }

    config->hwaddr[0] = 0x00;
    config->hwaddr[1] = 0x53;
    config->hwaddr[2] = 0x5a;
    config->hwaddr[3] = 0x4c;
    config->hwaddr[4] = 0x49;
    config->hwaddr[5] = (UINT8)smethnd->localno;

    config->flag = SMETHND_FLAG_BROADCAST;

#if LW_CFG_CPU_WORD_LENGHT == 32
    config->packet_base_hi = 0;
#else
    config->packet_base_hi = mem->packet_base >> 32;
#endif
    config->packet_base_lo = mem->packet_base & 0xffffffff;

    config->block_msg = 0;
    config->block_cnt = (UINT32)mem->packet_size / SMETHND_PACKET_TOTAL_SIZE;

    config->packet_in  = 0;
    config->packet_out = 0;

    LW_SPIN_INIT(&config->lock);
    KN_SMP_MB();

    netdev->magic_no = NETDEV_MAGIC;

    snprintf(netdev->dev_name, IF_NAMESIZE, "SM-Ethernet%d", smethnd->localno);
    lib_strcpy(netdev->if_name, "se");

    netdev->if_hostname  = "SylixOS smethnd";
    netdev->init_flags   = NETDEV_INIT_LOAD_PARAM
                         | NETDEV_INIT_LOAD_DNS
                         | NETDEV_INIT_IPV6_AUTOCFG;
    netdev->chksum_flags = smethnd->chksum_flags;
    netdev->net_type     = NETDEV_TYPE_ETHERNET;
    netdev->speed        = 0;
    netdev->mtu          = SMETHND_PACKET_PAYLOAD_SIZE;

    netdev->hwaddr_len = 6;
    netdev->hwaddr[0]  = config->hwaddr[0];
    netdev->hwaddr[1]  = config->hwaddr[1];
    netdev->hwaddr[2]  = config->hwaddr[2];
    netdev->hwaddr[3]  = config->hwaddr[3];
    netdev->hwaddr[4]  = config->hwaddr[4];
    netdev->hwaddr[5]  = config->hwaddr[5];

    netdev->drv  = &smethnd_drv;
    netdev->priv = (void *)smethnd;

    return  (netdev_add(netdev, ip, netmask, gw,
                        IFF_UP | IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST));
}
/*********************************************************************************************************
** 函数名称: smethndNotify
** 功能描述: share memory ethernet device notify
** 输　入  : smethnd     share memory ethernet device
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID smethndNotify (struct smethnd_netdev *smethnd)
{
    if (LW_CPU_GET_CUR_NESTING()) {
        netdev_notify(&smethnd->netdev, LINK_INPUT, 1);

    } else {
        netdev_notify(&smethnd->netdev, LINK_INPUT, 0);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
                                                                        /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_DRV_NIC_SMETHND      */
#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
