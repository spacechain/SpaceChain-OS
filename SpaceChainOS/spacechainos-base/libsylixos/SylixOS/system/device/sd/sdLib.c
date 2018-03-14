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
** 文   件   名: sdLib.c
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2010 年 11 月 22 日
**
** 描        述: sd设备操作库

** BUG:
**
2011.01.18  增加SD设备获取函数.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_OBJECT_HANDLE    _G_hSdListLock = LW_OBJECT_HANDLE_INVALID;   /* 主控组件锁                   */
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define __SD_LIST_LOCK()           API_SemaphoreBPend(_G_hSdListLock, LW_OPTION_WAIT_INFINITE)
#define __SD_LIST_UNLOCK()         API_SemaphoreBPost(_G_hSdListLock)

#define __SD_ADAPTER_LOCK(pAda)    API_SemaphoreBPend((pAda)->SDADAPTER_hBusLock, LW_OPTION_WAIT_INFINITE)
#define __SD_ADAPTER_UNLOCK(pAda)  API_SemaphoreBPost((pAda)->SDADAPTER_hBusLock)
/*********************************************************************************************************
  内部调试相关
*********************************************************************************************************/
#define __SD_DEBUG_EN              0
#if __SD_DEBUG_EN > 0
#define __SD_DEBUG_MSG             __SD_DEBUG_MSG
#else
#define __SD_DEBUG_MSG(level, msg)
#endif
/*********************************************************************************************************
** 函数名称: __sdAdapterFind
** 功能描述: 查找一个sd适配器
** 输    入: NpcName  适配器名称
** 输    出: NONE
** 返    回: 找到,返回主控器指针,否则返回LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PLW_SD_ADAPTER  __sdAdapterFind (CPCHAR  pcName)
{
    PLW_BUS_ADAPTER      pbusadapter;

    pbusadapter = __busAdapterGet(pcName);                              /*  查找总线层适配器            */

    return  ((PLW_SD_ADAPTER)pbusadapter);
}
/*********************************************************************************************************
** 函数名称: API_SdLibInit
** 功能描述: 初始化SD组件库
** 输    入: NONE
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
INT  API_SdLibInit (VOID)
{
    if (_G_hSdListLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hSdListLock = API_SemaphoreBCreate("sd_listlock",
                                              LW_TRUE, LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                              LW_NULL);
    }

    if (_G_hSdListLock) {
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_SdAdapterCreate
** 功能描述: 创建一个sd适配器
** 输    入: pcName     适配器名称
**           psdfunc    适配器总线操作
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
INT  API_SdAdapterCreate (CPCHAR pcName, PLW_SD_FUNCS psdfunc)
{
    PLW_SD_ADAPTER  psdadapter;

    if (!pcName || !psdfunc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (_Object_Name_Invalid(pcName)) {                                 /*  检查名字有效性              */
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (PX_ERROR);
    }

    /*
     *  创建控制块
     */
    psdadapter = (PLW_SD_ADAPTER)__SHEAP_ALLOC(sizeof(LW_SD_ADAPTER));
    if (psdadapter == LW_NULL) {
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(psdadapter, sizeof(LW_SD_ADAPTER));

    psdadapter->SDADAPTER_psdfunc         = psdfunc;
    psdadapter->SDADAPTER_plineDevHeader  = LW_NULL;                     /*  目前还不存在 sd设备        */

    /*
     *  创建锁
     */
    psdadapter->SDADAPTER_hBusLock = API_SemaphoreBCreate("sd_buslock",
                                                          LW_TRUE, 
                                                          LW_OPTION_WAIT_FIFO | 
                                                          LW_OPTION_OBJECT_GLOBAL,
                                                          LW_NULL);
    if (psdadapter->SDADAPTER_hBusLock == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(psdadapter);
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "create semaphore failed.\r\n");
        return  (PX_ERROR);
    }

    /*
     *  加入总线层
     */
    if (__busAdapterCreate(&psdadapter->SDADAPTER_busadapter, pcName) != ERROR_NONE) {
        API_SemaphoreBDelete(&psdadapter->SDADAPTER_hBusLock);
        __SHEAP_FREE(psdadapter);
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "add adapter to system bus failed.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdAdapterDelete
** 功能描述: 删除一个sd适配器
** 输    入: pcName     适配器名称
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
INT API_SdAdapterDelete (CPCHAR pcName)
{
    PLW_SD_ADAPTER  psdadapter = __sdAdapterFind(pcName);

    if (psdadapter == LW_NULL) {
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL,"adapter is not exist.\r\n");
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到                      */
        return  (PX_ERROR);
    }

    if (API_SemaphoreBPend(psdadapter->SDADAPTER_hBusLock,
                           LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (psdadapter->SDADAPTER_plineDevHeader) {                         /*  检查是否有设备链接到主控    */
        API_SemaphoreBPost(psdadapter->SDADAPTER_hBusLock);
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "some devices are still on the bus.\r\n");
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }

    if (__busAdapterDelete(pcName) != ERROR_NONE) {                     /*  从总线层移除                */
        API_SemaphoreBPost(psdadapter->SDADAPTER_hBusLock);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }

    API_SemaphoreBDelete(&psdadapter->SDADAPTER_hBusLock);              /*  删除总线锁信号量            */
    __SHEAP_FREE(psdadapter);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdAdapterGet
** 功能描述: 获得一个sd适配器
** 输    入: pcName     适配器名称
** 输    出: NONE
** 返    回: 找到,返回主控器指针,否则返回LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
PLW_SD_ADAPTER   API_SdAdapterGet (CPCHAR pcName)
{
    PLW_SD_ADAPTER  psdadapter = __sdAdapterFind(pcName);

    if (psdadapter == LW_NULL) {
       __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "adapter is not exist.\r\n");
       _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                          /*  未找到                      */
       return  (LW_NULL);
    }

    return  (psdadapter);
}
/*********************************************************************************************************
** 函数名称: API_SdDeviceCreate
** 功能描述: 创建一个sd设备
** 输    入: pcAdapterName     适配器名称
**           pcDeviceName      设备名称
** 输    出: NONE
** 返    回: 找到,返回主控器指针,否则返回LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
PLW_SD_DEVICE  API_SdDeviceCreate (CPCHAR pcAdapterName, CPCHAR pcDeviceName)
{
    PLW_SD_ADAPTER  psdadapter = __sdAdapterFind(pcAdapterName);
    PLW_SD_DEVICE   psddevice;

    if (psdadapter == LW_NULL) {
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "no adapter.\r\n");
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);
        return  (LW_NULL);
    }
    
    if (_Object_Name_Invalid(pcDeviceName)) {                           /*  检查名字有效性              */
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_NULL);
    }

    psddevice = (PLW_SD_DEVICE)__SHEAP_ALLOC(sizeof(LW_SD_DEVICE));
    if ( psddevice == LW_NULL) {
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    lib_bzero(psddevice, sizeof(LW_SD_DEVICE));
    
    psddevice->SDDEV_psdAdapter             = psdadapter;
    psddevice->SDDEV_atomicUsageCnt.counter = 0;                        /*  目前还没有被使用过          */
    lib_strcpy(psddevice->SDDEV_pDevName, pcDeviceName);

    __SD_LIST_LOCK();
    _List_Line_Add_Ahead(&psddevice->SDDEV_lineManage,
                         &psdadapter->SDADAPTER_plineDevHeader);        /*  链入适配器设备链表          */
    __SD_LIST_UNLOCK();

    LW_BUS_INC_DEV_COUNT(&psdadapter->SDADAPTER_busadapter);

    return  (psddevice);
}
/*********************************************************************************************************
** 函数名称: API_SdDeviceDelete
** 功能描述: 删除指定的 sd 设备
** 输    入: psddevice        指定的 sd 设备控制块
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
INT  API_SdDeviceDelete (PLW_SD_DEVICE   psddevice)
{
    PLW_SD_ADAPTER        psdadapter;

    if (psddevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdadapter = psddevice->SDDEV_psdAdapter;
    if (psdadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }

    if (API_AtomicGet(&psddevice->SDDEV_atomicUsageCnt)) {
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }

    __SD_LIST_LOCK();
    _List_Line_Del(&psddevice->SDDEV_lineManage,
                   &psdadapter->SDADAPTER_plineDevHeader);
    __SD_LIST_UNLOCK();

    LW_BUS_DEC_DEV_COUNT(&psdadapter->SDADAPTER_busadapter);            /*  总线设备--                  */

    __SHEAP_FREE(psddevice);                                            /*  释放内存                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdDeviceGet
** 功能描述: 查找指定的 sd 设备
** 输    入: pcAdapterName        适配器名
**           pcDeviceName         设备名
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
PLW_SD_DEVICE  API_SdDeviceGet (CPCHAR pcAdapterName, CPCHAR pcDeviceName)
{
    PLW_LIST_LINE          plineTemp  = LW_NULL;
    PLW_SD_ADAPTER         psdadapter = LW_NULL;
    PLW_SD_DEVICE          psddevice  = LW_NULL;

    if (pcAdapterName == LW_NULL || pcDeviceName == LW_NULL) {
        return  (LW_NULL);
    }

    psdadapter = __sdAdapterFind(pcAdapterName);
    if (!psdadapter) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (LW_NULL);
    }

    __SD_LIST_LOCK();
    for (plineTemp  = psdadapter->SDADAPTER_plineDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  遍历适配器表                */

        psddevice = _LIST_ENTRY(plineTemp, LW_SD_DEVICE, SDDEV_lineManage);
        if (lib_strcmp(pcDeviceName, psddevice->SDDEV_pDevName) == 0) {
            break;                                                      /*  已经找到了                  */
        }
    }
    __SD_LIST_UNLOCK();

    if (plineTemp) {
        return  (psddevice);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: API_SdDeviceUsageInc
** 功能描述: 指定的 sd 设备使用计数++
** 输    入: psddevice        指定的 sd 设备控制块
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
INT   API_SdDeviceUsageInc (PLW_SD_DEVICE    psddevice)
{
    if (psddevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (API_AtomicInc(&psddevice->SDDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** 函数名称: API_SdDeviceUsageDec
** 功能描述: 指定的 sd 设备使用计数--
** 输    入: psddevice        指定的 sd 设备控制块
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
INT  API_SdDeviceUsageDec (PLW_SD_DEVICE   psddevice)
{
    if (psddevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (API_AtomicDec(&psddevice->SDDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** 函数名称: API_SdDeviceUsageGet
** 功能描述: 获得指定的 sd 设备的使用计数
** 输    入: psddevice     指定的 sd 设备控制块
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
INT  API_SdDeviceUsageGet (PLW_SD_DEVICE   psddevice)
{
    if (psddevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (API_AtomicGet(&psddevice->SDDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** 函数名称: API_SdDeviceTransfer
** 功能描述: 对设备发送一次请求
** 输    入: psddevice        指定的 sd 设备控制块
**           psdmsg           传输控制消息块组
**           iNum             传输控制消息块组中消息数量
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdDeviceTransfer (PLW_SD_DEVICE   psddevice,
                                  PLW_SD_MESSAGE  psdmsg,
                                  INT             iNum)
{
    PLW_SD_ADAPTER  psdadapter ;
    INT             iError;

    if (!psddevice || !psdmsg) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdadapter = psddevice->SDDEV_psdAdapter;
    if (psdadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }

    if (SDADP_TXF(psdadapter)) {
        __SD_ADAPTER_LOCK(psdadapter);                                  /*  锁定SD总线                  */

        iError = SDBUS_TRANSFER(psdadapter,
                                psddevice,
                                psdmsg,
                                iNum);

        __SD_ADAPTER_UNLOCK(psdadapter);                                /*  解锁SD总线                  */
    } else {
        _ErrorHandle(ENOSYS);
        iError = PX_ERROR;
    }

    if (iError != ERROR_NONE) {
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "bus request error.\r\n");
        return  (PX_ERROR);
    }

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_SdDeviceCtl
** 功能描述: 对设备发送一次控制操作
** 输    入: psddevice      指定的 sd 设备控制块
**           iCmd           控制命令
**           lArg           控制参数
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdDeviceCtl (PLW_SD_DEVICE  psddevice,
                             INT            iCmd,
                             LONG           lArg)
{
    PLW_SD_ADAPTER  psdadapter ;
    INT             iError;

    if (!psddevice ) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdadapter = psddevice->SDDEV_psdAdapter;
    if (psdadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }

    if (SDADP_CTRL(psdadapter)) {
        __SD_ADAPTER_LOCK(psdadapter);                                  /*  锁定SD总线                  */

        iError = SDBUS_IOCTRL(psdadapter,
                              iCmd,
                              lArg);

        __SD_ADAPTER_UNLOCK(psdadapter);                                /*  解锁SD总线                  */
    } else {
        _ErrorHandle(ENOSYS);
        iError = PX_ERROR;
    }

    return  (iError);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
