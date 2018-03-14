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
** 文   件   名: tpsfs_super.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: tpsfs 超级块处理

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
/*********************************************************************************************************
** 函数名称: __tpsFsSBUnserial
** 功能描述: 反序列化super block
** 输　入  : psb           super block指针
**           pucSectorBuf  sector缓冲区
**           uiSectorSize  缓冲区大小
** 输　出  : 无
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __tpsFsSBUnserial (PTPS_SUPER_BLOCK psb,
                                PUCHAR           pucSectorBuf,
                                UINT             uiSectorSize)
{
    PUCHAR  pucPos = pucSectorBuf;

    TPS_LE32_TO_CPU(pucPos, psb->SB_uiMagic);
    TPS_LE32_TO_CPU(pucPos, psb->SB_uiVersion);
    TPS_LE32_TO_CPU(pucPos, psb->SB_uiSectorSize);
    TPS_LE32_TO_CPU(pucPos, psb->SB_uiSecPerBlk);
    TPS_LE32_TO_CPU(pucPos, psb->SB_uiBlkSize);
    TPS_LE32_TO_CPU(pucPos, psb->SB_uiFlags);
    TPS_LE64_TO_CPU(pucPos, psb->SB_ui64Generation);
    TPS_LE64_TO_CPU(pucPos, psb->SB_ui64TotalBlkCnt);
    TPS_LE64_TO_CPU(pucPos, psb->SB_ui64DataStartBlk);
    TPS_LE64_TO_CPU(pucPos, psb->SB_ui64DataBlkCnt);
    TPS_LE64_TO_CPU(pucPos, psb->SB_ui64LogStartBlk);
    TPS_LE64_TO_CPU(pucPos, psb->SB_ui64LogBlkCnt);
    TPS_LE64_TO_CPU(pucPos, psb->SB_ui64BPStartBlk);
    TPS_LE64_TO_CPU(pucPos, psb->SB_ui64BPBlkCnt);
    TPS_LE64_TO_CPU(pucPos, psb->SB_inumSpaceMng);
    TPS_LE64_TO_CPU(pucPos, psb->SB_inumRoot);
    TPS_LE64_TO_CPU(pucPos, psb->SB_inumDeleted);
}
/*********************************************************************************************************
** 函数名称: __tpsFsSerial
** 功能描述: 序列化super block
** 输　入  : psb           super block指针
**           pucSectorBuf  sector缓冲区
**           uiSectorSize  缓冲区大小
** 输　出  : 无
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __tpsFsSBSerial (PTPS_SUPER_BLOCK  psb,
                              PUCHAR            pucSectorBuf,
                              UINT              uiSectorSize)
{
    PUCHAR  pucPos = pucSectorBuf;

    TPS_CPU_TO_LE32(pucPos, psb->SB_uiMagic);
    TPS_CPU_TO_LE32(pucPos, psb->SB_uiVersion);
    TPS_CPU_TO_LE32(pucPos, psb->SB_uiSectorSize);
    TPS_CPU_TO_LE32(pucPos, psb->SB_uiSecPerBlk);
    TPS_CPU_TO_LE32(pucPos, psb->SB_uiBlkSize);
    TPS_CPU_TO_LE32(pucPos, psb->SB_uiFlags);
    TPS_CPU_TO_LE64(pucPos, psb->SB_ui64Generation);
    TPS_CPU_TO_LE64(pucPos, psb->SB_ui64TotalBlkCnt);
    TPS_CPU_TO_LE64(pucPos, psb->SB_ui64DataStartBlk);
    TPS_CPU_TO_LE64(pucPos, psb->SB_ui64DataBlkCnt);
    TPS_CPU_TO_LE64(pucPos, psb->SB_ui64LogStartBlk);
    TPS_CPU_TO_LE64(pucPos, psb->SB_ui64LogBlkCnt);
    TPS_CPU_TO_LE64(pucPos, psb->SB_ui64BPStartBlk);
    TPS_CPU_TO_LE64(pucPos, psb->SB_ui64BPBlkCnt);
    TPS_CPU_TO_LE64(pucPos, psb->SB_inumSpaceMng);
    TPS_CPU_TO_LE64(pucPos, psb->SB_inumRoot);
    TPS_CPU_TO_LE64(pucPos, psb->SB_inumDeleted);
}
/*********************************************************************************************************
** 函数名称: tpsFsMount
** 功能描述: 挂载tpsfs文件系统
** 输　入  : pDev         设备对象
**           uiFlags      挂载参数
**           ppsb         用于输出超级块指针
** 输　出  : 返回ERROR，成功ppsb输出超级块指针，失败ppsb输出LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsMount (PTPS_DEV pdev, UINT uiFlags, PTPS_SUPER_BLOCK *ppsb)
{
    PTPS_SUPER_BLOCK psb          = LW_NULL;
    PUCHAR           pucSectorBuf = LW_NULL;
    UINT             uiSectorSize = 0;
    UINT             uiSectorShift;

    if (LW_NULL == ppsb) {
        return  (EINVAL);
    }

    *ppsb = LW_NULL;

    if ((LW_NULL == pdev) || (LW_NULL == ppsb)) {
        return  (EINVAL);
    }

    if ((LW_NULL == pdev->DEV_SectorSize) ||
        (LW_NULL == pdev->DEV_ReadSector)) {                            /* 校验参数                     */
        return  (EINVAL);
    }

    if ((uiFlags & TPS_MOUNT_FLAG_WRITE) &&
        LW_NULL == pdev->DEV_WriteSector) {
        return  (EINVAL);
    }

    uiSectorSize = pdev->DEV_SectorSize(pdev);
    if ((uiSectorSize <= 0) || (uiSectorSize & (uiSectorSize - 1))) {   /* 必须是 2 的整数次方          */
        return  (EINVAL);
    }
    
    uiSectorShift = (UINT)archFindMsb((UINT32)uiSectorSize);
    if ((uiSectorShift <= 9) || (uiSectorShift >= 16)) {                /* sector 大小错误              */
        return  (TPS_ERR_SECTOR_SIZE);
    }
    uiSectorShift--;
    
    pucSectorBuf = (PUCHAR)TPS_ALLOC(uiSectorSize);
    if (LW_NULL == pucSectorBuf) {
        return  (ENOMEM);
    }

    if (pdev->DEV_ReadSector(pdev, pucSectorBuf,
                             TPS_SUPER_BLOCK_SECTOR, 1) != 0) {         /* 读取超级块                   */
        TPS_FREE(pucSectorBuf);
        return  (EIO);
    }

    psb = (PTPS_SUPER_BLOCK)TPS_ALLOC(sizeof(TPS_SUPER_BLOCK));
    if (LW_NULL == psb) {
        TPS_FREE(pucSectorBuf);
        return  (ENOMEM);
    }

    __tpsFsSBUnserial(psb, pucSectorBuf, uiSectorSize);

    if (psb->SB_uiMagic != TPS_MAGIC_SUPER_BLOCK1 &&
        psb->SB_uiMagic != TPS_MAGIC_SUPER_BLOCK2) {                    /* 校验Magic                    */
        _DebugHandle(__PRINTMESSAGE_LEVEL, "Magic number error, mount failed\r\n");
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (ERROR_NONE);
    }
    
    if (psb->SB_uiSecPerBlk & (psb->SB_uiSecPerBlk - 1)) {
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (ERROR_NONE);
    }

    TPS_FREE(pucSectorBuf);

    if (psb->SB_uiVersion > TPS_CUR_VERSION) {
        TPS_FREE(psb);
        _DebugFormat(__PRINTMESSAGE_LEVEL, "Mismatched tpsFs version! tpsFs version: %d, Partition version: %d\r\n",
                     TPS_CUR_VERSION, psb->SB_uiVersion);
        _DebugHandle(__PRINTMESSAGE_LEVEL, "Mount failed\r\n");
        return  (EFTYPE);

    } else if (psb->SB_uiVersion < TPS_CUR_VERSION) {
        _DebugFormat(__PRINTMESSAGE_LEVEL, "Old version compatibility mode! tpsFs version: %d, Partition version: %d\r\n",
                     TPS_CUR_VERSION, psb->SB_uiVersion);
    }

    pucSectorBuf = (PUCHAR)TPS_ALLOC(psb->SB_uiBlkSize);
    if (LW_NULL == pucSectorBuf) {
        TPS_FREE(psb);
        return  (ENOMEM);
    }
    psb->SB_pucSectorBuf = pucSectorBuf;

    psb->SB_uiSectorSize    = uiSectorSize;
    psb->SB_uiSectorShift   = uiSectorShift;
    psb->SB_uiSectorMask    = ((1 << uiSectorShift) - 1);
    
    psb->SB_uiBlkSize       = psb->SB_uiSecPerBlk * uiSectorSize;
    psb->SB_uiBlkShift      = (UINT)archFindMsb((UINT32)psb->SB_uiBlkSize) - 1;
    psb->SB_uiBlkMask       = ((1 << psb->SB_uiBlkShift) - 1);
    
    psb->SB_dev             = pdev;
    psb->SB_pinodeOpenList  = LW_NULL;
    psb->SB_uiInodeOpenCnt  = 0;
    psb->SB_uiFlags         = uiFlags;
    psb->SB_pinodeDeleted   = LW_NULL;
    psb->SB_pbp             = LW_NULL;

    if (tpsFsBtreeTransInit(psb) != TPS_ERR_NONE) {
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (ERROR_NONE);
    }

    if (tspFsCheckTrans(psb) != TPS_ERR_NONE) {                         /* 检查事务完整性 (无法修复)    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "check transaction failed!\r\n");
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (ERROR_NONE);
    }

    psb->SB_pinodeSpaceMng = tpsFsOpenInode(psb, psb->SB_inumSpaceMng); /* 打开空间管理inode            */
    if (LW_NULL == psb->SB_pinodeSpaceMng) {
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (ERROR_NONE);
    }

    psb->SB_pbp = (PTPS_BLK_POOL)TPS_ALLOC(sizeof(TPS_BLK_POOL));
    if (LW_NULL == psb->SB_pbp) {
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (ENOMEM);
    }

    if (tpsFsBtreeReadBP(psb) != TPS_ERR_NONE) {                        /* 读取块缓冲区                 */
        tpsFsCloseInode(psb->SB_pinodeSpaceMng);
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb->SB_pbp);
        TPS_FREE(psb);
        return  (ERROR_NONE);
    }

    *ppsb = psb;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsUnmount
