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
** 文   件   名: lib_memset.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 库

** BUG:
2011.06.22  当 iCount 小于 0 时, 不处理.
2013.03.29  memset iC 先转换为 uchar 类型.
2016.07.15  优化速度.
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  按字对齐方式拷贝
*********************************************************************************************************/
#define __LONGSIZE              sizeof(ULONG)
#define __LONGMASK              (__LONGSIZE - 1)

#define __BIGBLOCKSTEP          (16)
#define __BIGBLOCKSIZE          (__LONGSIZE << 4)

#define __LITLOCKSTEP           (1)
#define __LITLOCKSIZE           (__LONGSIZE)

#define __TLOOP(s)              if (ulTemp) {       \
                                    __TLOOP1(s);    \
                                }
#define __TLOOP1(s)             do {                \
                                    s;              \
                                } while (--ulTemp)
/*********************************************************************************************************
** 函数名称: lib_memset
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
** 注  意  : 
*********************************************************************************************************/
PVOID  lib_memset (PVOID  pvDest, INT  iC, size_t  stCount)
{
#ifdef __ARCH_MEMSET
    return  (__ARCH_MEMSET(pvDest, iC, stCount));
    
#else
    REGISTER INT       i;
    REGISTER ULONG     ulTemp;
    REGISTER PUCHAR    pucDest = (PUCHAR)pvDest;
    REGISTER ULONG    *pulDest;
    
             UCHAR     ucC     = (UCHAR)iC;
             ULONG     ulFill  = (ULONG)ucC;
             
    if (stCount == 0) {
        return  (pvDest);
    }
             
    for (i = 1; i < (__LONGSIZE / sizeof(UCHAR)); i++) {                /*  构建 ulong 对齐的赋值变量   */
        ulFill = (ulFill << 8) + ucC;
    }

    if ((ULONG)pucDest & __LONGMASK) {                                  /*  处理前端非对齐部分          */
        if (stCount < __LONGSIZE) {
            ulTemp = (ULONG)stCount;
        } else {
            ulTemp = (ULONG)(__LONGSIZE - ((ULONG)pucDest & __LONGMASK));
        }
        
        stCount -= (size_t)ulTemp;
        __TLOOP1(*pucDest++ = ucC);
    }
    
    /*
     *  按字对齐处理
     */
    ulTemp = (INT)(stCount / __LONGSIZE);
    
    pulDest = (ULONG *)pucDest;
    
    while (ulTemp >= __BIGBLOCKSTEP) {
        *pulDest++ = ulFill;
        *pulDest++ = ulFill;
        *pulDest++ = ulFill;
        *pulDest++ = ulFill;
        
        *pulDest++ = ulFill;
        *pulDest++ = ulFill;
        *pulDest++ = ulFill;
        *pulDest++ = ulFill;
        
        *pulDest++ = ulFill;
        *pulDest++ = ulFill;
        *pulDest++ = ulFill;
        *pulDest++ = ulFill;
        
        *pulDest++ = ulFill;
        *pulDest++ = ulFill;
        *pulDest++ = ulFill;
        *pulDest++ = ulFill;
        
        ulTemp -= __BIGBLOCKSTEP;
    }
    
    while (ulTemp >= __LITLOCKSTEP) {
        *pulDest++ = ulFill;
        ulTemp -= __LITLOCKSTEP;
    }
    
    pucDest = (PUCHAR)pulDest;
    
    /*
     *  剩余部分
     */
    ulTemp = (ULONG)(stCount & __LONGMASK);
    __TLOOP(*pucDest++ = ucC);
    
    return  (pvDest);
#endif                                                                  /*  __ARCH_MEMSET               */
}
/*********************************************************************************************************
** 函数名称: lib_bzero
** 功能描述: 置字节字符串s的前n个字节为零。
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
** 注  意  : 
*********************************************************************************************************/
VOID    lib_bzero (PVOID   pvStr, size_t  stCount)
{
#ifdef __ARCH_MEMSET
    __ARCH_MEMSET(pvStr, 0, stCount);

#else
    REGISTER ULONG     ulTemp;
    REGISTER PUCHAR    pucDest = (PUCHAR)pvStr;
    REGISTER ULONG    *pulDest;
    
    if ((ULONG)pucDest & __LONGMASK) {                                  /*  处理前端非对齐部分          */
        if (stCount < __LONGSIZE) {
            ulTemp = (ULONG)stCount;
        } else {
            ulTemp = (ULONG)(__LONGSIZE - ((ULONG)pucDest & __LONGMASK));
        }
        
        stCount -= (size_t)ulTemp;
        __TLOOP1(*pucDest++ = 0);
    }
    
    /*
     *  按字对齐处理
     */
    ulTemp = (INT)(stCount / __LONGSIZE);
    
    pulDest = (ULONG *)pucDest;
    
    while (ulTemp >= __BIGBLOCKSTEP) {
        *pulDest++ = 0;
        *pulDest++ = 0;
        *pulDest++ = 0;
        *pulDest++ = 0;
        
        *pulDest++ = 0;
        *pulDest++ = 0;
        *pulDest++ = 0;
        *pulDest++ = 0;
        
        *pulDest++ = 0;
        *pulDest++ = 0;
        *pulDest++ = 0;
        *pulDest++ = 0;
        
        *pulDest++ = 0;
        *pulDest++ = 0;
        *pulDest++ = 0;
        *pulDest++ = 0;
        
        ulTemp -= __BIGBLOCKSTEP;
    }
    
    while (ulTemp >= __LITLOCKSTEP) {
        *pulDest++ = 0;
        ulTemp -= __LITLOCKSTEP;
    }
    
    pucDest = (PUCHAR)pulDest;
    
    /*
     *  剩余部分
     */
    ulTemp = (ULONG)(stCount & __LONGMASK);
    __TLOOP(*pucDest++ = 0);
#endif                                                                  /*  __ARCH_MEMSET               */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
