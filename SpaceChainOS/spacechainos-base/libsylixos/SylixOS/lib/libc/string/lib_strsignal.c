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
** 文   件   名: lib_strsignal.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 10 月 15 日
**
** 描        述: 库
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
** 函数名称: lib_strsignal
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PCHAR  lib_strsignal (INT  iSigNo)
{
    PCHAR   pcSignalName[] = { "",
        "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT",
        "SIGUNUSED", "SIGFPE", "SIGKILL", "SIGBUS", "SIGSEGV", "SIGUNUSED2", 
        "SIGPIPE", "SIGALRM", "SIGTERM", "SIGCNCL", "SIGSTOP", "SIGTSTP", 
        "SIGCONT", "SIGCHLD", "SIGTTIN", "SIGTTOU", "SIGIO", "SIGXCPU", 
        "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGINFO", "SIGUSR1", 
        "SIGUSR2", "SIG_32", "SIGPWR", "SIGSYS", "SIGURG", "SIG_36", 
        "SIG_37", "SIG_38", "SIG_39", "SIG_40", "SIG_41", "SIG_42", 
        "SIG_43", "SIG_44", "SIG_45", "SIG_46", "SIGSTKSHOW", "SIG_RT_48",
        "SIG_RT_49", "SIG_RT_50", "SIG_RT_51", "SIG_RT_52", "SIG_RT_53", "SIG_RT_54", 
        "SIG_RT_55", "SIG_RT_56", "SIG_RT_57", "SIG_RT_58", "SIG_RT_59", "SIG_RT_60", 
        "SIG_RT_61", "SIG_RT_62", "SIG_RT_63", 
    };
    
    if (1 <= (iSigNo) && (iSigNo) <= 63) {
        return  (pcSignalName[iSigNo]);
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
