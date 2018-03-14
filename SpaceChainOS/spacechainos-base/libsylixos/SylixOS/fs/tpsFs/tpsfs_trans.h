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
** 文   件   名: tpsfs_trans.h
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: 事物声明

** BUG:
*********************************************************************************************************/

#ifndef __TPSFS_TRANS_H
#define __TPSFS_TRANS_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0
/*********************************************************************************************************
  事物magic
*********************************************************************************************************/
#define TPS_TRANS_MAGIC             0xEF34DDA4
/*********************************************************************************************************
  以下宏定义数值经过计算得出，
  当块大小为4096字节，B+树节点为5层时，
  最大分区大小 4096 * 80 * 177 * 177 * 177 * 177 * 2 = 600TB，
  在极端情况下，一次数B+树变动引发的最大扇区更改为 40 + 88 + 88 + 88 + 88 = 392，
  一次操作可能引发两次磁盘空间管理B+树变动，所以理论最多改变的元数据扇区数为784，
  其它元数据改动不超过100扇区。
  所以事务预留区间数设置为1024(0x400)，每个区间在内存中占用16字节。
  在块大小为4096时，本设置支持分区大小16MB--600TB,挂载后长期占用16k内存,数据与块大小成正比。
*********************************************************************************************************/
#define TPS_TRANS_MAXAREA       (0x400 << (psb->SB_uiBlkShift - 12))    /* 最大事务区间数量             */
#define TPS_TRANS_TRIGGERREA    (0x200 << (psb->SB_uiBlkShift - 12))    /* 触发事务提交的区间数量       */
#define TPS_TRANS_REV_DATASEC   (0x400 << (psb->SB_uiBlkShift - 12))    /* 保留的事务数据区空间大小     */
#define TPS_TRAN_PER_SEC        8                                       /* 每个扇区保存多少个事务头     */
#define TPS_TRAN_SIZE           64                                      /* 事务头大小                   */
#define TPS_TRAN_SHIFT          6                                       /* 事物头移位数                 */
/*********************************************************************************************************
  事物状态
*********************************************************************************************************/
#define TPS_TRANS_STATUS_UNINIT     0xFFFFFFFF                          /* 未知态                       */
#define TPS_TRANS_STATUS_INIT       0                                   /* 已分配和初始化               */
#define TPS_TRANS_STATUS_COMMIT     1                                   /* 已提交但未完成               */
#define TPS_TRANS_STATUS_COMPLETE   2                                   /* 已完成                       */
/*********************************************************************************************************
  事物类型，目前只支持纯数据型事务
*********************************************************************************************************/
#define TPS_TRANS_TYPE_DATA         1
/*********************************************************************************************************
  扇区区间列表
*********************************************************************************************************/
typedef struct tps_trans_data {
    UINT                TD_uiSecAreaCnt;                                /* 扇区区间数量                 */
    UINT                TD_uiReserved[3];                               /* 保留                         */
    struct {
        UINT64          TD_ui64SecStart;                                /* 区间起始扇区                 */
        UINT            TD_uiSecOff;                                    /* 扇区数据在事务数据中的偏移   */
        UINT            TD_uiSecCnt;                                    /* 扇区数量                     */
    } TD_secareaArr[1];                                                 /* 事务扇区区间列表             */
} TPS_TRANS_DATA;
typedef TPS_TRANS_DATA  *PTPS_TRANS_DATA;
/*********************************************************************************************************
  事物结构
*********************************************************************************************************/
typedef struct tps_trans {
    UINT                 TRANS_uiMagic;                                 /* 事务掩码                     */
    UINT                 TRANS_uiReserved;                              /* 保留                         */
    UINT64               TRANS_ui64Generation;                          /* 格式化ID                     */
    UINT64               TRANS_ui64SerialNum;                           /* 序列号                       */
    INT                  TRANS_iType;                                   /* 事务类型                     */
    INT                  TRANS_iStatus;                                 /* 事物状态                     */
    UINT64               TRANS_ui64Reserved;                            /* 保留                         */
    UINT64               TRANS_ui64Time;                                /* 修改时间                     */
    UINT64               TRANS_uiDataSecNum;                            /* 事务数据起始扇区             */
    UINT                 TRANS_uiDataSecCnt;                            /* 事务数据扇区数量             */
    UINT                 TRANS_uiCheckSum;                              /* 事物头校验和                 */

    struct tps_trans    *TRANS_pnext;                                   /* 事物列表指针                 */
    PTPS_SUPER_BLOCK     TRANS_psb;                                     /* 超级块指针                   */
    PTPS_TRANS_DATA      TRANS_pdata;                                   /* 事务数据结构指针             */
} TPS_TRANS;
typedef TPS_TRANS       *PTPS_TRANS;
/*********************************************************************************************************
  事物超级块结构
*********************************************************************************************************/
typedef struct tps_trans_sb {
    UINT64               TSB_ui64TransSecStart;                         /* 事务头列表起始扇区           */
    UINT64               TSB_ui64TransSecCnt;                           /* 事务头列表扇区数量           */
    UINT64               TSB_ui64TransDataStart;                        /* 事务数据起始扇区             */
    UINT64               TSB_ui64TransDataCnt;                          /* 事务数据扇区数量             */

    UINT64               TSB_ui64TransCurSec;                           /* 当前事务所在扇区             */
    UINT                 TSB_uiTransSecOff;                             /* 当前事务扇区偏移             */
    PUCHAR               TSB_pucSecBuff;                                /* 当前事务扇区缓冲区           */

    UINT64               TSP_ui64DataCurSec;                            /* 当前事务数据起始扇区         */

    UINT64               TSP_ui64SerialNum;                             /* 当前事务序列号               */

    struct tps_trans    *TSB_ptrans;                                    /* 事物列表                     */
} TPS_TRANS_SB;
typedef TPS_TRANS_SB       *PTPS_TRANS_SB;

