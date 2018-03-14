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
** 文   件   名: tpsfs_super.h
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: tpsfs 超级块结构声明

** BUG:
*********************************************************************************************************/

#ifndef __TPSFS_SUPER_H
#define __TPSFS_SUPER_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0

/*********************************************************************************************************
  magic 数值定义
*********************************************************************************************************/

#define TPS_MAGIC_SUPER_BLOCK1   0xad348878
#define TPS_MAGIC_SUPER_BLOCK2   0xad348879

/*********************************************************************************************************
  文件系统类型 id
*********************************************************************************************************/

#define TPS_TYPE_NUMBER         0x9c

/*********************************************************************************************************
  版本
*********************************************************************************************************/

#define TPS_CUR_VERSION         2
#define TPS_VER_SURPORT_HASHDIR 2

/*********************************************************************************************************
  super block 常量定义
*********************************************************************************************************/

#define TPS_MIN_LOG_SIZE        (512 * 1024)                            /* 最小日志大小                 */
#define TPS_SUPER_BLOCK_SECTOR  0                                       /* 超级快扇区号                 */
#define TPS_SUPER_BLOCK_NUM     0                                       /* 超级块号                     */
#define TPS_SPACE_MNG_INUM      1                                       /* 空间管理inode号              */
#define TPS_ROOT_INUM           2                                       /* 根inode号                    */
#define TPS_BPSTART_BLK         3                                       /* 块缓冲起始                   */
#define TPS_BPSTART_CNT         7                                       /* 块缓冲块数目                 */
#define TPS_DATASTART_BLK       10                                      /* 数据块起始                   */
#define TPS_MIN_BLK_SIZE        4096                                    /* 最小块大小限制               */
#define TPS_INODE_CACHE_LMT     1024

/*********************************************************************************************************
  super block 挂载标识
*********************************************************************************************************/

#define TPS_MOUNT_FLAG_WRITE    0x2                                     /* 可写                         */
#define TPS_MOUNT_FLAG_READ     0x1                                     /* 可读                         */
#define TPS_TRANS_FAULT         0x4                                     /* 事务错误态，文件系统不可访问 */

/*********************************************************************************************************
  super block 结构
*********************************************************************************************************/

typedef struct tps_super_block {
    UINT                    SB_uiMagic;                                 /* magic数值                    */
    UINT                    SB_uiVersion;                               /* 版本                         */
    
    UINT                    SB_uiSectorSize;                            /* 块设备的扇区大小             */
    UINT                    SB_uiSectorShift;
    UINT                    SB_uiSectorMask;
    UINT                    SB_uiSecPerBlk;                             /* 每块扇区数                   */
    
    UINT                    SB_uiBlkSize;                               /* 块大小                       */
    UINT                    SB_uiBlkShift;
    UINT                    SB_uiBlkMask;
    
    UINT                    SB_uiFlags;                                 /* 挂载标识                     */
    UINT64                  SB_ui64Generation;                          /* 标识一次格式化用于系统修复   */
    UINT64                  SB_ui64TotalBlkCnt;                         /* 总块数                       */
    UINT64                  SB_ui64DataStartBlk;                        /* 数据块起始                   */
    UINT64                  SB_ui64DataBlkCnt;                          /* 数据块数量                   */
    UINT64                  SB_ui64LogStartBlk;                         /* 日志块起始                   */
    UINT64                  SB_ui64LogBlkCnt;                           /* 日志块数量                   */
    UINT64                  SB_ui64BPStartBlk;                          /* btree块缓冲表起始块          */
    UINT64                  SB_ui64BPBlkCnt;                            /* btree块缓冲表块数量          */
    
    TPS_INUM                SB_inumSpaceMng;                            /* 空间管理inode号              */
    TPS_INUM                SB_inumRoot;                                /* 文件系统根inode号            */
    TPS_INUM                SB_inumDeleted;                             /* 已删除文件列表               */

    PTPS_DEV                SB_dev;                                     /* 设备对象指针                 */
    struct tps_inode       *SB_pinodeSpaceMng;                          /* 空间管理inode                */
    struct tps_inode       *SB_pinodeRoot;                              /* 文件系统根inode              */
    struct tps_inode       *SB_pinodeDeleted;                           /* 已经删除的文件               */
    struct tps_inode       *SB_pinodeOpenList;                          /* 以打开文件链表               */
    UINT                    SB_uiInodeOpenCnt;                          /* 当前打开文件数               */

    PUCHAR                  SB_pucSectorBuf;                            /* 磁盘页面缓冲区               */

    struct tps_blk_pool    *SB_pbp;                                     /* btree块缓冲链表              */

    struct tps_trans_sb    *SB_ptranssb;                                /* 事务系统超级块               */
} TPS_SUPER_BLOCK;
typedef TPS_SUPER_BLOCK    *PTPS_SUPER_BLOCK;

/*********************************************************************************************************
  super block 操作
*********************************************************************************************************/
#ifdef __cplusplus 
extern "C" { 
#endif 
                                                                        /* 挂载tpsfs文件系统            */
errno_t tpsFsMount(PTPS_DEV pdev, UINT uiFlags, PTPS_SUPER_BLOCK *ppsb);
                                                                        /* 卸载tpsfs文件系统            */
errno_t tpsFsUnmount(PTPS_SUPER_BLOCK pSB);
                                                                        /* 格式化tpsfs文件系统          */
errno_t tpsFsFormat(PTPS_DEV pdev, UINT uiBlkSize);

#ifdef __cplusplus 
}
#endif 

#endif                                                                  /* LW_CFG_TPSFS_EN > 0          */
#endif                                                                  /* __TPSFS_SUPER_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
