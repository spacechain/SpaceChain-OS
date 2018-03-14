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
** 文   件   名: yaffs_sylixos.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 12 月 07 日
**
** 描        述: yaffs 与操作系统兼容函数.
**
** 说        明: 所有的 yaffs 卷都挂载在一个 yaffs 设备上, 而不是 fat 等一个设备一个卷.

** BUG
2008.12.19  升级 format 函数. 提高效率.
2009.02.18  修正了删除目录的错误.
2009.03.14  对文件系统参数的获取支持 4GB 以上的卷. (__yaffsStatfsGet)
2009.04.07  format 互斥更加安全.
2009.04.22  在卸载卷时, 不关闭所有文件, 而是将有关文件变为异常状态.
2009.06.05  加入文件互斥保护, 当一个文件被写方式打开, 他不能被打开. 当文件被读方式打开, 打不能被初读以外
            的方式打开. 
2009.06.10  设置文件时间时, 要检测文件的可读写性.
2009.06.18  修正文件锁判断的问题.
2009.06.30  加入创建文件名错误的判断.
2009.07.05  使用 fsCommon 提供的函数进行文件名判断.
2009.07.09  支持 64 bit 文件指针.
2009.07.12  支持打开卷文件进行格式化.
            __yaffsFormat() 通过扫描设备链直接确定设备.
2009.08.08  __yaffsTimeset() 加入 errno 操作.
2009.08.26  readdir 时, 需要将 d_shortname 清零, 表示不支持短文件名.
2009.08.29  readdir 时, 可以列出 yaffs 设备上所有挂接的卷.
2009.09.28  升级 yaffs 文件系统, 使用 yaffs_fsync() 替换 yaffs_flush().
2009.10.22  read write 应为 ssize_t 类型.
2009.11.18  __yaffsStatGet() 确保根目录挂载设备损坏时, 还可以获得 stat.
2009.12.11  在 /proc 中加入 yaffs 文件显示磁盘相关信息.
2010.01.11  重命名的新文件名需要解压缩.
2010.01.14  支持 FIOSETFL 命令.
2010.03.10  升级 yaffs 修改相关接口.
2010.07.06  修正 yaffs 初始化接口调用(与当前版本一致 API_YaffsDrvInstall)
2010.09.03  __yaffsRemove() 卸载 yaffs 卷标时, 需要调用 yaffs_uninstalldev() 删除指定的卷.
                            卸载整个 yaffs io 设备时, 需要 yaffs_unmount() 所有挂在的卷.
2010.09.08  根据需要配置 __yaffsRemove() 时是否需要调用 yaffs_unmount() 函数.
2010.09.09  加入 d_type 设置.
2011.03.13  更新 yaffs 文件系统.
2011.03.22  加入 yaffs 一些底层命令.
2011.03.27  加入对创建文件类型支持类的判断.
2011.03.29  当 iosDevAddEx() 出错时需要返回 iosDevAddEx() 的错误号.
2011.07.24  yaffs 不再可以配置使用独立内存堆工作, 而必须使用系统内存堆.
2011.08.07  加入 yaffs 对符号链接的支持. 在 yaffscfg.c 中增加了两个 SylixOS 需要的符号链接函数.
2011.08.11  压缩目录不再由文件系统完成, 操作系统已处理了.
2012.03.10  open 操作时, 应首先判断链接文件情况, 然后再打开.
2012.03.11  yaffs 直接使用系统时间.
2012.03.31  yaffs 支持 fdatasync 操作.
2012.08.16  支持 pread 与 pwrite.  
2012.09.01  加入不可强制卸载卷的保护.
2012.09.25  yaffs 可以创建 socket 类型文件.
2012.11.09  __yaffsFsReadlink() 需要返回有效字节数.
2013.01.06  yaffs 使用最新 NEW_1 设备驱动类型. 这样可以支持文件锁.
2013.07.10  升级 yaffs 后, 支持通过 fd 操作目录, 这里不再保留以往目录的处理方式.
2013.07.12  升级 yaffs 后, 不再使用以前的格式化操作, 转而使用 yaffs 自带的 yaffs_format 函数进行格式化.
2017.04.27  yaffs 内部使用 O_RDWR 打开, 防止多重打开时内部权限判断错误.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_YAFFS_DRV
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
#include "../SylixOS/fs/include/fs_fs.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_YAFFS_EN > 0)
/*********************************************************************************************************
  内部全局变量
*********************************************************************************************************/
static LW_OBJECT_HANDLE _G_hYaffsOpLock;
static INT              _G_iYaffsDrvNum = PX_ERROR;
static BOOL             _G_bIsCreateDev = LW_FALSE;
static PCHAR            _G_pcDevName    = LW_NULL;
/*********************************************************************************************************
  内部结构
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR          YAFFS_devhdrHdr;                                /*  yaffs 文件系统设备头        */
    BOOL                YAFFS_bForceDelete;
    LW_LIST_LINE_HEADER YAFFS_plineFdNodeHeader;                        /*  fd_node 链表                */
} YAFFS_FSLIB;
typedef YAFFS_FSLIB    *PYAFFS_FSLIB;

typedef struct {
    PYAFFS_FSLIB        YAFFIL_pyaffs;                                  /*  指向 yaffs 设备             */
    INT                 YAFFIL_iFd;                                     /*  yaffs 文件描述符            */
    INT                 YAFFIL_iFileType;                               /*  文件类型                    */
    CHAR                YAFFIL_cName[1];                                /*  文件名                      */
} YAFFS_FILE;
typedef YAFFS_FILE     *PYAFFS_FILE;
/*********************************************************************************************************
  文件类型
*********************************************************************************************************/
#define __YAFFS_FILE_TYPE_NODE          0                               /*  open 打开文件               */
#define __YAFFS_FILE_TYPE_DIR           1                               /*  open 打开目录               */
#define __YAFFS_FILE_TYPE_DEV           2                               /*  open 打开设备               */
/*********************************************************************************************************
  检测路径字串是否为根目录或者直接指向设备
*********************************************************************************************************/
#define __STR_IS_ROOT(pcName)           ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))
#define __STR_IS_YVOL(pcName)           (lib_rindex(pcName, PX_DIVIDER) == pcName)
/*********************************************************************************************************
  内部操作宏
*********************************************************************************************************/
#define __YAFFS_OPLOCK()                API_SemaphoreMPend(_G_hYaffsOpLock, LW_OPTION_WAIT_INFINITE)
#define __YAFFS_OPUNLOCK()              API_SemaphoreMPost(_G_hYaffsOpLock)
/*********************************************************************************************************
  内部函数
*********************************************************************************************************/
static LONG     __yaffsOpen(PYAFFS_FSLIB    pyaffs,
                            PCHAR           pcName,
                            INT             iFlags,
                            INT             iMode);
static INT      __yaffsRemove(PYAFFS_FSLIB    pyaffs,
                            PCHAR           pcName);
static INT      __yaffsClose(PLW_FD_ENTRY   pfdentry);
static ssize_t  __yaffsRead(PLW_FD_ENTRY    pfdentry,
                            PCHAR           pcBuffer, 
                            size_t          stMaxBytes);
static ssize_t  __yaffsPRead(PLW_FD_ENTRY    pfdentry,
                             PCHAR           pcBuffer, 
                             size_t          stMaxBytes,
                             off_t          oftPos);
static ssize_t  __yaffsWrite(PLW_FD_ENTRY  pfdentry,
                             PCHAR         pcBuffer, 
                             size_t        stNBytes);
static ssize_t  __yaffsPWrite(PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer, 
                              size_t        stNBytes,
                              off_t          oftPos);
