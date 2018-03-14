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
** 文   件   名: pciStorageSata.c
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2016 年 03 月 17 日
**
** 描        述: AHCI 驱动 Marvell 88SE9215 (4-port Gen III 6Gb/s host controller) INTEL 等.

** BUG:
2016.11.07  探测函数中需要确认设备类型.
            ATI(0x1002) 0x4390 设备 ID 在匹配表内, 但是设备类型是 IDE
2016.11.09  增加将 AMD 桥 IDE 模式切换为 AHCI 模式的特殊处理.
2016.12.27  增加磁盘初始化复位等命令超时时间可配置.
2017.03.29  修正部分设备如 Sunrise Point-H 内存索引非 0 的问题. (v1.0.3-rc0)
*********************************************************************************************************/
#define  __SYLIXOS_PCI_DRV
#define  __SYLIXOS_AHCI_DRV
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "../SylixOS/config/driver/drv_cfg.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0) && (LW_CFG_AHCI_EN > 0) && (LW_CFG_DRV_SATA_AHCI > 0)
#include "pci_ids.h"
#include "linux/compat.h"
#include "pciStorageSata.h"
/*********************************************************************************************************
  板载类型.
*********************************************************************************************************/
enum {
    /* 
     * board IDs by feature in alphabetical order 
     */
    board_ahci,
    board_ahci_ign_iferr,
    board_ahci_nomsi,
    board_ahci_noncq,
    board_ahci_nosntf,
    board_ahci_yes_fbs,

    /* 
     * board IDs for specific chipsets in alphabetical order 
     */
    board_ahci_avn,
    board_ahci_mcp65,
    board_ahci_mcp77,
    board_ahci_mcp89,
    board_ahci_mv,
    board_ahci_sb600,
    board_ahci_sb700,                                                   /* for SB700 and SB800          */
    board_ahci_vt8251,

