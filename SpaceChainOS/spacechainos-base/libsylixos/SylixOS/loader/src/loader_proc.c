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
** 文   件   名: loader_proc.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 08 月 26 日
**
** 描        述: 进程的 proc 文件系统信息. 

** BUG:
2012.12.22  加入对进程文件描述符信息的显示功能.
2013.06.05  由于使用共享代码段, 显示进程内存消耗量时, 使用新的机制.
2013.09.04  加入 ioenv 文件.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_MODULELOADER_EN > 0) && (LW_CFG_PROCFS_EN > 0)
#include "../include/loader_lib.h"
#include "../include/loader_symbol.h"
/*********************************************************************************************************
  对应文件信息
*********************************************************************************************************/
#define __VPROC_PROCFS_CMDLINE_SIZE         MAX_FILENAME_LENGTH
#define __VPROC_PROCFS_MEM_SIZE             512
#define __VPROC_PROCFS_MODULES_SIZE         0                           /*  显示时确定大小              */
#define __VPROC_PROCFS_FILEDESC_SIZE        0                           /*  显示时确定大小              */
#define __VPROC_PROCFS_IOENV_SIZE           MAX_FILENAME_LENGTH + 64

#define __VPROC_PROCFS_EXE_EN               1
#define __VPROC_PROCFS_CMDLINE_EN           1
#define __VPROC_PROCFS_MEM_EN               1
#define __VPROC_PROCFS_MODULES_EN           1
#define __VPROC_PROCFS_FILEDESC_EN          1
#define __VPROC_PROCFS_IOENV_EN             1

#define __VPROC_PROCFS_EXE                  "exe"                       /*  可执行文件符号链接          */
#define __VPROC_PROCFS_CMDLINE              "cmdline"                   /*  命令行文件                  */
#define __VPROC_PROCFS_MEM                  "mem"                       /*  内存信息                    */
#define __VPROC_PROCFS_MODULES              "modules"                   /*  动态链接库情况              */
#define __VPROC_PROCFS_FILEDESC             "filedesc"                  /*  文件描述符信息              */
#define __VPROC_PROCFS_IOENV                "ioenv"                     /*  io 环境                     */
/*********************************************************************************************************
  内核 proc 文件函数声明
*********************************************************************************************************/
static ssize_t  __procFsProcCmdlineRead(PLW_PROCFS_NODE  p_pfsn, 
                                        PCHAR            pcBuffer, 
                                        size_t           stMaxBytes,
                                        off_t            oft);
static ssize_t  __procFsProcMemRead(PLW_PROCFS_NODE  p_pfsn, 
                                    PCHAR            pcBuffer, 
                                    size_t           stMaxBytes,
                                    off_t            oft);
static ssize_t  __procFsProcModulesRead(PLW_PROCFS_NODE  p_pfsn, 
                                        PCHAR            pcBuffer, 
                                        size_t           stMaxBytes,
                                        off_t            oft);
static ssize_t  __procFsProcFiledescRead(PLW_PROCFS_NODE  p_pfsn, 
                                         PCHAR            pcBuffer, 
                                         size_t           stMaxBytes,
                                         off_t            oft);
static ssize_t  __procFsProcIoenvRead(PLW_PROCFS_NODE  p_pfsn, 
                                      PCHAR            pcBuffer, 
                                      size_t           stMaxBytes,
                                      off_t            oft);
