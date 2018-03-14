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
** 文   件   名: symProc.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 03 月 04 日
**
** 描        述: 系统符号表管理器 proc 信息.

** BUG:
2011.03.06  修正 gcc 4.5.1 相关 warning.
2011.05.18  将 gsymbol 改为 ksymbol.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_SYMBOL_EN > 0) && (LW_CFG_PROCFS_EN > 0)
#include "symTable.h"
/*********************************************************************************************************
  符号表全局量
*********************************************************************************************************/
extern LW_OBJECT_HANDLE         _G_ulSymbolTableLock;
extern LW_LIST_LINE_HEADER      _G_plineSymbolHeaderHashTbl[LW_CFG_SYMBOL_HASH_SIZE];

#define __LW_SYMBOL_LOCK()      API_SemaphoreMPend(_G_ulSymbolTableLock, LW_OPTION_WAIT_INFINITE)
#define __LW_SYMBOL_UNLOCK()    API_SemaphoreMPost(_G_ulSymbolTableLock)
/*********************************************************************************************************
  记录全局量
*********************************************************************************************************/
extern size_t                   _G_stSymbolCounter;
extern size_t                   _G_stSymbolNameTotalLen;
/*********************************************************************************************************
  symbol proc 文件函数声明
*********************************************************************************************************/
static ssize_t  __procFsSymbolRead(PLW_PROCFS_NODE  p_pfsn, 
                                   PCHAR            pcBuffer, 
                                   size_t           stMaxBytes,
                                   off_t            oft);
/*********************************************************************************************************
  symbol proc 文件操作函数组
*********************************************************************************************************/
static LW_PROCFS_NODE_OP        _G_pfsnoSymbolFuncs = {
    __procFsSymbolRead, LW_NULL
};
/*********************************************************************************************************
  symbol proc 文件 (symbol 预置大小为零, 表示需要手动分配内存)
*********************************************************************************************************/
static LW_PROCFS_NODE           _G_pfsnSymbol[] = 
{          
    LW_PROCFS_INIT_NODE("ksymbol", 
                        (S_IRUSR | S_IRGRP | S_IROTH | S_IFREG), 
                        &_G_pfsnoSymbolFuncs, 
                        "S",
                        0),
};
/*********************************************************************************************************
  symbol 打印头
*********************************************************************************************************/
#if LW_CFG_CPU_WORD_LENGHT == 64
static const CHAR   _G_cSymbolInfoHdr[] = "\n\
         SYMBOL NAME                 ADDR         TYPE\n\
------------------------------ ---------------- --------\n";
#else
static const CHAR   _G_cSymbolInfoHdr[] = "\n\
         SYMBOL NAME             ADDR     TYPE\n\
------------------------------ -------- --------\n";
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */
/*********************************************************************************************************
** 函数名称: __procFsSymbolPrint
** 功能描述: 打印所有 symbol 信息
** 输　入  : pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
** 输　出  : 实际打印的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static size_t  __procFsSymbolPrint (PCHAR  pcBuffer, size_t  stMaxBytes)
{
    size_t          stRealSize;
    INT             i;
    PLW_SYMBOL      psymbol;
    PLW_LIST_LINE   plineTemp;
    
    stRealSize = bnprintf(pcBuffer, stMaxBytes, 0, "%s", _G_cSymbolInfoHdr);
    
    for (i = 0; i < LW_CFG_SYMBOL_HASH_SIZE; i++) {
        for (plineTemp  = _G_plineSymbolHeaderHashTbl[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            CHAR            cType[4] = "\0";
            
            psymbol = _LIST_ENTRY(plineTemp, LW_SYMBOL, SYM_lineManage);
            if (LW_SYMBOL_IS_REN(psymbol->SYM_iFlag)) {
                lib_strcat(cType, "R");
            }
            if (LW_SYMBOL_IS_WEN(psymbol->SYM_iFlag)) {
                lib_strcat(cType, "W");
            }
            if (LW_SYMBOL_IS_XEN(psymbol->SYM_iFlag)) {
                lib_strcat(cType, "X");
            }
            
            stRealSize = bnprintf(pcBuffer, stMaxBytes, stRealSize, 
#if LW_CFG_CPU_WORD_LENGHT == 64
                                  "%-30s %16lx %-8s\n",
#else
                                  "%-30s %08lx %-8s\n",
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */
                                  psymbol->SYM_pcName,
                                  (addr_t)psymbol->SYM_pcAddr,
                                  cType);
        }
    }
    
    stRealSize = bnprintf(pcBuffer, stMaxBytes, stRealSize, 
                          "\ntotal symbol: %zu\n", _G_stSymbolCounter);
    
    return  (stRealSize);
}
/*********************************************************************************************************
** 函数名称: __procFsSymbolRead
** 功能描述: procfs 读 symbol 命名节点 proc 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsSymbolRead (PLW_PROCFS_NODE  p_pfsn, 
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
        
        __LW_SYMBOL_LOCK();
        /*
         *  stNeedBufferSize 已按照最大大小计算了.
         */
        stNeedBufferSize  = (_G_stSymbolCounter * 48) + _G_stSymbolNameTotalLen;
        stNeedBufferSize += lib_strlen(_G_cSymbolInfoHdr) + 32;
        stNeedBufferSize += 64;                                         /*  最后一行的 total 字符串     */
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            errno = ENOMEM;
            __LW_SYMBOL_UNLOCK();
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  重新获得文件缓冲区地址      */
        
        stRealSize = __procFsSymbolPrint(pcFileBuffer, 
                                         stNeedBufferSize);             /*  打印信息                    */
                                             
        __LW_SYMBOL_UNLOCK();
        
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
** 函数名称: __procFsSymbolInit
** 功能描述: procfs 初始化 symbol proc 文件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __procFsSymbolInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnSymbol[0], "/");
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
                                                                        /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
