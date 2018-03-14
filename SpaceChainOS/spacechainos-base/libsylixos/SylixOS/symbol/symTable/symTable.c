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
** 文   件   名: symTable.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 02 月 26 日
**
** 描        述: 系统符号表管理器 (为程序装载器提供服务).

** BUG:
2010.03.13  查询时需要满足指定的 flag (有可能全局变量和函数同名)
2010.12.16  API_SymbolAdd() 分配内存大小错误, 导致内存访问越界.
2011.03.04  将 gcc 编译时自动调用的 c 库函数的符号添加到符号表, 装载动态链接库不会出现例如 memcpy memset.
            等符号缺失.
            加入 proc 信息.
2011.05.18  加入符号表查找 hook.
2012.02.03  API_SymbolAddStatic() 需要先将链表清零.
2012.05.10  加入符号表遍历函数 API_SymbolTraverse().
2012.12.18  __symbolHashInsert() 不加入表头, 这样优先使用 bsp 和 libc 安装的符号.
2013.01.13  进程中不可使用这些 API
2018.01.03  涉及兼容性符号不可导出.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_SYMBOL_EN > 0
#include "symTable.h"
#define  __SYMBOL_MAIN_FILE
#include "symStatic.h"
#include "../symBsp/symBsp.h"
#include "../symLibc/symLibc.h"
/*********************************************************************************************************
  hash
*********************************************************************************************************/
extern INT  __hashHorner(CPCHAR  pcKeyword, INT  iTableSize);           /*  hash 算法                   */
/*********************************************************************************************************
  proc 信息
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0
extern VOID __procFsSymbolInit(VOID);
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
  符号表全局量
*********************************************************************************************************/
LW_OBJECT_HANDLE                _G_ulSymbolTableLock = LW_OBJECT_HANDLE_INVALID;
LW_LIST_LINE_HEADER             _G_plineSymbolHeaderHashTbl[LW_CFG_SYMBOL_HASH_SIZE];
                                                                        /*  符号查询 hash 表            */
