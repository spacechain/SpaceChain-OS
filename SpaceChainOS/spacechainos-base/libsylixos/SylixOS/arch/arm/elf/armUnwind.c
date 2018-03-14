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
** 文   件   名: armUnwind.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2011 年 08 月 16 日
**
** 描        述: 此段代码算法出自 android 源码.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#ifdef   LW_CFG_CPU_ARCH_ARM                                            /*  ARM 体系结构                */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_lib.h"
/*********************************************************************************************************
** 函数名称: dl_unwind_find_exidx
** 功能描述: 根据pc指针，获取pc所在的模块并返回该模块中的.ARM.exidx段地址
** 输　入  : pc         程序寄存器指针
**           pcount     .ARM.exidx段中的表项数目
**           pvVProc    进程控制卡
** 输　出  : 成功返回.ARM.exidx段地址，失败返回NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
_Unwind_Ptr dl_unwind_find_exidx (_Unwind_Ptr pc, int *pcount, PVOID *pvVProc)
{
    LW_LIST_RING      *pringTemp = LW_NULL;
    LW_LD_EXEC_MODULE *pmodTemp  = LW_NULL;
    LW_LD_VPROC       *pvproc    = (LW_LD_VPROC *)pvVProc;
    PVOID              pvAddr    = (PVOID)pc;

    *pcount = 0;

    if (LW_NULL == pvproc) {
        return  (LW_NULL);
    }

    LW_VP_LOCK(pvproc);

    pringTemp = pvproc->VP_ringModules;
    if (!pringTemp) {
        LW_VP_UNLOCK(pvproc);
        return  (LW_NULL);
    }

    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

        if ((pvAddr >= pmodTemp->EMOD_pvBaseAddr) &&
            (pvAddr < (PVOID)((PCHAR)pmodTemp->EMOD_pvBaseAddr + pmodTemp->EMOD_stLen))) {
                                                                        /*  判断pc指针是否在模块内      */
            *pcount = pmodTemp->EMOD_stARMExidxCount;
            LW_VP_UNLOCK(pvproc);
            return  ((_Unwind_Ptr)pmodTemp->EMOD_pvARMExidx);           /*  返回.ARM.exidx的地址        */
        }

        pringTemp = _list_ring_get_next(pringTemp);
    } while (pvproc->VP_ringModules && pringTemp != pvproc->VP_ringModules);

    LW_VP_UNLOCK(pvproc);

    *pcount = 0;
    return  (LW_NULL);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
