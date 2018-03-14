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
** 文   件   名: tpsfs_dev_buf.h
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: 磁盘缓冲区操作

** BUG:
*********************************************************************************************************/

#ifndef __TSPSFS_DEV_BUF_H
#define __TSPSFS_DEV_BUF_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0
                                                                        /* 写入数据到磁盘               */
TPS_RESULT tpsFsDevBufWrite(PTPS_SUPER_BLOCK psb, TPS_IBLK blk, UINT uiOff,
                            PUCHAR pucBuff, size_t szLen, BOOL bSync);
                                                                        /* 从磁盘读取数据               */
TPS_RESULT tpsFsDevBufRead(PTPS_SUPER_BLOCK psb, TPS_IBLK inum, UINT uiOff,
                           PUCHAR pucBuff, size_t szLen);
                                                                        /* 同步磁盘缓冲区               */
TPS_RESULT tpsFsDevBufSync(PTPS_SUPER_BLOCK psb, TPS_IBLK blk, UINT uiOff, size_t szLen);
                                                                        /* 对磁盘执行FIOTRIM命令        */
TPS_RESULT tpsFsDevBufTrim(PTPS_SUPER_BLOCK psb, TPS_IBLK blkStart, TPS_IBLK blkCnt);

#endif                                                                  /* LW_CFG_TPSFS_EN > 0          */
#endif                                                                  /* __TSPSFS_DEV_BUF_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
