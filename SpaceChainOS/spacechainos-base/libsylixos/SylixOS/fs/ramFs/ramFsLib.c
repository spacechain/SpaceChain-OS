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
** 文   件   名: ramFsLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 05 月 24 日
**
** 描        述: 内存文件系统内部函数.
**
** BUG:
2015.11.25  修正 ramFs seek 反向查找错误.
2017.12.27  修正 __ram_move() 符合 POSIX 规范.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0 && LW_CFG_RAMFS_EN > 0
#include "ramFsLib.h"
/*********************************************************************************************************
  操作宏
*********************************************************************************************************/
#define RAM_FOOT(blk1, blk2)    ((blk1 > blk2) ? (blk1 - blk2) : (blk2 - blk1))
/*********************************************************************************************************
** 函数名称: __ram_open
** 功能描述: ramfs 打开一个文件
** 输　入  : pramfs           文件系统
**           pcName           文件名
**           ppramnFather     当无法找到节点时保存最接近的一个,
                              但寻找到节点时保存父系节点. 
                              LW_NULL 表示根
             pbRoot           是否为根节点
**           pbLast           当匹配失败时, 是否是最后一级文件匹配失败
**           ppcTail          如果存在连接文件, 指向连接文件后的路径
** 输　出  : 打开结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PRAM_NODE  __ram_open (PRAM_VOLUME  pramfs,
                       CPCHAR       pcName,
                       PRAM_NODE   *ppramnFather,
                       BOOL        *pbRoot,
                       BOOL        *pbLast,
                       PCHAR       *ppcTail)
{
    CHAR                pcTempName[MAX_FILENAME_LENGTH];
    PCHAR               pcNext;
    PCHAR               pcNode;
    
    PRAM_NODE           pramn;
    PRAM_NODE           pramnTemp;
    
    PLW_LIST_LINE       plineTemp;
    PLW_LIST_LINE       plineHeader;                                    /*  当前目录头                  */
    
    if (ppramnFather == LW_NULL) {
        ppramnFather = &pramnTemp;                                      /*  临时变量                    */
    }
    *ppramnFather = LW_NULL;
    
    if (*pcName == PX_ROOT) {                                           /*  忽略根符号                  */
        lib_strlcpy(pcTempName, (pcName + 1), PATH_MAX);
    } else {
        lib_strlcpy(pcTempName, pcName, PATH_MAX);
    }
    
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
    plineHeader = pramfs->RAMFS_plineSon;                               /*  从根目录开始搜索            */
    
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
            
            pramn = _LIST_ENTRY(plineTemp, RAM_NODE, RAMN_lineBrother);
            if (S_ISLNK(pramn->RAMN_mode)) {                            /*  链接文件                    */
                if (lib_strcmp(pramn->RAMN_pcName, pcNode) == 0) {
                    goto    __find_ok;                                  /*  找到链接                    */
                }
            
            } else if (S_ISDIR(pramn->RAMN_mode)) {
                if (lib_strcmp(pramn->RAMN_pcName, pcNode) == 0) {      /*  已经找到一级目录            */
                    break;
                }
                
            } else {
                if (lib_strcmp(pramn->RAMN_pcName, pcNode) == 0) {
                    if (pcNext) {                                       /*  还存在下级, 这里必须为目录  */
                        goto    __find_error;                           /*  不是目录直接错误            */
                    }
                    break;
                }
            }
        }
        if (plineTemp == LW_NULL) {                                     /*  无法继续搜索                */
            goto    __find_error;
        }
        
        *ppramnFather = pramn;                                          /*  从当前节点开始搜索          */
        plineHeader   = pramn->RAMN_plineSon;                           /*  从第一个儿子开始            */
        
    } while (pcNext);                                                   /*  不存在下级目录              */
    
