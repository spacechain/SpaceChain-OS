/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: c6xBacktrace.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 03 月 17 日
**
** 描        述: c6x 体系构架堆栈回溯.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  Only GCC support now.
*********************************************************************************************************/
#ifdef   __GNUC__
#include "c6xBacktrace.h"
#if LW_CFG_MODULELOADER_EN > 0
#include "unistd.h"
#include "loader/include/loader_lib.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** 函数名称: getEndStack
** 功能描述: 获得堆栈结束地址
** 输　入  : NONE
** 输　出  : 堆栈结束地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PVOID  getEndStack (VOID)
{
    PLW_CLASS_TCB  ptcbCur;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    return  ((PVOID)ptcbCur->TCB_pstkStackTop);
}
/*********************************************************************************************************
** 函数名称: backtrace
** 功能描述: 获得当前任务调用栈
** 输　入  : array     获取数组
**           size      数组大小
** 输　出  : 获取的数目
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  backtrace (void **array, int size)
{
    INT                 iCnt     = 0;
    ULONG              *pulEnd   = getEndStack();                       /*  获得栈顶                    */
    ULONG              *pulBegin = (ULONG *)archStackPointerGet();      /*  获得当前栈指针              */
    ULONG               ulValue;
#if LW_CFG_MODULELOADER_EN > 0
    INT                 i;
    LW_LIST_RING       *pringTemp;
    LW_LD_VPROC        *pvproc;
    LW_LD_EXEC_MODULE  *pmodTemp;
    BOOL                bStart;
    LW_LD_EXEC_SEGMENT *psegment;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    extern CHAR         __text[];                                       /*  内核代码段开始              */
    extern CHAR         __etext[];                                      /*  内核代码段结束              */

    addr_t              ulFiterBase = (addr_t)array;                    /*  将会跳过 array 数组         */
    addr_t              ulFiterEnd  = (addr_t)array + size * sizeof(void *);

#if LW_CFG_MODULELOADER_EN > 0
    if (getpid() == 0) {                                                /*  内核线程                    */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
        while ((pulBegin <= pulEnd) && (iCnt < size)) {                 /*  分析栈                      */
            if (((addr_t)pulBegin >= ulFiterBase) && ((addr_t)pulBegin < ulFiterEnd)) {
                pulBegin = (ULONG *)ulFiterEnd;                         /*  跳过 array 数组             */
                continue;
            }

            ulValue = *pulBegin++;
            if ((ulValue >= (ULONG)__text) && (ulValue < (ULONG)__etext)) {
                array[iCnt++] = (VOID *)ulValue;                        /*  落在内核代码段              */
            }
        }

#if LW_CFG_MODULELOADER_EN > 0
    } else {
        pvproc = vprocGetCur();                                         /*  获得当前 vproc              */

        LW_VP_LOCK(pvproc);                                             /*  vproc 加锁                  */

        while ((pulBegin <= pulEnd) && (iCnt < size)) {                 /*  分析栈                      */
            if (((addr_t)pulBegin >= ulFiterBase) && ((addr_t)pulBegin < ulFiterEnd)) {
                pulBegin = (ULONG *)ulFiterEnd;                         /*  跳过 array 数组             */
                continue;
            }

            ulValue = *pulBegin++;

            if ((ulValue >= (ULONG)__text) && (ulValue < (ULONG)__etext)) {
                array[iCnt++] = (VOID *)ulValue;                        /*  落在内核代码段              */
                continue;
            }

            for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE; /*  遍历每一个 module           */
                 pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
                 pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {

                pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
                psegment = pmodTemp->EMOD_psegmentArry;                 /*  遍历 module 的每一个 segment*/
                for (i = 0; i < pmodTemp->EMOD_ulSegCount; i++, psegment++) {
                    if (psegment->ESEG_stLen == 0) {                    /*  segment 有效                */
                        continue;
                    }
                    if (psegment->ESEG_bCanExec) {                      /*  segment 能执行              */
                        if ((ulValue >= psegment->ESEG_ulAddr) &&
                            (ulValue < (psegment->ESEG_ulAddr + psegment->ESEG_stLen))) {
                            array[iCnt++] = (VOID *)ulValue;            /*  落在该 segment              */
                            goto    __next;                             /*  跳出两重 for 循环           */
                        }
                    }
                }
            }

__next:
            continue;
        }

        LW_VP_UNLOCK(pvproc);                                           /*  vproc 解锁                  */
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    return  (iCnt);
}

#endif                                                                  /*  __GNUC__                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
