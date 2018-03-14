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
** 文   件   名: RegionCreate.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 11 日
**
** 描        述: 建立一个内存可变分区

** BUG
2007.08.29  加入对非对齐地址的检查功能。
2008.01.13  加入 _DebugHandle() 功能.
2011.07.29  加入对象创建/销毁回调.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_RegionCreate
** 功能描述: 建立一个内存可变分区
** 输　入  : 
**           pcName                        名字
**           pvLowAddr                     内存分区起始地址
**           stRegionByteSize              内存分区大小
**           ulOption                      选项
**           pulId                         Id 号
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)

LW_API  
LW_OBJECT_HANDLE API_RegionCreate (CPCHAR             pcName,
                                   PVOID              pvLowAddr,
                                   size_t             stRegionByteSize,
                                   ULONG              ulOption,
                                   LW_OBJECT_ID      *pulId)
{
    REGISTER PLW_CLASS_HEAP     pheap;
    REGISTER ULONG              ulIdTemp;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pvLowAddr) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pvLowAddr invalidate.\r\n");
        _ErrorHandle(ERROR_REGION_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (!_Addresses_Is_Aligned(pvLowAddr)) {                            /*  检查地址是否对齐            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pvLowAddr is not aligned.\r\n");
        _ErrorHandle(ERROR_KERNEL_MEMORY);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (_Heap_ByteSize_Invalid(stRegionByteSize)) {                     /*  分段太小                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulRegionByteSize is too low.\r\n");
        _ErrorHandle(ERROR_REGION_SIZE);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
#endif

    if (_Object_Name_Invalid(pcName)) {                                 /*  检查名字有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    pheap = _HeapCreate(pvLowAddr, stRegionByteSize);
    
    if (!pheap) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is no ID to buide a region.\r\n");
        _ErrorHandle(ERROR_REGION_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (pcName) {                                                       /*  拷贝名字                    */
        lib_strcpy(pheap->HEAP_cHeapName, pcName);
    } else {
        pheap->HEAP_cHeapName[0] = PX_EOS;                              /*  清空名字                    */
    }
    
    ulIdTemp = _MakeObjectId(_OBJECT_REGION, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             pheap->HEAP_usIndex);                      /*  构建对象 id                 */
    
    if (pulId) {
        *pulId = ulIdTemp;
    }
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, ulOption);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "region \"%s\" has been create.\r\n", (pcName ? pcName : ""));
    
    return  (ulIdTemp);
}

#endif                                                                  /*  (LW_CFG_REGION_EN > 0)      */
                                                                        /*  (LW_CFG_MAX_REGIONS > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