__find_ok:
    *ppramnFather = pramn->RAMN_pramnFather;                            /*  父系节点                    */
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
    return  (pramn);
    
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
** 函数名称: __ram_maken
** 功能描述: ramfs 创建一个文件
** 输　入  : pramfs           文件系统
**           pcName           文件名
**           pramnFather      父亲, NULL 表示根目录
**           mode             mode_t
**           pcLink           如果为连接文件, 这里指明连接目标.
** 输　出  : 创建结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PRAM_NODE  __ram_maken (PRAM_VOLUME  pramfs,
                        CPCHAR       pcName,
                        PRAM_NODE    pramnFather,
                        mode_t       mode,
                        CPCHAR       pcLink)
{
    PRAM_NODE   pramn = (PRAM_NODE)__SHEAP_ALLOC(sizeof(RAM_NODE));
    CPCHAR      pcFileName;
    
    if (pramn == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    lib_bzero(pramn, sizeof(RAM_NODE));
    
    pcFileName = lib_rindex(pcName, PX_DIVIDER);
    if (pcFileName) {
        pcFileName++;
    } else {
        pcFileName = pcName;
    }
    
    pramn->RAMN_pcName = (PCHAR)__SHEAP_ALLOC(lib_strlen(pcFileName) + 1);
    if (pramn->RAMN_pcName == LW_NULL) {
        __SHEAP_FREE(pramn);
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    lib_strcpy(pramn->RAMN_pcName, pcFileName);
    
    if (S_ISLNK(mode)) {
        pramn->RAMN_pcLink = (PCHAR)__SHEAP_ALLOC(lib_strlen(pcLink) + 1);
        if (pramn->RAMN_pcLink == LW_NULL) {
            __SHEAP_FREE(pramn->RAMN_pcName);
            __SHEAP_FREE(pramn);
            _ErrorHandle(ENOMEM);
            return  (LW_NULL);
        }
        lib_strcpy(pramn->RAMN_pcLink, pcLink);
        
    } else {
        if ((mode & S_IFMT) == 0) {
            mode |= S_IFREG;
        }
        pramn->RAMN_pcLink = LW_NULL;
    }
    
    pramn->RAMN_pramnFather = pramnFather;
    pramn->RAMN_pramfs      = pramfs;
    pramn->RAMN_mode        = mode;
    pramn->RAMN_timeCreate  = lib_time(LW_NULL);
    pramn->RAMN_timeAccess  = pramn->RAMN_timeCreate;
    pramn->RAMN_timeChange  = pramn->RAMN_timeCreate;
    pramn->RAMN_uid         = getuid();
    pramn->RAMN_gid         = getgid();
    pramn->RAMN_prambCookie = LW_NULL;
    
    if (pramnFather) {
        _List_Line_Add_Ahead(&pramn->RAMN_lineBrother, 
                             &pramnFather->RAMN_plineSon);
    } else {
        _List_Line_Add_Ahead(&pramn->RAMN_lineBrother, 
                             &pramfs->RAMFS_plineSon);
    }
    
    return  (pramn);
}
/*********************************************************************************************************
** 函数名称: __ram_unlink
** 功能描述: ramfs 删除一个文件
** 输　入  : pramn            文件节点
** 输　出  : 删除结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  __ram_unlink (PRAM_NODE  pramn)
{
    PRAM_VOLUME     pramfs;
    PRAM_NODE       pramnFather;
    PLW_LIST_LINE   plineTemp;
    
    if (S_ISDIR(pramn->RAMN_mode) && pramn->RAMN_plineSon) {
        _ErrorHandle(ENOTEMPTY);
        return  (PX_ERROR);
    }
    
    pramfs      = pramn->RAMN_pramfs;
    pramnFather = pramn->RAMN_pramnFather;
    
    if (pramnFather) {
        _List_Line_Del(&pramn->RAMN_lineBrother, 
                       &pramnFather->RAMN_plineSon);
    } else {
        _List_Line_Del(&pramn->RAMN_lineBrother, 
                       &pramfs->RAMFS_plineSon);
    }
    
    while (pramn->RAMN_plineBStart) {
        plineTemp = pramn->RAMN_plineBStart;
        _List_Line_Del(plineTemp, &pramn->RAMN_plineBStart);
        
        __RAM_BFREE(plineTemp);
        pramfs->RAMFS_ulCurBlk--;
        pramn->RAMN_ulCnt--;
    }
    
    if (S_ISLNK(pramn->RAMN_mode)) {
        __SHEAP_FREE(pramn->RAMN_pcLink);
    }
    __SHEAP_FREE(pramn->RAMN_pcName);
    __SHEAP_FREE(pramn);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ram_truncate
** 功能描述: ramfs 截断一个文件
** 输　入  : pramn            文件节点
**           stOft            阶段点
** 输　出  : 缩短结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __ram_truncate (PRAM_NODE  pramn, size_t  stOft)
{
    size_t          stTemp = 0;
    PLW_LIST_LINE   plineTemp;
    PLW_LIST_LINE   plineDel;
    PRAM_VOLUME     pramfs = pramn->RAMN_pramfs;
    
    for (plineTemp  = pramn->RAMN_plineBStart;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        if (stOft <= stTemp) {                                          /*  从此块开始删除              */
            break;
        } else {
            stTemp += __RAM_BDATASIZE;                                  /*  文件内容指针向前移动        */
        }
    }
    
    if (plineTemp) {                                                    /*  需要截断                    */
        if (plineTemp == pramn->RAMN_plineBStart) {                     /*  从第一个块开始删除          */
            pramn->RAMN_plineBStart = LW_NULL;
            pramn->RAMN_plineBEnd   = LW_NULL;
        
        } else if (plineTemp == pramn->RAMN_plineBEnd) {                /*  删除结束块                  */
            pramn->RAMN_plineBEnd = _list_line_get_prev(plineTemp);
        }
        
        do {
            plineDel  = plineTemp;
            plineTemp = _list_line_get_next(plineTemp);
            _List_Line_Del(plineDel, &pramn->RAMN_plineBStart);
            
            __RAM_BFREE(plineTemp);
            pramfs->RAMFS_ulCurBlk--;
            pramn->RAMN_ulCnt--;
        } while (plineTemp);
        
        pramn->RAMN_stSize      = stOft;                                /*  记录新的文件大小            */
        pramn->RAMN_stVSize     = stOft;
        pramn->RAMN_prambCookie = LW_NULL;                              /*  cookie 未知                 */
    }
}
/*********************************************************************************************************
** 函数名称: __ram_automem
** 功能描述: ramfs 根据需要设置文件缓冲区
** 输　入  : pramn            文件节点
**           ulNBlk           需求缓冲数量
**           stStart          在此范围内不清零
**           stLen
** 输　出  : 增长结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ram_automem (PRAM_NODE  pramn, ULONG  ulNBlk, size_t  stStart, size_t  stLen)
{
    ULONG           i;
    ULONG           ulNeedAllocBlk;
    PRAM_VOLUME     pramfs = pramn->RAMN_pramfs;
    size_t          stCur;
    
    if (ulNBlk <= pramn->RAMN_ulCnt) {                                  /*  实际数目够用                */
        return  (ERROR_NONE);
    
    } else {                                                            /*  需要增加块                  */
        PRAM_BUFFER     pramb;
        
        ulNeedAllocBlk = ulNBlk - pramn->RAMN_ulCnt;
        if ((ulNeedAllocBlk + pramfs->RAMFS_ulCurBlk) > 
            pramfs->RAMFS_ulMaxBlk) {                                   /*  超过文件系统大小限制        */
            _ErrorHandle(ENOSPC);
            return  (PX_ERROR);
        }
        
        stCur = (size_t)pramn->RAMN_ulCnt * __RAM_BDATASIZE;
        
        for (i = 0; i < ulNeedAllocBlk; i++) {
            pramb = (PRAM_BUFFER)__RAM_BALLOC(__RAM_BSIZE);
            if (pramb == LW_NULL) {
                _ErrorHandle(ENOSPC);
                return  (PX_ERROR);
            }
            
            if (((stCur < stStart) && ((stCur + __RAM_BDATASIZE) <= stStart)) ||
                ((stCur > stStart) && (stCur >= (stStart + stLen)))) {  /*  判断数据区是否要清零        */
                lib_bzero(pramb->RAMB_ucData, __RAM_BDATASIZE);
            }
            
            if (pramn->RAMN_plineBEnd) {                                /*  存在末尾块                  */
                _List_Line_Add_Right(&pramb->RAMB_lineManage,
                                     pramn->RAMN_plineBEnd);
                pramn->RAMN_plineBEnd = &pramb->RAMB_lineManage;
            
            } else {                                                    /*  没有缓冲                    */
                _List_Line_Add_Ahead(&pramb->RAMB_lineManage,
                                     &pramn->RAMN_plineBStart);
                pramn->RAMN_plineBEnd = pramn->RAMN_plineBStart;
            }
            
            pramfs->RAMFS_ulCurBlk++;
            pramn->RAMN_ulCnt++;
        }
        
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __ram_automem
** 功能描述: ramfs 根据需要设置文件缓冲区
** 输　入  : pramn            文件节点
**           ulNBlk           需求缓冲数量
**           stStart          在此范围内不清零
**           stLen
** 输　出  : 增长结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __ram_increase (PRAM_NODE  pramn, size_t  stNewSize)
{
    if (pramn->RAMN_stSize < stNewSize) {
        pramn->RAMN_stSize = stNewSize;
        if (pramn->RAMN_stVSize < pramn->RAMN_stSize) {
            pramn->RAMN_stVSize = pramn->RAMN_stSize;
        }
    }
}
/*********************************************************************************************************
** 函数名称: __ram_getbuf
** 功能描述: ramfs 读取文件内容
** 输　入  : pramn            文件节点
**           stOft            偏移量
**           pstBlkOft        获取的 block 内部偏移
** 输　出  : 获取的缓冲区
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PRAM_BUFFER  __ram_getbuf (PRAM_NODE  pramn, size_t  stOft, size_t  *pstBlkOft)
{
#define RAM_BCHK_VALID(x)   if (x == LW_NULL) { _ErrorHandle(ENOSPC); return  (LW_NULL); }

    PLW_LIST_LINE   plineTemp;
    ULONG           i;
    ULONG           ulFoot = __ARCH_ULONG_MAX;
    ULONG           ulBlkIndex;

    if (pramn->RAMN_plineBStart == LW_NULL) {
        _ErrorHandle(ENOSPC);
        return  (LW_NULL);
    }
    
    ulBlkIndex = (ULONG)(stOft / __RAM_BDATASIZE);
    *pstBlkOft = (ULONG)(stOft % __RAM_BDATASIZE);
    
    if (pramn->RAMN_prambCookie) {                                      /*  首先检查 cookie             */
        ulFoot = RAM_FOOT(pramn->RAMN_ulCookie, ulBlkIndex);
        if (ulFoot == 0ul) {
            return  (pramn->RAMN_prambCookie);
        }
    }
    
    if (ulFoot < ulBlkIndex) {                                          /*  通过 cookie 查找更快速      */
        plineTemp = (PLW_LIST_LINE)pramn->RAMN_prambCookie;
        if (pramn->RAMN_ulCookie < ulBlkIndex) {
            for (i = 0; i < ulFoot; i++) {
                plineTemp = _list_line_get_next(plineTemp);
                RAM_BCHK_VALID(plineTemp);
            }
        } else {
            for (i = 0; i < ulFoot; i++) {
                plineTemp = _list_line_get_prev(plineTemp);
                RAM_BCHK_VALID(plineTemp);
            }
        }
    } else {                                                            /*  需要通过 start end 查询     */
        REGISTER ULONG  ulForward  = ulBlkIndex;
        REGISTER ULONG  ulBackward = pramn->RAMN_ulCnt - ulBlkIndex - 1;
        
        if (ulForward <= ulBackward) {                                  /*  通过 start 指针查找更快     */
            plineTemp = pramn->RAMN_plineBStart;
            for (i = 0; i < ulForward; i++) {
                plineTemp = _list_line_get_next(plineTemp);
                RAM_BCHK_VALID(plineTemp);
            }
        } else {                                                        /*  通过 end 指针查找更快       */
            plineTemp = pramn->RAMN_plineBEnd;
            for (i = 0; i < ulBackward; i++) {
                plineTemp = _list_line_get_prev(plineTemp);
                RAM_BCHK_VALID(plineTemp);
            }
        }
    }
    
    pramn->RAMN_prambCookie = (PRAM_BUFFER)plineTemp;
    pramn->RAMN_ulCookie    = ulBlkIndex;
    return  (pramn->RAMN_prambCookie);
}
/*********************************************************************************************************
** 函数名称: __ram_getbuf_next
** 功能描述: ramfs 通过 cookie 获取下个缓冲
** 输　入  : pramn            文件节点
** 输　出  : 获取的缓冲区
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PRAM_BUFFER  __ram_getbuf_next (PRAM_NODE  pramn)
{
#define RAM_COOKIE_NEXT(cookie) (PRAM_BUFFER)_list_line_get_next(&(cookie)->RAMB_lineManage)

    if (pramn == LW_NULL) {
        return  (LW_NULL);
    }

    if (pramn->RAMN_prambCookie) {
        pramn->RAMN_prambCookie = RAM_COOKIE_NEXT(pramn->RAMN_prambCookie);
        pramn->RAMN_ulCookie++;
        return  (pramn->RAMN_prambCookie);
    
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: __ram_read
** 功能描述: ramfs 读取文件内容
** 输　入  : pramn            文件节点
**           pvBuffer         缓冲区
**           stSize           缓冲区大小
**           stOft            偏移量
** 输　出  : 读取的字节数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ssize_t  __ram_read (PRAM_NODE  pramn, PVOID  pvBuffer, size_t  stSize, size_t  stOft)
{
    PRAM_BUFFER     pramb;
    UINT8          *pucDest = (UINT8 *)pvBuffer;
    size_t          stDataLeft;
    size_t          stNBytes;
    size_t          stRead  = 0;
    size_t          stStart;
    
    if (pramn->RAMN_stVSize <= stOft) {                                 /*  已经到文件末尾              */
        return  (0);
    }
    
    stDataLeft = pramn->RAMN_stVSize - stOft;                           /*  计算剩余数据量              */
    stNBytes   = __MIN(stDataLeft, stSize);
    
    pramb = __ram_getbuf(pramn, stOft, &stStart);
    do {
        if (pramb == LW_NULL) {                                         /*  需要填充 0 (POSIX)          */
            lib_bzero(pucDest, stNBytes);
            stRead += stNBytes;
            break;
        
        } else {
            size_t  stBufSize = (__RAM_BDATASIZE - stStart);
            if (stBufSize >= stNBytes) {
                lib_memcpy(pucDest, &pramb->RAMB_ucData[stStart], stNBytes);
                stRead += stNBytes;
                break;
            
            } else {
                lib_memcpy(pucDest, &pramb->RAMB_ucData[stStart], stBufSize);
                pucDest  += stBufSize;
                stRead   += stBufSize;
                stNBytes -= stBufSize;
                stStart   = 0;                                          /*  下次拷贝从头开始           */
            }
        }
        pramb = __ram_getbuf_next(pramn);
    } while (stNBytes);
    
    return  ((ssize_t)stRead);
}
/*********************************************************************************************************
** 函数名称: __ram_write
** 功能描述: ramfs 写入文件内容
** 输　入  : pramn            文件节点
**           pvBuffer         缓冲区
**           stNBytes         需要读取的大小
**           stOft            偏移量
** 输　出  : 读取的字节数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ssize_t  __ram_write (PRAM_NODE  pramn, CPVOID  pvBuffer, size_t  stNBytes, size_t  stOft)
{
    PRAM_BUFFER     pramb;
    UINT8          *pucDest = (UINT8 *)pvBuffer;
    size_t          stEnd   = stOft + stNBytes;
    size_t          stWrite = 0;
    size_t          stStart;
    ULONG           ulNBlk;
    
    ulNBlk = (ULONG)(stEnd / __RAM_BDATASIZE);
    if (stEnd % __RAM_BDATASIZE) {
        ulNBlk++;
    }
    
    if (__ram_automem(pramn, ulNBlk, stOft, stNBytes)) {
        return  (0);                                                    /*  没有内存空间可供分配        */
    }
    
    pramb = __ram_getbuf(pramn, stOft, &stStart);
    while (pramb && stNBytes) {
        size_t  stBufSize = (__RAM_BDATASIZE - stStart);
        if (stBufSize >= stNBytes) {
            lib_memcpy(&pramb->RAMB_ucData[stStart], pucDest, stNBytes);
            stWrite += stNBytes;
            break;
        
        } else {
            lib_memcpy(&pramb->RAMB_ucData[stStart], pucDest, stBufSize);
            pucDest  += stBufSize;
            stWrite  += stBufSize;
            stNBytes -= stBufSize;
            stStart   = 0;                                              /*  下次从 0 开始               */
        }
        pramb = __ram_getbuf_next(pramn);
    }
    
    if (pramn->RAMN_stSize < (stOft + stWrite)) {
        pramn->RAMN_stSize = (stOft + stWrite);
        if (pramn->RAMN_stVSize < pramn->RAMN_stSize) {
            pramn->RAMN_stVSize = pramn->RAMN_stSize;
        }
    }
    
    return  ((ssize_t)stWrite);
}
/*********************************************************************************************************
** 函数名称: __ram_mount
** 功能描述: ramfs 挂载
** 输　入  : pramfs           文件系统
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __ram_mount (PRAM_VOLUME  pramfs)
{
}
/*********************************************************************************************************
** 函数名称: __ram_unlink_dir
** 功能描述: ramfs 删除一个目录
** 输　入  : plineDir         目录头链表
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __ram_unlink_dir (LW_LIST_LINE_HEADER plineDir)
{
    PLW_LIST_LINE   plineTemp;
    PRAM_NODE       pramn;
    
    while (plineDir) {
        plineTemp = plineDir;
        plineDir  = _list_line_get_next(plineDir);
        pramn     = _LIST_ENTRY(plineTemp, RAM_NODE, RAMN_lineBrother);
        if (S_ISDIR(pramn->RAMN_mode) && pramn->RAMN_plineSon) {        /*  目录文件                    */
            __ram_unlink_dir(pramn->RAMN_plineSon);                     /*  递归删除子目录              */
        }
        __ram_unlink(pramn);
    }
}
/*********************************************************************************************************
** 函数名称: __ram_ummount
** 功能描述: ramfs 卸载
** 输　入  : pramfs           文件系统
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __ram_unmount (PRAM_VOLUME  pramfs)
{
    __ram_unlink_dir(pramfs->RAMFS_plineSon);
}
/*********************************************************************************************************
** 函数名称: __ram_close
** 功能描述: ramfs 关闭一个文件
** 输　入  : pramn            文件节点
**           iFlag            打开文件时的方法
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __ram_close (PRAM_NODE  pramn, INT  iFlag)
{
    pramn->RAMN_timeAccess = lib_time(LW_NULL);

    if (pramn->RAMN_bChanged && ((iFlag & O_ACCMODE) != O_RDONLY)) {
        pramn->RAMN_timeChange = pramn->RAMN_timeAccess;
    }
}
/*********************************************************************************************************
** 函数名称: __ram_move_check
** 功能描述: ramfs 检查第二个节点是否为第一个节点的子孙
** 输　入  : pramn1        第一个节点
**           pramn2        第二个节点
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ram_move_check (PRAM_NODE  pramn1, PRAM_NODE  pramn2)
{
    do {
        if (pramn1 == pramn2) {
            return  (PX_ERROR);
        }
        pramn2 = pramn2->RAMN_pramnFather;
    } while (pramn2);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ram_move
** 功能描述: ramfs 移动或者重命名一个文件
** 输　入  : pramn            文件节点
**           pcNewName        新的名字
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  __ram_move (PRAM_NODE  pramn, PCHAR  pcNewName)
{
    INT         iRet;
    PRAM_VOLUME pramfs;
    PRAM_NODE   pramnTemp;
    PRAM_NODE   pramnFather;
    PRAM_NODE   pramnNewFather;
    BOOL        bRoot;
    BOOL        bLast;
    PCHAR       pcTail;
    PCHAR       pcTemp;
    PCHAR       pcFileName;
    
    pramfs      = pramn->RAMN_pramfs;
    pramnFather = pramn->RAMN_pramnFather;
    
    pramnTemp = __ram_open(pramfs, pcNewName, &pramnNewFather, &bRoot, &bLast, &pcTail);
    if (!pramnTemp && (bRoot || (bLast == LW_FALSE))) {                 /*  新名字指向根或者没有目录    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pramn == pramnTemp) {                                           /*  相同                        */
        return  (ERROR_NONE);
    }
    
    if (S_ISDIR(pramn->RAMN_mode) && pramnNewFather) {
        if (__ram_move_check(pramn, pramnNewFather)) {                  /*  检查目录合法性              */
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    }
    
    pcFileName = lib_rindex(pcNewName, PX_DIVIDER);
    if (pcFileName) {
        pcFileName++;
    } else {
        pcFileName = pcNewName;
    }
    
    pcTemp = (PCHAR)__SHEAP_ALLOC(lib_strlen(pcFileName) + 1);          /*  预分配名字缓存              */
    if (pcTemp == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    lib_strcpy(pcTemp, pcFileName);
    
    if (pramnTemp) {
        if (!S_ISDIR(pramn->RAMN_mode) && S_ISDIR(pramnTemp->RAMN_mode)) {
            __SHEAP_FREE(pcTemp);
            _ErrorHandle(EISDIR);
            return  (PX_ERROR);
        }
        if (S_ISDIR(pramn->RAMN_mode) && !S_ISDIR(pramnTemp->RAMN_mode)) {
            __SHEAP_FREE(pcTemp);
            _ErrorHandle(ENOTDIR);
            return  (PX_ERROR);
        }
        
        iRet = __ram_unlink(pramnTemp);                                 /*  删除目标                    */
        if (iRet) {
            __SHEAP_FREE(pcTemp);
            return  (PX_ERROR);
        }
    }
    
    if (pramnFather != pramnNewFather) {                                /*  目录发生改变                */
        if (pramnFather) {
            _List_Line_Del(&pramn->RAMN_lineBrother, 
                           &pramnFather->RAMN_plineSon);
        } else {
            _List_Line_Del(&pramn->RAMN_lineBrother, 
                           &pramfs->RAMFS_plineSon);
        }
        if (pramnNewFather) {
            _List_Line_Add_Ahead(&pramn->RAMN_lineBrother, 
                                 &pramnNewFather->RAMN_plineSon);
        } else {
            _List_Line_Add_Ahead(&pramn->RAMN_lineBrother, 
                                 &pramfs->RAMFS_plineSon);
        }
    }
    
    __SHEAP_FREE(pramn->RAMN_pcName);                                   /*  释放老名字                  */
    pramn->RAMN_pcName = pcTemp;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ram_stat
** 功能描述: ramfs 获得文件 stat
** 输　入  : pramn            文件节点
**           pramfs           文件系统
**           pstat            获得的 stat
** 输　出  : 创建结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __ram_stat (PRAM_NODE  pramn, PRAM_VOLUME  pramfs, struct stat  *pstat)
{
    if (pramn) {
        pstat->st_dev     = (dev_t)pramfs;
        pstat->st_ino     = (ino_t)pramn;
        pstat->st_mode    = pramn->RAMN_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = pramn->RAMN_uid;
        pstat->st_gid     = pramn->RAMN_gid;
        pstat->st_rdev    = 1;
        pstat->st_size    = (off_t)pramn->RAMN_stSize;
        pstat->st_atime   = pramn->RAMN_timeAccess;
        pstat->st_mtime   = pramn->RAMN_timeChange;
        pstat->st_ctime   = pramn->RAMN_timeCreate;
        pstat->st_blksize = __RAM_BSIZE;
        pstat->st_blocks  = (blkcnt_t)pramn->RAMN_ulCnt;
    
    } else {
        pstat->st_dev     = (dev_t)pramfs;
        pstat->st_ino     = (ino_t)0;
        pstat->st_mode    = pramfs->RAMFS_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = pramfs->RAMFS_uid;
        pstat->st_gid     = pramfs->RAMFS_gid;
        pstat->st_rdev    = 1;
        pstat->st_size    = 0;
        pstat->st_atime   = pramfs->RAMFS_time;
        pstat->st_mtime   = pramfs->RAMFS_time;
        pstat->st_ctime   = pramfs->RAMFS_time;
        pstat->st_blksize = __RAM_BSIZE;
        pstat->st_blocks  = 0;
    }
    
    pstat->st_resv1 = LW_NULL;
    pstat->st_resv2 = LW_NULL;
    pstat->st_resv3 = LW_NULL;
}
/*********************************************************************************************************
** 函数名称: __ram_statfs
** 功能描述: ramfs 获得文件 stat
** 输　入  : pramfs           文件系统
**           pstatfs          获得的 statfs
** 输　出  : 创建结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __ram_statfs (PRAM_VOLUME  pramfs, struct statfs  *pstatfs)
{
    pstatfs->f_type   = TMPFS_MAGIC;
    pstatfs->f_bsize  = __RAM_BSIZE;
    pstatfs->f_blocks = pramfs->RAMFS_ulMaxBlk;
    pstatfs->f_bfree  = pramfs->RAMFS_ulMaxBlk - pramfs->RAMFS_ulCurBlk;
    pstatfs->f_bavail = 1;
    
    pstatfs->f_files  = 0;
    pstatfs->f_ffree  = 0;
    
#if LW_CFG_CPU_WORD_LENGHT == 64
    pstatfs->f_fsid.val[0] = (int32_t)((addr_t)pramfs >> 32);
    pstatfs->f_fsid.val[1] = (int32_t)((addr_t)pramfs & 0xffffffff);
#else
    pstatfs->f_fsid.val[0] = (int32_t)pramfs;
    pstatfs->f_fsid.val[1] = 0;
#endif
    
    pstatfs->f_flag    = 0;
    pstatfs->f_namelen = PATH_MAX;
}

#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_RAMFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