/*********************************************************************************************************
  事务操作
*********************************************************************************************************/
                                                                        /* 初始化事物列表               */
TPS_RESULT  tpsFsBtreeTransInit(PTPS_SUPER_BLOCK psb);
                                                                        /* 释放事物列表                 */
TPS_RESULT  tpsFsBtreeTransFini(PTPS_SUPER_BLOCK psb);
                                                                        /* 检查事物完整性               */
TPS_RESULT tspFsCheckTrans(PTPS_SUPER_BLOCK psb);
                                                                        /* 标记事物为一致状态           */
TPS_RESULT tspFsCompleteTrans(PTPS_SUPER_BLOCK psb);
                                                                        /* 分配事物                     */
PTPS_TRANS tpsFsTransAllocAndInit(PTPS_SUPER_BLOCK psb);
                                                                        /* 回滚事物                     */
TPS_RESULT tpsFsTransRollBackAndFree(PTPS_TRANS ptrans);
                                                                        /* 提交事务                     */
TPS_RESULT tpsFsTransCommitAndFree(PTPS_TRANS ptrans);
                                                                        /* 从磁盘读取数据               */
TPS_RESULT tpsFsTransRead(PTPS_SUPER_BLOCK psb, TPS_IBLK blk, UINT uiOff,
                          PUCHAR pucBuff, size_t szLen);
                                                                        /* 写入数据到事物               */
TPS_RESULT tpsFsTransWrite(PTPS_TRANS ptrans, PTPS_SUPER_BLOCK psb,
                           TPS_IBLK blk, UINT uiOff,
                           PUCHAR pucBuff, size_t szLen);
BOOL       tpsFsTransTrigerChk(PTPS_TRANS ptrans);                      /* 是否到达事物提交触发点       */

#endif                                                                  /* LW_CFG_TPSFS_EN > 0          */
#endif                                                                  /* __TPSFS_TRANS_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
