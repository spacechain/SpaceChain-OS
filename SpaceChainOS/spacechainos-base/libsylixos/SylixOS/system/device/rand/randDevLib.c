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
** 文   件   名: randDevLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 10 月 31 日
**
** 描        述: UNIX 兼容随机数设备.

** BUG:
2012.11.08  __randRead() 中 iSeedInit 算子为 static 类型变量.
2013.12.12  只有 LW_IRQ_FLAG_SAMPLE_RAND 的中断才更新随机数种子.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"                                /*  需要根文件系统时间          */
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
#include "randDevLib.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static spinlock_t       _G_slRandLock;                                  /*  中断信息自旋锁              */
static struct timespec  _G_tvLastInt;                                   /*  最后一次中断时间戳          */
static INT64            _G_i64IntCounter;                               /*  中断计数器                  */
static LW_OBJECT_HANDLE _G_hRandSelMutex;                               /*  select list mutex           */
/*********************************************************************************************************
** 函数名称: __randInterHook
** 功能描述: 随机数发生器中断钩子函数 (此函数在退出中断时被调用, 这样可以避免延长中断响应时间)
** 输　入  : ulVector  中断向量
**           ulNesting 当前嵌套层数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __randInterHook (ULONG  ulVector, ULONG  ulNesting)
{
    INTREG  iregInterLevel;

    (VOID)ulNesting;
    
    if (LW_IVEC_GET_FLAG(ulVector) & LW_IRQ_FLAG_SAMPLE_RAND) {         /*  需要更新随机数种子          */
        LW_SPIN_LOCK_QUICK(&_G_slRandLock, &iregInterLevel);
        _G_tvLastInt = _K_tvTODCurrent;
        _G_i64IntCounter++;
        LW_SPIN_UNLOCK_QUICK(&_G_slRandLock, iregInterLevel);
    }
}
/*********************************************************************************************************
** 函数名称: __randInit
** 功能描述: 随机数发生器驱动初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __randInit (VOID)
{
    LW_SPIN_INIT(&_G_slRandLock);
    
    if (_G_hRandSelMutex == LW_OBJECT_HANDLE_INVALID) {
        _G_hRandSelMutex = API_SemaphoreMCreate("randsel_lock", LW_PRIO_DEF_CEILING, 
                                                LW_OPTION_WAIT_PRIORITY | 
                                                LW_OPTION_DELETE_SAFE |
                                                LW_OPTION_INHERIT_PRIORITY |
                                                LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: __randOpen
** 功能描述: 随机数发生器 open
** 输　入  : pranddev      设备
**           pcName        名字
**           iFlags        打开标志
**           iMode         创建属性
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LONG  __randOpen (PLW_RAND_DEV  pranddev, PCHAR  pcName, INT  iFlags, INT  iMode)
{
    PLW_RAND_FIL  prandfil;
    
    prandfil = (PLW_RAND_FIL)__SHEAP_ALLOC(sizeof(LW_RAND_FIL));
    if (prandfil == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    prandfil->RANDFIL_pranddev = pranddev;
    prandfil->RANDFIL_selwulList.SELWUL_hListLock     = _G_hRandSelMutex;
    prandfil->RANDFIL_selwulList.SELWUL_ulWakeCounter = 0;
    prandfil->RANDFIL_selwulList.SELWUL_plineHeader   = LW_NULL;

    if (LW_DEV_INC_USE_COUNT(&pranddev->RANDDEV_devhdr) == 1) {
        API_SystemHookAdd(__randInterHook, LW_OPTION_CPU_INT_EXIT);     /*  创建中断hook                */
    }
    
    return  ((LONG)prandfil);
}
/*********************************************************************************************************
** 函数名称: __randClose
** 功能描述: 随机数发生器 close
** 输　入  : prandfil      文件
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  __randClose (PLW_RAND_FIL  prandfil)
{
    PLW_RAND_DEV  pranddev = prandfil->RANDFIL_pranddev;

    SEL_WAKE_UP_ALL(&prandfil->RANDFIL_selwulList, SELREAD);
    SEL_WAKE_UP_ALL(&prandfil->RANDFIL_selwulList, SELWRITE);
    SEL_WAKE_UP_ALL(&prandfil->RANDFIL_selwulList, SELEXCEPT);

    if (LW_DEV_DEC_USE_COUNT(&pranddev->RANDDEV_devhdr) == 0) {
        API_SystemHookDelete(__randInterHook, LW_OPTION_CPU_INT_EXIT);  /*  删除中断hook                */
    }
    
    __SHEAP_FREE(prandfil);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __randRead
