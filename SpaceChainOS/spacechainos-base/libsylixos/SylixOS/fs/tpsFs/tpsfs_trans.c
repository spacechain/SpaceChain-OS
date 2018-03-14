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
** 文   件   名: tpsfs_trans.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: 事物操作

** BUG:
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0
#include "tpsfs_type.h"
#include "tpsfs_error.h"
#include "tpsfs_port.h"
#include "tpsfs_super.h"
#include "tpsfs_trans.h"
#include "tpsfs_btree.h"
#include "tpsfs_inode.h"
#include "tpsfs_dir.h"
#include "tpsfs_dev_buf.h"
/*********************************************************************************************************
** 函数名称: tpsFsBtreeTransInit
** 功能描述: 初始化事物列表
**           psb             超级块指针
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeTransInit (PTPS_SUPER_BLOCK psb)
{
    PTPS_TRANS_SB ptranssb;
    PTPS_TRANS    ptrans;

    if (LW_NULL == psb) {
        return  (TPS_ERR_PARAM_NULL);
    }

    ptranssb = TPS_ALLOC(sizeof(TPS_TRANS_SB));
    if (LW_NULL == ptranssb) {
        return  (TPS_ERR_ALLOC);
    }
    psb->SB_ptranssb = ptranssb;

    ptranssb->TSB_ui64TransSecStart  = (psb->SB_ui64LogStartBlk << psb->SB_uiBlkShift)
                                              >> psb->SB_uiSectorShift;
    ptranssb->TSB_ui64TransSecCnt    = psb->SB_ui64LogBlkCnt >> (TPS_TRAN_SHIFT + 1);
                                                                        /* 假设每个事物头对应2个块数据  */
    if (ptranssb->TSB_ui64TransSecCnt <= 0) {
        return  (TPS_TRAN_INIT_SIZE);
    }

    ptranssb->TSB_ui64TransDataStart = ptranssb->TSB_ui64TransSecStart + ptranssb->TSB_ui64TransSecCnt;
    ptranssb->TSB_ui64TransDataCnt   = ((psb->SB_ui64LogBlkCnt << psb->SB_uiBlkShift)
                                        >> psb->SB_uiSectorShift)
                                        - ptranssb->TSB_ui64TransSecCnt;
    if (ptranssb->TSB_ui64TransDataCnt <= TPS_TRANS_REV_DATASEC) {
        return  (TPS_TRAN_INIT_SIZE);
    }

    ptranssb->TSB_ui64TransCurSec    = ptranssb->TSB_ui64TransSecStart;
    ptranssb->TSB_uiTransSecOff      = 0;

    ptranssb->TSP_ui64DataCurSec     = ptranssb->TSB_ui64TransDataStart;
    ptranssb->TSP_ui64SerialNum      = 0;

    ptranssb->TSB_pucSecBuff = TPS_ALLOC(psb->SB_uiSectorSize);
    if (LW_NULL == ptranssb->TSB_pucSecBuff) {
        TPS_FREE(ptranssb);
        return  (TPS_ERR_ALLOC);
    }

    ptrans = (PTPS_TRANS)TPS_ALLOC(sizeof(TPS_TRANS));
    if (LW_NULL == ptrans) {
        TPS_FREE(ptranssb->TSB_pucSecBuff);
        TPS_FREE(ptranssb);

        return  (TPS_ERR_ALLOC);
    }
    ptranssb->TSB_ptrans = ptrans;

    ptrans->TRANS_uiMagic           = TPS_TRANS_MAGIC;
    ptrans->TRANS_ui64Generation    = psb->SB_ui64Generation;
    ptrans->TRANS_ui64SerialNum     = 0;
    ptrans->TRANS_iType             = TPS_TRANS_TYPE_DATA;
    ptrans->TRANS_iStatus           = TPS_TRANS_STATUS_INIT;
    ptrans->TRANS_ui64Time          = 0;
    ptrans->TRANS_uiDataSecNum      = 0;
    ptrans->TRANS_uiDataSecCnt      = 0;
    ptrans->TRANS_ui64Reserved      = 0;
    ptrans->TRANS_uiReserved        = 0;
    ptrans->TRANS_uiCheckSum        = 0;
    ptrans->TRANS_pnext             = 0;
    ptrans->TRANS_psb               = psb;

    ptrans->TRANS_pdata = (PTPS_TRANS_DATA)TPS_ALLOC(sizeof(TPS_TRANS_DATA) + 16 * TPS_TRANS_MAXAREA);
    if (LW_NULL == ptranssb->TSB_ptrans->TRANS_pdata) {
        TPS_FREE(ptranssb->TSB_pucSecBuff);
        TPS_FREE(ptrans);
        TPS_FREE(ptranssb);

        return  (TPS_ERR_ALLOC);
    }

    ptrans->TRANS_pdata->TD_uiSecAreaCnt = 0;

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsBtreeTransFini
** 功能描述: 释放事物列表
**           psb             超级块指针
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeTransFini (PTPS_SUPER_BLOCK psb)
{
    TPS_FREE(psb->SB_ptranssb->TSB_pucSecBuff);
    TPS_FREE(psb->SB_ptranssb->TSB_ptrans->TRANS_pdata);
    TPS_FREE(psb->SB_ptranssb->TSB_ptrans);
    TPS_FREE(psb->SB_ptranssb);

    return  (TPS_ERR_NONE);
}