static INT      __yaffsLStatGet(PYAFFS_FSLIB  pyaffs, 
                                PCHAR         pcName, 
                                struct stat  *pstat);
static INT      __yaffsIoctl(PLW_FD_ENTRY  pfdentry,
                             INT           iRequest,
                             LONG          lArg);
static INT      __yaffsFlush(PLW_FD_ENTRY   pfdentry);
static INT      __yaffsDataSync(PLW_FD_ENTRY  pfdentry);
static INT      __yaffsFsSymlink(PYAFFS_FSLIB  pyaffs, 
                                 PCHAR         pcName, 
                                 CPCHAR        pcLinkDst);
static ssize_t __yaffsFsReadlink(PYAFFS_FSLIB  pyaffs, 
                                 PCHAR         pcName,
                                 PCHAR         pcLinkDst,
                                 size_t        stMaxSize);
/*********************************************************************************************************
  yaffs proc 信息
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0
VOID  __procFsYaffsInit(VOID);
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
** 函数名称: __yaffsOsInit
** 功能描述: yaffs 文件系统操作系统接口初始化.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __yaffsOsInit (VOID)
{
    _G_hYaffsOpLock = API_SemaphoreMCreate("yaffs_oplock", LW_PRIO_DEF_CEILING, 
                                         LW_OPTION_WAIT_PRIORITY | LW_OPTION_INHERIT_PRIORITY | 
                                         LW_OPTION_DELETE_SAFE | LW_OPTION_OBJECT_GLOBAL,
                                         LW_NULL);
    if (!_G_hYaffsOpLock) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "yaffs oplock can not create.\r\n");
        return;
    }
}
/*********************************************************************************************************
** 函数名称: __tshellYaffsCmd
** 功能描述: yaffs 文件系统底层命令.
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellYaffsCmd (INT  iArgC, PCHAR  ppcArgV[])
{
    INT                  i;
    struct yaffs_dev    *pyaffsDev;
    
    if (iArgC < 3) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    __YAFFS_OPLOCK();
    pyaffsDev = (struct yaffs_dev *)yaffs_getdev(ppcArgV[1]);
    if (pyaffsDev == LW_NULL) {                                         /*  查找 nand 设备              */
        __YAFFS_OPUNLOCK();
        fprintf(stderr, "can not find yaffs device!\n");                /*  无法搜索到设备              */
        return  (PX_ERROR);
    }
    
    if (lib_strcmp("bad", ppcArgV[2]) == 0) {                           /*  打印坏块信息                */
        for (i = pyaffsDev->param.start_block; i <= pyaffsDev->param.end_block; i++) {
            enum yaffs_block_state  state = YAFFS_BLOCK_STATE_UNKNOWN;
            u32                     sequenceNumber;
            yaffs_query_init_block_state(pyaffsDev, i, &state, &sequenceNumber);
            if (state == YAFFS_BLOCK_STATE_DEAD) {
                printf("block 0x%x is bad block.\n", i);
            }
        }
    
    } else if (lib_strcmp("info", ppcArgV[2]) == 0) {                   /*  打印卷信息                  */
        printf("Device : \"%s\"\n"
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
               pyaffsDev->param.name,
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
               pyaffsDev->param.is_yaffs2);
    
    } else if (lib_strcmp("markbad", ppcArgV[2]) == 0) {                /*  将指定的块设为坏块          */
        int     iBlock = -1;
        
        if (iArgC < 4) {
            __YAFFS_OPUNLOCK();
            fprintf(stderr, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        sscanf(ppcArgV[3], "%x", &iBlock);                              /*  获取块的编号                */
        
        if (yaffs_mark_bad(pyaffsDev, iBlock) == YAFFS_OK) {
            printf("mark the block 0x%x is a bad ok.\n", iBlock);
        } else {
            __YAFFS_OPUNLOCK();
            printf("mark the block 0x%x is a bad error!\n", iBlock);
            return  (PX_ERROR);
        }
    
    } else if (lib_strcmp("erase", ppcArgV[2]) == 0) {                  /*  擦除芯片 (符合驱动的规则)   */
        for (i = pyaffsDev->param.start_block; i <= pyaffsDev->param.end_block; i++) {
            yaffs_erase_block(pyaffsDev, i);
        }
        
        printf("yaffs volume erase ok.\n");
    }
    __YAFFS_OPUNLOCK();
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** 函数名称: API_YaffsDrvInstall
** 功能描述: 安装 yaffs 文件系统驱动程序
** 输　入  : 
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_YaffsDrvInstall (VOID)
{
    struct file_operations     fileop;
    
    if (_G_iYaffsDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));
    
    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __yaffsOpen;
    fileop.fo_release  = __yaffsRemove;
    fileop.fo_open     = __yaffsOpen;
    fileop.fo_close    = __yaffsClose;
    fileop.fo_read     = __yaffsRead;
    fileop.fo_read_ex  = __yaffsPRead;
    fileop.fo_write    = __yaffsWrite;
    fileop.fo_write_ex = __yaffsPWrite;
    fileop.fo_lstat    = __yaffsLStatGet;
    fileop.fo_ioctl    = __yaffsIoctl;
    fileop.fo_symlink  = __yaffsFsSymlink;
    fileop.fo_readlink = __yaffsFsReadlink;
    
    _G_iYaffsDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);     /*  新型 NEW_1 驱动             */
    
    DRIVER_LICENSE(_G_iYaffsDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iYaffsDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iYaffsDrvNum, "yaffs2 driver.");
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "yaffs2 file system installed.\r\n");
    yaffs_start_up();                                                   /*  初始化 YAFFS                */

#if LW_CFG_PROCFS_EN > 0
    __procFsYaffsInit();                                                /*  建立 proc 中的节点          */
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */

