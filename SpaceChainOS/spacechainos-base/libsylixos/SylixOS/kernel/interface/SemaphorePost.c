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
** 文   件   名: SemaphorePost.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 01 月 16 日
**
** 描        述: 各类型信号量兼容释放函数. 为了方便移植其他操作系统应用软件而编写.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_SemaphorePost
** 功能描述: 释放信号量
** 输　入  : 
**           ulId                   事件句柄
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_SEM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphorePost (LW_OBJECT_HANDLE  ulId)
{
    REGISTER ULONG      ulObjectClass;
    REGISTER ULONG      ulErrorCode;
    
    ulObjectClass = _ObjectGetClass(ulId);                              /*  获得信号量句柄的类型        */
    
    switch (ulObjectClass) {
    
#if LW_CFG_SEMB_EN > 0
    case _OBJECT_SEM_B:
        ulErrorCode = API_SemaphoreBPost(ulId);
        break;
#endif                                                                  /*  LW_CFG_SEMB_EN > 0          */

#if LW_CFG_SEMC_EN > 0
    case _OBJECT_SEM_C:
        ulErrorCode = API_SemaphoreCPost(ulId);
        break;
#endif                                                                  /*  LW_CFG_SEMC_EN > 0          */

#if LW_CFG_SEMM_EN > 0
    case _OBJECT_SEM_M:
        ulErrorCode = API_SemaphoreMPost(ulId);
        break;
#endif                                                                  /*  LW_CFG_SEMM_EN > 0          */

#if LW_CFG_SEMRW_EN > 0
    case _OBJECT_SEM_RW:
        ulErrorCode = API_SemaphoreRWPost(ulId);
        break;
#endif                                                                  /*  LW_CFG_SEMRW_EN > 0         */
    
    default:
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);                         /*  句柄类型错误                */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }

    return  (ulErrorCode);
}

#endif                                                                  /*  (LW_CFG_SEM_EN > 0) &&      */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
