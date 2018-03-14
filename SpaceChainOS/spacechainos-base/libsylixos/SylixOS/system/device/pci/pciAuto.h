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
** 文   件   名: pciAuto.h
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2016 年 10 月 21 日
**
** 描        述: PCI 总线自动配置.
**
*********************************************************************************************************/

#ifndef __PCIAUTO_H
#define __PCIAUTO_H

#include "pciDev.h"
#include "pciError.h"

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
/*********************************************************************************************************
  调试级别
*********************************************************************************************************/
#define PCI_AUTO_LOG_RUN                    __LOGMESSAGE_LEVEL
#define PCI_AUTO_LOG_PRT                    __LOGMESSAGE_LEVEL
#define PCI_AUTO_LOG_ERR                    __ERRORMESSAGE_LEVEL
#define PCI_AUTO_LOG_BUG                    __BUGMESSAGE_LEVEL
#define PCI_AUTO_LOG_ALL                    __PRINTMESSAGE_LEVEL

#define PCI_AUTO_LOG                        _DebugFormat
/*********************************************************************************************************
  PCI 设备自动配置参数配置
*********************************************************************************************************/
typedef INT     PCI_AUTO_DEV_HANDLE;

#define PCI_AUTO_BUS(d)                     (((d) >> 16) & 0xff)
#define PCI_AUTO_DEV(d)                     (((d) >> 11) & 0x1f)
#define PCI_AUTO_FUNC(d)                    (((d) >> 8) & 0x7)
#define PCI_AUTO_DEVFN(d, f)                ((d) << 11 | (f) << 8)
#define PCI_AUTO_MASK_BUS(bdf)              ((bdf) & 0xffff)
#define PCI_AUTO_ADD_BUS(bus, devfn)        (((bus) << 16) | (devfn))
#define PCI_AUTO_BDF(b, d, f)               ((b) << 16 | PCI_AUTO_DEVFN(d, f))
#define PCI_AUTO_VENDEV(v, d)               (((v) << 16) | (d))
#define PCI_AUTO_ANY_ID                     (~0)

/*********************************************************************************************************
  PCI 设备自动配置参数配置
*********************************************************************************************************/
#define PCI_AUTO_CACHE_LINE_SIZE            8
#define PCI_AUTO_LATENCY_TIMER              0x80

/*********************************************************************************************************
  PCI 设备自动配置资源控制块
*********************************************************************************************************/
#define PCI_AUTO_REGION_MAX                 7
#define PCI_AUTO_REGION_INDEX_0             0
#define PCI_AUTO_REGION_INDEX_1             1
#define PCI_AUTO_REGION_INDEX_2             2
#define PCI_AUTO_REGION_INDEX_3             3
#define PCI_AUTO_REGION_INDEX_4             4
#define PCI_AUTO_REGION_INDEX_5             5
#define PCI_AUTO_REGION_INDEX_6             6

#define PCI_AUTO_REGION_MEM                 0x00000000                  /* PCI memory space             */
#define PCI_AUTO_REGION_IO                  0x00000001                  /* PCI IO space                 */
#define PCI_AUTO_REGION_TYPE                0x00000001
#define PCI_AUTO_REGION_PREFETCH            0x00000008                  /* prefetchable PCI memory      */

#define PCI_AUTO_REGION_SYS_MEMORY          0x00000100                  /* System memory                */
#define PCI_AUTO_REGION_RO                  0x00000200                  /* Read-only memory             */

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
#endif                                                                  /*  __PCIAUTOCFG_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
