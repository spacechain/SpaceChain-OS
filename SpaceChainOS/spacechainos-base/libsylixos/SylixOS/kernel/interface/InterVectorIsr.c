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
** 文   件   名: InterVectorIsr.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 02 日
**
** 描        述: 向量中断总服务

** BUG
2007.06.06  加入最多嵌套信息记录功能。
2010.08.03  使用 interrupt vector spinlock 作为多核锁.
2011.03.31  加入 vector queue 类型中断向量支持.
2013.07.19  加入核间中断处理分支.
2013.08.28  加入内核事件监视器.
2014.04.21  中断被处理后不在遍历后面的中断服务函数.
2014.05.09  加入对中断计数器的处理.
2014.05.09  提高 SMP 中断处理速度.
2016.04.14  支持 GJB7714 无返回值中断.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  中断测量回调
*********************************************************************************************************/
#if LW_CFG_INTER_MEASURE_HOOK_EN > 0

extern VOIDFUNCPTR  _K_pfuncInterVectorMeasureEnter;
extern VOIDFUNCPTR  _K_pfuncInterVectorMeasureExit;

#define INTER_VECTOR_MEASURE_ENTER(a, b, c, d)                  \
        if (_K_pfuncInterVectorMeasureEnter) {                  \
            LW_SOFUNC_PREPARE(_K_pfuncInterVectorMeasureEnter); \
            _K_pfuncInterVectorMeasureEnter(a, b, c, d);        \
        }
#define INTER_VECTOR_MEASURE_EXIT(a, b, c, d)                   \
        if (_K_pfuncInterVectorMeasureExit) {                   \
            LW_SOFUNC_PREPARE(_K_pfuncInterVectorMeasureExit);  \
            _K_pfuncInterVectorMeasureExit(a, b, c, d);         \
        }
#else

#define INTER_VECTOR_MEASURE_ENTER(a, b, c, d)
#define INTER_VECTOR_MEASURE_EXIT(a, b, c, d)
        
#endif                                                                  /*  LW_CFG_INTER_MEASURE_HOOK_EN*/
/*********************************************************************************************************
** 函数名称: API_InterVectorIsr
** 功能描述: 向量中断总服务
** 输　入  : ulVector                      中断向量号 (arch 层函数需要保证此参数正确)
** 输　出  : 中断返回值
** 全局变量: 
** 调用模块: 
** 注  意  : 这里并不处理中断嵌套, 他需要 arch 层移植函数支持.
                                           API 函数
*********************************************************************************************************/
LW_API
irqreturn_t  API_InterVectorIsr (ULONG  ulVector)
{
    PLW_CLASS_CPU       pcpu;
    PLW_LIST_LINE       plineTemp;
    PLW_CLASS_INTDESC   pidesc;
    PLW_CLASS_INTACT    piaction;
    irqreturn_t         irqret = LW_IRQ_NONE;
    
#if LW_CFG_INTER_MEASURE_HOOK_EN > 0
    struct timespec     tv;
#endif
           
    pcpu = LW_CPU_GET_CUR();                                            /*  中断处理程序中, 不会改变 CPU*/
    
    __LW_CPU_INT_ENTER_HOOK(ulVector, pcpu->CPU_ulInterNesting);
    
#if LW_CFG_SMP_EN > 0
    if (pcpu->CPU_ulIPIVector == ulVector) {                            /*  核间中断                    */
        _SmpProcIpi(pcpu);
        if (pcpu->CPU_pfuncIPIClear) {
            pcpu->CPU_pfuncIPIClear(pcpu->CPU_pvIPIArg, ulVector);      /*  清除核间中断                */
        }
    } else
#endif                                                                  /*  LW_CFG_SMP_EN               */
    {
        pidesc = LW_IVEC_GET_IDESC(ulVector);
        
#if LW_CFG_SMP_EN > 0
        LW_SPIN_LOCK(&pidesc->IDESC_slLock);                            /*  锁住 spinlock               */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
        
        for (plineTemp  = pidesc->IDESC_plineAction;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            piaction = _LIST_ENTRY(plineTemp, LW_CLASS_INTACT, IACT_plineManage);
            INTER_VECTOR_MEASURE_ENTER(&tv, ulVector, pcpu->CPU_ulCPUId, piaction->IACT_cInterName);
            
#if LW_CFG_GJB7714_EN > 0
            if (pidesc->IDESC_ulFlag & LW_IRQ_FLAG_GJB7714) {
                piaction->IACT_pfuncIsr(piaction->IACT_pvArg, ulVector);
                irqret = LW_IRQ_NONE;
            } else
#endif
            {
                irqret = piaction->IACT_pfuncIsr(piaction->IACT_pvArg, ulVector);
            }
            
            if (LW_IRQ_RETVAL(irqret)) {                                /*  中断是否已经被处理          */
                INTER_VECTOR_MEASURE_EXIT(&tv, ulVector, pcpu->CPU_ulCPUId, piaction->IACT_cInterName);
                
                piaction->IACT_iIntCnt[pcpu->CPU_ulCPUId]++;
                if (piaction->IACT_pfuncClear) {
                    piaction->IACT_pfuncClear(piaction->IACT_pvArg, ulVector);
                }
                break;
            }
        }
        
#if LW_CFG_SMP_EN > 0
        LW_SPIN_UNLOCK(&pidesc->IDESC_slLock);                          /*  解锁 spinlock               */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    }
    
    __LW_CPU_INT_EXIT_HOOK(ulVector, pcpu->CPU_ulInterNesting);
                      
    return  (irqret);
}
/*********************************************************************************************************
** 函数名称: API_InterVectorIpiEx
** 功能描述: 设置核间中断向量   
             BSP 在系统启动前调用此函数, 每个 CPU 都要设置, SylixOS 允许不同的 CPU 核间中断向量不同.
** 输　入  : 
**           ulCPUId                       CPU ID
**           ulIPIVector                   核间中断向量号
**           pfuncClear                    核间中断清除函数
**           pvArg                         核间中断参数
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

LW_API
VOID  API_InterVectorIpiEx (ULONG  ulCPUId, ULONG  ulIPIVector, FUNCPTR  pfuncClear, PVOID  pvArg)
{
    PLW_CLASS_CPU   pcpu;
    
    if (ulCPUId < LW_CFG_MAX_PROCESSORS) {
        pcpu = LW_CPU_GET(ulCPUId);
        pcpu->CPU_ulIPIVector   = ulIPIVector;
        pcpu->CPU_pfuncIPIClear = pfuncClear;
        pcpu->CPU_pvIPIArg      = pvArg;
    }
}
/*********************************************************************************************************
** 函数名称: API_InterVectorIpi
** 功能描述: 设置核间中断向量   
             BSP 在系统启动前调用此函数, 每个 CPU 都要设置, SylixOS 允许不同的 CPU 核间中断向量不同.
** 输　入  : 
**           ulCPUId                       CPU ID
**           ulIPIVector                   核间中断向量号
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
VOID  API_InterVectorIpi (ULONG  ulCPUId, ULONG  ulIPIVector)
{
    API_InterVectorIpiEx(ulCPUId, ulIPIVector, LW_NULL, LW_NULL);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
