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
** 文   件   名: SemaphorePend.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 01 月 16 日
**
** 描        述: 各类型信号量兼容等待函数. 为了方便移植其他操作系统应用软件而编写.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
ulTimeout 取值：
    
    LW_OPTION_NOT_WAIT                       不进行等待
    LW_OPTION_WAIT_A_TICK                    等待一个系统时钟
    LW_OPTION_WAIT_A_SECOND                  等待一秒
    LW_OPTION_WAIT_INFINITE                  永远等待，直到发生为止
    
注意：当 ulTimeout == LW_OPTION_NOT_WAIT 时 API_SemaphoreCPend 还是不同于 API_SemaphoreBTryPend

      API_SemaphoreBTryPend 可以在中断中使用，而 API_SemaphoreBPend 不行
    
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: API_SemaphorePend
** 功能描述: 等待信号量
** 输　入  : 
**           ulId            事件句柄
**           ulTimeout       等待时间
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_SEMCBM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphorePend (LW_OBJECT_HANDLE  ulId, ULONG  ulTimeout)
{
    REGISTER ULONG      ulObjectClass;
    REGISTER ULONG      ulErrorCode;
    
    ulObjectClass = _ObjectGetClass(ulId);                              /*  获得信号量句柄的类型        */
    
    switch (ulObjectClass) {
    
#if LW_CFG_SEMB_EN > 0
    case _OBJECT_SEM_B:
        ulErrorCode = API_SemaphoreBPend(ulId, ulTimeout);
        break;
#endif                                                                  /*  LW_CFG_SEMB_EN > 0          */

#if LW_CFG_SEMC_EN > 0
    case _OBJECT_SEM_C:
        ulErrorCode = API_SemaphoreCPend(ulId, ulTimeout);
        break;
#endif                                                                  /*  LW_CFG_SEMC_EN > 0          */

#if LW_CFG_SEMM_EN > 0
    case _OBJECT_SEM_M:
        ulErrorCode = API_SemaphoreMPend(ulId, ulTimeout);
        break;
#endif                                                                  /*  LW_CFG_SEMM_EN > 0          */
    
    default:
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);                         /*  句柄类型错误                */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    return  (ulErrorCode);
}

#endif                                                                  /*  (LW_CFG_SEMCBM_EN > 0) &&   */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
