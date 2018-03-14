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
** 文   件   名: shm.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 12 月 27 日
**
** 描        述: 共享内存设备. 
                 此设备下的文件只能通过唯一一次 truncat 操作改变文件大小, 然后通过 mmap 访问, 不可使用
                 read / write 进行访问.
** BUG:
2013.03.13  第一次 mmap 时分配物理内存, 最后一次 munmap 时释放物理内存.
2013.03.16  不再使用 DMA 内存, 转而使用通用物理内存.
2013.03.17  __shmMmap() 直接使用 mmap 确定的映射 flag.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"                                /*  需要根文件系统时间          */
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0 && LW_CFG_SHM_DEVICE_EN > 0 && LW_CFG_VMM_EN > 0 
/*********************************************************************************************************
  文件节点
*********************************************************************************************************/
typedef struct lw_shm_node {
    LW_LIST_LINE                 SHMN_lineBrother;                      /*  兄弟节点                    */
    struct lw_shm_node          *SHMN_pshmnFather;                      /*  父系节点                    */
    PLW_LIST_LINE                SHMN_plineSon;                         /*  儿子节点                    */
    PCHAR                        SHMN_pcName;                           /*  节点名                      */
    INT                          SHMN_iOpenNum;                         /*  打开的次数                  */
    off_t                        SHMN_oftSize;                          /*  文件大小                    */
    PVOID                        SHMN_pvPhyMem;                         /*  物理内存, 链接时用作连接名  */
#define SHMN_pvLink              SHMN_pvPhyMem
    ULONG                        SHMN_ulMapCnt;                         /*  映射计数器                  */
    mode_t                       SHMN_mode;                             /*  节点类型                    */
    time_t                       SHMN_time;                             /*  节点时间, 一般为当前时间    */
    uid_t                        SHMN_uid;
    gid_t                        SHMN_gid;
} LW_SHM_NODE;
typedef LW_SHM_NODE             *PLW_SHM_NODE;
/*********************************************************************************************************
  shm根
*********************************************************************************************************/
typedef struct lw_shm_root {
    PLW_LIST_LINE                SHMR_plineSon;                         /*  指向第一个儿子              */
    size_t                       SHMR_stMemUsed;                        /*  内存消耗量                  */
    time_t                       SHMR_time;                             /*  创建时间                    */
} LW_SHM_ROOT;
typedef LW_SHM_ROOT             *PLW_SHM_ROOT;
/*********************************************************************************************************
  设备结构
*********************************************************************************************************/
static LW_DEV_HDR                _G_devhdrShm;                          /*  共享内存设备                */
/*********************************************************************************************************
  定义全局变量
*********************************************************************************************************/
static LW_SHM_ROOT               _G_shmrRoot;
static LW_OBJECT_HANDLE          _G_ulShmLock;                          /*  procFs 操作锁               */
static INT                       _G_iShmDrvNum = PX_ERROR;
/*********************************************************************************************************
  定义全局变量
*********************************************************************************************************/
#define __LW_SHM_LOCK()          API_SemaphoreMPend(_G_ulShmLock, LW_OPTION_WAIT_INFINITE)
#define __LW_SHM_UNLOCK()        API_SemaphoreMPost(_G_ulShmLock)
/*********************************************************************************************************
  检测路径字串是否为根目录或者直接指向设备
*********************************************************************************************************/
#define __STR_IS_ROOT(pcName)    ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))
/*********************************************************************************************************
  节点物理内存管理
*********************************************************************************************************/
static INT  __shmPhymemAlloc(PLW_SHM_NODE  pshmn);
static INT  __shmPhymemFree(PLW_SHM_NODE  pshmn);
/*********************************************************************************************************
** 函数名称: __shmFindNode
** 功能描述: 共享内存设备查找一个节点
** 输　入  : pcName            节点名 (必须从 procfs 根开始)
**           ppshmnFather      当无法找到节点时保存最接近的一个,
                               但寻找到节点时保存父系节点. 
                               LW_NULL 表示根
**           pbRoot            节点名是否指向根节点.
**           pbLast            当匹配失败时, 是否是最后一级文件匹配失败
**           ppcTail           名字尾部, 当搜索到设备或者链接时, tail 指向名字尾.
** 输　出  : 节点, LW_NULL 表示根节点
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_SHM_NODE  __shmFindNode (CPCHAR            pcName, 
                                    PLW_SHM_NODE     *ppshmnFather, 
                                    BOOL             *pbRoot,
                                    BOOL             *pbLast,
                                    PCHAR            *ppcTail)
{
    static CHAR         pcTempName[MAX_FILENAME_LENGTH];
    PCHAR               pcNext;
    PCHAR               pcNode;
    
    PLW_SHM_NODE        pshmn;
    PLW_SHM_NODE        pshmnTemp;
    
    PLW_LIST_LINE       plineTemp;
    PLW_LIST_LINE       plineHeader;                                    /*  当前目录头                  */
    
    if (pcName == LW_NULL) {
__param_error:
        if (pbRoot) {
            *pbRoot = LW_FALSE;                                         /*  pcName 不为根               */
        }
        if (pbLast) {
            *pbLast = LW_FALSE;
        }
        return  (LW_NULL);
    }
    
    if (ppshmnFather == LW_NULL) {
        ppshmnFather = &pshmnTemp;                                      /*  临时变量                    */
    }
    *ppshmnFather = LW_NULL;
    
    if (*pcName == PX_ROOT) {                                           /*  忽略根符号                  */
        lib_strlcpy(pcTempName, (pcName + 1), PATH_MAX);
    } else {
        goto    __param_error;
    }
    
    /*
     *  判断文件名是否为根
     */
    if (pcTempName[0] == PX_EOS) {
        if (pbRoot) {
            *pbRoot = LW_TRUE;                                          /*  pcName 为根                 */
        }
        if (pbLast) {
            *pbLast = LW_FALSE;
        }
        return  (LW_NULL);
    } else {
        if (pbRoot) {
            *pbRoot = LW_FALSE;                                         /*  pcName 不为根               */
        }
    }
    
    pcNext      = pcTempName;
    plineHeader = _G_shmrRoot.SHMR_plineSon;                            /*  从根目录开始搜索            */
    
    do {
        pcNode = pcNext;
        pcNext = lib_index(pcNode, PX_DIVIDER);                         /*  移动到下级目录              */
        if (pcNext) {                                                   /*  是否可以进入下一层          */
            *pcNext = PX_EOS;
            pcNext++;                                                   /*  下一层的指针                */
        }
        
        for (plineTemp  = plineHeader;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            pshmn = _LIST_ENTRY(plineTemp, LW_SHM_NODE, SHMN_lineBrother);
            if (S_ISLNK(pshmn->SHMN_mode)) {                            /*  连接文件                    */
                if (lib_strcmp(pshmn->SHMN_pcName, pcNode) == 0) {
                    goto    __find_ok;                                  /*  找到连接                    */
                }
            
            } else if (S_ISSOCK(pshmn->SHMN_mode) || S_ISREG(pshmn->SHMN_mode)) {
                if (lib_strcmp(pshmn->SHMN_pcName, pcNode) == 0) {
                    if (pcNext) {                                       /*  还存在下级, 这里必须为目录  */
                        goto    __find_error;                           /*  不是目录直接错误            */
                    }
                    break;
                }
            
            } else {                                                    /*  目录节点                    */
                if (lib_strcmp(pshmn->SHMN_pcName, pcNode) == 0) {
                    break;
                }
            }
        }
        if (plineTemp == LW_NULL) {                                     /*  无法继续搜索                */
            goto    __find_error;
        }
        
        *ppshmnFather = pshmn;                                          /*  从当前节点开始搜索          */
        plineHeader   = pshmn->SHMN_plineSon;                           /*  从第一个儿子开始            */
        
    } while (pcNext);                                                   /*  不存在下级目录              */
    
