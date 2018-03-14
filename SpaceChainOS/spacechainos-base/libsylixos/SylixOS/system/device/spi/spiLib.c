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
** 文   件   名: spiLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 11 月 06 日
**
** 描        述: spi 设备操作库.

** BUG:
2011.04.02  加入对总线的请求与释放 API. 将片选的操作放在设备驱动程序中完成而不是总线驱动.
            SPI 总线互斥必须使用 mutex, 可支持多次递归调用.
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
static LW_OBJECT_HANDLE             _G_hSpiListLock = LW_OBJECT_HANDLE_INVALID;
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define __SPI_LIST_LOCK()           API_SemaphoreBPend(_G_hSpiListLock, LW_OPTION_WAIT_INFINITE)
#define __SPI_LIST_UNLOCK()         API_SemaphoreBPost(_G_hSpiListLock)
/*********************************************************************************************************
** 函数名称: __spiAdapterFind
** 功能描述: 查询 spi 适配器
** 输　入  : pcName        适配器名称
** 输　出  : 适配器指针
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_SPI_ADAPTER  __spiAdapterFind (CPCHAR  pcName)
{
    REGISTER PLW_BUS_ADAPTER        pbusadapter;
    
    pbusadapter = __busAdapterGet(pcName);                              /*  查找总线层适配器            */

    return  ((PLW_SPI_ADAPTER)pbusadapter);                             /*  总线结构为 SPI 适配器首元素 */
}
/*********************************************************************************************************
** 函数名称: API_SpiLibInit
** 功能描述: 初始化 spi 组件库
** 输　入  : NONE
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpiLibInit (VOID)
{
    if (_G_hSpiListLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hSpiListLock = API_SemaphoreBCreate("spi_listlock", 
                                               LW_TRUE, LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                               LW_NULL);
    }
    
    if (_G_hSpiListLock) {
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_SpiAdapterCreate
** 功能描述: 创建一个 spi 适配器
** 输　入  : pcName        适配器名称
**           pspifunc      操作函数组
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpiAdapterCreate (CPCHAR           pcName, 
                           PLW_SPI_FUNCS    pspifunc)
{
    REGISTER PLW_SPI_ADAPTER    pspiadapter;

    if (!pcName || !pspifunc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (_Object_Name_Invalid(pcName)) {                                 /*  检查名字有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (PX_ERROR);
    }
    
    /*
     *  创建控制块
     */
    pspiadapter = (PLW_SPI_ADAPTER)__SHEAP_ALLOC(sizeof(LW_SPI_ADAPTER));
    if (pspiadapter == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    pspiadapter->SPIADAPTER_pspifunc       = pspifunc;
    pspiadapter->SPIADAPTER_plineDevHeader = LW_NULL;                   /*  目前还不存在 spi 设备       */
    
    /*
     *  创建相关锁
     */
    pspiadapter->SPIADAPTER_hBusLock = API_SemaphoreMCreate("spi_buslock", LW_PRIO_DEF_CEILING,
                                                            LW_OPTION_WAIT_PRIORITY |
                                                            LW_OPTION_INHERIT_PRIORITY |
                                                            LW_OPTION_DELETE_SAFE |
                                                            LW_OPTION_OBJECT_GLOBAL,
                                                            LW_NULL);
    if (pspiadapter->SPIADAPTER_hBusLock == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pspiadapter);
        return  (PX_ERROR);
    }
    
    /*
     *  加入总线层
     */
    if (__busAdapterCreate(&pspiadapter->SPIADAPTER_pbusadapter, pcName) != ERROR_NONE) {
        API_SemaphoreMDelete(&pspiadapter->SPIADAPTER_hBusLock);
        __SHEAP_FREE(pspiadapter);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SpiAdapterDelete
** 功能描述: 移除一个 spi 适配器
** 输　入  : pcName        适配器名称
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : 在内核中的驱动必须确保永不再使用这个适配器了, 才能删除这个适配器.
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpiAdapterDelete (CPCHAR  pcName)
{
    REGISTER PLW_SPI_ADAPTER        pspiadapter = __spiAdapterFind(pcName);
    
    if (pspiadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }
    
    /*
     *  程序从查找设备, 到运行到这里, 并没有任何保护!
     */
    if (API_SemaphoreMPend(pspiadapter->SPIADAPTER_hBusLock, 
                           LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    if (pspiadapter->SPIADAPTER_plineDevHeader) {                       /*  检查是否有设备链接到适配器  */
        API_SemaphoreMPost(pspiadapter->SPIADAPTER_hBusLock);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    
    if (__busAdapterDelete(pcName) != ERROR_NONE) {                     /*  从总线层移除                */
        API_SemaphoreMPost(pspiadapter->SPIADAPTER_hBusLock);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    
    API_SemaphoreMDelete(&pspiadapter->SPIADAPTER_hBusLock);            /*  删除总线锁信号量            */
    __SHEAP_FREE(pspiadapter);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SpiAdapterGet
** 功能描述: 通过名字获取一个 spi 适配器
** 输　入  : pcName        适配器名称
** 输　出  : spi 适配器控制块
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PLW_SPI_ADAPTER  API_SpiAdapterGet (CPCHAR  pcName)
{
    REGISTER PLW_SPI_ADAPTER        pspiadapter = __spiAdapterFind(pcName);
    
    if (pspiadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (LW_NULL);
    
    } else {
        return  (pspiadapter);
    }
}
/*********************************************************************************************************
** 函数名称: API_SpiDeviceCreate
** 功能描述: 在指定 spi 适配器上, 创建一个 spi 设备
** 输　入  : pcAdapterName       适配器名称
**           pcDeviceName        设备名称
** 输　出  : spi 设备控制块
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PLW_SPI_DEVICE  API_SpiDeviceCreate (CPCHAR  pcAdapterName,
                                     CPCHAR  pcDeviceName)
{
    REGISTER PLW_SPI_ADAPTER    pspiadapter = __spiAdapterFind(pcAdapterName);
    REGISTER PLW_SPI_DEVICE     pspidevice;
    
    if (pspiadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (LW_NULL);
    }
    
    if (_Object_Name_Invalid(pcDeviceName)) {                           /*  检查名字有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_NULL);
    }
    
    pspidevice = (PLW_SPI_DEVICE)__SHEAP_ALLOC(sizeof(LW_SPI_DEVICE));
    if (pspidevice == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    
    pspidevice->SPIDEV_pspiadapter = pspiadapter;
    pspidevice->SPIDEV_atomicUsageCnt.counter = 0;                      /*  目前还没有被使用过          */
    lib_strcpy(pspidevice->SPIDEV_cName, pcDeviceName);
    
    __SPI_LIST_LOCK();                                                  /*  锁定 spi 链表               */
    _List_Line_Add_Ahead(&pspidevice->SPIDEV_lineManage, 
                         &pspiadapter->SPIADAPTER_plineDevHeader);      /*  链入适配器设备链表          */
    __SPI_LIST_UNLOCK();                                                /*  解锁 spi 链表               */
    
    LW_BUS_INC_DEV_COUNT(&pspiadapter->SPIADAPTER_pbusadapter);         /*  总线设备++                  */
    
    return  (pspidevice);
}
/*********************************************************************************************************
** 函数名称: API_SpiDeviceDelete
** 功能描述: 删除指定的 spi 设备
** 输　入  : pspidevice        指定的 spi 设备控制块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpiDeviceDelete (PLW_SPI_DEVICE   pspidevice)
{
    REGISTER PLW_SPI_ADAPTER        pspiadapter;

    if (pspidevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pspiadapter = pspidevice->SPIDEV_pspiadapter;
    if (pspiadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }
    
    if (API_AtomicGet(&pspidevice->SPIDEV_atomicUsageCnt)) {
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }
    
    __SPI_LIST_LOCK();                                                  /*  锁定 spi 链表               */
    _List_Line_Del(&pspidevice->SPIDEV_lineManage, 
                   &pspiadapter->SPIADAPTER_plineDevHeader);            /*  链入适配器设备链表          */
    __SPI_LIST_UNLOCK();                                                /*  解锁 spi 链表               */
    
    LW_BUS_DEC_DEV_COUNT(&pspiadapter->SPIADAPTER_pbusadapter);         /*  总线设备--                  */
    
    __SHEAP_FREE(pspidevice);                                           /*  释放内存                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SpiDeviceUsageInc
** 功能描述: 将指定 spi 设备使用计数++
** 输　入  : pspidevice        指定的 spi 设备控制块
** 输　出  : 当前的使用计数值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpiDeviceUsageInc (PLW_SPI_DEVICE   pspidevice)
{
    if (pspidevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (API_AtomicInc(&pspidevice->SPIDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** 函数名称: API_SpiDeviceUsageDec
** 功能描述: 将指定 spi 设备使用计数--
** 输　入  : pspidevice        指定的 spi 设备控制块
** 输　出  : 当前的使用计数值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpiDeviceUsageDec (PLW_SPI_DEVICE   pspidevice)
{
    if (pspidevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (API_AtomicDec(&pspidevice->SPIDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** 函数名称: API_SpiDeviceUsageGet
** 功能描述: 获得指定 spi 设备使用计数
** 输　入  : pspidevice        指定的 spi 设备控制块
** 输　出  : 当前的使用计数值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpiDeviceUsageGet (PLW_SPI_DEVICE   pspidevice)
{
    if (pspidevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (API_AtomicGet(&pspidevice->SPIDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** 函数名称: API_SpiDeviceBusRequest
** 功能描述: 获得指定 SPI 设备的总线使用权
** 输　入  : pspidevice        指定的 spi 设备控制块
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpiDeviceBusRequest (PLW_SPI_DEVICE   pspidevice)
{
    REGISTER PLW_SPI_ADAPTER        pspiadapter;

    if (!pspidevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pspiadapter = pspidevice->SPIDEV_pspiadapter;
    if (pspiadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }
    
    if (API_SemaphoreMPend(pspiadapter->SPIADAPTER_hBusLock, 
                           LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {    /*  锁定总线                    */
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SpiDeviceBusRelease
** 功能描述: 释放指定 SPI 设备的总线使用权
** 输　入  : pspidevice        指定的 spi 设备控制块
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpiDeviceBusRelease (PLW_SPI_DEVICE   pspidevice)
{
    REGISTER PLW_SPI_ADAPTER        pspiadapter;

    if (!pspidevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pspiadapter = pspidevice->SPIDEV_pspiadapter;
    if (pspiadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }
    
    if (API_SemaphoreMPost(pspiadapter->SPIADAPTER_hBusLock) != 
        ERROR_NONE) {                                                   /*  锁定总线                    */
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SpiDeviceTransfer
** 功能描述: 使用指定 spi 设备进行一次传输
** 输　入  : pspidevice        指定的 spi 设备控制块
**           pspimsg           传输消息控制块组
**           iNum              控制消息组中消息的数量
** 输　出  : 操作的 pspimsg 数量, 错误返回 -1
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpiDeviceTransfer (PLW_SPI_DEVICE   pspidevice, 
                            PLW_SPI_MESSAGE  pspimsg,
                            INT              iNum)
{
    REGISTER PLW_SPI_ADAPTER        pspiadapter;
             INT                    iRet;

    if (!pspidevice || !pspimsg || (iNum < 1)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pspiadapter = pspidevice->SPIDEV_pspiadapter;
    if (pspiadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }
    
    
    if (pspiadapter->SPIADAPTER_pspifunc->SPIFUNC_pfuncMasterXfer) {
        if (API_SemaphoreMPend(pspiadapter->SPIADAPTER_hBusLock, 
                               LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {/*  锁定总线                    */
            return  (PX_ERROR);
        }
        iRet = pspiadapter->SPIADAPTER_pspifunc->SPIFUNC_pfuncMasterXfer(pspiadapter, pspimsg, iNum);
        API_SemaphoreMPost(pspiadapter->SPIADAPTER_hBusLock);           /*  释放总线                    */
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SpiDeviceCtl
** 功能描述: 指定 spi 设备处理指定命令
** 输　入  : pspidevice        指定的 spi 设备控制块
**           iCmd              命令编号
**           lArg              命令参数
** 输　出  : 命令执行结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpiDeviceCtl (PLW_SPI_DEVICE   pspidevice, INT  iCmd, LONG  lArg)
{
    REGISTER PLW_SPI_ADAPTER        pspiadapter;

    if (!pspidevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pspiadapter = pspidevice->SPIDEV_pspiadapter;
    if (pspiadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }
    
    if (pspiadapter->SPIADAPTER_pspifunc->SPIFUNC_pfuncMasterCtl) {
        return  (pspiadapter->SPIADAPTER_pspifunc->SPIFUNC_pfuncMasterCtl(pspiadapter, iCmd, lArg));
    } else {
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
