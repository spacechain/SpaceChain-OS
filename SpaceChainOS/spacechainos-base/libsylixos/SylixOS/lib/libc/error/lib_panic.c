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
** 文   件   名: lib_panic.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 03 月 02 日
**
** 描        述: panic()
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
** 函数名称: panic
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
void  panic (const char  *pcFormat, ...)
{
    va_list     arglist;
    char        cName[LW_CFG_OBJECT_NAME_SIZE] = "";

    API_ThreadGetName(API_ThreadIdSelf(), cName);

    va_start(arglist, pcFormat);
    
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    perror("system panic");
    vfprintf(stderr, pcFormat, arglist);
    fprintf(stderr, "\n"
                    "thread name: %s\n"
                    "thread id:   %lx\n"
                    "kernel time: %8lx\n",
                    cName, 
                    API_ThreadIdSelf(),
                    API_TimeGet());
    fflush(stdout);
    fflush(stderr);
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_FIO_LIB_EN > 0)     */

#if LW_CFG_PANIC_FUNC > 0
    API_ThreadSuspend(API_ThreadIdSelf());
#else
    API_KernelReboot(LW_REBOOT_WARM);                                   /*  系统重新启动                */
#endif                                                                  /*  LW_CFG_PANIC_FUNC > 0       */

    va_end(arglist);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
