/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: s_system.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 13 日
**
** 描        述: 这是系统综合头文件库。

** BUG:
2009.10.02  调整了头文件的顺序.
*********************************************************************************************************/

#ifndef __S_SYSTEM_H
#define __S_SYSTEM_H

/*********************************************************************************************************
  系统结构部分
*********************************************************************************************************/
#include "../SylixOS/system/include/s_type.h"
#include "../SylixOS/system/include/s_option.h"
#include "../SylixOS/system/include/s_error.h"
#include "../SylixOS/system/include/s_const.h"
#ifdef   __SYLIXOS_KERNEL
#include "../SylixOS/system/include/s_class.h"
#endif
#include "../SylixOS/system/include/s_stat.h"
#include "../SylixOS/system/include/s_fcntl.h"
#include "../SylixOS/system/include/s_dirent.h"
#include "../SylixOS/system/include/s_utime.h"
#include "../SylixOS/system/include/s_api.h"
#ifdef   __SYLIXOS_KERNEL
#include "../SylixOS/system/include/s_globalvar.h"
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#include "../SylixOS/system/include/s_object.h"
#ifdef   __SYLIXOS_KERNEL
#include "../SylixOS/system/include/s_internal.h"
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#include "../SylixOS/system/include/s_systeminit.h"
/*********************************************************************************************************
  中间层
*********************************************************************************************************/
#include "../SylixOS/system/util/sioLib.h"

#ifdef   __SYLIXOS_KERNEL
#include "../SylixOS/system/util/bmsgLib.h"
#include "../SylixOS/system/util/rngLib.h"
#include "../SylixOS/system/excLib/excLib.h"
#include "../SylixOS/system/logLib/logLib.h"
#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  select
*********************************************************************************************************/
#include "../SylixOS/system/select/select.h"
#ifdef   __SYLIXOS_KERNEL
#include "../SylixOS/system/select/selectDrv.h"
#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  电源管理
*********************************************************************************************************/
#ifdef   __SYLIXOS_KERNEL
#include "../SylixOS/system/pm/pmAdapter.h"
#include "../SylixOS/system/pm/pmDev.h"
#include "../SylixOS/system/pm/pmIdle.h"
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#include "../SylixOS/system/pm/pmSystem.h"
/*********************************************************************************************************
  总线系统层
*********************************************************************************************************/
#include "../SylixOS/system/bus/busSystem.h"
/*********************************************************************************************************
  各类设备模型
*********************************************************************************************************/
#include "../SylixOS/system/device/hwrtc/hwrtc.h"                       /*  real time clock             */
#include "../SylixOS/system/device/spipe/spipe.h"                       /*  stream pipe                 */
#include "../SylixOS/system/device/pipe/pipe.h"                         /*  vxworks pipe                */
#include "../SylixOS/system/device/ty/tty.h"                            /*  terminal device             */
#include "../SylixOS/system/device/pty/pty.h"                           /*  pseudo terminal             */
#include "../SylixOS/system/device/block/blockIo.h"                     /*  block device                */
#include "../SylixOS/system/device/can/can.h"                           /*  CAN bus device              */
#include "../SylixOS/system/device/buzzer/buzzer.h"                     /*  buzzer device               */
#include "../SylixOS/system/device/graph/gmemDev.h"                     /*  graph memory device         */

