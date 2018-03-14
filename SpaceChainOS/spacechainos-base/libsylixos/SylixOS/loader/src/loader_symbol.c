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
** 文   件   名: loader_symbol.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2010 年 04 月 17 日
**
** 描        述: 符号管理

** BUG:
2011.02.20  改进符号表管理, 加入局部和全局的区别. (韩辉)
2011.03.06  修正 gcc 4.5.1 相关 warnning. (韩辉)
2011.05.18  重要修正: 不管是全局符号还是局部符号, 都只保存在模块内部, 系统通过查找 hook 进行查找.
            这样允许全局库中拥有相同符号(必须为相同功能)的符号, 不会造成加载冲突, 同时系统在卸载这些模块时
            不会造成符号卸载错误. (韩辉)
2012.03.23  加入模块与操作系统兼容性检查. (韩辉)
2012.05.10  加入内核模块符号表遍历功能. (韩辉)
2012.10.16  将系统对无法定位的弱符号初始化放在 __moduleSymGetValue() 之中, 加入一个参数, 如果是弱符号, 
            则对应符号地址为 0 , 如果不是弱符号, 则打印错误. (韩辉)
2013.03.31  __moduleVerifyVersion() 加入模块类型, 如果是 so 类型, 则不进行版本判断. (韩辉)
2013.05.22  导出符号表不必每次都分配内存, 而是用成块内存分配的方法加快速度减少内存分片数. (韩辉)
            模块内符号表使用 hash 表, 不再使用扁平数组查询.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_error.h"
#include "../include/loader_lib.h"
#include "../include/loader_symbol.h"
/*********************************************************************************************************
  计算最佳的符号缓冲内存大小 (我们假设一个符号通常大小为 48 个字节)
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#define __SYMBOL_PAGE_ALIGN                         LW_CFG_VMM_PAGE_SIZE
#define __SYMBOL_BUFER_OPTIMAL(maxsym, cursym)      ((maxsym > cursym) ? \
                                                    ((maxsym - cursym) * 48) : \
                                                    (LW_CFG_VMM_PAGE_SIZE))
#else
#define __SYMBOL_PAGE_ALIGN                         4096
#define __SYMBOL_BUFER_OPTIMAL(maxsym, cursym)      4096
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  字符串 hash 算法
*********************************************************************************************************/
extern INT  __hashHorner(CPCHAR  pcKeyword, INT  iTableSize);
/*********************************************************************************************************
  进程列表
*********************************************************************************************************/
extern LW_LIST_LINE_HEADER      _G_plineVProcHeader;
/*********************************************************************************************************
** 函数名称: __moduleVerifyVersion
** 功能描述: 模块系统版本与当前系统匹配性检查
** 输　入  : pcModuleName  模块名称
**           pcVersion     编译模块是使用的操作系统版本
**           ulType        装载库的类型
** 输　出  : 符号值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __moduleVerifyVersion (CPCHAR  pcModuleName, CPCHAR  pcVersion, ULONG  ulType)
{
    ULONG       ulKoComNewest = __SYLIXOS_VERSION;                      /*  内核模块兼容的最大版本号    */
    ULONG       ulKoComOldest = __SYLIXOS_MAKEVER(1, 4, 0);             /*  内核模块兼容的最小版本号    */
    ULONG       ulSoComOldest = __SYLIXOS_MAKEVER(1, 4, 0);             /*  应用动态库兼容的最小版本号  */
    
    ULONG       ulModuleOsVersion;
    
    ULONG       ulMajor = 0, ulMinor = 0, ulRevision = 0;
    
    if (sscanf(pcVersion, "%ld.%ld.%ld", &ulMajor, &ulMinor, &ulRevision) != 3) {
        goto    __bad_version;
    }
    
    ulModuleOsVersion = __SYLIXOS_MAKEVER(ulMajor, ulMinor, ulRevision);
    
    if (ulType == LW_LD_MOD_TYPE_SO) {
        if (ulModuleOsVersion < ulSoComOldest) {                        /*  SO 不再兼容范围内           */
            goto    __bad_version;
        }
    
    } else {
        if (ulModuleOsVersion > ulKoComNewest ||
            ulModuleOsVersion < ulKoComOldest) {                        /*  KO 不再兼容范围内           */
            goto    __bad_version;
        }
    }
    
    return  (ERROR_NONE);
    
