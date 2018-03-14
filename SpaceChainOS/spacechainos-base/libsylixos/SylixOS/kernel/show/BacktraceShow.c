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
** 文   件   名: BacktraceShow.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 11 月 12 日
**
** 描        述: 显示调用栈信息.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0
/*********************************************************************************************************
  loader
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "dlfcn.h"
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  backtrace 函数声明
*********************************************************************************************************/
extern int backtrace(void **array, int size);
/*********************************************************************************************************
** 函数名称: API_BacktraceShow
** 功能描述: 显示调用栈信息.
** 输　入  : iFd       输出文件
**           iMaxDepth 最大堆栈深度
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
VOID  API_BacktraceShow (INT  iFd, INT  iMaxDepth)
{
#ifdef __GNUC__
#define SHOWSTACK_SIZE  100

    PVOID   pvFrame[SHOWSTACK_SIZE];
    INT     i, iCnt;
    
    if (iMaxDepth > SHOWSTACK_SIZE) {
        iMaxDepth = SHOWSTACK_SIZE;
    
    } else if (iMaxDepth < 0) {
        iMaxDepth = 1;
    }
    
    iCnt = backtrace(pvFrame, iMaxDepth);
    if (iCnt > 0) {
#if LW_CFG_MODULELOADER_EN > 0
        Dl_info         dlinfo;
        LW_LD_VPROC    *pvproc = vprocGetCur();
    
        for (i = 0; i < iCnt; i++) {
            if ((API_ModuleAddr(pvFrame[i], &dlinfo, pvproc) == ERROR_NONE) &&
                (dlinfo.dli_sname)) {
                if (iFd >= 0) {
                    fdprintf(iFd, "[%d] %p (%s+%zu)\n", iCnt - i, pvFrame[i], dlinfo.dli_sname,
                                                        ((size_t)pvFrame[i] - (size_t)dlinfo.dli_saddr));
                } else {
                    _DebugFormat(__PRINTMESSAGE_LEVEL, "[%d] %p (%s+%zu)\r\n",
                                 iCnt - i, pvFrame[i], dlinfo.dli_sname,
                                 ((size_t)pvFrame[i] - (size_t)dlinfo.dli_saddr));
                }
            
            } else {
                if (iFd >= 0) {
                    fdprintf(iFd, "[%d] %p (<unknown>)\n", iCnt - i, pvFrame[i]);
                
                } else {
                    _DebugFormat(__PRINTMESSAGE_LEVEL, "[%d] %p (<unknown>)\r\n", iCnt - i, pvFrame[i]);
                }
            }
        }
#else
        for (i = 0; i < iCnt; i++) {
            if (iFd >= 0) {
                fdprintf(iFd, "[%d] %p\n", iCnt - i, pvFrame[i]);
                
            } else {
                _DebugFormat(__PRINTMESSAGE_LEVEL, "[%d] %p\r\n", iCnt - i, pvFrame[i]);
            }
        }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    }
#endif                                                                  /*  !__GNUC__                   */
}
/*********************************************************************************************************
** 函数名称: API_BacktracePrint
** 功能描述: 打印调用栈信息到缓存.
** 输　入  : pvBuffer  缓存位置
**           stSize    缓存大小
**           iMaxDepth 最大堆栈深度
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
VOID  API_BacktracePrint (PVOID  pvBuffer, size_t  stSize, INT  iMaxDepth)
{
#ifdef __GNUC__
#define SHOWSTACK_SIZE  100

    PVOID   pvFrame[SHOWSTACK_SIZE];
    INT     i, iCnt;
    size_t  stOft = 0;
    
    if (iMaxDepth > SHOWSTACK_SIZE) {
        iMaxDepth = SHOWSTACK_SIZE;
    
    } else if (iMaxDepth < 0) {
        iMaxDepth = 1;
    }
    
    iCnt = backtrace(pvFrame, iMaxDepth);
    if (iCnt > 0) {
#if LW_CFG_MODULELOADER_EN > 0
        Dl_info         dlinfo;
        LW_LD_VPROC    *pvproc = vprocGetCur();
    
        for (i = 0; i < iCnt; i++) {
            if ((API_ModuleAddr(pvFrame[i], &dlinfo, pvproc) == ERROR_NONE) &&
                (dlinfo.dli_sname)) {
                stOft = bnprintf(pvBuffer, stSize, stOft, "[%d] %p (%s+%zu)\n", 
                                 iCnt - i, pvFrame[i], dlinfo.dli_sname,
                                 ((size_t)pvFrame[i] - (size_t)dlinfo.dli_saddr));
            
            } else {
                stOft = bnprintf(pvBuffer, stSize, stOft, "[%d] %p (<unknown>)\n", iCnt - i, pvFrame[i]);
            }
        }
#else
        for (i = 0; i < iCnt; i++) {
            stOft = bnprintf(pvBuffer, stSize, stOft, "[%d] %p\n", iCnt - i, pvFrame[i]);
        }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    }
#endif                                                                  /*  !__GNUC__                   */
}

#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
