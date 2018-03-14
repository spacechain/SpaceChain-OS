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
** 文   件   名: procFsLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 03 日
**
** 描        述: proc 文件系统内部库.
**
** 注        意: 有些 API 没有参数判断(效率考虑). 驱动程序必须传入有效的参数!

** BUG:
2009.12.13  修正了 __procFsFindNode() 判断目录的错误.
2012.08.26  procfs 支持符号链接.
2013.08.13  加入对文件数量的统计功能.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "procFsLib.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0
/*********************************************************************************************************
  procfs 全局变量
*********************************************************************************************************/
LW_PROCFS_ROOT      _G_pfsrRoot;                                        /*  procFs 根                   */
LW_OBJECT_HANDLE    _G_ulProcFsLock;                                    /*  procFs 操作锁               */
/*********************************************************************************************************
** 函数名称: __procFsFindNode
** 功能描述: procfs 查找一个节点
** 输　入  : pcName            节点名 (必须从 procfs 根开始)
**           pp_pfsnFather     当无法找到节点时保存最接近的一个,
                               但寻找到节点时保存父系节点. 
                               LW_NULL 表示根
**           pbRoot            节点名是否指向根节点.
**           pbLast            当匹配失败时, 是否是最后一级文件匹配失败
**           ppcTail           名字尾部, 当搜索到设备或者链接时, tail 指向名字尾.
** 输　出  : 节点, LW_NULL 表示根节点
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_PROCFS_NODE  __procFsFindNode (CPCHAR            pcName, 
                                   PLW_PROCFS_NODE  *pp_pfsnFather, 
                                   BOOL             *pbRoot,
                                   BOOL             *pbLast,
                                   PCHAR            *ppcTail)
{
    CHAR                pcTempName[MAX_FILENAME_LENGTH];
    PCHAR               pcNext;
    PCHAR               pcNode;
    
    PLW_PROCFS_NODE     p_pfsn;
    PLW_PROCFS_NODE     p_pfsnTemp;
    
    PLW_LIST_LINE       plineTemp;
    PLW_LIST_LINE       plineHeader;                                    /*  当前目录头                  */
    
    if (pp_pfsnFather == LW_NULL) {
        pp_pfsnFather = &p_pfsnTemp;                                    /*  临时变量                    */
    }
    *pp_pfsnFather = LW_NULL;
    
    if (*pcName == PX_ROOT) {                                           /*  忽略根符号                  */
        lib_strlcpy(pcTempName, (pcName + 1), PATH_MAX);
    } else {
        lib_strlcpy(pcTempName, pcName, PATH_MAX);
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
    plineHeader = _G_pfsrRoot.PFSR_plineSon;                            /*  从根目录开始搜索            */
    
    /*
     *  这里使用循环 (避免递归!)
     */
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
             
            p_pfsn = _LIST_ENTRY(plineTemp, LW_PROCFS_NODE, PFSN_lineBrother);
            if (S_ISLNK(p_pfsn->PFSN_mode)) {                           /*  链接文件                    */
                if (lib_strcmp(p_pfsn->PFSN_pcName, pcNode) == 0) {
                    goto    __find_ok;                                  /*  找到链接                    */
                }
            
            } else if (S_ISDIR(p_pfsn->PFSN_mode)) {
                if (lib_strcmp(p_pfsn->PFSN_pcName, pcNode) == 0) {     /*  已经找到一级目录            */
                    break;
                }
            
            } else {                                                    /*  找到普通文件                */
                if (lib_strcmp(p_pfsn->PFSN_pcName, pcNode) == 0) {
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
        
        *pp_pfsnFather = p_pfsn;                                        /*  从当前节点开始搜索          */
        plineHeader    = p_pfsn->PFSN_plineSon;                         /*  从第一个儿子开始            */
        
    } while (pcNext);                                                   /*  不存在下级目录              */
    
__find_ok:
    *pp_pfsnFather = p_pfsn->PFSN_p_pfsnFather;                         /*  父系节点                    */
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
    return  (p_pfsn);
    
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
** 函数名称: API_ProcFsMakeNode
** 功能描述: procfs 创建一个节点 (必须使用 LW_PROCFS_INIT_NODE 或者 LW_PROCFS_INIT_NODE_IN_CODE 初始化)
** 输　入  : p_pfsnNew         新的节点 (由用户自行创建内存并初始化结构)
**           pcFatherName      父系节点名
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_ProcFsMakeNode (PLW_PROCFS_NODE  p_pfsnNew, CPCHAR  pcFatherName)
{
    PLW_PROCFS_NODE     p_pfsnFather;
    BOOL                bIsRoot;
    INT                 iError = PX_ERROR;

    if ((p_pfsnNew == LW_NULL) || (pcFatherName == LW_NULL)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (S_ISLNK(p_pfsnNew->PFSN_mode)) {                                /*  链接文件                    */
        if (p_pfsnNew->PFSN_pfsnmMessage.PFSNM_pvBuffer == LW_NULL) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
    }
    
    p_pfsnNew->PFSN_plineSon = LW_NULL;                                 /*  还没有儿子                  */
    p_pfsnNew->PFSN_uid      = getuid();
    p_pfsnNew->PFSN_gid      = getgid();
    
    __LW_PROCFS_LOCK();                                                 /*  锁 procfs                   */
    p_pfsnFather = __procFsFindNode(pcFatherName, LW_NULL, &bIsRoot,
                                    LW_NULL, LW_NULL);                  /*  查找父系节点                */
    if (p_pfsnFather == LW_NULL) {
        if (bIsRoot) {                                                  /*  父系为根节点                */
            p_pfsnNew->PFSN_p_pfsnFather = LW_NULL;
            _List_Line_Add_Ahead(&p_pfsnNew->PFSN_lineBrother,
                                 &_G_pfsrRoot.PFSR_plineSon);           /*  插入根节点                  */
            iError = ERROR_NONE;
        }
    } else {
        if (S_ISDIR(p_pfsnFather->PFSN_mode)) {                         /*  父亲必须为目录              */
            p_pfsnNew->PFSN_p_pfsnFather = p_pfsnFather;                /*  保存父系节点                */
            _List_Line_Add_Ahead(&p_pfsnNew->PFSN_lineBrother,
                                 &p_pfsnFather->PFSN_plineSon);         /*  链入指定的链表              */
            iError = ERROR_NONE;
        }
    }
    if (iError) {
        _ErrorHandle(ENOENT);                                           /*  没有指定的目录              */
    } else {
        _G_pfsrRoot.PFSR_ulFiles++;                                     /*  增加一个文件                */
    }
    __LW_PROCFS_UNLOCK();                                               /*  解锁 procfs                 */
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __procFsRemoveNode
** 功能描述: procfs 删除一个节点
** 输　入  : p_pfsn            节点控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __procFsRemoveNode (PLW_PROCFS_NODE  p_pfsn)
{
    PLW_PROCFS_NODE     p_pfsnFather;

    p_pfsnFather = p_pfsn->PFSN_p_pfsnFather;                           /*  获得父系节点                */
    if (p_pfsnFather == LW_NULL) {
        _List_Line_Del(&p_pfsn->PFSN_lineBrother,
                       &_G_pfsrRoot.PFSR_plineSon);                     /*  从根节点卸载                */
    } else {
        _List_Line_Del(&p_pfsn->PFSN_lineBrother,
                       &p_pfsnFather->PFSN_plineSon);                   /*  从父节点卸载                */
        p_pfsn->PFSN_p_pfsnFather = LW_NULL;
    }
    
    if (S_ISLNK(p_pfsn->PFSN_mode)) {                                   /*  链接文件                    */
        if (p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer) {
            __SHEAP_FREE(p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer);     /*  释放链接目标内存            */
        }
        p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer = LW_NULL;
        p_pfsn->PFSN_pfsnmMessage.PFSNM_stBufferSize = 0;
    }
    
    if (p_pfsn->PFSN_pfuncFree) {
        p_pfsn->PFSN_pfuncFree(p_pfsn);
    }
    
    _G_pfsrRoot.PFSR_ulFiles--;                                         /*  减少一个文件                */
}
/*********************************************************************************************************
** 函数名称: API_ProcFsRemoveNode
** 功能描述: procfs 删除一个节点 (可能会产生推迟删除)
** 输　入  : p_pfsn            节点控制块
**           pfuncFree         清除函数
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_ProcFsRemoveNode (PLW_PROCFS_NODE  p_pfsn, VOIDFUNCPTR  pfuncFree)
{
    if (p_pfsn == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __LW_PROCFS_LOCK();                                                 /*  锁 procfs                   */
    p_pfsn->PFSN_pfuncFree = pfuncFree;
    
    if (p_pfsn->PFSN_iOpenNum) {                                        /*  节点被打开                  */
        p_pfsn->PFSN_bReqRemove = LW_TRUE;                              /*  删除标志                    */
        __LW_PROCFS_UNLOCK();                                           /*  解锁 procfs                 */
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }
    
    __procFsRemoveNode(p_pfsn);                                         /*  释放节点                    */
    __LW_PROCFS_UNLOCK();                                               /*  解锁 procfs                 */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ProcFsAllocNodeBuffer
** 功能描述: procfs 为一个节点开辟缓存
** 输　入  : p_pfsn            节点控制块
**           stSize            缓存大小
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_ProcFsAllocNodeBuffer (PLW_PROCFS_NODE  p_pfsn, size_t  stSize)
{
    if (!p_pfsn) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!S_ISREG(p_pfsn->PFSN_mode)) {                                  /*  非 reg 文件不释放缓冲区     */
        return  (ERROR_NONE);
    }
    
    __LW_PROCFS_LOCK();                                                 /*  锁 procfs                   */
    if (p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer) {
        __SHEAP_FREE(p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer);
        p_pfsn->PFSN_pfsnmMessage.PFSNM_stBufferSize = 0;
    }
    p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer = __SHEAP_ALLOC(stSize);
    if (p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer == LW_NULL) {
        __LW_PROCFS_UNLOCK();                                           /*  解锁 procfs                 */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    p_pfsn->PFSN_pfsnmMessage.PFSNM_stBufferSize = stSize;
    __LW_PROCFS_UNLOCK();                                               /*  解锁 procfs                 */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ProcFsFreeNodeBuffer
** 功能描述: procfs 释放节点缓存
** 输　入  : p_pfsn            节点控制块
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_ProcFsFreeNodeBuffer (PLW_PROCFS_NODE  p_pfsn)
{
    if (!p_pfsn) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!S_ISREG(p_pfsn->PFSN_mode)) {                                  /*  非 reg 文件不释放缓冲区     */
        return  (ERROR_NONE);
    }
    
    __LW_PROCFS_LOCK();                                                 /*  锁 procfs                   */
    if (p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer) {
        __SHEAP_FREE(p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer);
    }
    p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer = LW_NULL;
    p_pfsn->PFSN_pfsnmMessage.PFSNM_stBufferSize = 0;
    __LW_PROCFS_UNLOCK();                                               /*  解锁 procfs                 */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ProcFsNodeBufferSize
** 功能描述: procfs 获得节点缓存大小
** 输　入  : p_pfsn            节点控制块
** 输　出  : 缓冲大小
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
size_t  API_ProcFsNodeBufferSize (PLW_PROCFS_NODE  p_pfsn)
{
    if (p_pfsn == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (0);
    }

    return  (p_pfsn->PFSN_pfsnmMessage.PFSNM_stBufferSize);
}
/*********************************************************************************************************
** 函数名称: API_ProcFsNodeBuffer
** 功能描述: procfs 获得节点缓存指针
** 输　入  : p_pfsn            节点控制块
** 输　出  : 缓冲指针
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
PVOID  API_ProcFsNodeBuffer (PLW_PROCFS_NODE  p_pfsn)
{
    if (p_pfsn == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    return  (p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer);
}
/*********************************************************************************************************
** 函数名称: API_ProcFsNodeMessageValue
** 功能描述: procfs 获得节点信息私有数据指针
** 输　入  : p_pfsn            节点控制块
** 输　出  : 所有数据指针
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
PVOID  API_ProcFsNodeMessageValue (PLW_PROCFS_NODE  p_pfsn)
{
    if (p_pfsn == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    return  (p_pfsn->PFSN_pfsnmMessage.PFSNM_pvValue);
}
/*********************************************************************************************************
** 函数名称: API_ProcFsNodeSetRealFileSize
** 功能描述: procfs 设置实际的 BUFFER 大小
** 输　入  : p_pfsn            节点控制块
**           stRealSize        实际文件内容大小
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID    API_ProcFsNodeSetRealFileSize (PLW_PROCFS_NODE  p_pfsn, size_t  stRealSize)
{
    if (p_pfsn) {
        p_pfsn->PFSN_pfsnmMessage.PFSNM_stRealSize = stRealSize;
    }
}
/*********************************************************************************************************
** 函数名称: API_ProcFsNodeGetRealFileSize
** 功能描述: procfs 获取实际的 BUFFER 大小
** 输　入  : p_pfsn            节点控制块
** 输　出  : 实际文件内容大小
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
size_t  API_ProcFsNodeGetRealFileSize (PLW_PROCFS_NODE  p_pfsn)
{
    if (p_pfsn) {
        return  (p_pfsn->PFSN_pfsnmMessage.PFSNM_stRealSize);
    } else {
        return  (0);
    }
}

#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