__bad_version:
    if ((ulType == LW_LD_MOD_TYPE_SO) && 
        (lib_strcmp(pcVersion, LW_LD_DEF_VER) == 0)) {
        return  (ERROR_NONE);                                           /*  没有包含版本的动态库, 通过! */
    }
    
    fprintf(stderr, 
            "[ld]Warning: %s %s OS-version %s, is not compatible with current SylixOS version.\n"
            "    Re-build this module with current SylixOS version, may solve this problem.\n",
            (ulType == LW_LD_MOD_TYPE_SO) ? "Share library" : "Kernel module",
            pcModuleName, pcVersion);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __moduleFindSymHook
** 功能描述: 系统查找全局符号表的回调函数. (这里仅搜索 ko 类型全局模块)
** 输　入  : pcSymName     符号名
**           iFlag         符号属性
** 输　出  : 符号值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PVOID  __moduleFindKernelSymHook (CPCHAR  pcSymName, INT  iFlag)
{
    LW_LD_EXEC_MODULE  *pmodTemp;
    PLW_SYMBOL          psymbol;
    PLW_LIST_RING       pringTemp;
    PVOID               pvAddr = LW_NULL;
    BOOL                bFind  = LW_FALSE;
    
    INT                 iHash;
    PLW_LIST_LINE       plineTemp;
    
    LW_VP_LOCK((&_G_vprocKernel));

    pringTemp = _G_vprocKernel.VP_ringModules;
    if (!pringTemp) {
        LW_VP_UNLOCK((&_G_vprocKernel));
        return  (pvAddr);
    }

    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        if (pmodTemp->EMOD_bIsGlobal &&
            pmodTemp->EMOD_ulModType == LW_LD_MOD_TYPE_KO) {            /*  只搜索 ko 类型全局模块      */
            
            if (pmodTemp->EMOD_ulSymHashSize) {
                iHash = __hashHorner(pcSymName, (INT)pmodTemp->EMOD_ulSymHashSize);
                for (plineTemp  = pmodTemp->EMOD_psymbolHash[iHash];
                     plineTemp != LW_NULL;
                     plineTemp  = _list_line_get_next(plineTemp)) {
                    
                    psymbol = _LIST_ENTRY(plineTemp, LW_SYMBOL, SYM_lineManage);
                    if ((lib_strcmp(pcSymName, psymbol->SYM_pcName) == 0) &&
                        (psymbol->SYM_iFlag & iFlag) == iFlag) {
                        pvAddr = (PVOID)psymbol->SYM_pcAddr;
                        bFind  = LW_TRUE;
                        break;
                    }
                }
            }
        }

        if (bFind) {
            break;
        }

        pringTemp = _list_ring_get_next(pringTemp);
    } while (pringTemp != _G_vprocKernel.VP_ringModules);

    LW_VP_UNLOCK((&_G_vprocKernel));
    
    return  (pvAddr);
}
/*********************************************************************************************************
** 函数名称: __moduleTraverseKernelSymHook
** 功能描述: 内核模块符号表遍历. (这里仅搜索 ko 类型全局模块)
** 输　入  : pfuncCb       回调函数
**           pvArg         回调参数
** 输　出  : 符号值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __moduleTraverseKernelSymHook (BOOL (*pfuncCb)(PVOID, PLW_SYMBOL), PVOID  pvArg)
{
    INT                 i;
    LW_LD_EXEC_MODULE  *pmodTemp;
    PLW_SYMBOL          psymbol;
    PLW_LIST_RING       pringTemp;
    PLW_LIST_LINE       plineTemp;

    LW_VP_LOCK((&_G_vprocKernel));
    
    pringTemp = _G_vprocKernel.VP_ringModules;
    if (!pringTemp) {
        LW_VP_UNLOCK((&_G_vprocKernel));
        return;
    }
    
    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        if (pmodTemp->EMOD_bIsGlobal &&
            pmodTemp->EMOD_ulModType == LW_LD_MOD_TYPE_KO) {            /*  只搜索 ko 类型全局模块      */
            
            for (i = 0; i < pmodTemp->EMOD_ulSymHashSize; i++) {
                for (plineTemp  = pmodTemp->EMOD_psymbolHash[i];
                     plineTemp != LW_NULL;
                     plineTemp  = _list_line_get_next(plineTemp)) {
                     
                    psymbol = _LIST_ENTRY(plineTemp, LW_SYMBOL, SYM_lineManage);
                    if (pfuncCb(pvArg, psymbol)) {
                        goto    __out;
                    }
                }
            }
        }
        pringTemp = _list_ring_get_next(pringTemp);
    } while (pringTemp != _G_vprocKernel.VP_ringModules);
    