#if LW_CFG_SHELL_EN > 0
    API_TShellKeywordAdd("yaffscmd", __tshellYaffsCmd);
    API_TShellFormatAdd("yaffscmd", " volname [{bad | info | markbad | erase}]");
    API_TShellHelpAdd("yaffscmd", "eg. yaffscmd n0 bad         show volume \"n0\" bad block.\n"
                                  "    yaffscmd n0 info        show volume \"n0\" infomation.\n"
                                  "    yaffscmd n0 markbad 3a  mark block 0x3a is a bad block.\n"
                                  "    yaffscmd n1 erase       erase volume \"n1\"\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

    return  ((_G_iYaffsDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** 函数名称: API_YaffsDevCreate
** 功能描述: 创建 Yaffs 设备. 主需要创建一次, 其他 yaffs 卷全部挂接在这个设备上.
**           与 sylixos FAT 不同, yaffs 每一个卷都是挂接在唯一的 yaffs 根设备上.
** 输　入  : pcName            设备名(设备挂接的节点地址)
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_YaffsDevCreate (PCHAR   pcName)
{    
    REGISTER PYAFFS_FSLIB    pyaffs;

    if (_G_iYaffsDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no yaffs driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (_G_bIsCreateDev) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is another yaffs device.\r\n");
        _ErrorHandle(ERROR_IO_FILE_EXIST);
        return  (PX_ERROR);
    }
    
    pyaffs = (PYAFFS_FSLIB)__SHEAP_ALLOC(sizeof(YAFFS_FSLIB));
    if (pyaffs == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pyaffs, sizeof(YAFFS_FSLIB));
    
    pyaffs->YAFFS_bForceDelete      = LW_FALSE;
    pyaffs->YAFFS_plineFdNodeHeader = LW_NULL;
    
    if (iosDevAddEx(&pyaffs->YAFFS_devhdrHdr, pcName, _G_iYaffsDrvNum, DT_DIR)
        != ERROR_NONE) {                                                /*  安装文件系统设备            */
        __SHEAP_FREE(pyaffs);
        return  (PX_ERROR);
    }
    _G_bIsCreateDev = LW_TRUE;                                          /*  创建 yaffs 设备成功         */
    _G_pcDevName    = pyaffs->YAFFS_devhdrHdr.DEVHDR_pcName;
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "yaffs \"%s\" has been create.\r\n", pcName);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_YaffsDevDelete
** 功能描述: 删除一个 yaffs 挂载设备, 例如: API_YaffsDevDelete("/yaffs2/n0");
** 输　入  : pcName            设备名(设备挂接的节点地址)
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_YaffsDevDelete (PCHAR   pcName)
{
    if (API_IosDevMatchFull(pcName)) {                                  /*  如果是设备, 这里就卸载设备  */
        return  (unlink(pcName));
    
    } else {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_YaffsDevMountShow
** 功能描述: 显示所有 yaffs 挂载设备
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID  API_YaffsDevMountShow (VOID)
{
    PCHAR   pcMountInfoHdr = "       VOLUME                    BLK NAME\n"
                             "-------------------- --------------------------------\n";
    PCHAR   pcMtdName;
    CHAR    cVolPath[MAX_FILENAME_LENGTH];
    INT     iIndex = 0;
    INT     iNext;
    
    printf("MTD-Mount point show >>\n");
    printf(pcMountInfoHdr);                                             /*  打印欢迎信息                */
    
    if (_G_pcDevName) {
        do {
            pcMtdName = yaffs_getdevname(iIndex, &iNext);
            if (pcMtdName) {
                iIndex = iNext;
                snprintf(cVolPath, sizeof(cVolPath), "%s%s", _G_pcDevName, pcMtdName);
                printf("%-20s MTD:%s\n", cVolPath, pcMtdName);
            }
        } while (pcMtdName);
    }
}
/*********************************************************************************************************
** 函数名称: __yaffsCloseFile
** 功能描述: yaffs 内部关闭一个文件
** 输　入  : pyaffile         yaffs 文件
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __yaffsCloseFile (PYAFFS_FILE   pyaffile)
{
    if (pyaffile) {
        if ((pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) ||
            (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_DIR)) {
            yaffs_close(pyaffile->YAFFIL_iFd);                          /*  标准 yaffs 文件             */
        }
    }
}
/*********************************************************************************************************
** 函数名称: __yaffsOpen
** 功能描述: yaffs open 操作
** 输　入  : pyaffs           设备
**           pcName           文件名
**           iFlags           方式
**           iMode            方法
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  __yaffsOpen (PYAFFS_FSLIB    pyaffs,
                          PCHAR           pcName,
                          INT             iFlags,
                          INT             iMode)
{
             INT            iError;
    REGISTER PYAFFS_FILE    pyaffile;
             PLW_FD_NODE    pfdnode;
             BOOL           bIsNew;
    struct yaffs_stat       yafstat;

    if (pcName == LW_NULL) {                                            /*  无文件名                    */
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {                                         /*  创建操作                    */
            if (__fsCheckFileName(pcName)) {
                _ErrorHandle(ENOENT);
                return  (PX_ERROR);
            }
            if (S_ISFIFO(iMode) || 
                S_ISBLK(iMode)  ||
                S_ISCHR(iMode)) {
                _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);                /*  不支持以上这些格式          */
                return  (PX_ERROR);
            }
        }
        
        pyaffile = (PYAFFS_FILE)__SHEAP_ALLOC(sizeof(YAFFS_FILE) + 
                                              lib_strlen(pcName));      /*  创建文件内存                */
        if (pyaffile == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        lib_strcpy(pyaffile->YAFFIL_cName, pcName);                     /*  记录文件名                  */
        
        pyaffile->YAFFIL_pyaffs = pyaffs;
        
        __YAFFS_OPLOCK();
        /*
         *  首先处理符号链接文件情况
         */
        {
            INT     iFollowLinkType;
            PCHAR   pcTail, pcSymfile, pcPrefix;
            struct yaffs_obj *obj = yaffsfs_FindSymlink(pcName, &pcTail, &pcSymfile);
                                                                        /*  检查目录是否包含 symlink    */
            if (obj) {                                                  /*  目录中有 symlink            */
                pcSymfile--;                                            /*  从 / 开始                   */
                if (pcSymfile == pcName) {
                    pcPrefix = LW_NULL;                                 /*  没有前缀                    */
                } else {
                    pcPrefix = pcName;
                    *pcSymfile = PX_EOS;
                }
                if (pcTail && lib_strlen(pcTail)) {
                    iFollowLinkType = FOLLOW_LINK_TAIL;                 /*  连接目标内部文件            */
                } else {
                    iFollowLinkType = FOLLOW_LINK_FILE;                 /*  链接文件本身                */
                }
                
                if (yaffsfs_PathBuildLink(pcName, pyaffs->YAFFS_devhdrHdr.DEVHDR_pcName,
                                          pcPrefix, obj, pcTail) == ERROR_NONE) {
                                                                        /*  构造链接目标                */
                    __YAFFS_OPUNLOCK();
                    __SHEAP_FREE(pyaffile);
                    return  (iFollowLinkType);
                
                } else {
                    __YAFFS_OPUNLOCK();
                    __SHEAP_FREE(pyaffile);
                    return  (PX_ERROR);
                }
            }
        }
        
        /*
         *  非符号链接文件.
         */
        if ((iFlags & O_CREAT) && S_ISDIR(iMode)) {                     /*  创建目录                    */
            iError = yaffs_mkdir(pcName, iMode);
            if ((iError != ERROR_NONE) && (iFlags & O_EXCL)) {
                __YAFFS_OPUNLOCK();
                __SHEAP_FREE(pyaffile);
                return  (PX_ERROR);
            }
            iError = yaffs_open(pcName, O_RDONLY, iMode);               /*  打开目录                    */
        
        } else {                                                        /*  打开或创建文件              */
            iError = yaffs_open(pcName, (iFlags & ~O_BINARY), iMode);   /*  打开普通文件                */
        }                                                               /*  yaffs 打开目录不能有此选项  */

        pyaffile->YAFFIL_iFd = iError;
        pyaffile->YAFFIL_iFileType = __YAFFS_FILE_TYPE_NODE;            /*  可以被 close 掉             */
        
        if (iError < ERROR_NONE) {                                      /*  产生错误                    */
            if (__STR_IS_ROOT(pyaffile->YAFFIL_cName)) {                /*  是否为 yaffs 文件系统设备   */
                yafstat.st_dev  = (int)pyaffile->YAFFIL_pyaffs;
                yafstat.st_ino  = (int)0;                               /*  绝不与普通文件重复(根目录)  */
                yafstat.st_mode = YAFFS_ROOT_MODE | S_IFDIR;
                yafstat.st_uid  = 0;
                yafstat.st_gid  = 0;
                yafstat.st_size = 0;
                pyaffile->YAFFIL_iFileType = __YAFFS_FILE_TYPE_DEV;
                goto    __file_open_ok;                                 /*  设备打开正常                */
            }
            __YAFFS_OPUNLOCK();
            __SHEAP_FREE(pyaffile);
            return  (PX_ERROR);                                         /*  打开失败                    */
        
        } else {
            yaffs_fstat(pyaffile->YAFFIL_iFd, &yafstat);                /*  获得文件属性                */
            if (S_ISDIR(yafstat.st_mode)) {
                pyaffile->YAFFIL_iFileType = __YAFFS_FILE_TYPE_DIR;     /*  目录                        */
            } else {
                yaffs_handle_rw_set(pyaffile->YAFFIL_iFd, 1, 1);        /*  拥有读写权限                */
            }
        }
        
__file_open_ok:
        pfdnode = API_IosFdNodeAdd(&pyaffs->YAFFS_plineFdNodeHeader,
                                    (dev_t)yafstat.st_dev,
                                    (ino64_t)yafstat.st_ino,
                                    iFlags,
                                    iMode,
                                    yafstat.st_uid,
                                    yafstat.st_gid,
                                    yafstat.st_size,
                                    (PVOID)pyaffile,
                                    &bIsNew);                           /*  添加文件节点                */
        if (pfdnode == LW_NULL) {                                       /*  无法创建 fd_node 节点       */
            __yaffsCloseFile(pyaffile);
            __YAFFS_OPUNLOCK();
            __SHEAP_FREE(pyaffile);
            return  (PX_ERROR);
        }
        
        LW_DEV_INC_USE_COUNT(&pyaffs->YAFFS_devhdrHdr);                 /*  更新计数器                  */
        
        if (bIsNew == LW_FALSE) {                                       /*  有重复打开                  */
            __yaffsCloseFile(pyaffile);
            __YAFFS_OPUNLOCK();
            __SHEAP_FREE(pyaffile);
        
        } else {
            __YAFFS_OPUNLOCK();
        }
        
        return  ((LONG)pfdnode);
    }
}
/*********************************************************************************************************
** 函数名称: __yaffsRemove
** 功能描述: yaffs remove 操作
** 输　入  : pyaffs           设备
**           pcName           文件名
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __yaffsRemove (PYAFFS_FSLIB    pyaffs,
                           PCHAR           pcName)
{
    REGISTER INT    iError;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    } else {
    
        __YAFFS_OPLOCK();
        
        if (__STR_IS_ROOT(pcName)) {                                    /*  根目录或者 yaffs 设备文件   */
