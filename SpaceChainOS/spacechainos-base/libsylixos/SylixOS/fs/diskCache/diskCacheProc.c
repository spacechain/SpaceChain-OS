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
** 文   件   名: yaffs_sylixosproc.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 11 日
**
** 描        述: yaffs 在 /proc 中的信息.

** BUG:
2010.08.11  简化了 read 操作.
2011.03.04  proc 文件 mode 为 S_IFREG.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
#include "diskCacheLib.h"
#include "diskCache.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0) && (LW_CFG_PROCFS_EN > 0)
/*********************************************************************************************************
  锁操作
*********************************************************************************************************/
#define __LW_DISKCACHE_LIST_LOCK()      \
        API_SemaphoreMPend(_G_ulDiskCacheListLock, LW_OPTION_WAIT_INFINITE)
#define __LW_DISKCACHE_LIST_UNLOCK()    \
        API_SemaphoreMPost(_G_ulDiskCacheListLock)
/*********************************************************************************************************
  diskcache proc 文件函数声明
*********************************************************************************************************/
static ssize_t  __procFsDiskCacheRead(PLW_PROCFS_NODE  p_pfsn, 
                                      PCHAR            pcBuffer, 
                                      size_t           stMaxBytes,
                                      off_t            oft);
/*********************************************************************************************************
  diskcache proc 文件操作函数组
*********************************************************************************************************/
static LW_PROCFS_NODE_OP        _G_pfsnoDiskCacheFuncs = {
    __procFsDiskCacheRead, LW_NULL
};
/*********************************************************************************************************
  diskcache proc 文件目录树
*********************************************************************************************************/
#define __PROCFS_BUFFER_SIZE_DISKCACHE  2048

static LW_PROCFS_NODE           _G_pfsnDiskCache[] = 
{
    LW_PROCFS_INIT_NODE("diskcache",  (S_IRUSR | S_IRGRP | S_IROTH | S_IFREG), 
                        &_G_pfsnoDiskCacheFuncs, "D", 
                        __PROCFS_BUFFER_SIZE_DISKCACHE),
};
/*********************************************************************************************************
** 函数名称: __procFsDiskCachePrint
** 功能描述: 打印 diskcache 信息
** 输　入  : pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
** 输　出  : 实际生成的信息大小
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static size_t  __procFsDiskCachePrint (PCHAR  pcBuffer, size_t  stMaxBytes)
{
    size_t              stRealSize;
    PCHAR               pcName;
    PLW_LIST_LINE       plineTemp;
    PLW_DISKCACHE_CB    pdiskcDiskCache;
    
    stRealSize = bnprintf(pcBuffer, stMaxBytes, 0, "DO NOT INCLUDE 'NAND' READ CACHE INFO.\n\n");
    stRealSize = bnprintf(pcBuffer, stMaxBytes, stRealSize,
                          "       NAME       OPT SECTOR-SIZE TOTAL-SECs VALID-SECs DIRTY-SECs BURST-R BURST-W  HASH\n"
                          "----------------- --- ----------- ---------- ---------- ---------- ------- ------- ------\n");
                           
    __LW_DISKCACHE_LIST_LOCK();
    for (plineTemp  = _G_plineDiskCacheHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pdiskcDiskCache = _LIST_ENTRY(plineTemp, 
                                      LW_DISKCACHE_CB, 
                                      DISKC_lineManage);
                                      
        if ((pdiskcDiskCache->DISKC_blkdCache.BLKD_pcName) &&
            (pdiskcDiskCache->DISKC_blkdCache.BLKD_pcName[0] != PX_EOS)) {
            pcName = pdiskcDiskCache->DISKC_blkdCache.BLKD_pcName;
        
        } else {
            pcName = "<unkown>";
        }
                                      
        stRealSize = bnprintf(pcBuffer, stMaxBytes, stRealSize,
                              "%-17s %3d %11lu %10lu %10lu %10lu %7d %7d %6d\n",
                              pcName,
                              pdiskcDiskCache->DISKC_iCacheOpt,
                              pdiskcDiskCache->DISKC_ulBytesPerSector,
                              pdiskcDiskCache->DISKC_ulNCacheNode,
                              pdiskcDiskCache->DISKC_ulValidCounter,
                              pdiskcDiskCache->DISKC_ulDirtyCounter,
                              pdiskcDiskCache->DISKC_iMaxRBurstSector,
                              pdiskcDiskCache->DISKC_iMaxWBurstSector,
                              pdiskcDiskCache->DISKC_iHashSize);
        
    }
    __LW_DISKCACHE_LIST_UNLOCK();
    
    return  (stRealSize);
}
/*********************************************************************************************************
** 函数名称: __procFsDiskCacheRead
** 功能描述: procfs 读一个 diskcache 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsDiskCacheRead (PLW_PROCFS_NODE  p_pfsn, 
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
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  需要生成文件                */
        /*
         *  在这里生成文件的内容
         */
        stRealSize = __procFsDiskCachePrint(pcFileBuffer, __PROCFS_BUFFER_SIZE_DISKCACHE);
        
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);              /*  回写文件实际大小            */
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
** 函数名称: __procFsDiskCacheInit
** 功能描述: procfs 初始化 diskcache proc 文件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __procFsDiskCacheInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnDiskCache[0], "/");
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
                                                                        /*  (LW_CFG_PROCFS_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
