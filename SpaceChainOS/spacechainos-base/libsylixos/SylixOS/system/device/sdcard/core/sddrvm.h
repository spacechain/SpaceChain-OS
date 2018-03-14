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
** 文   件   名: sddrvm.h
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2014 年 10 月 24 日
**
** 描        述: sd drv manager layer

** BUG:
2015.09.18  增加控制器扩展选项设置, 以适应更多实际应用的场合
2016.04.01  增加引导设备插入创建的事件. 主要用于系统引导(eMMC)设备的初始化.
2017.02.27  增加控制器配置标志的扩展选项,可提高性能和兼容性.
*********************************************************************************************************/

#ifndef __SDDRVM_H
#define __SDDRVM_H

#include "sdcore.h"

/*********************************************************************************************************
  SDM 事件类型 for API_SdmEventNotify()

  SDM_EVENT_DEV_INSERT : 通知 SDM 层设备插入
      如果参数指定了一个 SDM Host 对象, 则只对该 Host 进行设备探测操作, 此种情况通常用于
      控制器驱动程序的热插拔处理.
      如果参数为 LW_NULL, 则对所有的还不存在任何设备或仅存在孤儿设备的 SDM Host 进行
      设备探测, 此种情况可用于应用层驱动自行决定设备的热插拔处理时机, 并且此情况下, 控制器驱动通常不再
      处理热插拔事件.

  SDM_EVENT_DEV_REMOVE : 通知 SDM 层移除设备
      参数指定了一个 SDM Host 对象, 则只对该 Host 进行设备移除操作, 此种情况通常用于
      控制器驱动程序的热插拔处理. 应用驱动也可以通过 SD 核心设备对象查找到对应的 SDM Host 对象, 进行手动
      移除设备操作.

  SDM_EVENT_SDIO_INTERRUPT : 通知 SDM 层产生了一次 SDIO 中断. SDM 层将处理驱动注册的 SDIO 中断服务.

  SDM_EVENT_BOOT_DEV_INSERT : 通知 SDM 层一个 BOOT 类型设备插入. 不同于 SDM_EVENT_DEV_INSERT 处理设备探测
      是在热插拔线程里处理, 该类型消息将直接在当前线程执行设备探测过程. 专为 BOOT 设备快速初始化设计,
      如一个需要挂载根文件系统的 SD/MMC 卡, 这样可以显著提高系统启动速度.
*********************************************************************************************************/

#define SDM_EVENT_DEV_INSERT            0
#define SDM_EVENT_DEV_REMOVE            1
#define SDM_EVENT_SDIO_INTERRUPT        2
#define SDM_EVENT_BOOT_DEV_INSERT       3

/*********************************************************************************************************
  前置声明
*********************************************************************************************************/
struct sd_drv;
struct sd_host;

typedef struct sd_drv     SD_DRV;
typedef struct sd_host    SD_HOST;

/*********************************************************************************************************
  sd 驱动(用于sd memory 和 sdio base)
*********************************************************************************************************/

struct sd_drv {
    LW_LIST_LINE  SDDRV_lineManage;                               /*  驱动挂载链                        */

    CPCHAR        SDDRV_cpcName;
#define SDDRV_SDMEM_NAME      "sd memory"
#define SDDRV_SDIOB_NAME      "sdio base"

    INT         (*SDDRV_pfuncDevCreate)(SD_DRV *psddrv, PLW_SDCORE_DEVICE psdcoredev, VOID **ppvDevPriv);
    INT         (*SDDRV_pfuncDevDelete)(SD_DRV *psddrv, VOID *pvDevPriv);

    atomic_t      SDDRV_atomicDevCnt;

    VOID         *SDDRV_pvSpec;
};

/*********************************************************************************************************
  sd host 信息结构
*********************************************************************************************************/

#ifdef  __cplusplus
typedef INT     (*SD_CALLBACK)(...);
#else
typedef INT     (*SD_CALLBACK)();
#endif

struct sd_host {
    CPCHAR        SDHOST_cpcName;

    INT           SDHOST_iType;
#define SDHOST_TYPE_SD                  0
#define SDHOST_TYPE_SPI                 1

    INT           SDHOST_iCapbility;                                /*  主动支持的特性                  */
#define SDHOST_CAP_HIGHSPEED            (1 << 0)                    /*  支持高速传输                    */
#define SDHOST_CAP_DATA_4BIT            (1 << 1)                    /*  支持4位数据传输                 */
#define SDHOST_CAP_DATA_8BIT            (1 << 2)                    /*  支持8位数据传输                 */
#define SDHOST_CAP_DATA_4BIT_DDR        (1 << 3)                    /*  支持4位ddr数据传输              */
#define SDHOST_CAP_DATA_8BIT_DDR        (1 << 4)                    /*  支持8位ddr数据传输              */
#define SDHOST_CAP_MMC_FORCE_1BIT       (1 << 5)                    /*  MMC卡 强制使用 1 位总线         */
#define SDHOST_CAP_SDIO_FORCE_1BIT      (1 << 6)                    /*  SDIO 卡 强制使用 1 位总线       */

