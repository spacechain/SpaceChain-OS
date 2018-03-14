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
** 文   件   名: StackShow.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 04 月 01 日
**
** 描        述: 显示所有的线程的堆栈信息.

** BUG
2008.04.27  ulFree 和 ulUsed 显示反了.
2009.04.07  加入使用百分比项.
2009.04.11  加入 SMP 信息.
2012.12.11  tid 显示 7 位就可以了
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
  全局变量
*********************************************************************************************************/
static const CHAR   _G_cThreadStackInfoHdr[] = "\n\
       NAME        TID   PRI STK USE  STK FREE USED\n\
---------------- ------- --- -------- -------- ----\n";
static const CHAR   _G_cCPUStackInfoHdr[] = "\n\
CPU STK USE  STK FREE USED\n\
--- -------- -------- ----\n";
/*********************************************************************************************************
** 函数名称: API_StackShow
** 功能描述: 显示所有的线程的 CPU 占用率信息
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
VOID    API_StackShow (VOID)
{
    REGISTER INT              i;
    REGISTER PLW_CLASS_TCB    ptcb;
    
             size_t           stFreeByteSize;
             size_t           stUsedByteSize;
             ULONG            ulError;

    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return;
    }
    
    printf("thread stack usage show >>\n");
    printf(_G_cThreadStackInfoHdr);                                     /*  打印欢迎信息                */
    
    for (i = 0; i < LW_CFG_MAX_THREADS; i++) {
        ptcb = _K_ptcbTCBIdTable[i];                                    /*  获得 TCB 控制块             */
        if (ptcb == LW_NULL) {                                          /*  线程不存在                  */
            continue;
        }
        
        ulError = API_ThreadStackCheck(ptcb->TCB_ulId, 
                                       &stFreeByteSize,
                                       &stUsedByteSize,
                                       LW_NULL);
        if (ulError) {
            continue;                                                   /*  产生错误                    */
        }
        
        printf("%-16s %7lx %3d %8zd %8zd %3zd%%\n",
                      ptcb->TCB_cThreadName,                            /*  线程名                      */
                      ptcb->TCB_ulId,                                   /*  线程代码入口                */
                      ptcb->TCB_ucPriority,                             /*  优先级                      */
                      stUsedByteSize,
                      stFreeByteSize,
                      ((stUsedByteSize * 100) / (stUsedByteSize + stFreeByteSize)));
    }
    
    printf("\ninterrupt stack usage show >>\n");
    printf(_G_cCPUStackInfoHdr);                                        /*  打印欢迎信息                */
    
    LW_CPU_FOREACH (i) {                                                /*  打印所有 CPU 的中断栈情况   */
        API_InterStackCheck((ULONG)i,
                            &stFreeByteSize,
                            &stUsedByteSize);
        printf("%3d %8zd %8zd %3zd%%\n",
                      i, stUsedByteSize, stFreeByteSize, 
                      ((stUsedByteSize * 100) / (stUsedByteSize + stFreeByteSize)));
    }
}

#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
