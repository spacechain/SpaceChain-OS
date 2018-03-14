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
** 文   件   名: TimerShow.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 06 月 03 日
**
** 描        述: 定时器的相关信息 (打印在终端上).
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0 && ((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0))
/*********************************************************************************************************
** 函数名称: API_TimerShow
** 功能描述: 显示定时器的相关信息
** 输　入  : ulId         定时器句柄
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
VOID    API_TimerShow (LW_OBJECT_HANDLE  ulId)
{
    REGISTER ULONG      ulError;
             BOOL       bTimerRunning;
             ULONG      ulOption;
             ULONG      ulCounter;
             ULONG      ulInterval;
             CHAR       cTimerName[LW_CFG_OBJECT_NAME_SIZE] = "";

    ulError = API_TimerStatus(ulId, &bTimerRunning, &ulOption, &ulCounter, &ulInterval);
    if (ulError) {
        return;                                                         /*  发生了错误                  */
    }
    API_TimerGetName(ulId, cTimerName);
    
    printf("timer show >>\n\n");
    printf("timer name    : %s\n",  cTimerName);
    printf("timer running : %s\n",  bTimerRunning ? "true" : "false");
    printf("timer option  : %lu\n", ulOption);
    printf("timer cnt     : %lu\n", ulCounter);
    printf("timer interval: %lu\n", ulInterval);
    
    printf("\n");
}

#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
                                                                        /*  LW_CFG_HTIMER_EN > 0        */
                                                                        /*  LW_CFG_ITIMER_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
