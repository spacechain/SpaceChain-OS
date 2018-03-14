/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: procPosix.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 30 日
**
** 描        述: proc 文件系统中 posix 子系统信息.

** BUG:
2010.08.11  简化了 read 操作.
2011.03.04  proc 文件 mode 为 S_IFREG.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../include/posixLib.h"                                        /*  已包含操作系统头文件        */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_POSIX_EN > 0) && (LW_CFG_PROCFS_EN > 0)
/*********************************************************************************************************
  posic 命名对象表
*********************************************************************************************************/
extern LW_LIST_LINE_HEADER          _G_plinePxNameNodeHash[];
extern UINT                         _G_uiNamedNodeCounter;
/*********************************************************************************************************
  posix proc 文件函数声明
*********************************************************************************************************/
static ssize_t  __procFsPosixNamedRead(PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft);
/*********************************************************************************************************
  posix proc 文件操作函数组
*********************************************************************************************************/
static LW_PROCFS_NODE_OP        _G_pfsnoPosixNamedFuncs = {
    __procFsPosixNamedRead, LW_NULL
};
/*********************************************************************************************************
  posix proc 文件目录树 (pnamed 预置大小为零, 表示需要手动分配内存)
*********************************************************************************************************/
static LW_PROCFS_NODE           _G_pfsnPosix[] = 
{
    LW_PROCFS_INIT_NODE("posix",  
                        (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH), 
                        LW_NULL, 
                        LW_NULL,  
                        0),
                        
    LW_PROCFS_INIT_NODE("pnamed", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoPosixNamedFuncs, 
                        "N",
                        0),
};
/*********************************************************************************************************
** 函数名称: __procFsPosixNamedPrint
** 功能描述: 打印所有 posix 命名节点的信息
** 输　入  : pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
** 输　出  : 实际打印的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static size_t  __procFsPosixNamedPrint (PCHAR  pcBuffer, size_t  stMaxBytes)
{
    const CHAR      cPosixNamedInfoHdr[] = 
    "TYPE  OPEN               NAME\n"
    "---- ------ --------------------------------\n";
    
    size_t          stRealSize;
    INT             i;
    __PX_NAME_NODE *pxnode;
    PLW_LIST_LINE   plineTemp;
          
    stRealSize = bnprintf(pcBuffer, stMaxBytes, 0, "%s", cPosixNamedInfoHdr);
    
    for (i = 0; i < __PX_NAME_NODE_HASH_SIZE; i++) {
        for (plineTemp  = _G_plinePxNameNodeHash[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            PCHAR   pcType;
            INT     iOpenCnt;
            
            pxnode = _LIST_ENTRY(plineTemp, __PX_NAME_NODE, PXNODE_lineManage);
            switch (pxnode->PXNODE_iType) {
            
            case __PX_NAMED_OBJECT_SEM:
                pcType = "SEM";
                break;
            
            case __PX_NAMED_OBJECT_MQ:
                pcType = "MQ";
                break;
                
            default:
                pcType = "?";
                break;
            }
            iOpenCnt = API_AtomicGet(&pxnode->PXNODE_atomic);
            
            stRealSize = bnprintf(pcBuffer, stMaxBytes, stRealSize, 
                                  "%-4s %6d %s\n", pcType, iOpenCnt, pxnode->PXNODE_pcName);
        }
    }
    
    return  (stRealSize);
}
/*********************************************************************************************************
** 函数名称: __procFsPosixNamedRead
** 功能描述: procfs 读 posix 命名节点 proc 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsPosixNamedRead (PLW_PROCFS_NODE  p_pfsn, 
                                        PCHAR            pcBuffer, 
                                        size_t           stMaxBytes,
                                        off_t            oft)
{
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  实际的文件内容大小          */
             size_t     stCopeBytes;

    /*
     *  由于预置内存大小为 0 , 所以打开后第一次读取需要手动开辟内存.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  还没有分配内存              */
        size_t      stNeedBufferSize;
        
        __PX_LOCK();                                                    /*  锁住 posix                  */
        stNeedBufferSize = (_G_uiNamedNodeCounter * (NAME_MAX + 64) + 128);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            errno = ENOMEM;
            __PX_UNLOCK();                                              /*  解锁 posix                  */
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  重新获得文件缓冲区地址      */
        
        stRealSize = __procFsPosixNamedPrint(pcFileBuffer, 
                                             stNeedBufferSize);         /*  打印信息                    */
        __PX_UNLOCK();                                                  /*  解锁 posix                  */
        
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
** 函数名称: __procFsPosixInfoInit
** 功能描述: procfs 初始化 posix proc 文件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __procFsPosixInfoInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnPosix[0], "/");
    API_ProcFsMakeNode(&_G_pfsnPosix[1], "/posix");
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
