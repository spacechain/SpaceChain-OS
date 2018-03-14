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
** 文   件   名: tpsfs_dir.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: tpsfs 目录操作

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
** 函数名称: __tpsFsLE32ToCpuVal
** 功能描述: 小端数组转主机序整型，字符串哈希算法中不能使用TPS_LE32_TO_CPU_VAL，部分体现结果存在对齐问题
** 输　入  : pucData   小端数组
** 输　出  : 主机序整型
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT __tpsFsLE32ToCpuVal (const UCHAR *pucData)
{
    UINT uiRet = 0;

    uiRet  = ((UINT)pucData[3]) << 24;
    uiRet += ((UINT)pucData[2]) << 16;
    uiRet += ((UINT)pucData[1]) << 8;
    uiRet += ((UINT)pucData[0]);

    return  (uiRet);
}
/*********************************************************************************************************
** 函数名称: __tpsFsGetHash
** 功能描述: 获取文件名hash值 (murmurhash算法)
** 输　入  : pcFileName       文件名
** 输　出  : 文件名hash值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_IBLK __tpsFsGetHash (CPCHAR pcFileName)
{
    const UINT  uiM     = 0x5BD1E995;
    const UINT  uiSeed  = 0xEE6B27EB;
    const INT   iR      = 24;
    INT         iLen    = lib_strlen(pcFileName);

    UINT        uiH1    = uiSeed ^ iLen;
    UINT        uiH2    = 0;
    UINT64      ui64H   = 0;

    UINT        uiK1;
    UINT        uiK2;

    const UCHAR *pucData = (const UCHAR *)pcFileName;

    while(iLen >= 8) {
        uiK1     = __tpsFsLE32ToCpuVal(pucData);
        pucData += 4;
        uiK1 *= uiM;
        uiK1 ^= uiK1 >> iR;
        uiK1 *= uiM;
        uiH1 *= uiM;
        uiH1 ^= uiK1;
        iLen -= 4;

        uiK2     = __tpsFsLE32ToCpuVal(pucData);
        pucData += 4;
        uiK2 *= uiM;
        uiK2 ^= uiK2 >> iR;
        uiK2 *= uiM;
        uiH2 *= uiM;
        uiH2 ^= uiK2;
        iLen -= 4;
    }

    if(iLen >= 4) {
        uiK1     = __tpsFsLE32ToCpuVal(pucData);
        pucData += 4;
        uiK1 *= uiM;
        uiK1 ^= uiK1 >> iR;
        uiK1 *= uiM;
        uiH1 *= uiM;
        uiH1 ^= uiK1;
        iLen -= 4;
    }

    switch(iLen) {
    case 3:
        uiH2 ^= ((UINT)pucData[2]) << 16;
    case 2:
        uiH2 ^= ((UINT)pucData[1]) << 8;
    case 1:
        uiH2 ^= ((UINT)pucData[0]);
        uiH2 *= uiM;
    }

    uiH1 ^= uiH2 >> 18; uiH1 *= uiM;
    uiH2 ^= uiH1 >> 22; uiH2 *= uiM;
    uiH1 ^= uiH2 >> 17; uiH1 *= uiM;
    uiH2 ^= uiH1 >> 19; uiH2 *= uiM;

    ui64H = uiH1;
    ui64H = (ui64H << 32) | uiH2;

    return  ((TPS_IBLK)ui64H & 0x7FFFFFFFFFFFFFFFull);                  /* 最高位保留，返回有符号正数   */
}
/*********************************************************************************************************
** 函数名称: __tpsFsHashEntryCreate
** 功能描述: 在hash表中创建目录
** 输　入  : ptrans           事物
**           pinodeHash       hash根节点
**           pcFileName       文件名
**           inum             文件号
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsHashEntryCreate (PTPS_TRANS    ptrans,
                                           PTPS_INODE    pinodeHash,
                                           CPCHAR        pcFileName,
                                           TPS_INUM      inum)
{
    TPS_IBLK            blkKey          = 0;
    TPS_IBLK            blkPsc          = 0;
    TPS_IBLK            blkCnt          = 0;
    UINT                uiEntryLen      = 0;
    PUCHAR              pucPos          = LW_NULL;
    PTPS_SUPER_BLOCK    psb             = pinodeHash->IND_psb;
    TPS_IBLK            blkHash         = __tpsFsGetHash(pcFileName);
    TPS_RESULT          tpsres;

    tpsres = tpsFsBtreeGetNode(pinodeHash, blkHash, &blkKey, &blkPsc, &blkCnt);

    if (tpsres != TPS_ERR_NONE && tpsres != TPS_ERR_BTREE_OVERFLOW) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (blkKey == blkHash) {                                            /* hash值对应的key已存在        */
        return  (TPS_ERR_HASH_EXIST);
    }

    uiEntryLen = sizeof(UINT32) + sizeof(TPS_INUM) + lib_strlen(pcFileName) + 1;

    if (uiEntryLen > psb->SB_uiBlkSize) {
        return  (TPS_ERR_HASH_TOOLONG_NAME);
    }

    if (tpsFsInodeAllocBlk(ptrans, psb,
                           0, 1, &blkPsc, &blkCnt) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_ALLOC);
    }

    if (tpsFsBtreeInsertNode(ptrans, pinodeHash, blkHash,
                             blkPsc, blkCnt) != TPS_ERR_NONE) {         /* 使用btree保存哈希表          */
        return  (TPS_ERR_HASH_INSERT);
    }

    pucPos = pinodeHash->IND_pucBuff;
    TPS_CPU_TO_LE32(pucPos, uiEntryLen);
    TPS_CPU_TO_IBLK(pucPos, inum);
    lib_strcpy((PCHAR)pucPos, pcFileName);

    return  (tpsFsTransWrite(ptrans, psb, blkPsc, 0,
                             pinodeHash->IND_pucBuff, uiEntryLen));
}
/*********************************************************************************************************
** 函数名称: __tpsFsHashEntryRemove
** 功能描述: 从hash表中删除目录
** 输　入  : ptrans           事物
**           pinodeHash       hash表根节点
**           pentry           entry指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsHashEntryRemove (PTPS_TRANS ptrans,
                                           PTPS_INODE pinodeHash,
                                           PTPS_ENTRY pentry)
{
    TPS_IBLK            blkKey      = 0;
    TPS_IBLK            blkPsc      = 0;
    TPS_IBLK            blkCnt      = 0;
    UINT                uiEntryLen  = 0;
    TPS_INUM            inum        = 0;
    PTPS_SUPER_BLOCK    psb         = pinodeHash->IND_psb;
    TPS_IBLK            blkHash     = __tpsFsGetHash(pentry->ENTRY_pcName);
    PUCHAR              pucPos      = LW_NULL;

    if (tpsFsBtreeGetNode(pinodeHash, blkHash, &blkKey,
                          &blkPsc, &blkCnt) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (blkKey != blkHash) {                                            /* hash值对应的key不存在        */
        return  (TPS_ERR_HASH_NOT_EXIST);
    }

    if (tpsFsTransRead(psb, blkPsc, 0,
                       pinodeHash->IND_pucBuff, pentry->ENTRY_uiLen) != TPS_ERR_NONE) {
        return  (TPS_ERR_TRANS_READ);
    }

    pucPos = pinodeHash->IND_pucBuff;
    TPS_LE32_TO_CPU(pucPos, uiEntryLen);
    if (uiEntryLen != (sizeof(UINT32) + sizeof(TPS_INUM) + lib_strlen(pentry->ENTRY_pcName) + 1)) {
        return  (TPS_ERR_ENTRY_UNEQUAL);
    }

    TPS_LE64_TO_CPU(pucPos, inum);
    if (inum != pentry->ENTRY_inum) {
        return  (TPS_ERR_ENTRY_UNEQUAL);
    }

    if (lib_strcmp(pentry->ENTRY_pcName, (CPCHAR)pucPos) != 0) {        /* 文件名不匹配                 */
        return  (TPS_ERR_ENTRY_UNEQUAL);
    }

    if (tpsFsBtreeRemoveNode(ptrans, pinodeHash, blkKey,
                             blkPsc, blkCnt) != TPS_ERR_NONE) {         /* 删除hash值节点               */
        return  (TPS_ERR_HASH_REMOVE);
    }

    if (tpsFsBtreeFreeBlk(ptrans, psb->SB_pinodeSpaceMng, blkPsc,
                          blkPsc, blkCnt, LW_FALSE) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_FREE);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsHashEntryFind
