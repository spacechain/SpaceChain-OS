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
** 文   件   名: RmsShow.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 03 月 04 日
**
** 描        述: 打印操作系统精度单调调度器信息, (打印到标准输出终端上)
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0 && LW_CFG_RMS_EN > 0 && LW_CFG_MAX_RMSS > 0
/*********************************************************************************************************
** 函数名称: API_RmsShow
** 功能描述: 打印操作系统精度单调调度器信息
** 输　入  : ulId          精度单调调度器句柄
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
VOID  API_RmsShow (LW_OBJECT_HANDLE  ulId)
{
    REGISTER ULONG              ulError;
             UINT8              ucStatus;
             ULONG              ulTimeLeft;
             LW_OBJECT_HANDLE   ulOwnerId;
             
             CHAR               cName[LW_CFG_OBJECT_NAME_SIZE];
             PCHAR              pcStatus;
    
    ulError = API_RmsStatus(ulId,
                            &ucStatus,
                            &ulTimeLeft,
                            &ulOwnerId);
    if (ulError) {
        return;
    }
    
    ulError = API_RmsGetName(ulId, cName);
    if (ulError) {
        return;
    }
    
    switch (ucStatus) {
    
    case LW_RMS_INACTIVE:
        pcStatus = "INACTIVE";
        break;
        
    case LW_RMS_ACTIVE:
        pcStatus = "ACTIVE";
        break;
        
    case LW_RMS_EXPIRED:
        pcStatus = "EXPIRED";
        break;
        
    default:
        pcStatus = "UNKOWN STAT";
        break;
    }
    
    printf("Rate Monotonic Scheduler show >>\n\n");
    printf("Rate Monotonic Scheduler Name      : %s\n",      cName);
    printf("Rate Monotonic Scheduler Owner     : 0x%08lx\n", ulOwnerId);
    printf("Rate Monotonic Scheduler Status    : %s\n",      pcStatus);
    printf("Rate Monotonic Scheduler Time Left : %lu\n",     ulTimeLeft);
    
    printf("\n");
}

#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
                                                                        /*  LW_CFG_RMS_EN > 0           */
                                                                        /*  LW_CFG_MAX_RMSS > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