#define __LW_SYMBOL_LOCK()      API_SemaphoreMPend(_G_ulSymbolTableLock, LW_OPTION_WAIT_INFINITE)
#define __LW_SYMBOL_UNLOCK()    API_SemaphoreMPost(_G_ulSymbolTableLock)
/*********************************************************************************************************
  记录全局量
*********************************************************************************************************/
size_t                          _G_stSymbolCounter      = 0;
size_t                          _G_stSymbolNameTotalLen = 0;
/*********************************************************************************************************
  查找hook
*********************************************************************************************************/
static PVOIDFUNCPTR             _G_pfuncSymbolFindHook     = LW_NULL;
static VOIDFUNCPTR              _G_pfuncSymbolTraverseHook = LW_NULL;
/*********************************************************************************************************
  不可以倒出的符号
*********************************************************************************************************/
static const PCHAR              _G_pcSymHoldBack[] = {
    "pbuf_alloc",
    "pbuf_alloced_custom"
};
/*********************************************************************************************************
** 函数名称: __symbolFindHookSet
** 功能描述: 设置查找回调函数.
** 输　入  : pfuncSymbolFindHook           回调函数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOIDFUNCPTR  __symbolFindHookSet (PVOIDFUNCPTR  pfuncSymbolFindHook)
{
    PVOIDFUNCPTR     pfuncOld;
    
    __LW_SYMBOL_LOCK();                                                 /*  锁定符号表                  */
    pfuncOld = _G_pfuncSymbolFindHook;
    _G_pfuncSymbolFindHook = pfuncSymbolFindHook;
    __LW_SYMBOL_UNLOCK();                                               /*  解锁符号表                  */
    
    return  (pfuncOld);
}
/*********************************************************************************************************
** 函数名称: __symbolTraverseHookSet
** 功能描述: 设置遍历回调函数.
** 输　入  : pfuncSymbolTraverseHook       回调函数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOIDFUNCPTR  __symbolTraverseHookSet (VOIDFUNCPTR  pfuncSymbolTraverseHook)
{
    VOIDFUNCPTR     pfuncOld;
    
    __LW_SYMBOL_LOCK();                                                 /*  锁定符号表                  */
    pfuncOld = _G_pfuncSymbolTraverseHook;
    _G_pfuncSymbolTraverseHook = pfuncSymbolTraverseHook;
    __LW_SYMBOL_UNLOCK();                                               /*  解锁符号表                  */
    
    return  (pfuncOld);
}
/*********************************************************************************************************
** 函数名称: __symbolHashInsert
** 功能描述: 将一个符号插入到 hash 表.
** 输　入  : psymbol           符号
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __symbolHashInsert (PLW_SYMBOL  psymbol)
{
             INT             iHash = __hashHorner(psymbol->SYM_pcName, LW_CFG_SYMBOL_HASH_SIZE);
    REGISTER PLW_LIST_LINE  *pplineHashHeader;
    
    pplineHashHeader = &_G_plineSymbolHeaderHashTbl[iHash];
    
    _List_Line_Add_Tail(&psymbol->SYM_lineManage, 
                        pplineHashHeader);
}
/*********************************************************************************************************
** 函数名称: __symbolHashDelete
** 功能描述: 从 hash 表中删除一个符号.
** 输　入  : psymbol           符号
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __symbolHashDelete (PLW_SYMBOL  psymbol)
{
             INT             iHash = __hashHorner(psymbol->SYM_pcName, LW_CFG_SYMBOL_HASH_SIZE);
    REGISTER PLW_LIST_LINE  *pplineHashHeader;
    
    pplineHashHeader = &_G_plineSymbolHeaderHashTbl[iHash];
    
    _List_Line_Del(&psymbol->SYM_lineManage,
                   pplineHashHeader);
}
/*********************************************************************************************************
** 函数名称: __symbolHashFind
** 功能描述: 从 hash 表查询一个符号.
** 输　入  : pcName            符号名
**           iFlag             需要满足的条件 (0 表示任何条件)
** 输　出  : 符号
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_SYMBOL  __symbolHashFind (CPCHAR  pcName, INT  iFlag)
{
    INT             iHash = __hashHorner(pcName, LW_CFG_SYMBOL_HASH_SIZE);
    PLW_LIST_LINE   plineHash;
    PLW_SYMBOL      psymbol;
    
    plineHash = _G_plineSymbolHeaderHashTbl[iHash];
    for (;
         plineHash != LW_NULL;
         plineHash  = _list_line_get_next(plineHash)) {
         
        psymbol = _LIST_ENTRY(plineHash, LW_SYMBOL, SYM_lineManage);
        
        if ((lib_strcmp(pcName, psymbol->SYM_pcName) == 0) &&
            (psymbol->SYM_iFlag & iFlag) == iFlag) {                    /*  符号名和指定 flag 相同      */
            break;
        }
    }
    
    if (plineHash == LW_NULL) {
        return  (LW_NULL);
    } else {
        return  (psymbol);
    }
}
/*********************************************************************************************************
** 函数名称: API_SymbolInit
** 功能描述: 初始化系统符号表
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID  API_SymbolInit (VOID)
{
    static BOOL     bIsInit = LW_FALSE;
           INT      i;
    
    if (bIsInit) {
        return;
    } else {
        bIsInit = LW_TRUE;
    }
    
    /*
     *  创建错误将有 DEBUG 信息打印, 这里不判断创建错误.
     */
    _G_ulSymbolTableLock = API_SemaphoreMCreate("symtable_lock", LW_PRIO_DEF_CEILING, 
                                                LW_OPTION_WAIT_PRIORITY |
                                                LW_OPTION_INHERIT_PRIORITY | 
                                                LW_OPTION_DELETE_SAFE |
                                                LW_OPTION_OBJECT_GLOBAL, 
                                                LW_NULL);

    /*
     *  将系统静态符号表添加入 hash 表.
     */
    for (i = 0; i < (sizeof(_G_symStatic) / sizeof(LW_SYMBOL)); i++) {
        __symbolHashInsert(&_G_symStatic[i]);
        _G_stSymbolCounter++;
        _G_stSymbolNameTotalLen += lib_strlen(_G_symStatic[i].SYM_pcName);
    }
    
    __symbolAddBsp();                                                   /*  加入 BSP 符号表             */
    __symbolAddLibc();                                                  /*  加入常用 C 库符号表         */

