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
** 文   件   名: procPower.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 07 月 22 日
**
** 描        述: proc 文件系统 power 信息文件.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../procFs.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_PROCFS_EN > 0) && (LW_CFG_POWERM_EN > 0)
extern BOOL     _G_bPowerSavingMode;
/*********************************************************************************************************
  power proc 文件函数声明
*********************************************************************************************************/
static ssize_t  __procFsPowerAdapterRead(PLW_PROCFS_NODE  p_pfsn, 
                                         PCHAR            pcBuffer, 
                                         size_t           stMaxBytes,
                                         off_t            oft);
static ssize_t  __procFsPowerDevRead(PLW_PROCFS_NODE  p_pfsn, 
                                     PCHAR            pcBuffer, 
                                     size_t           stMaxBytes,
                                     off_t            oft);
static ssize_t  __procFsPowerInfoRead(PLW_PROCFS_NODE  p_pfsn, 
                                      PCHAR            pcBuffer, 
                                      size_t           stMaxBytes,
                                      off_t            oft);
/*********************************************************************************************************
  power proc 文件操作函数组
*********************************************************************************************************/
static LW_PROCFS_NODE_OP    _G_pfsnoPowerAdapterFuncs = {
    __procFsPowerAdapterRead, LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoPowerDevFuncs = {
    __procFsPowerDevRead, LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoPowerInfoFuncs = {
    __procFsPowerInfoRead, LW_NULL
};
/*********************************************************************************************************
  power proc 文件目录树
*********************************************************************************************************/
#define __PROCFS_BUFFER_SIZE_INFO       1024

static LW_PROCFS_NODE       _G_pfsnPower[] = 
{
    LW_PROCFS_INIT_NODE("power", (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH), 
                        LW_NULL, LW_NULL,  0),
    
    LW_PROCFS_INIT_NODE("adapter",  (S_IRUSR | S_IRGRP | S_IROTH | S_IFREG), 
                        &_G_pfsnoPowerAdapterFuncs, "A", 0),
                        
    LW_PROCFS_INIT_NODE("devices", (S_IRUSR | S_IRGRP | S_IROTH | S_IFREG), 
                        &_G_pfsnoPowerDevFuncs, "D", 0),
                        
    LW_PROCFS_INIT_NODE("pminfo", (S_IRUSR | S_IRGRP | S_IROTH | S_IFREG), 
                        &_G_pfsnoPowerInfoFuncs, "I", __PROCFS_BUFFER_SIZE_INFO),
};
/*********************************************************************************************************
** 函数名称: __procFsPowerAdapterRead
** 功能描述: procfs 读一个读取网络 adapter 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsPowerAdapterRead (PLW_PROCFS_NODE  p_pfsn, 
                                          PCHAR            pcBuffer, 
                                          size_t           stMaxBytes,
                                          off_t            oft)
{
    const CHAR      cAdapterInfoHdr[] = 
    "ADAPTER        MAX-CHANNEL\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  实际的文件内容大小          */
          size_t    stCopeBytes;
          
    /*
     *  由于预置内存大小为 0 , 所以打开后第一次读取需要手动开辟内存.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  还没有分配内存              */
        size_t          stNeedBufferSize = 0;
        PLW_LIST_LINE   plineTemp;
        PLW_PM_ADAPTER  pmadapter;
        
        __POWERM_LOCK();
        for (plineTemp  = _G_plinePMAdapter;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            stNeedBufferSize += 32;
        }
        __POWERM_UNLOCK();
        
        stNeedBufferSize += sizeof(cAdapterInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  重新获得文件缓冲区地址      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cAdapterInfoHdr); 
                                                                        /*  打印头信息                  */
        __POWERM_LOCK();
        for (plineTemp  = _G_plinePMAdapter;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            pmadapter = _LIST_ENTRY(plineTemp, LW_PM_ADAPTER, PMA_lineManage);
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "%-14s %u\n", 
                                  pmadapter->PMA_cName,
                                  pmadapter->PMA_uiMaxChan);
        }
        __POWERM_UNLOCK();
        
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    } else {
        stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  计算实际拷贝的字节数        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** 函数名称: __procFsPowerDevPrint
** 功能描述: 打印电源管理设备文件
** 输　入  : pmdev         设备节点
**           pcBuffer      缓冲
**           stTotalSize   缓冲区大小
**           pstOft        当前偏移量
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __procFsPowerDevPrint (PLW_PM_DEV    pmdev, PCHAR  pcBuffer, 
                                    size_t  stTotalSize, size_t *pstOft)
{
    PLW_PM_ADAPTER    pmadapter = pmdev->PMD_pmadapter;
    BOOL              bIsOn     = LW_FALSE;
    PCHAR             pcIsOn;
    PCHAR             pcName;
    
    if (pmadapter && 
        pmadapter->PMA_pmafunc &&
        pmadapter->PMA_pmafunc->PMAF_pfuncIsOn) {
        pmadapter->PMA_pmafunc->PMAF_pfuncIsOn(pmadapter, pmdev, &bIsOn);
        if (bIsOn) {
            pcIsOn = "on";
        } else {
            pcIsOn = "off";
        }
    } else {
        pcIsOn = "<unknown>";
    }
    
    if (pmdev->PMD_pcName) {
        pcName = pmdev->PMD_pcName;
    
    } else {
        pcName = "<unknown>";
    }
    
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%-14s %-14s %-9u %s\n",
                       pcName, pmadapter->PMA_cName,
                       pmdev->PMD_uiChannel, pcIsOn);
}
/*********************************************************************************************************
** 函数名称: __procFsPowerDevRead
** 功能描述: procfs 读一个读取网络 devices 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsPowerDevRead (PLW_PROCFS_NODE  p_pfsn, 
                                      PCHAR            pcBuffer, 
                                      size_t           stMaxBytes,
                                      off_t            oft)
{
    const CHAR      cDevInfoHdr[] = 
    "PM-DEV         ADAPTER        CHANNEL   POWER\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  实际的文件内容大小          */
          size_t    stCopeBytes;
          
    /*
     *  由于预置内存大小为 0 , 所以打开后第一次读取需要手动开辟内存.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  还没有分配内存              */
        size_t          stNeedBufferSize = 0;
        PLW_LIST_LINE   plineTemp;
        PLW_PM_DEV      pmdev;
        
        __POWERM_LOCK();
        for (plineTemp  = _G_plinePMDev;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            stNeedBufferSize += 64;
        }
        __POWERM_UNLOCK();
        
        stNeedBufferSize += sizeof(cDevInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  重新获得文件缓冲区地址      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cDevInfoHdr); 
                                                                        /*  打印头信息                  */
        __POWERM_LOCK();
        for (plineTemp  = _G_plinePMDev;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            pmdev = _LIST_ENTRY(plineTemp, LW_PM_DEV, PMD_lineManage);
            __procFsPowerDevPrint(pmdev, pcFileBuffer, 
                                  stNeedBufferSize, &stRealSize);
        }
        __POWERM_UNLOCK();
        
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    } else {
        stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  计算实际拷贝的字节数        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** 函数名称: __procFsPowerInfoRead
** 功能描述: procfs 读一个 pminfo 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsPowerInfoRead (PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft)
{
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  实际的文件内容大小          */
             size_t     stCopeBytes;
             
    /*
     *  程序运行到这里, 文件缓冲一定已经分配了预置的内存大小.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  需要生成文件                */
        PCHAR   pcSysMode;
        PCHAR   pcPowerLevel;
        UINT    uiPowerLevel;
        ULONG   ulActive;
        
        API_PowerMCpuGet(&ulActive, &uiPowerLevel);
        
        switch (uiPowerLevel) {
        
        case LW_CPU_POWERLEVEL_TOP:
            pcPowerLevel = "Top";
            break;
            
        case LW_CPU_POWERLEVEL_FAST:
            pcPowerLevel = "Fast";
            break;
        
        case LW_CPU_POWERLEVEL_NORMAL:
            pcPowerLevel = "Normal";
            break;
        
        case LW_CPU_POWERLEVEL_SLOW:
            pcPowerLevel = "Slow";
            break;
            
        default:
            pcPowerLevel = "<unknown>";
            break;
        }
        
        if (_G_bPowerSavingMode) {
            pcSysMode = "Power-Saving";
        } else {
            pcSysMode = "Running";
        }
        
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_INFO, 0,
                              "CPU Cores  : %u\n"
                              "CPU Active : %u\n"
                              "PWR Level  : %s\n"
                              "SYS Status : %s\n",
                              (UINT)LW_NCPUS,
                              (UINT)ulActive,
                              pcPowerLevel,
                              pcSysMode);
                              
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  计算实际读取的数量          */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** 函数名称: __procFsPowerInit
** 功能描述: procfs 初始化 Power proc 文件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __procFsPowerInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnPower[0], "/");
    API_ProcFsMakeNode(&_G_pfsnPower[1], "/power");
    API_ProcFsMakeNode(&_G_pfsnPower[2], "/power");
    API_ProcFsMakeNode(&_G_pfsnPower[3], "/power");
}

#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
                                                                        /*  LW_CFG_POWERM_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