__re_umount_vol:
            if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)pyaffs)) {           /*  检查是否有正在工作的文件    */
                if (!pyaffs->YAFFS_bForceDelete) {
                    _ErrorHandle(EBUSY);
                    return  (PX_ERROR);
                }
                
                __YAFFS_OPUNLOCK();
                
                _DebugHandle(__ERRORMESSAGE_LEVEL, "disk have open file.\r\n");
                iosDevFileAbnormal(&pyaffs->YAFFS_devhdrHdr);           /*  将所有相关文件设为异常模式  */
            
                __YAFFS_OPLOCK();
                goto    __re_umount_vol;
            }
            
            iosDevDelete((LW_DEV_HDR *)pyaffs);                         /*  IO 系统移除设备             */
            _G_bIsCreateDev = LW_FALSE;                                 /*  卸载 YAFFS 设备             */
            
#if LW_CFG_YAFFS_UNMOUNT_VOL > 0
            {
                INT                iIndex   = 0;
                PCHAR              pcDelVol = LW_NULL;
                struct yaffs_dev  *pyaffsdev;
                
                /*
                 *  卸载所有挂载的卷 (yaffs 需要 update check point)
                 */
                do {
                    pcDelVol = yaffs_getdevname(iIndex, &iIndex);       /*  获得下一个卷名              */
                    if (pcDelVol) {
                        iError = yaffs_unmount(pcDelVol);               /*  卸载设备                    */
                        if (iError >= 0) {
                            pyaffsdev = (struct yaffs_dev *)yaffs_getdev(pcDelVol);
                            if (pyaffsdev) {
                                yaffs_remove_device(pyaffsdev);         /*  从设备表中删除设备          */
                            }
                        }
                    }
                } while (pcDelVol);
            }
