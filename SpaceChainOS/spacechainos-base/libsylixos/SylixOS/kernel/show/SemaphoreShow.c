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
** 文   件   名: SemaphoreShow.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 03 月 02 日
**
** 描        述: 显示指定的信号量信息, (打印到标准输出终端上)

** BUG
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2009.04.08  加入对 SMP 多核的支持.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0
#if (LW_CFG_SEM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
/*********************************************************************************************************
** 函数名称: API_SemaphoreShow
** 功能描述: 显示指定的信号量信息
** 输　入  : ulId         信号量句柄
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
VOID    API_SemaphoreShow (LW_OBJECT_HANDLE  ulId)
{
             INTREG                 iregInterLevel;
    REGISTER ULONG                  ulObjectClass;
    REGISTER ULONG                  ulErrorCode;
    REGISTER UINT16                 usIndex;
    REGISTER PLW_CLASS_EVENT        pevent;
             LW_CLASS_EVENT         event;
    
             BOOL                   bValue;
             ULONG                  ulValue;
             ULONG                  ulMaxValue;
             ULONG                  ulWValue;
             ULONG                  ulRWCnt;
             ULONG                  ulOption;
             ULONG                  ulThreadNum;
             
             PCHAR                  pcType;
             PCHAR                  pcWaitType;
             PCHAR                  pcValue;
              CHAR                  cOwner[LW_CFG_OBJECT_NAME_SIZE];
              
              CHAR                  cValueStr[32];
    
    ulObjectClass = _ObjectGetClass(ulId);                              /*  获得信号量句柄的类型        */
    usIndex       = _ObjectGetIndex(ulId);
    
    switch (ulObjectClass) {
    
#if LW_CFG_SEMB_EN > 0
    case _OBJECT_SEM_B:
        ulErrorCode = API_SemaphoreBStatus(ulId,
                                           &bValue,
                                           &ulOption,
                                           &ulThreadNum);
        pcType  = "BINARY";
        pcValue = (bValue) ? "FULL" : "EMPTY";
        break;
#endif                                                                  /*  LW_CFG_SEMB_EN > 0          */

#if LW_CFG_SEMC_EN > 0
    case _OBJECT_SEM_C:
        ulErrorCode = API_SemaphoreCStatusEx(ulId,
                                             &ulValue,
                                             &ulOption,
                                             &ulThreadNum,
                                             &ulMaxValue);
        if (ulMaxValue == 1) {
            pcType  = "BINARY(COUNTER)";
        } else {
            pcType  = "COUNTER";
        }
        pcValue = &cValueStr[0];
        sprintf(pcValue, "%lu", ulValue);
        break;
#endif                                                                  /*  LW_CFG_SEMC_EN > 0          */

#if LW_CFG_SEMM_EN > 0
    case _OBJECT_SEM_M:
        ulErrorCode = API_SemaphoreMStatus(ulId,
                                           &bValue,
                                           &ulOption,
                                           &ulThreadNum);
        pcType  = "MUTEX";
        pcValue = (bValue) ? "FULL" : "EMPTY";
        break;
#endif                                                                  /*  LW_CFG_SEMM_EN > 0          */

#if LW_CFG_SEMRW_EN > 0
    case _OBJECT_SEM_RW:
        ulErrorCode = API_SemaphoreRWStatus(ulId,
                                            &ulRWCnt,
                                            &ulValue,
                                            &ulWValue,
                                            &ulOption, LW_NULL);
        pcType      = "RW";
        ulThreadNum = ulValue + ulWValue;
        break;
#endif                                                                  /*  LW_CFG_SEMRW_EN > 0         */
    
    default:
        fprintf(stderr, "\nInvalid semaphore id: 0x%08lx\n", ulId);
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);                         /*  句柄类型错误                */
        return;
    }
    
    if (ulErrorCode != ERROR_NONE) {
        fprintf(stderr, "\nInvalid semaphore id: 0x%08lx\n", ulId);
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);                         /*  句柄类型错误                */
        return;
    }
    
    pevent = &_K_eventBuffer[usIndex];
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核                    */
    if (pevent->EVENT_ucType == LW_TYPE_EVENT_UNUSED) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        fprintf(stderr, "\nInvalid semaphore id: 0x%08lx\n", ulId);
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);                         /*  句柄类型错误                */
        return;
    }
    event = *pevent;                                                    /*  拷贝信息                    */
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核                    */
    
    pcWaitType = (event.EVENT_ulOption & LW_OPTION_WAIT_PRIORITY)
               ? "PRIORITY" : "FIFO";
    
    printf("Semaphore show >>\n\n");                                    /*  打印基本信息                */
    printf("%-20s: %-10s\n",    "Semaphore Name", event.EVENT_cEventName);
    printf("%-20s: 0x%-10lx\n", "Semaphore Id",   ulId);
    printf("%-20s: %-10s\n",    "Semaphore Type", pcType);
    printf("%-20s: %-10s\n",    "Thread Queuing", pcWaitType);
    printf("%-20s: %-10ld\n",   "Pended Threads", ulThreadNum);
    
    if (ulObjectClass == _OBJECT_SEM_M) {                               /*  互斥信号量                  */
        PCHAR       pcSafeMode;
        PCHAR       pcMethod;
        
        if (bValue == LW_FALSE) {
            API_ThreadGetName(((PLW_CLASS_TCB)(event.EVENT_pvTcbOwn))->TCB_ulId,
                              cOwner);
        } else {
            lib_strcpy(cOwner, "NO THREAD");
        }
        
        pcSafeMode = (event.EVENT_ulOption & LW_OPTION_DELETE_SAFE)
                   ? "SEM_DELETE_SAFE" : "SEM_INVERSION_SAFE";          /*  是否使用安全模式            */
        pcMethod   = (event.EVENT_ulOption & LW_OPTION_INHERIT_PRIORITY)
                   ? "INHERIT_PRIORITY" : "PRIORITY_CEILING";
                   
        printf("%-20s: %-10s\n",  "Owner", cOwner);                     /*  拥有者                      */
        printf("%-20s: 0x%lx\t%s\t%s\n", "Options", event.EVENT_ulOption, 
               pcSafeMode, pcMethod);
                      
        if (!(event.EVENT_ulOption & LW_OPTION_INHERIT_PRIORITY)) {
            printf("%-20s: %-10d\n", "Ceiling", 
                   event.EVENT_ucCeilingPriority);                      /*  天花板优先级                */
        }
        
    } else if (ulObjectClass == _OBJECT_SEM_RW) {                       /*  读写信号量                  */
        PCHAR       pcSafeMode;
        
        if (bValue == LW_FALSE) {
            API_ThreadGetName(((PLW_CLASS_TCB)(event.EVENT_pvTcbOwn))->TCB_ulId,
                              cOwner);
        } else {
            if (ulRWCnt) {
                lib_strcpy(cOwner, "READERS");
            } else {
                lib_strcpy(cOwner, "NO THREAD");
            }
        }
        
        pcSafeMode = (event.EVENT_ulOption & LW_OPTION_DELETE_SAFE)
                   ? "SEM_DELETE_SAFE" : "SEM_INVERSION_SAFE";          /*  是否使用安全模式            */
                   
        printf("%-20s: %-10s\n", "Owner", cOwner);                      /*  拥有者                      */
        printf("%-20s: 0x%lx\t%s\n", "Options", event.EVENT_ulOption, 
               pcSafeMode);
                                                                        /*  当前计数值                  */
        printf("%-20s: %lu\n", "Read/Write Counter", ulRWCnt);
        printf("%-20s: %lu\n", "Read Pend", ulValue);
        printf("%-20s: %lu\n", "Write Pend", ulWValue);
        
    } else {
        if (ulObjectClass == _OBJECT_SEM_C) {
            printf("%-20s: %-10lu\n",   "Semaphore Max Value", 
                   event.EVENT_ulMaxCounter);                           /*  最大计数值                  */
        }
        
        printf("%-20s: %-10s\n",   "Semaphore Value", pcValue);         /*  当前计数值                  */
    }
    
    printf("\n");
}

#endif                                                                  /*  (LW_CFG_SEM_EN > 0) &&      */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