__out:
    LW_VP_UNLOCK((&_G_vprocKernel));
}
/*********************************************************************************************************
** 函数名称: __moduleFindGlobalSym
** 功能描述: 查找操作系统全局符号.
** 输　入  : pcSymName     符号名
**           pulSymVal     符号值
**           iFlag         符号属性
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __moduleFindGlobalSym (CPCHAR pcSymName, addr_t *pulSymVal, INT iFlag)
{
    if (!pulSymVal) {
        return  (PX_ERROR);
    }

    LD_DEBUG_MSG(("__moduleFindGlobalSym(), name: %s\r\n", pcSymName));
    
    *pulSymVal = (addr_t)API_SymbolFind(pcSymName, iFlag);
    if (*pulSymVal == (addr_t)LW_NULL) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __moduleFindSym
** 功能描述: 查找模块内符号.
** 输　入  : pmodule       模块指针
**           pcSymName     符号名
**           pulSymVal     符号值
**           iFlag         符号属性
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __moduleFindSym (LW_LD_EXEC_MODULE  *pmodule, CPCHAR pcSymName, addr_t *pulSymVal, INT iFlag)
{
    PLW_SYMBOL          psymbol = LW_NULL;
    INT                 iHash;
    PLW_LIST_LINE       plineTemp = LW_NULL;

    LD_DEBUG_MSG(("__moduleFindSym(), name: %s\n", pcSymName));
    
    if (pmodule->EMOD_ulSymHashSize) {
        iHash = __hashHorner(pcSymName, (INT)pmodule->EMOD_ulSymHashSize);
        for (plineTemp  = pmodule->EMOD_psymbolHash[iHash];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            psymbol = _LIST_ENTRY(plineTemp, LW_SYMBOL, SYM_lineManage);
            if ((lib_strcmp(pcSymName, psymbol->SYM_pcName) == 0) &&
                (psymbol->SYM_iFlag & iFlag) == iFlag) {
                break;
            }
        }
    }

    if (plineTemp == LW_NULL) {
        return  (PX_ERROR);
    }

    *pulSymVal = (addr_t)psymbol->SYM_pcAddr;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __moduleTraverseSym
** 功能描述: 遍历模块链表内所有符号.
** 输　入  : pmodule       模块指针
**           pfuncCb       回调函数 (最后一个参数为 module 指针)
**           pvArg         回调参数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __moduleTraverseSym (LW_LD_EXEC_MODULE  *pmodule, 
                          BOOL (*pfuncCb)(PVOID, PLW_SYMBOL, LW_LD_EXEC_MODULE *), 
                          PVOID  pvArg)
{
    INT                 i;
    PLW_SYMBOL          psymbol;
    LW_LD_EXEC_MODULE  *pmodTemp;
    PLW_LIST_RING       pringTemp;
    PLW_LIST_LINE       plineTemp;
    
    pringTemp = &pmodule->EMOD_ringModules;
    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        
        for (i = 0; i < pmodTemp->EMOD_ulSymHashSize; i++) {
            for (plineTemp  = pmodTemp->EMOD_psymbolHash[i];
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                
                psymbol = _LIST_ENTRY(plineTemp, LW_SYMBOL, SYM_lineManage);
                if (pfuncCb(pvArg, psymbol, pmodTemp)) {
                    return;
                }
            }
        }
        
        pringTemp = _list_ring_get_next(pringTemp);
    } while (pringTemp != &pmodule->EMOD_ringModules);
}
/*********************************************************************************************************
** 函数名称: __moduleSymGetValue
** 功能描述: 查找符号,首先在依赖链表中查找，然后在全局符号表中查找
** 输　入  : pmodule       模块指针
**           bIsWeak       是否是弱符号
**           pcSymName     符号名
**           pulSymVal     符号值
**           iFlag         符号属性
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __moduleSymGetValue (LW_LD_EXEC_MODULE  *pmodule, BOOL  bIsWeak, 
                         CPCHAR pcSymName, addr_t *pulSymVal, INT iFlag)
{
    LW_LD_EXEC_MODULE  *pmodTemp   = NULL;
    PLW_LIST_RING       pringTemp  = NULL;

    /*
     *  首先查找当前进程(内核模块)链表
     */
    pringTemp = _list_ring_get_next(&pmodule->EMOD_ringModules);
    while (pringTemp != &pmodule->EMOD_ringModules) {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        if (pmodTemp->EMOD_bIsGlobal) {
            if (ERROR_NONE == __moduleFindSym(pmodTemp, pcSymName, pulSymVal, iFlag)) {
                return  (ERROR_NONE);
            }
        }
        pringTemp = _list_ring_get_next(pringTemp);
    }

    /*
     *  查找内核全局符号表
     */
    if (ERROR_NONE == __moduleFindGlobalSym(pcSymName, pulSymVal, iFlag)) {
        return  (ERROR_NONE);
    }
    
    if (bIsWeak) {
        *pulSymVal = 0ul;                                               /*  弱符号对应 0 地址           */
        return  (ERROR_NONE);
    }
    
    fprintf(stderr, "[ld]Library %s can not find symbol: %s\n", pmodule->EMOD_pcModulePath, pcSymName);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __moduleExportSymbol