    /* 
     * aliases 
     */
    board_ahci_mcp_linux    = board_ahci_mcp65,
    board_ahci_mcp67    = board_ahci_mcp65,
    board_ahci_mcp73    = board_ahci_mcp65,
    board_ahci_mcp79    = board_ahci_mcp77
};
/*********************************************************************************************************
  驱动支持的设备 ID 表, 用于驱动与设备进行自动匹配, 与 Linux 参数保持一致.
*********************************************************************************************************/
static const PCI_DEV_ID_CB      pciStorageSataIdTbl[] = {
    /* 
     * Intel 
     */
    {   PCI_VDEVICE(INTEL, 0x2652), board_ahci  },                      /* ICH6                         */
    {   PCI_VDEVICE(INTEL, 0x2653), board_ahci  },                      /* ICH6M                        */
    {   PCI_VDEVICE(INTEL, 0x27c1), board_ahci  },                      /* ICH7                         */
    {   PCI_VDEVICE(INTEL, 0x27c5), board_ahci  },                      /* ICH7M                        */
    {   PCI_VDEVICE(INTEL, 0x27c3), board_ahci  },                      /* ICH7R                        */
    {   PCI_VDEVICE(AL,    0x5288), board_ahci_ign_iferr  },            /* ULi M5288                    */
    {   PCI_VDEVICE(INTEL, 0x2681), board_ahci  },                      /* ESB2                         */
    {   PCI_VDEVICE(INTEL, 0x2682), board_ahci  },                      /* ESB2                         */
    {   PCI_VDEVICE(INTEL, 0x2683), board_ahci  },                      /* ESB2                         */
    {   PCI_VDEVICE(INTEL, 0x27c6), board_ahci  },                      /* ICH7-M DH                    */
    {   PCI_VDEVICE(INTEL, 0x2821), board_ahci  },                      /* ICH8                         */
    {   PCI_VDEVICE(INTEL, 0x2822), board_ahci_nosntf  },               /* ICH8                         */
    {   PCI_VDEVICE(INTEL, 0x2824), board_ahci  },                      /* ICH8                         */
    {   PCI_VDEVICE(INTEL, 0x2829), board_ahci  },                      /* ICH8M                        */
    {   PCI_VDEVICE(INTEL, 0x282a), board_ahci  },                      /* ICH8M                        */
    {   PCI_VDEVICE(INTEL, 0x2922), board_ahci  },                      /* ICH9                         */
    {   PCI_VDEVICE(INTEL, 0x2923), board_ahci  },                      /* ICH9                         */
    {   PCI_VDEVICE(INTEL, 0x2924), board_ahci  },                      /* ICH9                         */
    {   PCI_VDEVICE(INTEL, 0x2925), board_ahci  },                      /* ICH9                         */
    {   PCI_VDEVICE(INTEL, 0x2927), board_ahci  },                      /* ICH9                         */
    {   PCI_VDEVICE(INTEL, 0x2929), board_ahci  },                      /* ICH9M                        */
    {   PCI_VDEVICE(INTEL, 0x292a), board_ahci  },                      /* ICH9M                        */
    {   PCI_VDEVICE(INTEL, 0x292b), board_ahci  },                      /* ICH9M                        */
    {   PCI_VDEVICE(INTEL, 0x292c), board_ahci  },                      /* ICH9M                        */
    {   PCI_VDEVICE(INTEL, 0x292f), board_ahci  },                      /* ICH9M                        */
    {   PCI_VDEVICE(INTEL, 0x294d), board_ahci  },                      /* ICH9                         */
    {   PCI_VDEVICE(INTEL, 0x294e), board_ahci  },                      /* ICH9M                        */
    {   PCI_VDEVICE(INTEL, 0x502a), board_ahci  },                      /* Tolapai                      */
    {   PCI_VDEVICE(INTEL, 0x502b), board_ahci  },                      /* Tolapai                      */
    {   PCI_VDEVICE(INTEL, 0x3a05), board_ahci  },                      /* ICH10                        */
    {   PCI_VDEVICE(INTEL, 0x3a22), board_ahci  },                      /* ICH10                        */
    {   PCI_VDEVICE(INTEL, 0x3a25), board_ahci  },                      /* ICH10                        */
    {   PCI_VDEVICE(INTEL, 0x3b22), board_ahci  },                      /* PCH AHCI                     */
    {   PCI_VDEVICE(INTEL, 0x3b23), board_ahci  },                      /* PCH AHCI                     */
    {   PCI_VDEVICE(INTEL, 0x3b24), board_ahci  },                      /* PCH RAID                     */
    {   PCI_VDEVICE(INTEL, 0x3b25), board_ahci  },                      /* PCH RAID                     */
    {   PCI_VDEVICE(INTEL, 0x3b29), board_ahci  },                      /* PCH AHCI                     */
    {   PCI_VDEVICE(INTEL, 0x3b2b), board_ahci  },                      /* PCH RAID                     */
    {   PCI_VDEVICE(INTEL, 0x3b2c), board_ahci  },                      /* PCH RAID                     */
    {   PCI_VDEVICE(INTEL, 0x3b2f), board_ahci  },                      /* PCH AHCI                     */
    {   PCI_VDEVICE(INTEL, 0x1c02), board_ahci  },                      /* CPT AHCI                     */
    {   PCI_VDEVICE(INTEL, 0x1c03), board_ahci  },                      /* CPT AHCI                     */
    {   PCI_VDEVICE(INTEL, 0x1c04), board_ahci  },                      /* CPT RAID                     */
    {   PCI_VDEVICE(INTEL, 0x1c05), board_ahci  },                      /* CPT RAID                     */
    {   PCI_VDEVICE(INTEL, 0x1c06), board_ahci  },                      /* CPT RAID                     */
    {   PCI_VDEVICE(INTEL, 0x1c07), board_ahci  },                      /* CPT RAID                     */
    {   PCI_VDEVICE(INTEL, 0x1d02), board_ahci  },                      /* PBG AHCI                     */
    {   PCI_VDEVICE(INTEL, 0x1d04), board_ahci  },                      /* PBG RAID                     */
    {   PCI_VDEVICE(INTEL, 0x1d06), board_ahci  },                      /* PBG RAID                     */
    {   PCI_VDEVICE(INTEL, 0x2826), board_ahci  },                      /* PBG RAID                     */
    {   PCI_VDEVICE(INTEL, 0x2323), board_ahci  },                      /* DH89xxCC AHCI                */
    {   PCI_VDEVICE(INTEL, 0x1e02), board_ahci  },                      /* Panther Point AHCI           */
    {   PCI_VDEVICE(INTEL, 0x1e03), board_ahci  },                      /* Panther Point AHCI           */
    {   PCI_VDEVICE(INTEL, 0x1e04), board_ahci  },                      /* Panther Point RAID           */
    {   PCI_VDEVICE(INTEL, 0x1e05), board_ahci  },                      /* Panther Point RAID           */
    {   PCI_VDEVICE(INTEL, 0x1e06), board_ahci  },                      /* Panther Point RAID           */
    {   PCI_VDEVICE(INTEL, 0x1e07), board_ahci  },                      /* Panther Point RAID           */
    {   PCI_VDEVICE(INTEL, 0x1e0e), board_ahci  },                      /* Panther Point RAID           */
    {   PCI_VDEVICE(INTEL, 0x8c02), board_ahci  },                      /* Lynx Point AHCI              */
    {   PCI_VDEVICE(INTEL, 0x8c03), board_ahci  },                      /* Lynx Point AHCI              */
    {   PCI_VDEVICE(INTEL, 0x8c04), board_ahci  },                      /* Lynx Point RAID              */
    {   PCI_VDEVICE(INTEL, 0x8c05), board_ahci  },                      /* Lynx Point RAID              */
    {   PCI_VDEVICE(INTEL, 0x8c06), board_ahci  },                      /* Lynx Point RAID              */
    {   PCI_VDEVICE(INTEL, 0x8c07), board_ahci  },                      /* Lynx Point RAID              */
    {   PCI_VDEVICE(INTEL, 0x8c0e), board_ahci  },                      /* Lynx Point RAID              */
    {   PCI_VDEVICE(INTEL, 0x8c0f), board_ahci  },                      /* Lynx Point RAID              */
    {   PCI_VDEVICE(INTEL, 0x9c02), board_ahci  },                      /* Lynx Point-LP AHCI           */
    {   PCI_VDEVICE(INTEL, 0x9c03), board_ahci  },                      /* Lynx Point-LP AHCI           */
    {   PCI_VDEVICE(INTEL, 0x9c04), board_ahci  },                      /* Lynx Point-LP RAID           */
    {   PCI_VDEVICE(INTEL, 0x9c05), board_ahci  },                      /* Lynx Point-LP RAID           */
    {   PCI_VDEVICE(INTEL, 0x9c06), board_ahci  },                      /* Lynx Point-LP RAID           */
    {   PCI_VDEVICE(INTEL, 0x9c07), board_ahci  },                      /* Lynx Point-LP RAID           */
    {   PCI_VDEVICE(INTEL, 0x9c0e), board_ahci  },                      /* Lynx Point-LP RAID           */
    {   PCI_VDEVICE(INTEL, 0x9c0f), board_ahci  },                      /* Lynx Point-LP RAID           */
    {   PCI_VDEVICE(INTEL, 0x1f22), board_ahci  },                      /* Avoton AHCI                  */
    {   PCI_VDEVICE(INTEL, 0x1f23), board_ahci  },                      /* Avoton AHCI                  */
    {   PCI_VDEVICE(INTEL, 0x1f24), board_ahci  },                      /* Avoton RAID                  */
    {   PCI_VDEVICE(INTEL, 0x1f25), board_ahci  },                      /* Avoton RAID                  */
    {   PCI_VDEVICE(INTEL, 0x1f26), board_ahci  },                      /* Avoton RAID                  */
    {   PCI_VDEVICE(INTEL, 0x1f27), board_ahci  },                      /* Avoton RAID                  */
    {   PCI_VDEVICE(INTEL, 0x1f2e), board_ahci  },                      /* Avoton RAID                  */
    {   PCI_VDEVICE(INTEL, 0x1f2f), board_ahci  },                      /* Avoton RAID                  */
    {   PCI_VDEVICE(INTEL, 0x1f32), board_ahci_avn  },                  /* Avoton AHCI                  */
    {   PCI_VDEVICE(INTEL, 0x1f33), board_ahci_avn  },                  /* Avoton AHCI                  */
    {   PCI_VDEVICE(INTEL, 0x1f34), board_ahci_avn  },                  /* Avoton RAID                  */
    {   PCI_VDEVICE(INTEL, 0x1f35), board_ahci_avn  },                  /* Avoton RAID                  */
    {   PCI_VDEVICE(INTEL, 0x1f36), board_ahci_avn  },                  /* Avoton RAID                  */
    {   PCI_VDEVICE(INTEL, 0x1f37), board_ahci_avn  },                  /* Avoton RAID                  */
    {   PCI_VDEVICE(INTEL, 0x1f3e), board_ahci_avn  },                  /* Avoton RAID                  */
    {   PCI_VDEVICE(INTEL, 0x1f3f), board_ahci_avn  },                  /* Avoton RAID                  */
    {   PCI_VDEVICE(INTEL, 0x2823), board_ahci  },                      /* Wellsburg RAID               */
    {   PCI_VDEVICE(INTEL, 0x2827), board_ahci  },                      /* Wellsburg RAID               */
    {   PCI_VDEVICE(INTEL, 0x8d02), board_ahci  },                      /* Wellsburg AHCI               */
    {   PCI_VDEVICE(INTEL, 0x8d04), board_ahci  },                      /* Wellsburg RAID               */
    {   PCI_VDEVICE(INTEL, 0x8d06), board_ahci  },                      /* Wellsburg RAID               */
    {   PCI_VDEVICE(INTEL, 0x8d0e), board_ahci  },                      /* Wellsburg RAID               */
    {   PCI_VDEVICE(INTEL, 0x8d62), board_ahci  },                      /* Wellsburg AHCI               */
    {   PCI_VDEVICE(INTEL, 0x8d64), board_ahci  },                      /* Wellsburg RAID               */
    {   PCI_VDEVICE(INTEL, 0x8d66), board_ahci  },                      /* Wellsburg RAID               */
    {   PCI_VDEVICE(INTEL, 0x8d6e), board_ahci  },                      /* Wellsburg RAID               */
    {   PCI_VDEVICE(INTEL, 0x23a3), board_ahci  },                      /* Coleto Creek AHCI            */
    {   PCI_VDEVICE(INTEL, 0x9c83), board_ahci  },                      /* Wildcat Point-LP AHCI        */
    {   PCI_VDEVICE(INTEL, 0x9c85), board_ahci  },                      /* Wildcat Point-LP RAID        */
    {   PCI_VDEVICE(INTEL, 0x9c87), board_ahci  },                      /* Wildcat Point-LP RAID        */
    {   PCI_VDEVICE(INTEL, 0x9c8f), board_ahci  },                      /* Wildcat Point-LP RAID        */
    {   PCI_VDEVICE(INTEL, 0x8c82), board_ahci  },                      /* 9 Series AHCI                */
    {   PCI_VDEVICE(INTEL, 0x8c83), board_ahci  },                      /* 9 Series AHCI                */
    {   PCI_VDEVICE(INTEL, 0x8c84), board_ahci  },                      /* 9 Series RAID                */
    {   PCI_VDEVICE(INTEL, 0x8c85), board_ahci  },                      /* 9 Series RAID                */
    {   PCI_VDEVICE(INTEL, 0x8c86), board_ahci  },                      /* 9 Series RAID                */
    {   PCI_VDEVICE(INTEL, 0x8c87), board_ahci  },                      /* 9 Series RAID                */
    {   PCI_VDEVICE(INTEL, 0x8c8e), board_ahci  },                      /* 9 Series RAID                */
    {   PCI_VDEVICE(INTEL, 0x8c8f), board_ahci  },                      /* 9 Series RAID                */
    {   PCI_VDEVICE(INTEL, 0x9d03), board_ahci  },                      /* Sunrise Point-LP AHCI        */
    {   PCI_VDEVICE(INTEL, 0x9d05), board_ahci  },                      /* Sunrise Point-LP RAID        */
    {   PCI_VDEVICE(INTEL, 0x9d07), board_ahci  },                      /* Sunrise Point-LP RAID        */
    {   PCI_VDEVICE(INTEL, 0xa102), board_ahci  },                      /* Sunrise Point-H AHCI         */
    {   PCI_VDEVICE(INTEL, 0xa103), board_ahci  },                      /* Sunrise Point-H AHCI         */
    {   PCI_VDEVICE(INTEL, 0xa105), board_ahci  },                      /* Sunrise Point-H RAID         */
    {   PCI_VDEVICE(INTEL, 0xa106), board_ahci  },                      /* Sunrise Point-H RAID         */
    {   PCI_VDEVICE(INTEL, 0xa107), board_ahci  },                      /* Sunrise Point-H RAID         */
    {   PCI_VDEVICE(INTEL, 0xa10f), board_ahci  },                      /* Sunrise Point-H RAID         */
    {   PCI_VDEVICE(INTEL, 0x2822), board_ahci  },                      /* Lewisburg RAID               */
    {   PCI_VDEVICE(INTEL, 0x2826), board_ahci  },                      /* Lewisburg RAID               */
    {   PCI_VDEVICE(INTEL, 0xa182), board_ahci  },                      /* Lewisburg AHCI               */
    {   PCI_VDEVICE(INTEL, 0xa184), board_ahci  },                      /* Lewisburg RAID               */
    {   PCI_VDEVICE(INTEL, 0xa186), board_ahci  },                      /* Lewisburg RAID               */
    {   PCI_VDEVICE(INTEL, 0xa18e), board_ahci  },                      /* Lewisburg RAID               */
    {   PCI_VDEVICE(INTEL, 0xa202), board_ahci  },                      /* Lewisburg AHCI               */
    {   PCI_VDEVICE(INTEL, 0xa204), board_ahci  },                      /* Lewisburg RAID               */
    {   PCI_VDEVICE(INTEL, 0xa206), board_ahci  },                      /* Lewisburg RAID               */
    {   PCI_VDEVICE(INTEL, 0xa20e), board_ahci  },                      /* Lewisburg RAID               */

    /*
     * JMicron 360/1/3/5/6, match class to avoid IDE function 
     */
    {   PCI_VENDOR_ID_JMICRON, PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID,
        PCI_CLASS_STORAGE_SATA_AHCI, 0xffffff, board_ahci_ign_iferr },
    /* 
     * JMicron 362B and 362C have an AHCI function with IDE class code 
     */
    {   PCI_VDEVICE(JMICRON, 0x2362), board_ahci_ign_iferr },
    {   PCI_VDEVICE(JMICRON, 0x236f), board_ahci_ign_iferr },
    /* 
     * May need to update quirk_jmicron_async_suspend() for additions 
     */

    /*
     * ATI 
     */
    {   PCI_VDEVICE(ATI, 0x4380), board_ahci_sb600  },                  /* ATI SB600                    */
    {   PCI_VDEVICE(ATI, 0x4390), board_ahci_sb700  },                  /* ATI SB700/800                */
    {   PCI_VDEVICE(ATI, 0x4391), board_ahci_sb700  },                  /* ATI SB700/800                */
    {   PCI_VDEVICE(ATI, 0x4392), board_ahci_sb700  },                  /* ATI SB700/800                */
    {   PCI_VDEVICE(ATI, 0x4393), board_ahci_sb700  },                  /* ATI SB700/800                */
    {   PCI_VDEVICE(ATI, 0x4394), board_ahci_sb700  },                  /* ATI SB700/800                */
    {   PCI_VDEVICE(ATI, 0x4395), board_ahci_sb700  },                  /* ATI SB700/800                */

    /* 
     * AMD 
     */
    {   PCI_VDEVICE(AMD, 0x7800), board_ahci  },                        /* AMD Hudson-2                 */
    {   PCI_VDEVICE(AMD, 0x7900), board_ahci  },                        /* AMD CZ                       */
    
    /* 
     * AMD is using RAID class only for ahci controllers 
     */
    {   PCI_VENDOR_ID_AMD, PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID,
        PCI_CLASS_STORAGE_RAID << 8, 0xffffff, board_ahci  },

    /* 
     * VIA 
     */
    {   PCI_VDEVICE(VIA, 0x3349), board_ahci_vt8251  },                 /* VIA VT8251                   */
    {   PCI_VDEVICE(VIA, 0x6287), board_ahci_vt8251  },                 /* VIA VT8251                   */

    /*
     * NVIDIA 
     */
    {   PCI_VDEVICE(NVIDIA, 0x044c), board_ahci_mcp65  },               /* MCP65                        */
    {   PCI_VDEVICE(NVIDIA, 0x044d), board_ahci_mcp65  },               /* MCP65                        */
    {   PCI_VDEVICE(NVIDIA, 0x044e), board_ahci_mcp65  },               /* MCP65                        */
    {   PCI_VDEVICE(NVIDIA, 0x044f), board_ahci_mcp65  },               /* MCP65                        */
    {   PCI_VDEVICE(NVIDIA, 0x045c), board_ahci_mcp65  },               /* MCP65                        */
    {   PCI_VDEVICE(NVIDIA, 0x045d), board_ahci_mcp65  },               /* MCP65                        */
    {   PCI_VDEVICE(NVIDIA, 0x045e), board_ahci_mcp65  },               /* MCP65                        */
    {   PCI_VDEVICE(NVIDIA, 0x045f), board_ahci_mcp65  },               /* MCP65                        */
    {   PCI_VDEVICE(NVIDIA, 0x0550), board_ahci_mcp67  },               /* MCP67                        */
    {   PCI_VDEVICE(NVIDIA, 0x0551), board_ahci_mcp67  },               /* MCP67                        */
    {   PCI_VDEVICE(NVIDIA, 0x0552), board_ahci_mcp67  },               /* MCP67                        */
    {   PCI_VDEVICE(NVIDIA, 0x0553), board_ahci_mcp67  },               /* MCP67                        */
    {   PCI_VDEVICE(NVIDIA, 0x0554), board_ahci_mcp67  },               /* MCP67                        */
    {   PCI_VDEVICE(NVIDIA, 0x0555), board_ahci_mcp67  },               /* MCP67                        */
    {   PCI_VDEVICE(NVIDIA, 0x0556), board_ahci_mcp67  },               /* MCP67                        */
    {   PCI_VDEVICE(NVIDIA, 0x0557), board_ahci_mcp67  },               /* MCP67                        */
    {   PCI_VDEVICE(NVIDIA, 0x0558), board_ahci_mcp67  },               /* MCP67                        */
    {   PCI_VDEVICE(NVIDIA, 0x0559), board_ahci_mcp67  },               /* MCP67                        */
    {   PCI_VDEVICE(NVIDIA, 0x055a), board_ahci_mcp67  },               /* MCP67                        */
    {   PCI_VDEVICE(NVIDIA, 0x055b), board_ahci_mcp67  },               /* MCP67                        */
    {   PCI_VDEVICE(NVIDIA, 0x0580), board_ahci_mcp_linux  },           /* Linux ID                     */
    {   PCI_VDEVICE(NVIDIA, 0x0581), board_ahci_mcp_linux  },           /* Linux ID                     */
    {   PCI_VDEVICE(NVIDIA, 0x0582), board_ahci_mcp_linux  },           /* Linux ID                     */
    {   PCI_VDEVICE(NVIDIA, 0x0583), board_ahci_mcp_linux  },           /* Linux ID                     */
    {   PCI_VDEVICE(NVIDIA, 0x0584), board_ahci_mcp_linux  },           /* Linux ID                     */
    {   PCI_VDEVICE(NVIDIA, 0x0585), board_ahci_mcp_linux  },           /* Linux ID                     */
    {   PCI_VDEVICE(NVIDIA, 0x0586), board_ahci_mcp_linux  },           /* Linux ID                     */
    {   PCI_VDEVICE(NVIDIA, 0x0587), board_ahci_mcp_linux  },           /* Linux ID                     */
    {   PCI_VDEVICE(NVIDIA, 0x0588), board_ahci_mcp_linux  },           /* Linux ID                     */
    {   PCI_VDEVICE(NVIDIA, 0x0589), board_ahci_mcp_linux  },           /* Linux ID                     */
    {   PCI_VDEVICE(NVIDIA, 0x058a), board_ahci_mcp_linux  },           /* Linux ID                     */
    {   PCI_VDEVICE(NVIDIA, 0x058b), board_ahci_mcp_linux  },           /* Linux ID                     */
    {   PCI_VDEVICE(NVIDIA, 0x058c), board_ahci_mcp_linux  },           /* Linux ID                     */
    {   PCI_VDEVICE(NVIDIA, 0x058d), board_ahci_mcp_linux  },           /* Linux ID                     */
    {   PCI_VDEVICE(NVIDIA, 0x058e), board_ahci_mcp_linux  },           /* Linux ID                     */
    {   PCI_VDEVICE(NVIDIA, 0x058f), board_ahci_mcp_linux  },           /* Linux ID                     */
    {   PCI_VDEVICE(NVIDIA, 0x07f0), board_ahci_mcp73  },               /* MCP73                        */
    {   PCI_VDEVICE(NVIDIA, 0x07f1), board_ahci_mcp73  },               /* MCP73                        */
    {   PCI_VDEVICE(NVIDIA, 0x07f2), board_ahci_mcp73  },               /* MCP73                        */
    {   PCI_VDEVICE(NVIDIA, 0x07f3), board_ahci_mcp73  },               /* MCP73                        */
    {   PCI_VDEVICE(NVIDIA, 0x07f4), board_ahci_mcp73  },               /* MCP73                        */
    {   PCI_VDEVICE(NVIDIA, 0x07f5), board_ahci_mcp73  },               /* MCP73                        */
    {   PCI_VDEVICE(NVIDIA, 0x07f6), board_ahci_mcp73  },               /* MCP73                        */
    {   PCI_VDEVICE(NVIDIA, 0x07f7), board_ahci_mcp73  },               /* MCP73                        */
    {   PCI_VDEVICE(NVIDIA, 0x07f8), board_ahci_mcp73  },               /* MCP73                        */
    {   PCI_VDEVICE(NVIDIA, 0x07f9), board_ahci_mcp73  },               /* MCP73                        */
    {   PCI_VDEVICE(NVIDIA, 0x07fa), board_ahci_mcp73  },               /* MCP73                        */
    {   PCI_VDEVICE(NVIDIA, 0x07fb), board_ahci_mcp73  },               /* MCP73                        */
    {   PCI_VDEVICE(NVIDIA, 0x0ad0), board_ahci_mcp77  },               /* MCP77                        */
    {   PCI_VDEVICE(NVIDIA, 0x0ad1), board_ahci_mcp77  },               /* MCP77                        */
    {   PCI_VDEVICE(NVIDIA, 0x0ad2), board_ahci_mcp77  },               /* MCP77                        */
    {   PCI_VDEVICE(NVIDIA, 0x0ad3), board_ahci_mcp77  },               /* MCP77                        */
    {   PCI_VDEVICE(NVIDIA, 0x0ad4), board_ahci_mcp77  },               /* MCP77                        */
    {   PCI_VDEVICE(NVIDIA, 0x0ad5), board_ahci_mcp77  },               /* MCP77                        */
    {   PCI_VDEVICE(NVIDIA, 0x0ad6), board_ahci_mcp77  },               /* MCP77                        */
    {   PCI_VDEVICE(NVIDIA, 0x0ad7), board_ahci_mcp77  },               /* MCP77                        */
    {   PCI_VDEVICE(NVIDIA, 0x0ad8), board_ahci_mcp77  },               /* MCP77                        */
    {   PCI_VDEVICE(NVIDIA, 0x0ad9), board_ahci_mcp77  },               /* MCP77                        */
    {   PCI_VDEVICE(NVIDIA, 0x0ada), board_ahci_mcp77  },               /* MCP77                        */
    {   PCI_VDEVICE(NVIDIA, 0x0adb), board_ahci_mcp77  },               /* MCP77                        */
    {   PCI_VDEVICE(NVIDIA, 0x0ab4), board_ahci_mcp79  },               /* MCP79                        */
    {   PCI_VDEVICE(NVIDIA, 0x0ab5), board_ahci_mcp79  },               /* MCP79                        */
    {   PCI_VDEVICE(NVIDIA, 0x0ab6), board_ahci_mcp79  },               /* MCP79                        */
    {   PCI_VDEVICE(NVIDIA, 0x0ab7), board_ahci_mcp79  },               /* MCP79                        */
    {   PCI_VDEVICE(NVIDIA, 0x0ab8), board_ahci_mcp79  },               /* MCP79                        */
    {   PCI_VDEVICE(NVIDIA, 0x0ab9), board_ahci_mcp79  },               /* MCP79                        */
    {   PCI_VDEVICE(NVIDIA, 0x0aba), board_ahci_mcp79  },               /* MCP79                        */
    {   PCI_VDEVICE(NVIDIA, 0x0abb), board_ahci_mcp79  },               /* MCP79                        */
    {   PCI_VDEVICE(NVIDIA, 0x0abc), board_ahci_mcp79  },               /* MCP79                        */
    {   PCI_VDEVICE(NVIDIA, 0x0abd), board_ahci_mcp79  },               /* MCP79                        */
    {   PCI_VDEVICE(NVIDIA, 0x0abe), board_ahci_mcp79  },               /* MCP79                        */
    {   PCI_VDEVICE(NVIDIA, 0x0abf), board_ahci_mcp79  },               /* MCP79                        */
    {   PCI_VDEVICE(NVIDIA, 0x0d84), board_ahci_mcp89  },               /* MCP89                        */
    {   PCI_VDEVICE(NVIDIA, 0x0d85), board_ahci_mcp89  },               /* MCP89                        */
    {   PCI_VDEVICE(NVIDIA, 0x0d86), board_ahci_mcp89  },               /* MCP89                        */
    {   PCI_VDEVICE(NVIDIA, 0x0d87), board_ahci_mcp89  },               /* MCP89                        */
    {   PCI_VDEVICE(NVIDIA, 0x0d88), board_ahci_mcp89  },               /* MCP89                        */
    {   PCI_VDEVICE(NVIDIA, 0x0d89), board_ahci_mcp89  },               /* MCP89                        */
    {   PCI_VDEVICE(NVIDIA, 0x0d8a), board_ahci_mcp89  },               /* MCP89                        */
    {   PCI_VDEVICE(NVIDIA, 0x0d8b), board_ahci_mcp89  },               /* MCP89                        */
    {   PCI_VDEVICE(NVIDIA, 0x0d8c), board_ahci_mcp89  },               /* MCP89                        */
    {   PCI_VDEVICE(NVIDIA, 0x0d8d), board_ahci_mcp89  },               /* MCP89                        */
    {   PCI_VDEVICE(NVIDIA, 0x0d8e), board_ahci_mcp89  },               /* MCP89                        */
    {   PCI_VDEVICE(NVIDIA, 0x0d8f), board_ahci_mcp89  },               /* MCP89                        */

    /* 
     * SiS 
     */
    {   PCI_VDEVICE(SI, 0x1184), board_ahci  },                         /* SiS 966                      */
    {   PCI_VDEVICE(SI, 0x1185), board_ahci  },                         /* SiS 968                      */
    {   PCI_VDEVICE(SI, 0x0186), board_ahci  },                         /* SiS 968                      */

    /* 
     * ST Microelectronics
     */
    {   PCI_VDEVICE(STMICRO, 0xCC06), board_ahci  },                    /* ST ConneXt                   */

    /* 
     * Marvell
     */
    {   PCI_VDEVICE(MARVELL, 0x6145), board_ahci_mv  },                 /* 6145                         */
    {   PCI_VDEVICE(MARVELL, 0x6121), board_ahci_mv  },                 /* 6121                         */

    {   PCI_DEVICE(PCI_VENDOR_ID_MARVELL_EXT, 0x9123),
        PCI_CLASS_STORAGE_SATA_AHCI, 0xffffff, board_ahci_yes_fbs  },   /* 88se9128                     */
    {   PCI_DEVICE(PCI_VENDOR_ID_MARVELL_EXT, 0x9125),
        PCI_CLASS_STORAGE_SATA_AHCI, 0xffffff, board_ahci_yes_fbs  },   /* 88se9125                     */
    {   PCI_DEVICE_SUB(PCI_VENDOR_ID_MARVELL_EXT, 0x9178, PCI_VENDOR_ID_MARVELL_EXT, 0x9170),
        PCI_CLASS_STORAGE_SATA_AHCI, 0xffffff, board_ahci_yes_fbs  },   /* 88se9170                     */
    {   PCI_DEVICE(PCI_VENDOR_ID_MARVELL_EXT, 0x917a),
        PCI_CLASS_STORAGE_SATA_AHCI, 0xffffff, board_ahci_yes_fbs  },   /* 88se9172                     */
    {   PCI_DEVICE(PCI_VENDOR_ID_MARVELL_EXT, 0x9172),
        PCI_CLASS_STORAGE_SATA_AHCI, 0xffffff, board_ahci_yes_fbs  },   /* 88se9182                     */
    {   PCI_DEVICE(PCI_VENDOR_ID_MARVELL_EXT, 0x9182),
        PCI_CLASS_STORAGE_SATA_AHCI, 0xffffff, board_ahci_yes_fbs  },   /* 88se9172                     */
    {   PCI_DEVICE(PCI_VENDOR_ID_MARVELL_EXT, 0x9192),
        PCI_CLASS_STORAGE_SATA_AHCI, 0xffffff, board_ahci_yes_fbs  },   /* 88se9172 on some Gigabyte    */
    {   PCI_DEVICE(PCI_VENDOR_ID_MARVELL_EXT, 0x91a0),
        PCI_CLASS_STORAGE_SATA_AHCI, 0xffffff, board_ahci_yes_fbs  },
    {   PCI_DEVICE(PCI_VENDOR_ID_MARVELL_EXT, 0x91a2),                  /* 88se91a2                     */
        PCI_CLASS_STORAGE_SATA_AHCI, 0xffffff, board_ahci_yes_fbs  },
    {   PCI_DEVICE(PCI_VENDOR_ID_MARVELL_EXT, 0x91a3),
        PCI_CLASS_STORAGE_SATA_AHCI, 0xffffff, board_ahci_yes_fbs  },
    {   PCI_DEVICE(PCI_VENDOR_ID_MARVELL_EXT, 0x9230),
        PCI_CLASS_STORAGE_SATA_AHCI, 0xffffff, board_ahci_yes_fbs  },
    {   PCI_DEVICE(PCI_VENDOR_ID_TTI, 0x0642),
        PCI_CLASS_STORAGE_SATA_AHCI, 0xffffff, board_ahci_yes_fbs  },
    {   PCI_DEVICE(PCI_VENDOR_ID_MARVELL_EXT, 0x9215),
        PCI_CLASS_STORAGE_SATA_AHCI, 0xffffff, board_ahci_yes_fbs  },

    /* 
     * Promise
     */
    {   PCI_VDEVICE(PROMISE, 0x3f20), board_ahci  },                    /* PDC42819                     */
    {   PCI_VDEVICE(PROMISE, 0x3781), board_ahci  },                    /* FastTrak TX8660 ahci-mode    */

    /* 
     * Asmedia
     */
    {   PCI_VDEVICE(ASMEDIA, 0x0601), board_ahci  },                    /* ASM1060                      */
    {   PCI_VDEVICE(ASMEDIA, 0x0602), board_ahci  },                    /* ASM1060                      */
    {   PCI_VDEVICE(ASMEDIA, 0x0611), board_ahci  },                    /* ASM1061                      */
    {   PCI_VDEVICE(ASMEDIA, 0x0612), board_ahci  },                    /* ASM1062                      */

    /*
     * Samsung SSDs found on some macbooks.  NCQ times out if MSI is
     * enabled.  https://bugzilla.kernel.org/show_bug.cgi?id=60731
     */
    {   PCI_VDEVICE(SAMSUNG, 0x1600), board_ahci_nomsi  },
    {   PCI_VDEVICE(SAMSUNG, 0xa800), board_ahci_nomsi  },

    /*
     * Enmotus
     */
    {   PCI_DEVICE(0x1c44, 0x8000), board_ahci  },

    /*
     * Generic, PCI class code for AHCI
     */
    {   PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID,
        PCI_CLASS_STORAGE_SATA_AHCI, 0xffffff, board_ahci  },

    { }                                                                 /* terminate list               */
};
/*********************************************************************************************************
  AHCI 控制器计数
*********************************************************************************************************/
static UINT     pciStorageSataCtrlNum = 0;
/*********************************************************************************************************
** 函数名称: pciStorageSataHeaderQuirkAmdIdeMode
** 功能描述: 读到设备头后, 将 AMD 桥 IDE 模式切换为 AHCI 模式
** 输　入  : hDevHandle         PCI 设备控制块句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  pciStorageSataHeaderQuirkAmdIdeMode (PCI_DEV_HANDLE  hPciDevHandle)
{
    UINT8       ucData;

    API_PciDevConfigReadByte(hPciDevHandle, PCI_CLASS_DEVICE, &ucData);
    if (ucData == 0x01) {
        API_PciDevConfigReadByte(hPciDevHandle, 0x40, &ucData);
        API_PciDevConfigWriteByte(hPciDevHandle, 0x40, ucData | 0x01);
        API_PciDevConfigWriteByte(hPciDevHandle, PCI_CLASS_PROG, 0x01);
        API_PciDevConfigWriteByte(hPciDevHandle, PCI_CLASS_DEVICE, 0x06);
        API_PciDevConfigWriteByte(hPciDevHandle, 0x40, ucData);
    }
}
/*********************************************************************************************************
** 函数名称: pciStorageSataHeaderQuirk
** 功能描述: 读到设备头后的特殊处理
** 输　入  : hDevHandle         PCI 设备控制块句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  pciStorageSataHeaderQuirk (PCI_DEV_HANDLE  hPciDevHandle)
{
    UINT16      usVendorId;
    UINT16      usDeviceId;

    if (!hPciDevHandle) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    usVendorId = PCI_DEV_VENDOR_ID(hPciDevHandle);
    usDeviceId = PCI_DEV_DEVICE_ID(hPciDevHandle);

    switch (usVendorId) {

    case PCI_VENDOR_ID_ATI:
        if ((usDeviceId == PCI_DEVICE_ID_ATI_IXP600_SATA) ||
            (usDeviceId == PCI_DEVICE_ID_ATI_IXP700_SATA) ||
            (usDeviceId == PCI_DEVICE_ID_AMD_HUDSON2_SATA_IDE)) {
            pciStorageSataHeaderQuirkAmdIdeMode(hPciDevHandle);
        }
        break;

    default:
        break;
    }

    return (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageSataDevIdTblGet
** 功能描述: 获取设备列表
** 输　入  : phPciDevId     设备 ID 列表句柄缓冲区
**           puiSzie        设备列表大小
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  pciStorageSataDevIdTblGet (PCI_DEV_ID_HANDLE *phPciDevId, UINT32 *puiSzie)
{
    if ((!phPciDevId) ||
        (!puiSzie)) {
        return  (PX_ERROR);
    }

    *phPciDevId = (PCI_DEV_ID_HANDLE)pciStorageSataIdTbl;
    *puiSzie    = sizeof(pciStorageSataIdTbl) / sizeof(PCI_DEV_ID_CB);

    return (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageSataDevRemove
** 功能描述: 移除 AHCI 设备
** 输　入  : hDevHandle         PCI 设备控制块句柄
**           hIdTable           设备 ID 列表
** 输　出  : 控制器句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static VOID  pciStorageSataDevRemove (PCI_DEV_HANDLE hHandle)
{
}
/*********************************************************************************************************
** 函数名称: pciStorageSataDevProbe
** 功能描述: AHCI 驱动探测到设备
** 输　入  : hDevHandle         PCI 设备控制块句柄
**           hIdEntry           匹配成功的设备 ID 条目(来自于设备 ID 表)
** 输　出  : 控制器句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static INT  pciStorageSataDevProbe (PCI_DEV_HANDLE hPciDevHandle, const PCI_DEV_ID_HANDLE hIdEntry)
{
    INT                     iRet = PX_ERROR;
    AHCI_CTRL_HANDLE        hAhciCtrl = LW_NULL;
    UINT16                  usCap;

    if ((!hPciDevHandle) ||
        (!hIdEntry)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pciStorageSataHeaderQuirk(hPciDevHandle);                           /* 读到设备头后的特殊处理       */
                                                                        /* 确认设备类型                 */
    iRet = API_PciDevConfigRead(hPciDevHandle, PCI_CLASS_DEVICE, (UINT8 *)&usCap, 2);
    if ((iRet != ERROR_NONE) ||
        (usCap != PCI_CLASS_STORAGE_SATA)) {                            /* 设备类型错误                 */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    hPciDevHandle->PCIDEV_uiDevVersion = SATA_PCI_DRV_VER_NUM;
    hPciDevHandle->PCIDEV_uiUnitNumber = pciStorageSataCtrlNum;
    pciStorageSataCtrlNum++;
    
    hAhciCtrl = API_AhciCtrlCreate(SATA_PCI_DRV_NAME,
                                   hPciDevHandle->PCIDEV_uiUnitNumber,
                                   (PVOID)hPciDevHandle);
    if (!hAhciCtrl) {
        hPciDevHandle->PCIDEV_pvDevDriver = LW_NULL;
        pciStorageSataCtrlNum--;
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageSataCtrlOpt
** 功能描述: 驱动相关的选项操作
** 输　入  : hCtrl      控制器句柄
**           uiDrive    驱动器索引
**           iCmd       命令 (AHCI_OPT_CMD_xxxx)
**           lArg       参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  pciStorageSataCtrlOpt (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, INT  iCmd, LONG  lArg)
{
    if (!hCtrl) {
        return  (PX_ERROR);
    }

    switch (iCmd) {

    case AHCI_OPT_CMD_SECTOR_START_GET:
        if (lArg) {
            *(ULONG *)lArg = 0;
        }
        break;

    case AHCI_OPT_CMD_SECTOR_SIZE_GET:
        if (lArg) {
            *(ULONG *)lArg = AHCI_SECTOR_SIZE;
        }
        break;

    case AHCI_OPT_CMD_CACHE_BURST_RD_GET:
        if (lArg) {
            *(ULONG *)lArg = AHCI_CACHE_BURST_RD;
        }
        break;

    case AHCI_OPT_CMD_CACHE_BURST_WR_GET:
        if (lArg) {
            *(ULONG *)lArg = AHCI_CACHE_BURST_WR;
        }
        break;

    case AHCI_OPT_CMD_CACHE_SIZE_GET:
        if (lArg) {
            *(ULONG *)lArg = AHCI_CACHE_SIZE;
        }
        break;

    case AHCI_OPT_CMD_PROB_TIME_UNIT_GET:
        if (lArg) {
            *(ULONG *)lArg = AHCI_DRIVE_PROB_TIME_UNIT;
        }
        break;

    case AHCI_OPT_CMD_PROB_TIME_COUNT_GET:
        if (lArg) {
            *(ULONG *)lArg = AHCI_DRIVE_PROB_TIME_COUNT;
        }
        break;

    case AHCI_OPT_CMD_RSTON_INTER_TIME_GET:
        if (lArg) {
            *(ULONG *)lArg = AHCI_DRIVE_RSTON_INTER_TIME_UNIT;
        }
        break;

    case AHCI_OPT_CMD_RSTON_INTER_COUNT_GET:
        if (lArg) {
            *(ULONG *)lArg = AHCI_DRIVE_RSTON_INTER_TIME_COUNT;
        }
        break;

    case AHCI_OPT_CMD_RSTOFF_INTER_TIME_GET:
        if (lArg) {
            *(ULONG *)lArg = AHCI_DRIVE_RSTOFF_INTER_TIME_UNIT;
        }
        break;

    case AHCI_OPT_CMD_RSTOFF_INTER_COUNT_GET:
        if (lArg) {
            *(ULONG *)lArg = AHCI_DRIVE_RSTOFF_INTER_TIME_COUNT;
        }
        break;

    case AHCI_OPT_CMD_DC_MSG_COUNT_GET:
        if (lArg) {
            *(ULONG *)lArg = AHCI_DRIVE_DISKCACHE_MSG_COUNT;
        }
        break;

    case AHCI_OPT_CMD_DC_PARALLEL_EN_GET:
        if (lArg) {
            *(ULONG *)lArg = (AHCI_DRIVE_DISKCACHE_PARALLEL_EN);
        }
        break;

    case AHCI_OPT_CMD_CTRL_ENDIAN_TYPE_GET:
        if (lArg) {
            *(INT *)lArg = (AHCI_CTRL_ENDIAN_TYPE);
        }
        break;

    case AHCI_OPT_CMD_PORT_ENDIAN_TYPE_GET:
        if (lArg) {
            *(INT *)lArg = (AHCI_PORT_ENDIAN_TYPE);
        }
        break;

    case AHCI_OPT_CMD_PARAM_ENDIAN_TYPE_GET:
        if (lArg) {
            *(INT *)lArg = (AHCI_PARAM_ENDIAN_TYPE);
        }
        break;

    default:
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageSataVendorDriveInfoShow
** 功能描述: 驱动器设备厂商自定义信息
** 输　入  : hCtrl      控制器句柄
**           uiDrive    驱动器号
**           hParam     参数控制块句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static INT  pciStorageSataVendorDriveInfoShow (AHCI_CTRL_HANDLE  hCtrl, 
                                               UINT              uiDrive, 
                                               AHCI_PARAM_HANDLE hParam)
{
    return (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageSataVerdorDriveRegNameGet
** 功能描述: 获取驱动器设备厂商自定义端口寄存器名
** 输　入  : hDrive     驱动器句柄
**           uiOffset   端口寄存器偏移
** 输　出  : 寄存器名称缓冲区
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static PCHAR  pciStorageSataVendorDriveRegNameGet (AHCI_DRIVE_HANDLE  hDrive, UINT32  uiOffset)
{
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: pciStorageSataVendorDriveInit
** 功能描述: 厂商驱动器特殊处理
** 输　入  : hDrive      驱动器句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static INT  pciStorageSataVendorDriveInit (AHCI_DRIVE_HANDLE  hDrive)
{
    INT         iRet;
    UINT32      uiReg;

    AHCI_PORT_REG_MSG(hDrive, AHCI_PxCMD);
    uiReg  = AHCI_PORT_READ(hDrive, AHCI_PxCMD);
    uiReg &= ~(AHCI_PCMD_SUD);
    uiReg &= ~(AHCI_PCMD_POD);
    uiReg &= ~(AHCI_PCMD_ST);
    AHCI_PORT_WRITE(hDrive, AHCI_PxCMD, uiReg);
    AHCI_PORT_READ(hDrive, AHCI_PxCMD);
    AHCI_PORT_REG_MSG(hDrive, AHCI_PxCMD);

    iRet = API_AhciDriveRegWait(hDrive, AHCI_PxCMD, AHCI_PCMD_CR, LW_TRUE, AHCI_PCMD_CR, 20, 50, &uiReg);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    AHCI_PORT_REG_MSG(hDrive, AHCI_PxCMD);
    uiReg  = AHCI_PORT_READ(hDrive, AHCI_PxCMD);
    uiReg |= AHCI_PCMD_ST;
    AHCI_PORT_WRITE(hDrive, AHCI_PxCMD, uiReg);
    AHCI_PORT_READ(hDrive, AHCI_PxCMD);
    AHCI_PORT_REG_MSG(hDrive, AHCI_PxCMD);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageSataVendorCtrlInfoShow
** 功能描述: 厂商自定义控制器信息打印
** 输　入  : hCtrl      控制器句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static INT  pciStorageSataVendorCtrlInfoShow (AHCI_CTRL_HANDLE  hCtrl)
{
    return (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageSataVendorCtrlRegNameGet
** 功能描述: 获取厂商自定义控制器寄存器名
** 输　入  : hCtrl      控制器句柄
**           uiOffset   全局寄存器偏移
** 输　出  : 寄存器名称缓冲区
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static PCHAR  pciStorageSataVendorCtrlRegNameGet (AHCI_CTRL_HANDLE  hCtrl, UINT32  uiOffset)
{
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: pciStorageSataVendorCtrlTypeNameGet
** 功能描述: 获取控制器类型名称
** 输　入  : hCtrl      控制器句柄
** 输　出  : 控制器类型名称
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static PCHAR  pciStorageSataVendorCtrlTypeNameGet (AHCI_CTRL_HANDLE  hCtrl)
{
    PCHAR       pcType;
    UINT16      usCap = 0;
    PCI_DEV_HANDLE      hPciDev;

    hPciDev = (PCI_DEV_HANDLE)hCtrl->AHCICTRL_pvPciArg;                 /* 获取设备句柄                 */

    API_PciDevConfigReadWord(hPciDev, PCI_CLASS_DEVICE, &usCap);
    if (usCap == PCI_CLASS_STORAGE_IDE) {
        pcType = "IDE";
    } else if (usCap == PCI_CLASS_STORAGE_SATA) {
        pcType = "SATA";
    } else if (usCap == PCI_CLASS_STORAGE_RAID) {
        pcType = "RAID";
    } else {
        pcType = "Unknown";
    }

    return  (pcType);
}
/*********************************************************************************************************
** 函数名称: __ahciPciVendorCtrlIntEnable
** 功能描述: 使能控制器中断
** 输　入  : hCtrl      控制器句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static INT  pciStorageSataVendorCtrlIntEnable (AHCI_CTRL_HANDLE  hCtrl)
{
    PCI_DEV_HANDLE      hPciDev;

    hPciDev = (PCI_DEV_HANDLE)hCtrl->AHCICTRL_pvPciArg;                 /* 获取设备句柄                 */

    API_PciDevInterEnable(hPciDev, hCtrl->AHCICTRL_ulIrqVector, hCtrl->AHCICTRL_pfuncIrq, hCtrl);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ahciPciVendorCtrlIntConnect
** 功能描述: 链接控制器中断
** 输　入  : hCtrl      控制器句柄
**           pfuncIsr   中断服务函数
**           cpcName    中断服务名称
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static INT  pciStorageSataVendorCtrlIntConnect (AHCI_CTRL_HANDLE  hCtrl, 
                                                PINT_SVR_ROUTINE  pfuncIsr, 
                                                CPCHAR            cpcName)
{
    PCI_DEV_HANDLE      hPciDev;

    hPciDev = (PCI_DEV_HANDLE)hCtrl->AHCICTRL_pvPciArg;                 /* 获取设备句柄                 */

    API_PciDevInterConnect(hPciDev, hCtrl->AHCICTRL_ulIrqVector, pfuncIsr, hCtrl, cpcName);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageSataVendorCtrlInit
** 功能描述: 厂商控制器特殊处理
** 输　入  : hCtrl    控制器句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static INT  pciStorageSataVendorCtrlInit (AHCI_CTRL_HANDLE  hCtrl)
{
    UINT32          uiReg;

    AHCI_CTRL_REG_MSG(hCtrl, AHCI_GHC);
    uiReg = AHCI_CTRL_READ(hCtrl, AHCI_GHC);
    uiReg &= ~(AHCI_GHC_AE);
    AHCI_CTRL_WRITE(hCtrl, AHCI_GHC, uiReg);
    AHCI_CTRL_REG_MSG(hCtrl, AHCI_GHC);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageSataVendorCtrlReadyWork
** 功能描述: 控制器准备操作
** 输　入  : hCtrl    控制器句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static INT  pciStorageSataVendorCtrlReadyWork (AHCI_CTRL_HANDLE  hCtrl)
{
    INT                     iRet;
    PCI_DEV_HANDLE          hPciDev;
    UINT16                  usPciDevId;
    UINT16                  usCmd;
    UINT16                  usStatus;
    ULONG                   ulBaseAddr;
    PCI_RESOURCE_HANDLE     hResource;
    pci_resource_size_t     stStart;

    hPciDev = (PCI_DEV_HANDLE)hCtrl->AHCICTRL_pvPciArg;                 /* 获取设备句柄                 */

    API_PciDevConfigReadWord(hPciDev, PCI_DEVICE_ID, &usPciDevId);
    AHCI_LOG(AHCI_LOG_PRT, "ctrl name %s index %d unit %d for pci dev %d:%d.%d dev id 0x%04x.\r\n",
             hCtrl->AHCICTRL_cCtrlName, hCtrl->AHCICTRL_uiIndex, hCtrl->AHCICTRL_uiUnitIndex,
             hPciDev->PCIDEV_iDevBus,
             hPciDev->PCIDEV_iDevDevice,
             hPciDev->PCIDEV_iDevFunction, usPciDevId);
                                                                        /* BAR5                         */
    stStart = (pci_resource_size_t)PCI_DEV_BASE_START(hPciDev, PCI_BAR_INDEX_5);
                                                                        /* 查找对应资源信息             */
    hResource = API_PciDevStdResourceFind(hPciDev, PCI_IORESOURCE_MEM, stStart);
    if (!hResource) {
        AHCI_LOG(AHCI_LOG_ERR, "pci BAR index %d error.\r\n", PCI_BAR_INDEX_5);
        return  (PX_ERROR);
    }

    ulBaseAddr                = (ULONG)(PCI_RESOURCE_START(hResource));
    hCtrl->AHCICTRL_pvRegAddr = (PVOID)ulBaseAddr;
    hCtrl->AHCICTRL_stRegSize = (size_t)(PCI_RESOURCE_SIZE(hResource));
    hCtrl->AHCICTRL_pvRegAddr = API_PciDevIoRemap(hCtrl->AHCICTRL_pvRegAddr,
                                                  hCtrl->AHCICTRL_stRegSize);
    if (hCtrl->AHCICTRL_pvRegAddr == LW_NULL) {
        AHCI_LOG(AHCI_LOG_ERR, "pci mem resource ioremap failed addr 0x%llx 0x%llx.\r\n",
                 hCtrl->AHCICTRL_pvRegAddr,  hCtrl->AHCICTRL_stRegSize);
        return  (PX_ERROR);
    }
    AHCI_LOG(AHCI_LOG_PRT, "ahci reg addr 0x%llx szie %llx.\r\n",
             hCtrl->AHCICTRL_pvRegAddr, hCtrl->AHCICTRL_stRegSize);
    hPciDev->PCIDEV_pvDevDriver = (PVOID)hCtrl;

    iRet = API_PciDevMsiEnableSet(hPciDev, LW_TRUE);
    if (iRet != ERROR_NONE) {
        goto  __intx_handle;
    }
    
    iRet = API_PciDevMsiRangeEnable(hPciDev, 1, 1);
    if (iRet != ERROR_NONE) {
        goto  __intx_handle;
    
    } else {
        hCtrl->AHCICTRL_ulIrqVector = hPciDev->PCIDEV_ulDevIrqVector;
        goto __msi_handlel;
    }

__intx_handle:
    iRet = API_PciDevMsiEnableSet(hPciDev, LW_FALSE);
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "pci msi disable failed dev %d:%d.%d.\r\n",
                 hPciDev->PCIDEV_iDevBus, hPciDev->PCIDEV_iDevDevice, hPciDev->PCIDEV_iDevFunction);
    }
    hResource = API_PciDevResourceGet(hPciDev, PCI_IORESOURCE_IRQ, 0);
    hCtrl->AHCICTRL_ulIrqVector = (ULONG)PCI_RESOURCE_START(hResource);

__msi_handlel:
    API_PciDevConfigReadWord(hPciDev,  PCI_STATUS,  &usStatus);
    API_PciDevConfigWriteWord(hPciDev, PCI_STATUS,  usStatus);
    API_PciDevConfigReadWord(hPciDev,  PCI_COMMAND, &usCmd);
    API_PciDevConfigWriteWord(hPciDev, PCI_COMMAND, (usCmd | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER));

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageSataVendorPlatformInit
** 功能描述: 控制器平台相关初始化
** 输　入  : hCtrl     控制器句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static INT  pciStorageSataVendorPlatformInit (AHCI_CTRL_HANDLE  hCtrl)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageSataVendorDrvReadyWork
** 功能描述: 驱动注册前准备操作
** 输　入  : hDrv      驱动控制句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static INT  pciStorageSataVendorDrvReadyWork (AHCI_DRV_HANDLE  hDrv)
{
    INT                 iRet;
    PCI_DRV_CB          tPciDrv;
    PCI_DRV_HANDLE      hPciDrv = &tPciDrv;

    lib_bzero(hPciDrv, sizeof(PCI_DRV_CB));
    iRet = pciStorageSataDevIdTblGet(&hPciDrv->PCIDRV_hDrvIdTable, &hPciDrv->PCIDRV_uiDrvIdTableSize);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    lib_strlcpy(& hPciDrv->PCIDRV_cDrvName[0], SATA_PCI_DRV_NAME, PCI_DRV_NAME_MAX);
    hPciDrv->PCIDRV_pvPriv = (PVOID)hDrv;
    hPciDrv->PCIDRV_hDrvErrHandler = LW_NULL;
    hPciDrv->PCIDRV_pfuncDevProbe  = pciStorageSataDevProbe;
    hPciDrv->PCIDRV_pfuncDevRemove = pciStorageSataDevRemove;

    iRet = API_PciDrvRegister(hPciDrv);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageSataInit
** 功能描述: PCI 类型控制器驱动相关初始化
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  pciStorageSataInit (VOID)
{
    AHCI_DRV_CB         tDrvReg;
    AHCI_DRV_HANDLE     hDrvReg = &tDrvReg;

    API_AhciDrvInit();

    lib_bzero(hDrvReg, sizeof(AHCI_DRV_CB));
    lib_strlcpy(&hDrvReg->AHCIDRV_cDrvName[0], SATA_PCI_DRV_NAME, AHCI_DRV_NAME_MAX);

    hDrvReg->AHCIDRV_uiDrvVer                   = SATA_PCI_DRV_VER_NUM;
    hDrvReg->AHCIDRV_hCtrl                      = LW_NULL;
    hDrvReg->AHCIDRV_pfuncOptCtrl               = pciStorageSataCtrlOpt;
    hDrvReg->AHCIDRV_pfuncVendorDriveInfoShow   = pciStorageSataVendorDriveInfoShow;
    hDrvReg->AHCIDRV_pfuncVendorDriveRegNameGet = pciStorageSataVendorDriveRegNameGet;
    hDrvReg->AHCIDRV_pfuncVendorDriveInit       = pciStorageSataVendorDriveInit;
    hDrvReg->AHCIDRV_pfuncVendorCtrlInfoShow    = pciStorageSataVendorCtrlInfoShow;
    hDrvReg->AHCIDRV_pfuncVendorCtrlRegNameGet  = pciStorageSataVendorCtrlRegNameGet;
    hDrvReg->AHCIDRV_pfuncVendorCtrlTypeNameGet = pciStorageSataVendorCtrlTypeNameGet;
    hDrvReg->AHCIDRV_pfuncVendorCtrlIntEnable   = pciStorageSataVendorCtrlIntEnable;
    hDrvReg->AHCIDRV_pfuncVendorCtrlIntConnect  = pciStorageSataVendorCtrlIntConnect;
    hDrvReg->AHCIDRV_pfuncVendorCtrlInit        = pciStorageSataVendorCtrlInit;
    hDrvReg->AHCIDRV_pfuncVendorCtrlReadyWork   = pciStorageSataVendorCtrlReadyWork;
    hDrvReg->AHCIDRV_pfuncVendorPlatformInit    = pciStorageSataVendorPlatformInit;
    hDrvReg->AHCIDRV_pfuncVendorDrvReadyWork    = pciStorageSataVendorDrvReadyWork;

    API_AhciDrvRegister(hDrvReg);

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0) &&      */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
                                                                        /*  (LW_CFG_DRV_SATA_AHCI > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
