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
2010.01.07  修改一些 errno.
2010.03.10  升级 yaffs 修改相关接口.
2010.07.06  升级 yaffs 修改相关接口.
2010.08.11  简化了 read 操作.
2011.03.04  proc 文件 mode 为 S_IFREG
            修正计算最大剩余长度错误.
2011.03.13  更新 yaffs 文件系统.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_YAFFS_DRV
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_YAFFS_EN > 0) && (LW_CFG_PROCFS_EN > 0)
/*********************************************************************************************************
  yaffs proc 文件函数声明
*********************************************************************************************************/
static ssize_t  __procFsYaffsRead(PLW_PROCFS_NODE  p_pfsn, 
                                  PCHAR            pcBuffer, 
                                  size_t           stMaxBytes,
                                  off_t            oft);
/*********************************************************************************************************
  yaffs proc 文件操作函数组
*********************************************************************************************************/
static LW_PROCFS_NODE_OP        _G_pfsnoYaffsFuncs = {
    __procFsYaffsRead, LW_NULL
};
/*********************************************************************************************************
  yaffs proc 文件目录树
*********************************************************************************************************/
#define __PROCFS_BUFFER_SIZE_YAFFS      4096

static LW_PROCFS_NODE           _G_pfsnYaffs[] = 
{
    LW_PROCFS_INIT_NODE("yaffs",  (S_IRUSR | S_IRGRP | S_IROTH | S_IFREG), 
                        &_G_pfsnoYaffsFuncs, "Y", __PROCFS_BUFFER_SIZE_YAFFS),
};
/*********************************************************************************************************
** 函数名称: __procFsYaffsPrint
** 功能描述: 打印 yaffs 信息
** 输　入  : pyaffsDev     yaffs 设备
**           pcVolName     设备名
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           stOffset      偏移量
** 输　出  : 新的偏移量
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static size_t  __procFsYaffsPrint (struct yaffs_dev *pyaffsDev, 
                                   PCHAR             pcVolName,
                                   PCHAR             pcBuffer, 
                                   size_t            stMaxBytes,
                                   size_t            stOffset)
{
    return  (bnprintf(pcBuffer, stMaxBytes, stOffset,
                      "Device : \"%s\"\n"
                      "startBlock......... %d\n"
                      "endBlock........... %d\n"
                      "totalBytesPerChunk. %d\n"
                      "chunkGroupBits..... %d\n"
                      "chunkGroupSize..... %d\n"
                      "nErasedBlocks...... %d\n"
                      "nReservedBlocks.... %d\n"
                      "nCheckptResBlocks.. nil\n"
                      "blocksInCheckpoint. %d\n"
                      "nObjects........... %d\n"
                      "nTnodes............ %d\n"
                      "nFreeChunks........ %d\n"
                      "nPageWrites........ %d\n"
                      "nPageReads......... %d\n"
                      "nBlockErasures..... %d\n"
                      "nErasureFailures... %d\n"
                      "nGCCopies.......... %d\n"
                      "allGCs............. %d\n"
                      "passiveGCs......... %d\n"
                      "nRetriedWrites..... %d\n"
                      "nShortOpCaches..... %d\n"
                      "nRetiredBlocks..... %d\n"
                      "eccFixed........... %d\n"
                      "eccUnfixed......... %d\n"
                      "tagsEccFixed....... %d\n"
                      "tagsEccUnfixed..... %d\n"
                      "cacheHits.......... %d\n"
                      "nDeletedFiles...... %d\n"
                      "nUnlinkedFiles..... %d\n"
                      "nBackgroudDeletions %d\n"
                      "useNANDECC......... %d\n"
                      "isYaffs2........... %d\n\n",
                      
                      pcVolName,
                      pyaffsDev->param.start_block,
                      pyaffsDev->param.end_block,
                      pyaffsDev->param.total_bytes_per_chunk,
                      pyaffsDev->chunk_grp_bits,
                      pyaffsDev->chunk_grp_size,
                      pyaffsDev->n_erased_blocks,
                      pyaffsDev->param.n_reserved_blocks,
                      pyaffsDev->blocks_in_checkpt,
                      pyaffsDev->n_obj,
                      pyaffsDev->n_tnodes,
                      pyaffsDev->n_free_chunks,
                      pyaffsDev->n_page_writes,
                      pyaffsDev->n_page_reads,
                      pyaffsDev->n_erasures,
                      pyaffsDev->n_erase_failures,
                      pyaffsDev->n_gc_copies,
                      pyaffsDev->all_gcs,
                      pyaffsDev->passive_gc_count,
                      pyaffsDev->n_retried_writes,
                      pyaffsDev->param.n_caches,
                      pyaffsDev->n_retired_blocks,
                      pyaffsDev->n_ecc_fixed,
                      pyaffsDev->n_ecc_unfixed,
                      pyaffsDev->n_tags_ecc_fixed,
                      pyaffsDev->n_tags_ecc_unfixed,
                      pyaffsDev->cache_hits,
                      pyaffsDev->n_deleted_files,
                      pyaffsDev->n_unlinked_files,
                      pyaffsDev->n_bg_deletions,
                      pyaffsDev->param.use_nand_ecc,
                      pyaffsDev->param.is_yaffs2));
}
/*********************************************************************************************************
** 函数名称: __procFsYaffsRead
** 功能描述: procfs 读一个 yaffs 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsYaffsRead (PLW_PROCFS_NODE  p_pfsn, 
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
        /*
         *  在这里生成文件的内容
         */
        INT                 i = 0;
        PCHAR               pcVolName;
        struct yaffs_dev   *pyaffsDev;                                  /*  yaffs 挂载设备              */
        
        do {
            pcVolName = yaffs_getdevname(i, &i);
            if (pcVolName == LW_NULL) {
                break;                                                  /*  没有其他的卷                */
            }
            pyaffsDev = (struct yaffs_dev *)yaffs_getdev(pcVolName);
            if (pyaffsDev == LW_NULL) {
                break;                                                  /*  无法查询到设备节点          */
            }
            
            /*
             *  (__PROCFS_BUFFER_SIZE_YAFFS - stRealSize) 为文件缓冲区剩余的空间大小
             */
            stRealSize = __procFsYaffsPrint(pyaffsDev, 
                                            pcVolName,
                                            pcFileBuffer,
                                            __PROCFS_BUFFER_SIZE_YAFFS,
                                            stRealSize);
        } while (i != PX_ERROR);
    
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
** 函数名称: __procFsYaffsInit
** 功能描述: procfs 初始化 yaffs proc 文件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __procFsYaffsInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnYaffs[0], "/");
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_YAFFS_EN > 0)       */
                                                                        /*  (LW_CFG_PROCFS_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