** 功能描述: 导出符号. (最后两个参数是为了提高内存效率)
** 输　入  : pmodule       模块指针
**           pcSymName     符号名
**           ulSymVal      符号值
**           iFlag         符号属性
**           ulAllSymCnt   总符号数
**           ulCurSymNum   当前为第几个
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误.
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __moduleExportSymbol (LW_LD_EXEC_MODULE  *pmodule,
                          CPCHAR              pcSymName,
                          addr_t              ulSymVal,
                          INT                 iFlag,
                          ULONG               ulAllSymCnt,
                          ULONG               ulCurSymNum)
{
    PLW_LIST_LINE           plineTemp;
    LW_LD_EXEC_SYMBOL      *pesym;
    PLW_SYMBOL              psymbol = LW_NULL;
    size_t                  stNeedSize;
    INT                     iHash;
    LW_LIST_LINE_HEADER    *pplineHeader;

    if (LW_NULL == pcSymName) {
        return  (PX_ERROR);
    }
    
    stNeedSize = sizeof(LW_SYMBOL) + lib_strlen(pcSymName) + 1;
    stNeedSize = ROUND_UP(stNeedSize, sizeof(LW_STACK));                /*  计算对齐内存大小            */
    
    for (plineTemp  = pmodule->EMOD_plineSymbolBuffer;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  遍历已经存在的符号缓冲      */
         
        pesym = _LIST_ENTRY(plineTemp, LW_LD_EXEC_SYMBOL, ESYM_lineManage);
        if ((pesym->ESYM_stSize - pesym->ESYM_stUsed) >= stNeedSize) {
            psymbol = (PLW_SYMBOL)((PCHAR)pesym + pesym->ESYM_stUsed);  /*  剩余空间足够使用            */
            pesym->ESYM_stUsed += stNeedSize;
            break;
        }
    }
    
    if (plineTemp == LW_NULL) {                                         /*  没有找到合适的内存          */
        size_t   stSymBufferSize = (size_t)__SYMBOL_BUFER_OPTIMAL(ulAllSymCnt, ulCurSymNum);
        stSymBufferSize = ROUND_UP(stSymBufferSize, __SYMBOL_PAGE_ALIGN);
        
        pesym = (LW_LD_EXEC_SYMBOL *)LW_LD_VMSAFEMALLOC(stSymBufferSize);
        if (pesym == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "vmm low memory!\r\n");
            return  (PX_ERROR);
        }
        pesym->ESYM_stSize = stSymBufferSize;
        pesym->ESYM_stUsed = LW_LD_EXEC_SYMBOL_HDR_SIZE;                /*  控制结构已经占了一部分内存  */
        if ((pesym->ESYM_stSize - pesym->ESYM_stUsed) >= stNeedSize) {
            psymbol = (PLW_SYMBOL)((PCHAR)pesym + pesym->ESYM_stUsed);  /*  剩余空间足够使用            */
            pesym->ESYM_stUsed += stNeedSize;
        } else {
            LD_DEBUG_MSG(("symbol name too long\n"));
            LW_LD_VMSAFEFREE(pesym);
            return  (PX_ERROR);
        }
        _List_Line_Add_Ahead(&pesym->ESYM_lineManage, 
                             &pmodule->EMOD_plineSymbolBuffer);         /*  加入缓冲链表                */
    }
    
    psymbol->SYM_pcName = (PCHAR)psymbol + sizeof(LW_SYMBOL);
    lib_strcpy(psymbol->SYM_pcName, pcSymName);
    psymbol->SYM_pcAddr = (caddr_t)ulSymVal;
    psymbol->SYM_iFlag  = iFlag;

    iHash = __hashHorner(pcSymName, (INT)pmodule->EMOD_ulSymHashSize);
    pplineHeader = &pmodule->EMOD_psymbolHash[iHash];
    
    _List_Line_Add_Ahead(&psymbol->SYM_lineManage, pplineHeader);       /*  连入符号 hash 表            */
    
    pmodule->EMOD_ulSymCount++;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __moduleDeleteAllSymbol
