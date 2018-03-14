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
** 文   件   名: cdump.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 01 日
**
** 描        述: 系统/应用崩溃信息记录.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_CDUMP_EN > 0) && (LW_CFG_DEVICE_EN > 0)
/*********************************************************************************************************
** 函数名称: API_CrashDumpBuffer
** 功能描述: 重新定位系统/应用崩溃信息记录位置. (必须是内核能访问的地址)
** 输　入  : pvCdump           缓冲地址
**           stSize            缓冲大小
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_CrashDumpBuffer (PVOID  pvCdump, size_t  stSize)
{
    if (!pvCdump || (pvCdump == (PVOID)PX_ERROR) || (stSize < 512)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    _CrashDumpSet(pvCdump, stSize);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_CrashDumpSave
** 功能描述: 最近一次系统/应用崩溃信息保存入文件.
** 输　入  : pcLogFile         日志文件名
**           iFlag             open 第二个参数
**           iMode             open 第三个参数
**           bClear            成功保存后是否清空缓冲区
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_CrashDumpSave (CPCHAR  pcLogFile, INT  iFlag, INT  iMode, BOOL  bClear)
{
    PCHAR   pcCdump, pcBuffer;
    size_t  stSize, stLen;
    INT     iFd;
    
    pcCdump = (PCHAR)_CrashDumpGet(&stSize);
    if (!pcCdump || (pcCdump == (PCHAR)PX_ERROR) || (stSize < 512)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (((UINT8)pcCdump[0] != LW_CDUMP_MAGIC_0) ||
        ((UINT8)pcCdump[1] != LW_CDUMP_MAGIC_1) ||
        ((UINT8)pcCdump[2] != LW_CDUMP_MAGIC_2) ||
        ((UINT8)pcCdump[3] != LW_CDUMP_MAGIC_3)) {
        _ErrorHandle(EMSGSIZE);
        return  (PX_ERROR);
    }
    
    pcCdump[stSize - 1] = PX_EOS;
    
    iFd = open(pcLogFile, iFlag, iMode);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    pcBuffer = &pcCdump[4];
    stLen    = lib_strlen(pcBuffer);
    
    if (write(iFd, pcBuffer, stLen) != stLen) {
        close(iFd);
        return  (PX_ERROR);
    }
    
    close(iFd);
    
    if (bClear) {
        pcCdump[0] = 0;
        pcCdump[1] = 0;
        pcCdump[2] = 0;
        pcCdump[3] = 0;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_CrashDumpShow
** 功能描述: 显示最近一次系统/应用崩溃信息.
** 输　入  : iFd               打印文件描述符
**           bClear            是否清空缓冲区
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_CrashDumpShow (INT  iFd, BOOL  bClear)
{
    PCHAR   pcCdump, pcBuffer;
    size_t  stSize;
    
    pcCdump = (PCHAR)_CrashDumpGet(&stSize);
    if (!pcCdump || (pcCdump == (PCHAR)PX_ERROR) || (stSize < 512)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (((UINT8)pcCdump[0] != LW_CDUMP_MAGIC_0) ||
        ((UINT8)pcCdump[1] != LW_CDUMP_MAGIC_1) ||
        ((UINT8)pcCdump[2] != LW_CDUMP_MAGIC_2) ||
        ((UINT8)pcCdump[3] != LW_CDUMP_MAGIC_3)) {
        _ErrorHandle(EMSGSIZE);
        return  (PX_ERROR);
    }
    
    pcCdump[stSize - 1] = PX_EOS;
    pcBuffer            = &pcCdump[4];
    
    fdprintf(iFd, "%s", pcBuffer);
    
    if (bClear) {
        pcCdump[0] = 0;
        pcCdump[1] = 0;
        pcCdump[2] = 0;
        pcCdump[3] = 0;
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_CDUMP_EN > 0         */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
