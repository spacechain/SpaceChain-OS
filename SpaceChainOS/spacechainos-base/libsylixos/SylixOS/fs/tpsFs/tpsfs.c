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
** 文   件   名: tpsfs.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: tpsfs API 实现

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
#include "tpsfs_dev_buf.h"
#include "tpsfs_inode.h"
#include "tpsfs_dir.h"
#include "tpsfs.h"
/*********************************************************************************************************
** 函数名称: __tpsFsCheckFileName
** 功能描述: 检查文件名操作
** 输　入  : pcName           文件名
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static errno_t  __tpsFsCheckFileName (CPCHAR  pcName)
{
    register CPCHAR  pcTemp;

    /*
     *  不能建立 . 或 .. 文件
     */
    pcTemp = lib_rindex(pcName, PX_DIVIDER);
    if (pcTemp) {
        pcTemp++;
        if (*pcTemp == '\0') {                                          /*  文件名长度为 0              */
            return  (ENOENT);
        }
        if ((lib_strcmp(pcTemp, ".")  == 0) ||
            (lib_strcmp(pcTemp, "..") == 0)) {                          /*  . , .. 检查                 */
            return  (ENOENT);
        }
    } else {
        if (pcName[0] == '\0') {                                        /*  文件名长度为 0              */
            return  (ENOENT);
        }
    }

    /*
     *  不能包含非法字符
     */
    pcTemp = (PCHAR)pcName;
    for (; *pcTemp != '\0'; pcTemp++) {
        if (lib_strchr("\\*?<>:\"|\t\r\n", *pcTemp)) {                  /*  检查合法性                  */
            return  (ENOENT);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFSCreate
** 功能描述: 根据名称创建文件和目录
** 输　入  : pinodeDir        父目录
**           pcFileName       文件名称
**           iMode            文件模式
** 输　出  : 错误返回 LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PTPS_ENTRY  __tpsFSCreate (PTPS_INODE pinodeDir, CPCHAR pcFileName, INT iMode, INT *piErr)
{
    PTPS_SUPER_BLOCK    psb     = pinodeDir->IND_psb;
    TPS_INUM            inum    = 0;
    TPS_IBLK            blkPscCnt;
    PTPS_TRANS          ptrans  = LW_NULL;
    PTPS_ENTRY          pentry  = LW_NULL;

    if (piErr == LW_NULL) {
        return  (LW_NULL);
    }

    ptrans = tpsFsTransAllocAndInit(psb);                               /* 分配事物                     */
    if (ptrans == LW_NULL) {
        *piErr = EINVAL;
        return  (LW_NULL);
    }

    if (tpsFsInodeAllocBlk(ptrans, psb,
                           0, 1, &inum, &blkPscCnt) != TPS_ERR_NONE) {  /* 分配inode                    */
        tpsFsTransRollBackAndFree(ptrans);
        *piErr = ENOSPC;
        return  (LW_NULL);
    }

    if (tpsFsCreateInode(ptrans, psb, inum, iMode) != TPS_ERR_NONE) {   /* 初始化inode,引用计数为1      */
        tpsFsTransRollBackAndFree(ptrans);
        *piErr = EIO;
        return  (LW_NULL);
    }

    if (tpsFsCreateEntry(ptrans, pinodeDir,
                         pcFileName, inum) != TPS_ERR_NONE) {           /* 创建目录项并关联到inode      */
        tpsFsTransRollBackAndFree(ptrans);
        *piErr = EIO;
        return  (LW_NULL);
    }

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* 提交事务                     */
        tpsFsTransRollBackAndFree(ptrans);
        *piErr = EIO;
        return  (LW_NULL);
    }

    if (tpsFsFindEntry(pinodeDir, pcFileName, &pentry) != TPS_ERR_NONE) {
        *piErr = EIO;
        return  (LW_NULL);
    }

    return  (pentry);
}
/*********************************************************************************************************
** 函数名称: __tpsFsWalkPath
** 功能描述: 查找文件所在目录并返回
** 输　入  : psb              super block指针
**           pcPath           文件路径
**           bFindParent      是否只查找条目所在目录
**           ppcSymLink       符号链接位置
**           piErr            返回错误码
** 输　出  : 成功返回entry项，失败分两种情况，存在父目录返回一个inumber为0的entry，否则返回NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PTPS_ENTRY  __tpsFsWalkPath (PTPS_SUPER_BLOCK psb, CPCHAR pcPath, PCHAR *ppcRemain, INT *piErr)
{
    PCHAR   pcPathDup    = LW_NULL;
    PCHAR   pcPathRemain = LW_NULL;
    PCHAR   pcFileName   = LW_NULL;

    PTPS_ENTRY pentry    = LW_NULL;
    PTPS_ENTRY pentrySub = LW_NULL;

    TPS_RESULT tpsres    = TPS_ERR_NONE;

    if (__tpsFsCheckFileName(pcPath) != ERROR_NONE) {                   /* 检查文件路径有效性           */
        *piErr = EINVAL;
        return  (LW_NULL);
    }

    pcPathDup = (PCHAR)TPS_ALLOC(lib_strlen(pcPath) + 1);
    if (LW_NULL == pcPathDup) {
        *piErr = EINVAL;
        return  (LW_NULL);
    }
    lib_strcpy(pcPathDup, pcPath);
    
    pcPathRemain = pcPathDup;

    /*
     *  打开根目录
     */
    pentry = (PTPS_ENTRY)TPS_ALLOC(sizeof(TPS_ENTRY) + lib_strlen(PX_STR_DIVIDER) + 1);
    if (LW_NULL == pentry) {
        TPS_FREE(pcPathDup);
        *piErr = ENOMEM;
        return  (LW_NULL);
    }
    lib_bzero(pentry, sizeof(TPS_ENTRY) + lib_strlen(PX_STR_DIVIDER) + 1);
    
    pentry->ENTRY_offset    = 0;
    pentry->ENTRY_psb       = psb;
    pentry->ENTRY_uiLen     = sizeof(TPS_ENTRY) + lib_strlen(PX_STR_DIVIDER) + 1;
    pentry->ENTRY_inum      = psb->SB_inumRoot;
    pentry->ENTRY_inumDir   = 0;
    lib_strcpy(pentry->ENTRY_pcName, (PCHAR)PX_STR_DIVIDER);

    pentry->ENTRY_pinode = tpsFsOpenInode(pentry->ENTRY_psb, pentry->ENTRY_inum);
    if (LW_NULL == pentry->ENTRY_pinode) {
        TPS_FREE(pentry);
        TPS_FREE(pcPathDup);
        *piErr = EIO;
        return  (LW_NULL);
    }

    /*
     *  目录查找
     */
    while (pcPathRemain) {
        while (*pcPathRemain == PX_DIVIDER) {
            pcPathRemain++;
        }
        pcFileName = pcPathRemain;

        if ((*pcFileName == 0) || !S_ISDIR(pentry->ENTRY_pinode->IND_iMode)) {
            break;
        }

        while ((*pcPathRemain) && (*pcPathRemain != PX_DIVIDER)) {
            pcPathRemain++;
        }
        if (*pcPathRemain != 0) {
            *pcPathRemain++ = 0;
        }

        tpsres = tpsFsFindEntry(pentry->ENTRY_pinode,
                                pcFileName,
                                &pentrySub);                            /* 查找目录项                   */
        if (tpsres != TPS_ERR_NONE) {
            if (tpsres != TPS_ERR_ENTRY_NOT_EXIST) {                    /* 内存或磁盘访问错误           */
                tpsFsEntryFree(pentry);
                pentry = LW_NULL;
                *piErr = EIO;
            } else {
                *piErr = ENOENT;
            }

            break;
        }

        tpsFsEntryFree(pentry);
        pentry = pentrySub;
    }

    TPS_FREE(pcPathDup);

    if (ppcRemain) {
        *ppcRemain = (PCHAR)pcPath + (pcFileName - pcPathDup);
    }

    return  (pentry);
}
/*********************************************************************************************************
** 函数名称: tpsFsOpen
** 功能描述: 打开文件
** 输　入  : psb              super block指针
**           pcPath           文件路径
**           iFlags           方式
**           iMode            文件模式
**           ppinode          用于输出inode结构指针
** 输　出  : 返回ERROR，成功ppinode输出inode结构指针，失败ppinode输出LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsOpen (PTPS_SUPER_BLOCK     psb,
                    CPCHAR               pcPath,
                    INT                  iFlags,
                    INT                  iMode,
                    PCHAR               *ppcRemain,
                    PTPS_INODE          *ppinode)
{
    PTPS_ENTRY pentry       = LW_NULL;
    PTPS_ENTRY pentryNew    = LW_NULL;
    CPCHAR     pcFileName   = LW_NULL;
    PCHAR      pcRemain     = LW_NULL;
    errno_t    iErr         = ERROR_NONE;

    if (LW_NULL == ppinode) {
        return  (EINVAL);
    }
    *ppinode = LW_NULL;

    if (LW_NULL == pcPath) {
        return  (EINVAL);
    }

    if (LW_NULL == psb) {
        return  (EFORMAT);
    }

    if (psb->SB_uiFlags & TPS_TRANS_FAULT) {                            /* 事务处理出错                 */
        return  (EIO);
    }

    if (iFlags & (O_CREAT | O_TRUNC)) {
        if (!(psb->SB_uiFlags & TPS_MOUNT_FLAG_WRITE)) {
            return  (EROFS);
        }
    }

    if ((lib_strcmp(pcPath, PX_STR_DIVIDER) == 0) || (pcPath[0] == '\0')) {
        *ppinode = tpsFsOpenInode(psb, psb->SB_inumRoot);

        return  (*ppinode == LW_NULL ? EIO : ERROR_NONE);
    }

    pentry = __tpsFsWalkPath(psb, pcPath, &pcRemain, &iErr);
    if (LW_NULL == pentry) {
        return  (iErr);
    }

    if (!S_ISLNK(pentry->ENTRY_pinode->IND_iMode) && (*pcRemain == 0)) {
        if ((iFlags & O_CREAT) && (iFlags & O_EXCL)) {                  /* 以互斥方式打开               */
            tpsFsEntryFree(pentry);
            return  (EEXIST);
        }
    }

    if (S_ISLNK(pentry->ENTRY_pinode->IND_iMode) || (*pcRemain == 0)) {
        goto  entry_ok;
    }

    if (!(iFlags & O_CREAT)) {
        tpsFsEntryFree(pentry);
        return  (ENOENT);
    }

    pcFileName = lib_rindex(pcPath, PX_DIVIDER);                        /* 获取文件名                   */
    if (pcFileName) {
        pcFileName += 1;
    } else {
        pcFileName = pcPath;
    }

    if (pcRemain != pcFileName) {
        tpsFsEntryFree(pentry);
        return  (ENOENT);
    }

    if (!S_ISDIR(pentry->ENTRY_pinode->IND_iMode)) {                    /* 父目录不是目录               */
        tpsFsEntryFree(pentry);
        return  (ENOENT);
    }

    pentryNew = __tpsFSCreate(pentry->ENTRY_pinode, pcFileName, iMode, &iErr);
    tpsFsEntryFree(pentry);
    pentry = pentryNew;
    if (LW_NULL == pentry) {
        return  (iErr);
    }