#ifdef   __SYLIXOS_KERNEL
#include "../SylixOS/system/device/block/blockRaw.h"                    /*  block raw device            */
#include "../SylixOS/system/device/block/ramDisk.h"                     /*  RAM disk                    */
#include "../SylixOS/system/device/ata/ata.h"                           /*  ATA device                  */
#include "../SylixOS/system/device/dma/dma.h"                           /*  DMA device                  */
#include "../SylixOS/system/device/dma/dmaLib.h"
#include "../SylixOS/system/device/gpio/gpioLib.h"                      /*  GPIO 驱动框架               */
#include "../SylixOS/system/device/gpio/gpioDev.h"                      /*  GPIO 用空态操作设备接口     */
#include "../SylixOS/system/device/hstimerfd/hstimerfdDev.h"            /*  高速定时器设备              */
#include "../SylixOS/system/device/rand/randDev.h"                      /*  random number generator     */
#include "../SylixOS/system/device/shm/shm.h"                           /*  shared memory device        */
#include "../SylixOS/system/device/i2c/i2cDev.h"                        /*  i2c 总线符号仅对内核开放    */
#include "../SylixOS/system/device/spi/spiDev.h"                        /*  spi 总线符号仅对内核开放    */
#include "../SylixOS/system/device/sd/sdDev.h"                          /*  sd 总线符号仅对内核开放     */
#include "../SylixOS/system/device/sdcard/include/sdcardLib.h"          /*  sd 卡相关驱动框架           */
#include "../SylixOS/system/device/mem/memDev.h"                        /*  VxWorks memDev              */
#include "../SylixOS/system/device/mii/miiDev.h"                        /*  mii phy 接口驱动            */
#include "../SylixOS/system/device/eventfd/eventfdDev.h"                /*  eventfd 设备                */
/*********************************************************************************************************
  AHCI 总线及其设备驱动模型
*********************************************************************************************************/
#ifdef   __SYLIXOS_AHCI_DRV
#include "../SylixOS/system/device/ahci/ahci.h"                         /*  AHCI 控制器                 */
#include "../SylixOS/system/device/ahci/ahciLib.h"                      /*  AHCI 库                     */
#include "../SylixOS/system/device/ahci/ahciDrv.h"                      /*  AHCI 驱动接口               */
#endif                                                                  /*  __SYLIXOS_AHCI_DRV          */
/*********************************************************************************************************
  NVMe 总线及其设备驱动模型
*********************************************************************************************************/
#ifdef   __SYLIXOS_NVME_DRV
#include "../SylixOS/system/device/nvme/nvme.h"                         /*  NVME 控制器                 */
#include "../SylixOS/system/device/nvme/nvmeLib.h"                      /*  NVME 库                     */
#include "../SylixOS/system/device/nvme/nvmeDrv.h"                      /*  NVME 驱动接口               */
#endif                                                                  /*  __SYLIXOS_NVME_DRV          */
/*********************************************************************************************************
  PCI 总线及其设备驱动模型
*********************************************************************************************************/
#ifdef   __SYLIXOS_PCI_DRV
#include "../SylixOS/system/device/pci/pciDev.h"                        /*  pci 设备符号仅对内核开放    */
#include "../SylixOS/system/device/pci/pciDrv.h"                        /*  pci 驱动符号仅对内核开放    */
#include "../SylixOS/system/device/pci/pciScan.h"                       /*  pci 总线自动扫描安装对应驱动*/
#endif                                                                  /*  __SYLIXOS_PCI_DRV           */
#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  系统信息接口
*********************************************************************************************************/
#include "../SylixOS/system/distribute/distribute.h"
/*********************************************************************************************************
  应用接口
*********************************************************************************************************/
#include "../SylixOS/system/signal/signal.h"

#ifdef   __SYLIXOS_KERNEL
#include "../SylixOS/system/signal/signalLib.h"
#include "../SylixOS/system/signal/signalDev.h"
#endif

#include "../SylixOS/system/ptimer/ptimer.h"
#ifdef   __SYLIXOS_KERNEL
#include "../SylixOS/system/ptimer/ptimerDev.h"
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#include "../SylixOS/system/hotplugLib/hotplugLib.h"
/*********************************************************************************************************
  初始化
*********************************************************************************************************/
#ifdef   __SYLIXOS_KERNEL
#include "../SylixOS/system/sysInit/sysInit.h"
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#endif                                                                  /*  __S_SYSTEM_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
