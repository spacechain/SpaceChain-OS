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
** 文   件   名: diskio.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 09 月 26 日
**
** 描        述: FAT 文件系统与 BLOCK 设备接口.
**
** BUG:
2014.12.31  支持 ff10c 接口.
2015.03.24  重新设计获取磁盘状态接口.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "diskio.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
PLW_BLK_DEV     __blockIoDevGet(INT  iIndex);
INT             __blockIoDevRead(INT     iIndex, 
                                 VOID   *pvBuffer, 
                                 ULONG   ulStartSector, 
                                 ULONG   ulSectorCount);
INT             __blockIoDevWrite(INT     iIndex, 
                                  VOID   *pvBuffer, 
                                  ULONG   ulStartSector, 
                                  ULONG   ulSectorCount);
INT             __blockIoDevReset(INT     iIndex);
INT             __blockIoDevStatus(INT     iIndex);
INT             __blockIoDevIoctl(INT  iIndex, INT  iCmd, LONG  lArg);
/*********************************************************************************************************
** 函数名称: disk_initialize
** 功能描述: 初始化块设备
** 输　入  : ucDriver      卷序号
** 输　出  : VOID
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
DSTATUS  disk_initialize (BYTE  ucDriver)
{
    REGISTER INT    iError = __blockIoDevIoctl((INT)ucDriver, FIODISKINIT, 0);
    
    if (iError < 0) {
        return  ((DSTATUS)(STA_NOINIT | STA_NODISK));
    
    } else {
        return  ((DSTATUS)ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: disk_status
** 功能描述: 获得块设备状态
** 输　入  : ucDriver      卷序号
** 输　出  : ERROR_NONE 表示块设备当前正常, 其他表示不正常.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
DSTATUS  disk_status (BYTE  ucDriver)
{
    DSTATUS     dstat = 0;
    PLW_BLK_DEV pblk  = __blockIoDevGet((INT)ucDriver);

    if (!pblk) {
        return  (STA_NODISK);
    }
    
    if ((pblk->BLKD_iFlag & O_ACCMODE) == O_RDONLY) {
        dstat |= STA_PROTECT;
    }
    
    dstat |= (DSTATUS)__blockIoDevStatus((INT)ucDriver);
    
    return  (dstat);
}
/*********************************************************************************************************
** 函数名称: disk_status
** 功能描述: 读一个块设备
** 输　入  : ucDriver          卷序号
**           ucBuffer          缓冲区
**           dwSectorNumber    起始扇区号
**           uiSectorCount     扇区数量
** 输　出  : DRESULT
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
DRESULT disk_read (BYTE  ucDriver, BYTE  *ucBuffer, DWORD   dwSectorNumber, UINT  uiSectorCount)
{
    REGISTER INT    iError;
    
    iError = __blockIoDevRead((INT)ucDriver, 
                              (PVOID)ucBuffer, 
                              (ULONG)dwSectorNumber,
                              (ULONG)uiSectorCount);
    if (iError >= ERROR_NONE) {
        return  (RES_OK);
    
    } else {
        return  (RES_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: disk_write
** 功能描述: 写一个块设备
** 输　入  : ucDriver          卷序号
**           ucBuffer          缓冲区
**           dwSectorNumber    起始扇区号
**           uiSectorCount     扇区数量
** 输　出  : DRESULT
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
DRESULT disk_write (BYTE  ucDriver, const BYTE  *ucBuffer, DWORD   dwSectorNumber, UINT  uiSectorCount)
{
    REGISTER INT    iError;
    
    iError = __blockIoDevWrite((INT)ucDriver, 
                               (PVOID)ucBuffer, 
                               (ULONG)dwSectorNumber,
                               (ULONG)uiSectorCount);
    if (iError >= ERROR_NONE) {
        return  (RES_OK);
    
    } else {
        return  (RES_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: disk_write
** 功能描述: 写一个块设备
** 输　入  : ucDriver          卷序号
**           ucCmd             命令
**           pvArg             参数
** 输　出  : DRESULT
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
DRESULT  disk_ioctl (BYTE  ucDriver, BYTE ucCmd, void  *pvArg)
{
    REGISTER INT            iError;
             ULONG          ulArgLong;
             LW_BLK_RANGE   blkrange;
    
    switch (ucCmd) {                                                    /*  转换命令                    */
    
    case CTRL_SYNC:
        return  (RES_OK);                                               /*  注意, 目前此条命令忽略      */
    
    case GET_SECTOR_COUNT:                                              /*  获得扇区总数量              */
        if (__blockIoDevIoctl((INT)ucDriver, 
                              LW_BLKD_GET_SECNUM, 
                              (LONG)&ulArgLong)) {
            return  (RES_ERROR);
        
        } else {
            *(DWORD *)pvArg = (DWORD)ulArgLong;                         /*  FatFs set 32 bit arg        */
        }
        return  (RES_OK);
        
    case GET_SECTOR_SIZE:                                               /*  获得扇区大小                */
        if (__blockIoDevIoctl((INT)ucDriver, 
                              LW_BLKD_GET_SECSIZE, 
                              (LONG)&ulArgLong)) {
            return  (RES_ERROR);
        
        } else {
            *(WORD *)pvArg = (WORD)ulArgLong;                           /*  FatFs set 16 bit arg        */
        }
        return  (RES_OK);
        
    case GET_BLOCK_SIZE:                                                /*  获得块大小                  */
        if (__blockIoDevIoctl((INT)ucDriver, 
                              LW_BLKD_GET_BLKSIZE, 
                              (LONG)&ulArgLong)) {
            return  (RES_ERROR);
        
        } else {
            *(DWORD *)pvArg = (DWORD)ulArgLong;                         /*  FatFs set 32 bit arg        */
        }
        return  (RES_OK);
        
    case CTRL_TRIM:                                                     /*  ATA 释放扇区                */
        ucCmd = FIOTRIM;
        blkrange.BLKR_ulStartSector = (ULONG)(((DWORD *)pvArg)[0]);
        blkrange.BLKR_ulEndSector   = (ULONG)(((DWORD *)pvArg)[1]);
        pvArg = (PVOID)&blkrange;
        break;
        
    case CTRL_POWER:                                                    /*  电源控制                    */
        ucCmd = LW_BLKD_CTRL_POWER;
        break;
        
    case CTRL_LOCK:                                                     /*  锁定设备                    */
        ucCmd = LW_BLKD_CTRL_LOCK;
        break;
        
    case CTRL_EJECT:                                                    /*  弹出设备                    */
        ucCmd = LW_BLKD_CTRL_EJECT;
        break;
    }
    
    iError = __blockIoDevIoctl((INT)ucDriver, (INT)ucCmd, (LONG)pvArg);
    if (iError >= ERROR_NONE) {
        return  (RES_OK);
    
    } else {
        return  (RES_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_MAX_VOLUMES          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
