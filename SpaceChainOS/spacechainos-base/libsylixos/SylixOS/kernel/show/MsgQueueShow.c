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
** 文   件   名: MsgQueueShow.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 03 月 03 日
**
** 描        述: 显示指定的消息队列信息, (打印到标准输出终端上)
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0 && LW_CFG_MSGQUEUE_EN > 0
/*********************************************************************************************************
** 函数名称: API_MsgQueueShow
** 功能描述: 显示指定的消息队列信息
** 输　入  : ulId         消息队列句柄
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
VOID   API_MsgQueueShow (LW_OBJECT_HANDLE  ulId)
{
    REGISTER ULONG      ulErrorCode;
    
             ULONG      ulMaxMsgNum;
             ULONG      ulCounter;
             size_t     stMsgLen;
             ULONG      ulOption;
             ULONG      ulThreadBlockNum;
             size_t     stMaxMsgLen;
             
             CHAR       cMsgQueueName[LW_CFG_OBJECT_NAME_SIZE];

             PCHAR      pcWaitType;

    API_MsgQueueGetName(ulId, cMsgQueueName);                           /*  获得名称                    */
    
    ulErrorCode = API_MsgQueueStatusEx(ulId,
                                       &ulMaxMsgNum,
                                       &ulCounter,
                                       &stMsgLen,
                                       &ulOption,
                                       &ulThreadBlockNum,
                                       &stMaxMsgLen);                   /*  获得当前状态                */
    if (ulErrorCode) {
        fprintf(stderr, "\nInvalid MsgQueue id: 0x%08lx\n", ulId);
        return;                                                         /*  产生错误                    */
    }
    
    pcWaitType = (ulOption & LW_OPTION_WAIT_PRIORITY)
               ? "PRIORITY" : "FIFO";
    
    printf("MsgQueue show >>\n\n");                                     /*  打印基本信息                */
    printf("%-20s: %-10s\n",    "MsgQueue Name",        cMsgQueueName);
    printf("%-20s: 0x%-10lx\n", "MsgQueue Id",          ulId);
    printf("%-20s: %-10ld\n",   "MsgQueue Max Msgs",    ulMaxMsgNum);
    printf("%-20s: %-10ld\n",   "MsgQueue N Msgs",      ulCounter);
    printf("%-20s: %-10zd\n",   "MsgQueue Max Msg Len", stMaxMsgLen);
    printf("%-20s: %-10s\n",    "Thread Queuing",       pcWaitType);
    printf("%-20s: %-10ld\n",   "Pended Threads",       ulThreadBlockNum);
    
    printf("\n");
}

#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
                                                                        /*  LW_CFG_MSGQUEUE_EN > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