** 功能描述: 删除模块内符号表.
** 输　入  : pmodule       模块指针
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __moduleDeleteAllSymbol (LW_LD_EXEC_MODULE *pmodule)
{
    PLW_LIST_LINE       plineTemp;
    LW_LD_EXEC_SYMBOL  *pesym;
    
    plineTemp = pmodule->EMOD_plineSymbolBuffer;
    while (plineTemp) {
        pesym     = _LIST_ENTRY(plineTemp, LW_LD_EXEC_SYMBOL, ESYM_lineManage);
        plineTemp = _list_line_get_next(plineTemp);
        LW_LD_VMSAFEFREE(pesym);
    }
    pmodule->EMOD_plineSymbolBuffer = LW_NULL;                          /*  已经释放所有符号缓冲内存    */
    
    lib_bzero(pmodule->EMOD_psymbolHash, 
              sizeof(LW_LIST_LINE_HEADER) * (size_t)pmodule->EMOD_ulSymHashSize);
    pmodule->EMOD_ulSymCount = 0ul;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __moduleSymbolBufferSize
** 功能描述: 获取模块符号表缓冲内存大小
** 输　入  : pmodule       模块指针
** 输　出  : 缓冲大小
** 全局变量:
** 调用模块:
*********************************************************************************************************/
size_t __moduleSymbolBufferSize (LW_LD_EXEC_MODULE *pmodule)
{
    PLW_LIST_LINE       plineTemp;
    LW_LD_EXEC_SYMBOL  *pesym;
    size_t              stBufferSize = 0;
    
    for (plineTemp  = pmodule->EMOD_plineSymbolBuffer;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  遍历已经存在的符号缓冲      */
         
        pesym = _LIST_ENTRY(plineTemp, LW_LD_EXEC_SYMBOL, ESYM_lineManage);
        stBufferSize += pesym->ESYM_stSize;
    }
    
    return  (stBufferSize);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
