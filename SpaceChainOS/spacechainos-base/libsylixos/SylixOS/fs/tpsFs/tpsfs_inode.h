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
** 文   件   名: tpsfs_inode.h
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: inode结构及函数声明

** BUG:
*********************************************************************************************************/

#ifndef __TPSFS_INODE_H
#define __TPSFS_INODE_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0

/*********************************************************************************************************
  magic数值定义
*********************************************************************************************************/

#define TPS_MAGIC_INODE         0xad45df32

/*********************************************************************************************************
  inode头块内的数据起始位置
*********************************************************************************************************/

#define TPS_INODE_DATASTART     2048
#define TPS_INODE_MAX_HEADSIZE  512
#define TPS_INODE_ATTRIZE       (TPS_INODE_DATASTART - TPS_INODE_MAX_HEADSIZE)

/*********************************************************************************************************
  块分配数量限制
*********************************************************************************************************/
#define TPS_INODE_MIN_BLKALLC   4
#define TPS_INODE_MAX_BLKALLC   32

/*********************************************************************************************************
  文件类型定义
*********************************************************************************************************/
#define TPS_INODE_TYPE_REG      DT_REG
#define TPS_INODE_TYPE_DIR      DT_DIR
#define TPS_INODE_TYPE_SLINK    DT_LNK
#define TPS_INODE_TYPE_BDEV     DT_CHR
#define TPS_INODE_TYPE_BNEV     DT_SOCK
#define TPS_INODE_TYPE_BFIFO    DT_FIFO
#define TPS_INODE_TYPE_BLK      DT_BLK
#define TPS_INODE_TYPE_WHT      DT_WHT

#define TPS_INODE_TYPE_HASH     0x01000000

/*********************************************************************************************************
  节点池定义，用于提高节点分配效率
*********************************************************************************************************/
#define TPS_BN_POOL_SIZE        8                                       /* 节点池大小                   */
#define TPS_BN_POOL_NULL        0                                       /* 节点为空，表示非分配         */
#define TPS_BN_POOL_FREE        1                                       /* 节点已分配但未使用           */
#define TPS_BN_POOL_BUSY        2                                       /* 节点正在使用                 */

/*********************************************************************************************************
  inode 结构
*********************************************************************************************************/

typedef struct tps_inode {
    struct tps_inode   *IND_pnext;                                      /* inode链表                    */
    struct tps_inode   *IND_pinodeHash;                                 /* 保存目录hash表               */
    UINT                IND_uiOpenCnt;                                  /* 打开次数                     */
    BOOL                IND_bDirty;                                     /* 标记结构体是否被更改         */
    BOOL                IND_bDeleted;                                   /* 文件是否已被删除             */
    PTPS_SUPER_BLOCK    IND_psb;                                        /* 超级块                       */
    UINT                IND_uiMagic;                                    /* magic数值                    */
    UINT                IND_uiVersion;                                  /* 版本                         */
    UINT64              IND_ui64Generation;                             /* 标识一次格式化用于系统修复   */
    TPS_INUM            IND_inum;                                       /* 文件号                       */
    TPS_SIZE_T          IND_szData;                                     /* 文件大小                     */
    TPS_SIZE_T          IND_szAttr;                                     /* attr 长度                    */
    UINT64              IND_ui64CTime;                                  /* 创建时间                     */
    UINT64              IND_ui64MTime;                                  /* 修改时间                     */
    UINT64              IND_ui64ATime;                                  /* 访问时间                     */
    INT                 IND_iMode;                                      /* 文件模式                     */
    INT                 IND_iFlag;                                      /* 文件打开方式                 */
    INT                 IND_iType;                                      /* 文件类型                     */
    UINT                IND_uiRefCnt;                                   /* inode引用计数                */
    UINT                IND_uiDataStart;                                /* inode头块内的数据起始位置    */
    INT                 IND_iUid;                                       /* 用户ID                       */
    INT                 IND_iGid;                                       /* 用户组ID                     */
    TPS_INUM            IND_inumDeleted;                                /* 已删除文件列表               */
    TPS_INUM            IND_inumHash;                                   /* 目录hash表所在节点           */
    PUCHAR              IND_pucBuff;                                    /* 文件头序列化缓冲区           */
    TPS_IBLK            IND_blkCnt;                                     /* 文件块数量                   */
    PTPS_BTR_NODE       IND_pBNPool[TPS_BN_POOL_SIZE];                  /* B+树节点池                   */
    INT                 IND_iBNRefCnt[TPS_BN_POOL_SIZE];                /* B+树节点池权值               */
    PVOID               IND_pvPriv;                                     /* inode私有数据                */
    CHAR                IND_attr[TPS_INODE_ATTRIZE];                    /* inode attr                   */
    UINT                IND_uiSecAreaCnt;                               /* 未同步扇区数量               */
} TPS_INODE;
typedef TPS_INODE      *PTPS_INODE;

