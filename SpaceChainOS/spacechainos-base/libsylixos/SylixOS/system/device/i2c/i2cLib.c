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
** 文   件   名: i2cLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 10 月 20 日
**
** 描        述: i2c 设备操作库.

** BUG:
2009.10.27  系统加入总线层, 将 i2c 总线适配器挂入总线层.
2012.11.09  API_I2cAdapterCreate() 不需要拷贝名字, 由总线创建时拷贝.
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
static LW_OBJECT_HANDLE             _G_hI2cListLock = LW_OBJECT_HANDLE_INVALID;
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define __I2C_LIST_LOCK()           API_SemaphoreBPend(_G_hI2cListLock, LW_OPTION_WAIT_INFINITE)
#define __I2C_LIST_UNLOCK()         API_SemaphoreBPost(_G_hI2cListLock)
/*********************************************************************************************************
** 函数名称: __i2cAdapterFind
** 功能描述: 查询 i2c 适配器
** 输　入  : pcName        适配器名称
** 输　出  : 适配器指针
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_I2C_ADAPTER  __i2cAdapterFind (CPCHAR  pcName)
{
    REGISTER PLW_BUS_ADAPTER        pbusadapter;
    
    pbusadapter = __busAdapterGet(pcName);                              /*  查找总线层适配器            */

    return  ((PLW_I2C_ADAPTER)pbusadapter);                             /*  总线结构为 i2c 适配器首元素 */
}
/*********************************************************************************************************
** 函数名称: API_I2cLibInit
** 功能描述: 初始化 i2c 组件库
** 输　入  : NONE
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_I2cLibInit (VOID)
{
    if (_G_hI2cListLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hI2cListLock = API_SemaphoreBCreate("i2c_listlock", 
                                               LW_TRUE, LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                               LW_NULL);
    }
    
    if (_G_hI2cListLock) {
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_I2cAdapterCreate
** 功能描述: 创建一个 i2c 适配器
** 输　入  : pcName        适配器名称
**           pi2cfunc      操作函数组
**           ulTimeout     操作超时时间 (ticks)
**           iRetry        重试次数
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_I2cAdapterCreate (CPCHAR           pcName, 
                           PLW_I2C_FUNCS    pi2cfunc,
                           ULONG            ulTimeout,
                           INT              iRetry)
{
    REGISTER PLW_I2C_ADAPTER    pi2cadapter;

    if (!pcName || !pi2cfunc || (iRetry < 0)) {
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
    pi2cadapter = (PLW_I2C_ADAPTER)__SHEAP_ALLOC(sizeof(LW_I2C_ADAPTER));
    if (pi2cadapter == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    pi2cadapter->I2CADAPTER_pi2cfunc       = pi2cfunc;
    pi2cadapter->I2CADAPTER_ulTimeout      = ulTimeout;
    pi2cadapter->I2CADAPTER_iRetry         = iRetry;
    pi2cadapter->I2CADAPTER_plineDevHeader = LW_NULL;                   /*  目前还不存在 i2c 设备       */
    
    /*
     *  创建相关锁
     */
    pi2cadapter->I2CADAPTER_hBusLock = API_SemaphoreBCreate("i2c_buslock", 
                                                            LW_TRUE, 
                                                            LW_OPTION_WAIT_FIFO |
                                                            LW_OPTION_OBJECT_GLOBAL,
                                                            LW_NULL);
    if (pi2cadapter->I2CADAPTER_hBusLock == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pi2cadapter);
        return  (PX_ERROR);
    }
    
    /*
     *  加入总线层
     */
    if (__busAdapterCreate(&pi2cadapter->I2CADAPTER_pbusadapter, pcName) != ERROR_NONE) {
        API_SemaphoreBDelete(&pi2cadapter->I2CADAPTER_hBusLock);
        __SHEAP_FREE(pi2cadapter);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_I2cAdapterDelete
** 功能描述: 移除一个 i2c 适配器
** 输　入  : pcName        适配器名称
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : 在内核中的驱动必须确保永不再使用这个适配器了, 才能删除这个适配器.
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_I2cAdapterDelete (CPCHAR  pcName)
{
    REGISTER PLW_I2C_ADAPTER        pi2cadapter = __i2cAdapterFind(pcName);
    
    if (pi2cadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }
    
    /*
     *  程序从查找设备, 到运行到这里, 并没有任何保护!
     */
    if (API_SemaphoreBPend(pi2cadapter->I2CADAPTER_hBusLock, 
                           LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    if (pi2cadapter->I2CADAPTER_plineDevHeader) {                       /*  检查是否有设备链接到适配器  */
        API_SemaphoreBPost(pi2cadapter->I2CADAPTER_hBusLock);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    
    if (__busAdapterDelete(pcName) != ERROR_NONE) {                     /*  从总线层移除                */
        API_SemaphoreBPost(pi2cadapter->I2CADAPTER_hBusLock);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    
    API_SemaphoreBDelete(&pi2cadapter->I2CADAPTER_hBusLock);            /*  删除总线锁信号量            */
    __SHEAP_FREE(pi2cadapter);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_I2cAdapterGet
** 功能描述: 通过名字获取一个 i2c 适配器
** 输　入  : pcName        适配器名称
** 输　出  : i2c 适配器控制块
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PLW_I2C_ADAPTER  API_I2cAdapterGet (CPCHAR  pcName)
{
    REGISTER PLW_I2C_ADAPTER        pi2cadapter = __i2cAdapterFind(pcName);
    
    if (pi2cadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (LW_NULL);
    
    } else {
        return  (pi2cadapter);
    }
}
/*********************************************************************************************************
** 函数名称: API_I2cDeviceCreate
** 功能描述: 在指定 i2c 适配器上, 创建一个 i2c 设备
** 输　入  : pcAdapterName 适配器名称
**           pcDeviceName  设备名称
**           usAddr        设备地址
**           usFlag        设备标志
** 输　出  : i2c 设备控制块
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PLW_I2C_DEVICE  API_I2cDeviceCreate (CPCHAR  pcAdapterName,
                                     CPCHAR  pcDeviceName,
                                     UINT16  usAddr,
                                     UINT16  usFlag)
{
    REGISTER PLW_I2C_ADAPTER    pi2cadapter = __i2cAdapterFind(pcAdapterName);
    REGISTER PLW_I2C_DEVICE     pi2cdevice;
    
    if (pi2cadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (LW_NULL);
    }
    
    if (_Object_Name_Invalid(pcDeviceName)) {                           /*  检查名字有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_NULL);
    }
    
    pi2cdevice = (PLW_I2C_DEVICE)__SHEAP_ALLOC(sizeof(LW_I2C_DEVICE));
    if (pi2cdevice == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    
    pi2cdevice->I2CDEV_usAddr      = usAddr;
    pi2cdevice->I2CDEV_usFlag      = usFlag;
    pi2cdevice->I2CDEV_pi2cadapter = pi2cadapter;
    pi2cdevice->I2CDEV_atomicUsageCnt.counter = 0;                      /*  目前还没有被使用过          */
    lib_strcpy(pi2cdevice->I2CDEV_cName, pcDeviceName);
    
    __I2C_LIST_LOCK();                                                  /*  锁定 i2c 链表               */
    _List_Line_Add_Ahead(&pi2cdevice->I2CDEV_lineManage, 
                         &pi2cadapter->I2CADAPTER_plineDevHeader);      /*  链入适配器设备链表          */
    __I2C_LIST_UNLOCK();                                                /*  解锁 i2c 链表               */
    
    LW_BUS_INC_DEV_COUNT(&pi2cadapter->I2CADAPTER_pbusadapter);         /*  总线设备++                  */
    
    return  (pi2cdevice);
}
/*********************************************************************************************************
** 函数名称: API_I2cDeviceDelete
** 功能描述: 删除指定的 i2c 设备
** 输　入  : pi2cdevice        指定的 i2c 设备控制块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_I2cDeviceDelete (PLW_I2C_DEVICE   pi2cdevice)
{
    REGISTER PLW_I2C_ADAPTER        pi2cadapter;

    if (pi2cdevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pi2cadapter = pi2cdevice->I2CDEV_pi2cadapter;
    if (pi2cadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }
    
    if (API_AtomicGet(&pi2cdevice->I2CDEV_atomicUsageCnt)) {
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }
    
    __I2C_LIST_LOCK();                                                  /*  锁定 i2c 链表               */
    _List_Line_Del(&pi2cdevice->I2CDEV_lineManage, 
                   &pi2cadapter->I2CADAPTER_plineDevHeader);            /*  链入适配器设备链表          */
    __I2C_LIST_UNLOCK();                                                /*  解锁 i2c 链表               */
    
    LW_BUS_DEC_DEV_COUNT(&pi2cadapter->I2CADAPTER_pbusadapter);         /*  总线设备--                  */
    
    __SHEAP_FREE(pi2cdevice);                                           /*  释放内存                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_I2cDeviceUsageInc
** 功能描述: 将指定 i2c 设备使用计数++
** 输　入  : pi2cdevice        指定的 i2c 设备控制块
** 输　出  : 当前的使用计数值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_I2cDeviceUsageInc (PLW_I2C_DEVICE   pi2cdevice)
{
    if (pi2cdevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (API_AtomicInc(&pi2cdevice->I2CDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** 函数名称: API_I2cDeviceUsageDec
** 功能描述: 将指定 i2c 设备使用计数--
** 输　入  : pi2cdevice        指定的 i2c 设备控制块
** 输　出  : 当前的使用计数值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_I2cDeviceUsageDec (PLW_I2C_DEVICE   pi2cdevice)
{
    if (pi2cdevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (API_AtomicDec(&pi2cdevice->I2CDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** 函数名称: API_I2cDeviceUsageGet
** 功能描述: 获得指定 i2c 设备使用计数
** 输　入  : pi2cdevice        指定的 i2c 设备控制块
** 输　出  : 当前的使用计数值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_I2cDeviceUsageGet (PLW_I2C_DEVICE   pi2cdevice)
{
    if (pi2cdevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (API_AtomicGet(&pi2cdevice->I2CDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** 函数名称: API_I2cDeviceTransfer
** 功能描述: 使用指定 i2c 设备进行一次传输
** 输　入  : pi2cdevice        指定的 i2c 设备控制块
**           pi2cmsg           传输消息控制块组
**           iNum              控制消息组中消息的数量
** 输　出  : 操作的 pi2cmsg 数量, 错误返回 -1
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_I2cDeviceTransfer (PLW_I2C_DEVICE   pi2cdevice, 
                            PLW_I2C_MESSAGE  pi2cmsg,
                            INT              iNum)
{
    REGISTER PLW_I2C_ADAPTER        pi2cadapter;
             INT                    iRet;

    if (!pi2cdevice || !pi2cmsg || (iNum < 1)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pi2cadapter = pi2cdevice->I2CDEV_pi2cadapter;
    if (pi2cadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }
    
    
    if (pi2cadapter->I2CADAPTER_pi2cfunc->I2CFUNC_pfuncMasterXfer) {
        if (API_SemaphoreBPend(pi2cadapter->I2CADAPTER_hBusLock, 
                               LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {/*  锁定总线                    */
            return  (PX_ERROR);
        }
        iRet = pi2cadapter->I2CADAPTER_pi2cfunc->I2CFUNC_pfuncMasterXfer(pi2cadapter, pi2cmsg, iNum);
        API_SemaphoreBPost(pi2cadapter->I2CADAPTER_hBusLock);           /*  释放总线                    */
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_I2cDeviceMasterSend
** 功能描述: 使用指定 i2c 设备进行一次发送传输
** 输　入  : pi2cdevice        指定的 i2c 设备控制块
**           pcBuffer          发送缓冲
**           iCount            需要发送的数量
** 输　出  : 实际发送数量, 错误返回 -1
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_I2cDeviceMasterSend (PLW_I2C_DEVICE   pi2cdevice,
                              CPCHAR           pcBuffer,
                              INT              iCount)
{
    REGISTER PLW_I2C_ADAPTER        pi2cadapter;
             LW_I2C_MESSAGE         i2cmsg;
             INT                    iRet;
    
    if (!pi2cdevice || !pcBuffer || (iCount < 1)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pi2cadapter = pi2cdevice->I2CDEV_pi2cadapter;
    if (pi2cadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }
    
    if (pi2cadapter->I2CADAPTER_pi2cfunc->I2CFUNC_pfuncMasterXfer) {
        i2cmsg.I2CMSG_usAddr    = pi2cdevice->I2CDEV_usAddr;
        i2cmsg.I2CMSG_usFlag    = (UINT16)(pi2cdevice->I2CDEV_usFlag & LW_I2C_M_TEN);
        i2cmsg.I2CMSG_usLen     = (UINT16)iCount;
        i2cmsg.I2CMSG_pucBuffer = (UINT8 *)pcBuffer;

        if (API_SemaphoreBPend(pi2cadapter->I2CADAPTER_hBusLock, 
                               LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {/*  锁定总线                    */
            return  (PX_ERROR);
        }
        iRet = pi2cadapter->I2CADAPTER_pi2cfunc->I2CFUNC_pfuncMasterXfer(pi2cadapter, &i2cmsg, 1);
        API_SemaphoreBPost(pi2cadapter->I2CADAPTER_hBusLock);           /*  释放总线                    */
        
        iRet = (iRet == 1) ? (iCount) : (iRet);
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_I2cDeviceMasterRecv
** 功能描述: 使用指定 i2c 设备进行一次接收传输
** 输　入  : pi2cdevice        指定的 i2c 设备控制块
**           pcBuffer          接收缓冲
**           iCount            接收缓冲大小
** 输　出  : 实际接收数量, 错误返回 -1
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_I2cDeviceMasterRecv (PLW_I2C_DEVICE   pi2cdevice,
                              PCHAR            pcBuffer,
                              INT              iCount)
{
    REGISTER PLW_I2C_ADAPTER        pi2cadapter;
             LW_I2C_MESSAGE         i2cmsg;
             INT                    iRet;
             
    if (!pi2cdevice || !pcBuffer || (iCount < 1)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pi2cadapter = pi2cdevice->I2CDEV_pi2cadapter;
    if (pi2cadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }
    
    if (pi2cadapter->I2CADAPTER_pi2cfunc->I2CFUNC_pfuncMasterXfer) {
        i2cmsg.I2CMSG_usAddr    = pi2cdevice->I2CDEV_usAddr;
        i2cmsg.I2CMSG_usFlag    = (UINT16)(pi2cdevice->I2CDEV_usFlag & LW_I2C_M_TEN);
        i2cmsg.I2CMSG_usFlag    = (UINT16)(i2cmsg.I2CMSG_usFlag | LW_I2C_M_RD);
        i2cmsg.I2CMSG_usLen     = (UINT16)iCount;
        i2cmsg.I2CMSG_pucBuffer = (UINT8 *)pcBuffer;
        
        if (API_SemaphoreBPend(pi2cadapter->I2CADAPTER_hBusLock, 
                               LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {/*  锁定总线                    */
            return  (PX_ERROR);
        }
        iRet = pi2cadapter->I2CADAPTER_pi2cfunc->I2CFUNC_pfuncMasterXfer(pi2cadapter, &i2cmsg, 1);
        API_SemaphoreBPost(pi2cadapter->I2CADAPTER_hBusLock);           /*  释放总线                    */
        
        iRet = (iRet == 1) ? (iCount) : (iRet);
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_I2cDeviceCtl
** 功能描述: 指定 i2c 设备处理指定命令
** 输　入  : pi2cdevice        指定的 i2c 设备控制块
**           iCmd              命令编号
**           lArg              命令参数
** 输　出  : 命令执行结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_I2cDeviceCtl (PLW_I2C_DEVICE   pi2cdevice, INT  iCmd, LONG  lArg)
{
    REGISTER PLW_I2C_ADAPTER        pi2cadapter;

    if (!pi2cdevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pi2cadapter = pi2cdevice->I2CDEV_pi2cadapter;
    if (pi2cadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }
    
    if (pi2cadapter->I2CADAPTER_pi2cfunc->I2CFUNC_pfuncMasterCtl) {
        return  (pi2cadapter->I2CADAPTER_pi2cfunc->I2CFUNC_pfuncMasterCtl(pi2cadapter, iCmd, lArg));
    } else {
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
