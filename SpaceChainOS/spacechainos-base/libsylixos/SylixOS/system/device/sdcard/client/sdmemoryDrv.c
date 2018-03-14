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
** 文   件   名: sdmemoryDrv.c
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2014 年 10 月 25 日
**
** 描        述: sd 记忆卡注册到 SDM 模块的驱动源文件
**
** BUG:
2015.09.12  __SDMEM_CACHE_BURST 猝发读写长度减小到 32 个扇区.
2015.09.18  猝发读写长度和磁盘缓冲大小可由驱动决定.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0)
#include "../SylixOS/fs/oemDisk/oemDisk.h"
#include "../core/sddrvm.h"
#include "sdmemory.h"
#include "sdmemoryDrv.h"
#include "../include/sddebug.h"
/*********************************************************************************************************
  内部宏定义
*********************************************************************************************************/
#define __SDMEM_MEDIA           "/media/sdcard"
#define __SDMEM_CACHE_BURST     64
#define __SDMEM_CACHE_SIZE      (512 * LW_CFG_KB_SIZE)
#define __SDMEM_CACHE_PL        1                                       /*  默认使用单管线加速写        */
#define __SDMEM_CACHE_COHERENCE LW_FALSE                                /*  默认不需要 CACHE 一致性要求 */
/*********************************************************************************************************
  sdmem 设备私有数据
*********************************************************************************************************/
struct __sdmem_priv {
    PLW_BLK_DEV         SDMEMPRIV_pblkdev;
    PLW_OEMDISK_CB      SDMEMPRIV_poemdisk;
};
typedef struct __sdmem_priv __SDMEM_PRIV;
/*********************************************************************************************************
  前置声明
*********************************************************************************************************/
static INT   __sdmemDevCreate(SD_DRV *psddrv, PLW_SDCORE_DEVICE psdcoredev, VOID **ppvDevPriv);
static INT   __sdmemDevDelete(SD_DRV *psddrv, VOID *pvDevPriv);
/*********************************************************************************************************
  sdmem 驱动对象
*********************************************************************************************************/
static SD_DRV  _G_sddrvMem;
/*********************************************************************************************************
** 函数名称: API_SdMemDrvInstall
** 功能描述: 安装 sd memory 驱动
** 输    入: NONE
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT API_SdMemDrvInstall (VOID)
{
    _G_sddrvMem.SDDRV_cpcName        = SDDRV_SDMEM_NAME;
    _G_sddrvMem.SDDRV_pfuncDevCreate = __sdmemDevCreate;
    _G_sddrvMem.SDDRV_pfuncDevDelete = __sdmemDevDelete;
    
    API_SdmSdDrvRegister(&_G_sddrvMem);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdmemDevCreate
** 功能描述: sd memory 设备创建
** 输    入: psddrv       sd 驱动
**           psdcoredev   sd 核心传输对象
**           ppvDevPriv   用于保存设备创建成功后的设备私有数据
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdmemDevCreate (SD_DRV *psddrv, PLW_SDCORE_DEVICE psdcoredev, VOID **ppvDevPriv)
{
    PLW_BLK_DEV       pblkdev;
    PLW_OEMDISK_CB    poemdisk;
    __SDMEM_PRIV     *psdmempriv;
    LONG              lCacheSize;
    LONG              lSectorBurst;
    LONG              lPl;
    LONG              lCoherence = (LONG)__SDMEM_CACHE_COHERENCE;
    LW_DISKCACHE_ATTR dcattrl;                                          /* CACHE 参数                   */

    psdmempriv= (__SDMEM_PRIV *)__SHEAP_ALLOC(sizeof(__SDMEM_PRIV));
    if (!psdmempriv) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        return  (PX_ERROR);
    }

    /*
     * 增加了SDM后, 约定: 当适配器名称和设备名称为空时, 表示coredev 由 SDM 创建
     * 此时, psdmemchan 指向对应的coredev
     */
    pblkdev = API_SdMemDevCreate(-1, LW_NULL, LW_NULL, (PLW_SDMEM_CHAN)psdcoredev);
    if (!pblkdev) {
        goto    __err1;
    }

    API_SdmHostExtOptGet(psdcoredev, SDHOST_EXTOPT_CACHE_SIZE_GET, (LONG)&lCacheSize);
    API_SdmHostExtOptGet(psdcoredev, SDHOST_EXTOPT_MAXBURST_SECTOR_GET, (LONG)&lSectorBurst);
    API_SdmHostExtOptGet(psdcoredev, SDHOST_EXTOPT_CACHE_PL_GET, (LONG)&lPl);
    API_SdmHostExtOptGet(psdcoredev, SDHOST_EXTOPT_CACHE_COHERENCE_GET, (LONG)&lCoherence);

    if (lCacheSize < 0) {
        lCacheSize = __SDMEM_CACHE_SIZE;
    }

    if (lSectorBurst <= 0) {
        lSectorBurst = __SDMEM_CACHE_BURST;
    }
    
    if (lPl <= 0) {
        lPl = __SDMEM_CACHE_PL;
    }

    dcattrl.DCATTR_pvCacheMem       = LW_NULL;
    dcattrl.DCATTR_stMemSize        = (size_t)lCacheSize;
    dcattrl.DCATTR_bCacheCoherence  = (BOOL)lCoherence;
    dcattrl.DCATTR_iMaxRBurstSector = (INT)lSectorBurst;
    dcattrl.DCATTR_iMaxWBurstSector = (INT)(lSectorBurst << 1);
    dcattrl.DCATTR_iMsgCount        = 4;
    dcattrl.DCATTR_iPipeline        = (INT)lPl;
    dcattrl.DCATTR_bParallel        = LW_FALSE;

    poemdisk = oemDiskMount2(__SDMEM_MEDIA,
                             pblkdev,
                             &dcattrl);
    if (!poemdisk) {
        printk("\nmount sd memory card failed.\r\n");
        goto    __err2;
    }

#if LW_CFG_HOTPLUG_EN > 0
    oemDiskHotplugEventMessage(poemdisk,
                               LW_HOTPLUG_MSG_SD_STORAGE,
                               LW_TRUE,
                               0, 0, 0, 0);
#endif

    printk("\nmount sd memory card successfully.\r\n");

    API_SdMemDevShow(pblkdev);

    psdmempriv->SDMEMPRIV_pblkdev  = pblkdev;
    psdmempriv->SDMEMPRIV_poemdisk = poemdisk;

    *ppvDevPriv = (VOID *)psdmempriv;

    return  (ERROR_NONE);

__err2:
    API_SdMemDevDelete(pblkdev);

__err1:
    __SHEAP_FREE(psdmempriv);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __sdmemDevDelete
** 功能描述: sd memory 设备删除
** 输    入: psddrv       sd 驱动
**           pvDevPriv    设备私有数据
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdmemDevDelete (SD_DRV *psddrv,  VOID *pvDevPriv)
{
    __SDMEM_PRIV  *psdmempriv = (__SDMEM_PRIV *)pvDevPriv;
    if (!psdmempriv) {
        return  (PX_ERROR);
    }

#if LW_CFG_HOTPLUG_EN > 0
    oemDiskHotplugEventMessage(psdmempriv->SDMEMPRIV_poemdisk,
                               LW_HOTPLUG_MSG_SD_STORAGE,
                               LW_FALSE,
                               0, 0, 0, 0);
#endif

    oemDiskUnmount(psdmempriv->SDMEMPRIV_poemdisk);
    
    API_SdMemDevDelete(psdmempriv->SDMEMPRIV_pblkdev);

    __SHEAP_FREE(psdmempriv);

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