entry_ok:
    *ppinode = tpsFsOpenInode(psb, pentry->ENTRY_inum);
    tpsFsEntryFree(pentry);
    if (*ppinode == LW_NULL) {
        return  (EIO);
    }

    /*
     *  截断文件,只有当不是符号链接和目录时才有效
     */
    if (!S_ISLNK((*ppinode)->IND_iMode) && !S_ISDIR((*ppinode)->IND_iMode)) {
        if (iFlags & O_TRUNC) {
            iErr = tpsFsTrunc(*ppinode, 0);
            if (iErr != ERROR_NONE) {
                tpsFsCloseInode(*ppinode);
                *ppinode = LW_NULL;
                return  (iErr);
            }
        }
    }

    if (ppcRemain) {
        *ppcRemain = pcRemain;
    }

    (*ppinode)->IND_iFlag = iFlags;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsRemove
** 功能描述: 删除entry
** 输　入  : psb              super block指针
**           pcPath           文件路径
** 输　出  : 成功返回0，失败返回错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsRemove (PTPS_SUPER_BLOCK psb, CPCHAR pcPath)
{
    PTPS_ENTRY pentry       = LW_NULL;
    PTPS_ENTRY pentrySub    = LW_NULL;
    PTPS_TRANS ptrans       = LW_NULL;
    PCHAR      pcRemain     = LW_NULL;
    PTPS_INODE pinodeDir    = LW_NULL;
    TPS_INUM   inumDir      = 0;
    TPS_RESULT tpsres       = TPS_ERR_NONE;
    TPS_SIZE_T iSize;
    errno_t    iErr         = ERROR_NONE;

    if ((LW_NULL == psb) || (LW_NULL == pcPath)) {
        return  (EINVAL);
    }

    if (psb->SB_uiFlags & TPS_TRANS_FAULT) {                            /* 事务处理出错                 */
        return  (EIO);
    }

    pentry = __tpsFsWalkPath(psb, pcPath, &pcRemain, &iErr);
    if (LW_NULL == pentry) {
        return  (iErr);
    }

    if (*pcRemain != 0) {
        tpsFsEntryFree(pentry);
        return  (ENOENT);
    }

    if (S_ISDIR(pentry->ENTRY_pinode->IND_iMode)) {                     /* 不能删除非空目录             */
        iErr = tpsFsReadDir(pentry->ENTRY_pinode, LW_TRUE,
                            MAX_BLK_NUM, &pentrySub);
        if (pentrySub) {
            tpsFsEntryFree(pentrySub);
        }

        if (iErr != ENOENT) {
            tpsFsEntryFree(pentry);
            return  (ENOTEMPTY);
        }
    }

    while (LW_TRUE) {
        ptrans = tpsFsTransAllocAndInit(psb);                           /* 分配事物                     */
        if (ptrans == LW_NULL) {
            tpsFsEntryFree(pentry);
            return  (ENOMEM);
        }

        tpsres = tpsFsInodeDelRef(ptrans, pentry->ENTRY_pinode);
        if (tpsres == TPS_ERR_TRANS_NEED_COMMIT) {                      /* 删除文件可能需要多次事务     */
            if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {      /* 提交事务                     */
                tpsFsEntryFree(pentry);
                tpsFsTransRollBackAndFree(ptrans);
                return  (EIO);
            }
            continue;

        } else if (tpsres != TPS_ERR_NONE) {
            tpsFsEntryFree(pentry);
            tpsFsTransRollBackAndFree(ptrans);
            return  (EIO);
        }

        if (tpsFsEntryRemove(ptrans, pentry) != TPS_ERR_NONE) {         /* 移除entry                    */
            tpsFsEntryFree(pentry);
            tpsFsTransRollBackAndFree(ptrans);
            return  (EIO);
        }

        inumDir = pentry->ENTRY_inumDir;
        tpsFsEntryFree(pentry);

        if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {          /* 提交事务                     */
            tpsFsTransRollBackAndFree(ptrans);
            return  (EIO);
        }

        break;
    }

    /*
     *  截断目录到最后一个有效目录项结束位置
     */
    pinodeDir = tpsFsOpenInode(psb, inumDir);
    if (pinodeDir != LW_NULL) {
        iSize = tpsFsGetDirSize(pinodeDir);
        if (iSize >= 0 && iSize < tpsFsInodeGetSize(pinodeDir)) {
            tpsFsTrunc(pinodeDir, iSize);
        }

        tpsFsCloseInode(pinodeDir);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsMove
** 功能描述: 移动entry
** 输　入  : psb              super block指针
**           pcPathSrc        源文件路径
**           pcPathDst        目标文件路径
** 输　出  : 成功返回0，失败返回错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsMove (PTPS_SUPER_BLOCK psb, CPCHAR pcPathSrc, CPCHAR pcPathDst)
{
    PTPS_ENTRY pentrySrc        = LW_NULL;
    PTPS_ENTRY pentryDst        = LW_NULL;
    PTPS_TRANS ptrans           = LW_NULL;
    CPCHAR     pcFileName       = LW_NULL;
    PCHAR      pcRemain         = LW_NULL;
    errno_t    iErr             = ERROR_NONE;

    if ((LW_NULL == psb) || (LW_NULL == pcPathSrc) || (LW_NULL == pcPathDst)) {
        return  (EINVAL);
    }

    if (psb->SB_uiFlags & TPS_TRANS_FAULT) {                            /* 事务处理出错                 */
        return  (EIO);
    }

    pentrySrc = __tpsFsWalkPath(psb, pcPathSrc, &pcRemain, &iErr);
    if (LW_NULL == pentrySrc) {
        return  (iErr);
    }
    
    if (*pcRemain != 0) {                                               /* 源文件必须存在               */
        tpsFsEntryFree(pentrySrc);
        return  (ENOENT);
    }

    pentryDst = __tpsFsWalkPath(psb, pcPathDst, &pcRemain, &iErr);
    if (LW_NULL == pentryDst) {
        tpsFsEntryFree(pentrySrc);
        return  (iErr);
    }

    if (pentrySrc->ENTRY_inumDir == pentryDst->ENTRY_inumDir &&
        pentrySrc->ENTRY_offset == pentryDst->ENTRY_offset   &&
        pentrySrc->ENTRY_bInHash == pentryDst->ENTRY_bInHash) {         /* 源和目标路径相等             */
        tpsFsEntryFree(pentryDst);
        tpsFsEntryFree(pentrySrc);
        return  (ERROR_NONE);
    }

    if (*pcRemain == 0) {                                               /* 目标已存在                   */
        if (!S_ISDIR(pentrySrc->ENTRY_pinode->IND_iMode) &&
            S_ISDIR(pentryDst->ENTRY_pinode->IND_iMode)) {              /* 源为文件而目标为目录         */
            tpsFsEntryFree(pentryDst);
            tpsFsEntryFree(pentrySrc);
            return  (EISDIR);
        }

        if (S_ISDIR(pentrySrc->ENTRY_pinode->IND_iMode) &&
            !S_ISDIR(pentryDst->ENTRY_pinode->IND_iMode)) {             /* 源为目录而目标为文件         */
            tpsFsEntryFree(pentryDst);
            tpsFsEntryFree(pentrySrc);
            return  (ENOTDIR);
        }

        tpsFsEntryFree(pentryDst);

        iErr = tpsFsRemove(psb, pcPathDst);                             /* 删除现有目标                 */
        if (iErr) {
            tpsFsEntryFree(pentrySrc);
            return  (iErr);
        }

        pentryDst = __tpsFsWalkPath(psb, pcPathDst, &pcRemain, &iErr);  /* 获取目标父目录节点           */
        if (LW_NULL == pentryDst) {
            tpsFsEntryFree(pentrySrc);
            return  (iErr);
        }
    }

    pcFileName = lib_rindex(pcPathDst, PX_DIVIDER);                     /* 获取文件名                   */
    if (pcFileName) {
        pcFileName += 1;
    } else {
        pcFileName = pcPathDst;
    }

    if (pcRemain != pcFileName) {
        tpsFsEntryFree(pentryDst);
        tpsFsEntryFree(pentrySrc);
        return  (ENOENT);
    }

    ptrans = tpsFsTransAllocAndInit(psb);                               /* 分配事物                     */
    if (ptrans == LW_NULL) {
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        return  (ENOMEM);
    }

    if (tpsFsCreateEntry(ptrans, pentryDst->ENTRY_pinode,
                         pcFileName, pentrySrc->ENTRY_inum) != TPS_ERR_NONE) {
                                                                        /* 创建目录项并关联到inode      */
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsEntryRemove(ptrans, pentrySrc) != TPS_ERR_NONE) {          /* 移除entry                    */
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    tpsFsEntryFree(pentrySrc);
    tpsFsEntryFree(pentryDst);

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* 提交事务                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsLink
** 功能描述: 建立硬链接
** 输　入  : psb              super block指针
**           pcPathSrc        源文件路径
**           pcPathDst        目标文件路径
** 输　出  : 成功返回0，失败返回错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsLink (PTPS_SUPER_BLOCK psb, CPCHAR pcPathSrc, CPCHAR pcPathDst)
{
    PTPS_ENTRY pentrySrc        = LW_NULL;
    PTPS_ENTRY pentryDst        = LW_NULL;
    PTPS_TRANS ptrans           = LW_NULL;
    CPCHAR     pcFileName       = LW_NULL;
    PCHAR      pcRemain         = LW_NULL;
    errno_t    iErr             = ERROR_NONE;

    if ((LW_NULL == psb) || (LW_NULL == pcPathSrc) || (LW_NULL == pcPathDst)) {
        return  (EINVAL);
    }

    if (psb->SB_uiFlags & TPS_TRANS_FAULT) {                            /* 事务处理出错                 */
        return  (EIO);
    }

    pentrySrc = __tpsFsWalkPath(psb, pcPathSrc, &pcRemain, &iErr);
    if (LW_NULL == pentrySrc) {
        return  (iErr);
    }
    
    if (*pcRemain != 0) {                                               /* 源文件必须存在               */
        tpsFsEntryFree(pentrySrc);
        return  (ENOENT);
    }

    pentryDst = __tpsFsWalkPath(psb, pcPathDst, &pcRemain, &iErr);
    if (LW_NULL == pentryDst) {
        tpsFsEntryFree(pentrySrc);
        return  (iErr);
    }

    if (*pcRemain == 0) {                                               /* 文件已存在                   */
        tpsFsEntryFree(pentryDst);
        tpsFsEntryFree(pentrySrc);
        return  (EEXIST);
    }

    pcFileName = lib_rindex(pcPathDst, PX_DIVIDER);                     /* 获取文件名                   */
    if (pcFileName) {
        pcFileName += 1;
    } else {
        pcFileName = pcPathDst;
    }

    if (pcRemain != pcFileName) {
        tpsFsEntryFree(pentryDst);
        tpsFsEntryFree(pentrySrc);
        return  (ENOENT);
    }

    ptrans = tpsFsTransAllocAndInit(psb);                               /* 分配事物                     */
    if (ptrans == LW_NULL) {
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        return  (ENOMEM);
    }

    if (tpsFsCreateEntry(ptrans, pentryDst->ENTRY_pinode,
                         pcFileName, pentrySrc->ENTRY_inum) != TPS_ERR_NONE) {
                                                                        /* 创建目录项并关联到inode      */
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsInodeAddRef(ptrans,
                         pentrySrc->ENTRY_pinode) != TPS_ERR_NONE) {    /* inode引用加1                 */
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    tpsFsEntryFree(pentrySrc);
    tpsFsEntryFree(pentryDst);

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* 提交事务                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsRead
** 功能描述: 读取文件
** 输　入  : pinode           inode指针
**           off              起始位置
**           pucItemBuf       缓冲区
**           szLen            读取长度
**           pszRet           用于输出实际读取长度
** 输　出  : 返回ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsRead (PTPS_INODE   pinode,
                    PUCHAR       pucBuff,
                    TPS_OFF_T    off,
                    TPS_SIZE_T   szLen,
                    TPS_SIZE_T  *pszRet)
{
    errno_t  iErr;
    
    if (LW_NULL == pszRet) {
        return  (EINVAL);
    }

    *pszRet = 0;

    if ((LW_NULL == pinode) || (LW_NULL == pucBuff)) {
        return  (EINVAL);
    }

    if (pinode->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {                /* 事务处理出错                 */
        return  (EIO);
    }

    *pszRet = tpsFsInodeRead(pinode, off, pucBuff, szLen, LW_FALSE);
    if (*pszRet < 0) {
        iErr = (errno_t)(-(*pszRet));
        return  (iErr);
    
    } else {
        return  (TPS_ERR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: tpsFsWrite
** 功能描述: 写文件
** 输　入  : pinode           inode指针
**           off              起始位置
**           pucBuff          缓冲区
**           szLen            读取长度
**           pszRet           用于输出实际写入长度
** 输　出  : 返回ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsWrite (PTPS_INODE  pinode,
                     PUCHAR      pucBuff,
                     TPS_OFF_T   off,
                     TPS_SIZE_T  szLen,
                     TPS_SIZE_T *pszRet)
{
    PTPS_TRANS  ptrans  = LW_NULL;
    TPS_SIZE_T  szWrite = 0;
    errno_t     iErr;

    if (LW_NULL == pszRet) {
        return  (EINVAL);
    }

    *pszRet = 0;

    if ((LW_NULL == pinode) || (LW_NULL == pucBuff)) {
        return  (EINVAL);
    }

    if (pinode->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {                /* 事务处理出错                 */
        return  (EIO);
    }

    if (S_ISDIR(pinode->IND_iMode)) {                                   /* 目录不允许直接写操作         */
        return  (EISDIR);
    }

    while (szLen > 0) {                                                 /* 避免事务过大可能将写操作分解 */
        ptrans = tpsFsTransAllocAndInit(pinode->IND_psb);               /* 分配事务                     */
        if (ptrans == LW_NULL) {
            return  (ENOMEM);
        }

        szWrite = tpsFsInodeWrite(ptrans, pinode, (off + (*pszRet)),
                                  (pucBuff + (*pszRet)), szLen, LW_FALSE);
        if (szWrite <= 0) {
            tpsFsTransRollBackAndFree(ptrans);
            iErr = (errno_t)(-szWrite);
            return  (iErr);
        }

        if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {          /* 提交事务                     */
            tpsFsTransRollBackAndFree(ptrans);
            return  (EIO);
        }

        (*pszRet) += szWrite;
        szLen     -= szWrite;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsClose
** 功能描述: 关闭文件
** 输　入  : pinode           文件inode指针
** 输　出  : 成功返回0，失败返回错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsClose (PTPS_INODE pinode)
{
    PTPS_TRANS  ptrans      = LW_NULL;

    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    if (pinode->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {                /* 事务处理出错                 */
        return  (EIO);
    }

    ptrans = tpsFsTransAllocAndInit(pinode->IND_psb);                   /* 分配事物                     */
    if (ptrans == LW_NULL) {
        return  (ENOMEM);
    }

    if (tpsFsFlushInodeHead(ptrans, pinode) != TPS_ERR_NONE) {
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* 提交事务                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (tpsFsCloseInode(pinode));
}
/*********************************************************************************************************
** 函数名称: tpsFsFlushHead
** 功能描述: 提交文件头属性修改
** 输　入  : pinode           文件inode指针
** 输　出  : 成功返回0，失败返回错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsFlushHead (PTPS_INODE pinode)
{
    PTPS_TRANS  ptrans  = LW_NULL;

    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    if (pinode->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {                /* 事务处理出错                 */
        return  (EIO);
    }

    ptrans = tpsFsTransAllocAndInit(pinode->IND_psb);                   /* 分配事物                     */
    if (ptrans == LW_NULL) {
        return  (ENOMEM);
    }

    if (tpsFsFlushInodeHead(ptrans, pinode) != TPS_ERR_NONE) {
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* 提交事务                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsTrunc
** 功能描述: 截断文件
** 输　入  : pinode           inode指针
**           ui64Off          截断后的长度
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsTrunc (PTPS_INODE pinode, TPS_SIZE_T szNewSize)
{
    PTPS_TRANS  ptrans  = LW_NULL;
    TPS_RESULT  tpsres  = TPS_ERR_NONE;

    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    if (pinode->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {                /* 事务处理出错                 */
        return  (EIO);
    }

    do {
        ptrans = tpsFsTransAllocAndInit(pinode->IND_psb);                   /* 分配事物                     */
        if (ptrans == LW_NULL) {
            return  (ENOMEM);
        }

        tpsres = tpsFsTruncInode(ptrans, pinode, szNewSize);
        if (tpsres != TPS_ERR_NONE && tpsres != TPS_ERR_TRANS_NEED_COMMIT) {
            tpsFsTransRollBackAndFree(ptrans);
            return  (EIO);
        }

        if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* 提交事务                     */
            tpsFsTransRollBackAndFree(ptrans);
            return  (EIO);
        }
    } while (tpsres == TPS_ERR_TRANS_NEED_COMMIT);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: 创建目录
** 功能描述: 打开文件
** 输　入  : psb              super block指针
**           pcPath           文件路径
**           iFlags           方式
**           iMode            文件模式
** 输　出  : 成功返回inode结构指针，失败返回NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsMkDir (PTPS_SUPER_BLOCK psb, CPCHAR pcPath, INT iFlags, INT iMode)
{
    PTPS_INODE  pinode;
    errno_t     iErr    = ERROR_NONE;

    if ((LW_NULL == psb) || (LW_NULL == pcPath)) {
        return  (EINVAL);
    }

    if (psb->SB_uiFlags & TPS_TRANS_FAULT) {                            /* 事务处理出错                 */
        return  (EIO);
    }

    iMode &= ~S_IFMT;

    iErr = tpsFsOpen(psb, pcPath, iFlags | O_CREAT,
                     iMode | S_IFDIR, LW_NULL, &pinode);
    if (iErr != ERROR_NONE) {
        return  (iErr);
    }

    return  (tpsFsClose(pinode));
}
/*********************************************************************************************************
** 函数名称: tpsFsReadDir
** 功能描述: 读取目录
** 输　入  : pinodeDir        目录inode指针
**           bInHash          目录是否在hash表中
**           off              读取偏移
**           ppentry          用于输出entry指针
** 输　出  : 返回ERROR，成功ppentry输出entry指针，失败ppentry输出LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsReadDir (PTPS_INODE pinodeDir, BOOL bInHash, TPS_OFF_T off, PTPS_ENTRY *ppentry)
{
    if (LW_NULL == ppentry) {
        return  (EINVAL);
    }

    *ppentry = LW_NULL;

    if ((LW_NULL == pinodeDir) || (LW_NULL == ppentry)) {
        return  (EINVAL);
    }

    if (pinodeDir->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {             /* 事务处理出错                 */
        return  (EIO);
    }

    if (tpsFsEntryRead(pinodeDir, bInHash, off, ppentry) == TPS_ERR_ENTRY_NOT_EXIST) {
        return  (ENOENT);
    }

    if (*ppentry == LW_NULL) {                                          /* 内存或磁盘访问错误           */
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsSync
** 功能描述: 同步文件
** 输　入  : pinode           inode指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsSync (PTPS_INODE pinode)
{
    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    if (pinode->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {                /* 事务处理出错                 */
        return  (EIO);
    }

    if (tpsFsInodeSync(pinode) != TPS_ERR_NONE) {
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsVolSync
** 功能描述: 同步整个文件系统分区
** 输　入  : psb               超级块指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsVolSync (PTPS_SUPER_BLOCK psb)
{
    if (LW_NULL == psb) {
        return  (EINVAL);
    }

    if (psb->SB_uiFlags & TPS_TRANS_FAULT) {                            /* 事务处理出错                 */
        return  (EIO);
    }

    if (tpsFsDevBufSync(psb,
                        psb->SB_ui64DataStartBlk,
                        0,
                        (size_t)(psb->SB_ui64DataBlkCnt << psb->SB_uiBlkShift)) != TPS_ERR_NONE) {
                                                                        /* 同步所有数据块               */
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsStat
** 功能描述: tpsfs 获得文件 stat
** 输　入  : psb              文件系统超级块
**           pinode           inode指针
**           pstat            获得的 stat
** 输　出  : 创建结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#ifndef WIN32

VOID  tpsFsStat (PTPS_SUPER_BLOCK  psb, PTPS_INODE  pinode, struct stat *pstat)
{
    if (pinode) {
        if (LW_NULL == pinode->IND_psb) {
            lib_bzero(pstat, sizeof(struct stat));

        } else {
            pstat->st_dev     = (dev_t)pinode->IND_psb->SB_dev;
            pstat->st_ino     = (ino_t)pinode->IND_inum;
            pstat->st_mode    = pinode->IND_iMode;
            pstat->st_nlink   = pinode->IND_uiRefCnt;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 1;
            pstat->st_size    = pinode->IND_szData;
            pstat->st_atime   = pinode->IND_ui64ATime;
            pstat->st_mtime   = pinode->IND_ui64MTime;
            pstat->st_ctime   = pinode->IND_ui64CTime;
            pstat->st_blksize = pinode->IND_psb->SB_uiBlkSize;
            pstat->st_blocks  = 0;
        }

    } else if (psb) {
        pstat->st_dev     = (dev_t)psb;
        pstat->st_ino     = (ino_t)psb->SB_inumRoot;
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;                                          /*  不支持                      */
        pstat->st_gid     = 0;                                          /*  不支持                      */
        pstat->st_rdev    = 1;                                          /*  不支持                      */
        pstat->st_atime   = (time_t)psb->SB_ui64Generation;
        pstat->st_mtime   = (time_t)psb->SB_ui64Generation;
        pstat->st_ctime   = (time_t)psb->SB_ui64Generation;
        pstat->st_blksize = psb->SB_uiBlkSize;
        pstat->st_blocks  = (blkcnt_t)psb->SB_ui64DataBlkCnt;

        pstat->st_mode = S_IFDIR;
        if (psb->SB_uiFlags & TPS_MOUNT_FLAG_READ) {
            pstat->st_mode |= S_IRUSR | S_IRGRP | S_IROTH;
        }
        if (psb->SB_uiFlags & TPS_MOUNT_FLAG_WRITE) {
            pstat->st_mode |= S_IWUSR | S_IWGRP | S_IWOTH;
        }
        
    } else {
        pstat->st_dev     = (dev_t)TPS_SUPER_MAGIC;
        pstat->st_ino     = (ino_t)TPS_SUPER_MAGIC;
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;                                          /*  不支持                      */
        pstat->st_gid     = 0;                                          /*  不支持                      */
        pstat->st_rdev    = 1;                                          /*  不支持                      */
        pstat->st_atime   = (time_t)TPS_UTC_TIME();
        pstat->st_mtime   = (time_t)TPS_UTC_TIME();
        pstat->st_ctime   = (time_t)TPS_UTC_TIME();
        pstat->st_blksize = 512;
        pstat->st_blocks  = (blkcnt_t)1;
        pstat->st_mode    = S_IFDIR | DEFAULT_DIR_PERM;
    }
}

#endif                                                                  /*  WIN32                       */
/*********************************************************************************************************
** 函数名称: tpsFsStatfs
** 功能描述: tpsfs 获得文件系统 statfs
** 输　入  : psb              文件系统超级块
**           pstatfs            获得的 statfs
** 输　出  : 创建结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  tpsFsStatfs (PTPS_SUPER_BLOCK  psb, struct statfs *pstatfs)
{
    TPS_INUM    inum;
    PTPS_INODE  pinode;

    if ((LW_NULL == psb) || (LW_NULL == pstatfs)) {
        return;
    }

    pstatfs->f_bsize  = (long)psb->SB_uiBlkSize;
    pstatfs->f_blocks = (long)psb->SB_ui64DataBlkCnt;
    pstatfs->f_bfree  = (long)tpsFsBtreeGetBlkCnt(psb->SB_pinodeSpaceMng);
    pstatfs->f_bavail = pstatfs->f_bfree;

    /*
     * 统计已删除节点列表
     */
    inum = psb->SB_inumDeleted;
    while (inum != 0) {
        pinode = tpsFsOpenInode(psb, inum);
        if (LW_NULL == pinode) {
            break;
        }

        pstatfs->f_bfree += pinode->IND_blkCnt;
        pstatfs->f_bfree++;                                             /* inode所占用的块              */
        inum = pinode->IND_inumDeleted;
        tpsFsClose(pinode);
    }
}
/*********************************************************************************************************
** 函数名称: tpsFsGetSize
** 功能描述: 获取文件大小
** 输　入  : pinode           inode指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_SIZE_T  tpsFsGetSize (PTPS_INODE pinode)
{
    if (LW_NULL == pinode) {
        return  (0);
    }

    return  (pinode->IND_szData);
}
/*********************************************************************************************************
** 函数名称: tpsFsGetmod
** 功能描述: 获取文件模式
** 输　入  : pinode           inode指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  tpsFsGetmod (PTPS_INODE pinode)
{
    if (LW_NULL == pinode) {
        return  (0);
    }

    return  (pinode->IND_iMode);
}
/*********************************************************************************************************
** 函数名称: tpsFsChmod
** 功能描述: 修改文件模式
** 输　入  : pinode           inode指针
**           iMode            模式
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsChmod (PTPS_INODE pinode, INT iMode)
{
    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    if (pinode->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {                /* 事务处理出错                 */
        return  (EIO);
    }

    iMode |= S_IRUSR;
    iMode &= ~S_IFMT;

    pinode->IND_iMode  &= S_IFMT;
    pinode->IND_iMode  |= iMode;
    pinode->IND_bDirty  = LW_TRUE;
    
    return  (tpsFsFlushHead(pinode));
}
/*********************************************************************************************************
** 函数名称: tpsFsChown
** 功能描述: 修改文件所有者
** 输　入  : pinode           inode指针
**           uid              用户id
**           gid              用户组id
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsChown (PTPS_INODE pinode, uid_t uid, gid_t gid)
{
    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    if (pinode->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {                /* 事务处理出错                 */
        return  (EIO);
    }

    pinode->IND_iUid    = uid;
    pinode->IND_iGid    = gid;
    pinode->IND_bDirty  = LW_TRUE;
    
    return  (tpsFsFlushHead(pinode));
}
/*********************************************************************************************************
** 函数名称: tpsFsChtime
** 功能描述: 修改文件时间
** 输　入  : pinode           inode指针
**           utim             时间
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsChtime (PTPS_INODE pinode, struct utimbuf  *utim)
{
    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    if (pinode->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {                /* 事务处理出错                 */
        return  (EIO);
    }

    pinode->IND_ui64ATime = utim->actime;
    pinode->IND_ui64MTime = utim->modtime;
    pinode->IND_bDirty    = LW_TRUE;
    
    return  (tpsFsFlushHead(pinode));
}
/*********************************************************************************************************
** 函数名称: tpsFsFlushInodes
** 功能描述: 回写所有脏inode
** 输　入  : psb           超级块指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  tpsFsFlushInodes (PTPS_SUPER_BLOCK psb)
{
    PTPS_INODE  pinode = LW_NULL;

    if (LW_NULL == psb) {
        return;
    }

    if (psb->SB_uiFlags & TPS_TRANS_FAULT) {                            /* 事务处理出错                 */
        return;
    }

    pinode = psb->SB_pinodeOpenList;
    while (pinode) {
        if (pinode->IND_bDirty) {
            tpsFsFlushHead(pinode);
        }
        pinode = pinode->IND_pnext;
    }
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
