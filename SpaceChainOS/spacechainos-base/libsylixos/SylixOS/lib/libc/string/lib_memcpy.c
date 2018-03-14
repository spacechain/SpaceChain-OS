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
** 文   件   名: lib_memcpy.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 库
**
** BUG:
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
** 函数名称: lib_memcpy
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
** 注  意  : 
*********************************************************************************************************/
PVOID  lib_memcpy (PVOID  pvDest, CPVOID   pvSrc, size_t  stCount)
{
#ifdef __ARCH_MEMCPY
    return  (__ARCH_MEMCPY(pvDest, pvSrc, stCount));
    
#else
    REGISTER PUCHAR    pucDest;
    REGISTER PUCHAR    pucSrc;
    
    REGISTER ULONG    *pulDest;
    REGISTER ULONG    *pulSrc;
    
             ULONG     ulTemp;
    
    pucDest = (PUCHAR)pvDest;
    pucSrc  = (PUCHAR)pvSrc;
    
    if (stCount == 0 || pucDest == pucSrc) {
        return  (pvDest);
    }
    
    if (pucDest < pucSrc) {
        /*
         *  正常循序拷贝
         */
        ulTemp = (ULONG)pucSrc;
        if ((ulTemp | (ULONG)pucDest) & __LONGMASK) {
            /*
             *  拷贝非对齐部分
             */
            if (((ulTemp ^ (ULONG)pucDest) & __LONGMASK) || (stCount < __LONGSIZE)) {
                ulTemp = (ULONG)stCount;
            } else {
                ulTemp = (ULONG)(__LONGSIZE - (ulTemp & __LONGMASK));
            }
            
            stCount -= (size_t)ulTemp;
            __TLOOP1(*pucDest++ = *pucSrc++);
        }
        
        /*
         *  按字对齐拷贝
         */
        ulTemp = (ULONG)(stCount / __LONGSIZE);
        
        pulDest = (ULONG *)pucDest;
        pulSrc  = (ULONG *)pucSrc;
        
        while (ulTemp >= __BIGBLOCKSTEP) {
            *pulDest++ = *pulSrc++;
            *pulDest++ = *pulSrc++;
            *pulDest++ = *pulSrc++;
            *pulDest++ = *pulSrc++;
            
            *pulDest++ = *pulSrc++;
            *pulDest++ = *pulSrc++;
            *pulDest++ = *pulSrc++;
            *pulDest++ = *pulSrc++;
            
            *pulDest++ = *pulSrc++;
            *pulDest++ = *pulSrc++;
            *pulDest++ = *pulSrc++;
            *pulDest++ = *pulSrc++;
            
            *pulDest++ = *pulSrc++;
            *pulDest++ = *pulSrc++;
            *pulDest++ = *pulSrc++;
            *pulDest++ = *pulSrc++;
            
            ulTemp -= __BIGBLOCKSTEP;
        }
        
        while (ulTemp >= __LITLOCKSTEP) {
            *pulDest++ = *pulSrc++;
            ulTemp -= __LITLOCKSTEP;
        }
        
        pucDest = (PUCHAR)pulDest;
        pucSrc  = (PUCHAR)pulSrc;
        
        /*
         *  剩余部分
         */
        ulTemp = (ULONG)(stCount & __LONGMASK);
        __TLOOP(*pucDest++ = *pucSrc++);
    
    } else {
        /*
         *  反向循序拷贝
         */
        pucSrc  += stCount;
        pucDest += stCount;
        
        ulTemp = (ULONG)pucSrc;
        if ((ulTemp | (ULONG)pucDest) & __LONGMASK) {
            /*
             *  拷贝非对齐部分
             */
            if (((ulTemp ^ (ULONG)pucDest) & __LONGMASK) || (stCount <= __LONGSIZE)) {
                ulTemp = (ULONG)stCount;
            } else {
                ulTemp &= __LONGMASK;
            }
            stCount -= (size_t)ulTemp;
            __TLOOP1(*--pucDest = *--pucSrc);
        }
        
        /*
         *  按字对齐拷贝
         */
        ulTemp = (ULONG)(stCount / __LONGSIZE);
        
        pulDest = (ULONG *)pucDest;
        pulSrc  = (ULONG *)pucSrc;
        
        while (ulTemp >= __BIGBLOCKSTEP) {
            *--pulDest = *--pulSrc;
            *--pulDest = *--pulSrc;
            *--pulDest = *--pulSrc;
            *--pulDest = *--pulSrc;
            
            *--pulDest = *--pulSrc;
            *--pulDest = *--pulSrc;
            *--pulDest = *--pulSrc;
            *--pulDest = *--pulSrc;
            
            *--pulDest = *--pulSrc;
            *--pulDest = *--pulSrc;
            *--pulDest = *--pulSrc;
            *--pulDest = *--pulSrc;
            
            *--pulDest = *--pulSrc;
            *--pulDest = *--pulSrc;
            *--pulDest = *--pulSrc;
            *--pulDest = *--pulSrc;
            
            ulTemp -= __BIGBLOCKSTEP;
        }
        
        while (ulTemp >= __LITLOCKSTEP) {
            *--pulDest = *--pulSrc;
            ulTemp -= __LITLOCKSTEP;
        }
        
        pucDest = (PUCHAR)pulDest;
        pucSrc  = (PUCHAR)pulSrc;
        
        /*
         *  剩余部分
         */
        ulTemp = (ULONG)(stCount & __LONGMASK);
        __TLOOP(*--pucDest = *--pucSrc);
    }
    
    return  (pvDest);
#endif                                                                  /*  __ARCH_MEMCPY               */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
