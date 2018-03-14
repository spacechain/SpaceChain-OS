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
** 文   件   名: EventSetShow.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 03 月 30 日
**
** 描        述: 显示事件集的相关信息 (打印在终端上).
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0 && LW_CFG_EVENTSET_EN > 0
/*********************************************************************************************************
** 函数名称: API_EventSetShow
** 功能描述: 显示事件集的相关信息
** 输　入  : ulId         事件集句柄
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
VOID    API_EventSetShow (LW_OBJECT_HANDLE  ulId)
{
    REGISTER ULONG      ulError;
             ULONG      ulEvent;
             CHAR       cEventSetName[LW_CFG_OBJECT_NAME_SIZE] = "";

    ulError = API_EventSetStatus(ulId, &ulEvent, LW_NULL);
    if (ulError) {
        return;                                                         /*  发生了错误                  */
    }
    API_EventSetGetName(ulId, cEventSetName);
    
    printf("event set show >>\n\n");
    printf("event set name  : %s\n", cEventSetName);
    printf("event set event : 0x%08lx\n", ulEvent);
    
    printf("\n");
}

#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
                                                                        /*  LW_CFG_EVENTSET_EN > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