** 功能描述: 在hash表中查找目录
** 输　入  : pinodeDir        父目录inode指针
**           pinodeHash       hash表根节点
**           pcFileName       文件名
**           ppentry          返回目录结构指针，失败返回NULL
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsHashEntryFind(PTPS_INODE  pinodeDir,
                                        PTPS_INODE  pinodeHash,
                                        CPCHAR      pcFileName,
                                        PTPS_ENTRY *ppentry)
{
    TPS_IBLK            blkKey          = 0;
    TPS_IBLK            blkPsc          = 0;
    TPS_IBLK            blkCnt          = 0;
    UINT                uiEntryLenRd    = 0;
    UINT                uiEntryLen      = 0;
    TPS_INUM            inum            = 0;
    PTPS_SUPER_BLOCK    psb             = pinodeHash->IND_psb;
    TPS_IBLK            blkHash         = __tpsFsGetHash(pcFileName);
    PUCHAR              pucPos          = LW_NULL;
    PTPS_ENTRY          pentry          = LW_NULL;

    *ppentry = LW_NULL;

    if (tpsFsBtreeGetNode(pinodeHash, blkHash, &blkKey,
                          &blkPsc, &blkCnt) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (blkKey != blkHash) {                                            /* hash值对应的key不存在        */
        return  (TPS_ERR_HASH_NOT_EXIST);
    }

    uiEntryLenRd = sizeof(UINT32) + sizeof(TPS_INUM) + lib_strlen(pcFileName) + 1;
    if (tpsFsTransRead(psb, blkPsc, 0,
                       pinodeHash->IND_pucBuff,
                       uiEntryLenRd) != TPS_ERR_NONE) {
        return  (TPS_ERR_TRANS_READ);
    }

    pucPos = pinodeHash->IND_pucBuff;

    TPS_LE32_TO_CPU(pucPos, uiEntryLen);
    if (uiEntryLen != uiEntryLenRd) {
        return  (TPS_ERR_ENTRY_UNEQUAL);
    }

    TPS_LE64_TO_CPU(pucPos, inum);

    if (lib_strcmp(pcFileName, (CPCHAR)pucPos) != 0) {                  /* 文件名不匹配                 */
        return  (TPS_ERR_ENTRY_UNEQUAL);
    }

    /*
     *  构建entry对象
     */
    pentry = (PTPS_ENTRY)TPS_ALLOC(sizeof(TPS_ENTRY) + lib_strlen(pcFileName) + 1);
    if (LW_NULL == pentry) {
        return  (TPS_ERR_ALLOC);
    }
    lib_bzero(pentry, sizeof(TPS_ENTRY) + lib_strlen(pcFileName) + 1);

    pentry->ENTRY_offset    = blkKey;
    pentry->ENTRY_bInHash   = LW_TRUE;
    pentry->ENTRY_psb       = psb;
    pentry->ENTRY_inumDir   = pinodeDir->IND_inum;
    pentry->ENTRY_uiLen     = uiEntryLen;
    pentry->ENTRY_inum      = inum;
    lib_strcpy(pentry->ENTRY_pcName, (PCHAR)pucPos);

    pentry->ENTRY_pinode = tpsFsOpenInode(pentry->ENTRY_psb, pentry->ENTRY_inum);
    if (LW_NULL == pentry->ENTRY_pinode) {
        TPS_FREE(pentry);
        return  (TPS_ERR_INODE_OPEN);
    }

    *ppentry = pentry;

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsHashEntryRead
** 功能描述: 在hash表中从指定偏移读取目录
** 输　入  : pinodeDir        父目录inode指针
**           pinodeHash       hash表根节点
**           off              目录偏移量
**           ppentry          返回目录结构指针，失败返回NULL
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsHashEntryRead(PTPS_INODE  pinodeDir,
                                        PTPS_INODE  pinodeHash,
                                        TPS_OFF_T   off,
                                        PTPS_ENTRY *ppentry)
{
    TPS_IBLK            blkKey      = 0;
    TPS_IBLK            blkPsc      = 0;
    TPS_IBLK            blkCnt      = 0;
    UINT                uiEntryLen  = 0;
    TPS_INUM            inum        = 0;
    PTPS_SUPER_BLOCK    psb         = pinodeHash->IND_psb;
    TPS_IBLK            blkHash     = (TPS_IBLK)off;                    /* 偏移量即hash值               */
    PUCHAR              pucPos      = LW_NULL;
    PTPS_ENTRY          pentry      = LW_NULL;

    *ppentry = LW_NULL;

__retry_find:
    if (tpsFsBtreeGetNode(pinodeHash, blkHash, &blkKey,
                          &blkPsc, &blkCnt) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (tpsFsTransRead(psb, blkPsc, 0,
                       pinodeHash->IND_pucBuff,
                       psb->SB_uiBlkSize) != TPS_ERR_NONE) {
        return  (TPS_ERR_TRANS_READ);
    }

    pucPos = pinodeHash->IND_pucBuff;
    TPS_LE32_TO_CPU(pucPos, uiEntryLen);
    TPS_LE64_TO_CPU(pucPos, inum);

    /*
     *  构建entry对象
     */
    pentry = (PTPS_ENTRY)TPS_ALLOC(sizeof(TPS_ENTRY) + uiEntryLen);
    if (LW_NULL == pentry) {
        return  (TPS_ERR_ALLOC);
    }
    lib_bzero(pentry, sizeof(TPS_ENTRY) + uiEntryLen);

    pentry->ENTRY_offset    = blkKey;
    pentry->ENTRY_bInHash   = LW_TRUE;
    pentry->ENTRY_psb       = psb;
    pentry->ENTRY_inumDir   = pinodeDir->IND_inum;
    pentry->ENTRY_uiLen     = uiEntryLen;
    pentry->ENTRY_inum      = inum;
    lib_strncpy(pentry->ENTRY_pcName, (PCHAR)pucPos, uiEntryLen);

    pentry->ENTRY_pinode = tpsFsOpenInode(pentry->ENTRY_psb, pentry->ENTRY_inum);
    if (LW_NULL == pentry->ENTRY_pinode) {
        blkHash = blkKey - 1;
        TPS_FREE(pentry);
        goto  __retry_find;
    }

    *ppentry = pentry;

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsCreateEntry
** 功能描述: 创建文件或目录
** 输　入  : ptrans           事物
**           pinodeDir        父目录
**           pcFileName       文件名
**           inum             文件号
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsCreateEntry (PTPS_TRANS    ptrans,
                              PTPS_INODE    pinodeDir,
                              CPCHAR        pcFileName,
                              TPS_INUM      inum)
{
    TPS_SIZE_T szDir;
    TPS_OFF_T  off          = 0;
    UINT       uiEntryLen   = 0;
    UINT       uiItemCnt    = 0;
    UINT       i            = 0;
    PUCHAR     pucItemBuf   = LW_NULL;
    PUCHAR     pucPos       = LW_NULL;
    TPS_RESULT tpsres       = TPS_ERR_NONE;

    if ((ptrans == LW_NULL) || (pinodeDir == LW_NULL) || (pcFileName == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    uiEntryLen = sizeof(UINT32) + sizeof(TPS_INUM) + lib_strlen(pcFileName) + 1;

    if (pinodeDir->IND_pinodeHash &&
        uiEntryLen <= pinodeDir->IND_psb->SB_uiBlkSize) {               /* 在hash表中创建目录           */
        tpsres = __tpsFsHashEntryCreate(ptrans, pinodeDir->IND_pinodeHash,
                                        pcFileName, inum);

        if (tpsres != TPS_ERR_HASH_EXIST  &&
            tpsres != TPS_ERR_HASH_TOOLONG_NAME) {
            return  (tpsres);
        }
    }

    /*
     *  hash值已存在或文件名太长时在目录文件中创建entry
     */
    uiItemCnt  = uiEntryLen >> TPS_ENTRY_ITEM_SHIFT;                    /* 计算entry占用的item数目      */
    if ((uiEntryLen & TPS_ENTRY_ITEM_MASK) != 0) {
        uiItemCnt++;
    }

    pucItemBuf = (PUCHAR)TPS_ALLOC(TPS_ENTRY_ITEM_SIZE);
    if (LW_NULL == pucItemBuf) {
        return  (TPS_ERR_ALLOC);
    }

    /*
     *  使用首次适应算法查找足够长的空间
     */
    szDir = tpsFsInodeGetSize(pinodeDir);
    while (off < szDir) {
        for (i = 0; i < uiItemCnt; i++) {
            if (tpsFsInodeRead(pinodeDir,
                               off + i * TPS_ENTRY_ITEM_SIZE,
                               pucItemBuf,
                               TPS_ENTRY_ITEM_SIZE,
                               LW_TRUE) < TPS_ENTRY_ITEM_SIZE) {
                break;
            }

            if (TPS_LE32_TO_CPU_VAL(pucItemBuf) != 0) {
                break;
            }
        }

        if ((i == uiItemCnt) || ((off + (i + 1) * TPS_ENTRY_ITEM_SIZE) > szDir)) {
            break;
        }

        off += (i + 1) * TPS_ENTRY_ITEM_SIZE;
    }

    TPS_FREE(pucItemBuf);

    pucItemBuf = (PUCHAR)TPS_ALLOC(TPS_ENTRY_ITEM_SIZE * uiItemCnt);
    if (LW_NULL == pucItemBuf) {
        return  (TPS_ERR_ALLOC);
    }
    lib_bzero(pucItemBuf, TPS_ENTRY_ITEM_SIZE * uiItemCnt);

    /*
     *  写入新entry到目录inode
     */
    pucPos = pucItemBuf;
    TPS_CPU_TO_LE32(pucPos, uiEntryLen);
    TPS_CPU_TO_IBLK(pucPos, inum);
    lib_strcpy((PCHAR)pucPos, pcFileName);
    
    if (tpsFsInodeWrite(ptrans, pinodeDir,
                        off, pucItemBuf,
                        TPS_ENTRY_ITEM_SIZE * uiItemCnt,
                        LW_TRUE) < (TPS_ENTRY_ITEM_SIZE * uiItemCnt)) {
        TPS_FREE(pucItemBuf);
        return  (TPS_ERR_INODE_WRITE);
    }
    
    TPS_FREE(pucItemBuf);
    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsEntryRemove
** 功能描述: 删除文件或目录
** 输　入  : ptrans           事物
**           pentry           entry指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsEntryRemove (PTPS_TRANS ptrans, PTPS_ENTRY pentry)
{
    UINT       uiEntryLen   = 0;
    UINT       uiItemCnt    = 0;
    PUCHAR     pucItemBuf   = LW_NULL;
    PTPS_INODE pinodeDir    = LW_NULL;
    TPS_RESULT tpsres;

    if ((ptrans == LW_NULL) || (pentry == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    pinodeDir = tpsFsOpenInode(pentry->ENTRY_psb, pentry->ENTRY_inumDir);
    if (LW_NULL == pinodeDir) {
        return  (TPS_ERR_INODE_OPEN);
    }

    if (pentry->ENTRY_bInHash) {                                        /* entry在hash表中              */
        if (LW_NULL == pinodeDir->IND_pinodeHash) {
            tpsFsCloseInode(pinodeDir);
            return  (TPS_ERR_PARAM);
        }

        tpsres = __tpsFsHashEntryRemove(ptrans, pinodeDir->IND_pinodeHash, pentry);

        tpsFsCloseInode(pinodeDir);

        return  (tpsres);
    }

    uiEntryLen = sizeof(UINT32) + sizeof(TPS_INUM) + lib_strlen(pentry->ENTRY_pcName) + 1;
    uiItemCnt  = uiEntryLen >> TPS_ENTRY_ITEM_SHIFT;                    /* 计算entry占用的item数目      */
    if ((uiEntryLen & TPS_ENTRY_ITEM_MASK) != 0) {
        uiItemCnt++;
    }

    pucItemBuf = (PUCHAR)TPS_ALLOC(TPS_ENTRY_ITEM_SIZE * uiItemCnt);
    if (LW_NULL == pucItemBuf) {
        tpsFsCloseInode(pinodeDir);
        return  (TPS_ERR_ALLOC);
    }
    lib_bzero(pucItemBuf, TPS_ENTRY_ITEM_SIZE * uiItemCnt);

    if (tpsFsInodeWrite(ptrans, pinodeDir,
                        pentry->ENTRY_offset, pucItemBuf,
                        TPS_ENTRY_ITEM_SIZE * uiItemCnt,
                        LW_TRUE) < (TPS_ENTRY_ITEM_SIZE * uiItemCnt)) {
        tpsFsCloseInode(pinodeDir);
        TPS_FREE(pucItemBuf);
        return  (TPS_ERR_INODE_WRITE);
    }

    tpsFsCloseInode(pinodeDir);
    TPS_FREE(pucItemBuf);
    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsGetDirSize
** 功能描述: 获取最后一个有效目录项结束位置
** 输　入  : pinodeDir        目录文件
** 输　出  : 小于0--失败，大于或等于0--最后一个有效目录项结束位置
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_SIZE_T  tpsFsGetDirSize (PTPS_INODE pinodeDir)
{
    PUCHAR     pucItemBuf   = LW_NULL;
    TPS_SIZE_T off;

    if (pinodeDir == LW_NULL) {
        return  (-1);
    }

    pucItemBuf = (PUCHAR)TPS_ALLOC(TPS_ENTRY_ITEM_SIZE);
    if (LW_NULL == pucItemBuf) {
        return  (-1);
    }

    off = tpsFsInodeGetSize(pinodeDir);
    while (off >= TPS_ENTRY_ITEM_SIZE) {                                /* 查找最后一个目录项           */
        off -= TPS_ENTRY_ITEM_SIZE;
        if (tpsFsInodeRead(pinodeDir,
                           off,
                           pucItemBuf,
                           TPS_ENTRY_ITEM_SIZE,
                           LW_TRUE) < TPS_ENTRY_ITEM_SIZE) {
            break;
        }

        if (TPS_LE32_TO_CPU_VAL(pucItemBuf) != 0) {
            break;
        }
    }

    TPS_FREE(pucItemBuf);

    return  (off + TPS_ENTRY_ITEM_SIZE);
}
/*********************************************************************************************************
** 函数名称: tpsFsFindEntry
** 功能描述: 查找entry
** 输　入  : pinodeDir           父目录
**           pcFileName          文件名
**           ppentry             返回目录结构指针，失败返回NULL
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsFindEntry (PTPS_INODE pinodeDir, CPCHAR pcFileName, PTPS_ENTRY *ppentry)
{
    TPS_SIZE_T szDir;
    TPS_OFF_T  off          = 0;
    UINT       uiEntryLen   = 0;
    UINT       uiItemCnt    = 0;
    PUCHAR     pucItemBuf   = LW_NULL;
    PUCHAR     pucPos       = LW_NULL;
    PTPS_ENTRY pentry       = LW_NULL;

    if ((pinodeDir == LW_NULL) || (pcFileName == LW_NULL) || (ppentry == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    *ppentry = LW_NULL;

    if (pinodeDir->IND_pinodeHash) {                                    /* 在hash表中查找目录           */
        if (__tpsFsHashEntryFind(pinodeDir, pinodeDir->IND_pinodeHash,
                                 pcFileName, ppentry) == TPS_ERR_NONE) {
            return  (TPS_ERR_NONE);
        }
    }

    uiEntryLen = sizeof(UINT32) + sizeof(TPS_INUM) + lib_strlen(pcFileName) + 1;
    uiItemCnt  = uiEntryLen >> TPS_ENTRY_ITEM_SHIFT;                    /* 计算entry占用的item数目      */
    if ((uiEntryLen & TPS_ENTRY_ITEM_MASK) != 0) {
        uiItemCnt++;
    }

    pucItemBuf = (PUCHAR)TPS_ALLOC(TPS_ENTRY_ITEM_SIZE * uiItemCnt);
    if (LW_NULL == pucItemBuf) {
        return  (TPS_ERR_ALLOC);
    }

    /*
     *  查找entry
     */
    szDir = tpsFsInodeGetSize(pinodeDir);
    while (off < szDir) {
        if (tpsFsInodeRead(pinodeDir,
                           off,
                           pucItemBuf,
                           TPS_ENTRY_ITEM_SIZE,
                           LW_TRUE) < TPS_ENTRY_ITEM_SIZE) {
            TPS_FREE(pucItemBuf);
            return  (TPS_ERR_INODE_READ);
        }

        if (TPS_LE32_TO_CPU_VAL(pucItemBuf) != uiEntryLen) {
            off += (TPS_LE32_TO_CPU_VAL(pucItemBuf) == 0 ? TPS_ENTRY_ITEM_SIZE : TPS_LE32_TO_CPU_VAL(pucItemBuf));
            if (off & TPS_ENTRY_ITEM_MASK) {
                off = TPS_ROUND_UP(off, TPS_ENTRY_ITEM_SIZE);
            }
            continue;
        }

        if (tpsFsInodeRead(pinodeDir,
                           off,
                           pucItemBuf,
                           uiEntryLen,
                           LW_TRUE) < uiEntryLen) {
            TPS_FREE(pucItemBuf);
            return  (TPS_ERR_INODE_READ);
        }

        if (lib_strcmp((PCHAR)pucItemBuf + sizeof(UINT32) + sizeof(TPS_INUM), pcFileName) != 0) {
            off += uiEntryLen;
            if (off & TPS_ENTRY_ITEM_MASK) {
                off = TPS_ROUND_UP(off, TPS_ENTRY_ITEM_SIZE);
            }
            continue;
        }
        
        break;
    }

    if (off >= szDir) {
        TPS_FREE(pucItemBuf);
        return  (TPS_ERR_ENTRY_NOT_EXIST);
    }

    /*
     *  构建entry对象
     */
    pentry = (PTPS_ENTRY)TPS_ALLOC(sizeof(TPS_ENTRY) + lib_strlen(pcFileName) + 1);
    if (LW_NULL == pentry) {
        TPS_FREE(pucItemBuf);
        return  (TPS_ERR_ALLOC);
    }
    lib_bzero(pentry, sizeof(TPS_ENTRY) + lib_strlen(pcFileName) + 1);
    
    pentry->ENTRY_offset    = off;
    pentry->ENTRY_bInHash   = LW_FALSE;
    pentry->ENTRY_psb       = pinodeDir->IND_psb;
    pentry->ENTRY_inumDir   = pinodeDir->IND_inum;
    pucPos = pucItemBuf;
    TPS_LE32_TO_CPU(pucPos, pentry->ENTRY_uiLen);
    TPS_IBLK_TO_CPU(pucPos, pentry->ENTRY_inum);
    lib_strcpy(pentry->ENTRY_pcName, (PCHAR)pucPos);

    TPS_FREE(pucItemBuf);

    pentry->ENTRY_pinode = tpsFsOpenInode(pentry->ENTRY_psb, pentry->ENTRY_inum);
    if (LW_NULL == pentry->ENTRY_pinode) {
        TPS_FREE(pentry);
        return  (TPS_ERR_INODE_OPEN);
    }
    
    *ppentry = pentry;

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsEntryRead
** 功能描述: 读取entry
** 输　入  : pinodeDir           父目录
**           bInHash             目录是否在hash表中
**           off                 偏移位置
**           ppentry             返回目录结构指针，失败返回NULL
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsEntryRead (PTPS_INODE pinodeDir, BOOL bInHash, TPS_OFF_T off, PTPS_ENTRY *ppentry)
{
    TPS_SIZE_T szDir;
    PUCHAR     pucItemBuf   = LW_NULL;
    UINT       uiEntryLen   = 0;
    PUCHAR     pucPos       = LW_NULL;
    PTPS_ENTRY pentry       = LW_NULL;
    
    if ((pinodeDir == LW_NULL) || (ppentry == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    *ppentry = LW_NULL;

    if (bInHash) {                                                      /* off在hash表中                */
        if (LW_NULL != pinodeDir->IND_pinodeHash) {
            if (__tpsFsHashEntryRead(pinodeDir, pinodeDir->IND_pinodeHash,
                                     off, ppentry) == TPS_ERR_NONE) {
                return  (TPS_ERR_NONE);
            }
        }

        off     = 0;
        bInHash = LW_FALSE;
    }

    if (off & TPS_ENTRY_ITEM_MASK) {
        off = TPS_ROUND_UP(off, TPS_ENTRY_ITEM_SIZE);
    }

__retry_find:
    pucItemBuf = (PUCHAR)TPS_ALLOC(TPS_ENTRY_ITEM_SIZE);
    if (LW_NULL == pucItemBuf) {
        return  (TPS_ERR_ALLOC);
    }
    
    /*
     *  查找entry
     */
    szDir = tpsFsInodeGetSize(pinodeDir);
    while (off < szDir) {
        if (tpsFsInodeRead(pinodeDir,
                           off,
                           pucItemBuf,
                           TPS_ENTRY_ITEM_SIZE,
                           LW_TRUE) < TPS_ENTRY_ITEM_SIZE) {
            TPS_FREE(pucItemBuf);
            return  (TPS_ERR_INODE_READ);
        }

        uiEntryLen = TPS_LE32_TO_CPU_VAL(pucItemBuf);
        if (uiEntryLen != 0) {
            break;
        }

        off += TPS_ENTRY_ITEM_SIZE;
    }

    if ((off >= szDir) || (uiEntryLen == 0)) {
        TPS_FREE(pucItemBuf);
        return  (TPS_ERR_ENTRY_NOT_EXIST);
    }

    TPS_FREE(pucItemBuf);

    pucItemBuf = (PUCHAR)TPS_ALLOC(uiEntryLen);
    if (LW_NULL == pucItemBuf) {
        TPS_FREE(pucItemBuf);
        return  (TPS_ERR_ALLOC);
    }

    if (tpsFsInodeRead(pinodeDir,
                       off,
                       pucItemBuf,
                       uiEntryLen,
                       LW_TRUE) < uiEntryLen) {
        TPS_FREE(pucItemBuf);
        return  (TPS_ERR_INODE_READ);
    }


    /*
     *  构建entry对象
     */
    pentry = (PTPS_ENTRY)TPS_ALLOC(sizeof(TPS_ENTRY) + uiEntryLen);
    if (LW_NULL == pentry) {
        TPS_FREE(pucItemBuf);
        return  (TPS_ERR_ALLOC);
    }
    lib_bzero(pentry, sizeof(TPS_ENTRY) + uiEntryLen);
    
    pentry->ENTRY_offset    = off;
    pentry->ENTRY_bInHash   = LW_FALSE;
    pentry->ENTRY_psb       = pinodeDir->IND_psb;
    pentry->ENTRY_inumDir   = pinodeDir->IND_inum;
    pucPos = pucItemBuf;
    TPS_LE32_TO_CPU(pucPos, pentry->ENTRY_uiLen);
    TPS_IBLK_TO_CPU(pucPos, pentry->ENTRY_inum);
    lib_strncpy(pentry->ENTRY_pcName, (PCHAR)pucPos, uiEntryLen);

    TPS_FREE(pucItemBuf);

    pentry->ENTRY_pinode = tpsFsOpenInode(pentry->ENTRY_psb, pentry->ENTRY_inum);
    if (LW_NULL == pentry->ENTRY_pinode) {                              /* 碰到无效条目则重试           */
        off += TPS_ENTRY_ITEM_SIZE;
        TPS_FREE(pentry);
        goto  __retry_find;
    }

    *ppentry = pentry;

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsEntryFree
** 功能描述: 释放entry内存指针
** 输　入  : pentry           entry对象指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsEntryFree (PTPS_ENTRY pentry)
{
    if (pentry == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (pentry->ENTRY_pinode) {
        tpsFsCloseInode(pentry->ENTRY_pinode);
    }

    TPS_FREE(pentry);

    return  (TPS_ERR_NONE);
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