/*********************************************************************************************************
  inode 操作
*********************************************************************************************************/
                                                                        /* 创建并初始化inode            */
TPS_RESULT tpsFsCreateInode(PTPS_TRANS ptrans, PTPS_SUPER_BLOCK psb, TPS_INUM inum, INT iMode);
                                                                        /* 打开inode                    */
PTPS_INODE tpsFsOpenInode(PTPS_SUPER_BLOCK psb, TPS_INUM inum);
                                                                        /* 关闭inode                    */
TPS_RESULT tpsFsCloseInode(PTPS_INODE pinode);
                                                                        /* 截断inode                    */
TPS_RESULT tpsFsTruncInode(PTPS_TRANS ptrans, PTPS_INODE pinode, TPS_SIZE_T ui64Off);
                                                                        /* inode引用计数加1             */
TPS_RESULT tpsFsInodeAddRef(PTPS_TRANS ptrans, PTPS_INODE pinode);
                                                                        /* inode引用计数减1             */
TPS_RESULT tpsFsInodeDelRef(PTPS_TRANS ptrans, PTPS_INODE pinode);
                                                                        /* 读取inode                    */
TPS_SIZE_T tpsFsInodeRead(PTPS_INODE pinode, TPS_OFF_T off,
                          PUCHAR pucItemBuf, TPS_SIZE_T szLen, BOOL bTransData);
                                                                        /* 写inode                      */
TPS_SIZE_T tpsFsInodeWrite(PTPS_TRANS ptrans, PTPS_INODE pinode, TPS_OFF_T off,
                           PUCHAR pucItemBuf, TPS_SIZE_T szLen, BOOL bTransData);
                                                                        /* 获取inode 大小               */
TPS_SIZE_T tpsFsInodeGetSize(PTPS_INODE pinode);
                                                                        /* 如果inode为脏，则写入头部，  */
                                                                        /* 一般用于flush文件大小        */
TPS_RESULT tpsFsFlushInodeHead(PTPS_TRANS ptrans, PTPS_INODE pinode);
                                                                        /* 同步文件                     */
TPS_RESULT tpsFsInodeSync(PTPS_INODE pinode);
                                                                        /* flush超级块                  */
TPS_RESULT tpsFsFlushSuperBlock(PTPS_TRANS ptrans, PTPS_SUPER_BLOCK psb);
                                                                        /* 分配块                       */
TPS_RESULT tpsFsInodeAllocBlk(PTPS_TRANS ptrans, PTPS_SUPER_BLOCK psb,
                              TPS_IBLK blkKey, TPS_IBLK blkCnt,
                              TPS_IBLK *blkAllocStart, TPS_IBLK *blkAllocCnt);
                                                                        /* 无效inode事务相关缓冲区      */
TPS_RESULT tpsFsInodeBuffInvalid(PTPS_INODE pinode, UINT64 ui64SecStart, UINT uiSecCnt);

#endif                                                                  /* LW_CFG_TPSFS_EN > 0          */
#endif                                                                  /* __TPSFS_INODE_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