/*********************************************************************************************************
  内核 proc 文件操作函数组
*********************************************************************************************************/
static LW_PROCFS_NODE_OP        _G_pfsnoProcCmdlineFuncs = {
    __procFsProcCmdlineRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoProcMemFuncs = {
    __procFsProcMemRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoProcModulesFuncs = {
    __procFsProcModulesRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoProcFiledescFuncs = {
    __procFsProcFiledescRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoProcIoenvFuncs = {
    __procFsProcIoenvRead, LW_NULL
};
/*********************************************************************************************************
** 函数名称: __procFsProcCmdlineRead
** 功能描述: procfs 读一个进程 cmdline 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsProcCmdlineRead (PLW_PROCFS_NODE  p_pfsn, 
                                         PCHAR            pcBuffer, 
                                         size_t           stMaxBytes,
                                         off_t            oft)
{
    REGISTER PCHAR        pcFileBuffer;
             size_t       stRealSize;                                   /*  实际的文件内容大小          */
             size_t       stCopeBytes;
             LW_LD_VPROC *pvproc;

    /*
     *  程序运行到这里, 文件缓冲一定已经分配了预置的内存大小(创建节点时预置大小为 64 字节).
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  需要生成文件                */
        LW_LD_LOCK();
        pvproc = (LW_LD_VPROC *)p_pfsn->PFSN_pvValue;
        if (!pvproc) {
            LW_LD_UNLOCK();
            return  (PX_ERROR);
        }
        stRealSize = bnprintf(pcFileBuffer, 
                              __VPROC_PROCFS_CMDLINE_SIZE, 0,
                              "%s",
                              pvproc->VP_pcName);
        LW_LD_UNLOCK();
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
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
** 函数名称: __procFsProcMemRead
** 功能描述: procfs 读一个进程 mem 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsProcMemRead (PLW_PROCFS_NODE  p_pfsn, 
                                     PCHAR            pcBuffer, 
                                     size_t           stMaxBytes,
                                     off_t            oft)
{
    REGISTER PCHAR        pcFileBuffer;
             size_t       stRealSize;                                   /*  实际的文件内容大小          */
             size_t       stCopeBytes;
             LW_LD_VPROC *pvproc;

    /*
     *  程序运行到这里, 文件缓冲一定已经分配了预置的内存大小(创建节点时预置大小为 64 字节).
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  需要生成文件                */
        INT                 i, iNum;
        ULONG               ulPages;
        size_t              stTotalMem;
        size_t              stMmapSize = 0;
        size_t              stStatic;
        BOOL                bStart;
    
        LW_LIST_RING       *pringTemp;
        LW_LD_EXEC_MODULE  *pmodTemp;
        
#if LW_CFG_VMM_EN == 0
        PLW_CLASS_HEAP      pheapVpPatch;
#endif                                                                  /*  LW_CFG_VMM_EN == 0          */

        PVOID               pvVmem[LW_LD_VMEM_MAX];
        
        LW_LD_LOCK();
        pvproc = (LW_LD_VPROC *)p_pfsn->PFSN_pvValue;
        if (!pvproc) {
            LW_LD_UNLOCK();
            return  (PX_ERROR);
        }
        
        LW_VP_LOCK(pvproc);
        stTotalMem = 0;
        for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
             pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
             pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {

            pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
            
#if LW_CFG_VMM_EN > 0
            ulPages = 0;
            if (API_VmmPCountInArea(pmodTemp->EMOD_pvBaseAddr, &ulPages) == ERROR_NONE) {
                stTotalMem += (size_t)(ulPages << LW_CFG_VMM_PAGE_SHIFT);
            }
#else
            stTotalMem += pmodTemp->EMOD_stLen;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
            stTotalMem += __moduleSymbolBufferSize(pmodTemp);
        }
        stStatic = stTotalMem;
        
        if (stTotalMem) {                                               /*  至少存在一个模块            */
            pmodTemp = _LIST_ENTRY(pvproc->VP_ringModules, LW_LD_EXEC_MODULE, EMOD_ringModules);
            ulPages  = 0;
            
#if LW_CFG_VMM_EN > 0
            iNum = __moduleVpPatchVmem(pmodTemp, pvVmem, LW_LD_VMEM_MAX);
            if (iNum > 0) {
                for (i = 0; i < iNum; i++) {
                    if (API_VmmPCountInArea(pvVmem[i], 
                                            &ulPages) == ERROR_NONE) {
                        stTotalMem += (size_t)(ulPages 
                                   << LW_CFG_VMM_PAGE_SHIFT);
                    }
                }
            }
#else
            pheapVpPatch = (PLW_CLASS_HEAP)__moduleVpPatchHeap(pmodTemp);
            if (pheapVpPatch) {                                         /*  获得 vp 进程私有 heap       */
                stTotalMem += (size_t)(pheapVpPatch->HEAP_stTotalByteSize);
            }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
        }
        LW_VP_UNLOCK(pvproc);
        
#if LW_CFG_VMM_EN > 0
        API_VmmMmapPCount(pvproc->VP_pid, &stMmapSize);                 /*  计算 mmap 内存实际消耗量    */
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
        LW_LD_UNLOCK();
        
        stRealSize = bnprintf(pcFileBuffer, 
                              __VPROC_PROCFS_MEM_SIZE, 0,
                              "static memory : %zu Bytes\n"
                              "heap memory   : %zu Bytes\n"
                              "mmap memory   : %zu Bytes\n"
                              "total memory  : %zu Bytes",
                              stStatic, (stTotalMem - stStatic), 
                              stMmapSize, (stTotalMem + stMmapSize));
                              
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
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
** 函数名称: __procFsProcModulesRead
** 功能描述: procfs 读一个进程 modules 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsProcModulesRead (PLW_PROCFS_NODE  p_pfsn, 
                                         PCHAR            pcBuffer, 
                                         size_t           stMaxBytes,
                                         off_t            oft)
{
    static const CHAR     cModuleInfoHdr[] = \
    "NAME HANDLE TYPE GLB BASE SIZE SYMCNT\n";

    REGISTER PCHAR        pcFileBuffer;
             size_t       stRealSize;                                   /*  实际的文件内容大小          */
             size_t       stCopeBytes;
             LW_LD_VPROC *pvproc;

    /*
     *  由于预置内存大小为 0 , 所以打开后第一次读取需要手动开辟内存.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  还没有分配内存              */
        size_t              stNeedBufferSize;
        BOOL                bStart;
        PCHAR               pcModuleName;
        
        LW_LIST_RING       *pringTemp;
        LW_LD_EXEC_MODULE  *pmodTemp;
        PCHAR               pcVpVersion;

        LW_LD_LOCK();
        pvproc = (LW_LD_VPROC *)p_pfsn->PFSN_pvValue;
        if (!pvproc) {
            LW_LD_UNLOCK();
            return  (PX_ERROR);
        }
        
        LW_VP_LOCK(pvproc);
        stNeedBufferSize = 0;
        for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
             pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
             pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {
            
            size_t  stSizeModule;
            size_t  stNameLen;
            
            pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
            _PathLastName(pmodTemp->EMOD_pcModulePath, &pcModuleName);
            stNameLen = lib_strlen(pcModuleName) + 1;
            stSizeModule = stNameLen + 100;                             /*  额外打印信息                */
             
            stNeedBufferSize += stSizeModule;
        }
        stNeedBufferSize += 100;                                        /*  vp 版本信息                 */
        LW_VP_UNLOCK(pvproc);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            errno = ENOMEM;
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  重新获得文件缓冲区地址      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cModuleInfoHdr);
        
        LW_VP_LOCK(pvproc);
        for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
             pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
             pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {

            pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

            _PathLastName(pmodTemp->EMOD_pcModulePath, &pcModuleName);

            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                   "%s %08lx %s %s %lx %lx %ld\n",
                   pcModuleName,
                   (addr_t)pmodTemp,
                   (((pmodTemp->EMOD_bIsGlobal) && (pmodTemp->EMOD_pcSymSection)) ? "KERNEL" : "USER"),
                   ((pmodTemp->EMOD_bIsGlobal) ? "YES" : "NO"),
                   (addr_t)pmodTemp->EMOD_pvBaseAddr,
                   (ULONG)pmodTemp->EMOD_stLen,
                   (ULONG)pmodTemp->EMOD_ulSymCount);
        }
        
        if (stRealSize) {                                               /*  有模块信息                  */
            pmodTemp    = _LIST_ENTRY(pvproc->VP_ringModules, LW_LD_EXEC_MODULE, EMOD_ringModules);
            pcVpVersion = __moduleVpPatchVersion(pmodTemp);
            if (pcVpVersion) {
                stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                   "<VP Ver:%s>\n", pcVpVersion);
            }
        }
        LW_VP_UNLOCK(pvproc);
        LW_LD_UNLOCK();
        
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
** 函数名称: __procFsProcFiledescRead
** 功能描述: procfs 读一个进程 filedesc 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsProcFiledescRead (PLW_PROCFS_NODE  p_pfsn, 
                                          PCHAR            pcBuffer, 
                                          size_t           stMaxBytes,
                                          off_t            oft)
{
    static const CHAR     cFiledescInfoHdr[] = \
    "FD NAME\n";
    
    REGISTER PCHAR        pcFileBuffer;
             size_t       stRealSize;                                   /*  实际的文件内容大小          */
             size_t       stCopeBytes;
             LW_LD_VPROC *pvproc;
             
    /*
     *  由于预置内存大小为 0 , 所以打开后第一次读取需要手动开辟内存.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  还没有分配内存              */
        INT             i;
        INT             iFileNum = 0;
        size_t          stNeedBufferSize;
        PLW_FD_ENTRY    pfdentry;
        
        LW_LD_LOCK();
        pvproc = (LW_LD_VPROC *)p_pfsn->PFSN_pvValue;
        if (!pvproc) {
            LW_LD_UNLOCK();
            return  (PX_ERROR);
        }
        
        _IosLock();
        for (i = 0; i < LW_CFG_MAX_FILES; i++) {
            if (vprocIoFileGetEx(pvproc, i, LW_TRUE)) {                 /*  获取总文件数, 包含异常文件  */
                iFileNum++;
            }
        }
        if (iFileNum <= 0) {
            stNeedBufferSize = sizeof(cFiledescInfoHdr);
        } else {
            stNeedBufferSize = 5 + (iFileNum * MAX_FILENAME_LENGTH);    /*  计算最大总内存需求数        */
        }
        _IosUnlock();
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {      /*  开辟内存                    */
            errno = ENOMEM;
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  重新获得文件缓冲区地址      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cFiledescInfoHdr);
        
        _IosLock();
        for (i = 0; i < LW_CFG_MAX_FILES; i++) {
            pfdentry = vprocIoFileGetEx(pvproc, i, LW_TRUE);
            if (pfdentry) {
                stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize,
                    "%d %s\n",
                    i, (pfdentry->FDENTRY_pcName) ? (pfdentry->FDENTRY_pcName) : "(unknown)");
            }
        }
        _IosUnlock();
        LW_LD_UNLOCK();
        
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
** 函数名称: __procFsProcIoenvRead
** 功能描述: procfs 读一个进程 ioenv 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsProcIoenvRead (PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft)
{
    REGISTER PCHAR        pcFileBuffer;
             size_t       stRealSize;                                   /*  实际的文件内容大小          */
             size_t       stCopeBytes;
             LW_LD_VPROC *pvproc;

    /*
     *  程序运行到这里, 文件缓冲一定已经分配了预置的内存大小(创建节点时预置大小为 64 字节).
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  需要生成文件                */
        LW_LD_LOCK();
        pvproc = (LW_LD_VPROC *)p_pfsn->PFSN_pvValue;
        if (!pvproc) {
            LW_LD_UNLOCK();
            return  (PX_ERROR);
        }
        stRealSize = bnprintf(pcFileBuffer, 
                              __VPROC_PROCFS_IOENV_SIZE, 0,
                              "umask:%x\nwd:%s",
                              pvproc->VP_pioeIoEnv->IOE_modeUMask,
                              pvproc->VP_pioeIoEnv->IOE_cDefPath);
        LW_LD_UNLOCK();
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
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
** 函数名称: vprocProcAdd
** 功能描述: 添加一个进程进入 proc 文件系统
** 输　入  : pvproc        进程控制块
** 输　出  : ERROR or NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vprocProcAdd (LW_LD_VPROC *pvproc)
{
    LW_PROCFS_NODE     *pfsn;
    PCHAR               pcName;
    
    pfsn = (LW_PROCFS_NODE *)__SHEAP_ALLOC(sizeof(LW_PROCFS_NODE) + 32);
    if (pfsn == LW_NULL) {
        return  (PX_ERROR);
    }
    pcName = (PCHAR)pfsn + sizeof(LW_PROCFS_NODE);
    lib_itoa(pvproc->VP_pid, pcName, 10);                               /*  不会超过 10 个字符          */
    
    LW_PROCFS_INIT_NODE_IN_CODE(pfsn, pcName, (S_IFDIR | S_IRUSR | S_IRGRP), LW_NULL, (PVOID)pvproc, 0);
    
    if (API_ProcFsMakeNode(pfsn, "/") < ERROR_NONE) {                   /*  加入 proc 文件系统          */
        __SHEAP_FREE(pfsn);
    }
    
    pvproc->VP_pvProcInfo = (PVOID)pfsn;
    
#if __VPROC_PROCFS_EXE_EN                                               /*  %d/exe 文件                 */
    {
        LW_PROCFS_NODE     *pfsnExe;
        PCHAR               pcExe;
        
        pfsnExe = (LW_PROCFS_NODE *)__SHEAP_ALLOC(sizeof(LW_PROCFS_NODE) + 
                                                  lib_strlen(__VPROC_PROCFS_EXE) + 1);
        if (pfsnExe == LW_NULL) {
            return  (PX_ERROR);
        }
        pcExe = (PCHAR)pfsnExe + sizeof(LW_PROCFS_NODE);
        lib_strcpy(pcExe, __VPROC_PROCFS_EXE);
        
        LW_PROCFS_INIT_SYMLINK_IN_CODE(pfsnExe, pcExe, (S_IFLNK | S_IRUSR | S_IRGRP), LW_NULL, 
                                       (PVOID)pvproc, pvproc->VP_pcName);
                                       
        if (API_ProcFsMakeNode(pfsnExe, pcName) < ERROR_NONE) {         /*  加入 proc 文件系统          */
            __SHEAP_FREE(pfsnExe);
        }
    }
#endif                                                                  /*  __VPROC_PROCFS_EXE_EN       */

#if __VPROC_PROCFS_CMDLINE_EN                                           /*  %d/cmdline 文件             */
    {
        LW_PROCFS_NODE     *pfsnCmdline;
        PCHAR               pcCmdline;
        
        pfsnCmdline = (LW_PROCFS_NODE *)__SHEAP_ALLOC(sizeof(LW_PROCFS_NODE) + 
                                                      lib_strlen(__VPROC_PROCFS_CMDLINE) + 1);
        if (pfsnCmdline == LW_NULL) {
            return  (PX_ERROR);
        }
        pcCmdline = (PCHAR)pfsnCmdline + sizeof(LW_PROCFS_NODE);
        lib_strcpy(pcCmdline, __VPROC_PROCFS_CMDLINE);
        
        LW_PROCFS_INIT_NODE_IN_CODE(pfsnCmdline, pcCmdline, (S_IFREG | S_IRUSR | S_IRGRP), 
                                    &_G_pfsnoProcCmdlineFuncs, 
                                    (PVOID)pvproc, __VPROC_PROCFS_CMDLINE_SIZE);
                                    
        if (API_ProcFsMakeNode(pfsnCmdline, pcName) < ERROR_NONE) {     /*  加入 proc 文件系统          */
            __SHEAP_FREE(pfsnCmdline);
        }
    }
#endif                                                                  /*  __VPROC_PROCFS_CMDLINE_EN   */

#if __VPROC_PROCFS_MEM_EN                                               /*  %d/mem 文件                 */
    {
        LW_PROCFS_NODE     *pfsnMem;
        PCHAR               pcMem;
        
        pfsnMem = (LW_PROCFS_NODE *)__SHEAP_ALLOC(sizeof(LW_PROCFS_NODE) + 
                                                  lib_strlen(__VPROC_PROCFS_MEM) + 1);
        if (pfsnMem == LW_NULL) {
            return  (PX_ERROR);
        }
        pcMem = (PCHAR)pfsnMem + sizeof(LW_PROCFS_NODE);
        lib_strcpy(pcMem, __VPROC_PROCFS_MEM);
        
        LW_PROCFS_INIT_NODE_IN_CODE(pfsnMem, pcMem, (S_IFREG | S_IRUSR | S_IRGRP), 
                                    &_G_pfsnoProcMemFuncs, 
                                    (PVOID)pvproc, __VPROC_PROCFS_MEM_SIZE);
                                    
        if (API_ProcFsMakeNode(pfsnMem, pcName) < ERROR_NONE) {         /*  加入 proc 文件系统          */
            __SHEAP_FREE(pfsnMem);
        }
    }
#endif                                                                  /*  __VPROC_PROCFS_MEM_EN       */

#if __VPROC_PROCFS_MODULES_EN                                           /*  %d/modules 文件             */
    {
        LW_PROCFS_NODE     *pfsnModules;
        PCHAR               pcModules;
        
        pfsnModules = (LW_PROCFS_NODE *)__SHEAP_ALLOC(sizeof(LW_PROCFS_NODE) + 
                                                      lib_strlen(__VPROC_PROCFS_MODULES) + 1);
        if (pfsnModules == LW_NULL) {
            return  (PX_ERROR);
        }
        pcModules = (PCHAR)pfsnModules + sizeof(LW_PROCFS_NODE);
        lib_strcpy(pcModules, __VPROC_PROCFS_MODULES);
        
        LW_PROCFS_INIT_NODE_IN_CODE(pfsnModules, pcModules, (S_IFREG | S_IRUSR | S_IRGRP), 
                                    &_G_pfsnoProcModulesFuncs, 
                                    (PVOID)pvproc, __VPROC_PROCFS_MODULES_SIZE);
                                    
        if (API_ProcFsMakeNode(pfsnModules, pcName) < ERROR_NONE) {     /*  加入 proc 文件系统          */
            __SHEAP_FREE(pfsnModules);
        }
    }
#endif                                                                  /*  __VPROC_PROCFS_MODULES_EN   */

#if __VPROC_PROCFS_FILEDESC_EN                                          /*  %d/filedesc 文件            */
    {
        LW_PROCFS_NODE     *pfsnFiledesc;
        PCHAR               pcFiledesc;
        
        pfsnFiledesc = (LW_PROCFS_NODE *)__SHEAP_ALLOC(sizeof(LW_PROCFS_NODE) + 
                                                       lib_strlen(__VPROC_PROCFS_FILEDESC) + 1);
        if (pfsnFiledesc == LW_NULL) {
            return  (PX_ERROR);
        }
        pcFiledesc = (PCHAR)pfsnFiledesc + sizeof(LW_PROCFS_NODE);
        lib_strcpy(pcFiledesc, __VPROC_PROCFS_FILEDESC);
        
        LW_PROCFS_INIT_NODE_IN_CODE(pfsnFiledesc, pcFiledesc, (S_IFREG | S_IRUSR | S_IRGRP), 
                                    &_G_pfsnoProcFiledescFuncs, 
                                    (PVOID)pvproc, __VPROC_PROCFS_FILEDESC_SIZE);
                                    
        if (API_ProcFsMakeNode(pfsnFiledesc, pcName) < ERROR_NONE) {    /*  加入 proc 文件系统          */
            __SHEAP_FREE(pfsnFiledesc);
        }
    }
#endif                                                                  /*  __VPROC_PROCFS_FILEDESC_EN  */
    
#if __VPROC_PROCFS_IOENV_EN                                             /*  %d/ioenv 文件               */
    {
        LW_PROCFS_NODE     *pfsnIoenv;
        PCHAR               pcIoenv;
        
        pfsnIoenv = (LW_PROCFS_NODE *)__SHEAP_ALLOC(sizeof(LW_PROCFS_NODE) + 
                                                    lib_strlen(__VPROC_PROCFS_IOENV) + 1);
        if (pfsnIoenv == LW_NULL) {
            return  (PX_ERROR);
        }
        pcIoenv = (PCHAR)pfsnIoenv + sizeof(LW_PROCFS_NODE);
        lib_strcpy(pcIoenv, __VPROC_PROCFS_IOENV);
        
        LW_PROCFS_INIT_NODE_IN_CODE(pfsnIoenv, pcIoenv, (S_IFREG | S_IRUSR | S_IRGRP), 
                                    &_G_pfsnoProcIoenvFuncs, 
                                    (PVOID)pvproc, __VPROC_PROCFS_IOENV_SIZE);
                                    
        if (API_ProcFsMakeNode(pfsnIoenv, pcName) < ERROR_NONE) {       /*  加入 proc 文件系统          */
            __SHEAP_FREE(pfsnIoenv);
        }
    }
#endif                                                                  /*  __VPROC_PROCFS_IOENV_EN     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __vprocProcFree
** 功能描述: 释放 proc 进程节点
** 输　入  : pfsn          procfs 节点
** 输　出  : ERROR or NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __vprocProcFree (LW_PROCFS_NODE  *pfsn)
{
    __SHEAP_FREE(pfsn);
}
/*********************************************************************************************************
** 函数名称: vprocProcDelete
** 功能描述: 从 proc 文件系统中删除一个进程信息
** 输　入  : pvproc        进程控制块
** 输　出  : ERROR or NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vprocProcDelete (LW_LD_VPROC *pvproc)
{
    LW_PROCFS_NODE     *pfsn = (LW_PROCFS_NODE *)pvproc->VP_pvProcInfo;
    LW_PROCFS_NODE     *pfsnSon;
    PLW_LIST_LINE       plineSon;
    
    if (pfsn == LW_NULL) {
        return  (PX_ERROR);
    }
    
    pvproc->VP_pvProcInfo = LW_NULL;
    
    plineSon = pfsn->PFSN_plineSon;
    while (plineSon) {
        pfsnSon  = _LIST_ENTRY(plineSon, LW_PROCFS_NODE, PFSN_lineBrother);
        plineSon = _list_line_get_next(plineSon);
        
        LW_LD_LOCK();
        pfsnSon->PFSN_pvValue = LW_NULL;
        LW_LD_UNLOCK();
        
        API_ProcFsRemoveNode(pfsnSon, __vprocProcFree);
    }
    
    LW_LD_LOCK();
    pfsn->PFSN_pvValue = LW_NULL;
    LW_LD_UNLOCK();
    
    API_ProcFsRemoveNode(pfsn, __vprocProcFree);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
                                                                        /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