#endif                                                                  /*  LW_CFG_YAFFS_UNMOUNT_VOL > 0*/
            
            __YAFFS_OPUNLOCK();
            __SHEAP_FREE(pyaffs);
            
            _DebugHandle(__LOGMESSAGE_LEVEL, "yaffs unmount ok.\r\n");
            
            return  (ERROR_NONE);
        
        } else if (__STR_IS_YVOL(pcName)) {                             /*  yaffs 挂载的设备            */
            struct yaffs_dev  *pyaffsdev;
            
            iError = yaffs_unmount(pcName);                             /*  卸载设备                    */
            if (iError >= 0) {
                pyaffsdev = (struct yaffs_dev *)yaffs_getdev(pcName);
                if (pyaffsdev) {
                    yaffs_remove_device(pyaffsdev);                     /*  从设备表中删除设备          */
                }
            }
        
            __YAFFS_OPUNLOCK();
            if (iError < ERROR_NONE) {
                return  (iError);
            }
            
            _DebugFormat(__LOGMESSAGE_LEVEL, "yaffs volume \"%s\" unmount ok.\r\n", pcName);
            
            return  (ERROR_NONE);
        
        } else {
            struct yaffs_stat   yaffsstat;
            
            iError = yaffs_lstat(pcName, &yaffsstat);
            if (iError >= 0) {
                if (S_ISDIR(yaffsstat.st_mode)) {                       /*  目录文件                    */
                    iError = yaffs_rmdir(pcName);
                    
                } else {
                    iError = yaffs_unlink(pcName);                      /*  删除文件                    */
                }
                
            } else {                                                    /*  如果是链接文件, tail一定存在*/
                PCHAR   pcTail, pcSymfile, pcPrefix;
                struct yaffs_obj *obj = yaffsfs_FindSymlink(pcName, &pcTail, &pcSymfile);
                                                                        /*  检查目录是否包含 symlink    */
                if (obj) {                                              /*  目录中有 symlink            */
                    pcSymfile--;                                        /*  从 / 开始                   */
                    if (pcSymfile == pcName) {
                        pcPrefix = LW_NULL;                             /*  没有前缀                    */
                    } else {
                        pcPrefix = pcName;
                        *pcSymfile = PX_EOS;
                    }
                    if (yaffsfs_PathBuildLink(pcName, pyaffs->YAFFS_devhdrHdr.DEVHDR_pcName,
                                              pcPrefix, obj, pcTail) == ERROR_NONE) {
                                                                        /*  构造链接目标                */
                        __YAFFS_OPUNLOCK();
                        return  (FOLLOW_LINK_TAIL);                     /*  一定不是连接文件本身        */
                    }
                }
            }
            __YAFFS_OPUNLOCK();
            
            return  (iError);
        }
    }
}
/*********************************************************************************************************
** 函数名称: __yaffsClose
** 功能描述: yaffs close 操作
** 输　入  : pfdentry         文件控制块
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __yaffsClose (PLW_FD_ENTRY    pfdentry)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    PYAFFS_FSLIB  pyaffs   = pyaffile->YAFFIL_pyaffs;
    BOOL          bFree    = LW_FALSE;
    BOOL          bRemove  = LW_FALSE;

    if (pyaffile) {
        __YAFFS_OPLOCK();
        if (API_IosFdNodeDec(&pyaffs->YAFFS_plineFdNodeHeader,
                             pfdnode, &bRemove) == 0) {                 /*  fd_node 是否完全释放        */
            __yaffsCloseFile(pyaffile);
            bFree = LW_TRUE;
        }
        
        LW_DEV_DEC_USE_COUNT(&pyaffs->YAFFS_devhdrHdr);
        
        if (bRemove) {
            if (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) {
                yaffs_unlink(pyaffile->YAFFIL_cName);
            
            } else if (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_DIR) {
                yaffs_rmdir(pyaffile->YAFFIL_cName);
            }
        }
        
        __YAFFS_OPUNLOCK();
        
        if (bFree) {
            __SHEAP_FREE(pyaffile);
        }
        
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __yaffsRead
** 功能描述: yaffs read 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __yaffsRead (PLW_FD_ENTRY   pfdentry,
                             PCHAR          pcBuffer, 
                             size_t         stMaxBytes)
{
    PLW_FD_NODE   pfdnode    = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile   = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    ssize_t       sstReadNum = PX_ERROR;
             
    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType != __YAFFS_FILE_TYPE_NODE) {
        __YAFFS_OPUNLOCK();
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    sstReadNum = (ssize_t)yaffs_pread(pyaffile->YAFFIL_iFd,
                                      (PVOID)pcBuffer, (unsigned int)stMaxBytes, 
                                      pfdentry->FDENTRY_oftPtr);
    if (sstReadNum > 0) {
        pfdentry->FDENTRY_oftPtr += (off_t)sstReadNum;                  /*  更新文件指针                */
    }
    __YAFFS_OPUNLOCK();
    
    return  (sstReadNum);
}
/*********************************************************************************************************
** 函数名称: __yaffsPRead
** 功能描述: yaffs pread 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
**           oftPos           位置
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __yaffsPRead (PLW_FD_ENTRY   pfdentry,
                              PCHAR          pcBuffer, 
                              size_t         stMaxBytes,
                              off_t          oftPos)
{
    PLW_FD_NODE   pfdnode    = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile   = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    ssize_t       sstReadNum = PX_ERROR;
             
    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType != __YAFFS_FILE_TYPE_NODE) {
        __YAFFS_OPUNLOCK();
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    sstReadNum = (ssize_t)yaffs_pread(pyaffile->YAFFIL_iFd,
                                      (PVOID)pcBuffer, (unsigned int)stMaxBytes, 
                                      oftPos);
    __YAFFS_OPUNLOCK();
    
    return  (sstReadNum);
}
/*********************************************************************************************************
** 函数名称: __yaffsWrite
** 功能描述: yaffs write 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         缓冲区
**           stNBytes         需要写入的数据
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __yaffsWrite (PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer, 
                              size_t        stNBytes)
{
    PLW_FD_NODE   pfdnode     = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile    = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    ssize_t       sstWriteNum = PX_ERROR;
    
    
    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType != __YAFFS_FILE_TYPE_NODE) {
        __YAFFS_OPUNLOCK();
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (pfdentry->FDENTRY_iFlag & O_APPEND) {                           /*  追加模式                    */
        pfdentry->FDENTRY_oftPtr = pfdnode->FDNODE_oftSize;             /*  移动读写指针到末尾          */
    }
    
    sstWriteNum = (ssize_t)yaffs_pwrite(pyaffile->YAFFIL_iFd,
                                        (CPVOID)pcBuffer, (unsigned int)stNBytes,
                                        pfdentry->FDENTRY_oftPtr);
    if (sstWriteNum > 0) {
        struct yaffs_stat   yafstat;
        pfdentry->FDENTRY_oftPtr += (off_t)sstWriteNum;                 /*  更新文件指针                */
        yaffs_fstat(pyaffile->YAFFIL_iFd, &yafstat);
        pfdnode->FDNODE_oftSize = yafstat.st_size;                      /*  更新文件大小                */
    }
    __YAFFS_OPUNLOCK();
    
    if (sstWriteNum >= 0) {
        if (pfdentry->FDENTRY_iFlag & O_SYNC) {                         /*  需要立即同步                */
            __yaffsFlush(pfdentry);
        
        } else if (pfdentry->FDENTRY_iFlag & O_DSYNC) {
            __yaffsDataSync(pfdentry);
        }
    }
    
    return  (sstWriteNum);
}
/*********************************************************************************************************
** 函数名称: __yaffsPWrite
** 功能描述: yaffs pwrite 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         缓冲区
**           stNBytes         需要写入的数据
**           oftPos           位置
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __yaffsPWrite (PLW_FD_ENTRY  pfdentry,
                               PCHAR         pcBuffer, 
                               size_t        stNBytes,
                               off_t         oftPos)
{
    PLW_FD_NODE   pfdnode     = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile    = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    ssize_t       sstWriteNum = PX_ERROR;
    
    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType != __YAFFS_FILE_TYPE_NODE) {
        __YAFFS_OPUNLOCK();
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    sstWriteNum = (ssize_t)yaffs_pwrite(pyaffile->YAFFIL_iFd,
                                        (CPVOID)pcBuffer, (unsigned int)stNBytes,
                                        oftPos);
    if (sstWriteNum > 0) {
        struct yaffs_stat   yafstat;
        yaffs_fstat(pyaffile->YAFFIL_iFd, &yafstat);
        pfdnode->FDNODE_oftSize = yafstat.st_size;                      /*  更新文件大小                */
    }
    __YAFFS_OPUNLOCK();
    
    if (sstWriteNum >= 0) {
        if (pfdentry->FDENTRY_iFlag & O_SYNC) {                         /*  需要立即同步                */
            __yaffsFlush(pfdentry);
        
        } else if (pfdentry->FDENTRY_iFlag & O_DSYNC) {
            __yaffsDataSync(pfdentry);
        }
    }
    
    return  (sstWriteNum);
}
/*********************************************************************************************************
** 函数名称: __yaffsSeek
** 功能描述: yaffs seek 操作
** 输　入  : pfdentry         文件控制块
**           oftOffset        偏移量
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __yaffsSeek (PLW_FD_ENTRY  pfdentry,
                         off_t         oftOffset)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    INT           iError   = ERROR_NONE;
    ULONG         ulError  = ERROR_NONE;

    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) {
        off_t   oftRet = yaffs_lseek(pyaffile->YAFFIL_iFd, oftOffset, SEEK_SET);
        if (oftRet != oftOffset) {
            iError = PX_ERROR;
        } else {
            pfdentry->FDENTRY_oftPtr = oftOffset;
        }
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    __YAFFS_OPUNLOCK();
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __yaffsWhere
** 功能描述: yaffs 获得文件当前读写指针位置 (使用参数作为返回值, 与 FIOWHERE 的要求稍有不同)
** 输　入  : pfdentry            文件控制块
**           poftPos             读写指针位置
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __yaffsWhere (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    __YAFFS_OPLOCK();
    if (poftPos) {
        *poftPos = (off_t)pfdentry->FDENTRY_oftPtr;
    }
    __YAFFS_OPUNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __yaffsNRead
** 功能描述: yaffs 获得文件剩余字节数
** 输　入  : pfdentry            文件控制块
**           piPos               保存剩余数据量
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __yaffsNRead (PLW_FD_ENTRY  pfdentry, INT  *piPos)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    INT           iError   = ERROR_NONE;
    ULONG         ulError  = ERROR_NONE;
    
    if (piPos == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) {
        *piPos = (INT)(pfdnode->FDNODE_oftSize - pfdentry->FDENTRY_oftPtr);
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    __YAFFS_OPUNLOCK();
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __yaffsNRead64
** 功能描述: yaffs 获得文件剩余字节数
** 输　入  : pfdentry            文件控制块
**           poftPos             保存剩余数据量
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __yaffsNRead64 (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    INT           iError   = ERROR_NONE;
    ULONG         ulError  = ERROR_NONE;
    
    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) {
        *poftPos = (pfdnode->FDNODE_oftSize - pfdentry->FDENTRY_oftPtr);
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    __YAFFS_OPUNLOCK();
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __yaffsFlush
** 功能描述: yaffs flush 操作
** 输　入  : pfdentry            文件控制块
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __yaffsFlush (PLW_FD_ENTRY  pfdentry)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    INT           iRet;

    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) {
        iRet = yaffs_fsync(pyaffile->YAFFIL_iFd);
    
    } else {
        yaffs_sync(pyaffile->YAFFIL_cName);
        iRet = ERROR_NONE;
    }
    __YAFFS_OPUNLOCK();
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __yaffsDataSync
** 功能描述: yaffs data sync 操作
** 输　入  : pfdentry            文件控制块
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __yaffsDataSync (PLW_FD_ENTRY  pfdentry)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    INT           iRet;

    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) {
        iRet = yaffs_fdatasync(pyaffile->YAFFIL_iFd);
    
    } else {
        yaffs_sync(pyaffile->YAFFIL_cName);
        iRet = ERROR_NONE;
    }
    __YAFFS_OPUNLOCK();
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __yaffsFormat
** 功能描述: yaffs mkfs 操作
** 输　入  : pfdentry            文件控制块
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __yaffsFormat (PLW_FD_ENTRY  pfdentry)
{
    INT                  iError;
    PLW_FD_NODE          pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE          pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    struct yaffs_dev    *pyaffsDev;
    
    __YAFFS_OPLOCK();
    pyaffsDev = (struct yaffs_dev *)yaffs_getdev(pyaffile->YAFFIL_cName);
    if (pyaffsDev == LW_NULL) {                                         /*  查找 nand 设备              */
        __YAFFS_OPUNLOCK();
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);                       /*  无法搜索到设备              */
        return  (PX_ERROR);
    }
    
    __yaffsCloseFile(pyaffile);                                         /*  提前关闭文件                */
    
    pyaffile->YAFFIL_iFileType = __YAFFS_FILE_TYPE_DEV;                 /*  文件已经提前关闭, 变为设备  */
    
    yaffs_sync(pyaffile->YAFFIL_cName);                                 /*  同步 yaffs 卷               */
    
    iError = yaffs_format(pyaffile->YAFFIL_cName, 1, 0, 1);             /*  format 后重新挂载           */
    __YAFFS_OPUNLOCK();

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __yaffsRename
** 功能描述: yaffs rename 操作
** 输　入  : pfdentry         文件控制块
**           pcNewName        新文件名
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __yaffsRename (PLW_FD_ENTRY  pfdentry, PCHAR  pcNewName)
{
    REGISTER INT            iError  = PX_ERROR;
    
             CHAR           cNewPath[PATH_MAX + 1];
    REGISTER PCHAR          pcNewPath = &cNewPath[0];
             PLW_FD_NODE    pfdnode   = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
             PYAFFS_FILE    pyaffile  = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
             PYAFFS_FSLIB   pyaffsNew;

    if (__STR_IS_ROOT(pyaffile->YAFFIL_cName)) {                        /*  检查是否为设备文件          */
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                         /*  不支持设备重命名            */
        return  (PX_ERROR);
    }
    if (pcNewName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    if (__STR_IS_ROOT(pcNewName)) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    __YAFFS_OPLOCK();
    if ((pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) ||
        (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_DIR)) {        /*  open 创建的普通文件或目录   */
        if (ioFullFileNameGet(pcNewName, 
                              (LW_DEV_HDR **)&pyaffsNew, 
                              cNewPath) != ERROR_NONE) {                /*  获得新目录路径              */
            __YAFFS_OPUNLOCK();
            return  (PX_ERROR);
        }
        if (pyaffsNew != pyaffile->YAFFIL_pyaffs) {                     /*  必须为同一个 yaffs 设备节点 */
            __YAFFS_OPUNLOCK();
            _ErrorHandle(EXDEV);
            return  (PX_ERROR);
        }
        
        if (cNewPath[0] == PX_DIVIDER) {
            pcNewPath++;
        }
        
        iError = yaffs_rename(pyaffile->YAFFIL_cName, pcNewPath);       /*  重命名                      */
    } else {
        _ErrorHandle(ENOSYS);
    }
    __YAFFS_OPUNLOCK();
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __yaffsStatGet
** 功能描述: yaffs stat 操作
** 输　入  : pfdentry         文件控制块
**           pstat            文件状态
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __yaffsStatGet (PLW_FD_ENTRY  pfdentry, struct stat *pstat)
{
    INT                 iError;
    PLW_FD_NODE         pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE         pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    struct yaffs_stat   yaffsstat;

    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __YAFFS_OPLOCK();
    iError = yaffs_stat(pyaffile->YAFFIL_cName, &yaffsstat);
    if (iError == ERROR_NONE) {
        pstat->st_dev     = (dev_t)yaffsstat.st_dev;                    /* device                       */
        pstat->st_ino     = yaffsstat.st_ino;                           /* inode                        */
        pstat->st_mode    = yaffsstat.st_mode;                          /* protection                   */
        pstat->st_nlink   = yaffsstat.st_nlink;                         /* number of hard links         */
        pstat->st_uid     = (uid_t)yaffsstat.st_uid;                    /* user ID of owner             */
        pstat->st_gid     = (gid_t)yaffsstat.st_gid;                    /* group ID of owner            */
        pstat->st_rdev    = (dev_t)yaffsstat.st_rdev;                   /* device type (if inode device)*/
        pstat->st_size    = yaffsstat.st_size;                          /* total size, in bytes         */
        pstat->st_blksize = yaffsstat.st_blksize;                       /* blocksize for filesystem I/O */
        pstat->st_blocks  = yaffsstat.st_blocks;                        /* number of blocks allocated   */
        
        pstat->st_atime = yaffsstat.yst_atime;                          /* time of last access          */
        pstat->st_mtime = yaffsstat.yst_mtime;                          /* time of last modification    */
        pstat->st_ctime = yaffsstat.yst_ctime;                          /* time of last create          */
        
        /*
         *  如果是 yaffs 根设备, 那么一定是 DIR, 如果卷出现损坏, st_mode 可能没有 dir 标志.
         *  所以如果是 yaffs 根设备, 这里一定要强制为 dir 类型, 以便 readdir() 顺利显示卷标以供
         *  格式化时使用.
         */
        if (__STR_IS_ROOT(pyaffile->YAFFIL_cName)) {                    /* yaffs root                   */
            pstat->st_mode |= S_IFDIR;
        }
    
    } else {
        if (__STR_IS_ROOT(pyaffile->YAFFIL_cName)) {                    /* yaffs root                   */
            pstat->st_dev     = (dev_t)pyaffile->YAFFIL_pyaffs;
            pstat->st_ino     = (ino_t)0;                               /* 绝不与文件重复(根目录)       */
            pstat->st_mode    = YAFFS_ROOT_MODE | S_IFDIR;              /* root dir                     */
            pstat->st_nlink   = 1;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 1;
            pstat->st_size    = 0;
            pstat->st_blksize = 0;
            pstat->st_blocks  = 0;
            
            pstat->st_atime = API_RootFsTime(LW_NULL);                  /*  use root fs time            */
            pstat->st_mtime = API_RootFsTime(LW_NULL);
            pstat->st_ctime = API_RootFsTime(LW_NULL);
            
            iError = ERROR_NONE;                                        /*  ok                          */
        }
    }
    __YAFFS_OPUNLOCK();
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __yaffsLStatGet
** 功能描述: yaffs stat 操作
** 输　入  : pyaffs           设备
**           pcName           文件名
**           pstat            文件状态
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __yaffsLStatGet (PYAFFS_FSLIB  pyaffs, PCHAR  pcName, struct stat *pstat)
{
    INT                 iError;
    struct yaffs_stat   yaffsstat;

    if (!pcName || !pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __YAFFS_OPLOCK();
    iError = yaffs_lstat(pcName, &yaffsstat);
    if (iError == ERROR_NONE) {
        pstat->st_dev     = (dev_t)yaffsstat.st_dev;                    /* device                       */
        pstat->st_ino     = yaffsstat.st_ino;                           /* inode                        */
        pstat->st_mode    = yaffsstat.st_mode;                          /* protection                   */
        pstat->st_nlink   = yaffsstat.st_nlink;                         /* number of hard links         */
        pstat->st_uid     = (uid_t)yaffsstat.st_uid;                    /* user ID of owner             */
        pstat->st_gid     = (gid_t)yaffsstat.st_gid;                    /* group ID of owner            */
        pstat->st_rdev    = (dev_t)yaffsstat.st_rdev;                   /* device type (if inode device)*/
        pstat->st_size    = yaffsstat.st_size;                          /* total size, in bytes         */
        pstat->st_blksize = yaffsstat.st_blksize;                       /* blocksize for filesystem I/O */
        pstat->st_blocks  = yaffsstat.st_blocks;                        /* number of blocks allocated   */
        
        pstat->st_atime = yaffsstat.yst_atime;                          /* time of last access          */
        pstat->st_mtime = yaffsstat.yst_mtime;                          /* time of last modification    */
        pstat->st_ctime = yaffsstat.yst_ctime;                          /* time of last create          */
        
        /*
         *  如果是 yaffs 根设备, 那么一定是 DIR, 如果卷出现损坏, st_mode 可能没有 dir 标志.
         *  所以如果是 yaffs 根设备, 这里一定要强制为 dir 类型, 以便 readdir() 顺利显示卷标以供
         *  格式化时使用.
         */
        if (__STR_IS_ROOT(pcName)) {                                    /* yaffs root                   */
            pstat->st_mode |= S_IFDIR;
        }
    
    } else {
        if (__STR_IS_ROOT(pcName)) {                                    /* yaffs root                   */
            pstat->st_dev     = (dev_t)pyaffs;
            pstat->st_ino     = (ino_t)0;                               /* 绝不与文件重复(根目录)       */
            pstat->st_mode    = YAFFS_ROOT_MODE | S_IFDIR;              /* root dir                     */
            pstat->st_nlink   = 1;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 1;
            pstat->st_size    = 0;
            pstat->st_blksize = 0;
            pstat->st_blocks  = 0;
            
            pstat->st_atime = API_RootFsTime(LW_NULL);                  /*  use root fs time            */
            pstat->st_mtime = API_RootFsTime(LW_NULL);
            pstat->st_ctime = API_RootFsTime(LW_NULL);
            
            iError = ERROR_NONE;                                        /*  ok                          */
        }
    }
    __YAFFS_OPUNLOCK();
    
    /*
     *  如果 iError < 0 系统会自动使用 stat 获取
     */
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __yaffsStatfsGet
** 功能描述: yaffs statfs 操作
** 输　入  : pfdentry         文件控制块
**           pstatfs          文件系统状态
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __yaffsStatfsGet (PLW_FD_ENTRY  pfdentry, struct statfs *pstatfs)
{
    INT                 iError;
    struct yaffs_stat   yaffsstat;
    PLW_FD_NODE         pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE         pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    
    loff_t              oftFreeBytes;
    loff_t              oftTotalBytes;
    
    if (!pstatfs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __YAFFS_OPLOCK();
    iError = yaffs_lstat(pyaffile->YAFFIL_cName, &yaffsstat);
    if ((iError == ERROR_NONE) && (yaffsstat.st_blksize > 0)) {
        struct yaffs_dev  *pyaffsdev;
        
        oftFreeBytes  = yaffs_freespace(pyaffile->YAFFIL_cName);
        oftTotalBytes = yaffs_totalspace(pyaffile->YAFFIL_cName);
        
        pstatfs->f_type  = YAFFS_MAGIC;                                 /* type of info, YAFFS_MAGIC   */
        pstatfs->f_bsize = yaffsstat.st_blksize;                        /* fundamental file system block*/
                                                                        /* size                         */
        pstatfs->f_blocks = (long)(oftTotalBytes / yaffsstat.st_blksize);
                                                                        /* total blocks in file system  */
        pstatfs->f_bfree  = (long)(oftFreeBytes / yaffsstat.st_blksize);/* free block in fs             */
        pstatfs->f_bavail = (long)(oftFreeBytes / yaffsstat.st_blksize);/* free blocks avail to         */
                                                                        /* non-superuser                */
        pstatfs->f_files = 0;                                           /* total file nodes in file     */
                                                                        /* system                       */
        pstatfs->f_ffree = 0;                                           /* free file nodes in fs        */
        
#if LW_CFG_CPU_WORD_LENGHT == 64
        pstatfs->f_fsid.val[0] = (int32_t)((addr_t)pyaffile->YAFFIL_pyaffs >> 32);
        pstatfs->f_fsid.val[1] = (int32_t)((addr_t)pyaffile->YAFFIL_pyaffs & 0xffffffff);
#else
        pstatfs->f_fsid.val[0] = (int32_t)pyaffile->YAFFIL_pyaffs;      /* file system sid              */
        pstatfs->f_fsid.val[1] = 0;
#endif
        
        pyaffsdev = (struct yaffs_dev *)yaffs_getdev(pyaffile->YAFFIL_cName);
        if (pyaffsdev && pyaffsdev->read_only) {
            pstatfs->f_flag = ST_RDONLY;
        } else {
            pstatfs->f_flag = 0;
        }
        
        pstatfs->f_namelen = YAFFS_MAX_NAME_LENGTH;                     /* max name len                 */
    
    } else {
        iError = PX_ERROR;
    }
    __YAFFS_OPUNLOCK();
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __yaffsChmod
** 功能描述: yaffs chmod 操作
** 输　入  : pfdentry            文件控制块
**           iMode               新的 mode
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __yaffsChmod (PLW_FD_ENTRY  pfdentry, INT  iMode)
{
    INT             iError   = PX_ERROR;
    PLW_FD_NODE     pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE     pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    
    iMode |= S_IRUSR;
    
    __YAFFS_OPLOCK();
    if ((pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) ||
        (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_DIR)) {        /*  open 创建的普通文件或目录   */
        iError = yaffs_chmod(pyaffile->YAFFIL_cName, (mode_t)iMode);
    }
    __YAFFS_OPUNLOCK();
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __yaffsReadDir
** 功能描述: yaffs 获得指定目录信息
** 输　入  : pfdentry            文件控制块
**           dir                 目录结构
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __yaffsReadDir (PLW_FD_ENTRY  pfdentry, DIR  *dir)
{
    REGISTER INT             iError  = PX_ERROR;
    struct yaffs_dirent     *yafdirent;
    PLW_FD_NODE              pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE              pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    
    if (__STR_IS_ROOT(pyaffile->YAFFIL_cName)) {
        /*
         *  浏览 yaffs 根目录, 与 fat 不同, 所有的 yaffs 卷都是挂载到 yaffs 根设备上
         *  这里需要列出当前所有 yaffs 卷.
         */
        INT     iIndex = (INT)dir->dir_pos;
        INT     iNext;
        PCHAR   pcVolName = yaffs_getdevname(iIndex, &iNext);
        
        if (pcVolName) {                                                /*  是否查询到设备              */
            dir->dir_pos = (LONG)iNext;
            lib_strcpy(dir->dir_dirent.d_name, &pcVolName[1]);          /*  忽略 '/' 字符拷贝           */
            dir->dir_dirent.d_shortname[0] = PX_EOS;                    /*  不支持短文件名              */
            dir->dir_dirent.d_type = DT_DIR;
            iError = ERROR_NONE;
        }
        
        return  (iError);
    }
    
    if (pyaffile->YAFFIL_iFileType != __YAFFS_FILE_TYPE_DIR) {
        _ErrorHandle(ENOTDIR);                                          /*  不支持                      */
        return  (PX_ERROR);
    }
    
    __YAFFS_OPLOCK();
    if (dir->dir_pos == 0) {
        yaffs_rewinddir_fd(pyaffile->YAFFIL_iFd);
    }
    
    yafdirent = yaffs_readdir_fd(pyaffile->YAFFIL_iFd);
    if (yafdirent) {
        lib_strcpy(dir->dir_dirent.d_name, yafdirent->d_name);
        dir->dir_dirent.d_shortname[0] = PX_EOS;                        /*  不支持短文件名              */
        dir->dir_dirent.d_type = yafdirent->d_type;                     /*  类型                        */
        dir->dir_pos++;
        iError = ERROR_NONE;
    }
    __YAFFS_OPUNLOCK();
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __yaffsTimeset
** 功能描述: yaffs 设置文件时间
** 输　入  : pfdentry            文件控制块
**           utim                utimbuf 结构
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
** 注  意  : 目前此函数不进行任何处理.
*********************************************************************************************************/
static INT  __yaffsTimeset (PLW_FD_ENTRY  pfdentry, struct utimbuf  *utim)
{
    struct yaffs_utimbuf    yafutim;
    REGISTER INT            iError;
             PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
             PYAFFS_FILE    pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    
    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_DEV) {
        iError = PX_ERROR;
        _ErrorHandle(ENOSYS);                                           /*  不支持相关操作              */
    } else {
        yafutim.actime  = (unsigned long)utim->actime;
        yafutim.modtime = (unsigned long)utim->modtime;
        iError = yaffs_futime(pyaffile->YAFFIL_iFd, &yafutim);
    }
    __YAFFS_OPUNLOCK();

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __yaffsFsTruncate
** 功能描述: yaffs 设置文件大小
** 输　入  : pfdentry            文件控制块
**           oftSize             文件大小
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __yaffsTruncate (PLW_FD_ENTRY  pfdentry, off_t  oftSize)
{
    REGISTER INT            iError   = PX_ERROR;
             PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
             PYAFFS_FILE    pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    
    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) {
        iError = yaffs_ftruncate(pyaffile->YAFFIL_iFd, oftSize);
        if (iError == ERROR_NONE) {
            struct yaffs_stat   yafstat;
            yaffs_fstat(pyaffile->YAFFIL_iFd, &yafstat);
            pfdnode->FDNODE_oftSize = yafstat.st_size;                  /*  更新文件大小                */
        }
    }
    __YAFFS_OPUNLOCK();
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __yaffsFsSymlink
** 功能描述: yaffs 创建符号链接文件
** 输　入  : pyaffs              设备
**           pcName              创建的连接文件
**           pcLinkDst           链接目标
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT __yaffsFsSymlink (PYAFFS_FSLIB  pyaffs, PCHAR  pcName, CPCHAR  pcLinkDst)
{
    REGISTER INT   iError;

    __YAFFS_OPLOCK();
    iError = yaffs_symlink(pcLinkDst, pcName);
    __YAFFS_OPUNLOCK();
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __yaffsFsReadlink
** 功能描述: yaffs 读取符号链接文件内容
** 输　入  : pyaffs              设备
**           pcName              链接原始文件名
**           pcLinkDst           链接目标文件名
**           stMaxSize           缓冲大小
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t __yaffsFsReadlink (PYAFFS_FSLIB  pyaffs, 
                                  PCHAR         pcName,
                                  PCHAR         pcLinkDst,
                                  size_t        stMaxSize)
{
    REGISTER INT   iError;

    __YAFFS_OPLOCK();
    iError = yaffs_readlink(pcName, pcLinkDst, stMaxSize);
    __YAFFS_OPUNLOCK();
    
    if (iError == 0) {
        return  ((ssize_t)lib_strnlen(pcLinkDst, stMaxSize));
    } else {
        return  ((ssize_t)iError);
    }
}
/*********************************************************************************************************
** 函数名称: __yaffsIoctl
** 功能描述: yaffs ioctl 操作
** 输　入  : pfdentry           文件控制块
**           request,           命令
**           arg                命令参数
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __yaffsIoctl (PLW_FD_ENTRY  pfdentry,
                          INT           iRequest,
                          LONG          lArg)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE    pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    off_t          oftTemp;
    INT            iError;

    switch (iRequest) {
    
    case FIOCONTIG:
    case FIOTRUNC:
    case FIOLABELSET:
    case FIOATTRIBSET:
    case FIOSQUEEZE:
        if ((pfdentry->FDENTRY_iFlag & O_ACCMODE) == O_RDONLY) {
            _ErrorHandle(ERROR_IO_WRITE_PROTECTED);
            return  (PX_ERROR);
        }
	}
    
    switch (iRequest) {
    
    case FIODISKFORMAT:                                                 /*  磁盘格式化                  */
        return  (__yaffsFormat(pfdentry));
    
    case FIODISKINIT:                                                   /*  磁盘初始化                  */
        return  (ERROR_NONE);
    
    case FIOSEEK:                                                       /*  文件重定位                  */
        oftTemp = *(off_t *)lArg;
        return  (__yaffsSeek(pfdentry, oftTemp));

    case FIOWHERE:                                                      /*  获得文件当前读写指针        */
        iError = __yaffsWhere(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }
        
    case FIONREAD:                                                      /*  获得文件剩余字节数          */
        return  (__yaffsNRead(pfdentry, (INT *)lArg));
        
    case FIONREAD64:                                                    /*  获得文件剩余字节数          */
        iError = __yaffsNRead64(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }

    case FIORENAME:                                                     /*  文件重命名                  */
        return  (__yaffsRename(pfdentry, (PCHAR)lArg));
    
    case FIOLABELGET:                                                   /*  获取卷标                    */
    case FIOLABELSET:                                                   /*  设置卷标                    */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    
    case FIOFSTATGET:                                                   /*  获得文件状态                */
        return  (__yaffsStatGet(pfdentry, (struct stat *)lArg));
    
    case FIOFSTATFSGET:                                                 /*  获得文件系统状态            */
        return  (__yaffsStatfsGet(pfdentry, (struct statfs *)lArg));
    
    case FIOREADDIR:                                                    /*  获取一个目录信息            */
        return  (__yaffsReadDir(pfdentry, (DIR *)lArg));
    
    case FIOTIMESET:                                                    /*  设置文件时间                */
        return  (__yaffsTimeset(pfdentry, (struct utimbuf *)lArg));
        
    /*
     *  FIOTRUNC is 64 bit operate.
     */
    case FIOTRUNC:                                                      /*  改变文件大小                */
        oftTemp = *(off_t *)lArg;
        return  (__yaffsTruncate(pfdentry, oftTemp));
    
    case FIOSYNC:                                                       /*  将文件缓存回写              */
    case FIOFLUSH:
        return  (__yaffsFlush(pfdentry));
    
    case FIODATASYNC:                                                   /*  回写数据部分                */
        return  (__yaffsDataSync(pfdentry));
    
    case FIOCHMOD:
        return  (__yaffsChmod(pfdentry, (INT)lArg));                    /*  改变文件访问权限            */
    
    case FIOSETFL:                                                      /*  设置新的 flag               */
        if ((INT)lArg & O_NONBLOCK) {
            pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);
    
    case FIOFSTYPE:                                                     /*  获得文件系统类型            */
        *(PCHAR *)lArg = "YAFFS FileSystem";
        return  (ERROR_NONE);
    
    case FIOGETFORCEDEL:                                                /*  强制卸载设备是否被允许      */
        *(BOOL *)lArg = pyaffile->YAFFIL_pyaffs->YAFFS_bForceDelete;
        return  (ERROR_NONE);
        
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_YAFFS_EN > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
