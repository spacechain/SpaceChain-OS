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
** 文   件   名: bmsgLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 10 月 02 日
**
** 描        述: 成块消息缓冲区.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  相关参数
*********************************************************************************************************/
#define LW_BMSG_PREFIX_LEN      2
/*********************************************************************************************************
** 函数名称: _bmsgCreate
** 功能描述: 创建一个 block msg 缓冲区
** 输　入  : stSize       缓冲区大小 
** 输　出  : 新建的缓冲区
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_BMSG  _bmsgCreate (size_t  stSize)
{
    PLW_BMSG    pbmsg;
    
    if (stSize < LW_BMSG_MIN_SIZE) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    pbmsg = (PLW_BMSG)__SHEAP_ALLOC(sizeof(LW_BMSG) + stSize);
    if (!pbmsg) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    
    pbmsg->BM_pucBuffer = (PUCHAR)pbmsg + sizeof(LW_BMSG);
    pbmsg->BM_stSize    = stSize;
    pbmsg->BM_stLeft    = stSize;
    
    pbmsg->BM_pucPut    = pbmsg->BM_pucBuffer;
    pbmsg->BM_pucGet    = pbmsg->BM_pucBuffer;
    
    return  (pbmsg);
}
/*********************************************************************************************************
** 函数名称: _bmsgDelete
** 功能描述: 删除一个 block msg 缓冲区
** 输　入  : pbmsg        缓冲区
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _bmsgDelete (PLW_BMSG  pbmsg)
{
    __SHEAP_FREE(pbmsg);
}
/*********************************************************************************************************
** 函数名称: _bmsgPut
** 功能描述: 将一条数据存入 block msg 缓冲区
** 输　入  : pbmsg        缓冲区
**           pvMsg        消息
**           stSize       消息长度 (不能大于 65535)
** 输　出  : 保存的字节数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _bmsgPut (PLW_BMSG  pbmsg, CPVOID  pvMsg, size_t  stSize)
{
#define LW_BMSG_ADJUST_PUT() \
        if (pbmsg->BM_pucPut >= (pbmsg->BM_pucBuffer + pbmsg->BM_stSize)) { \
            pbmsg->BM_pucPut  = pbmsg->BM_pucBuffer; \
        }

    size_t  stLen;
    size_t  stSave = stSize + LW_BMSG_PREFIX_LEN;
    
    if (stSave >= (UINT16)(~0)) {
        return  (0);
    }
    
    if (pbmsg->BM_stLeft < stSave) {
        return  (0);
    }
    
    *pbmsg->BM_pucPut = (UCHAR)(stSize >> 8);                           /*  保存两个字节长度信息        */
    pbmsg->BM_pucPut++;
    LW_BMSG_ADJUST_PUT();
    *pbmsg->BM_pucPut = (UCHAR)(stSize & 0xff);
    pbmsg->BM_pucPut++;
    LW_BMSG_ADJUST_PUT();
    
    stLen = (pbmsg->BM_pucBuffer + pbmsg->BM_stSize) - pbmsg->BM_pucPut;
    if (stLen >= stSize) {
        lib_memcpy(pbmsg->BM_pucPut, pvMsg, stSize);
        
        if (stLen > stSize) {
            pbmsg->BM_pucPut += stSize;
        
        } else {
            pbmsg->BM_pucPut = pbmsg->BM_pucBuffer;
        }
        
    } else {
        lib_memcpy(pbmsg->BM_pucPut, pvMsg, stLen);
        pbmsg->BM_pucPut  = pbmsg->BM_pucBuffer;
        
        lib_memcpy(pbmsg->BM_pucPut, (PUCHAR)pvMsg + stLen, stSize - stLen);
        pbmsg->BM_pucPut += (stSize - stLen);
    }
    
    pbmsg->BM_stLeft -= stSave;
    
    return  ((INT)stSize);
}
/*********************************************************************************************************
** 函数名称: _bmsgGet
** 功能描述: 从 block msg 缓冲区取出一条消息
** 输　入  : pbmsg        缓冲区
**           pvMsg        接收缓冲
**           stBufferSize 接收缓冲长度
** 输　出  : 获取的字节数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _bmsgGet (PLW_BMSG  pbmsg, PVOID  pvMsg, size_t  stBufferSize)
{
#define LW_BMSG_ADJUST_GET() \
        if (pbmsg->BM_pucGet >= (pbmsg->BM_pucBuffer + pbmsg->BM_stSize)) { \
            pbmsg->BM_pucGet  = pbmsg->BM_pucBuffer; \
        }

    size_t  stLen;
    size_t  stSize;
    UCHAR   ucHigh;
    UCHAR   ucLow;
    
    stLen = pbmsg->BM_stSize - pbmsg->BM_stLeft;
    if (stLen == 0) {
        return  (0);
    }
    
    ucHigh = pbmsg->BM_pucGet[0];
    if ((pbmsg->BM_pucGet - pbmsg->BM_pucBuffer) >= pbmsg->BM_stSize) {
        ucLow = pbmsg->BM_pucBuffer[0];
    } else {
        ucLow = pbmsg->BM_pucGet[1];
    }
    
    stSize = (size_t)((ucHigh << 8) + ucLow);
    
    if (stSize > stBufferSize) {
        return  (PX_ERROR);                                             /*  缓冲区太小                  */
    }
    
    pbmsg->BM_pucGet++;
    LW_BMSG_ADJUST_GET();
    pbmsg->BM_pucGet++;
    LW_BMSG_ADJUST_GET();
    
    stLen = (pbmsg->BM_pucBuffer + pbmsg->BM_stSize) - pbmsg->BM_pucGet;
    if (stLen >= stSize) {
        lib_memcpy(pvMsg, pbmsg->BM_pucGet, stSize);
        
        if (stLen > stSize) {
            pbmsg->BM_pucGet += stSize;
        
        } else {
            pbmsg->BM_pucGet = pbmsg->BM_pucBuffer;
        }
    
    } else {
        lib_memcpy(pvMsg, pbmsg->BM_pucGet, stLen);
        pbmsg->BM_pucGet  = pbmsg->BM_pucBuffer;
        
        lib_memcpy((PUCHAR)pvMsg + stLen, pbmsg->BM_pucGet, stSize - stLen);
        pbmsg->BM_pucGet += (stSize - stLen);
    }
    
    pbmsg->BM_stLeft += (stSize + LW_BMSG_PREFIX_LEN);
    
    return  ((INT)stSize);
}
/*********************************************************************************************************
** 函数名称: _bmsgFlush
** 功能描述: 清空 block msg 缓冲区
** 输　入  : pbmsg        缓冲区
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _bmsgFlush (PLW_BMSG  pbmsg)
{
    pbmsg->BM_stLeft = pbmsg->BM_stSize;
    pbmsg->BM_pucPut = pbmsg->BM_pucBuffer;
    pbmsg->BM_pucGet = pbmsg->BM_pucBuffer;
}
/*********************************************************************************************************
** 函数名称: _bmsgIsEmpty
** 功能描述: block msg 缓冲区是否为空
** 输　入  : pbmsg        缓冲区
** 输　出  : 是否为空
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _bmsgIsEmpty (PLW_BMSG  pbmsg)
{
    return  (pbmsg->BM_stLeft == pbmsg->BM_stSize);
}
/*********************************************************************************************************
** 函数名称: _bmsgIsFull
** 功能描述: block msg 缓冲区是否为满
** 输　入  : pbmsg        缓冲区
** 输　出  : 是否已满
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _bmsgIsFull (PLW_BMSG  pbmsg)
{
    return  (pbmsg->BM_stLeft < (LW_BMSG_PREFIX_LEN + 1));
}
/*********************************************************************************************************
** 函数名称: _bmsgSizeGet
** 功能描述: block msg 缓冲区大小
** 输　入  : pbmsg        缓冲区
** 输　出  : 总大小
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _bmsgSizeGet (PLW_BMSG  pbmsg)
{
    return  (pbmsg->BM_stSize);
}
/*********************************************************************************************************
** 函数名称: _bmsgFreeByte
** 功能描述: block msg 缓冲区剩余空间大小
** 输　入  : pbmsg        缓冲区
** 输　出  : 剩余空间大小
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _bmsgFreeByte (PLW_BMSG  pbmsg)
{
    if (pbmsg->BM_stLeft < (LW_BMSG_PREFIX_LEN + 1)) {
        return  (0);
    } else {
        return  ((INT)(pbmsg->BM_stLeft - (LW_BMSG_PREFIX_LEN + 1)));
    }
}
/*********************************************************************************************************
** 函数名称: _bmsgNBytes
** 功能描述: block msg 缓冲区总信息量
** 输　入  : pbmsg        缓冲区
** 输　出  : 总信息大小
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _bmsgNBytes (PLW_BMSG  pbmsg)
{
    return  ((INT)(pbmsg->BM_stSize - pbmsg->BM_stLeft));
}
/*********************************************************************************************************
** 函数名称: _bmsgNBytesNext
** 功能描述: block msg 缓冲区下一条信息长度
** 输　入  : pbmsg        缓冲区
** 输　出  : 下一条信息长度
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _bmsgNBytesNext (PLW_BMSG  pbmsg)
{
    size_t  stSize;
    UCHAR   ucHigh;
    UCHAR   ucLow;
    
    if (pbmsg->BM_stSize == pbmsg->BM_stLeft) {
        return  (0);
    }
    
    ucHigh = pbmsg->BM_pucGet[0];
    if ((pbmsg->BM_pucGet - pbmsg->BM_pucBuffer) >= pbmsg->BM_stSize) {
        ucLow = pbmsg->BM_pucBuffer[0];
    } else {
        ucLow = pbmsg->BM_pucGet[1];
    }
    
    stSize = (size_t)((ucHigh << 8) + ucLow);
    
    return  ((INT)stSize);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
