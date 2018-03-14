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
** 文   件   名: blockIo.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 09 月 26 日
**
** 描        述: 系统块设备底层接口.

** BUG:
2009.03.21  逻辑设备没有物理复位, FIOUNMOUNT & 电源控制 & 弹出操作将被忽略.
            当读写出现严重错误时, 同时设备为可移动设备, 这时系统将向下层发送 FIOCANCLE 命令, DISK CACHE 
            收到此命令会将 CACHE 设置为复位状态, 防止误操作其他设备.
2009.03.27  对 O_RDONLY 的判断有错误.
2009.04.17  LW_BLKD_GET_SECSIZE 内部使用 WORD 类型(16bit).
2009.07.15  LW_BLKD_GET_SECSIZE ioctl 时也要注意 WORD 问题.
2009.10.28  计数操作使用 atomic 原子锁操作.
2009.11.03  可移动设备一旦发生磁盘媒质改变. 立即不允许读写. 等待重新创建.
2010.03.09  块读写时如果出现错误, 需要打印 _DebugHandle();
2010.05.05  修正创建块设备时对操作方法的判断. 块设备复位和获取状态不再是必须的方法.
2015.03.24  __blockIoDevGet() 不需要信号量保护, 保护由上层文件系统承担.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static PLW_BLK_DEV          _G_pblkdBLockIoTbl[LW_CFG_MAX_VOLUMES];
static LW_OBJECT_HANDLE     _G_hBLockIoTblLock;
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define __BLOCKIO_DEV_LOCK()        API_SemaphoreBPend(_G_hBLockIoTblLock, LW_OPTION_WAIT_INFINITE)
#define __BLOCKIO_DEV_UNLOCK()      API_SemaphoreBPost(_G_hBLockIoTblLock);
/*********************************************************************************************************
** 函数名称: __blockIoDevInit
** 功能描述: 初始化块设备驱动层
** 输　入  : VOID
** 输　出  : VOID
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __blockIoDevInit (VOID)
{
    REGISTER INT        i;
    
    for (i = 0; i < LW_CFG_MAX_VOLUMES; i++) {
        _G_pblkdBLockIoTbl[i] = LW_NULL;
    }
    
    _G_hBLockIoTblLock = API_SemaphoreBCreate("blk_lock", LW_TRUE, LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (!_G_hBLockIoTblLock) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "couldn't create block io lock.\r\n");
    }
}
/*********************************************************************************************************
** 函数名称: __blockIoDevCreate
** 功能描述: 在驱动层创建一个块设备驱动
** 输　入  : pblkdNew      新的块设备
** 输　出  : -1:           块设备错误
**           -2:           空间已满
**           正数:         设备表索引
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __blockIoDevCreate (PLW_BLK_DEV  pblkdNew)
{
    REGISTER INT        i;

    if (!pblkdNew) {
        return  (PX_ERROR);
    }
    
    if ((pblkdNew->BLKD_pfuncBlkRd    == LW_NULL) ||
        (pblkdNew->BLKD_pfuncBlkIoctl == LW_NULL)) {                    /*  不可缺少基本操作方法        */
        return  (PX_ERROR);
    }
    
    if (((pblkdNew->BLKD_iFlag & O_WRONLY) ||
         (pblkdNew->BLKD_iFlag & O_RDWR)) &&
         (pblkdNew->BLKD_pfuncBlkWrt == LW_NULL)) {                     /*  可写介质不可缺少写方法      */
        return  (PX_ERROR);
    }
    
    if (pblkdNew->BLKD_iRetry < 1) {
        return  (PX_ERROR);
    }
    
    __BLOCKIO_DEV_LOCK();
    for (i = 0; i < LW_CFG_MAX_VOLUMES; i++) {
        if (_G_pblkdBLockIoTbl[i] == LW_NULL) {
            _G_pblkdBLockIoTbl[i] =  pblkdNew;
            break;
        }
    }
    __BLOCKIO_DEV_UNLOCK();
    
    if (i >= LW_CFG_MAX_VOLUMES) {
        return  (-2);
    
    } else {
        return  (i);
    }
}
/*********************************************************************************************************
** 函数名称: __blockIoDevDelete
** 功能描述: 在驱动层删除一个块设备驱动
** 输　入  : iIndex        块设备
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __blockIoDevDelete (INT  iIndex)
{
    if ((iIndex >= LW_CFG_MAX_VOLUMES) ||
        (iIndex <  0)) {
        return;
    }
    
    __BLOCKIO_DEV_LOCK();
    _G_pblkdBLockIoTbl[iIndex] = LW_NULL;
    __BLOCKIO_DEV_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __blockIoDevGet
** 功能描述: 通过驱动程序索引获取一个磁盘设备控制块
** 输　入  : iIndex        块设备
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_BLK_DEV  __blockIoDevGet (INT  iIndex)
{
    REGISTER PLW_BLK_DEV    pblkd;

    if ((iIndex >= LW_CFG_MAX_VOLUMES) ||
        (iIndex <  0)) {
        return  (LW_NULL);
    }
    
    pblkd = _G_pblkdBLockIoTbl[iIndex];
    
    return  (pblkd);
}
/*********************************************************************************************************
** 函数名称: __blockIoDevRead
** 功能描述: 在驱动层读取一个块设备
** 输　入  : iIndex        块设备
**           pvBuffer      缓冲区
**           ulStartSector  起始扇区
**           ulSectorCount  扇区数量
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __blockIoDevRead (INT     iIndex, 
                       VOID   *pvBuffer, 
                       ULONG   ulStartSector, 
                       ULONG   ulSectorCount)
{
    REGISTER PLW_BLK_DEV  pblkd = _G_pblkdBLockIoTbl[iIndex];
    REGISTER INT          i = 0;
    
    if (pblkd->BLKD_iFlag & O_WRONLY) {
        return  (PX_ERROR);
    }
    
    if (pblkd->BLKD_bRemovable && pblkd->BLKD_bDiskChange) {            /*  设备改变不可操作            */
        return  (PX_ERROR);
    }
    
    if (pblkd->BLKD_pfuncBlkRd(pblkd,
                               pvBuffer, 
                               ulStartSector, 
                               ulSectorCount) < 0) {
                               
        for (i = 0; i < pblkd->BLKD_iRetry; i++) {
            if (pblkd->BLKD_bRemovable) {
                if (pblkd->BLKD_pfuncBlkStatusChk) {
                    if (pblkd->BLKD_pfuncBlkStatusChk(pblkd) != ERROR_NONE) {
                        continue;                                       /*  设备状态错误, 重新检测      */
                    }
                }
            }
            if (pblkd->BLKD_pfuncBlkRd(pblkd,
                                       pvBuffer, 
                                       ulStartSector, 
                                       ulSectorCount) >= 0) {
                break;
            }
        }
    }
    
    if (i >= pblkd->BLKD_iRetry) {
        if (pblkd->BLKD_bRemovable) {
            pblkd->BLKD_pfuncBlkIoctl(pblkd, FIOCANCEL, 0);
        }
        _DebugFormat(__ERRORMESSAGE_LEVEL, "can not read block: blk %d sector %lu [%ld].\r\n",
                     iIndex, ulStartSector, ulSectorCount);
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __blockIoDevWrite
** 功能描述: 在驱动层写入一个块设备
** 输　入  : iIndex        块设备
**           pvBuffer      缓冲区
**           ulStartSector  起始扇区
**           ulSectorCount  扇区数量
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __blockIoDevWrite (INT     iIndex, 
                        VOID   *pvBuffer, 
                        ULONG   ulStartSector, 
                        ULONG   ulSectorCount)
{
    REGISTER PLW_BLK_DEV  pblkd = _G_pblkdBLockIoTbl[iIndex];
    REGISTER INT          i = 0;
    
    if (pblkd->BLKD_iFlag == O_RDONLY) {
        return  (PX_ERROR);
    }
    
    if (pblkd->BLKD_bRemovable && pblkd->BLKD_bDiskChange) {            /*  设备改变不可操作            */
        return  (PX_ERROR);
    }
    
    if (pblkd->BLKD_pfuncBlkWrt(pblkd,
                                pvBuffer, 
                                ulStartSector, 
                                ulSectorCount) < 0) {
                               
        for (i = 0; i < pblkd->BLKD_iRetry; i++) {
            if (pblkd->BLKD_bRemovable) {
                if (pblkd->BLKD_pfuncBlkStatusChk) {
                    if (pblkd->BLKD_pfuncBlkStatusChk(pblkd) != ERROR_NONE) {
                        continue;                                       /*  设备状态错误, 重新检测      */
                    }
                }
            }
            if (pblkd->BLKD_pfuncBlkWrt(pblkd,
                                        pvBuffer, 
                                        ulStartSector, 
                                        ulSectorCount) >= 0) {
                break;
            }
        }
    }
    
    if (i >= pblkd->BLKD_iRetry) {
        if (pblkd->BLKD_bRemovable) {
            pblkd->BLKD_pfuncBlkIoctl(pblkd, FIOCANCEL, 0);
        }
        _DebugFormat(__ERRORMESSAGE_LEVEL, "can not write block: blk %d sector %lu [%ld].\r\n",
                     iIndex, ulStartSector, ulSectorCount);
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __blockIoDevIoctl
** 功能描述: 块设备执行指定命令
** 输　入  : iIndex        块设备
**           pvBuffer      缓冲区
**           iStartSector  起始扇区
**           iSectorCount  扇区数量
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __blockIoDevIoctl (INT  iIndex, INT  iCmd, LONG  lArg)
{
             INTREG       iregInterLevel;
    REGISTER PLW_BLK_DEV  pblkd = _G_pblkdBLockIoTbl[iIndex];
             PLW_BLK_DEV  pblkdPhy;                                     /*  物理设备                    */
             
    switch (iCmd) {
    
    case FIOUNMOUNT:                                                    /*  卸载磁盘                    */
        if (pblkd->BLKD_pvLink) {
            pblkdPhy = (PLW_BLK_DEV)pblkd->BLKD_pvLink;
            __LW_ATOMIC_LOCK(iregInterLevel);
            if (pblkdPhy->BLKD_uiLinkCounter == 1) {                    /*  物理设备仅当前一次挂载      */
                pblkdPhy->BLKD_uiInitCounter  = 0;                      /*  清除计数器                  */
                pblkdPhy->BLKD_uiPowerCounter = 0;
                __LW_ATOMIC_UNLOCK(iregInterLevel);
                break;
            } else {
                __LW_ATOMIC_UNLOCK(iregInterLevel);
            }
        } else {                                                        /*  此设备为物理设备            */
            break;
        }
        return  (ERROR_NONE);                                           /*  多次挂载逻辑设备不需此操作  */
        
    case FIODISKINIT:
        if (pblkd->BLKD_pvLink) {
            pblkdPhy = (PLW_BLK_DEV)pblkd->BLKD_pvLink;
            __LW_ATOMIC_LOCK(iregInterLevel);
            pblkdPhy->BLKD_uiInitCounter++;                             /*  增加初始化次数              */
            if (pblkdPhy->BLKD_uiInitCounter == 1) {                    /*  物理设备没有初始化          */
                __LW_ATOMIC_UNLOCK(iregInterLevel);
                break;
            } else {
                __LW_ATOMIC_UNLOCK(iregInterLevel);
            }
        } else {                                                        /*  此设备为物理设备            */
            break;
        }
        return  (ERROR_NONE);                                           /*  多次初始化逻辑设备不需此操作*/
    
    case LW_BLKD_GET_SECNUM:
        if (pblkd->BLKD_ulNSector > 0) {
            *(ULONG *)lArg = pblkd->BLKD_ulNSector;                     /*  ULONG 类型                  */
            return  (ERROR_NONE);
        }
        break;
        
    case LW_BLKD_GET_SECSIZE:
        if (pblkd->BLKD_ulBytesPerSector > 0) {
            *(ULONG *)lArg = pblkd->BLKD_ulBytesPerSector;              /*  ULONG 类型                  */
            return  (ERROR_NONE);
        }
        break;
        
    case LW_BLKD_GET_BLKSIZE:
        if (pblkd->BLKD_ulBytesPerBlock > 0) {
            *(ULONG *)lArg = pblkd->BLKD_ulBytesPerBlock;               /*  ULONG 类型                  */
            return  (ERROR_NONE);
        }
        break;
        
    case LW_BLKD_CTRL_POWER:
        if (lArg == LW_BLKD_POWER_ON) {                                 /*  打开电源                    */
            if (pblkd->BLKD_pvLink) {
                pblkdPhy = (PLW_BLK_DEV)pblkd->BLKD_pvLink;
                __LW_ATOMIC_LOCK(iregInterLevel);
                pblkdPhy->BLKD_uiPowerCounter++;                        /*  增加电源打开次数            */
                if (pblkdPhy->BLKD_uiPowerCounter == 1) {
                    __LW_ATOMIC_UNLOCK(iregInterLevel);
                    break;
                } else {
                    __LW_ATOMIC_UNLOCK(iregInterLevel);
                }
            } else {                                                    /*  物理设备直接打开            */
                break;
            }
        } else {                                                        /*  断开电源                    */
            if (pblkd->BLKD_pvLink) {
                pblkdPhy = (PLW_BLK_DEV)pblkd->BLKD_pvLink;
                if (pblkdPhy->BLKD_uiPowerCounter) {
                    __LW_ATOMIC_LOCK(iregInterLevel);
                    pblkdPhy->BLKD_uiPowerCounter--;                    /*  减少电源打开次数            */
                    if (pblkdPhy->BLKD_uiPowerCounter == 0) {
                        __LW_ATOMIC_UNLOCK(iregInterLevel);
                        break;
                    } else {
                        __LW_ATOMIC_UNLOCK(iregInterLevel);
                    }
                }
            } else {
                break;
            }
        }
        return  (ERROR_NONE);                                           /*  不需此操作                  */
    
    case LW_BLKD_CTRL_EJECT:
        if (pblkd->BLKD_bRemovable == LW_TRUE) {                        /*  允许移除                    */
            if (pblkd->BLKD_pvLink) {
                pblkdPhy = (PLW_BLK_DEV)pblkd->BLKD_pvLink;
                if (pblkdPhy->BLKD_uiLinkCounter == 1) {                /*  物理设备仅当前一次挂载      */
                    break;
                }
            } else {
                break;
            }
        }
        return  (ERROR_NONE);                                           /*  不需此操作                  */
    }
    
    return  (pblkd->BLKD_pfuncBlkIoctl(pblkd, iCmd, lArg));
}
/*********************************************************************************************************
** 函数名称: __blockIoDevReset
** 功能描述: 块设备复位
** 输　入  : iIndex        块设备
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __blockIoDevReset (INT     iIndex)
{
    REGISTER PLW_BLK_DEV  pblkd = _G_pblkdBLockIoTbl[iIndex];
    REGISTER INT          i = 0;

    if (pblkd->BLKD_iLogic) {                                           /*  逻辑设备复位为空            */
        return  (ERROR_NONE);
    }
    
    if (pblkd->BLKD_pfuncBlkReset == LW_NULL) {
        return  (ERROR_NONE);
    }

    if (pblkd->BLKD_pfuncBlkReset(pblkd) < 0) {
        for (i = 0; i < pblkd->BLKD_iRetry; i++) {
            if (pblkd->BLKD_pfuncBlkReset(pblkd) >= 0) {
                break;
            }
        }
    }
    
    if (i >= pblkd->BLKD_iRetry) {
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __blockIoDevStatus
** 功能描述: 获得块设备状态
** 输　入  : iIndex        块设备
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __blockIoDevStatus (INT     iIndex)
{
    REGISTER PLW_BLK_DEV  pblkd = _G_pblkdBLockIoTbl[iIndex];
    
    if (pblkd->BLKD_pfuncBlkStatusChk) {
        return  (pblkd->BLKD_pfuncBlkStatusChk(pblkd));
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __blockIoDevIsLogic
** 功能描述: 测试块设备是否为逻辑设备
** 输　入  : iIndex        块设备
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __blockIoDevIsLogic (INT     iIndex)
{
    REGISTER PLW_BLK_DEV  pblkd = _G_pblkdBLockIoTbl[iIndex];
    
    return  (pblkd->BLKD_iLogic);
}
/*********************************************************************************************************
** 函数名称: __blockIoDevFlag
** 功能描述: 获得块设备 iFlag 标志位
** 输　入  : iIndex        块设备
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __blockIoDevFlag (INT     iIndex)
{
    REGISTER PLW_BLK_DEV  pblkd = _G_pblkdBLockIoTbl[iIndex];
    
    return  (pblkd->BLKD_iFlag);
}

#endif                                                                  /*  LW_CFG_MAX_VOLUMES          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