** 功能描述: 卸载tpsfs文件系统
** 输　入  : psb         设备对象
** 输　出  : 成功返回0，失败返回错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsUnmount (PTPS_SUPER_BLOCK psb)
{
    if (LW_NULL == psb) {
        return  (EINVAL);
    }

    if (LW_NULL != psb->SB_pinodeDeleted) {
        tpsFsCloseInode(psb->SB_pinodeDeleted);
    }

    if (LW_NULL != psb->SB_pinodeSpaceMng) {
        tpsFsCloseInode(psb->SB_pinodeSpaceMng);
    }

    if (LW_NULL != psb->SB_pinodeOpenList) {                            /* 存在未关闭的inode            */
        return  (EBUSY);
    }

    if (tspFsCompleteTrans(psb) != TPS_ERR_NONE) {                      /* 标记事物为一致状态           */
        return  (EIO);
    }

    if (psb->SB_dev->DEV_Sync) {                                        /* 同步磁盘                     */
        if (psb->SB_dev->DEV_Sync(psb->SB_dev, 0, psb->SB_dev->DEV_SectorCnt(psb->SB_dev)) != 0) {
            return  (EIO);
        }
    }

    tpsFsBtreeTransFini(psb);

    if (LW_NULL != psb->SB_pucSectorBuf) {
        TPS_FREE(psb->SB_pucSectorBuf);
    }

    if (LW_NULL != psb->SB_pbp) {
        TPS_FREE(psb->SB_pbp);
    }

    TPS_FREE(psb);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsFormat
** 功能描述: 格式化tpsfs文件系统
** 输　入  : pDev         设备对象
**           uiBlkSize    块大小
**           uiLogSize    日志空间大小
** 输　出  : 成功返回0，失败返回错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsFormat (PTPS_DEV pdev, UINT uiBlkSize)
{
    PTPS_SUPER_BLOCK    psb          = LW_NULL;
    PUCHAR              pucSectorBuf = LW_NULL;
    UINT                uiSectorSize;
    UINT64              uiTotalBlkCnt;
    UINT                uiSctPerBlk;
    UINT64              uiLogBlkCnt;
    UINT64              uiLogSize;
    INT                 iMode = 0;
    errno_t             iErr  = ERROR_NONE;

    
    if ((LW_NULL == pdev) || (LW_NULL == pdev->DEV_WriteSector)) {
        return  (EINVAL);
    }
    
    uiSectorSize = pdev->DEV_SectorSize(pdev);
    if ((uiSectorSize <= 0) || (uiSectorSize & (uiSectorSize - 1))) {
        return  (EINVAL);
    }
    
    if ((uiBlkSize == 0) || (uiBlkSize % uiSectorSize) ||
        (uiBlkSize < TPS_MIN_BLK_SIZE)) {
        return  (EINVAL);
    }

    /*
     * 磁盘最小为 4MB
     */
    uiSctPerBlk   = uiBlkSize / uiSectorSize;
    uiTotalBlkCnt = pdev->DEV_SectorCnt(pdev) / uiSctPerBlk;
    if (uiTotalBlkCnt < 1024) {
        return  (ENOSPC);
    }
    
    /*
     * logsize 为磁盘的 1/16
     */
    uiLogBlkCnt = uiTotalBlkCnt >> 4;
    uiLogSize   = uiLogBlkCnt * uiBlkSize;
    if (uiLogSize < TPS_MIN_LOG_SIZE) {
        uiLogSize = TPS_MIN_LOG_SIZE;
    }

    /*
     *  结构体赋值
     */
    psb = (PTPS_SUPER_BLOCK)TPS_ALLOC(sizeof(TPS_SUPER_BLOCK));
    if (LW_NULL == psb) {
        return  (ENOMEM);
    }

    psb->SB_uiMagic             = TPS_MAGIC_SUPER_BLOCK2;
    psb->SB_uiVersion           = TPS_CUR_VERSION;
    psb->SB_ui64Generation      = TPS_UTC_TIME();
    psb->SB_uiSectorSize        = uiSectorSize;
    psb->SB_uiSectorShift       = (UINT)archFindMsb((UINT32)uiSectorSize) - 1;
    psb->SB_uiSectorMask        = ((1 << psb->SB_uiSectorShift) - 1);
    psb->SB_uiSecPerBlk         = uiSctPerBlk;
    psb->SB_uiBlkSize           = uiBlkSize;
    psb->SB_uiBlkShift          = (UINT)archFindMsb((UINT32)psb->SB_uiBlkSize) - 1;
    psb->SB_uiBlkMask           = ((1 << psb->SB_uiBlkShift) - 1);
    psb->SB_uiFlags             = TPS_MOUNT_FLAG_READ | TPS_MOUNT_FLAG_WRITE;
    psb->SB_ui64TotalBlkCnt     = uiTotalBlkCnt;
    psb->SB_ui64DataStartBlk    = TPS_DATASTART_BLK;
    psb->SB_ui64DataBlkCnt      = uiTotalBlkCnt - TPS_DATASTART_BLK - uiLogBlkCnt;
    psb->SB_ui64LogStartBlk     = uiTotalBlkCnt - uiLogBlkCnt;
    psb->SB_ui64LogBlkCnt       = uiLogBlkCnt;
    psb->SB_ui64BPStartBlk      = TPS_BPSTART_BLK;
    psb->SB_ui64BPBlkCnt        = TPS_BPSTART_CNT;
    psb->SB_inumSpaceMng        = TPS_SPACE_MNG_INUM;
    psb->SB_inumRoot            = TPS_ROOT_INUM;
    psb->SB_inumDeleted         = 0;
    psb->SB_pinodeOpenList      = LW_NULL;
    psb->SB_dev                 = pdev;
    psb->SB_uiInodeOpenCnt      = 0;
    psb->SB_pinodeDeleted       = LW_NULL;
    psb->SB_pbp                 = LW_NULL;

    pucSectorBuf = (PUCHAR)TPS_ALLOC(uiBlkSize);
    if (LW_NULL == pucSectorBuf) {
        TPS_FREE(psb);
        return  (ENOMEM);
    }
    psb->SB_pucSectorBuf = pucSectorBuf;

    if (tpsFsBtreeTransInit(psb) != TPS_ERR_NONE) {
        TPS_FREE(psb);
        return  (ENOMEM);
    }

    /*
     *  初始化空间管理inode和root inode
     */
    iMode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
    if (tpsFsCreateInode(LW_NULL, psb, psb->SB_inumSpaceMng, iMode) != TPS_ERR_NONE) {
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (EIO);
    }
    
    psb->SB_pinodeSpaceMng = tpsFsOpenInode(psb, psb->SB_inumSpaceMng);
    if (psb->SB_pinodeSpaceMng == LW_NULL) {
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (EIO);
    }

    if (tpsFsBtreeInitBP(psb,
                         psb->SB_ui64DataStartBlk,
                         TPS_ADJUST_BP_BLK) != TPS_ERR_NONE) {          /* 初始化块缓冲区               */
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (EIO);
    }

    psb->SB_pbp = (PTPS_BLK_POOL)TPS_ALLOC(sizeof(TPS_BLK_POOL));
    if (LW_NULL == psb->SB_pbp) {
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (ENOMEM);
    }

    if (tpsFsBtreeReadBP(psb) != TPS_ERR_NONE) {                        /* 读取块缓冲区                 */
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb->SB_pbp);
        TPS_FREE(psb);
        return  (EIO);
    }

    if (tpsFsBtreeFreeBlk(LW_NULL,
                         psb->SB_pinodeSpaceMng,
                         psb->SB_ui64DataStartBlk + TPS_ADJUST_BP_BLK,
                         psb->SB_ui64DataStartBlk + TPS_ADJUST_BP_BLK,
                         psb->SB_ui64DataBlkCnt - TPS_ADJUST_BP_BLK,
                         LW_TRUE) != TPS_ERR_NONE) {
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb->SB_pbp);
        TPS_FREE(psb);
        return  (EIO);
    }

    iMode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
    iErr  = tpsFsCreateInode(LW_NULL, psb, psb->SB_inumRoot, iMode);
    if (iErr != ERROR_NONE) {
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb->SB_pbp);
        TPS_FREE(psb);
        return  (iErr);
    }

    tpsFsCloseInode(psb->SB_pinodeSpaceMng);

    lib_bzero(pucSectorBuf, uiSectorSize);

    __tpsFsSBSerial(psb, pucSectorBuf, uiSectorSize);

    if (pdev->DEV_WriteSector(pdev, pucSectorBuf, TPS_SUPER_BLOCK_SECTOR,
                              1, LW_TRUE) != 0) {                       /* 写super block扇区            */
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb->SB_pbp);
        TPS_FREE(psb);
        return  (EIO);
    }

    tpsFsBtreeTransFini(psb);
    TPS_FREE(pucSectorBuf);
    TPS_FREE(psb->SB_pbp);
    TPS_FREE(psb);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsFlushSuperBlock
** 功能描述: flush超级块
** 输　入  : psb          超级块指针
**           ptrans       事物指针
** 输　出  : 成功返回0，失败返回错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT tpsFsFlushSuperBlock (TPS_TRANS *ptrans, PTPS_SUPER_BLOCK psb)
{
    __tpsFsSBSerial(psb, psb->SB_pucSectorBuf, psb->SB_uiSectorSize);

    if (tpsFsTransWrite(ptrans, psb, TPS_SUPER_BLOCK_NUM,
                        0, psb->SB_pucSectorBuf, psb->SB_uiSectorSize) != 0) {
        return  (TPS_ERR_WRITE_DEV);
    }

    return  (TPS_ERR_NONE);
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
