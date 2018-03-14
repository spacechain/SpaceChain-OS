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
** 文   件   名: tpsfs_dev_buf.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: 磁盘缓冲区操作

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
** 函数名称: tpsFsDevBufWrite
** 功能描述: 写入数据到磁盘
** 输　入  : psb              超级块指针
**           blk              块号
**           uiOff            块内偏移
**           pucBuff          数据缓冲区
**           szLen            写入长度
**           bSync            是否同步写入
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsDevBufWrite (PTPS_SUPER_BLOCK  psb,
                              TPS_IBLK          blk,
                              UINT              uiOff,
                              PUCHAR            pucBuff,
                              size_t            szLen,
                              BOOL              bSync)
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

        lib_memcpy(pucSecBuf + uiSecOff, pucBuff, uiReadLen);

        if (psb->SB_dev->DEV_WriteSector(psb->SB_dev, pucSecBuf, ui64SecNum, 1, bSync) != 0) {
            return  (TPS_ERR_BUF_WRITE);
        }

        szCompleted += uiReadLen;
        ui64SecNum++;
        if (ui64SecCnt > 0) {
            ui64SecCnt--;
        }
    }

    if (ui64SecCnt > 0) {                                               /* 对齐扇区直接操作缓冲区       */
        if (psb->SB_dev->DEV_WriteSector(psb->SB_dev, pucBuff + szCompleted,
                                         ui64SecNum, ui64SecCnt, bSync) != 0) {
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

            lib_memcpy(pucSecBuf, pucBuff + szCompleted, uiReadLen);

            if (psb->SB_dev->DEV_WriteSector(psb->SB_dev, pucSecBuf, ui64SecNum, 1, bSync) != 0) {
                return  (TPS_ERR_BUF_WRITE);
            }

            szCompleted += uiReadLen;
        }
    }


    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsDevBufRead
** 功能描述: 从磁盘读取数据
** 输　入  : psb              超级块指针
**           blk              块号
**           uiOff            块内偏移
**           pucBuff          数据缓冲区
**           szLen            写入长度
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsDevBufRead (PTPS_SUPER_BLOCK   psb,
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

            lib_memcpy(pucBuff + szCompleted, pucSecBuf, uiReadLen);

            szCompleted += uiReadLen;
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsDevBufSync
** 功能描述: 同步磁盘缓冲区
** 输　入  : psb              超级块指针
**           blk              块号
**           uiOff            块内偏移
**           szLen            写入长度
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsDevBufSync (PTPS_SUPER_BLOCK psb, TPS_IBLK blk, UINT uiOff, size_t szLen)
{
    UINT64  ui64SecNum = 0;
    UINT64  ui64SecCnt = 0;

    if (psb == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }
    
    ui64SecNum = ((blk << psb->SB_uiBlkShift) + uiOff) >> psb->SB_uiSectorShift;
    ui64SecCnt = (((blk << psb->SB_uiBlkShift) + uiOff + szLen) >> psb->SB_uiSectorShift) - ui64SecNum;
    
    if ((uiOff + szLen) & psb->SB_uiSectorMask) {
        ui64SecCnt++;
    }

    if (psb->SB_dev->DEV_Sync(psb->SB_dev, ui64SecNum, ui64SecCnt) != 0) {
        return  (TPS_ERR_BUF_SYNC);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsDevBufTrim
** 功能描述: 对磁盘执行FIOTRIM命令
** 输　入  : psb              超级块指针
**           blk              块号
**           blkStart         起始块
**           blkCnt           块数目
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsDevBufTrim (PTPS_SUPER_BLOCK psb, TPS_IBLK blkStart, TPS_IBLK blkCnt)
{
    UINT64  ui64SecNum = 0;
    UINT64  ui64SecCnt = 0;

    if (psb == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    ui64SecNum = (blkStart << psb->SB_uiBlkShift) >> psb->SB_uiSectorShift;
    ui64SecCnt = (blkCnt << psb->SB_uiBlkShift) >> psb->SB_uiSectorShift;

    if (psb->SB_dev->DEV_Trim(psb->SB_dev, ui64SecNum, ui64SecCnt) != 0) {
        return  (TPS_ERR_BUF_TRIM);
    }

    return  (TPS_ERR_NONE);
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
