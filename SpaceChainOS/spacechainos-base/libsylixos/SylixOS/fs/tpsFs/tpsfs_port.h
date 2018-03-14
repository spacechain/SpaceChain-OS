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
** 文   件   名: tpsfs_port.h
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: tpsfs 移植层

** BUG:
*********************************************************************************************************/

#ifndef __TPSFS_PORT_H
#define __TPSFS_PORT_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0

#include "endian.h"

/*********************************************************************************************************
  标准时间获取
*********************************************************************************************************/

#define TPS_UTC_TIME()      lib_time(LW_NULL);

/*********************************************************************************************************
  设备结构，移植层需实现
*********************************************************************************************************/

typedef struct tps_dev {
    UINT       (*DEV_SectorSize)(struct tps_dev *pdev);                 /* 获取扇区大小                 */
    UINT64     (*DEV_SectorCnt)(struct tps_dev *pdev);                  /* 获取总扇区数                 */
    INT        (*DEV_ReadSector)(struct tps_dev *pdev,
                                 PUCHAR pucBuf,
                                 UINT64 ui64StartSector,
                                 UINT64 uiSectorCnt);                   /* 读扇区                       */
    INT        (*DEV_WriteSector)(struct tps_dev *pdev,
                                  PUCHAR pucBuf,
                                  UINT64 ui64StartSector,
                                  UINT64 uiSectorCnt,
                                  BOOL bSync);                          /* 写扇区                       */
    INT        (*DEV_Sync)(struct tps_dev *pdev, 
                           UINT64 ui64StartSector, 
                           UINT64 uiSectorCnt);                         /* 同步磁盘                     */
    INT        (*DEV_Trim)(struct tps_dev *pdev,
                           UINT64 ui64StartSector,
                           UINT64 uiSectorCnt);                         /* Trim磁盘                     */
    PVOID        DEV_pvPriv;                                            /* 私有成员                     */
} TPS_DEV;
typedef TPS_DEV *PTPS_DEV;

/*********************************************************************************************************
  大小端转换，tpsfs 采用小端存储
*********************************************************************************************************/

#define TPS_LE32_TO_CPU(pos, val)   {   val = le32toh(*(UINT32*)pos); pos += sizeof(UINT32);    }
#define TPS_LE64_TO_CPU(pos, val)   {   val = le64toh(*(UINT64*)pos); pos += sizeof(UINT64);    }
#define TPS_LE32_TO_CPU_VAL(pos)    le32toh(*(UINT32*)(pos))
#define TPS_LE64_TO_CPU_VAL(pos)    le64toh(*(UINT64*)(pos))

#define TPS_CPU_TO_LE32(pos, val)   {   (*(UINT32*)(pos)) = htole32(val); pos += sizeof(UINT32);    }
#define TPS_CPU_TO_LE64(pos, val)   {   (*(UINT64*)(pos)) = htole64(val); pos += sizeof(UINT64);    }

#define TPS_CPU_TO_IBLK             TPS_CPU_TO_LE64
#define TPS_IBLK_TO_CPU             TPS_LE64_TO_CPU
#define TPS_IBLK_TO_CPU_VAL         TPS_LE64_TO_CPU_VAL

/*********************************************************************************************************
  内存分配
*********************************************************************************************************/

#define TPS_ALLOC(sz)               __SHEAP_ALLOC(((size_t)(sz)))
#define TPS_FREE(p)                 __SHEAP_FREE((p))

/*********************************************************************************************************
  对齐处理
*********************************************************************************************************/

#define TPS_ROUND_UP(x, align)      (TPS_SIZE_T)(((TPS_SIZE_T)(x) +  (align - 1)) & ~(align - 1))
#define TPS_ROUND_DOWN(x, align)    (TPS_SIZE_T)( (TPS_SIZE_T)(x) & ~(align - 1))
#define TPS_ALIGNED(x, align)       (((TPS_SIZE_T)(x) & (align - 1)) == 0)

#endif                                                                  /* LW_CFG_TPSFS_EN > 0          */
#endif                                                                  /* __TPSFS_PORT_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
