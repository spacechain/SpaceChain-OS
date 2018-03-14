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
** 文   件   名: ioLicense.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 09 月 25 日
**
** 描        述: 显示系统 IO 驱动程序许可证管理系统
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
** 函数名称: API_IoSetDrvLicense
** 功能描述: 设置指定驱动程序的许可证信息
** 输　入  : 
**           iDrvNum                       驱动程式号 (主设备号)
**           pcLicense                     许可证字符串
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
********************************************************************************************************/
LW_API  
INT   API_IoSetDrvLicense (INT  iDrvNum, PCHAR  pcLicense)
{
    REGISTER LW_DRV_LICENSE     *pdrvlic;
    
    if (iDrvNum >= LW_CFG_MAX_DRIVERS ||
        iDrvNum <  0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
        
    if (_S_deventryTbl[iDrvNum].DEVENTRY_bInUse == LW_FALSE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    pdrvlic = &_S_deventryTbl[iDrvNum].DEVENTRY_drvlicLicense;
    
    pdrvlic->DRVLIC_pcLicense = pcLicense;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_IoGetDrvLicense
** 功能描述: 获得指定驱动程序的许可证信息
** 输　入  : 
**           iDrvNum                       驱动程式号 (主设备号)
** 输　出  : 许可证字符串
** 全局变量: 
** 调用模块: 
                                           API 函数
********************************************************************************************************/
LW_API  
PCHAR   API_IoGetDrvLicense (INT  iDrvNum)
{
    REGISTER LW_DRV_LICENSE     *pdrvlic;
    
    if (iDrvNum >= LW_CFG_MAX_DRIVERS ||
        iDrvNum <  0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (LW_NULL);
    }
        
    if (_S_deventryTbl[iDrvNum].DEVENTRY_bInUse == LW_FALSE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (LW_NULL);
    }
    
    pdrvlic = &_S_deventryTbl[iDrvNum].DEVENTRY_drvlicLicense;
    
    return  (pdrvlic->DRVLIC_pcLicense);
}
/*********************************************************************************************************
** 函数名称: API_IoSetDrvAuthor
** 功能描述: 设置指定驱动程序的作者信息
** 输　入  : 
**           iDrvNum                       驱动程式号 (主设备号)
**           pcAuthor                      作者信息字符串
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
********************************************************************************************************/
LW_API  
INT   API_IoSetDrvAuthor (INT  iDrvNum, PCHAR  pcAuthor)
{
    REGISTER LW_DRV_LICENSE     *pdrvlic;
    
    if (iDrvNum >= LW_CFG_MAX_DRIVERS ||
        iDrvNum <  0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
        
    if (_S_deventryTbl[iDrvNum].DEVENTRY_bInUse == LW_FALSE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    pdrvlic = &_S_deventryTbl[iDrvNum].DEVENTRY_drvlicLicense;
    
    pdrvlic->DRVLIC_pcAuthor = pcAuthor;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_IoGetDrvAuthor
** 功能描述: 获得指定驱动程序的作者信息
** 输　入  : 
**           iDrvNum                       驱动程式号 (主设备号)
** 输　出  : 作者信息字符串
** 全局变量: 
** 调用模块: 
                                           API 函数
********************************************************************************************************/
LW_API  
PCHAR   API_IoGetDrvAuthor (INT  iDrvNum)
{
    REGISTER LW_DRV_LICENSE     *pdrvlic;
    
    if (iDrvNum >= LW_CFG_MAX_DRIVERS ||
        iDrvNum <  0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (LW_NULL);
    }
        
    if (_S_deventryTbl[iDrvNum].DEVENTRY_bInUse == LW_FALSE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (LW_NULL);
    }
    
    pdrvlic = &_S_deventryTbl[iDrvNum].DEVENTRY_drvlicLicense;
    
    return  (pdrvlic->DRVLIC_pcAuthor);
}
/*********************************************************************************************************
** 函数名称: API_IoSetDrvDescroption
** 功能描述: 设置指定驱动程序的描述信息
** 输　入  : 
**           iDrvNum                       驱动程式号 (主设备号)
**           pcDescription                 描述信息
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
********************************************************************************************************/
LW_API  
INT   API_IoSetDrvDescription (INT  iDrvNum, PCHAR  pcDescription)
{
    REGISTER LW_DRV_LICENSE     *pdrvlic;
    
    if (iDrvNum >= LW_CFG_MAX_DRIVERS ||
        iDrvNum <  0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
        
    if (_S_deventryTbl[iDrvNum].DEVENTRY_bInUse == LW_FALSE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    pdrvlic = &_S_deventryTbl[iDrvNum].DEVENTRY_drvlicLicense;
    
    pdrvlic->DRVLIC_pcDescription = pcDescription;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_IoGetDrvDescroption
** 功能描述: 获得指定驱动程序的描述信息
** 输　入  : 
**           iDrvNum                       驱动程式号 (主设备号)
** 输　出  : 描述信息
** 全局变量: 
** 调用模块: 
                                           API 函数
********************************************************************************************************/
LW_API  
PCHAR   API_IoGetDrvDescription (INT  iDrvNum)
{
    REGISTER LW_DRV_LICENSE     *pdrvlic;
    
    if (iDrvNum >= LW_CFG_MAX_DRIVERS ||
        iDrvNum <  0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (LW_NULL);
    }
        
    if (_S_deventryTbl[iDrvNum].DEVENTRY_bInUse == LW_FALSE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (LW_NULL);
    }
    
    pdrvlic = &_S_deventryTbl[iDrvNum].DEVENTRY_drvlicLicense;
    
    return  (pdrvlic->DRVLIC_pcDescription);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
