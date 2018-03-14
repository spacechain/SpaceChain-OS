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
** 文   件   名: sdiodrvm.c
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2014 年 10 月 27 日
**
** 描        述: sd drv manager layer for sdio

** BUG:
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0) && (LW_CFG_SDCARD_SDIO_EN > 0)
#include "sddrvm.h"
#include "sdiodrvm.h"
#include "sdutil.h"
#include "../include/sddebug.h"
/*********************************************************************************************************
  SDM 新驱动注册事件通知
*********************************************************************************************************/
#define SDM_EVENT_NEW_DRV       10
/*********************************************************************************************************
  全局对象
*********************************************************************************************************/
static LW_OBJECT_HANDLE         _G_hSdmDrvLock        = LW_OBJECT_HANDLE_INVALID;
static LW_LIST_LINE_HEADER      _G_plineSdiodrvHeader = LW_NULL;

#define __SDM_DRV_LOCK()        API_SemaphoreBPend(_G_hSdmDrvLock, LW_OPTION_WAIT_INFINITE)
#define __SDM_DRV_UNLOCK()      API_SemaphoreBPost(_G_hSdmDrvLock)
/*********************************************************************************************************
  前置声明
*********************************************************************************************************/
static VOID  __sdmIoDrvInsert(SDIO_DRV *psdiodrv);
static VOID  __sdmIoDrvDelete(SDIO_DRV *psdiodrv);
/*********************************************************************************************************
** 函数名称: API_SdmSdioLibInit
** 功能描述: SDM SDIO部分 组件库初始化
** 输    入: NONE
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT   API_SdmSdioLibInit (VOID)
{
    if (_G_hSdmDrvLock != LW_OBJECT_HANDLE_INVALID) {
        return  (ERROR_NONE);
    }

    _G_hSdmDrvLock = API_SemaphoreBCreate("sdmdrv_lock", LW_TRUE, LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (_G_hSdmDrvLock == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdmSdioDrvRegister
** 功能描述: 向SDM注册一个SDIO设备应用驱动
** 输    入: psdiodrv         SDIO 驱动对象. 注意SDM内部会直接引用该对象, 因此该对象需要持续有效
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT   API_SdmSdioDrvRegister (SDIO_DRV *psdiodrv)
{
    SDIO_DRV          *psdiodrvTmp;
    PLW_LIST_LINE      plineTmp;

    if (!psdiodrv) {
        return  (PX_ERROR);
    }

    __SDM_DRV_LOCK();
    for (plineTmp  = _G_plineSdiodrvHeader;
         plineTmp != LW_NULL;
         plineTmp  = _list_line_get_next(plineTmp)) {

        psdiodrvTmp = _LIST_ENTRY(plineTmp, SDIO_DRV, SDIODRV_lineManage);
        if (psdiodrvTmp == psdiodrv) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "drv has been already registered.\r\n");
            __SDM_DRV_UNLOCK();
            return  (PX_ERROR);
        }

        /*
         * 因为 SDM 内部对驱动的名字不敏感, 这里仅仅警告
         */
        if (lib_strcmp(psdiodrvTmp->SDIODRV_cpcName, psdiodrv->SDIODRV_cpcName) == 0) {
            SDCARD_DEBUG_MSG(__LOGMESSAGE_LEVEL, " warning: exist a same name drv"
                                                 " as current registering.\r\n");
        }
    }
    __SDM_DRV_UNLOCK();

    API_AtomicSet(0, &psdiodrv->SDIODRV_atomicDevCnt);
    __sdmIoDrvInsert(psdiodrv);

    API_SdmEventNotify(LW_NULL, SDM_EVENT_NEW_DRV);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdmSdioDrvUnRegister
** 功能描述: 从SDM注销一个SDIO设备应用驱动
** 输    入: psdiodrv     SDIO 驱动对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT   API_SdmSdioDrvUnRegister (SDIO_DRV *psdiodrv)
{
    if (!psdiodrv) {
        return  (PX_ERROR);
    }

    if (API_AtomicGet(&psdiodrv->SDIODRV_atomicDevCnt)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "exist device using this drv.\r\n");
        return  (PX_ERROR);
    }

    __sdmIoDrvDelete(psdiodrv);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdmIoDrvInsert
** 功能描述: 插入一个 SD 驱动
** 输    入: psdiodrv     SDIO 驱动对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __sdmIoDrvInsert (SDIO_DRV *psdiodrv)
{
    __SDM_DRV_LOCK();
    _List_Line_Add_Ahead(&psdiodrv->SDIODRV_lineManage, &_G_plineSdiodrvHeader);
    __SDM_DRV_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __sdmIoDrvDelete
** 功能描述: 删除一个 SD 驱动
** 输    入: psdiodrv     SDIO 驱动对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __sdmIoDrvDelete (SDIO_DRV *psdiodrv)
{
    __SDM_DRV_LOCK();
    _List_Line_Del(&psdiodrv->SDIODRV_lineManage, &_G_plineSdiodrvHeader);
    __SDM_DRV_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __sdmSdioDrvHeader
** 功能描述: 获得第一个 SDIO 驱动(仅sdiobase 驱动模块可见)
** 输    入: NONE
** 输    出: 第一个 SDIO 驱动的链表节点
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PLW_LIST_LINE  __sdmSdioDrvHeader (VOID)
{
    return  (_G_plineSdiodrvHeader);
}
/*********************************************************************************************************
** 函数名称: __sdmSdioDrvAccessRequest
** 功能描述: SDIO 驱动链表访问请求(仅sdiobase 驱动模块可见)
** 输    入: NONE
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __sdmSdioDrvAccessRequest (VOID)
{
    __SDM_DRV_LOCK();
}
/*********************************************************************************************************
** 函数名称: __sdmSdioDrvAccessRelease
** 功能描述: SDIO 驱动链表访问释放(仅sdiobase 驱动模块可见)
** 输    入: NONE
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __sdmSdioDrvAccessRelease (VOID)
{
    __SDM_DRV_UNLOCK();
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_SDIO_EN > 0) */
/*********************************************************************************************************
  END
*********************************************************************************************************/