    VOID          (*SDHOST_pfuncSpicsEn)(SD_HOST *psdhost);
    VOID          (*SDHOST_pfuncSpicsDis)(SD_HOST *psdhost);
    INT           (*SDHOST_pfuncCallbackInstall)
                  (
                  SD_HOST          *psdhost,
                  INT               iCallbackType,                  /*  安装的回调函数的类型            */
                  SD_CALLBACK       callback,                       /*  回调函数指针                    */
                  PVOID             pvCallbackArg                   /*  回调函数的参数                  */
                  );

    INT           (*SDHOST_pfuncCallbackUnInstall)
                  (
                  SD_HOST          *psdhost,
                  INT               iCallbackType                   /*  安装的回调函数的类型            */
                  );
#define SDHOST_CALLBACK_CHECK_DEV       0                           /*  卡状态检测                      */
#define SDHOST_DEVSTA_UNEXIST           0                           /*  卡状态:不存在                   */
#define SDHOST_DEVSTA_EXIST             1                           /*  卡状态:存在                     */

    VOID          (*SDHOST_pfuncSdioIntEn)(SD_HOST *psdhost, BOOL bEnable);
    BOOL          (*SDHOST_pfuncIsCardWp)(SD_HOST *psdhost);

    VOID          (*SDHOST_pfuncDevAttach)(SD_HOST *psdhost, CPCHAR cpcDevName);
    VOID          (*SDHOST_pfuncDevDetach)(SD_HOST *psdhost);
};


/*********************************************************************************************************
  sd host 扩展选项,这些选项均仅对指定的一个硬件控制器通道有效
*********************************************************************************************************/

#define SDHOST_EXTOPT_RESERVE_SECTOR_SET        0                   /*  设置保留扇区数量                */
#define SDHOST_EXTOPT_RESERVE_SECTOR_GET        1                   /*  获得保留扇区数量                */

#define SDHOST_EXTOPT_MAXBURST_SECTOR_SET       2                   /*  设置最大猝发扇区数量            */
#define SDHOST_EXTOPT_MAXBURST_SECTOR_GET       3                   /*  获得最大猝发扇区数量            */

#define SDHOST_EXTOPT_CACHE_SIZE_SET            4                   /*  设置磁盘 cache 大小             */
#define SDHOST_EXTOPT_CACHE_SIZE_GET            5                   /*  获得磁盘 cache 大小             */

#define SDHOST_EXTOPT_CACHE_PL_SET              6                   /*  设置磁盘管线数量                */
#define SDHOST_EXTOPT_CACHE_PL_GET              7                   /*  获得磁盘管线数量                */

#define SDHOST_EXTOPT_CACHE_COHERENCE_SET       8                   /*  设置磁盘 cache 一致性属性       */
#define SDHOST_EXTOPT_CACHE_COHERENCE_GET       9                   /*  获得磁盘 cache 一致性属性       */

#define SDHOST_EXTOPT_CONFIG_FLAG_SET           10                  /*  设置配置标志                    */
#define SDHOST_EXTOPT_CONFIG_FLAG_GET           11                  /*  获得配置标志                    */

#define SDHOST_EXTOPT_CONFIG_RESELECT_SDMEM     (1 << 0)            /*  SDMEM 每一次传输都重新选择卡    */
#define SDHOST_EXTOPT_CONFIG_RESELECT_SDIO      (1 << 1)            /*  SDIO 每一次传输都重新选择卡     */
#define SDHOST_EXTOPT_CONFIG_SKIP_SDMEM         (1 << 2)            /*  跳过 SDMEM 驱动探测             */
#define SDHOST_EXTOPT_CONFIG_SKIP_SDIO          (1 << 3)            /*  跳过 SDIO 驱动探测              */

/*********************************************************************************************************
  API
*********************************************************************************************************/

LW_API INT   API_SdmLibInit(VOID);

LW_API PVOID API_SdmHostRegister(SD_HOST *psdhost);
LW_API INT   API_SdmHostUnRegister(PVOID  pvSdmHost);

LW_API INT   API_SdmHostCapGet(PLW_SDCORE_DEVICE psdcoredev, INT *piCapbility);
LW_API VOID  API_SdmHostInterEn(PLW_SDCORE_DEVICE psdcoredev, BOOL bEnable);
LW_API BOOL  API_SdmHostIsCardWp(PLW_SDCORE_DEVICE psdcoredev);

LW_API PVOID API_SdmHostGet(PLW_SDCORE_DEVICE psdcoredev);

LW_API INT   API_SdmSdDrvRegister(SD_DRV *psddrv);
LW_API INT   API_SdmSdDrvUnRegister(SD_DRV *psddrv);

LW_API INT   API_SdmEventNotify(PVOID pvSdmHost, INT iEvtType);

/*********************************************************************************************************
  扩展选项设置 API
  API_SdmHostExtOptSet 用于驱动设置控制器的扩展选项
  API_SdmHostExtOptGet 用于协议层获取控制器的扩展选项
*********************************************************************************************************/

LW_API INT   API_SdmHostExtOptSet(PVOID pvSdmHost, INT  iOption, LONG  lArg);
LW_API INT   API_SdmHostExtOptGet(PLW_SDCORE_DEVICE psdcoredev, INT  iOption, LONG  lArg);

#endif                                                              /*  __SDDRVM_H                      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