#if LW_CFG_PROCFS_EN > 0
    __procFsSymbolInit();
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
}
/*********************************************************************************************************
** 函数名称: API_SymbolAddStatic
** 功能描述: 向符号表添加一个静态符号 
**           (此函数一般由 BSP 启动代码将系统的所有 API 导入到符号表中, 供模块装载器使用)
** 输　入  : psymbol       符号表数组首地址
**           iNum          数组大小
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_SymbolAddStatic (PLW_SYMBOL  psymbol, INT  iNum)
{
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)   (sizeof(x) / sizeof((x)[0]))
#endif

    INT      i;

    if (!psymbol || !iNum) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  只能由内核加入              */
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    __LW_SYMBOL_LOCK();                                                 /*  锁定符号表                  */
    for (i = 0; i < iNum; i++) {
        _INIT_LIST_LINE_HEAD(&psymbol->SYM_lineManage);                 /*  先将链表清零                */
        __symbolHashInsert(psymbol);
        _G_stSymbolCounter++;
        _G_stSymbolNameTotalLen += lib_strlen(psymbol->SYM_pcName);
        psymbol++;
    }
    __LW_SYMBOL_UNLOCK();                                               /*  解锁符号表                  */
    
    for (i = 0; i < ARRAY_SIZE(_G_pcSymHoldBack); i++) {
        API_SymbolDelete(_G_pcSymHoldBack[i], LW_SYMBOL_FLAG_XEN);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SymbolAdd
** 功能描述: 向符号表添加一个符号
** 输　入  : pcName        符号名
**           pcAddr        地址
**           iFlag         选项 (这里忽略静态符号)
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_SymbolAdd (CPCHAR  pcName, caddr_t  pcAddr, INT  iFlag)
{
    PLW_SYMBOL      psymbol;
    
    if (!pcName || !pcAddr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  只能由内核加入              */
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    iFlag &= ~LW_SYMBOL_FLAG_STATIC;
    
    /*
     *  注意, 这里有 1 个字节的 '\0' 空间.
     */
    psymbol = (PLW_SYMBOL)__SHEAP_ALLOC(sizeof(LW_SYMBOL) + lib_strlen(pcName) + 1);
    if (psymbol == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    psymbol->SYM_pcName = (PCHAR)psymbol + sizeof(LW_SYMBOL);
    lib_strcpy(psymbol->SYM_pcName, pcName);
    psymbol->SYM_pcAddr = pcAddr;
    psymbol->SYM_iFlag  = iFlag;
    
    __LW_SYMBOL_LOCK();                                                 /*  锁定符号表                  */
    __symbolHashInsert(psymbol);
    _G_stSymbolCounter++;
    _G_stSymbolNameTotalLen += lib_strlen(psymbol->SYM_pcName);
    __LW_SYMBOL_UNLOCK();                                               /*  解锁符号表                  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SymbolDelete
** 功能描述: 从符号表删除一个符号
** 输　入  : pcName            符号名
**           iFlag             需要满足的条件 (0 表示任何条件)
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_SymbolDelete (CPCHAR  pcName, INT  iFlag)
{
    PLW_SYMBOL      psymbol;

    if (!pcName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  只能由内核删除              */
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    __LW_SYMBOL_LOCK();                                                 /*  锁定符号表                  */
    psymbol = __symbolHashFind(pcName, iFlag);
    if (psymbol == LW_NULL) {
        __LW_SYMBOL_UNLOCK();                                           /*  解锁符号表                  */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    _G_stSymbolCounter--;
    _G_stSymbolNameTotalLen -= lib_strlen(psymbol->SYM_pcName);
    
    __symbolHashDelete(psymbol);
    __LW_SYMBOL_UNLOCK();                                               /*  解锁符号表                  */
    
    if (!LW_SYMBOL_IS_STATIC(psymbol->SYM_iFlag)) {                     /*  非静态符号                  */
        __SHEAP_FREE(psymbol);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SymbolFind
** 功能描述: 从符号表查找一个符号
** 输　入  : pcName        符号名
**           iFlag         需要满足的条件 (0 表示任何条件)
** 输　出  : 符号地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
PVOID  API_SymbolFind (CPCHAR  pcName, INT  iFlag)
{
    PLW_SYMBOL      psymbol;
    PVOID           pvAddr = LW_NULL;

    if (!pcName) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    __LW_SYMBOL_LOCK();                                                 /*  锁定符号表                  */
    psymbol = __symbolHashFind(pcName, iFlag);
    if (psymbol) {
        pvAddr = (PVOID)psymbol->SYM_pcAddr;
    }
    __LW_SYMBOL_UNLOCK();                                               /*  解锁符号表                  */
    
    if ((pvAddr == LW_NULL) && _G_pfuncSymbolFindHook) {
        /*
         *  _G_pfuncSymbolFindHook 非常安全, 只要一次设置, 是不会变成非法指针的.
         *  所以这里没有加入保护.
         */
        pvAddr = _G_pfuncSymbolFindHook(pcName, iFlag);                 /*  查找回调                    */
    }
    
    if (pvAddr == LW_NULL) {
        _ErrorHandle(ENOENT);
    }
    
    return  (pvAddr);
}
/*********************************************************************************************************
** 函数名称: API_SymbolTraverse
** 功能描述: 遍历系统符号表
** 输　入  : pfuncCb       回调函数 (注意, 此函数在符号表锁定时被调用, 返回值为 true 立即停止遍历)
**           pvArg         回调函数参数
** 输　出  : 符号地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID  API_SymbolTraverse (BOOL (*pfuncCb)(PVOID, PLW_SYMBOL), PVOID  pvArg)
{
    INT             i;
    PLW_SYMBOL      psymbol;
    PLW_LIST_LINE   plineTemp;
    BOOL            bStop = LW_FALSE;
    
    if (!pfuncCb) {
        _ErrorHandle(EINVAL);
        return;
    }
    
    __LW_SYMBOL_LOCK();                                                 /*  锁定符号表                  */
    for (i = 0; i < LW_CFG_SYMBOL_HASH_SIZE; i++) {
        for (plineTemp  = _G_plineSymbolHeaderHashTbl[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            psymbol = _LIST_ENTRY(plineTemp, LW_SYMBOL, SYM_lineManage);
            if (pfuncCb(pvArg, psymbol)) {
                bStop = LW_TRUE;
                break;
            }
        }
    }
    __LW_SYMBOL_UNLOCK();                                               /*  解锁符号表                  */
    
    if ((bStop == LW_FALSE) && _G_pfuncSymbolTraverseHook) {
        _G_pfuncSymbolTraverseHook(pfuncCb, pvArg);
    }
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
