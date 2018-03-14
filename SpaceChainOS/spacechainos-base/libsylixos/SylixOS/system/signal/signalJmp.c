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
** 文   件   名: signalJmp.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 10 月 23 日
**
** 描        述: posix signal jump lib. 
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "setjmp.h"
/*********************************************************************************************************
** 函数名称: __sigsetjmpSetup
** 功能描述: sigsetjmp() 内部信号屏蔽处理
** 输　入  : sigjmpEnv      sigjmp 上下文
**           iSaveSigs      是否保存信号屏蔽码
** 输　出  : 0 : 正常设置   其他值表示 siglongjmp 返回.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __sigsetjmpSetup (sigjmp_buf sigjmpEnv, INT iSaveSigs)
{
#if LW_CFG_SIGNAL_EN > 0
    sigjmpEnv[0].__mask_was_saved = iSaveSigs;
    
    if (iSaveSigs) {
        sigprocmask(SIG_BLOCK, LW_NULL, &sigjmpEnv[0].__saved_mask);
    }
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
}
/*********************************************************************************************************
** 函数名称: __siglongjmpSetup
** 功能描述: 带有信号屏蔽状态记录的跳转
** 输　入  : sigjmpEnv      sigjmp 上下文
**           iVal           返回值
** 输　出  : 不返回
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __siglongjmpSetup (sigjmp_buf sigjmpEnv, INT iVal)
{
#if LW_CFG_SIGNAL_EN > 0
    if (sigjmpEnv[0].__mask_was_saved) {
        sigprocmask(SIG_SETMASK, &sigjmpEnv[0].__saved_mask, (sigset_t *) NULL);
    }
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
