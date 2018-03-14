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
** 文   件   名: rngLib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 13 日
**
** 描        述: VXWORKS 兼容 RING BUFFER 库
*********************************************************************************************************/

#ifndef  __RNGLIB_H
#define  __RNGLIB_H

/*********************************************************************************************************
  VxWorks 兼容 RING BUFFER
*********************************************************************************************************/

typedef struct {
    INT                        VXRING_iToBuf;                           /* offset from start of buffer  */
                                                                        /* where to write next          */
    INT                        VXRING_iFromBuf;                         /* offset from start of buffer  */
                                                                        /* where to read next           */
    INT                        VXRING_iBufByteSize;                     /* size of ring in bytes        */
    PCHAR                      VXRING_pcBuf;                            /* pointer to start of buffer   */
} VX_RING;
typedef VX_RING               *VX_RING_ID;

/*********************************************************************************************************
  MACRO
*********************************************************************************************************/

#define  __RNG_ELEM_GET(vxringid, pcB, iFrom)                \
         (                                                   \
             iFrom = (vxringid)->VXRING_iFromBuf,            \
             ((vxringid)->VXRING_iToBuf == iFrom) ?          \
             0                                               \
             :                                               \
             (                                               \
                 *pcB = (vxringid)->VXRING_pcBuf[iFrom],     \
                 ++iFrom,                                    \
                 (vxringid)->VXRING_iFromBuf = ((iFrom == (vxringid)->VXRING_iBufByteSize) ? 0 : iFrom), \
                 1                                           \
             )                                               \
         )
         
#define  __RNG_ELEM_PUT(vxringid, cB, iTo)                   \
         (                                                   \
             iTo = (vxringid)->VXRING_iToBuf,                \
             (iTo == (vxringid)->VXRING_iFromBuf - 1) ?      \
             0                                               \
             :                                               \
             (                                               \
                 (iTo == (vxringid)->VXRING_iBufByteSize - 1) ?  \
                 (                                           \
                     ((vxringid)->VXRING_iFromBuf == 0) ?    \
                     0                                       \
                     :                                       \
                     (                                       \
                         (vxringid)->VXRING_pcBuf[iTo] = cB, \
                         (vxringid)->VXRING_iToBuf = 0,      \
                         1                                   \
                     )                                       \
                 )                                           \
                 :                                           \
                 (                                           \
                     (vxringid)->VXRING_pcBuf[iTo] = cB,     \
                     (vxringid)->VXRING_iToBuf++,            \
                     1                                       \
                 )                                           \
             )                                               \
         )

/*********************************************************************************************************
  VxWorks compatible
*********************************************************************************************************/

#define  rngIsEmpty                     _rngIsEmpty
#define  rngIsFull                      _rngIsFull

#define  rngCreate                      _rngCreate
#define  rngDelete                      _rngDelete

#define  rngBufGet                      _rngBufGet
#define  rngBufPut                      _rngBufPut

#define  rngSizeGet                     _rngSizeGet
#define  rngFreeBytes                   _rngFreeBytes
#define  rngNBytes                      _rngNBytes

#define  rngFlush                       _rngFlush
#define  rngMoveAhead                   _rngMoveAhead
#define  rngPutAhead                    _rngPutAhead

#define  RNG_ELEM_GET                   __RNG_ELEM_GET
#define  RNG_ELEM_PUT                   __RNG_ELEM_PUT

/*********************************************************************************************************
  功能声明
*********************************************************************************************************/

VX_RING_ID  _rngCreate(INT  iNBytes);

VOID        _rngDelete(VX_RING_ID  vxringid);

VOID        _rngFlush(VX_RING_ID  vxringid);

INT         _rngBufGet(VX_RING_ID  vxringid,
                       PCHAR       pcBuffer,
                       INT         iMaxBytes);
                 
INT         _rngBufPut(VX_RING_ID  vxringid,
                       PCHAR       pcBuffer,
                       INT         iNBytes);
                 
INT         _rngIsEmpty(VX_RING_ID  vxringid);

INT         _rngIsFull(VX_RING_ID  vxringid);

INT         _rngSizeGet(VX_RING_ID  vxringid);

INT         _rngFreeBytes(VX_RING_ID  vxringid);

INT         _rngNBytes(VX_RING_ID  vxringid);

VOID        _rngPutAhead(VX_RING_ID  vxringid,
                         CHAR        cByte,
                         INT         iOffset);
                    
VOID        _rngMoveAhead(VX_RING_ID  vxringid, INT  iNum);

#endif                                                                  /*  __RNGLIB_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