** 功能描述: 随机数发生器 read
** 输　入  : prandfil      文件
**           pcBuffer      读缓冲区
**           stMaxBytes    缓冲区大小
** 输　出  : 实际读出字节数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ssize_t  __randRead (PLW_RAND_FIL  prandfil, PCHAR  pcBuffer, size_t  stMaxBytes)
{
    static INT32    iSeedInit = 1;
    
    INTREG          iregInterLevel;
    INT             i;
    INT             iTimes = stMaxBytes / 4;                            /*  一次 4 个字节               */
    INT             iLefts = stMaxBytes % 4;
    INT32           iSeed;
    INT64           llTmp;
    
    LW_SPIN_LOCK_QUICK(&_G_slRandLock, &iregInterLevel);
    iSeedInit += (INT32)_G_tvLastInt.tv_nsec;                           /*  快速重复调用确保不相同      */
    LW_SPIN_UNLOCK_QUICK(&_G_slRandLock, iregInterLevel);
    
    iSeedInit = (iSeedInit >= 0) ? iSeedInit : (0 - iSeedInit);
    iSeed     = iSeedInit;
    
    for (i = 0; i < iTimes; i++) {
        llTmp   = (INT64)((INT64)iSeed * 1103515245);
        llTmp  += ((INT64)16807L ^ (_G_i64IntCounter + API_TimeGet64()));
        llTmp >>= 16;
        iSeed   = (INT32)(llTmp % __ARCH_UINT_MAX);
        iSeed   = (iSeed >= 0) ? iSeed : (0 - iSeed);
        
        *pcBuffer++ = (CHAR)(iSeed >> 24);
        *pcBuffer++ = (CHAR)(iSeed >> 8);
        *pcBuffer++ = (CHAR)(iSeed >> 16);
        *pcBuffer++ = (CHAR)(iSeed);
    }
    
    if (iLefts) {
        llTmp   = (INT64)((INT64)iSeed * 1103515245);
        llTmp  += ((INT64)16807L ^ (_G_i64IntCounter + API_TimeGet64()));
        llTmp >>= 16;
        iSeed   = (INT32)(llTmp % __ARCH_UINT_MAX);
        iSeed   = (iSeed >= 0) ? iSeed : (0 - iSeed);
        
        for (i = 0; i < iLefts; i++) {
            *pcBuffer++ = (CHAR)(iSeed >> 24);
            iSeed <<= 8;
        }
    }
    
    iSeedInit = iSeed;
    
    return  ((ssize_t)stMaxBytes);
}
/*********************************************************************************************************
** 函数名称: __randWrite
** 功能描述: 随机数发生器 write
** 输　入  : prandfil      文件
**           pcBuffer      写缓冲区
**           stNBytes      写入数据量
** 输　出  : 实际写入字节数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ssize_t  __randWrite (PLW_RAND_FIL  prandfil, CPCHAR  pcBuffer, size_t  stNBytes)
{
    _ErrorHandle(ENOSYS);
    return  (0);
}
/*********************************************************************************************************
** 函数名称: __randIoctl
** 功能描述: 随机数发生器 ioctl
** 输　入  : prandfil      文件
**           iRequest      命令
**           lArg          命令参数
** 输　出  : 实际写入字节数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  __randIoctl (PLW_RAND_FIL  prandfil, INT  iRequest, LONG  lArg)
{
    PLW_RAND_DEV  pranddev = prandfil->RANDFIL_pranddev;
    struct stat  *pstat;
    
    switch (iRequest) {
    
    case FIOFSTATGET:                                                   /*  获得文件属性                */
        pstat = (struct stat *)lArg;
        pstat->st_dev     = (dev_t)pranddev;
        pstat->st_ino     = (ino_t)pranddev->RANDDEV_bIsURand;
        pstat->st_mode    = 0444 | S_IFCHR;                             /*  默认属性                    */
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;
        pstat->st_gid     = 0;
        pstat->st_rdev    = 1;
        pstat->st_size    = 0;
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        pstat->st_atime   = API_RootFsTime(LW_NULL);                    /*  默认使用 root fs 基准时间   */
        pstat->st_mtime   = API_RootFsTime(LW_NULL);
        pstat->st_ctime   = API_RootFsTime(LW_NULL);
        break;
        
    case FIOSELECT: {
            PLW_SEL_WAKEUPNODE pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
            SEL_WAKE_NODE_ADD(&prandfil->RANDFIL_selwulList, pselwunNode);
            if (pselwunNode->SELWUN_seltypType == SELREAD) {
                SEL_WAKE_UP(pselwunNode);                               /*  文件可读                    */
            }
        }
        break;
        
    case FIOUNSELECT:
        SEL_WAKE_NODE_DELETE(&prandfil->RANDFIL_selwulList, (PLW_SEL_WAKEUPNODE)lArg);
        break;
        
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
