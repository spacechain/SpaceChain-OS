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
** 文   件   名: excLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 03 月 30 日
**
** 描        述: 系统异常处理内部函数库

** BUG
2007.06.25  将系统线程名字改为 t_??? 的形式。
2008.01.13  系统使用的消息数量可以配置 LW_CFG_MAX_EXCEMSGS.
2008.01.16  修改了消息的名称.
2010.09.18  _excJobAdd() NULL 函数不能加入 job 队列.
2012.03.12  sigthread 使用动态 attr
2013.07.14  修正注释.
2013.08.28  修正不合适的命名.
2013.12.01  不再使用消息队列, 使用内核提供的工作队列模型.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SIGNAL_EN > 0
#define MAX_EXC_MSG_NUM     (NSIG + LW_CFG_MAX_EXCEMSGS + LW_CFG_MAX_THREADS)
#else
#define MAX_EXC_MSG_NUM     LW_CFG_MAX_EXCEMSGS
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */

static LW_JOB_QUEUE         _G_jobqExc;
static LW_JOB_MSG           _G_jobmsgExc[MAX_EXC_MSG_NUM];
/*********************************************************************************************************
  INTERNAL FUNC
*********************************************************************************************************/
static VOID  _ExcThread(VOID);                                          /*  异常处理程序                */
/*********************************************************************************************************
** 函数名称: _excInit
** 功能描述: 初始化 异常处理 机制
** 输　入  : NONE
** 输　出  : 是否初始化成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _excInit (VOID)
{
    LW_CLASS_THREADATTR threadattr;

    if (_jobQueueInit(&_G_jobqExc, &_G_jobmsgExc[0], MAX_EXC_MSG_NUM, LW_FALSE)) {
        return  (PX_ERROR);
    }
    
    API_ThreadAttrBuild(&threadattr, 
                        LW_CFG_THREAD_SIG_STK_SIZE, 
                        LW_PRIO_T_EXCPT,                                /*  最高优先级                  */
                        (LW_OPTION_THREAD_STK_CHK | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL), 
                        LW_NULL);
                        
    _S_ulThreadExceId = API_ThreadCreate("t_except",
                                         (PTHREAD_START_ROUTINE)_ExcThread,
                                         &threadattr,
                                         LW_NULL);                      /*  建立异常处理线程            */
    if (!_S_ulThreadExceId) {
        _jobQueueFinit(&_G_jobqExc);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _excJobAdd
** 功能描述: 加入待处理异常消息
** 输　入  : pfunc                      函数指针
**           pvArg0                     函数参数
**           pvArg1                     函数参数
**           pvArg2                     函数参数
**           pvArg3                     函数参数
**           pvArg4                     函数参数
**           pvArg5                     函数参数
** 输　出  : 操作是否成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _excJobAdd (VOIDFUNCPTR  pfunc, 
                 PVOID        pvArg0,
                 PVOID        pvArg1,
                 PVOID        pvArg2,
                 PVOID        pvArg3,
                 PVOID        pvArg4,
                 PVOID        pvArg5)
{
    if (!pfunc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (_jobQueueAdd(&_G_jobqExc, pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5)) {
        _ErrorHandle(ERROR_EXCE_LOST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _excJobDel
** 功能描述: 删除待处理异常消息
** 输　入  : uiMatchArgNum              匹配参数个数
**           pfunc                      函数指针
**           pvArg0                     函数参数
**           pvArg1                     函数参数
**           pvArg2                     函数参数
**           pvArg3                     函数参数
**           pvArg4                     函数参数
**           pvArg5                     函数参数
** 输　出  : 操作是否成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _excJobDel (UINT         uiMatchArgNum,
                  VOIDFUNCPTR  pfunc, 
                  PVOID        pvArg0, 
                  PVOID        pvArg1, 
                  PVOID        pvArg2, 
                  PVOID        pvArg3, 
                  PVOID        pvArg4, 
                  PVOID        pvArg5)
{
    _jobQueueDel(&_G_jobqExc, uiMatchArgNum, pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
}
/*********************************************************************************************************
** 函数名称: _ExcThread
** 功能描述: 处理异常消息的线程
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _ExcThread (VOID)
{
    for (;;) {
        _jobQueueExec(&_G_jobqExc, LW_OPTION_WAIT_INFINITE);
    }
}
/*********************************************************************************************************
** 函数名称: _ExcGetLost
** 功能描述: 获得异常消息丢失的数量
** 输　入  : NONE
** 输　出  : 异常消息丢失的数量
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
size_t  _ExcGetLost (VOID)
{
    return  (_jobQueueLost(&_G_jobqExc));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