/*********************************************************************************************************
** 函数名称: tspFsCompleteTrans
** 功能描述: 标记事物为一致状态
** 输　入  : ptrans           事物
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tspFsCompleteTrans (PTPS_SUPER_BLOCK psb)
{
    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsTransAllocAndInit
** 功能描述: 分配并初始化事物
** 输　入  : ptrans           事物
** 输　出  : 事物对象指针
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PTPS_TRANS  tpsFsTransAllocAndInit (PTPS_SUPER_BLOCK psb)
{
    PTPS_TRANS_SB ptranssb = psb->SB_ptranssb;
    PTPS_TRANS    ptrans   = ptranssb->TSB_ptrans;

    if (psb->SB_uiFlags & TPS_TRANS_FAULT) {
        return  (LW_NULL);
    }

    ptrans->TRANS_ui64SerialNum          = ptranssb->TSP_ui64SerialNum;
    ptrans->TRANS_uiDataSecNum           = ptranssb->TSP_ui64DataCurSec;
    ptrans->TRANS_uiDataSecCnt           = 0;
    ptrans->TRANS_ui64Time               = TPS_UTC_TIME();
    ptrans->TRANS_pdata->TD_uiSecAreaCnt = 0;

    ptrans->TRANS_iStatus = TPS_TRANS_STATUS_INIT;

    return  (ptrans);
}
/*********************************************************************************************************
** 函数名称: tpsFsTransRollBackAndFree
** 功能描述: 回滚并释放事物
** 输　入  : ptrans           事物
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsTransRollBackAndFree (PTPS_TRANS ptrans)
{
    INT           i;
    UINT64        ui64SecStart;
    UINT          uiSecCnt;

    PTPS_INODE       pinode   = LW_NULL;
    PTPS_SUPER_BLOCK psb      = ptrans->TRANS_psb;
    PTPS_TRANS_SB    ptranssb = ptrans->TRANS_psb->SB_ptranssb;

    UINT64        ui64BpSecStart;
    UINT64        ui64BpSecCnt;
    BOOL          bBpInvalid  = LW_FALSE;

    /*
     * 计算块缓冲区起始扇区以及扇区数
     */
    ui64BpSecStart  = psb->SB_ui64BPStartBlk << psb->SB_uiBlkShift >> psb->SB_uiSectorShift;
    ui64BpSecCnt    = psb->SB_ui64BPBlkCnt << psb->SB_uiBlkShift >> psb->SB_uiSectorShift;

    for (i = 0; i < ptrans->TRANS_pdata->TD_uiSecAreaCnt; i++) {        /* 无效inode中的事务相关缓冲块  */
        ui64SecStart = ptrans->TRANS_pdata->TD_secareaArr[i].TD_ui64SecStart;
        uiSecCnt     = ptrans->TRANS_pdata->TD_secareaArr[i].TD_uiSecCnt;
        pinode = psb->SB_pinodeOpenList;
        while (pinode) {
            if (tpsFsInodeBuffInvalid(pinode, ui64SecStart, uiSecCnt) != TPS_ERR_NONE) {
                psb->SB_uiFlags |= TPS_TRANS_FAULT;
                return  (TPS_ERR_TRANS_ROLLBK_FAULT);
            }

            pinode = pinode->IND_pnext;
        }

        if (max(ui64BpSecStart, ui64SecStart) <
            min((ui64SecStart + uiSecCnt), (ui64BpSecStart + ui64BpSecCnt))) {
            bBpInvalid = LW_TRUE;
        }
    }

    if (psb->SB_pinodeDeleted != LW_NULL) {
        if (tpsFsCloseInode(psb->SB_pinodeDeleted) != TPS_ERR_NONE) {   /* 清除已删除inode链表缓冲      */
            psb->SB_uiFlags |= TPS_TRANS_FAULT;
            return  (TPS_ERR_TRANS_ROLLBK_FAULT);
        }
        psb->SB_pinodeDeleted = LW_NULL;
    }

    ptrans->TRANS_ui64SerialNum          = ptranssb->TSP_ui64SerialNum;
    ptrans->TRANS_uiDataSecNum           = ptranssb->TSP_ui64DataCurSec;
    ptrans->TRANS_uiDataSecCnt           = 0;
    ptrans->TRANS_ui64Time               = TPS_UTC_TIME();
    ptrans->TRANS_pdata->TD_uiSecAreaCnt = 0;
    ptrans->TRANS_iStatus                = TPS_TRANS_STATUS_UNINIT;

    if (bBpInvalid) {                                                   /* 重新读取块缓冲区             */
        if (tpsFsBtreeReadBP(psb) != TPS_ERR_NONE) {
            psb->SB_uiFlags |= TPS_TRANS_FAULT;
            return  (TPS_ERR_BP_READ);
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsTransSecAreaSerial
** 功能描述: 扇区区间列表
** 输　入  : ptransdata         扇区列表指针
**           iIndex             从第几个区间开始序列化
**           pucSecBuf          序列化缓冲区
**           uiLen              缓冲区长度
** 输　出  : 扇区区间列表序列化完成时的索引,下一次序列化时传入
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsTransSecAreaSerial (PTPS_TRANS_DATA   ptransdata,
                                       INT               iIndex,
                                       PUCHAR            pucSecBuf,
                                       UINT              uiLen)
{
    PUCHAR  pucPos   = pucSecBuf;

    TPS_CPU_TO_LE32(pucPos, ptransdata->TD_uiSecAreaCnt);
    TPS_CPU_TO_LE32(pucPos, iIndex);
    TPS_CPU_TO_LE32(pucPos, ptransdata->TD_uiReserved[0]);
    TPS_CPU_TO_LE32(pucPos, ptransdata->TD_uiReserved[1]);

    while ((pucPos - pucSecBuf) < (uiLen - 16) &&
           (iIndex < ptransdata->TD_uiSecAreaCnt)) {
        TPS_CPU_TO_LE64(pucPos, ptransdata->TD_secareaArr[iIndex].TD_ui64SecStart);
        TPS_CPU_TO_LE32(pucPos, ptransdata->TD_secareaArr[iIndex].TD_uiSecOff);
        TPS_CPU_TO_LE32(pucPos, ptransdata->TD_secareaArr[iIndex].TD_uiSecCnt);

        iIndex++;
    }

    return  (iIndex);
}
/*********************************************************************************************************
** 函数名称: __tpsFsTransSecAreaUnserial
** 功能描述: 逆扇区区间列表
** 输　入  : ptransdata         扇区列表指针
**           pucSecBuf          序列化缓冲区
**           uiLen              缓冲区长度
** 输　出  : 本此逆序化的扇区最小区间索引，索引为0表示逆序列化完成
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsTransSecAreaUnserial (PTPS_TRANS_DATA ptransdata, PUCHAR pucSecBuf, UINT uiLen)
{
    PUCHAR  pucPos   = pucSecBuf;
    INT     iIndex;
    INT     i;

    TPS_LE32_TO_CPU(pucPos, ptransdata->TD_uiSecAreaCnt);
    TPS_LE32_TO_CPU(pucPos, iIndex);
    TPS_LE32_TO_CPU(pucPos, ptransdata->TD_uiReserved[0]);
    TPS_LE32_TO_CPU(pucPos, ptransdata->TD_uiReserved[1]);

    i = iIndex;
    while ((pucPos - pucSecBuf) < (uiLen - 16) &&
           (i < ptransdata->TD_uiSecAreaCnt)) {
        TPS_LE64_TO_CPU(pucPos, ptransdata->TD_secareaArr[i].TD_ui64SecStart);
        TPS_LE32_TO_CPU(pucPos, ptransdata->TD_secareaArr[i].TD_uiSecOff);
        TPS_LE32_TO_CPU(pucPos, ptransdata->TD_secareaArr[i].TD_uiSecCnt);

        i++;
    }

    return  (iIndex);
}
/*********************************************************************************************************
** 函数名称: __tpsFsTransSerial
** 功能描述: 序列化事务头
** 输　入  : ptrans         事务指针
**           pucSecBuf      序列化缓冲区
**           uiLen          缓冲区长度
** 输　出  : 成功返回LW_TRUE,否则返回LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  __tpsFsTransSerial (PTPS_TRANS ptrans, PUCHAR pucSecBuf, UINT uiLen)
{
    PUCHAR  pucPos   = pucSecBuf;

    /*
     *  计算校验和
     */
    ptrans->TRANS_uiCheckSum = 0;
    ptrans->TRANS_uiCheckSum += (UINT)ptrans->TRANS_uiMagic;
    ptrans->TRANS_uiCheckSum += (UINT)ptrans->TRANS_uiReserved;
    ptrans->TRANS_uiCheckSum += (UINT)ptrans->TRANS_ui64Generation;
    ptrans->TRANS_uiCheckSum += (UINT)(ptrans->TRANS_ui64Generation >> 32);
    ptrans->TRANS_uiCheckSum += (UINT)ptrans->TRANS_ui64SerialNum;
    ptrans->TRANS_uiCheckSum += (UINT)(ptrans->TRANS_ui64SerialNum >> 32);
    ptrans->TRANS_uiCheckSum += (UINT)ptrans->TRANS_iType;
    ptrans->TRANS_uiCheckSum += (UINT)ptrans->TRANS_iStatus;
    ptrans->TRANS_uiCheckSum += (UINT)ptrans->TRANS_ui64Reserved;
    ptrans->TRANS_uiCheckSum += (UINT)(ptrans->TRANS_ui64Reserved >> 32);
    ptrans->TRANS_uiCheckSum += (UINT)ptrans->TRANS_ui64Time;
    ptrans->TRANS_uiCheckSum += (UINT)(ptrans->TRANS_ui64Time >> 32);
    ptrans->TRANS_uiCheckSum += (UINT)(ptrans->TRANS_uiDataSecNum >> 32);
    ptrans->TRANS_uiCheckSum += (UINT)ptrans->TRANS_uiDataSecCnt;

    /*
     *  序列化
     */
    TPS_CPU_TO_LE32(pucPos, ptrans->TRANS_uiMagic);
    TPS_CPU_TO_LE32(pucPos, ptrans->TRANS_uiReserved);
    TPS_CPU_TO_LE64(pucPos, ptrans->TRANS_ui64Generation);
    TPS_CPU_TO_LE64(pucPos, ptrans->TRANS_ui64SerialNum);
    TPS_CPU_TO_LE32(pucPos, ptrans->TRANS_iType);
    TPS_CPU_TO_LE32(pucPos, ptrans->TRANS_iStatus);
    TPS_CPU_TO_LE64(pucPos, ptrans->TRANS_ui64Reserved);
    TPS_CPU_TO_LE64(pucPos, ptrans->TRANS_ui64Time);
    TPS_CPU_TO_LE64(pucPos, ptrans->TRANS_uiDataSecNum);
    TPS_CPU_TO_LE32(pucPos, ptrans->TRANS_uiDataSecCnt);
    TPS_CPU_TO_LE32(pucPos, ptrans->TRANS_uiCheckSum);

    return  (LW_TRUE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsTransUnserial
** 功能描述: 逆序列化事务头，判断事务有效性
** 输　入  : ptrans         事务指针
**           pucSecBuf      序列化缓冲区
**           uiLen          缓冲区长度
** 输　出  : 成功返回LW_TRUE,否则返回LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  __tpsFsTransUnserial (PTPS_SUPER_BLOCK  psb,
                                   PTPS_TRANS        ptrans,
                                   PUCHAR            pucSecBuf,
                                   UINT              uiLen)
{
    PUCHAR  pucPos      = pucSecBuf;
    UINT    uiCheckSum  = 0;

    /*
     *  逆序列化
     */
    TPS_LE32_TO_CPU(pucPos, ptrans->TRANS_uiMagic);
    TPS_LE32_TO_CPU(pucPos, ptrans->TRANS_uiReserved);
    TPS_LE64_TO_CPU(pucPos, ptrans->TRANS_ui64Generation);
    TPS_LE64_TO_CPU(pucPos, ptrans->TRANS_ui64SerialNum);
    TPS_LE32_TO_CPU(pucPos, ptrans->TRANS_iType);
    TPS_LE32_TO_CPU(pucPos, ptrans->TRANS_iStatus);
    TPS_LE64_TO_CPU(pucPos, ptrans->TRANS_ui64Reserved);
    TPS_LE64_TO_CPU(pucPos, ptrans->TRANS_ui64Time);
    TPS_LE64_TO_CPU(pucPos, ptrans->TRANS_uiDataSecNum);
    TPS_LE32_TO_CPU(pucPos, ptrans->TRANS_uiDataSecCnt);
    TPS_LE32_TO_CPU(pucPos, ptrans->TRANS_uiCheckSum);

    /*
     *  计算校验和
     */
    uiCheckSum += (UINT)ptrans->TRANS_uiMagic;
    uiCheckSum += (UINT)ptrans->TRANS_uiReserved;
    uiCheckSum += (UINT)ptrans->TRANS_ui64Generation;
    uiCheckSum += (UINT)(ptrans->TRANS_ui64Generation >> 32);
    uiCheckSum += (UINT)ptrans->TRANS_ui64SerialNum;
    uiCheckSum += (UINT)(ptrans->TRANS_ui64SerialNum >> 32);
    uiCheckSum += (UINT)ptrans->TRANS_iType;
    uiCheckSum += (UINT)ptrans->TRANS_iStatus;
    uiCheckSum += (UINT)ptrans->TRANS_ui64Reserved;
    uiCheckSum += (UINT)(ptrans->TRANS_ui64Reserved >> 32);
    uiCheckSum += (UINT)ptrans->TRANS_ui64Time;
    uiCheckSum += (UINT)(ptrans->TRANS_ui64Time >> 32);
    uiCheckSum += (UINT)(ptrans->TRANS_uiDataSecNum >> 32);
    uiCheckSum += (UINT)ptrans->TRANS_uiDataSecCnt;

    /*
     *  判断事务有效性
     */
    if (uiCheckSum != ptrans->TRANS_uiCheckSum   ||
        ptrans->TRANS_uiMagic != TPS_TRANS_MAGIC ||
        ptrans->TRANS_ui64Generation != psb->SB_ui64Generation) {
        ptrans->TRANS_iStatus = TPS_TRANS_STATUS_UNINIT;
    }

    return  (LW_TRUE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsGetTrans
** 功能描述: 读取事务头
** 输　入  : psb         超级块指针
**           ptrans      事务指针
**           ui64Index   事务索引
** 输　出  : 成功返回LW_TRUE,否则返回LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsGetTrans (PTPS_SUPER_BLOCK psb, PTPS_TRANS ptrans, UINT64 ui64Index)
{
    PTPS_TRANS_SB    ptranssb = psb->SB_ptranssb;

    ptranssb->TSB_ui64TransCurSec = ptranssb->TSB_ui64TransSecStart + 
                                    (ui64Index >> (psb->SB_uiSectorShift - TPS_TRAN_SHIFT));
    ptranssb->TSB_uiTransSecOff   = (ui64Index & ((1 << (psb->SB_uiSectorShift - TPS_TRAN_SHIFT)) - 1))
                                    << TPS_TRAN_SHIFT;

    if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, ptranssb->TSB_pucSecBuff,
                                    ptranssb->TSB_ui64TransCurSec, 1) != 0) {
        ptrans->TRANS_iStatus = TPS_TRANS_STATUS_UNINIT;

        return  (TPS_ERR_BUF_WRITE);
    }

    __tpsFsTransUnserial(psb, ptrans,
                         ptranssb->TSB_pucSecBuff + ptranssb->TSB_uiTransSecOff,
                         TPS_TRAN_SIZE);

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsLoadTransData
** 功能描述: 加载事务数据
** 输　入  : ptrans      事务指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  __tpsFsLoadTransData (PTPS_TRANS ptrans)
{
    INT              i   = 0;
    PTPS_SUPER_BLOCK psb = ptrans->TRANS_psb;

    /*
     *  循环逆序列化扇区区间列表
     */
    do {
        if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, psb->SB_pucSectorBuf,
                                        (ptrans->TRANS_uiDataSecNum + ptrans->TRANS_uiDataSecCnt - 1),
                                        1) != 0) {
            return  (TPS_ERR_BUF_WRITE);
        }

        i = __tpsFsTransSecAreaUnserial(ptrans->TRANS_pdata,
                                        psb->SB_pucSectorBuf,
                                        psb->SB_uiSectorSize);

        ptrans->TRANS_uiDataSecCnt--;

    } while (i > 0);

    return  (TPS_ERR_NONE);
}

/*********************************************************************************************************
** 函数名称: tpsFsTransCommitAndFree
** 功能描述: 提交并释放事物
** 输　入  : ptrans           事物
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsTransCommitAndFree (PTPS_TRANS ptrans)
{
    INT     i = 0;
    INT     j;
    UINT64  ui64DataSecCnt    = 0;
    UINT64  ui64SecWrCnt      = 0;   
    PTPS_SUPER_BLOCK psb      = ptrans->TRANS_psb;
    PTPS_TRANS_SB    ptranssb = psb->SB_ptranssb;

    if (psb->SB_pbp != LW_NULL) {                                       /* 检查是否需要调整块缓冲队列   */
        if (tpsFsBtreeAdjustBP(ptrans, psb) != TPS_ERR_NONE) {
            return  (TPS_ERR_BP_ADJUST);
        }
    }

    if (ptrans->TRANS_pdata->TD_uiSecAreaCnt == 0) {                    /* 没有数据需要提交             */
        ptrans->TRANS_iStatus = TPS_TRANS_STATUS_COMPLETE;
        return  (TPS_ERR_NONE);
    }

    while (i < ptrans->TRANS_pdata->TD_uiSecAreaCnt) {                  /* 序列化扇区区间列表           */
        i = __tpsFsTransSecAreaSerial(ptrans->TRANS_pdata, i,
                                      psb->SB_pucSectorBuf, psb->SB_uiSectorSize);

        if (psb->SB_dev->DEV_WriteSector(psb->SB_dev, psb->SB_pucSectorBuf,
                                         ptrans->TRANS_uiDataSecNum + ptrans->TRANS_uiDataSecCnt,
                                         1, LW_FALSE) != 0) {
            return  (TPS_ERR_BUF_WRITE);
        }

        ptrans->TRANS_uiDataSecCnt++;
    }

    if (psb->SB_dev->DEV_Sync(psb->SB_dev, ptrans->TRANS_uiDataSecNum,
                              ptrans->TRANS_uiDataSecCnt) != 0) {
        return  (TPS_ERR_BUF_SYNC);
    }

    ptrans->TRANS_iStatus = TPS_TRANS_STATUS_COMMIT;
    __tpsFsTransSerial(ptrans,
                       ptranssb->TSB_pucSecBuff + ptranssb->TSB_uiTransSecOff,
                       psb->SB_uiSectorSize - ptranssb->TSB_uiTransSecOff);

    if (psb->SB_dev->DEV_WriteSector(psb->SB_dev, ptranssb->TSB_pucSecBuff,
                                     ptranssb->TSB_ui64TransCurSec,
                                     1, LW_TRUE) != 0) {                /* 标记事务头为提交态到磁盘     */
        ptrans->TRANS_iStatus = TPS_TRANS_STATUS_INIT;
        return  (TPS_ERR_BUF_WRITE);
    }

    /*
     *  本行代码后出错，则只能重做事物，不能回滚，因此将文件系统设置为不可访问状态
     */
    for (i = 0; i < ptrans->TRANS_pdata->TD_uiSecAreaCnt; i++) {        /* 写实际扇区                   */
        ui64DataSecCnt = ptrans->TRANS_uiDataSecNum + ptrans->TRANS_pdata->TD_secareaArr[i].TD_uiSecOff;

        for (j = 0; j < ptrans->TRANS_pdata->TD_secareaArr[i].TD_uiSecCnt; j += ui64SecWrCnt) {
            ui64SecWrCnt = min((ptrans->TRANS_pdata->TD_secareaArr[i].TD_uiSecCnt - j),
                               psb->SB_uiSecPerBlk);
            if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, psb->SB_pucSectorBuf,
                                            ui64DataSecCnt + j, ui64SecWrCnt) != 0) {
                psb->SB_uiFlags |= TPS_TRANS_FAULT;
                return  (TPS_ERR_TRANS_COMMIT_FAULT);
            }

            if (psb->SB_dev->DEV_WriteSector(psb->SB_dev, psb->SB_pucSectorBuf,
                                             ptrans->TRANS_pdata->TD_secareaArr[i].TD_ui64SecStart + j,
                                             ui64SecWrCnt,
                                             LW_FALSE) != 0) {
                psb->SB_uiFlags |= TPS_TRANS_FAULT;
                return  (TPS_ERR_TRANS_COMMIT_FAULT);
            }
        }

        if (psb->SB_dev->DEV_Sync(psb->SB_dev,
                                  ptrans->TRANS_pdata->TD_secareaArr[i].TD_ui64SecStart,
                                  ptrans->TRANS_pdata->TD_secareaArr[i].TD_uiSecCnt) != 0) {
            psb->SB_uiFlags |= TPS_TRANS_FAULT;
            return  (TPS_ERR_TRANS_COMMIT_FAULT);
        }
    }

    ptrans->TRANS_iStatus = TPS_TRANS_STATUS_COMPLETE;
    __tpsFsTransSerial(ptrans,
                       ptranssb->TSB_pucSecBuff + ptranssb->TSB_uiTransSecOff,
                       psb->SB_uiSectorSize - ptranssb->TSB_uiTransSecOff);

    if (psb->SB_dev->DEV_WriteSector(psb->SB_dev, ptranssb->TSB_pucSecBuff,
                                     ptranssb->TSB_ui64TransCurSec,
                                     1, LW_TRUE) != 0) {                /* 标记事务为完成态到磁盘       */
        ptrans->TRANS_iStatus = TPS_TRANS_STATUS_COMMIT;
        psb->SB_uiFlags |= TPS_TRANS_FAULT;
        return  (TPS_ERR_TRANS_COMMIT_FAULT);
    }

    ptranssb->TSB_uiTransSecOff += TPS_TRAN_SIZE;
    if (ptranssb->TSB_uiTransSecOff >= psb->SB_uiSectorSize) {          /* 事务技术自增，内部指针调整   */
        ptranssb->TSB_ui64TransCurSec++;
        ptranssb->TSB_uiTransSecOff = 0;
        if (ptranssb->TSB_ui64TransCurSec >= (ptranssb->TSB_ui64TransSecStart +
                                              ptranssb->TSB_ui64TransSecCnt)) {
            ptranssb->TSB_ui64TransCurSec = ptranssb->TSB_ui64TransSecStart;
        }

        lib_bzero(ptranssb->TSB_pucSecBuff, psb->SB_uiSectorSize);
    }

    ptranssb->TSP_ui64DataCurSec = ptrans->TRANS_uiDataSecNum + ptrans->TRANS_uiDataSecCnt;
    if (ptranssb->TSP_ui64DataCurSec >= (ptranssb->TSB_ui64TransDataStart +
                                         ptranssb->TSB_ui64TransDataCnt - TPS_TRANS_REV_DATASEC)) {
        ptranssb->TSP_ui64DataCurSec = ptranssb->TSB_ui64TransDataStart;
    }
    ptranssb->TSP_ui64SerialNum++;

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tspFsCheckTrans
** 功能描述: 检查事物完整性
** 输　入  : ptrans           事物
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tspFsCheckTrans (PTPS_SUPER_BLOCK psb)
{
    UINT64          ui64LIndex;
    UINT64          ui64HIndex;
    UINT64          ui64MIndex;
    TPS_TRANS       transStart;
    TPS_TRANS       transEnd;
    TPS_TRANS       transMid;
    INT             i;

    TPS_TRANS_SB   *ptranssb = psb->SB_ptranssb;

    transStart.TRANS_iStatus = TPS_TRANS_STATUS_UNINIT;
    transEnd.TRANS_iStatus   = TPS_TRANS_STATUS_UNINIT;
    transMid.TRANS_iStatus   = TPS_TRANS_STATUS_UNINIT;

    ui64LIndex = 0;
    ui64HIndex = (ptranssb->TSB_ui64TransSecCnt << (psb->SB_uiSectorShift - TPS_TRAN_SHIFT)) - 1;

    /*
     *  使用二分法查找序列号最大的事务，即最后一次提交的事务
     */
    while (1) {
        __tpsFsGetTrans(psb, &transStart, ui64LIndex);
        if (transStart.TRANS_iStatus == TPS_TRANS_STATUS_UNINIT) {      /* 事务无效则向前查找16个事务   */
            for (i = 0; i < 16; i++) {
                __tpsFsGetTrans(psb, &transStart, ui64LIndex + i);
                if (transStart.TRANS_iStatus != TPS_TRANS_STATUS_UNINIT) {
                    ui64LIndex += i;
                    break;
                }
            }

            /*
             *  起始16个事务一直无效表示还没有事务提交过
             */
            if (i == 16) {
                ui64MIndex = ui64LIndex;
                break;
            }
        }

        __tpsFsGetTrans(psb, &transEnd, ui64HIndex);
        if (transEnd.TRANS_iStatus == TPS_TRANS_STATUS_UNINIT) {
            for (i = 0; i < 16; i++) {
                __tpsFsGetTrans(psb, &transEnd, ui64HIndex - i);
                if (transEnd.TRANS_iStatus != TPS_TRANS_STATUS_UNINIT) {
                    ui64HIndex -= i;
                    break;
                }
            }
        }

        if (transEnd.TRANS_iStatus != TPS_TRANS_STATUS_UNINIT) {
            if (transStart.TRANS_ui64SerialNum < transEnd.TRANS_ui64SerialNum) {
                ui64MIndex = ui64HIndex;
                break;
            }
        }

        ui64MIndex = (ui64LIndex + ui64HIndex) >> 1;

        __tpsFsGetTrans(psb, &transMid, ui64MIndex);
        if (transMid.TRANS_iStatus == TPS_TRANS_STATUS_UNINIT) {
            for (i = 0; i < 16; i++) {
                __tpsFsGetTrans(psb, &transMid, ui64MIndex - i);
                if (transMid.TRANS_iStatus != TPS_TRANS_STATUS_UNINIT) {
                    ui64MIndex -= i;
                    break;
                }
            }
        }

        if (ui64MIndex == ui64LIndex) {
            break;
        }

        if (transMid.TRANS_iStatus == TPS_TRANS_STATUS_UNINIT) {
            ui64HIndex = ui64MIndex;
        } else if (transStart.TRANS_ui64SerialNum > transMid.TRANS_ui64SerialNum) {
            ui64HIndex = ui64MIndex;
        } else {
            ui64LIndex = ui64MIndex;
        }
    }

    __tpsFsGetTrans(psb, &transMid, ui64MIndex);                        /* 获取事务                     */

    if (transEnd.TRANS_iStatus != TPS_TRANS_STATUS_UNINIT && 
        transEnd.TRANS_ui64SerialNum > transMid.TRANS_ui64SerialNum) {
        transMid   = transEnd;
        ui64MIndex = ui64HIndex;
    }

    if (transMid.TRANS_iStatus != TPS_TRANS_STATUS_UNINIT) {
        ptranssb->TSP_ui64SerialNum   = transMid.TRANS_ui64SerialNum;
        ptranssb->TSB_ui64TransCurSec = ptranssb->TSB_ui64TransSecStart +
                                        (ui64MIndex >> (psb->SB_uiSectorShift - TPS_TRAN_SHIFT));
        ptranssb->TSB_uiTransSecOff   = (ui64MIndex &
                                         ((1 << (psb->SB_uiSectorShift - TPS_TRAN_SHIFT)) - 1))
                                        << TPS_TRAN_SHIFT;
    }

    if (transMid.TRANS_iStatus == TPS_TRANS_STATUS_COMPLETE) {          /* 事务为已完成态无需重做       */
        ptranssb->TSB_uiTransSecOff += TPS_TRAN_SIZE;
        if (ptranssb->TSB_uiTransSecOff >= psb->SB_uiSectorSize) {
            ptranssb->TSB_ui64TransCurSec++;
            ptranssb->TSB_uiTransSecOff = 0;
            if (ptranssb->TSB_ui64TransCurSec >= (ptranssb->TSB_ui64TransSecStart +
                ptranssb->TSB_ui64TransSecCnt)) {
                ptranssb->TSB_ui64TransCurSec = ptranssb->TSB_ui64TransSecStart;
            }
        }

        ptranssb->TSP_ui64DataCurSec = transMid.TRANS_uiDataSecNum + transMid.TRANS_uiDataSecCnt;
        if (ptranssb->TSP_ui64DataCurSec >= (ptranssb->TSB_ui64TransDataStart +
            ptranssb->TSB_ui64TransDataCnt - TPS_TRANS_REV_DATASEC)) {
            ptranssb->TSP_ui64DataCurSec = ptranssb->TSB_ui64TransDataStart;
        }
        ptranssb->TSP_ui64SerialNum++;

        return  (TPS_ERR_NONE);

    } else if (transMid.TRANS_iStatus == TPS_TRANS_STATUS_COMMIT) {     /* 重做提交但未完成的事务       */
        transMid.TRANS_pdata = ptranssb->TSB_ptrans->TRANS_pdata;
        transMid.TRANS_pnext = ptranssb->TSB_ptrans->TRANS_pnext;
        transMid.TRANS_psb   = ptranssb->TSB_ptrans->TRANS_psb;
        (*ptranssb->TSB_ptrans) = transMid;

#ifdef WIN32                                                            /* windows 打印格式             */
        _DebugFormat(__LOGMESSAGE_LEVEL,
                     "tpsFs trans recover:\r\n "
                     "serial number = %016I64x  sector = %016I64x  count = %08x  time = %016I64x\r\n",
                     transMid.TRANS_ui64SerialNum, transMid.TRANS_uiDataSecNum,
                     transMid.TRANS_uiDataSecCnt, transMid.TRANS_ui64Time);
#else
        _DebugFormat(__LOGMESSAGE_LEVEL,
                     "tpsFs trans recover:\r\n "
                     "serial number = %016qx  sector = %016qx  count = %08x  time = %016qx\r\n",
                     transMid.TRANS_ui64SerialNum, transMid.TRANS_uiDataSecNum,
                     transMid.TRANS_uiDataSecCnt, transMid.TRANS_ui64Time);
#endif

        if (__tpsFsLoadTransData(ptranssb->TSB_ptrans)) {
            return  (TPS_ERR_READ_DEV);
        }

        return (tpsFsTransCommitAndFree(ptranssb->TSB_ptrans));
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tspFsCheckTrans
** 功能描述: 从事务链表获取数据，因为部分磁盘数据可能被缓存在事务链表中
** 输　入  : psb           超级块指针
**           pucSecBuf     扇区数据缓冲区
**           ui64SecNum    扇区起始
**           ui64SecCnt    扇区数
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __tpsFsTransGetData (PTPS_SUPER_BLOCK   psb,
                                  PUCHAR             pucSecBuf,
                                  UINT64             ui64SecNum,
                                  UINT64             ui64SecCnt)
{
    INT                 i;
    UINT64              ui64SecAreaStart;
    UINT64              ui64SecAreaCnt;
    UINT64              ui64DataSecCnt;
    PTPS_TRANS          ptrans = psb->SB_ptranssb->TSB_ptrans;
    UINT                uiSecNeedCpy;
    PTPS_TRANS_DATA     ptrdata;

    /*
     *  遍历事务链表
     */
    while (ptrans) {
        if ((ptrans->TRANS_iStatus != TPS_TRANS_STATUS_INIT) &&
            (ptrans->TRANS_iStatus != TPS_TRANS_STATUS_COMMIT)) {       /* 跳过未初始化或以完成的事务   */
            ptrans = ptrans->TRANS_pnext;
            continue;
        }

        ptrdata = ptrans->TRANS_pdata;
        for (i = 0; i < ptrdata->TD_uiSecAreaCnt; i++) {                /* 遍历扇区区间列表             */
            ui64SecAreaStart = ptrdata->TD_secareaArr[i].TD_ui64SecStart;
            ui64DataSecCnt   = ptrans->TRANS_uiDataSecNum + ptrdata->TD_secareaArr[i].TD_uiSecOff;
            ui64SecAreaCnt   = ptrdata->TD_secareaArr[i].TD_uiSecCnt;

            /*
             *  是否重叠，只需处理存在重叠的扇区区间
             */
            if (max(ui64SecAreaStart, ui64SecNum) >=
                min((ui64SecAreaStart + ui64SecAreaCnt), (ui64SecNum + ui64SecCnt))) {
                continue;
            }

            /*
             *  计算重叠区间大小
             */
            uiSecNeedCpy = min((ui64SecAreaStart + ui64SecAreaCnt), (ui64SecNum + ui64SecCnt)) -
                           max(ui64SecAreaStart, ui64SecNum);

            if (ui64SecAreaStart > ui64SecNum) {
                psb->SB_dev->DEV_ReadSector(psb->SB_dev,
                                            pucSecBuf + ((ui64SecAreaStart - ui64SecNum)
                                                         << psb->SB_uiSectorShift),
                                            ui64DataSecCnt,
                                            uiSecNeedCpy);
            } else {
                psb->SB_dev->DEV_ReadSector(psb->SB_dev,
                                            pucSecBuf,
                                            ui64DataSecCnt + ui64SecNum - ui64SecAreaStart,
                                            uiSecNeedCpy);
            }
        }

        ptrans = ptrans->TRANS_pnext;
    }
}
/*********************************************************************************************************
** 函数名称: tspFsCheckTrans
** 功能描述: 添加数据到事务
** 输　入  : ptrans        事务指针
**           pucSecBuf     扇区数据缓冲区
**           ui64SecNum    扇区起始
**           ui64SecCnt    扇区数
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsTransPutData (PTPS_TRANS   ptrans,
                                        PUCHAR       pucSecBuf,
                                        UINT64       ui64SecNum,
                                        UINT64       ui64SecCnt)
{
    PTPS_SUPER_BLOCK    psb = ptrans->TRANS_psb;
    PTPS_TRANS_DATA     ptrdata = ptrans->TRANS_pdata;
    INT                 i;
    INT                 j;
    UINT64              ui64SecAreaStart;
    UINT64              ui64SecAreaCnt;
    UINT                uiSecNeedCpy;

    /*
     *  安全性判断
     */
    if ((ptrdata->TD_uiSecAreaCnt >= (TPS_TRANS_MAXAREA - 1)) ||
        ((ptrans->TRANS_uiDataSecNum + ptrans->TRANS_uiDataSecCnt + ui64SecCnt) >
         (psb->SB_ptranssb->TSB_ui64TransDataStart + psb->SB_ptranssb->TSB_ui64TransDataCnt))) {
        return (TPS_ERR_TRANS_OVERFLOW);
    }

    for (i = 0; i < ptrdata->TD_uiSecAreaCnt; i++) {                    /* 对现有扇区区间去重复         */
        ui64SecAreaStart = ptrdata->TD_secareaArr[i].TD_ui64SecStart;
        ui64SecAreaCnt   = ptrdata->TD_secareaArr[i].TD_uiSecCnt;

        /*
         *  区间数增加
         */
        if (ui64SecAreaStart < ui64SecNum &&
           (ui64SecAreaStart + ui64SecAreaCnt) > (ui64SecNum + ui64SecCnt)) {
            ptrdata->TD_secareaArr[i].TD_uiSecCnt = ui64SecNum - ui64SecAreaStart;

            for (j = ptrdata->TD_uiSecAreaCnt; j > i + 1; j--) {
                ptrdata->TD_secareaArr[j] = ptrdata->TD_secareaArr[j - 1];
            }
            ptrdata->TD_secareaArr[i + 1].TD_ui64SecStart = ui64SecNum + ui64SecCnt;
            ptrdata->TD_secareaArr[i + 1].TD_uiSecCnt     = (ui64SecAreaStart + ui64SecAreaCnt) -
                                                            (ui64SecNum + ui64SecCnt);
            ptrdata->TD_secareaArr[i + 1].TD_uiSecOff     = ptrdata->TD_secareaArr[i].TD_uiSecOff +
                                                            (ui64SecNum + ui64SecCnt - ui64SecAreaStart);

            ptrdata->TD_uiSecAreaCnt++;
            i++;
            continue;
        }

        /*
         *  区间数减少
         */
        if (ui64SecAreaStart >= ui64SecNum &&
           (ui64SecAreaStart + ui64SecAreaCnt) <= (ui64SecNum + ui64SecCnt)) {
            for (j = i; j < ptrdata->TD_uiSecAreaCnt - 1; j++) {
                ptrdata->TD_secareaArr[j] = ptrdata->TD_secareaArr[j + 1];
            }

            ptrdata->TD_uiSecAreaCnt--;
            i--;
            continue;
        }

        /*
         *  区间不重叠
         */
        if (max(ui64SecAreaStart, ui64SecNum) >=
            min((ui64SecAreaStart + ui64SecAreaCnt), (ui64SecNum + ui64SecCnt))) {
            continue;
        }

        /*
         *  区间重叠
         */
        uiSecNeedCpy = min((ui64SecAreaStart + ui64SecAreaCnt), (ui64SecNum + ui64SecCnt)) -
                       max(ui64SecAreaStart, ui64SecNum);
        ptrdata->TD_secareaArr[i].TD_uiSecCnt -= uiSecNeedCpy;
        if (ui64SecAreaStart >= ui64SecNum) {
            ptrdata->TD_secareaArr[i].TD_ui64SecStart += uiSecNeedCpy;
            ptrdata->TD_secareaArr[i].TD_uiSecOff     += uiSecNeedCpy;
        }
    }

    /*
     *  追加扇区区间
     */
    ptrdata->TD_secareaArr[ptrdata->TD_uiSecAreaCnt].TD_ui64SecStart = ui64SecNum;
    ptrdata->TD_secareaArr[ptrdata->TD_uiSecAreaCnt].TD_uiSecCnt     = ui64SecCnt;
    ptrdata->TD_secareaArr[ptrdata->TD_uiSecAreaCnt].TD_uiSecOff     = ptrans->TRANS_uiDataSecCnt;

    /*
     *  写入扇区数据
     */
    if (psb->SB_dev->DEV_WriteSector(psb->SB_dev, pucSecBuf,
                                     ptrans->TRANS_uiDataSecNum + ptrans->TRANS_uiDataSecCnt,
                                     ui64SecCnt, LW_FALSE) != 0) {
        return  (TPS_ERR_BUF_WRITE);
    }

    ptrans->TRANS_uiDataSecCnt += ui64SecCnt;
    ptrdata->TD_uiSecAreaCnt++;

    return  (TPS_ERR_NONE);
}

/*********************************************************************************************************
** 函数名称: tpsFsTransRead
** 功能描述: 从磁盘读取元数据
** 输　入  : psb              超级块指针
**           blk              块号
**           uiOff            块内偏移
**           pucBuff          数据缓冲区
**           szLen            写入长度
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsTransRead (PTPS_SUPER_BLOCK   psb,
                            TPS_IBLK           blk,
                            UINT               uiOff,
                            PUCHAR             pucBuff,
                            size_t             szLen)
{
    size_t        szCompleted  = 0;
    UINT          uiReadLen;
    PUCHAR        pucSecBuf    = LW_NULL;
    UINT          uiSecSize    = 0;
    UINT          uiSecOff     = 0;
    UINT64        ui64SecNum   = 0;
    UINT64        ui64SecCnt   = 0;

    if ((psb == LW_NULL)     ||
        (pucBuff == LW_NULL) ||
        (psb->SB_pucSectorBuf == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    uiSecSize = psb->SB_uiSectorSize;
    pucSecBuf = psb->SB_pucSectorBuf;

    ui64SecNum = ((blk << psb->SB_uiBlkShift) + uiOff) >> psb->SB_uiSectorShift;
    ui64SecCnt = (((blk << psb->SB_uiBlkShift) + uiOff + szLen) >> psb->SB_uiSectorShift) - ui64SecNum;

    uiSecOff = uiOff & psb->SB_uiSectorMask;
    if (uiSecOff != 0) {                                                /* 起始位置不对齐               */
        uiReadLen = uiSecSize - uiSecOff;
        uiReadLen = min(szLen, uiReadLen);
        if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, pucSecBuf, ui64SecNum, 1) != 0) {
            return  (TPS_ERR_BUF_READ);
        }

        __tpsFsTransGetData(psb, pucSecBuf, ui64SecNum, 1);             /* 从事务链表获取数据           */

        lib_memcpy(pucBuff, pucSecBuf + uiSecOff, uiReadLen);

        szCompleted += uiReadLen;
        ui64SecNum++;
        if (ui64SecCnt > 0) {
            ui64SecCnt--;
        }
    }

    if (ui64SecCnt > 0) {                                               /* 对齐扇区直接写缓冲区         */
        if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, pucBuff + szCompleted,
                                        ui64SecNum, ui64SecCnt) != 0) {
            return  (TPS_ERR_BUF_READ);
        }

        __tpsFsTransGetData(psb, pucBuff + szCompleted, ui64SecNum, ui64SecCnt);

        szCompleted += (size_t)(ui64SecCnt * uiSecSize);
        ui64SecNum  += ui64SecCnt;
        ui64SecCnt   = 0;
    }

    if (szCompleted <  szLen) {                                         /* 结束位置不对齐               */
        uiReadLen = ((uiOff + szLen) & psb->SB_uiSectorMask);
        if (uiReadLen > 0) {
            if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, pucSecBuf, ui64SecNum, 1) != 0) {
                return  (TPS_ERR_BUF_READ);
            }

            __tpsFsTransGetData(psb, pucSecBuf, ui64SecNum, 1);         /* 从事务链表获取数据           */

            lib_memcpy(pucBuff + szCompleted, pucSecBuf, uiReadLen);

            szCompleted += uiReadLen;
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsTransWrite
** 功能描述: 写入数据到事物
** 输　入  : ptrans           事物
**           psb              超级快指针
**           blk              块号
**           uiOff            块内偏移
**           pucBuff          数据缓冲区
**           szLen            写入长度
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsTransWrite (PTPS_TRANS        ptrans,
                             PTPS_SUPER_BLOCK  psb,
                             TPS_IBLK          blk,
                             UINT              uiOff,
                             PUCHAR            pucBuff,
                             size_t            szLen)
{
    size_t        szCompleted = 0;
    UINT          uiReadLen;
    PUCHAR        pucSecBuf   = LW_NULL;
    UINT          uiSecSize   = 0;
    UINT          uiSecOff    = 0;
    UINT64        ui64SecNum  = 0;
    UINT64        ui64SecCnt  = 0;

    if ((psb == LW_NULL)     ||
        (pucBuff == LW_NULL) ||
        (psb->SB_pucSectorBuf == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (LW_NULL == ptrans) {
        return  (tpsFsDevBufWrite(psb, blk, uiOff, pucBuff, szLen, LW_TRUE));
    }

    uiSecSize = psb->SB_uiSectorSize;
    pucSecBuf = psb->SB_pucSectorBuf;

    ui64SecNum = ((blk << psb->SB_uiBlkShift) + uiOff) >> psb->SB_uiSectorShift;
    ui64SecCnt = (((blk << psb->SB_uiBlkShift) + uiOff + szLen) >> psb->SB_uiSectorShift) - ui64SecNum;

    uiSecOff = uiOff & psb->SB_uiSectorMask;
    if (uiSecOff != 0) {                                                /* 起始位置不对齐,先读取再写入  */
        uiReadLen = uiSecSize - uiSecOff;
        uiReadLen = min(szLen, uiReadLen);
        if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, pucSecBuf, ui64SecNum, 1) != 0) {
            return  (TPS_ERR_BUF_READ);
        }

        __tpsFsTransGetData(psb, pucSecBuf, ui64SecNum, 1);             /* 从事务链表获取数据           */

        lib_memcpy(pucSecBuf + uiSecOff, pucBuff, uiReadLen);

        if (__tpsFsTransPutData(ptrans, pucSecBuf, ui64SecNum, 1) != TPS_ERR_NONE) {
            return  (TPS_ERR_BUF_WRITE);
        }

        szCompleted += uiReadLen;
        ui64SecNum++;
        if (ui64SecCnt > 0) {
            ui64SecCnt--;
        }
    }

    if (ui64SecCnt > 0) {                                               /* 对齐扇区直接操作缓冲区       */
        if (__tpsFsTransPutData(ptrans, pucBuff + szCompleted,
                                ui64SecNum, ui64SecCnt) != TPS_ERR_NONE) {
            return  (TPS_ERR_BUF_WRITE);
        }

        szCompleted += (size_t)(ui64SecCnt * uiSecSize);
        ui64SecNum  += ui64SecCnt;
        ui64SecCnt   = 0;
    }

    if (szCompleted < szLen) {                                          /* 结束位置不对齐,先读取再写入  */
        uiReadLen = ((uiOff + szLen) & psb->SB_uiSectorMask);
        if (uiReadLen > 0) {
            if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, pucSecBuf, ui64SecNum, 1) != 0) {
                return  (TPS_ERR_BUF_READ);
            }

            __tpsFsTransGetData(psb, pucSecBuf, ui64SecNum, 1);         /* 从事务链表获取数据           */

            lib_memcpy(pucSecBuf, pucBuff + szCompleted, uiReadLen);

            if (__tpsFsTransPutData(ptrans, pucSecBuf, ui64SecNum, 1) != TPS_ERR_NONE) {
                return  (TPS_ERR_BUF_WRITE);
            }

            szCompleted += uiReadLen;
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsTransTrigerChk
** 功能描述: 判断事务是否应该提交事务，防止事务数据溢出，一般用于一个将大事务划分成多个小事务
** 输　入  : ptrans           事物
** 输　出  : 返回LW_TRUE表示事务需要提交，否则表示不需要提交
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  tpsFsTransTrigerChk (PTPS_TRANS ptrans)
{
    PTPS_SUPER_BLOCK psb      = ptrans->TRANS_psb;
    PTPS_TRANS_SB    ptranssb = psb->SB_ptranssb;

    if (LW_NULL == ptrans) {
        return  (LW_FALSE);
    }

    if (((ptrans->TRANS_uiDataSecNum + ptrans->TRANS_uiDataSecCnt) >
        (ptranssb->TSB_ui64TransDataStart + ptranssb->TSB_ui64TransDataCnt - TPS_TRANS_REV_DATASEC))) {
        return (LW_TRUE);
    }

    return (ptrans->TRANS_pdata->TD_uiSecAreaCnt > TPS_TRANS_TRIGGERREA ? LW_TRUE : LW_FALSE);
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