__find_ok:
    *ppshmnFather = pshmn->SHMN_pshmnFather;                            /*  父系节点                    */
    /*
     *  计算 tail 的位置.
     */
    if (ppcTail) {
        if (pcNext) {
            INT   iTail = pcNext - pcTempName;
            *ppcTail = (PCHAR)pcName + iTail;                           /*  指向没有被处理的 / 字符     */
        } else {
            *ppcTail = (PCHAR)pcName + lib_strlen(pcName);              /*  指向最末尾                  */
        }
    }
    return  (pshmn);
    
__find_error:
    if (pbLast) {
        if (pcNext == LW_NULL) {                                        /*  最后一级查找失败            */
            *pbLast = LW_TRUE;
        } else {
            *pbLast = LW_FALSE;
        }
    }
    return  (LW_NULL);                                                  /*  无法找到节点                */
}
/*********************************************************************************************************
** 函数名称: __shmMakeNode
** 功能描述: 共享内存设备创建一个节点
** 输　入  : pcName        全名 (从根开始)
**           iFlags        方式
**           iMode         mode_t
**           pcLink        如果是连接文件, 保存连接目标
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __shmMakeNode (CPCHAR  pcName, INT  iFlags, INT  iMode, CPCHAR  pcLink)
{
    PLW_SHM_NODE     pshmn       = LW_NULL;
    PLW_SHM_NODE     pshmnFather = LW_NULL;
    BOOL             bIsRoot     = LW_FALSE;
    BOOL             bLast       = LW_FALSE;
    PCHAR            pcTail      = LW_NULL;
    
    size_t           stAllocSize;
    INT              iError = PX_ERROR;
    PLW_SHM_NODE     pshmnNew;
    CPCHAR           pcLastName;
    size_t           stLen;
    
    if ((pcName == LW_NULL) || (*pcName != PX_ROOT)) {                  /*  路径名                      */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (S_ISLNK(iMode) && pcLink == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if ((iMode & S_IFMT) == 0) {
        iMode |= S_IFREG;                                               /*  默认为 REG 文件             */
    }
    
    pcLastName = lib_rindex(pcName, PX_DIVIDER);
    if (pcLastName == LW_NULL) {
        pcLastName = pcName;
    } else {
        pcLastName++;                                                   /*  指向文件名的第一个节点      */
    }
    
    stLen = lib_strlen(pcLastName);
    if (stLen == 0) {
        _ErrorHandle(EINVAL);                                           /*  socket 文件名错误           */
        return  (PX_ERROR);
    }
    
    stLen++;                                                            /*  预留 \0 空间                */
    stAllocSize = (size_t)(sizeof(LW_SHM_NODE) + stLen);
    pshmnNew    = (PLW_SHM_NODE)__SHEAP_ALLOC(stAllocSize);
    if (!pshmnNew) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    pshmnNew->SHMN_pcName = (PCHAR)pshmnNew + sizeof(LW_SHM_NODE);
    lib_strcpy(pshmnNew->SHMN_pcName, pcLastName);
    
    if (S_ISLNK(iMode)) {
        pshmnNew->SHMN_pvLink = __SHEAP_ALLOC(lib_strlen(pcLink) + 1);
        if (pshmnNew->SHMN_pvLink == LW_NULL) {
            __SHEAP_FREE(pshmnNew);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        lib_strcpy((PCHAR)pshmnNew->SHMN_pvLink, pcLink);
    } else {
        pshmnNew->SHMN_pvPhyMem = LW_NULL;
    }
    
    pshmnNew->SHMN_plineSon = LW_NULL;                                  /*  没有任何儿子                */
    pshmnNew->SHMN_iOpenNum = 0;
    pshmnNew->SHMN_oftSize  = 0;
    pshmnNew->SHMN_ulMapCnt = 0;                                        /*  没有映射                    */
    pshmnNew->SHMN_mode     = iMode;
    pshmnNew->SHMN_time     = lib_time(LW_NULL);                        /*  以 UTC 时间作为时间基准     */
    pshmnNew->SHMN_uid      = getuid();
    pshmnNew->SHMN_gid      = getgid();
    
    __LW_SHM_LOCK();
    pshmn = __shmFindNode(pcName, &pshmnFather, 
                          &bIsRoot, &bLast, &pcTail);                   /*  查询设备                    */
    if (pshmn) {
        if ((pcTail && *pcTail == PX_ROOT) && 
            S_ISLNK(pshmn->SHMN_mode)) {
            _ErrorHandle(ERROR_IOS_FILE_SYMLINK);                       /*  父节点为 symlink 类型       */
        } else {
            _ErrorHandle(ERROR_IOS_DUPLICATE_DEVICE_NAME);              /*  设备重名                    */
        }
    } else {
        if (pshmnFather && !S_ISDIR(pshmnFather->SHMN_mode)) {          /*  父系节点不为目录            */
            _ErrorHandle(ENOENT);
        
        } else if (bLast == LW_FALSE) {                                 /*  缺少中间目录项              */
            _ErrorHandle(ENOENT);                                       /*  XXX errno ?                 */
        
        } else {
            if (pshmnFather) {
                pshmnNew->SHMN_pshmnFather = pshmnFather;
                _List_Line_Add_Ahead(&pshmnNew->SHMN_lineBrother,
                                     &pshmnFather->SHMN_plineSon);      /*  链入指定的链表              */
            } else {
                pshmnNew->SHMN_pshmnFather = LW_NULL;
                _List_Line_Add_Ahead(&pshmnNew->SHMN_lineBrother,
                                     &_G_shmrRoot.SHMR_plineSon);       /*  插入根节点                  */
            }
            iError = ERROR_NONE;
        }
    }
    __LW_SHM_UNLOCK();                                                  /*  解锁共享内存设备            */
    
    if (iError) {
        if (S_ISLNK(iMode)) {
            __SHEAP_FREE(pshmnNew->SHMN_pvLink);                        /*  释放链接文件内容            */
        }
        __SHEAP_FREE(pshmnNew);                                         /*  释放内存                    */
    }
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __shmRemoveNode
** 功能描述: 共享内存设备删除一个节点, 
** 输　入  : pcName        节点名
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __shmRemoveNode (CPCHAR  pcName)
{
    PLW_SHM_NODE    pshmnFather = LW_NULL;
    PLW_SHM_NODE    pshmn;
    BOOL            bIsRoot = LW_FALSE;
    INT             iError = PX_ERROR;
    
    if (pcName == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    /*
     *  设备特殊处理
     */
    if ((pcName[0] == PX_EOS) || 
        (lib_strcmp(pcName, PX_STR_ROOT) == 0)) {                       /*  不能移除此设备              */
        return  (ERROR_NONE);
    }
    
    __LW_SHM_LOCK();
    pshmn = __shmFindNode(pcName, &pshmnFather, 
                          &bIsRoot, LW_NULL, LW_NULL);                  /*  查询设备                    */
    if (pshmn) {
        if (pshmn->SHMN_plineSon) {
            _ErrorHandle(ENOTEMPTY);                                    /*  不为空                      */
        } else if (pshmn->SHMN_iOpenNum) {
            _ErrorHandle(EBUSY);                                        /*  节点没有关闭                */
        } else {
            if (pshmnFather == LW_NULL) {
                _List_Line_Del(&pshmn->SHMN_lineBrother,
                               &_G_shmrRoot.SHMR_plineSon);             /*  从根节点卸载                */
            } else {
                _List_Line_Del(&pshmn->SHMN_lineBrother,
                               &pshmnFather->SHMN_plineSon);            /*  从父节点卸载                */
            }
            iError = ERROR_NONE;
        }
    }
    __LW_SHM_UNLOCK();                                                  /*  解锁共享内存设备            */
    
    if (iError == ERROR_NONE) {
        if (S_ISLNK(pshmn->SHMN_mode)) {
            __SHEAP_FREE(pshmn->SHMN_pvLink);                           /*  释放链接文件内容            */
        } else if (pshmn->SHMN_pvPhyMem) {
            __shmPhymemFree(pshmn);                                     /*  释放共享的物理内存          */
        }
        __SHEAP_FREE(pshmn);                                            /*  释放内存                    */
    }
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __shmPhymemAlloc
** 功能描述: 共享内存设备节点创建物理内存, 
** 输　入  : pshmn               文件节点
**           oftSize             文件大小
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __shmPhymemAlloc (PLW_SHM_NODE  pshmn)
{
    REGISTER ULONG  ulPageNum = (ULONG) (pshmn->SHMN_oftSize >> LW_CFG_VMM_PAGE_SHIFT);
    REGISTER size_t stExcess  = (size_t)(pshmn->SHMN_oftSize & ~LW_CFG_VMM_PAGE_MASK);
    
             size_t stRealSize;
             
    if (stExcess) {
        ulPageNum++;
    }
    
    stRealSize = (size_t)(ulPageNum << LW_CFG_VMM_PAGE_SHIFT);
    if (stRealSize == 0) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pshmn->SHMN_pvPhyMem = API_VmmPhyAlloc(stRealSize);
    if (pshmn->SHMN_pvPhyMem == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    
    _G_shmrRoot.SHMR_stMemUsed += (size_t)pshmn->SHMN_oftSize;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __shmPhymemFree
** 功能描述: 共享内存设备节点释放物理内存, 
** 输　入  : pshmn               文件节点
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __shmPhymemFree (PLW_SHM_NODE  pshmn)
{
    if (pshmn->SHMN_pvPhyMem) {
        API_VmmPhyFree(pshmn->SHMN_pvPhyMem);                           /*  释放共享的物理内存          */
        _G_shmrRoot.SHMR_stMemUsed -= (size_t)pshmn->SHMN_oftSize;
        pshmn->SHMN_pvPhyMem = LW_NULL;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __shmOpen
** 功能描述: 共享内存设备 open 操作
** 输　入  : pdevhdr          shm设备
**           pcName           文件名
**           iFlags           方式
**           iMode            mode_t
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  __shmOpen (LW_DEV_HDR     *pdevhdr,
                        PCHAR           pcName,
                        INT             iFlags,
                        INT             iMode)
{
    PLW_SHM_NODE     pshmn       = LW_NULL;
    PLW_SHM_NODE     pshmnFather = LW_NULL;
    BOOL             bIsRoot;
    PCHAR            pcTail      = LW_NULL;
    INT              iError;
    
    if (__STR_IS_ROOT(pcName)) {
        LW_DEV_INC_USE_COUNT(&_G_devhdrShm);                            /*  更新计数器                  */
        return  ((LONG)LW_NULL);
    }
    
    if (iFlags & O_CREAT) {                                             /*  这里不过滤 socket 文件      */
        if (S_ISFIFO(iMode) || 
            S_ISBLK(iMode)  ||
            S_ISCHR(iMode)) {
            _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);                    /*  不支持以上这些格式          */
            return  (PX_ERROR);
        }
    }
    
    if (iFlags & O_TRUNC) {                                             /*  不允许打开截断              */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    if (iFlags & O_CREAT) {                                             /*  创建目录或文件              */
        iError = __shmMakeNode(pcName, iFlags, iMode, LW_NULL);
        if ((iError != ERROR_NONE) && (iFlags & O_EXCL)) {
            return  (PX_ERROR);                                         /*  无法创建                    */
        }
    }
    
    __LW_SHM_LOCK();
    pshmn = __shmFindNode(pcName, &pshmnFather, &bIsRoot, LW_NULL, &pcTail);
    if (pshmn) {
        pshmn->SHMN_iOpenNum++;
        if (!S_ISLNK(pshmn->SHMN_mode)) {                               /*  不是链接文件                */
            __LW_SHM_UNLOCK();                                          /*  解锁共享内存设备            */
            LW_DEV_INC_USE_COUNT(&_G_devhdrShm);                        /*  更新计数器                  */
            return  ((LONG)pshmn);
        }
    } else {
        __LW_SHM_UNLOCK();                                              /*  解锁共享内存设备            */
        return  (PX_ERROR);
    }
    __LW_SHM_UNLOCK();                                                  /*  解锁共享内存设备            */
    
    if (pshmn) {                                                        /*  链接文件处理                */
        INT     iFollowLinkType;
        PCHAR   pcSymfile = pcTail - lib_strlen(pshmn->SHMN_pcName) - 1;
        PCHAR   pcPrefix;
        
        if (*pcSymfile != PX_DIVIDER) {
            pcSymfile--;
        }
        if (pcSymfile == pcName) {
            pcPrefix = LW_NULL;                                         /*  没有前缀                    */
        } else {
            pcPrefix = pcName;
            *pcSymfile = PX_EOS;
        }
        if (pcTail && lib_strlen(pcTail)) {
            iFollowLinkType = FOLLOW_LINK_TAIL;                         /*  连接目标内部文件            */
        } else {
            iFollowLinkType = FOLLOW_LINK_FILE;                         /*  链接文件本身                */
        }
        
        iError = _PathBuildLink(pcName, MAX_FILENAME_LENGTH, 
                                LW_NULL, pcPrefix, (CPCHAR)pshmn->SHMN_pvLink, pcTail);
                                
        __LW_SHM_LOCK();                                                /*  锁定共享内存设备            */
        pshmn->SHMN_iOpenNum--;
        __LW_SHM_UNLOCK();                                              /*  解锁共享内存设备            */
        
        if (iError) {
            return  (PX_ERROR);                                         /*  无法建立被链接目标目录      */
        } else {
            return  (iFollowLinkType);
        }
    }
    
    LW_DEV_INC_USE_COUNT(&_G_devhdrShm);                                /*  更新计数器                  */
    
    return  ((LONG)pshmn);
}
/*********************************************************************************************************
** 函数名称: __shmRemove
** 功能描述: 共享内存设备 remove 操作
** 输　入  : pdevhdr
**           pcName           文件名
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __shmRemove (PLW_DEV_HDR     pdevhdr,
                         PCHAR           pcName)
{
    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        PLW_SHM_NODE    pshmnFather;
        PLW_SHM_NODE    pshmn;
        BOOL            bIsRoot;
        PCHAR           pcTail = LW_NULL;
        
        __LW_SHM_LOCK();                                                /*  锁定共享内存设备            */
        pshmn = __shmFindNode(pcName, &pshmnFather, &bIsRoot, LW_NULL, &pcTail);
        if (pshmn) {
            if (S_ISLNK(pshmn->SHMN_mode)) {                            /*  链接文件                    */
                size_t  stLenTail = 0;
                
                if (pcTail) {
                    stLenTail = lib_strlen(pcTail);                     /*  确定 tail 长度              */
                }
                
                /*
                 *  没有尾部, 表明需要删除链接文件本身, 即仅删除链接文件即可.
                 *  那么直接使用 __shmRemoveNode 删除即可.
                 */
                if (stLenTail) {
                    PCHAR   pcSymfile = pcTail - lib_strlen(pshmn->SHMN_pcName) - 1;
                    PCHAR   pcPrefix;
                    
                    if (*pcSymfile != PX_DIVIDER) {
                        pcSymfile--;
                    }
                    if (pcSymfile == pcName) {
                        pcPrefix = LW_NULL;                             /*  没有前缀                    */
                    } else {
                        pcPrefix = pcName;
                        *pcSymfile = PX_EOS;
                    }
                    
                    if (_PathBuildLink(pcName, MAX_FILENAME_LENGTH, 
                                       LW_NULL, pcPrefix, 
                                       (CPCHAR)pshmn->SHMN_pvLink, 
                                       pcTail) < ERROR_NONE) {
                        __LW_SHM_UNLOCK();                              /*  解锁共享内存设备            */
                        return  (PX_ERROR);                             /*  无法建立被链接目标目录      */
                    
                    } else {
                        __LW_SHM_UNLOCK();                              /*  解锁共享内存设备            */
                        return  (FOLLOW_LINK_TAIL);                     /*  连接文件内部文件            */
                    }
                }
            }
        }
        __LW_SHM_UNLOCK();                                              /*  解锁共享内存设备            */
    
        return  (__shmRemoveNode(pcName));
    }
}
/*********************************************************************************************************
** 函数名称: __shmClose
** 功能描述: 共享内存设备 close 操作
** 输　入  : pshmn             文件节点
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __shmClose (PLW_SHM_NODE    pshmn)
{
    __LW_SHM_LOCK();                                                    /*  锁定共享内存设备            */
    if (pshmn) {
        pshmn->SHMN_iOpenNum--;
    }
    __LW_SHM_UNLOCK();                                                  /*  解锁共享内存设备            */
    
    LW_DEV_DEC_USE_COUNT(&_G_devhdrShm);                                /*  更新计数器                  */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __shmRead
** 功能描述: 共享内存设备 read 操作
** 输　入  : pshmn            文件节点
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __shmRead (PLW_SHM_NODE  pshmn,
                           PCHAR         pcBuffer, 
                           size_t        stMaxBytes)
{
    _ErrorHandle(ENOSYS);
    return  (0);
}
/*********************************************************************************************************
** 函数名称: __shmPRead
** 功能描述: 共享内存设备 pread 操作
** 输　入  : pshmn         文件节点
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oftPos        位置
** 输　出  : 实际读取的个数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __shmPRead (PLW_SHM_NODE  pshmn,
                            PCHAR         pcBuffer, 
                            size_t        stMaxBytes,
                            off_t         oftPos)
{
    _ErrorHandle(ENOSYS);
    return  (0);
}
/*********************************************************************************************************
** 函数名称: __shmWrite
** 功能描述: 共享内存设备 write 操作
** 输　入  : pshmn            文件节点
**           pcBuffer         缓冲区
**           stNBytes         需要写入的数据
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __shmWrite (PLW_SHM_NODE pshmn,
                            PCHAR        pcBuffer, 
                            size_t       stNBytes)
{
    _ErrorHandle(ENOSYS);
    return  (0);
}
/*********************************************************************************************************
** 函数名称: __shmPWrite
** 功能描述: 共享内存设备 pwrite 操作
** 输　入  : pshmn         文件节点
**           pcBuffer      缓冲区
**           stBytes       需要写入的字节数
**           oftPos        位置
** 输　出  : 实际写入的个数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __shmPWrite (PLW_SHM_NODE pshmn, 
                             PCHAR        pcBuffer, 
                             size_t       stBytes,
                             off_t        oftPos)
{
    _ErrorHandle(ENOSYS);
    return  (0);
}
/*********************************************************************************************************
** 函数名称: __shmStatGet
** 功能描述: 共享内存设备获得文件状态及属性
** 输　入  : pshmn               文件节点
**           pstat               stat 结构
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __shmStatGet (PLW_SHM_NODE  pshmn, struct stat *pstat)
{
    if (pstat == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pshmn) {
        pstat->st_dev     = (dev_t)&_G_devhdrShm;
        pstat->st_ino     = (ino_t)pshmn;
        pstat->st_mode    = pshmn->SHMN_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = pshmn->SHMN_uid;
        pstat->st_gid     = pshmn->SHMN_gid;
        pstat->st_rdev    = 1;
        if (S_ISLNK(pshmn->SHMN_mode)) {
            pstat->st_size = lib_strlen((CPCHAR)pshmn->SHMN_pvLink);
        } else {
            pstat->st_size = pshmn->SHMN_oftSize;
        }
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        pstat->st_atime = pshmn->SHMN_time;                             /*  节点创建基准时间            */
        pstat->st_mtime = pshmn->SHMN_time;
        pstat->st_ctime = pshmn->SHMN_time;
    
    } else {
        pstat->st_dev     = (dev_t)&_G_devhdrShm;
        pstat->st_ino     = (ino_t)0;                                   /*  根目录                      */
        pstat->st_mode    = (0666 | S_IFDIR);                           /*  默认属性                    */
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;
        pstat->st_gid     = 0;
        pstat->st_rdev    = 1;
        pstat->st_size    = 0;
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        pstat->st_atime   = _G_shmrRoot.SHMR_time;                      /*  节点创建基准时间            */
        pstat->st_mtime   = _G_shmrRoot.SHMR_time;
        pstat->st_ctime   = _G_shmrRoot.SHMR_time;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __shmLStatGet
** 功能描述: 共享内存设备获得文件状态及属性 (如果是链接文件则获取连接文件的属性)
** 输　入  : pshmn               文件节点
**           pcName              文件名
**           pstat               stat 结构
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __shmLStatGet (LW_DEV_HDR *pdevhdr, PCHAR  pcName, struct stat *pstat)
{
    PLW_SHM_NODE    pshmnFather;
    PLW_SHM_NODE    pshmn;
    BOOL            bIsRoot;
    PCHAR           pcTail = LW_NULL;
    
    if (pcName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    __LW_SHM_LOCK();                                                    /*  锁定共享内存设备            */
    pshmn = __shmFindNode(pcName, &pshmnFather, &bIsRoot, LW_NULL, &pcTail);
                                                                        /*  查询设备                    */
    if (pshmn) {                                                        /*  一定不是 FOLLOW_LINK_TAIL   */
        pstat->st_dev     = (dev_t)&_G_devhdrShm;
        pstat->st_ino     = (ino_t)pshmn;
        pstat->st_mode    = pshmn->SHMN_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = pshmn->SHMN_uid;
        pstat->st_gid     = pshmn->SHMN_gid;
        pstat->st_rdev    = 1;
        if (S_ISLNK(pshmn->SHMN_mode)) {
            pstat->st_size = lib_strlen((CPCHAR)pshmn->SHMN_pvLink);
        } else {
            pstat->st_size = pshmn->SHMN_oftSize;
        }
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        pstat->st_atime = pshmn->SHMN_time;                             /*  节点创建基准时间            */
        pstat->st_mtime = pshmn->SHMN_time;
        pstat->st_ctime = pshmn->SHMN_time;
        
        __LW_SHM_UNLOCK();                                              /*  解锁共享内存设备            */
        
        return  (ERROR_NONE);
        
    } else {
        __LW_SHM_UNLOCK();                                              /*  解锁共享内存设备            */
        
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __shmStatfsGet
** 功能描述: 共享内存设备获得文件系统状态及属性
** 输　入  : pshmn               文件节点
**           pstatfs             statfs 结构
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __shmStatfsGet (PLW_SHM_NODE  pshmn, struct statfs *pstatfs)
{
    if (pstatfs) {
        pstatfs->f_type   = 0;
        pstatfs->f_bsize  = (long)_G_shmrRoot.SHMR_stMemUsed;           /*  文件系统内存使用量          */
        pstatfs->f_blocks = 1;
        pstatfs->f_bfree  = 0;
        pstatfs->f_bavail = 1;
        
        pstatfs->f_files  = 0;
        pstatfs->f_ffree  = 0;
        
#if LW_CFG_CPU_WORD_LENGHT == 64
        pstatfs->f_fsid.val[0] = (int32_t)((addr_t)&_G_devhdrShm >> 32);
        pstatfs->f_fsid.val[1] = (int32_t)((addr_t)&_G_devhdrShm & 0xffffffff);
#else
        pstatfs->f_fsid.val[0] = (int32_t)&_G_devhdrShm;
        pstatfs->f_fsid.val[1] = 0;
#endif
        
        pstatfs->f_flag    = 0;
        pstatfs->f_namelen = PATH_MAX;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __shmReadDir
** 功能描述: 共享内存设备获得指定目录信息
** 输　入  : pshmn               文件节点
**           dir                 目录结构
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __shmReadDir (PLW_SHM_NODE  pshmn, DIR  *dir)
{
             INT                i;
             INT                iError = ERROR_NONE;
    REGISTER LONG               iStart = dir->dir_pos;
             PLW_LIST_LINE      plineTemp;
             
             PLW_LIST_LINE      plineHeader;
             PLW_SHM_NODE       pshmnTemp;
             
    __LW_SHM_LOCK();                                                    /*  锁定共享内存设备            */
    if (pshmn == LW_NULL) {
        plineHeader = _G_shmrRoot.SHMR_plineSon;
    } else {
        plineHeader = pshmn->SHMN_plineSon;
    }
    
    for ((plineTemp  = plineHeader), (i = 0); 
         (plineTemp != LW_NULL) && (i < iStart); 
         (plineTemp  = _list_line_get_next(plineTemp)), (i++));         /*  忽略                        */
    
    if (plineTemp == LW_NULL) {
        _ErrorHandle(ENOENT);
        iError = PX_ERROR;                                              /*  没有多余的节点              */
    
    } else {
        pshmnTemp = _LIST_ENTRY(plineTemp, LW_SHM_NODE, SHMN_lineBrother);
        dir->dir_pos++;
        /*
         *  拷贝文件名
         */
        lib_strlcpy(dir->dir_dirent.d_name, 
                    pshmnTemp->SHMN_pcName, 
                    sizeof(dir->dir_dirent.d_name));                    /*  拷贝文件名                  */
                    
        dir->dir_dirent.d_type = IFTODT(pshmnTemp->SHMN_mode);
        dir->dir_dirent.d_shortname[0] = PX_EOS;
    }
    __LW_SHM_UNLOCK();                                                  /*  解锁共享内存设备            */
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __shmTruncate
** 功能描述: 共享内存设备设置文件大小
** 输　入  : pshmn               文件节点
**           oftSize             文件大小
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
** 注  意  : 每个文件只能设置一次.
*********************************************************************************************************/
static INT  __shmTruncate (PLW_SHM_NODE  pshmn, off_t  oftSize)
{
    __LW_SHM_LOCK();                                                    /*  锁定共享内存设备            */
    if (!pshmn) {
        __LW_SHM_UNLOCK();                                              /*  解锁共享内存设备            */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    if (!S_ISREG(pshmn->SHMN_mode)) {
        __LW_SHM_UNLOCK();                                              /*  解锁共享内存设备            */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (pshmn->SHMN_oftSize == oftSize) {
        __LW_SHM_UNLOCK();                                              /*  解锁共享内存设备            */
        return  (ERROR_NONE);
    }
    if (pshmn->SHMN_pvPhyMem) {                                         /*  正在映射工作中, 不能改变大小*/
        __LW_SHM_UNLOCK();                                              /*  解锁共享内存设备            */
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    pshmn->SHMN_oftSize = oftSize;                                      /*  记录新的文件大小            */
    __LW_SHM_UNLOCK();                                                  /*  解锁共享内存设备            */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __shmMmap
** 功能描述: 共享内存设备虚拟内存空间映射
** 输　入  : pshmn               文件节点
**           pdmap               虚拟空间信息
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT   __shmMmap (PLW_SHM_NODE  pshmn, PLW_DEV_MMAP_AREA  pdmap)
{
    addr_t   ulPhysical;

    __LW_SHM_LOCK();                                                    /*  锁定共享内存设备            */
    if (!pshmn) {
        __LW_SHM_UNLOCK();                                              /*  解锁共享内存设备            */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    if (!S_ISREG(pshmn->SHMN_mode)) {
        __LW_SHM_UNLOCK();                                              /*  解锁共享内存设备            */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (pshmn->SHMN_oftSize == 0) {
        __LW_SHM_UNLOCK();                                              /*  解锁共享内存设备            */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (pshmn->SHMN_pvPhyMem == LW_NULL) {
        if (__shmPhymemAlloc(pshmn) < ERROR_NONE) {                     /*  开辟物理内存                */
            __LW_SHM_UNLOCK();                                          /*  解锁共享内存设备            */
            return  (PX_ERROR);
        }
    }
    
    pshmn->SHMN_ulMapCnt++;                                             /*  mmap 计数++                 */

    ulPhysical  = (addr_t)pshmn->SHMN_pvPhyMem;
    ulPhysical += (addr_t)(pdmap->DMAP_offPages << LW_CFG_VMM_PAGE_SHIFT);                                   
    
    if (API_VmmRemapArea(pdmap->DMAP_pvAddr, (PVOID)ulPhysical, 
                         pdmap->DMAP_stLen, pdmap->DMAP_ulFlag,         /*  直接使用 mmap 指定的 flag   */
                         LW_NULL, LW_NULL)) {                           /*  将物理内存映射到虚拟内存    */
        if (pshmn->SHMN_ulMapCnt > 1) {
            pshmn->SHMN_ulMapCnt--;
        } else {
            pshmn->SHMN_ulMapCnt = 0;
            __shmPhymemFree(pshmn);                                     /*  释放物理内存                */
        }
        __LW_SHM_UNLOCK();                                              /*  解锁共享内存设备            */
        return  (PX_ERROR);
    }
    __LW_SHM_UNLOCK();                                                  /*  解锁共享内存设备            */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __shmUnmap
** 功能描述: 共享内存设备虚拟内存空间解除映射
** 输　入  : pshmn               文件节点
**           pdmap               虚拟空间信息
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT   __shmUnmap (PLW_SHM_NODE  pshmn, PLW_DEV_MMAP_AREA  pdmap)
{
    __LW_SHM_LOCK();                                                    /*  锁定共享内存设备            */
    if (!pshmn) {
        __LW_SHM_UNLOCK();                                              /*  解锁共享内存设备            */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    if (!S_ISREG(pshmn->SHMN_mode)) {
        __LW_SHM_UNLOCK();                                              /*  解锁共享内存设备            */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (pshmn->SHMN_oftSize == 0) {
        __LW_SHM_UNLOCK();                                              /*  解锁共享内存设备            */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (pshmn->SHMN_pvPhyMem) {
        if (pshmn->SHMN_ulMapCnt > 1) {
            pshmn->SHMN_ulMapCnt--;
        } else {
            pshmn->SHMN_ulMapCnt = 0;
            __shmPhymemFree(pshmn);                                     /*  不存在任何映射, 释放物理内存*/
        }
    }
    __LW_SHM_UNLOCK();                                                  /*  解锁共享内存设备            */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __shmLseek
** 功能描述: 共享内存设备 lseek 操作
** 输　入  : pshmn            文件节点
**           oftOffset        偏移量
**           iWhence          定位基准
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static off_t  __shmLseek (PLW_SHM_NODE  pshmn,
                          off_t         oftOffset, 
                          INT           iWhence)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __shmIoctl
** 功能描述: 共享内存设备 ioctl 操作
** 输　入  : pshmn              文件节点
**           request,           命令
**           arg                命令参数
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __shmIoctl (PLW_SHM_NODE  pshmn,
                        INT           iRequest,
                        LONG          lArg)
{
    off_t   oftSize;

    switch (iRequest) {
    
    case FIOFSTATGET:                                                   /*  获得文件状态                */
        return  (__shmStatGet(pshmn, (struct stat *)lArg));
        
    case FIOFSTATFSGET:                                                 /*  获得文件系统状态            */
        return  (__shmStatfsGet(pshmn, (struct statfs *)lArg));
    
    case FIOREADDIR:                                                    /*  获取一个目录信息            */
        return  (__shmReadDir(pshmn, (DIR *)lArg));
    
    case FIOTRUNC:                                                      /*  设置文件大小                */
        oftSize = *(off_t *)lArg;
        return  (__shmTruncate(pshmn, oftSize));
    
    case FIOSYNC:                                                       /*  将文件缓存回写              */
    case FIODATASYNC:
    case FIOFLUSH:
        return  (ERROR_NONE);
        
    case FIOFSTYPE:                                                     /*  获得文件系统类型            */
        *(PCHAR *)lArg = "Share Memory FileSystem";
        return  (ERROR_NONE);
        
    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __shmSymlink
** 功能描述: 共享内存设备 symlink 操作
** 输　入  : pdevhdr            
**           pcName             创建的连接文件
**           pcLinkDst          链接目标
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __shmSymlink (PLW_DEV_HDR     pdevhdr,
                          PCHAR           pcName,
                          CPCHAR          pcLinkDst)
{
    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
    
    return  (__shmMakeNode(pcName, O_CREAT | O_EXCL, DEFAULT_FILE_PERM | S_IFLNK, pcLinkDst));
}
/*********************************************************************************************************
** 函数名称: __shmReadlink
** 功能描述: 共享内存设备 read link 操作
** 输　入  : pdevhdr                       设备头
**           pcName                        链接原始文件名
**           pcLinkDst                     链接目标文件名
**           stMaxSize                     缓冲大小
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __shmReadlink (PLW_DEV_HDR    pdevhdr,
                               PCHAR          pcName,
                               PCHAR          pcLinkDst,
                               size_t         stMaxSize)
{
    PLW_SHM_NODE    pshmnFather = LW_NULL;
    PLW_SHM_NODE    pshmn;
    BOOL            bIsRoot = LW_FALSE;
    ssize_t         sstRet  = PX_ERROR;

    if ((pcName == LW_NULL) || (pcLinkDst == LW_NULL) || (stMaxSize == 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __LW_SHM_LOCK();                                                    /*  锁定共享内存设备            */
    pshmn = __shmFindNode(pcName, &pshmnFather, 
                          &bIsRoot, LW_NULL, LW_NULL);                  /*  查询设备                    */
    if (pshmn) {
        if (S_ISLNK(pshmn->SHMN_mode)) {
            size_t  stLen = lib_strlen((CPCHAR)pshmn->SHMN_pvLink);
            lib_strncpy(pcLinkDst, 
                        (CPCHAR)pshmn->SHMN_pvLink, stMaxSize);         /*  拷贝链接内容                */
            if (stLen > stMaxSize) {
                stLen = stMaxSize;                                      /*  计算有效字节数              */
            }
            sstRet = (ssize_t)stLen;
        
        } else {
            _ErrorHandle(ENOENT);
        }
    }
    __LW_SHM_UNLOCK();                                                  /*  解锁共享内存设备            */
    
    return  (sstRet);
}
/*********************************************************************************************************
** 函数名称: API_ShmDrvInstall
** 功能描述: 安装共享内存驱动程序
** 输　入  : NONE
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_ShmDrvInstall (VOID)
{
    struct file_operations     fileop;

    if (_G_iShmDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    if (_G_ulShmLock == LW_OBJECT_HANDLE_INVALID) {
        _G_ulShmLock = API_SemaphoreMCreate("shm_lock", LW_PRIO_DEF_CEILING, 
                                            LW_OPTION_WAIT_PRIORITY |
                                            LW_OPTION_INHERIT_PRIORITY |
                                            LW_OPTION_DELETE_SAFE |
                                            LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));
    
    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __shmOpen;
    fileop.fo_release  = __shmRemove;
    fileop.fo_open     = __shmOpen;
    fileop.fo_close    = __shmClose;
    fileop.fo_read     = __shmRead;
    fileop.fo_read_ex  = __shmPRead;
    fileop.fo_write    = __shmWrite;
    fileop.fo_write_ex = __shmPWrite;
    fileop.fo_lstat    = __shmLStatGet;
    fileop.fo_ioctl    = __shmIoctl;
    fileop.fo_lseek    = __shmLseek;
    fileop.fo_symlink  = __shmSymlink;
    fileop.fo_readlink = __shmReadlink;
    fileop.fo_mmap     = __shmMmap;
    fileop.fo_unmap    = __shmUnmap;
    
    _G_iShmDrvNum = iosDrvInstallEx(&fileop);
     
    DRIVER_LICENSE(_G_iShmDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iShmDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iShmDrvNum, "share memory driver.");
    
    return  ((_G_iShmDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** 函数名称: API_ShmDevCreate
** 功能描述: 安装共享内存设备
** 输　入  : NONE
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_ShmDevCreate (VOID)
{
    static BOOL     bIsInit = LW_FALSE;
    
    if (bIsInit) {
        return  (ERROR_NONE);
    }

    if (_G_iShmDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&_G_devhdrShm, "/dev/shm", _G_iShmDrvNum, DT_DIR) != ERROR_NONE) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    bIsInit = LW_TRUE;
    
    lib_time(&_G_shmrRoot.SHMR_time);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
                                                                        /*  LW_CFG_SHM_DEVICE_EN        */
                                                                        /*  LW_CFG_VMM_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
