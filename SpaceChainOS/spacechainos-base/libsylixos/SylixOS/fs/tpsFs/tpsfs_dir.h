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
** 文   件   名: tpsfs_dir.h
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: tpsfs 目录操作

** BUG:
*********************************************************************************************************/

#ifndef __TPSFS_DIR_H
#define __TPSFS_DIR_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0

/*********************************************************************************************************
  磁盘entry大小根据文件名长度而定，必须为TPS_ENTRY_SIZE的整数倍
*********************************************************************************************************/

#define TPS_ENTRY_ITEM_SIZE     256
#define TPS_ENTRY_ITEM_SHIFT    8
#define TPS_ENTRY_ITEM_MASK     0xff

#define TPS_ENTRY_MAGIC_NUM     0x00df87ad

/*********************************************************************************************************
  entry 结构
*********************************************************************************************************/

typedef struct tps_entry {
    struct tps_entry   *ENTRY_pnext;                                    /* entry链表                    */
    PTPS_SUPER_BLOCK    ENTRY_psb;                                      /* 超级块                       */
    TPS_INUM            ENTRY_inumDir;                                  /* 父目录inode号                */
    PTPS_INODE          ENTRY_pinode;                                   /* 文件inode指针                */
    UINT                ENTRY_uiLen;                                    /* entry磁盘结构长度            */
    UINT                ENTRY_uiMagic;                                  /* entry模数                    */
    TPS_INUM            ENTRY_inum;                                     /* 文件号                       */
    BOOL                ENTRY_bInHash;                                  /* entry是否在hash表中          */
    TPS_OFF_T           ENTRY_offset;                                   /* 文件在目录文件中的偏移       */
    CHAR                ENTRY_pcName[1];                                /* 文件名                       */
} TPS_ENTRY;
typedef TPS_ENTRY      *PTPS_ENTRY;

/*********************************************************************************************************
  entry 操作
*********************************************************************************************************/
#ifdef __cplusplus 
extern "C" { 
#endif 
                                                                        /* 创建entry                    */
TPS_RESULT tpsFsCreateEntry(PTPS_TRANS ptrans, PTPS_INODE pinodeDir,
                            CPCHAR pcFileName, TPS_INUM inum);
                                                                        /* 查找entry                    */
TPS_RESULT tpsFsFindEntry(PTPS_INODE pinodeDir, CPCHAR pcFileName, PTPS_ENTRY *ppentry);
                                                                        /* 释放entry内存指针            */
TPS_RESULT tpsFsEntryFree(PTPS_ENTRY pentry);
                                                                        /* 删除entry                    */
TPS_RESULT tpsFsEntryRemove(PTPS_TRANS ptrans, PTPS_ENTRY pentry);
                                                                        /* 从指定偏移开始读取entry      */
TPS_RESULT tpsFsEntryRead(PTPS_INODE pinodeDir, BOOL bHash, TPS_OFF_T off, PTPS_ENTRY *ppentry);
                                                                        /* 获取最后一个目录项结束位置   */
TPS_SIZE_T tpsFsGetDirSize (PTPS_INODE pinodeDir);

#ifdef __cplusplus 
}
#endif 

#endif                                                                  /* LW_CFG_TPSFS_EN > 0          */
#endif                                                                  /* __TPSFS_DIR_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
