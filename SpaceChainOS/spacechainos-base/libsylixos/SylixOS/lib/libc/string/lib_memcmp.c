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
** 文   件   名: lib_memcmp.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 库

** BUG:
2013.06.09  memcmp 需要使用 unsigned char 比较大小.
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: lib_memcmp
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT    lib_memcmp (CPVOID  pvMem1, CPVOID  pvMem2, size_t  stCount)
{
    REGISTER PUCHAR     pucMem1Reg = (PUCHAR)pvMem1;
    REGISTER PUCHAR     pucMem2Reg = (PUCHAR)pvMem2;
    REGISTER INT        i;
    
    for (i = 0; i < stCount; i++) {
        if (*pucMem1Reg != *pucMem2Reg) {
            break;
        }
        pucMem1Reg++;
        pucMem2Reg++;
    }
    if (i >= stCount) {
        return  (0);
    }
    
    if (*pucMem1Reg > *pucMem2Reg) {
        return  (1);
    } else {
        return  (-1);
    }
}
/*********************************************************************************************************
** 函数名称: lib_bcmp
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT    lib_bcmp (CPVOID  pvMem1, CPVOID  pvMem2, size_t  stCount)
{
    REGISTER PUCHAR      pucMem1Reg = (PUCHAR)pvMem1;
    REGISTER PUCHAR      pucMem2Reg = (PUCHAR)pvMem2;
    REGISTER INT        i;
    
    for (i = 0; i < stCount; i++) {
        if (*pucMem1Reg != *pucMem2Reg) {
            break;
        }
        pucMem1Reg++;
        pucMem2Reg++;
    }
    if (i >= stCount) {
        return  (0);
    }
    
    if (*pucMem1Reg > *pucMem2Reg) {
        return  (1);
    } else {
        return  (-1);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
