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
** 文   件   名: rngLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 03 月 05 日
**
** 描        述: VxWorks 兼容 ring buffer 缓冲区操作.

** BUG:
2009.08.18  加入获取缓冲区大小的函数.
2009.12.14  _rngCreate() 使用单一的内存, 避免内存碎片.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  声明
*********************************************************************************************************/
VOID  _rngFlush(VX_RING_ID  vxringid);
/*********************************************************************************************************
** 函数名称: _rngCreate
** 功能描述: 建立一个 VxWorks 兼容 ring buffer 缓冲区
** 输　入  : 缓冲区大小
** 输　出  : 缓冲区控制块地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VX_RING_ID  _rngCreate (INT  iNBytes)
{
    REGISTER PCHAR        pcBuffer;
    REGISTER VX_RING_ID   vxringid;
             size_t       stAllocSize;
    
    /* 
     *  bump number of bytes requested because ring buffer algorithm
     *  always leaves at least one empty byte in buffer 
     */
    iNBytes++;
    stAllocSize = (size_t)(sizeof(VX_RING) + iNBytes);
    
    /*
     *  统一分配内存避免内存碎片
     */
    vxringid = (VX_RING_ID)__SHEAP_ALLOC(stAllocSize);
    if (vxringid == LW_NULL) {
        return  (LW_NULL);
    }
    pcBuffer = (PCHAR)vxringid + sizeof(VX_RING);
    
    vxringid->VXRING_iBufByteSize = iNBytes;
    vxringid->VXRING_pcBuf        = pcBuffer;
    
    _rngFlush(vxringid);
    
    return  (vxringid);
}
/*********************************************************************************************************
** 函数名称: _rngDelete
** 功能描述: 删除一个 VxWorks 兼容 ring buffer 缓冲区
** 输　入  : 缓冲区控制块地址
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _rngDelete (VX_RING_ID  vxringid)
{
    __SHEAP_FREE((PVOID)vxringid);
}
/*********************************************************************************************************
** 函数名称: _rngSizeGet
** 功能描述: 获得一个 VxWorks 兼容 ring buffer 缓冲区大小
** 输　入  : 缓冲区大小
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _rngSizeGet (VX_RING_ID  vxringid)
{
    if (vxringid) {
        return  (vxringid->VXRING_iBufByteSize - 1);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: _rngFlush
** 功能描述: 将一个 VxWorks 兼容 ring buffer 缓冲区清空
** 输　入  : 缓冲区控制块地址
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _rngFlush (VX_RING_ID  vxringid)
{
    vxringid->VXRING_iToBuf   = 0;
    vxringid->VXRING_iFromBuf = 0;
}
/*********************************************************************************************************
** 函数名称: _rngBufGet
** 功能描述: 从一个 VxWorks 兼容 ring buffer 缓冲区读出最多 iMaxBytes 个字节
** 输　入  : 
**           vxringid           缓冲区控制块地址
**           pcBuffer           读出数据存放位置
**           iMaxBytes          读出最多的字节数
** 输　出  : 实际读出的字节数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _rngBufGet (VX_RING_ID  vxringid,
                 PCHAR       pcBuffer,
                 INT         iMaxBytes)
{
    REGISTER INT        iBytesGot = 0;
    REGISTER INT        iToBuf = vxringid->VXRING_iToBuf;
    REGISTER INT        iBytes2;
    REGISTER INT        iRngTemp = 0;
    
    if (iToBuf >= vxringid->VXRING_iFromBuf) {
    
        /* pToBuf has not wrapped around */
    
        iBytesGot = __MIN(iMaxBytes, iToBuf - vxringid->VXRING_iFromBuf);
        lib_memcpy(pcBuffer, &vxringid->VXRING_pcBuf[vxringid->VXRING_iFromBuf], iBytesGot);
        vxringid->VXRING_iFromBuf += iBytesGot;
    
    } else {
        
        /* pToBuf has wrapped around.  Grab chars up to the end of the
         * buffer, then wrap around if we need to. */

        iBytesGot = __MIN(iMaxBytes, vxringid->VXRING_iBufByteSize - vxringid->VXRING_iFromBuf);
        lib_memcpy(pcBuffer, &vxringid->VXRING_pcBuf[vxringid->VXRING_iFromBuf], iBytesGot);
        iRngTemp = vxringid->VXRING_iFromBuf + iBytesGot;
        
        /* If pFromBuf is equal to bufSize, we've read the entire buffer,
         * and need to wrap now.  If bytesgot < maxbytes, copy some more chars
         * in now. */
        
        if (iRngTemp == vxringid->VXRING_iBufByteSize) {
        
            iBytes2 = __MIN(iMaxBytes - iBytesGot, iToBuf);
            lib_memcpy(pcBuffer + iBytesGot, vxringid->VXRING_pcBuf, iBytes2);
            vxringid->VXRING_iFromBuf = iBytes2;
            iBytesGot += iBytes2;
        
        } else {
        
            vxringid->VXRING_iFromBuf = iRngTemp;
        }
    }
    
    return  (iBytesGot);
}
/*********************************************************************************************************
** 函数名称: _rngBufPut
** 功能描述: 向一个 VxWorks 兼容 ring buffer 缓冲区写入最多 iMaxBytes 个字节
** 输　入  : 
**           vxringid           缓冲区控制块地址
**           pcBuffer           写入数据存放位置
**           iNBytes            写入字节数
** 输　出  : 实际读出的字节数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _rngBufPut (VX_RING_ID  vxringid,
                 PCHAR       pcBuffer,
                 INT         iNBytes)
{
    REGISTER INT        iBytesPut = 0;
    REGISTER INT        iFromBuf = vxringid->VXRING_iFromBuf;
    REGISTER INT        iBytes2;
    REGISTER INT        iRngTemp = 0;
    
    if (iFromBuf > vxringid->VXRING_iToBuf) {
        
        /* pFromBuf is ahead of pToBuf.  We can fill up to two bytes
         * before it */
         
        iBytesPut = __MIN(iNBytes, iFromBuf - vxringid->VXRING_iToBuf - 1);
        lib_memcpy(&vxringid->VXRING_pcBuf[vxringid->VXRING_iToBuf], pcBuffer, iBytesPut);
        vxringid->VXRING_iToBuf += iBytesPut;
        
    } else if (iFromBuf == 0) {
        
        /* pFromBuf is at the beginning of the buffer.  We can fill till
         * the next-to-last element */
        
        iBytesPut = __MIN(iNBytes, vxringid->VXRING_iBufByteSize - vxringid->VXRING_iToBuf - 1);
        lib_memcpy(&vxringid->VXRING_pcBuf[vxringid->VXRING_iToBuf], pcBuffer, iBytesPut);
        vxringid->VXRING_iToBuf += iBytesPut;
        
    } else {
        
        /* pFromBuf has wrapped around, and its not 0, so we can fill
         * at least to the end of the ring buffer.  Do so, then see if
         * we need to wrap and put more at the beginning of the buffer. */
         
        iBytesPut = __MIN(iNBytes, vxringid->VXRING_iBufByteSize - vxringid->VXRING_iToBuf);
        lib_memcpy(&vxringid->VXRING_pcBuf[vxringid->VXRING_iToBuf], pcBuffer, iBytesPut);
        iRngTemp = vxringid->VXRING_iToBuf + iBytesPut;
        
        if (iRngTemp == vxringid->VXRING_iBufByteSize) {
            
            /* We need to wrap, and perhaps put some more chars */
            
            iBytes2 = __MIN(iNBytes - iBytesPut, iFromBuf - 1);
            lib_memcpy(vxringid->VXRING_pcBuf, pcBuffer + iBytesPut, iBytes2);
            vxringid->VXRING_iToBuf = iBytes2;
            iBytesPut += iBytes2;
        
        } else {
            
            vxringid->VXRING_iToBuf = iRngTemp;
        }
    }
    
    return  (iBytesPut);
}
/*********************************************************************************************************
** 函数名称: _rngIsEmpty
** 功能描述: 检查一个 VxWorks 兼容 ring buffer 缓冲区是否为空
** 输　入  : 缓冲区控制块地址
** 输　出  : 是否为空
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _rngIsEmpty (VX_RING_ID  vxringid)
{
    return  (vxringid->VXRING_iToBuf == vxringid->VXRING_iFromBuf);
}
/*********************************************************************************************************
** 函数名称: _rngIsFull
** 功能描述: 检查一个 VxWorks 兼容 ring buffer 缓冲区是否已满
** 输　入  : 缓冲区控制块地址
** 输　出  : 是否已满
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _rngIsFull (VX_RING_ID  vxringid)
{
    REGISTER INT    iNum = vxringid->VXRING_iToBuf - vxringid->VXRING_iFromBuf + 1;
    
    return  ((iNum == 0) || (iNum == vxringid->VXRING_iBufByteSize));
}
/*********************************************************************************************************
** 函数名称: _rngFreeBytes
** 功能描述: 检查一个 VxWorks 兼容 ring buffer 缓冲区的空闲字符数
** 输　入  : 缓冲区控制块地址
** 输　出  : 字符数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _rngFreeBytes (VX_RING_ID  vxringid)
{
    REGISTER INT    iNum = vxringid->VXRING_iFromBuf - vxringid->VXRING_iToBuf - 1;
    
    if (iNum < 0) {
        iNum += vxringid->VXRING_iBufByteSize;
    }
    
    return  (iNum);
}
/*********************************************************************************************************
** 函数名称: _rngNBytes
** 功能描述: 检查一个 VxWorks 兼容 ring buffer 缓冲区的有效字符数
** 输　入  : 缓冲区控制块地址
** 输　出  : 字符数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _rngNBytes (VX_RING_ID  vxringid)
{
    REGISTER INT    iNum = vxringid->VXRING_iToBuf - vxringid->VXRING_iFromBuf;
    
    if (iNum < 0) {
        iNum += vxringid->VXRING_iBufByteSize;
    }
    
    return  (iNum);
}
/*********************************************************************************************************
** 函数名称: _rngPutAhead
** 功能描述: 将一个字节信息放入一个 VxWorks 兼容 ring buffer 缓冲区的头部
** 输　入  : 
**           vxringid               缓冲区控制块地址
**           cByte                  字符
**           iOffset                偏移量
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _rngPutAhead (VX_RING_ID  vxringid,
                    CHAR        cByte,
                    INT         iOffset)
{
    REGISTER INT    iNum = vxringid->VXRING_iToBuf + iOffset;
    
    if (iNum >= vxringid->VXRING_iBufByteSize) {
        iNum -= vxringid->VXRING_iBufByteSize;
    }
    
    *(vxringid->VXRING_pcBuf + iNum) = cByte;
}
/*********************************************************************************************************
** 函数名称: _rngMoveAhead
** 功能描述: 将一个 VxWorks 兼容 ring buffer 缓冲区的指针向前推移
** 输　入  : 
**           vxringid               缓冲区控制块地址
**           iNum                   推移量
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _rngMoveAhead (VX_RING_ID  vxringid, INT  iNum)
{
    iNum += vxringid->VXRING_iToBuf;
    
    if (iNum >= vxringid->VXRING_iBufByteSize) {
        iNum -= vxringid->VXRING_iBufByteSize;
    }
    
    vxringid->VXRING_iToBuf = iNum;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
