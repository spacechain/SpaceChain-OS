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
** 文   件   名: lib_strncat.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 25 日
**
** 描        述: 库

** BUG:
2009.07.18  加入 strlcat() 函数.
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: lib_strncat
** 功能描述: At most strlen(pcDest) + iN + 1 bytes
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PCHAR  lib_strncat (PCHAR  pcDest, CPCHAR  pcSrc, size_t  stN)
{    
    REGISTER PCHAR    pcDestReg = (PCHAR)pcDest;
    REGISTER PCHAR    pcSrcReg  = (PCHAR)pcSrc;
    
    while (*pcDestReg != PX_EOS) {
        pcDestReg++;
    }
    
    while (*pcSrcReg != PX_EOS && stN > 0) {
        *pcDestReg++ = *pcSrcReg++;
        stN--;
    }
    *pcDestReg = PX_EOS;
    
    return  (pcDest);
}
/*********************************************************************************************************
** 函数名称: lib_strlcat
** 功能描述: unlike strncat, stN is the full size of dst, not space left.
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
size_t  lib_strlcat (PCHAR  pcDest, CPCHAR  pcSrc, size_t  stN)
{
    REGISTER PCHAR    pcDestReg = (PCHAR)pcDest;
    REGISTER PCHAR    pcSrcReg  = (PCHAR)pcSrc;
             size_t   stCnt     = stN;
             size_t   stDlen;
    
    while ((stCnt > 0) && (*pcDestReg != PX_EOS)) {
        pcDestReg++;
        stCnt--;
    }
    
    stDlen = (size_t)(pcDestReg - pcDest);
    stCnt  = stN - stDlen;
    
    if (stCnt == 0) {
        return  (stDlen + lib_strlen(pcSrc));
    }
    
    while (*pcSrcReg != PX_EOS) {
        if (stCnt > 1) {
            *pcDestReg++ = *pcSrcReg;
            stCnt--;
        }
        pcSrcReg++;
    }
    *pcDestReg = PX_EOS;
    
    /* 
     * Returns strlen(pcSrc) + MIN(stN, strlen(initial pcDest)).
     */
    return  (stDlen + (pcSrcReg - pcSrc));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
