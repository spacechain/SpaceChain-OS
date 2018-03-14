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
** 文   件   名: RegionShow.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 04 月 01 日
**
** 描        述: 显示指定的内存池信息.

** BUG:
2009.04.04  加入使用量比例信息.
2009.11.13  更新显示格式. 与 vmm 相仿.
2014.05.27  修复 heap 内存超过 40MB 显示比例关系溢出问题.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if (LW_CFG_FIO_LIB_EN > 0) && (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static const CHAR   _G_cHeapInfoHdr[] = "\n\
     HEAP         TOTAL      USED     MAX USED  SEGMENT USED\n\
-------------- ---------- ---------- ---------- ------- ----\n";
/*********************************************************************************************************
** 函数名称: API_RegionShow
** 功能描述: 显示指定的内存池信息
** 输　入  : ulId      内存池句柄 0: 表示显示内核堆和系统堆信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
VOID   API_RegionShow (LW_OBJECT_HANDLE  ulId)
{
    CHAR        cRegionName[LW_CFG_OBJECT_NAME_SIZE];

    size_t      stByteSize;
    ULONG       ulSegmentCounter;
    size_t      stUsedByteSize;
    size_t      stMaxUsedByteSize;
    
    printf("heap show >>\n");
    printf(_G_cHeapInfoHdr);                                            /*  打印欢迎信息                */
    
    if (ulId == 0) {
        API_KernelHeapInfo(LW_OPTION_HEAP_KERNEL, 
                           &stByteSize,
                           &ulSegmentCounter,
                           &stUsedByteSize,
                           LW_NULL,
                           &stMaxUsedByteSize);
        
        if (API_KernelHeapInfo(LW_OPTION_HEAP_SYSTEM, LW_NULL, LW_NULL, 
                               LW_NULL, LW_NULL, LW_NULL)) {            /*  检测是否包含 system 堆      */
            printf("%-14s %8zuKB %8zuKB %8zuKB %7ld %3zd%%\n", "kersys",
                   stByteSize / LW_CFG_KB_SIZE, 
                   stUsedByteSize  / LW_CFG_KB_SIZE, 
                   stMaxUsedByteSize / LW_CFG_KB_SIZE,
                   ulSegmentCounter, (stUsedByteSize / (stByteSize / 100)));
        
        } else {
            printf("%-14s %8zuKB %8zuKB %8zuKB %7ld %3zd%%\n", "kernel",
                   stByteSize / LW_CFG_KB_SIZE, 
                   stUsedByteSize  / LW_CFG_KB_SIZE, 
                   stMaxUsedByteSize / LW_CFG_KB_SIZE,
                   ulSegmentCounter, (stUsedByteSize / (stByteSize / 100)));
                   
            API_KernelHeapInfo(LW_OPTION_HEAP_SYSTEM, 
                               &stByteSize,
                               &ulSegmentCounter,
                               &stUsedByteSize,
                               LW_NULL,
                               &stMaxUsedByteSize);
                               
            printf("%-14s %8zuKB %8zuKB %8zuKB %7ld %3zd%%\n", "system",
                   stByteSize / LW_CFG_KB_SIZE, 
                   stUsedByteSize  / LW_CFG_KB_SIZE, 
                   stMaxUsedByteSize / LW_CFG_KB_SIZE,
                   ulSegmentCounter, (stUsedByteSize / (stByteSize / 100)));
        }
    
    } else {
        if (API_RegionGetName(ulId, cRegionName)) {
            return;
        }
        if (API_RegionStatus(ulId, 
                             &stByteSize,
                             &ulSegmentCounter,
                             &stUsedByteSize,
                             LW_NULL,
                             &stMaxUsedByteSize)) {
            return;
        }
        printf("%-14s %8zuKB %8zuKB %8zuKB %7ld %3zd%%\n", cRegionName,
               stByteSize / LW_CFG_KB_SIZE, 
               stUsedByteSize  / LW_CFG_KB_SIZE, 
               stMaxUsedByteSize / LW_CFG_KB_SIZE,
               ulSegmentCounter, (stUsedByteSize / (stByteSize / 100)));
    }
    
    printf("\n");
}

#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
                                                                        /*  LW_CFG_REGION_EN > 0        */
                                                                        /*  LW_CFG_MAX_REGIONS > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
