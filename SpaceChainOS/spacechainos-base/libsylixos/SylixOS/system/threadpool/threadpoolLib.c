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
** 文   件   名: threadpoolLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 03 月 20 日
**
** 描        述: 系统线程池功能内部函数库

** BUG
2008.03.09  修改代码格式.
2009.04.09  使用资源顺序分配.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
** 函数名称: _ThreadPoolInit
** 功能描述: 初始化 ThreadPool
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_THREAD_POOL_EN > 0 && LW_CFG_MAX_THREAD_POOLS > 0

VOID  _ThreadPoolInit (VOID)
{
#if  LW_CFG_MAX_THREAD_POOLS == 1

    REGISTER PLW_CLASS_THREADPOOL     pthreadpoolTemp1;
    
    _S_resrcThreadPool.RESRC_pmonoFreeHeader = &_S_threadpoolBuffer[0].TPCB_monoResrcList;
    
    pthreadpoolTemp1 = &_S_threadpoolBuffer[0];
    
    pthreadpoolTemp1->TPCB_usIndex = 0;
    
    _INIT_LIST_MONO_HEAD(_S_resrcThreadPool.RESRC_pmonoFreeHeader);
    
    _S_resrcThreadPool.RESRC_pmonoFreeTail = _S_resrcThreadPool.RESRC_pmonoFreeHeader;
    
#else 

    REGISTER ULONG                  ulI;
    REGISTER PLW_LIST_MONO          pmonoTemp1;
    REGISTER PLW_LIST_MONO          pmonoTemp2;
    REGISTER PLW_CLASS_THREADPOOL   pthreadpoolTemp1;
    REGISTER PLW_CLASS_THREADPOOL   pthreadpoolTemp2;
    
    _S_resrcThreadPool.RESRC_pmonoFreeHeader = &_S_threadpoolBuffer[0].TPCB_monoResrcList;
    
    pthreadpoolTemp1 = &_S_threadpoolBuffer[0];
    pthreadpoolTemp2 = &_S_threadpoolBuffer[1];
    
    for (ulI = 0; ulI < ((LW_CFG_MAX_THREAD_POOLS) - 1); ulI++) {
        pthreadpoolTemp1->TPCB_usIndex = (UINT16)ulI;
        
        pmonoTemp1 = &pthreadpoolTemp1->TPCB_monoResrcList;
        pmonoTemp2 = &pthreadpoolTemp2->TPCB_monoResrcList;
        
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);
        
        pthreadpoolTemp1++;
        pthreadpoolTemp2++;
    }
    
    pthreadpoolTemp1->TPCB_usIndex = (UINT16)ulI;
    
    pmonoTemp1 = &pthreadpoolTemp1->TPCB_monoResrcList;
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);
    
    _S_resrcThreadPool.RESRC_pmonoFreeTail = pmonoTemp1;
    
#endif                                                                  /*  LW_CFG_MAX_THREAD_POOLS == 1*/

    _S_resrcThreadPool.RESRC_uiUsed    = 0;
    _S_resrcThreadPool.RESRC_uiMaxUsed = 0;
}
/*********************************************************************************************************
** 函数名称: _Allocate_ThreadPool_Object
** 功能描述: 从空闲 ThreadPool 控件池中取出一个空闲 ThreadPool
** 输　入  : 
** 输　出  : 获得的 ThreadPool 地址，失败返回 NULL
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_CLASS_THREADPOOL  _Allocate_ThreadPool_Object (VOID)
{
    REGISTER PLW_LIST_MONO         pmonoFree;
    REGISTER PLW_CLASS_THREADPOOL  pthreadpool;
    
    if (_LIST_MONO_IS_EMPTY(_S_resrcThreadPool.RESRC_pmonoFreeHeader)) {
        return  (LW_NULL);
    }
        
    pmonoFree   = _list_mono_allocate_seq(&_S_resrcThreadPool.RESRC_pmonoFreeHeader,
                                          &_S_resrcThreadPool.RESRC_pmonoFreeTail); 
                                                                        /*  获得资源                    */
                                          
    pthreadpool = _LIST_ENTRY(pmonoFree, LW_CLASS_THREADPOOL, TPCB_monoResrcList);
    
    _S_resrcThreadPool.RESRC_uiUsed++;
    if (_S_resrcThreadPool.RESRC_uiUsed > _S_resrcThreadPool.RESRC_uiMaxUsed) {
        _S_resrcThreadPool.RESRC_uiMaxUsed = _S_resrcThreadPool.RESRC_uiUsed;
    }

    return  (pthreadpool);
}
/*********************************************************************************************************
** 函数名称: _Free_ThreadPool_Object
** 功能描述: 将 ThreadPool 控制块交还缓冲池
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _Free_ThreadPool_Object (PLW_CLASS_THREADPOOL    pthreadpool)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &pthreadpool->TPCB_monoResrcList;
    
    _list_mono_free_seq(&_S_resrcThreadPool.RESRC_pmonoFreeHeader, 
                        &_S_resrcThreadPool.RESRC_pmonoFreeTail, 
                        pmonoFree);
                        
    _S_resrcThreadPool.RESRC_uiUsed--;
}

#endif                                                                  /*  LW_CFG_THREAD_POOL_EN > 0   */
                                                                        /*  LW_CFG_MAX_THREAD_POOLS > 0 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
