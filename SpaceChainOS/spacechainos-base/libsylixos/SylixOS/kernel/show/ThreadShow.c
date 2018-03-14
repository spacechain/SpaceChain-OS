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
** 文   件   名: ThreadShow.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 09 月 26 日
**
** 描        述: 显示所有的线程的信息.

** BUG
2007.10.28  不再使用 tcb 管理链表. 使用 id 表.
2008.03.01  将 \r 字符去掉, 使用 tty 设备发送时, 可采用 OPT_CRMOD.
2008.03.05  加入看门狗定时器显示.
2008.04.14  加入线程运行 CPU 信息的现实.
2008.04.22  显示 errno 时使用十进制.
2009.07.28  延迟时间使用无符号数显示格式.
            线程状态使用字符串表示.
2011.02.22  加入等待信号类型.
2011.02.24  加入僵死状态等待.
2011.12.05  加入每个线程缺页中断数量显示.
2012.09.12  加入对线程是否使用 FPU 的显示.
2012.12.11  不再显示堆栈首地址和入口, 加入所属进程 ID 显示.
            加入显示扩展接口, 可以显示指定进程内的线程.
2014.05.05  加入 pend show 功能, 可以查看正在阻塞的线程信息.
2014.06.02  pend 类型的显示更加详尽.
2015.04.22  pend 事件更加详细.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0
/*********************************************************************************************************
  进程相关
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static const CHAR   _G_cThreadInfoHdr[] = "\n\
      NAME         TID    PID  PRI STAT  ERRNO     DELAY   PAGEFAILS FPU CPU\n\
---------------- ------- ----- --- ---- ------- ---------- --------- --- ---\n";
static const CHAR   _G_cThreadPendHdr[] = "\n\
      NAME         TID    PID  STAT    DELAY          PEND EVENT        OWNER\n\
---------------- ------- ----- ---- ---------- ----------------------- -------\n";
/*********************************************************************************************************
** 函数名称: API_ThreadShowEx
** 功能描述: 显示所有的线程的信息
** 输　入  : pid       需要显示对应进程内的线程, (-1 表示所有线程)
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
VOID    API_ThreadShowEx (pid_t  pid)
{
    REGISTER INT                i;
    REGISTER INT                iThreadCounter = 0;
    REGISTER PLW_CLASS_TCB      ptcb;
             LW_CLASS_TCB_DESC  tcbdesc;
             CHAR               cWakeupLeft[11];
             
             PCHAR              pcPendType = LW_NULL;
             PCHAR              pcFpu = LW_NULL;
             pid_t              pidGet;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return;
    }
    
    printf("thread show >>\n");
    printf(_G_cThreadInfoHdr);                                          /*  打印欢迎信息                */
    
    for (i = 0; i < LW_CFG_MAX_THREADS; i++) {
        ptcb = _K_ptcbTCBIdTable[i];                                    /*  获得 TCB 控制块             */
        if (ptcb == LW_NULL) {                                          /*  线程不存在                  */
            continue;
        }
        
        if (API_ThreadDesc(ptcb->TCB_ulId, &tcbdesc)) {
            continue;
        }
        
#if LW_CFG_MODULELOADER_EN > 0
        pidGet = vprocGetPidByTcbdesc(&tcbdesc);
#else
        pidGet = 0;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
        
        if ((pidGet != pid) && (pid != -1)) {
            continue;
        }
        
#if LW_CFG_CPU_WORD_LENGHT == 64
        if (tcbdesc.TCBD_ulWakeupLeft > 9999999999l) {
            lib_strcpy(cWakeupLeft, "--");
        } else 
#endif
        {
            lib_itoa((int)tcbdesc.TCBD_ulWakeupLeft, cWakeupLeft, 10);
        }
        
        if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_SEM) {             /*  等待信号量                  */
            pcPendType = "SEM";
        
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_MSGQUEUE) { /*  等待消息队列                */
            pcPendType = "MSGQ";
        
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_JOIN) {     /*  等待其他线程                */
            pcPendType = "JOIN";
        
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_SUSPEND) {  /*  挂起                        */
            pcPendType = "SUSP";
        
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_EVENTSET) { /*  等待事件组                  */
            pcPendType = "ENTS";
        
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_SIGNAL) {   /*  等待信号                    */
            pcPendType = "WSIG";
        
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_INIT) {     /*  初始化中                    */
            pcPendType = "INIT";
        
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_WDEATH) {   /*  僵死状态                    */
            pcPendType = "WDEA";
        
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_DELAY) {    /*  睡眠                        */
            pcPendType = "SLP";
            
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_STOP) {     /*  停止                        */
            pcPendType = "STOP";
            
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_WSTAT) {    /*  等待状态转换                */
            pcPendType = "WSTA";
        
        } else {
            pcPendType = "RDY";                                         /*  就绪态                      */
        }
        
        if (tcbdesc.TCBD_ulOption & LW_OPTION_THREAD_USED_FP) {
            pcFpu = "USE";
        } else {
            pcFpu = "";
        }
        
        printf("%-16s %7lx %5d %3d %-4s %7ld %10s %9llu %-3s %3ld\n",
               tcbdesc.TCBD_cThreadName,                                /*  线程名                      */
               tcbdesc.TCBD_ulId,                                       /*  ID 号                       */
               pidGet,                                                  /*  所属进程 ID                 */
               tcbdesc.TCBD_ucPriority,                                 /*  优先级                      */
               pcPendType,                                              /*  状态                        */
               tcbdesc.TCBD_ulLastError,                                /*  错误号                      */
               cWakeupLeft,                                             /*  等待计数器                  */
               tcbdesc.TCBD_i64PageFailCounter,                         /*  缺页中断                    */
               pcFpu,
               tcbdesc.TCBD_ulCPUId);
        iThreadCounter++;
    }
    
    printf("\nthread: %d\n", iThreadCounter);                           /*  显示线程数量                */
}
/*********************************************************************************************************
** 函数名称: API_ThreadPendShowEx
** 功能描述: 显示正在阻塞的线程的信息
** 输　入  : pid       需要显示对应进程内的线程, (-1 表示所有线程)
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
VOID    API_ThreadPendShowEx (pid_t  pid)
{
#define MAKE_ID(type, index)    \
        _MakeObjectId(type, LW_CFG_PROCESSOR_NUMBER, index)

    REGISTER INT                i;
    REGISTER INT                iThreadCounter = 0;
    REGISTER PLW_CLASS_TCB      ptcb;
             PLW_CLASS_TCB      ptcbOwner;
             LW_CLASS_TCB_DESC  tcbdesc;
             PLW_CLASS_EVENT    pevent;
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
             PLW_CLASS_EVENTSET pes;
#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
             CHAR               cWakeupLeft[11];
             PCHAR              pcPendType = LW_NULL;
             pid_t              pidGet;
             CHAR               cEventName[LW_CFG_OBJECT_NAME_SIZE + 2];
             LW_OBJECT_HANDLE   ulEvent;
             LW_OBJECT_HANDLE   ulOwner;
             
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return;
    }
    
    printf("thread pending show >>\n");
    printf(_G_cThreadPendHdr);                                          /*  打印欢迎信息                */
    
    for (i = 0; i < LW_CFG_MAX_THREADS; i++) {
        ptcb = _K_ptcbTCBIdTable[i];                                    /*  获得 TCB 控制块             */
        if (ptcb == LW_NULL) {                                          /*  线程不存在                  */
            continue;
        }
        
        __KERNEL_ENTER();                                               /*  锁定内核                    */
        if (ptcb->TCB_peventPtr) {                                      /*  等待事件                    */
            pevent = ptcb->TCB_peventPtr;
            
            lib_strlcpy(cEventName, pevent->EVENT_cEventName, LW_CFG_OBJECT_NAME_SIZE);
            
            switch (pevent->EVENT_ucType) {
            
            case LW_TYPE_EVENT_MSGQUEUE:
                ulEvent = MAKE_ID(_OBJECT_MSGQUEUE, pevent->EVENT_usIndex);
                if (ptcb->TCB_iPendQ == EVENT_MSG_Q_R) {
                    lib_strlcat(cEventName, ":R", LW_CFG_OBJECT_NAME_SIZE + 2);
                } else {
                    lib_strlcat(cEventName, ":S", LW_CFG_OBJECT_NAME_SIZE + 2);
                }
                ulOwner = LW_OBJECT_HANDLE_INVALID;
                break;
            
            case LW_TYPE_EVENT_SEMC:
                ulEvent = MAKE_ID(_OBJECT_SEM_C, pevent->EVENT_usIndex);
                ulOwner = LW_OBJECT_HANDLE_INVALID;
                break;
                
            case LW_TYPE_EVENT_SEMB:
                ulEvent = MAKE_ID(_OBJECT_SEM_B, pevent->EVENT_usIndex);
                ulOwner = LW_OBJECT_HANDLE_INVALID;
                break;
                
            case LW_TYPE_EVENT_SEMRW:
                ulEvent = MAKE_ID(_OBJECT_SEM_RW, pevent->EVENT_usIndex);
                if (ptcb->TCB_iPendQ == EVENT_RW_Q_R) {
                    lib_strlcat(cEventName, ":R", LW_CFG_OBJECT_NAME_SIZE + 2);
                } else {
                    lib_strlcat(cEventName, ":W", LW_CFG_OBJECT_NAME_SIZE + 2);
                }
                if (pevent->EVENT_pvTcbOwn) {
                    ptcbOwner = (PLW_CLASS_TCB)pevent->EVENT_pvTcbOwn;
                    ulOwner   = ptcbOwner->TCB_ulId;
                } else {
                    ulOwner   = LW_OBJECT_HANDLE_INVALID;
                }
                break;
            
            case LW_TYPE_EVENT_MUTEX:
                ulEvent = MAKE_ID(_OBJECT_SEM_M, pevent->EVENT_usIndex);
                if (pevent->EVENT_pvTcbOwn) {
                    ptcbOwner = (PLW_CLASS_TCB)pevent->EVENT_pvTcbOwn;
                    ulOwner   = ptcbOwner->TCB_ulId;
                } else {
                    ulOwner   = LW_OBJECT_HANDLE_INVALID;
                }
                break;
                
            default:
                ulEvent = LW_OBJECT_HANDLE_INVALID;
                ulOwner = LW_OBJECT_HANDLE_INVALID;
                break;
            }
            
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
        } else if (ptcb->TCB_pesnPtr) {                                 /*  等待事件标志组              */
            pes     = (PLW_CLASS_EVENTSET)ptcb->TCB_pesnPtr->EVENTSETNODE_pesEventSet;
            ulEvent = MAKE_ID(_OBJECT_EVENT_SET, pes->EVENTSET_usIndex);
            lib_strlcpy(cEventName, pes->EVENTSET_cEventSetName, LW_CFG_OBJECT_NAME_SIZE);
            ulOwner = LW_OBJECT_HANDLE_INVALID;
#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
        } else if (ptcb->TCB_ptcbJoin) {
            ulEvent = ptcb->TCB_ptcbJoin->TCB_ulId;
            lib_strlcpy(cEventName, ptcb->TCB_ptcbJoin->TCB_cThreadName, LW_CFG_OBJECT_NAME_SIZE);
            ulOwner = LW_OBJECT_HANDLE_INVALID;
        
        } else {
            cEventName[0] = PX_EOS;
            ulEvent       = LW_OBJECT_HANDLE_INVALID;
            ulOwner       = LW_OBJECT_HANDLE_INVALID;
        }
        __KERNEL_EXIT();                                                /*  解锁内核                    */
        
        if (API_ThreadDesc(ptcb->TCB_ulId, &tcbdesc)) {
            continue;
        }
        
#if LW_CFG_MODULELOADER_EN > 0
        pidGet = vprocGetPidByTcbdesc(&tcbdesc);
#else
        pidGet = 0;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
        
        if ((pidGet != pid) && (pid != -1)) {
            continue;
        }
        
#if LW_CFG_CPU_WORD_LENGHT == 64
        if (tcbdesc.TCBD_ulWakeupLeft > 9999999999l) {
            lib_strcpy(cWakeupLeft, "--");
        } else 
#endif
        {
            lib_itoa((int)tcbdesc.TCBD_ulWakeupLeft, cWakeupLeft, 10);
        }
        
        if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_SEM) {             /*  等待信号量                  */
            pcPendType = "SEM";
        
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_MSGQUEUE) { /*  等待消息队列                */
            pcPendType = "MSGQ";
        
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_JOIN) {     /*  等待其他线程                */
            pcPendType = "JOIN";
        
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_SUSPEND) {  /*  挂起                        */
            pcPendType = "SUSP";
        
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_EVENTSET) { /*  等待事件组                  */
            pcPendType = "ENTS";
        
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_SIGNAL) {   /*  等待信号                    */
            pcPendType = "WSIG";
        
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_INIT) {     /*  初始化中                    */
            pcPendType = "INIT";
        
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_WDEATH) {   /*  僵死状态                    */
            pcPendType = "WDEA";
        
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_DELAY) {    /*  睡眠                        */
            pcPendType = "SLP";
        
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_STOP) {     /*  停止                        */
            pcPendType = "STOP";
            
        } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_WSTAT) {    /*  等待状态转换                */
            pcPendType = "WSTA";
        
        } else {
            continue;                                                   /*  就绪态                      */
        }
        
        if (ulOwner) {
            printf("%-16s %7lx %5d %-4s %10s %8lx:%-14s %7lx\n",
                   tcbdesc.TCBD_cThreadName,                            /*  线程名                      */
                   tcbdesc.TCBD_ulId,                                   /*  ID 号                       */
                   pidGet,                                              /*  所属进程 ID                 */
                   pcPendType,                                          /*  状态                        */
                   cWakeupLeft,                                         /*  等待计数器                  */
                   ulEvent,
                   cEventName,
                   ulOwner);
        } else {
            printf("%-16s %7lx %5d %-4s %10s %8lx:%-14s\n",
                   tcbdesc.TCBD_cThreadName,                            /*  线程名                      */
                   tcbdesc.TCBD_ulId,                                   /*  ID 号                       */
                   pidGet,                                              /*  所属进程 ID                 */
                   pcPendType,                                          /*  状态                        */
                   cWakeupLeft,                                         /*  等待计数器                  */
                   ulEvent,
                   cEventName);
        }
        iThreadCounter++;
    }
    
    printf("\npending thread: %d\n", iThreadCounter);                   /*  显示线程数量                */
}
/*********************************************************************************************************
** 函数名称: API_ThreadShow
** 功能描述: 显示所有的线程的信息
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
VOID    API_ThreadShow (VOID)
{
    API_ThreadShowEx(PX_ERROR);                                         /*  显示所有线程                */
}
/*********************************************************************************************************
** 函数名称: API_ThreadPendShow
** 功能描述: 显示正在阻塞的线程的信息
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
VOID    API_ThreadPendShow (VOID)
{
    API_ThreadPendShowEx(PX_ERROR);                                     /*  显示所有线程                */
}

#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
