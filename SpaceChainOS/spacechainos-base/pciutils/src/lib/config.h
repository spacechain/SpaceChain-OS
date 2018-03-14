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
** 文   件   名: config.h
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2015 年 09 月 28 日
**
** 描        述: 工具配置参数.
*********************************************************************************************************/

#ifndef __CONFIG_H
#define __CONFIG_H

#define PCI_CONFIG_H
#define PCI_OS_SYLIXOS
#define PCI_PATH_SYLIXOS_DEVICE             "/proc/pci"
#define PCI_HAVE_PM_SYLIXOS_DEVICE
#define PCI_HAVE_64BIT_ADDRESS
#define PCI_IDS                             "pci.ids"
#define PCI_PATH_IDS_DIR                    "/etc/pci"
#define PCILIB_VERSION                      "3.5.2"

#define PCI_SCAN_FUNC                       "PCI_SCAN_TBL"

#endif                                                                  /*  __CONFIG_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
