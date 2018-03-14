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
** 文   件   名: elf_loader.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2010 年 04 月 17 日
**
** 描        述: 实现 elf 加载

** BUG:
2010.08.05  加入 status 详细信息.
2011.02.20  改进符号表管理, 加入局部和全局的区别. (韩辉)
2011.03.01  当 st_other != STV_DEFAULT 时, 不将此符号导入符号表.
2011.03.23  支持 .so 与 C++ 模块.
2011.05.20  loader 仅调用 API_CacheTextUpdate() 即可. (韩辉)
2011.12.09  将 API_CacheTextUpdate() 放在最外层. (韩辉)
2012.03.23  加入模块与操作系统兼容性检查. (韩辉)
2012.04.21  elfSymbolExport() 如果是 __sylixos_version 符号, 则强制导出. (韩辉)
2012.09.29  使 elf stat 函数获取的信息更加详尽和易读. (韩辉)
2012.10.16  __moduleSymGetValue() 来负责弱符号的定位, 这样比较统一. (韩辉)
2012.12.10  如果是符号匹配找到的入口地址, 这里记录下来. (韩辉)
2013.03.27  装载依赖的兄弟库时, 应该使用倒叙方式. (韩辉)
2013.05.22  elfSymbolExport 加两个参数, 一个为总符号数, 一个为当前导出的符号数. 以提高整体内存效率.
            模块内符号表使用 hash 表, 不再使用扁平数组. (韩辉)
            不再使用缓冲 IO 而是用标准 IO 接口.
2017.02.26  加入可信计算支持.
2017.08.17  加入 c6x DSP 的支持.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
#include "../include/loader_symbol.h"
#include "../include/loader_error.h"
#include "elf_type.h"
#include "elf_loader.h"
#include "elf_arch.h"
/*********************************************************************************************************
  _G_pcInitSecArr: 初始化函数节区名
  _G_pcFiniSecArr: 结束函数节区名
  目前不的实现不处理节区间调用的循序
*********************************************************************************************************/
static const PCHAR              _G_pcInitSecArr[] = {".preinit_array", ".init_array"};
static const PCHAR              _G_pcFiniSecArr[] = {".fini_array"};
#define __LW_CTORS_SECTION      ".ctors"                                /*  GCC 默认构造与析构函数节    */
#define __LW_DTORS_SECTION      ".dtors"
/*********************************************************************************************************
  module管理层提供的回调函数，用于依赖库加载
*********************************************************************************************************/
extern LW_LD_EXEC_MODULE *moduleLoadSub(LW_LD_EXEC_MODULE *pmodule, CPCHAR pchLibName, BOOL bCreate);
/*********************************************************************************************************
  分析函数
*********************************************************************************************************/
static CPCHAR  __elfGetMachineStr(Elf_Half  ehMachine);
/*********************************************************************************************************
  是否将 entry 函数导入符号表
*********************************************************************************************************/
#define __LW_ENTRY_SYMBOL       1
/*********************************************************************************************************
** 函数名称: elfSymHashSize
** 功能描述: 根据符号数量确定 hash 表大小.
** 输　入  : ulSymMax      符号数量
** 输　出  : hash 大小
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT elfSymHashSize (ULONG  ulSymMax)
{
    if (ulSymMax < 10) {
        return  (1);
    } else if (ulSymMax < 100) {
        return  (13);
    } else if (ulSymMax < 1000) {
        return  (97);
    } else if (ulSymMax < 5000) {
        return  (191);
    } else {
        return  (397);
    }
}
/*********************************************************************************************************
** 函数名称: elfCheck
** 功能描述: 检查elf文件头有效性.
** 输　入  : pehdr         文件头
**           bLoad         是否为装载操作
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT elfCheck (Elf_Ehdr *pehdr, BOOL bLoad)
{
    if ((pehdr->e_ident[EI_MAG0] != ELFMAG0) ||                         /*  检查ELF魔数                 */
        (pehdr->e_ident[EI_MAG1] != ELFMAG1) ||
        (pehdr->e_ident[EI_MAG2] != ELFMAG2) ||
        (pehdr->e_ident[EI_MAG3] != ELFMAG3)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown file format!\r\n");
        _ErrorHandle(ERROR_LOADER_FORMAT);
        return  (PX_ERROR);
    }
    
    if (ELF_CLASS != pehdr->e_ident[EI_CLASS]) {                        /*  检查ELF CPU字长是否匹配     */
        if (bLoad) {
            fprintf(stderr, "[ld]Architecture error: this CPU is %d-bits but ELF file is %d-bits!\n",
                    (ELF_CLASS == ELFCLASS32) ? 32 : 64,
                    (pehdr->e_ident[EI_CLASS] == ELFCLASS32) ? 32 : 64);
        }
        _ErrorHandle(ERROR_LOADER_ARCH);
        return  (PX_ERROR);
    }

    if (ELF_ARCH != pehdr->e_machine) {                                 /*  检查ELF体系结构是否匹配     */
        if (bLoad) {
            fprintf(stderr, "[ld]Architecture error: this CPU is \"%s\" but ELF file CPU is \"%s\"!\n",
                    __elfGetMachineStr(ELF_ARCH), __elfGetMachineStr(pehdr->e_machine));
        }
        _ErrorHandle(ERROR_LOADER_ARCH);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: elfRelaRelocate
** 功能描述: 处理带加数的重定位表项.
** 输　入  : pmodule       模块指针
**           psymTable     符号表
**           prela         重定位表
**           pcTargetSect  目标节区
**           ulRelocCount  重定位项数
**           pcStrTab      字符串表
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT elfRelaRelocate (LW_LD_EXEC_MODULE *pmodule,
                            Elf_Sym           *psymTable,
                            Elf_Rela          *prela,
                            PCHAR              pcTargetSect,
                            ULONG              ulRelocCount,
                            PCHAR              pcStrTab)
{
    Elf_Sym   *psym;
    PCHAR      pcJmpTable;
    PCHAR      pcSymName;

    size_t     stJmpTableItem;
    Elf_Addr   addrSymVal;
    ULONG      i;

    LD_DEBUG_MSG(("relocateSectionRela()\r\n"));

    pcJmpTable     =                                                    /*  跳转表起始地址              */
        (PCHAR)pmodule->EMOD_psegmentArry[pmodule->EMOD_ulSegCount].ESEG_ulAddr;
    stJmpTableItem =                                                    /*  跳转表大小                  */
        pmodule->EMOD_psegmentArry[pmodule->EMOD_ulSegCount].ESEG_stLen;

    for (i = 0; i < ulRelocCount; i++) {
        psym = &psymTable[ELF_R_SYM(prela->r_info)];
        if (SHN_UNDEF == psym->st_shndx) {                              /*  外部符号需查找符号表        */
            pcSymName = pcStrTab + psym->st_name;
            if (lib_strlen(pcSymName) == 0) {                           /* 特殊符号，和体系结构相关     */
                addrSymVal = 0;
            } else {
                BOOL    bWeak = (STB_WEAK == ELF_ST_BIND(psym->st_info));
                if (__moduleSymGetValue(pmodule,
                                        bWeak,
                                        pcSymName,
                                        &addrSymVal,
                                        LW_LD_SYM_ANY) < 0) {           /*  查询对应符号的地址          */
                    _ErrorHandle(ERROR_LOADER_NO_SYMBOL);
                    return  (PX_ERROR);
                }
            }
            
        } else if (psym->st_shndx < pmodule->EMOD_ulSegCount) {         /*  模块内部符号                */
            addrSymVal = (Elf_Addr)pmodule->EMOD_psegmentArry[psym->st_shndx].ESEG_ulAddr
                       + psym->st_value;
        }

        LD_DEBUG_MSG(("relocate %s :", pcSymName));

#if !defined(LW_CFG_CPU_ARCH_C6X) && !defined(LW_CFG_CPU_ARCH_SPARC)
        if (archElfRelocateRela(pmodule,
                                prela,
                                addrSymVal,
                                pcTargetSect,
                                pcJmpTable,
                                stJmpTableItem) < 0) {                  /*  与体系结构相关的重定位      */
            _ErrorHandle(ERROR_LOADER_RELOCATE);
            return  (PX_ERROR);
        }
#else
        if (archElfRelocateRela(pmodule,
                                prela,
                                psym,
                                addrSymVal,
                                pcTargetSect,
                                pcJmpTable,
                                stJmpTableItem) < 0) {                  /*  与体系结构相关的重定位      */
            _ErrorHandle(ERROR_LOADER_RELOCATE);
            return  (PX_ERROR);
        }
#endif

        prela++;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: elfRelRelocate
** 功能描述: 处理不带加数的重定位表项.
** 输　入  : pmodule       模块指针
**           psymTable     符号表
**           prel          重定位表
**           pcTargetSect  目标节区
**           ulRelocCount  重定位项数
**           pcStrTab      字符串表
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT elfRelRelocate (LW_LD_EXEC_MODULE *pmodule,
                           Elf_Sym           *psymTable,
                           Elf_Rel           *prel,
                           PCHAR              pcTargetSect,
                           ULONG              ulRelocCount,
                           PCHAR              pcStrTab)
{
    Elf_Sym   *psym;
    PCHAR      pcJmpTable;
    PCHAR      pcSymName;

    size_t     stJmpTableItem;
    Elf_Addr   symVal;
    ULONG      i;

    LD_DEBUG_MSG(("relocateSectionRel()\r\n"));

    pcJmpTable     =                                                    /*  跳转表起始地址              */
            (PCHAR)pmodule->EMOD_psegmentArry[pmodule->EMOD_ulSegCount].ESEG_ulAddr;
    stJmpTableItem =                                                    /*  跳转表大小                  */
            pmodule->EMOD_psegmentArry[pmodule->EMOD_ulSegCount].ESEG_stLen;

    for (i = 0; i < ulRelocCount; i++) {
        psym = &psymTable[ELF_R_SYM(prel->r_info)];
        pcSymName = pcStrTab + psym->st_name;
        if (SHN_UNDEF == psym->st_shndx) {                              /*  外部符号需查找符号表        */
            if (lib_strlen(pcSymName) == 0) {                           /* 特殊符号，和体系结构相关     */
                symVal = 0;
            } else {
                BOOL    bWeak = (STB_WEAK == ELF_ST_BIND(psym->st_info));
                if (__moduleSymGetValue(pmodule,
                                        bWeak,
                                        pcSymName,
                                        &symVal,
                                        LW_LD_SYM_ANY) < 0) {           /*  查询对应符号的地址          */
                    _ErrorHandle(ERROR_LOADER_NO_SYMBOL);
                    return  (PX_ERROR);
                }
            }
            
        } else if (psym->st_shndx < pmodule->EMOD_ulSegCount) {         /*  模块内部符号                */
            symVal = (Elf_Addr)pmodule->EMOD_psegmentArry[psym->st_shndx].ESEG_ulAddr
                   + psym->st_value;
        }

        LD_DEBUG_MSG(("relocate %s :", pcSymName));
        if (archElfRelocateRel(pmodule,
                               prel,
                               symVal,
                               pcTargetSect,
                               pcJmpTable,
                               stJmpTableItem) < 0) {                   /*  与体系结构相关的重定位      */
            _ErrorHandle(ERROR_LOADER_RELOCATE);
            return  (PX_ERROR);
        }

        prel++;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: elfSectionsRelocate
** 功能描述: 处理所有重定位节.
** 输　入  : pmodule       模块指针
**           pshdrArr      节区头部表
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT elfSectionsRelocate (LW_LD_EXEC_MODULE *pmodule, Elf_Shdr *pshdrArr)
{
    Elf_Shdr  *pshdr;
    Elf_Sym   *psymTable;
    PCHAR      pcTargetSect;
    PCHAR      pcStrTab;

    ULONG      ulRelocCount;
    ULONG      ulStrShIndex;
    INT        iError = PX_ERROR;
    ULONG      i;

    LD_DEBUG_MSG(("relocateSections()\r\n"));
    pshdr = pshdrArr;
    for (i = 0; i < pmodule->EMOD_ulSegCount; i++, pshdr++) {
        if (pshdr->sh_type != SHT_REL && pshdr->sh_type != SHT_RELA) {  /* 只处理重定位节               */
            continue;
        }

        if (pshdr->sh_entsize == 0) {
            _ErrorHandle(ERROR_LOADER_FORMAT);
            return  (PX_ERROR);
        }

        /*
         *  重定位目标节
         */
        pcTargetSect = (PCHAR)pmodule->EMOD_psegmentArry[pshdr->sh_info].ESEG_ulAddr;
        if (pcTargetSect == 0) {
            continue;                                                   /*  不是可加载节区              */
        }

        /*
         *  重定位所使用的符号表
         */
        psymTable = (Elf_Sym *)pmodule->EMOD_psegmentArry[pshdr->sh_link].ESEG_ulAddr;
        if (psymTable == 0) {
            _ErrorHandle(ERROR_LOADER_FORMAT);
            return  (PX_ERROR);
        }

        /*
         *  重定位符号表使用的字符串表
         */
        ulStrShIndex = (INT)pshdrArr[pshdr->sh_link].sh_link;
        pcStrTab    = (PCHAR)pmodule->EMOD_psegmentArry[ulStrShIndex].ESEG_ulAddr;
        if (LW_NULL == pcStrTab) {
            _ErrorHandle(ERROR_LOADER_FORMAT);
            return  (PX_ERROR);
        }

        ulRelocCount = (INT)(pshdr->sh_size / pshdr->sh_entsize);       /*  计算重定位表项数目          */
        switch (pshdr->sh_type) {

        case SHT_REL:                                                   /*  不带加数的重定位项          */
            iError = elfRelRelocate(pmodule,
                                    psymTable,
                                    (Elf_Rel *)(pmodule->EMOD_psegmentArry[i].ESEG_ulAddr),
                                    pcTargetSect,
                                    ulRelocCount,
                                    pcStrTab);
            break;

        case SHT_RELA:                                                  /*  带加数的重定位项            */
            iError = elfRelaRelocate(pmodule,
                                     psymTable,
                                     (Elf_Rela *)(pmodule->EMOD_psegmentArry[i].ESEG_ulAddr),
                                     pcTargetSect,
                                     ulRelocCount,
                                     pcStrTab);
            break;

        default:
            _ErrorHandle(ERROR_LOADER_FORMAT);
            iError = PX_ERROR;
            break;
        }

        if(iError < 0) {
            return  (iError);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: elfSymbolExport
** 功能描述: 查找模块初始化符号和入口符号，导出全局符号. (最后两个参数是为了提高内存效率)
** 输　入  : pmodule       模块指针
**           pcShName      节区名
**           pcSymName     符号名
**           psym          符号指针
**           addrSymVal    符号值
**           ulAllSymCnt   总符号数
**           ulCurSymNum   当前为第几个
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
** 注  意  : __sylixos_version 符号不管是否有导出节区限制, 都必须导出符号.
*********************************************************************************************************/
static INT elfSymbolExport (LW_LD_EXEC_MODULE *pmodule,
                            PCHAR              pcShName,
                            PCHAR              pcSymName,
                            Elf_Sym           *psym,
                            Elf_Addr           addrSymVal,
                            ULONG              ulAllSymCnt,
                            ULONG              ulCurSymNum)
{
    if ((LW_NULL != pmodule->EMOD_pcInit) &&
        (0 == lib_strcmp(pcSymName, pmodule->EMOD_pcInit))) {           /*  初始化函数                  */
        pmodule->EMOD_pfuncInit = (FUNCPTR)addrSymVal;
        LD_DEBUG_MSG(("init: %lx\r\n", addrSymVal));
        return  (ERROR_NONE);
    }

    if ((LW_NULL != pmodule->EMOD_pcExit) &&
        (0 == lib_strcmp(pcSymName, pmodule->EMOD_pcExit))) {           /*  销毁函数                    */
        pmodule->EMOD_pfuncExit = (FUNCPTR)addrSymVal;
        LD_DEBUG_MSG(("exit: %lx\r\n", addrSymVal));
        return  (ERROR_NONE);
    }

    if ((LW_NULL != pmodule->EMOD_pcEntry) &&
        (!pmodule->EMOD_bIsSymbolEntry) &&
        (0 == lib_strcmp(pcSymName, pmodule->EMOD_pcEntry))) {          /*  入口函数                    */
        pmodule->EMOD_pfuncEntry = (FUNCPTR)addrSymVal;
        pmodule->EMOD_bIsSymbolEntry = LW_TRUE;                         /*  通过符号匹配找到入口        */
        LD_DEBUG_MSG(("entry: %lx\r\n", addrSymVal));
        
#if __LW_ENTRY_SYMBOL == 0
        return  (ERROR_NONE);
#endif                                                                  /*  __LW_ENTRY_SYMBOL == 0      */
    }

    if (LW_FALSE == pmodule->EMOD_bExportSym) {
        return  (ERROR_NONE);
    }

#if !defined(LW_CFG_CPU_ARCH_C6X)
    if ((pcShName != LW_NULL) &&
        (pmodule->EMOD_pcSymSection != LW_NULL) &&
        (0 != lib_strcmp(pcShName, pmodule->EMOD_pcSymSection)) &&
        (0 != lib_strcmp(pcSymName, "__sylixos_version"))) {            /* 只导出指定节的符号           */
        return  (ERROR_NONE);
    }
#else
    if ((pcShName != LW_NULL) &&
        (pmodule->EMOD_pcSymSection != LW_NULL) &&
        (pcShName != lib_strstr(pcShName, pmodule->EMOD_pcSymSection)) &&
        (0 != lib_strcmp(pcSymName, "__sylixos_version"))) {            /* 只导出指定节的符号           */
        return  (ERROR_NONE);
    }
#endif

    switch (ELF_ST_TYPE(psym->st_info)) {

    case STT_FUNC:                                                      /*  导出函数                    */
        if (__moduleExportSymbol(pmodule, pcSymName, addrSymVal,
                                 LW_LD_SYM_FUNCTION,
                                 ulAllSymCnt, ulCurSymNum) < 0) {
            return  (PX_ERROR);
        }
        return  (ERROR_NONE);

    case STT_OBJECT:                                                    /*  导出变量                    */
#if defined(LW_CFG_CPU_ARCH_C6X)
    case STT_COMMON:
#endif
        if (__moduleExportSymbol(pmodule, pcSymName, addrSymVal,
                                 LW_LD_SYM_DATA,
                                 ulAllSymCnt, ulCurSymNum) < 0) {
            return  (PX_ERROR);
        }
        return  (ERROR_NONE);

    default:
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: elfSymbolsExport
** 功能描述: 处理可重定位文件的所有符号表.
** 输　入  : pmodule       模块指针
**           pshdrArr      节区头部表
**           uiShStrNdx    保存节区名的字符串节序号
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT elfSymbolsExport (LW_LD_EXEC_MODULE *pmodule, Elf_Shdr *pshdrArr, UINT uiShStrNdx)
{
    Elf_Shdr  *pshdr;
    Elf_Sym   *psymTable;
    PCHAR      pcStrTab;
    Elf_Sym   *psym;
    PCHAR      pcSymName;
    PCHAR      pcShName;

    Elf_Addr   addrSymVal;
    ULONG      ulSymCount;
    ULONG      ulStrShIndex;
    ULONG      i, j;

    LD_DEBUG_MSG(("exportSymbols()\r\n"));
    pshdr = pshdrArr;
    for (i = 0; i < pmodule->EMOD_ulSegCount; i++, pshdr++) {
        if (pshdr->sh_type != SHT_SYMTAB) {                             /*  只处理符号表节区            */
            continue;
        }

        /*
         *  获取符号表指针
         */
        psymTable = (Elf_Sym *)pmodule->EMOD_psegmentArry[i].ESEG_ulAddr;
        if (psymTable == 0) {
            continue;
        }

        /*
         *  获取与符号表对应的字符串表指针
         */
        ulStrShIndex = pshdr->sh_link;
        LD_DEBUG_MSG(("sybol table: %lx, string table: %lx\r\n", i, ulStrShIndex));
        pcStrTab = (PCHAR)pmodule->EMOD_psegmentArry[ulStrShIndex].ESEG_ulAddr;
        if (pcStrTab == 0) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "string table is NULL!\r\n");
            _ErrorHandle(ERROR_LOADER_FORMAT);
            return  (PX_ERROR);
        }

        ulSymCount = (ULONG)(pshdr->sh_size / sizeof(Elf_Sym));
        for (j = 0; j < ulSymCount; j++) {                              /*  遍历符号表                  */

            psym = &psymTable[j];
            pcSymName = pcStrTab + psym->st_name;
            if ((psym->st_shndx == SHN_UNDEF)                ||
#if !defined(LW_CFG_CPU_ARCH_C6X)
                (psym->st_other != STV_DEFAULT)              ||
#endif
                (ELF_ST_BIND(psym->st_info) == STB_LOCAL)    ||
                (psym->st_shndx >= pmodule->EMOD_ulSegCount) ||
                (0 == lib_strlen(pcSymName))) {
                continue;
            }

            addrSymVal = (Elf_Addr)pmodule->EMOD_psegmentArry[psym->st_shndx].ESEG_ulAddr
                       + psym->st_value;                                /*  获取符号的位置              */
            LD_DEBUG_MSG(("symbol: %s val: %lx\r\n", pcSymName, addrSymVal));

            pcShName = (PCHAR)pmodule->EMOD_psegmentArry[uiShStrNdx].ESEG_ulAddr
                     + pshdrArr[psym->st_shndx].sh_name;                /* 获取符号所在节名称           */

            if (elfSymbolExport(pmodule, pcShName, pcSymName, 
                                psym, addrSymVal, ulSymCount, j) < 0) {
                _ErrorHandle(ERROR_LOADER_EXPORT_SYM);
                return  (PX_ERROR);
            }
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: elfModuleMemoryInit
** 功能描述: 初始化可重定位文件模块内存.
** 输　入  : pmodule       模块指针
**           pshdrArr      节区头部表
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT elfModuleMemoryInit (LW_LD_EXEC_MODULE *pmodule, Elf_Shdr *pshdrArr)
{
    Elf_Shdr  *pshdr;
    Elf_Sym   *psymTable;
    Elf_Sym   *psym;

    size_t     stTotalSize;
    ULONG      ulSecAddr;
    ULONG      ulAlign;
    ULONG      ulMaxAlign = 0;
    ULONG      ulSymCount;
    ULONG      ulCommonSize  = 0;                                       /*  公共块大小                  */
    ULONG      ulExportCount = 0;                                       /*  导出符号数                  */
    ULONG      ulJmpItemCnt  = 0;
    ULONG      i, j;

    LD_DEBUG_MSG(("initModuleMemory()\r\n"));

    lib_bzero(&pshdrArr[pmodule->EMOD_ulSegCount], sizeof(Elf_Shdr) * 2);

    /*
     *  计算跳转表、BSS和导出符号表大小
     */
    pshdr = pshdrArr;
    for(i = 0; i < pmodule->EMOD_ulSegCount; i++, pshdr++) {
        if (pshdr->sh_type != SHT_SYMTAB) {                             /*  只需处理符号表              */
            continue;
        }

        psymTable = (Elf_Sym *)pmodule->EMOD_psegmentArry[i].ESEG_ulAddr;
        if (psymTable == 0) {
            continue;
        }
        
        ulSymCount = (INT)(pshdr->sh_size / sizeof(Elf_Sym));
        for (j = 0; j < ulSymCount; j++) {
            psym = &psymTable[j];

            if (SHN_UNDEF == psym->st_shndx) {                          /*  引用模块外的符号            */
                ulJmpItemCnt++;

            }else if(SHN_COMMON == psym->st_shndx) {                    /*  公共块                      */

                ulAlign = psym->st_value < sizeof(ULONG) ? sizeof(ULONG) : psym->st_value;
                ulCommonSize += ulAlign - 1;                            /*  st_value中保存了对齐值      */
                ulCommonSize = (ulCommonSize/ulAlign) * ulAlign;
                psym->st_shndx = (Elf32_Half)pmodule->EMOD_ulSegCount;
                psym->st_value = ulCommonSize;
                ulCommonSize += psym->st_size;
                if (ulAlign > ulMaxAlign) {
                    ulMaxAlign = ulAlign;
                }

            } else if (psym->st_shndx < pmodule->EMOD_ulSegCount) {

                if (SHT_NOBITS == pshdrArr[psym->st_shndx].sh_type &&   /*  计算BSS大小                 */
                    (psym->st_value + psym->st_size) >
                    pshdrArr[psym->st_shndx].sh_size) {
                    pshdrArr[psym->st_shndx].sh_size = psym->st_value
                                                     + psym->st_size;
                }

                if (((STT_FUNC  == ELF_ST_TYPE(psym->st_info)) ||       /*  计算导出符号表大小          */
                     (STT_OBJECT == ELF_ST_TYPE(psym->st_info))) &&
                    (STB_GLOBAL == ELF_ST_BIND(psym->st_info))) {
                    ulExportCount++;
                }
            }
        }
    }

    /*
     *  为导出符号表分配内存
     */
    if (pmodule->EMOD_bExportSym && ulExportCount > 0) {
        ULONG   ulHashBytes;
        pmodule->EMOD_ulSymHashSize = (ULONG)elfSymHashSize(ulExportCount);
        ulHashBytes = sizeof(LW_LIST_LINE_HEADER) * pmodule->EMOD_ulSymHashSize;
        pmodule->EMOD_psymbolHash = (LW_LIST_LINE_HEADER *)LW_LD_SAFEMALLOC(ulHashBytes);
        if (LW_NULL == pmodule->EMOD_psymbolHash) {
            pmodule->EMOD_ulSymHashSize = 0ul;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory!\r\n");
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        lib_bzero(pmodule->EMOD_psymbolHash, ulHashBytes);
        LD_DEBUG_MSG(("symbol table addr: %lx\r\n", (ULONG)pmodule->EMOD_psymbolHash));
    }

    /*
     *  构造Common节，保存Common变量.
     */
    if (ulCommonSize > 0) {
        pshdrArr[pmodule->EMOD_ulSegCount].sh_size      = ulCommonSize;
        pshdrArr[pmodule->EMOD_ulSegCount].sh_addralign = ulMaxAlign;
        pshdrArr[pmodule->EMOD_ulSegCount].sh_flags     = SHF_ALLOC;
        pshdrArr[pmodule->EMOD_ulSegCount].sh_type      = SHT_NOBITS;

        pmodule->EMOD_ulSegCount++;
    }

    /*
     *  构造一个节用于保存跳转表
     */
    pshdrArr[pmodule->EMOD_ulSegCount].sh_size  = ulJmpItemCnt
                                                * archElfRGetJmpBuffItemLen(pmodule);
    pshdrArr[pmodule->EMOD_ulSegCount].sh_flags = SHF_ALLOC;
    pshdrArr[pmodule->EMOD_ulSegCount].sh_type  = SHT_NOBITS;

    /*
     *  计算模块需要分配的内存大小
     */
    stTotalSize = 0;
    ulMaxAlign  = 0;
    pshdr = pshdrArr;
    for (i = 0; i <= pmodule->EMOD_ulSegCount; i++, pshdr++) {
        if (pshdr->sh_flags & SHF_ALLOC) {
            ulAlign = pshdr->sh_addralign > sizeof(ULONG) ? pshdr->sh_addralign : sizeof(ULONG);
            stTotalSize += ulAlign - 1;
            stTotalSize  = (stTotalSize / ulAlign) * ulAlign;
            pmodule->EMOD_psegmentArry[i].ESEG_ulAddr = stTotalSize;
            pmodule->EMOD_psegmentArry[i].ESEG_stLen = pshdr->sh_size;
            stTotalSize += pshdr->sh_size;
            if (ulAlign > ulMaxAlign) {
                ulMaxAlign = ulAlign;
            }
        }
    }
    stTotalSize += ulMaxAlign;

    pmodule->EMOD_pvBaseAddr = LW_LD_VMSAFEMALLOC(stTotalSize);         /*  分配内存                    */
    if (LW_NULL == pmodule->EMOD_pvBaseAddr) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "alloc vm-memory error!\r\n");
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    lib_bzero(pmodule->EMOD_pvBaseAddr, stTotalSize);

    pmodule->EMOD_stLen = stTotalSize;
    LD_DEBUG_MSG(("base addr: %lx\r\n", (addr_t)pmodule->EMOD_pvBaseAddr));

    /*
     *  设置每个节在内存中的位置
     */
    pshdr      = pshdrArr;
    ulSecAddr  = (ULONG)pmodule->EMOD_pvBaseAddr;
    ulSecAddr += ulMaxAlign - 1;
    ulSecAddr  = (ulSecAddr / ulMaxAlign) * ulMaxAlign;
    for (i = 0; i <= pmodule->EMOD_ulSegCount; i++, pshdr++) {
        if (pshdr->sh_flags & SHF_ALLOC) {
            pmodule->EMOD_psegmentArry[i].ESEG_ulAddr += ulSecAddr;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: elfBuildInitTbl
** 功能描述: 构建初始化函数和析构函数表
** 输　入  : pmodule       模块指针
**           pshdrArr      节区头部表
**           uiShStrNdx    保存节区名的字符串节序号
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT elfBuildInitTbl (LW_LD_EXEC_MODULE *pmodule, Elf_Shdr *pshdr, UINT uiShStrNdx)
{
    INT  i;
    INT  j;
    INT  k;

    UINT      uiInitTblSize = 0;
    UINT      uiFiniTblSize = 0;
    PCHAR     pcShName      = LW_NULL;
    Elf_Addr *paddr         = LW_NULL;

    /*
     *  构建init函数表，将init节中的项拷贝到init函数数组中
     */
    for (j = 0; j < (sizeof(_G_pcInitSecArr) / sizeof(_G_pcInitSecArr[0])); j++) {
        for (i = 0; i < pmodule->EMOD_ulSegCount; i++) {                /*  计算init表大小              */
            pcShName = (PCHAR)pmodule->EMOD_psegmentArry[uiShStrNdx].ESEG_ulAddr
                     + pshdr[i].sh_name;                                /*  获取符号所在节名称          */
            if (0 == lib_strcmp(pcShName, _G_pcInitSecArr[j])) {        /*  匹配.init_array节           */
                uiInitTblSize += (pshdr[i].sh_size / sizeof(Elf_Addr));
            }
        }
    }

    for (i = 0; i < pmodule->EMOD_ulSegCount; i++) {
        pcShName = (PCHAR)pmodule->EMOD_psegmentArry[uiShStrNdx].ESEG_ulAddr
                 + pshdr[i].sh_name;
        if (pcShName == lib_strstr(pcShName, __LW_CTORS_SECTION)) {     /*  匹配.ctor节                 */
            uiInitTblSize += (pshdr[i].sh_size / sizeof(Elf_Addr));
        }
    }

    if (uiInitTblSize == 0) {                                           /*  没有任何函数                */
        goto    __finibuild;
    }

    pmodule->EMOD_ppfuncInitArray = 
        (VOIDFUNCPTR *)LW_LD_SAFEMALLOC(uiInitTblSize * sizeof(VOIDFUNCPTR));
    if (!pmodule->EMOD_ppfuncInitArray) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory!\r\n");
        _ErrorHandle(ENOMEM);
        goto    __error;
    }

    pmodule->EMOD_ulInitArrCnt = uiInitTblSize;
    uiInitTblSize = 0;

    for (j = 0; j < (sizeof(_G_pcInitSecArr) / sizeof(_G_pcInitSecArr[0])); j++) {
        for (i = 0; i < pmodule->EMOD_ulSegCount; i++) {                /* 复制init表内容               */
            pcShName = (PCHAR)pmodule->EMOD_psegmentArry[uiShStrNdx].ESEG_ulAddr
                     + pshdr[i].sh_name;                                /* 获取符号所在节名称           */
            paddr = (Elf_Addr *)pmodule->EMOD_psegmentArry[i].ESEG_ulAddr;

            if (0 != lib_strcmp(pcShName, _G_pcInitSecArr[j])) {
                continue;
            }
            for (k = 0; k < (pshdr[i].sh_size / sizeof(Elf_Addr)); k++) {
                pmodule->EMOD_ppfuncInitArray[uiInitTblSize++] = (VOIDFUNCPTR)paddr[k];
            }
        }
    }

    for (i = 0; i < pmodule->EMOD_ulSegCount; i++) {
        pcShName = (PCHAR)pmodule->EMOD_psegmentArry[uiShStrNdx].ESEG_ulAddr
                 + pshdr[i].sh_name;
        paddr = (Elf_Addr *)pmodule->EMOD_psegmentArry[i].ESEG_ulAddr;
        if (pcShName == lib_strstr(pcShName, __LW_CTORS_SECTION)) {     /*  匹配.ctor节                 */
            for (k = 0; k < (pshdr[i].sh_size / sizeof(Elf_Addr)); k++) {
                pmodule->EMOD_ppfuncInitArray[uiInitTblSize++] = (VOIDFUNCPTR)paddr[k];
            }
        }
    }

__finibuild:
    /*
     *  构建Fini函数表，将Fini节中的项拷贝到Fini函数数组中
     */
    for (j = 0; j < (sizeof(_G_pcFiniSecArr) / sizeof(_G_pcFiniSecArr[0])); j++) {
        for (i = 0; i < pmodule->EMOD_ulSegCount; i++) {                /* 计算Fini表大小               */
            pcShName = (PCHAR)pmodule->EMOD_psegmentArry[uiShStrNdx].ESEG_ulAddr
                     + pshdr[i].sh_name;                                /* 获取符号所在节名称           */

            if (0 == lib_strcmp(pcShName, _G_pcFiniSecArr[j])) {        /* 匹配.fini_array节            */
                uiFiniTblSize += (pshdr[i].sh_size / sizeof(Elf_Addr));
            }
        }
    }

    for (i = 0; i < pmodule->EMOD_ulSegCount; i++) {
        pcShName = (PCHAR)pmodule->EMOD_psegmentArry[uiShStrNdx].ESEG_ulAddr
                 + pshdr[i].sh_name;
        if (pcShName == lib_strstr(pcShName, __LW_DTORS_SECTION)) {     /*  匹配.dtor节                 */
            uiFiniTblSize += (pshdr[i].sh_size / sizeof(Elf_Addr));
        }
    }

    if (uiFiniTblSize == 0) {                                           /*  没有析构函数                */
        goto    __out;
    }

    pmodule->EMOD_ppfuncFiniArray = 
        (VOIDFUNCPTR *)LW_LD_SAFEMALLOC(uiFiniTblSize * sizeof(VOIDFUNCPTR));
    if (!pmodule->EMOD_ppfuncFiniArray) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory!\r\n");
        _ErrorHandle(ENOMEM);
        goto    __error;
    }

    pmodule->EMOD_ulFiniArrCnt = uiFiniTblSize;
    uiFiniTblSize = 0;
    for (j = 0; j < (sizeof(_G_pcFiniSecArr) / sizeof(_G_pcFiniSecArr[0])); j++) {
        for (i = 0; i < pmodule->EMOD_ulSegCount; i++) {
            pcShName = (PCHAR)pmodule->EMOD_psegmentArry[uiShStrNdx].ESEG_ulAddr
                     + pshdr[i].sh_name;                                /* 获取符号所在节名称           */
            paddr = (Elf_Addr *)pmodule->EMOD_psegmentArry[i].ESEG_ulAddr;

            if (0 != lib_strcmp(pcShName, _G_pcFiniSecArr[j])) {
                continue;
            }
            for (k = 0; k < (pshdr[i].sh_size / sizeof(Elf_Addr)); k++) {
                pmodule->EMOD_ppfuncFiniArray[uiFiniTblSize++] = (VOIDFUNCPTR)paddr[k];
            }
        }
    }

    for (i = 0; i < pmodule->EMOD_ulSegCount; i++) {
        pcShName = (PCHAR)pmodule->EMOD_psegmentArry[uiShStrNdx].ESEG_ulAddr
                 + pshdr[i].sh_name;                                    /* 获取符号所在节名称           */
        paddr = (Elf_Addr *)pmodule->EMOD_psegmentArry[i].ESEG_ulAddr;
        if (pcShName == lib_strstr(pcShName, __LW_DTORS_SECTION)) {     /*  匹配.dtor节                 */
            for (k = 0; k < (pshdr[i].sh_size / sizeof(Elf_Addr)); k++) {
                pmodule->EMOD_ppfuncFiniArray[uiFiniTblSize++] = (VOIDFUNCPTR)paddr[k];
            }
        }
    }

__out:
    return  (ERROR_NONE);

__error:
    LW_LD_SAFEFREE(pmodule->EMOD_ppfuncInitArray);
    LW_LD_SAFEFREE(pmodule->EMOD_ppfuncFiniArray);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: elfLoadReloc
** 功能描述: 加载可重定位elf文件.
** 输　入  : pmodule       模块指针
**           pehdr         文件头
**           iFd           文件句柄
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT elfLoadReloc (LW_LD_EXEC_MODULE *pmodule, Elf_Ehdr *pehdr, INT  iFd)
{
    Elf_Shdr    *pshdr;
    PCHAR        pcBuf = LW_NULL;

    size_t       stShdrSize;
    ULONG        i;
    INT          iError = PX_ERROR;

    if (LW_NULL != pmodule->EMOD_psegmentArry ||
        LW_NULL != pmodule->EMOD_psymbolHash  ||
        LW_NULL != pmodule->EMOD_pvBaseAddr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pmodule->EMOD_ulModType = LW_LD_MOD_TYPE_KO;

    /*
     *  读取节区头部表
     */
    stShdrSize = pehdr->e_shentsize * pehdr->e_shnum;
    pcBuf = (PCHAR)LW_LD_SAFEMALLOC((stShdrSize +
                                    pehdr->e_shentsize * 2));           /*  可能存在公共块和长跳转表    */
    if (LW_NULL == pcBuf) {
        _ErrorHandle(ENOMEM);
        goto    __out0;
    }

    if (lseek(iFd, pehdr->e_shoff, SEEK_SET) < 0) {
        goto    __out0;
    }

    if (read(iFd, pcBuf, stShdrSize) < stShdrSize) {
        goto    __out0;
    }

    pmodule->EMOD_psegmentArry = (LW_LD_EXEC_SEGMENT *)LW_LD_SAFEMALLOC(
                                 (sizeof(LW_LD_EXEC_SEGMENT) * (pehdr->e_shnum + 2)));
    if (!pmodule->EMOD_psegmentArry) {
        _ErrorHandle(ENOMEM);
        goto    __out0;
    }
    
    pmodule->EMOD_ulSegCount = pehdr->e_shnum;
    lib_bzero(pmodule->EMOD_psegmentArry,
              sizeof(LW_LD_EXEC_SEGMENT) * (pehdr->e_shnum + 2));

    /*
     *  读入符号表， 重定位节，字符串表
     */
    pshdr = (Elf_Shdr *)pcBuf;

    for (i = 0; i < pehdr->e_shnum; i++, pshdr++) {
        if ((pshdr->sh_type == SHT_SYMTAB) ||
            (pshdr->sh_type == SHT_STRTAB) ||
            (pshdr->sh_type == SHT_RELA)   ||
            (pshdr->sh_type == SHT_REL)) {

            if (pshdr->sh_size <= 0) {
                continue;
            }
            LD_DEBUG_MSG(("load section %x, %x, %x\r\n",
                    pshdr->sh_type,
                    pshdr->sh_offset,
                    pshdr->sh_size));

            pmodule->EMOD_psegmentArry[i].ESEG_ulAddr =
                     (addr_t)LW_LD_VMSAFEMALLOC((size_t)pshdr->sh_size);
            pmodule->EMOD_psegmentArry[i].ESEG_stLen = pshdr->sh_size;
            if (0 == pmodule->EMOD_psegmentArry[i].ESEG_ulAddr) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "vmm low memory!\r\n");
                _ErrorHandle(ENOMEM);
                goto    __out1;
            }

            if (lseek(iFd, pshdr->sh_offset, SEEK_SET) < 0) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "seek file error!\r\n");
                goto    __out1;
            }

            if (read(iFd, (PVOID)pmodule->EMOD_psegmentArry[i].ESEG_ulAddr,
                     pshdr->sh_size) < pshdr->sh_size) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "read file error!\r\n");
                goto    __out1;
            }
        }
    }

    if (elfModuleMemoryInit(pmodule, (Elf_Shdr *)pcBuf) < 0) {
        goto    __out1;
    }

    /*
     *  读取程序数据段、代码段
     */
    pshdr = (Elf_Shdr *)pcBuf;
    for (i = 0; i < pehdr->e_shnum; i++, pshdr++) {
        if ((pshdr->sh_flags & SHF_ALLOC)  &&
            (pshdr->sh_type != SHT_NOBITS) &&
            (pshdr->sh_size > 0)) {

            if (lseek(iFd, pshdr->sh_offset, SEEK_SET) < 0) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "seek file error!\r\n");
                goto    __out2;
            }

            if (read(iFd, (PVOID)pmodule->EMOD_psegmentArry[i].ESEG_ulAddr,
                     pshdr->sh_size) < pshdr->sh_size) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "read file error!\r\n");
                goto    __out2;
            }
            
#if (LW_CFG_TRUSTED_COMPUTING_EN > 0) || defined(LW_CFG_CPU_ARCH_C6X)   /*  C6X 使用SEGMENT实现backtrace*/
            pmodule->EMOD_psegmentArry[i].ESEG_bCanExec = (pshdr->sh_flags & SHF_EXECINSTR) ?
                                                          LW_TRUE : LW_FALSE;
#endif                                                                  /*  LW_CFG_TRUSTED_COMPUTING_EN */
        }
    }

    if (elfSectionsRelocate(pmodule, (Elf_Shdr *)pcBuf) < 0) {          /*  重定位                      */
        _DebugHandle(__ERRORMESSAGE_LEVEL, ("relocation failed\r\n"));
        _ErrorHandle(ERROR_LOADER_RELOCATE);
        goto    __out2;
    }

    if (elfSymbolsExport(pmodule, (Elf_Shdr *)pcBuf, pehdr->e_shstrndx)) {
                                                                        /*  导出符号表，查找初始化函数  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, ("export symbol failed\r\n"));
        _ErrorHandle(ERROR_LOADER_EXPORT_SYM);
        goto    __out2;
    }

    if (elfBuildInitTbl(pmodule, (Elf_Shdr *)pcBuf, pehdr->e_shstrndx)) {/* 构建初始化函数表            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, ("build init & fini table failed\r\n"));
        _ErrorHandle(ERROR_LOADER_EXPORT_SYM);
    }

    iError = ERROR_NONE;
    goto    __out1;

__out2:
    LW_LD_VMSAFEFREE(pmodule->EMOD_pvBaseAddr);
    __moduleDeleteAllSymbol(pmodule);
    LW_LD_SAFEFREE(pmodule->EMOD_psymbolHash);
    LW_LD_SAFEFREE(pmodule->EMOD_ppfuncInitArray);
    LW_LD_SAFEFREE(pmodule->EMOD_ppfuncFiniArray);

__out1:
    pshdr = (Elf_Shdr *)pcBuf;
    for (i = 0; i < pehdr->e_shnum; i++, pshdr++) {
        if(!(pshdr->sh_flags & SHF_ALLOC)) {
            if (pmodule->EMOD_psegmentArry[i].ESEG_ulAddr) {
                LW_LD_VMSAFEFREE(pmodule->EMOD_psegmentArry[i].ESEG_ulAddr);
            }
        }
    }
    
#if (LW_CFG_HWDBG_GDBMOD_EN == 0) && \
    (LW_CFG_TRUSTED_COMPUTING_EN == 0) && \
     !defined(LW_CFG_CPU_ARCH_C6X)                                      /*  硬件调试器与可信计算需要    */
    LW_LD_SAFEFREE(pmodule->EMOD_psegmentArry);
    pmodule->EMOD_ulSegCount = 0;
#endif                                                                  /*  !LW_CFG_HWDBG_GDBMOD_EN     */
                                                                        /*  !LW_CFG_TRUSTED_COMPUTING_EN*/
__out0:
    LW_LD_SAFEFREE(pcBuf);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: dynPhdrParse
** 功能描述: 解析dynamic段
** 输　入  : pmodule       模块指针
**           pdyndir       dynamic数据结构
**           pphdr         段表
**           iPhdrCnt      段数目
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT dynPhdrParse (LW_LD_EXEC_MODULE *pmodule,
                         ELF_DYN_DIR       *pdyndir,
                         Elf_Phdr          *pphdr,
                         INT                iPhdrCnt)
{
    Elf_Addr     addrMin     = pdyndir->addrMin;
    Elf_Dyn     *pdyn        = LW_NULL;                                 /*  dynamic段指针               */
    ULONG        ulItemCount = 0;                                       /*  dynamic段条目数             */

    INT          i, j;

    /*
     *  解析dynamic段
     */
    for (i = 0; i < iPhdrCnt; i++, pphdr++) {
        if (PT_DYNAMIC == pphdr->p_type) {                              /*  找到dynamic段               */
            pdyn = (Elf_Dyn *)LW_LD_V2PADDR(addrMin,
                                            pmodule->EMOD_pvBaseAddr,
                                            pphdr->p_vaddr);
            ulItemCount = pphdr->p_filesz / sizeof(Elf_Dyn);            /*  动态结构项数                */

            for (j = 0; j < ulItemCount; j++, pdyn++) {
                switch (pdyn->d_tag) {

                case DT_NEEDED:
                    if (pdyndir->ulNeededCnt >= __LW_MAX_NEEDED_LIB) {
                        _DebugHandle(__ERRORMESSAGE_LEVEL, "too many needed librarys.\r\n");
                        _ErrorHandle(ERROR_LOADER_FORMAT);
                        return  (PX_ERROR);
                    }
                    pdyndir->wdNeededArr[pdyndir->ulNeededCnt++] = pdyn->d_un.d_val;
                    break;

                case DT_SYMTAB:                                         /*  符号表                      */
                    pdyndir->psymTable   = (Elf_Sym *)LW_LD_V2PADDR(addrMin,
                                                           pmodule->EMOD_pvBaseAddr,
                                                           pdyn->d_un.d_ptr);
                    break;

                case DT_STRTAB:                                         /*  字符串表                    */
                    pdyndir->pcStrTable  = (PCHAR)LW_LD_V2PADDR(addrMin,
                                                       pmodule->EMOD_pvBaseAddr,
                                                       pdyn->d_un.d_ptr);
                    break;

                case DT_REL:                                            /*  REL重定位表                 */
                    pdyndir->prelTable   = (Elf_Rel *)LW_LD_V2PADDR(addrMin,
                                                           pmodule->EMOD_pvBaseAddr,
                                                           pdyn->d_un.d_ptr);
                    break;

                case DT_RELSZ:                                          /*  REL重定位表大小             */
                    pdyndir->ulRelSize   = pdyn->d_un.d_val;
                    break;

                case DT_RELA:                                           /*  RELA重定位表                */
                    pdyndir->prelaTable  = (Elf_Rela *)LW_LD_V2PADDR(addrMin,
                                                            pmodule->EMOD_pvBaseAddr,
                                                            pdyn->d_un.d_ptr);
                    break;

                case DT_RELASZ:                                         /*  RELA重定位表大小            */
                    pdyndir->ulRelaSize  = pdyn->d_un.d_val;
                    break;

                case DT_HASH:                                           /*  HASH表                      */
                    pdyndir->phash       = (Elf_Hash *)LW_LD_V2PADDR(addrMin,
                                                            pmodule->EMOD_pvBaseAddr,
                                                            pdyn->d_un.d_ptr);
                    break;

                case DT_PLTREL:                                         /*  PLT重定位表类型             */
                    pdyndir->ulPltRel    = pdyn->d_un.d_val;
                    break;

                case DT_JMPREL:                                         /*  PLT重定位表                 */
                    pdyndir->pvJmpRTable = (PCHAR)LW_LD_V2PADDR(addrMin,
                                                       pmodule->EMOD_pvBaseAddr,
                                                       pdyn->d_un.d_ptr);
                    break;

                case DT_PLTRELSZ:                                       /*  PLT重定位表大小             */
                    pdyndir->ulJmpRSize  = pdyn->d_un.d_val;
                    break;

                case DT_INIT_ARRAY:
                    pdyndir->paddrInitArray = (Elf_Addr *)LW_LD_V2PADDR(addrMin,
                                                       pmodule->EMOD_pvBaseAddr,
                                                       pdyn->d_un.d_ptr);
                    break;

                case DT_INIT_ARRAYSZ:
                    pdyndir->ulInitArrSize  = pdyn->d_un.d_val;
                    break;

                case DT_FINI_ARRAY:
                    pdyndir->paddrFiniArray = (Elf_Addr *)LW_LD_V2PADDR(addrMin,
                                                       pmodule->EMOD_pvBaseAddr,
                                                       pdyn->d_un.d_ptr);
                    break;

                case DT_FINI_ARRAYSZ:
                    pdyndir->ulFiniArrSize = pdyn->d_un.d_val;
                    break;

                case DT_INIT:
                    pdyndir->addrInit = (Elf_Addr)LW_LD_V2PADDR(addrMin,
                                                        pmodule->EMOD_pvBaseAddr,
                                                        pdyn->d_un.d_val);
                    break;

                case DT_FINI:
                    pdyndir->addrFini = (Elf_Addr)LW_LD_V2PADDR(addrMin,
                                                        pmodule->EMOD_pvBaseAddr,
                                                        pdyn->d_un.d_val);
                    break;
                    
                case DT_PLTGOT:
                    pdyndir->ulPltGotAddr  = (Elf_Addr *)LW_LD_V2PADDR(addrMin,
                                                         pmodule->EMOD_pvBaseAddr,
                                                         pdyn->d_un.d_ptr);
                    break;
                    
#ifdef  LW_CFG_CPU_ARCH_MIPS
                case DT_MIPS_GOTSYM:
                    pdyndir->ulMIPSGotSymIdx      = pdyn->d_un.d_val;
                    break;
                
                case DT_MIPS_LOCAL_GOTNO:
                    pdyndir->ulMIPSLocalGotNumIdx = pdyn->d_un.d_val;
                    break;
                
                case DT_MIPS_SYMTABNO:
                    pdyndir->ulMIPSSymNumIdx      = pdyn->d_un.d_val;
                    break;
                
                case DT_MIPS_PLTGOT:
                    pdyndir->ulMIPSPltGotIdx      = pdyn->d_un.d_val;
                    break;
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS        */

#if defined(LW_CFG_CPU_ARCH_C6X)
                case DT_C6000_DSBT_BASE:
                    pmodule->EMOD_pulDsbtTable = (Elf_Addr *)LW_LD_V2PADDR(addrMin,
                                                 pmodule->EMOD_pvBaseAddr,
                                                 pdyn->d_un.d_ptr);

                case DT_C6000_DSBT_SIZE:
                    pmodule->EMOD_ulDsbtSize   = pdyn->d_un.d_val;

                case DT_C6000_DSBT_INDEX:
                    pmodule->EMOD_ulDsbtIndex  = pdyn->d_un.d_val;
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */
                }
            }
            break;
        }
    }

    if (i == iPhdrCnt) {                                                /*  找不到.dynamic段            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not find dynamic header.\r\n");
        _ErrorHandle(ERROR_LOADER_FORMAT);
        return  (PX_ERROR);
    }

    pdyndir->ulRelCount  = pdyndir->ulRelSize / sizeof(Elf_Rel);        /*  REL重定位表项数             */
    pdyndir->ulRelaCount = pdyndir->ulRelaSize / sizeof(Elf_Rela);      /*  RELA重定位表项数            */
    pdyndir->ulSymCount  = pdyndir->phash->nchain;                      /*  符号表项数                  */

    /*
     *  TODO: 目前认为JMPREL跟在REL或RELA表之后，如果没有找到REL表和RELA表，则使用JMPREL表。
     */
#if !defined(LW_CFG_CPU_ARCH_PPC) && !defined(LW_CFG_CPU_ARCH_C6X) && !defined(LW_CFG_CPU_ARCH_SPARC)
    if (pdyndir->pvJmpRTable) {
        if (DT_REL == pdyndir->ulPltRel) {
            if (!pdyndir->prelTable) {
                pdyndir->prelTable = (Elf_Rel*)(pdyndir->pvJmpRTable);
            }
            pdyndir->ulRelCount += pdyndir->ulJmpRSize/sizeof(Elf_Rel);

        } else if (DT_RELA == pdyndir->ulPltRel) {
            if (!pdyndir->prelaTable) {
                pdyndir->prelaTable = (Elf_Rela*)pdyndir->pvJmpRTable;
            }
            pdyndir->ulRelaCount += pdyndir->ulJmpRSize/sizeof(Elf_Rela);
        }
    }
#endif

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: elfPhdrRead
** 功能描述: 读取并解析elf文件
** 输　入  : pmodule       模块指针
**           pdyndir       dynamic数据结构
**           pehdr         elf文件头
**           iFd           elf文件句柄
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT elfPhdrRead (LW_LD_EXEC_MODULE *pmodule,
                        ELF_DYN_DIR       *pdyndir,
                        Elf_Ehdr          *pehdr,
                        INT                iFd)
{
    INT             iError = PX_ERROR;

    PCHAR           pcBuf  = LW_NULL;
    Elf_Phdr       *pphdr;

    size_t          stShdrSize;
    ULONG           i;
    Elf_Word        dwAlign  = 0;
    Elf_Word        dwMapOff = 0;
    Elf_Addr        addrMin  = (Elf_Addr)~0;
    Elf_Addr        addrMax  = 0x0;
    
    BOOL            bCanShare;
    BOOL            bCanExec;
    struct stat64   stat64Buf;

    /*
     *  读取段头部表
     */
    stShdrSize = pehdr->e_phentsize * pehdr->e_phnum;
    pcBuf = (PCHAR)LW_LD_SAFEMALLOC((stShdrSize + pehdr->e_shentsize));

    if (LW_NULL == pcBuf) {
        _ErrorHandle(ENOMEM);
        goto    __out0;
    }

    if (lseek(iFd, pehdr->e_phoff, SEEK_SET) < 0) {
        goto    __out0;
    }

    if (read(iFd, pcBuf, stShdrSize) < stShdrSize) {
        goto    __out0;
    }
    
    if (fstat64(iFd, &stat64Buf)) {
        goto    __out0;
    }

    pphdr = (Elf_Phdr *)pcBuf;
    for (i = 0; i < pehdr->e_phnum; i++, pphdr++) {
        if (PT_LOAD != pphdr->p_type) {                                 /*  只处理可加载段              */
            continue;
        }

        if (pphdr->p_vaddr < addrMin) {
            addrMin = pphdr->p_vaddr;
        }

        if (pphdr->p_vaddr + pphdr->p_memsz > addrMax) {
            addrMax = pphdr->p_vaddr + pphdr->p_memsz;
        }

        if (pphdr->p_align > dwAlign) {
            dwAlign = pphdr->p_align;
        }
    }

    if(addrMin >= addrMax) {                                            /*  没有可加载的段              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no segment can be loaded.\r\n");
        _ErrorHandle(ERROR_LOADER_FORMAT);
        goto    __out0;
    }

    /*
     *  TODO: 目前尚未处理长跳转表，根据现在分析的结果，动态链接的可执行文件中自带跳转表(PLT)
     */
    pmodule->EMOD_stLen      = (size_t)(addrMax - addrMin);
    pmodule->EMOD_pvBaseAddr = LW_LD_VMSAFEMALLOC_AREA_ALIGN(pmodule->EMOD_stLen,
                                                             (size_t)dwAlign);
    if(!pmodule->EMOD_pvBaseAddr) {                                     /*  为可执行文件分配内存空间    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "vmm low memory!\r\n");
        _ErrorHandle(ENOMEM);
        goto    __out0;
    }

    /*
     *  分段映射可加载段
     */
#if (LW_CFG_TRUSTED_COMPUTING_EN > 0)  || defined(LW_CFG_CPU_ARCH_C6X)  /*  动态度量需要确定内存结构    */
    pmodule->EMOD_ulSegCount   = pehdr->e_phnum;
    pmodule->EMOD_psegmentArry = (LW_LD_EXEC_SEGMENT *)LW_LD_SAFEMALLOC(
                                 (sizeof(LW_LD_EXEC_SEGMENT) * pmodule->EMOD_ulSegCount));
    if (!pmodule->EMOD_psegmentArry) {
        pmodule->EMOD_ulSegCount = 0ul;
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory!\r\n");
        _ErrorHandle(ENOMEM);
        goto    __out0;
    }
    lib_bzero(pmodule->EMOD_psegmentArry,
              sizeof(LW_LD_EXEC_SEGMENT) * pmodule->EMOD_ulSegCount);
#endif                                                                  /*  LW_CFG_TRUSTED_COMPUTING_EN */
     
    pphdr = (Elf_Phdr *)pcBuf;
    for (i = 0; i < pehdr->e_phnum; i++, pphdr++) {
        
#ifdef LW_CFG_CPU_ARCH_ARM
        if (pphdr->p_type == PT_ARM_EXIDX) {
            pmodule->EMOD_pvARMExidx = (PVOID)LW_LD_V2PADDR(addrMin,
                                       pmodule->EMOD_pvBaseAddr, pphdr->p_vaddr);
            pmodule->EMOD_stARMExidxCount = pphdr->p_filesz / 8;
        }
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */
        if ((PT_LOAD != pphdr->p_type) || (pphdr->p_filesz == 0)) {
            continue;
        }

        dwMapOff  = pphdr->p_vaddr - addrMin;
        bCanShare = PF_W & pphdr->p_flags ? LW_FALSE : LW_TRUE;         /*  是否可共享                  */
        bCanExec  = PF_X & pphdr->p_flags ? LW_TRUE  : LW_FALSE;        /*  是否可执行                  */

        if (LW_LD_VMSAFEMAP_AREA(pmodule->EMOD_pvBaseAddr, dwMapOff, iFd, &stat64Buf,
                                 pphdr->p_offset, pphdr->p_filesz, 
                                 bCanShare, bCanExec) != ERROR_NONE) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "mmap error!\r\n");
            goto    __out1;
        }
        
#if (LW_CFG_TRUSTED_COMPUTING_EN > 0) || defined(LW_CFG_CPU_ARCH_C6X)   /*  动态度量需要确定内存结构    */
        pmodule->EMOD_psegmentArry[i].ESEG_ulAddr   = (addr_t)pmodule->EMOD_pvBaseAddr
                                                    + dwMapOff;
        pmodule->EMOD_psegmentArry[i].ESEG_stLen    = pphdr->p_filesz;
        pmodule->EMOD_psegmentArry[i].ESEG_bCanExec = bCanExec;
#endif                                                                  /* LW_CFG_TRUSTED_COMPUTING_EN  */

        if (pphdr->p_memsz > pphdr->p_filesz) {                         /* 所需内存空间大于文件数据长度 */
            dwMapOff += pphdr->p_filesz;
            
            if (LW_LD_VMSAFEMAP_AREA(pmodule->EMOD_pvBaseAddr, dwMapOff, PX_ERROR, &stat64Buf,
                                     0, (pphdr->p_memsz - pphdr->p_filesz), 
                                     bCanShare, bCanExec)  != ERROR_NONE) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "mmap error!\r\n");
                goto    __out1;
            }
        }
    }
    
    LW_LD_VMSAFE_SHARE(pmodule->EMOD_pvBaseAddr, pmodule->EMOD_stLen, 
                       stat64Buf.st_dev, stat64Buf.st_ino);             /*  启用共享功能                */
    
    /*
     *   解析dynamic段
     */
    pdyndir->addrMin = addrMin;
    pdyndir->addrMax = addrMax;
    if (dynPhdrParse(pmodule, pdyndir, (Elf_Phdr *)pcBuf, pehdr->e_phnum)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "seek file error!\r\n");
        _ErrorHandle(ERROR_LOADER_FORMAT);
        goto    __out1;
    }

    iError = ERROR_NONE;
    goto    __out0;

__out1:
    LW_LD_VMSAFEFREE_AREA(pmodule->EMOD_pvBaseAddr);

__out0:
    LW_LD_SAFEFREE(pcBuf);

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: elfPhdrRelocate
** 功能描述: 重定位
** 输　入  : pmodule       模块指针
**           pdyndir       dynamic数据结构
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT elfPhdrRelocate (LW_LD_EXEC_MODULE *pmodule, ELF_DYN_DIR  *pdyndir)
{
    Elf_Addr     addrMin   = pdyndir->addrMin;
    Elf_Rel     *prel      = LW_NULL;
    Elf_Rela    *prela     = LW_NULL;
    PCHAR        pcSymName = LW_NULL;
    Elf_Sym     *psym      = LW_NULL;
    Elf_Addr     addrSymVal;

    PCHAR        pcBase    = LW_NULL;
    INT          i;

    pcBase = (PCHAR)LW_LD_V2PADDR(addrMin,
                                  pmodule->EMOD_pvBaseAddr,
                                  0);                                   /*  elf段0地址的加载地址        */

#if defined(LW_CFG_CPU_ARCH_PPC) || defined(LW_CFG_CPU_ARCH_MIPS) || defined(LW_CFG_CPU_ARCH_C6X)
    if (archElfGotInit(pmodule) < 0) {
        return  (PX_ERROR);
    }
#endif

    /*
     *  重定位
     */
    if (pdyndir->prelTable) {                                           /*  REL重定位结构               */
        for (i = 0; i < pdyndir->ulRelCount; i++) {
            prel = &pdyndir->prelTable[i];
            psym = &pdyndir->psymTable[ELF_R_SYM(prel->r_info)];
            pcSymName = pdyndir->pcStrTable + psym->st_name;

            addrSymVal = 0;
            if (SHN_UNDEF == psym->st_shndx) {                          /*  外部符号需查找符号表        */
                if (lib_strlen(pcSymName) == 0) {                       /* 特殊符号，和体系结构相关     */
                    addrSymVal = 0;
                } else {
                    BOOL    bWeak = (STB_WEAK == ELF_ST_BIND(psym->st_info));
                    if (__moduleSymGetValue(pmodule,
                                            bWeak,
                                            pcSymName,
                                            &addrSymVal,
                                            LW_LD_SYM_ANY) < 0) {       /*  查询对应符号的地址          */
                        _ErrorHandle(ERROR_LOADER_NO_SYMBOL);
                        return  (PX_ERROR);
                    }
                }
                
            } else {                                                    /*  模块内部符号                */
                addrSymVal = LW_LD_V2PADDR(addrMin,
                                           pmodule->EMOD_pvBaseAddr,
                                           psym->st_value);
            }

            if (archElfRelocateRel(pmodule,                             /*  重定位符号                  */
                                   prel,                                
                                   addrSymVal,
                                   pcBase,
                                   LW_NULL, 0) < 0) {
                _ErrorHandle(ERROR_LOADER_RELOCATE);
                return  (PX_ERROR);
            }
        }
    } else if (pdyndir->prelaTable) {                                   /*  RELA重定位结构              */
        for (i = 0; i < pdyndir->ulRelaCount; i++) {
            prela = &pdyndir->prelaTable[i];
            psym = &pdyndir->psymTable[ELF_R_SYM(prela->r_info)];
            pcSymName = pdyndir->pcStrTable + psym->st_name;

            addrSymVal = 0;
            if (SHN_UNDEF == psym->st_shndx) {                          /*  外部符号需查找符号表        */
                if (lib_strlen(pcSymName) == 0) {                       /* 特殊符号，和体系结构相关     */
                    addrSymVal = 0;
                } else {
                    BOOL    bWeak = (STB_WEAK == ELF_ST_BIND(psym->st_info));
                    if (__moduleSymGetValue(pmodule,
                                            bWeak,
                                            pcSymName,
                                            &addrSymVal,
                                            LW_LD_SYM_ANY) < 0) {       /*  查询对应符号的地址          */
                        _ErrorHandle(ERROR_LOADER_NO_SYMBOL);
                        return  (PX_ERROR);
                    }
                }
                
            } else {                                                    /*  模块内部符号                */
                addrSymVal = LW_LD_V2PADDR(addrMin,
                                           pmodule->EMOD_pvBaseAddr,
                                           psym->st_value);
            }

#if !defined(LW_CFG_CPU_ARCH_C6X) && !defined(LW_CFG_CPU_ARCH_SPARC)
            if (archElfRelocateRela(pmodule,                            /*  重定位符号                  */
                                    prela,
                                    addrSymVal,
                                    pcBase,
                                    LW_NULL, 0) < 0) {
                _ErrorHandle(ERROR_LOADER_RELOCATE);
                return  (PX_ERROR);
            }
#else
            if (archElfRelocateRela(pmodule,                            /*  重定位符号                  */
                                    prela,
                                    psym,
                                    addrSymVal,
                                    pcBase,
                                    LW_NULL, 0) < 0) {
                _ErrorHandle(ERROR_LOADER_RELOCATE);
                return  (PX_ERROR);
            }
#endif
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: elfPhdrSymExport
** 功能描述: 导出符号
** 输　入  : pmodule       模块指针
**           pdyndir       dynamic数据结构
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT elfPhdrSymExport (LW_LD_EXEC_MODULE *pmodule, ELF_DYN_DIR  *pdyndir)
{
    Elf_Addr     addrMin   = pdyndir->addrMin;
    PCHAR        pcSymName = LW_NULL;
    Elf_Sym     *psym      = LW_NULL;

    Elf_Addr     addrSymVal;
    INT          i;

    if (pmodule->EMOD_bExportSym && pdyndir->ulSymCount) {              /*  为导出符号表分配内存        */
        ULONG   ulHashBytes;
        pmodule->EMOD_ulSymHashSize = (ULONG)elfSymHashSize(pdyndir->ulSymCount);
        ulHashBytes = sizeof(LW_LIST_LINE_HEADER) * pmodule->EMOD_ulSymHashSize;
        pmodule->EMOD_psymbolHash = (LW_LIST_LINE_HEADER *)LW_LD_SAFEMALLOC(ulHashBytes);
        if (LW_NULL == pmodule->EMOD_psymbolHash) {
            pmodule->EMOD_ulSymHashSize = 0ul;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory!\r\n");
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        lib_bzero(pmodule->EMOD_psymbolHash, ulHashBytes);
        LD_DEBUG_MSG(("symbol table addr: %lx\r\n", (addr_t)pmodule->EMOD_psymbolHash));
    }

    for (i = 0; i < pdyndir->ulSymCount; i++) {                         /*  处理符号表                  */
        psym = &pdyndir->psymTable[i];
        pcSymName = pdyndir->pcStrTable + psym->st_name;
        if (psym->st_shndx               == SHN_UNDEF ||
            ELF_ST_BIND(psym->st_info)   == STB_LOCAL ||
            0 == lib_strlen(pcSymName)) {
            continue;
        }

        addrSymVal = LW_LD_V2PADDR(addrMin,
                                   pmodule->EMOD_pvBaseAddr,
                                   psym->st_value);                     /*  获取符号的位置              */
        LD_DEBUG_MSG(("symbol: %s val: %lx\r\n", pcSymName, addrSymVal));

        if (elfSymbolExport(pmodule, LW_NULL, pcSymName, 
                            psym, addrSymVal, pdyndir->ulSymCount, i) < 0) {
                                                                        /*  导出符号表，初始化入口函数  */
            _ErrorHandle(ERROR_LOADER_EXPORT_SYM);
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: elfPhdrBuildInitTable
** 功能描述: 创建 dymanic 初始化表
** 输　入  : pmodule       模块指针
**           pdyndir       dynamic数据结构
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT elfPhdrBuildInitTable (LW_LD_EXEC_MODULE *pmodule,
                                  ELF_DYN_DIR       *pdyndir)
{
    INT i;

    UINT      uiInitTblSize = (UINT)(pdyndir->ulInitArrSize / sizeof(Elf_Addr));
    UINT      uiFiniTblSize = (UINT)(pdyndir->ulFiniArrSize / sizeof(Elf_Addr));

    if ((uiInitTblSize > 0) && (LW_NULL != pdyndir->paddrInitArray)) {
        pmodule->EMOD_ppfuncInitArray = 
            (VOIDFUNCPTR *)LW_LD_SAFEMALLOC(uiInitTblSize * sizeof(VOIDFUNCPTR));
        if (!pmodule->EMOD_ppfuncInitArray) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory!\r\n");
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }

        pmodule->EMOD_ulInitArrCnt = uiInitTblSize;

        for (i = 0; i < uiInitTblSize; i++) {
            pmodule->EMOD_ppfuncInitArray[i] = (VOIDFUNCPTR)pdyndir->paddrInitArray[i];
        }
    }

    if (pdyndir->addrInit != 0) {
        pmodule->EMOD_pfuncInit = (FUNCPTR)pdyndir->addrInit;
    }

    if ((uiFiniTblSize > 0) && (LW_NULL != pdyndir->paddrFiniArray)) {
        pmodule->EMOD_ppfuncFiniArray = 
            (VOIDFUNCPTR *)LW_LD_SAFEMALLOC(uiFiniTblSize * sizeof(VOIDFUNCPTR));
        if (!pmodule->EMOD_ppfuncFiniArray) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory!\r\n");
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }

        pmodule->EMOD_ulFiniArrCnt = uiFiniTblSize;

        for (i = 0; i < uiFiniTblSize; i++) {
            pmodule->EMOD_ppfuncFiniArray[i] = (VOIDFUNCPTR)pdyndir->paddrFiniArray[i];
        }
    }

    if (pdyndir->addrFini != 0) {
        pmodule->EMOD_pfuncExit = (FUNCPTR)pdyndir->addrFini;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: elfLoadExec
** 功能描述: 加载可执行elf文件.
** 输　入  : pmodule       模块指针
**           pehdr         文件头
**           iFd           文件句柄
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT elfLoadExec (LW_LD_EXEC_MODULE *pmodule, Elf_Ehdr *pehdr, INT iFd)
{
    ELF_DYN_DIR *pdyndir     = NULL;
    INT          i;
    PCHAR        pchLibName  = NULL;

    LW_LD_EXEC_MODULE **ppmodUseArr = NULL;

    if (LW_NULL != pmodule->EMOD_psegmentArry ||
        LW_NULL != pmodule->EMOD_psymbolHash  ||
        LW_NULL != pmodule->EMOD_pvBaseAddr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pmodule->EMOD_ulModType = LW_LD_MOD_TYPE_SO;

    pdyndir = (ELF_DYN_DIR *)LW_LD_SAFEMALLOC(sizeof(ELF_DYN_DIR));
    if (!pdyndir) {
        goto __out;
    }

    lib_bzero(pdyndir, sizeof(ELF_DYN_DIR));
    if (elfPhdrRead(pmodule, pdyndir, pehdr, iFd)) {                    /*  读取和解析elf中的各个段     */
        goto    __out;
    }

    /*
     *  TODO: ELF头中会带有程序入口地址，而loader接口中也包括了入口函数名称，当二者不一致时会导致混乱。
     */
    if (pehdr->e_entry) {                                               /*  程序入口                    */
        pmodule->EMOD_pfuncEntry = (FUNCPTR)LW_LD_V2PADDR(pdyndir->addrMin,
                                                 pmodule->EMOD_pvBaseAddr,
                                                 pehdr->e_entry);
    }

    if (elfPhdrSymExport(pmodule, pdyndir)) {                           /*  导出符号表                  */
        goto    __out;
    }

    /*
     *  加载所依赖的库，为解决循环依赖问题，将导出符号表放在这一步前面。
     */
    if (pdyndir->ulNeededCnt > 0) {
        pmodule->EMOD_ulUsedCnt = pdyndir->ulNeededCnt;
        pmodule->EMOD_pvUsedArr = LW_LD_SAFEMALLOC(pmodule->EMOD_ulUsedCnt *
                                                   sizeof (LW_LD_EXEC_MODULE *));
        if (!pmodule->EMOD_pvUsedArr) {
            goto    __out;
        }

        lib_bzero(pmodule->EMOD_pvUsedArr,
                  pmodule->EMOD_ulUsedCnt * sizeof (LW_LD_EXEC_MODULE *));
    }

    /*
     *  按倒序的方式加载所有依赖的库.
     */
    ppmodUseArr = pmodule->EMOD_pvUsedArr;
    for (i = 0; i < pdyndir->ulNeededCnt; i++) {
        if (NULL == pdyndir->pcStrTable) {
            break;
        }
        pchLibName = pdyndir->pcStrTable + pdyndir->wdNeededArr[pdyndir->ulNeededCnt - i - 1];
#if !defined(LW_CFG_CPU_ARCH_C6X)
        ppmodUseArr[i] = moduleLoadSub(pmodule, pchLibName, LW_TRUE);
#else
        ppmodUseArr[i] = moduleLoadSub(pmodule, _PathLastNamePtr(pchLibName), LW_TRUE);
#endif
        if (ppmodUseArr[i] == LW_NULL) {
            goto    __out;
        }
        ppmodUseArr[i]->EMOD_ulRefs++;
    }

    pmodule->EMOD_pvFormatInfo = (PVOID)pdyndir;                        /*  运行到这里装载成功!         */

    return  (ERROR_NONE);

__out:
    LW_LD_VMSAFEFREE_AREA(pmodule->EMOD_pvBaseAddr);
    __moduleDeleteAllSymbol(pmodule);
    LW_LD_SAFEFREE(pmodule->EMOD_psymbolHash);
    LW_LD_SAFEFREE(pmodule->EMOD_ppfuncInitArray);
    LW_LD_SAFEFREE(pmodule->EMOD_ppfuncFiniArray);
    LW_LD_SAFEFREE(pmodule->EMOD_pvUsedArr);
    LW_LD_SAFEFREE(pdyndir);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __elfLoad
** 功能描述: 加载elf文件.
** 输　入  : pmodule       模块指针
**           pcPath        文件路径
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __elfLoad (LW_LD_EXEC_MODULE *pmodule, CPCHAR pcPath)
{
    INT           iFd;
    struct stat   statBuf;
    Elf_Ehdr      ehdr;
    INT           iError = PX_ERROR;
    PCHAR         pcModVersion = LW_NULL;

    iFd = open(pcPath, O_RDONLY);
    if (iFd < 0) {                                                      /*  打开文件                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "open file error!\r\n");
        return  (PX_ERROR);
    }
    
    if (fstat(iFd, &statBuf) < ERROR_NONE) {                            /*  获得文件信息                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not get file stat!\r\n");
        goto    __out;
    }
    
    if (_IosCheckPermissions(O_RDONLY, LW_TRUE, statBuf.st_mode,        /*  检查文件执行权限            */
                             statBuf.st_uid, statBuf.st_gid) < ERROR_NONE) {
        _ErrorHandle(ERROR_LOADER_EACCES);
        goto    __out;
    }

    if (read(iFd, &ehdr, sizeof(ehdr)) < sizeof(ehdr)) {                /*  读取ELF头                   */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "read elf header error!\r\n");
        goto    __out;
    }

    if (elfCheck(&ehdr, LW_TRUE) < 0) {                                 /*  检查文件头有效性            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "elf format error!\r\n");
        goto    __out;
    }

    switch (ehdr.e_type) {

    case ET_REL:
        iError = elfLoadReloc(pmodule, &ehdr, iFd);                     /*  加载可重定位文件            */
        break;

    case ET_EXEC:
    case ET_DYN:
        iError = elfLoadExec(pmodule, &ehdr, iFd);                      /*  加载可执行文件              */
        break;

    default:
        _ErrorHandle(ERROR_LOADER_FORMAT);
    }

    if (iError == ERROR_NONE) {
        if (ERROR_NONE != __moduleFindSym(pmodule,
                                          LW_LD_VER_SYM,
                                          (ULONG*)&pcModVersion,
                                          0)) {                         /*  获取模块版本                */
            pcModVersion = LW_LD_DEF_VER;
        }

        if (pcModVersion == LW_NULL) {
            pcModVersion = LW_LD_DEF_VER;                               /*  使用默认版本                */
        }

        if (ERROR_NONE != LW_LD_VERIFY_VER(pmodule->EMOD_pcModulePath, 
                                           pcModVersion, 
                                           pmodule->EMOD_ulModType)) {
            _ErrorHandle(ERROR_LOADER_VERSION);                         /*  版本兼容性错误              */
            iError = PX_ERROR;
        }
    }

__out:
    close(iFd);
    
    if (iError == ERROR_NONE) {
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_LOADER, MONITOR_EVENT_LOADER_LOAD,
                          pmodule, pmodule->EMOD_pvproc->VP_pid, pcPath);
    }

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __elfListLoad
** 功能描述: 加载模块链表指针，用于带依赖关系的模块加载
** 输　入  : pmodule       模块链表指针
**           pcPath        文件路径
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __elfListLoad (LW_LD_EXEC_MODULE *pmodule, CPCHAR pcPath)
{
    INT                iError    = PX_ERROR;
    LW_LD_EXEC_MODULE *pmodTemp  = LW_NULL;
    LW_LIST_RING      *pringTemp = LW_NULL;

    /*
     *  首先装载所有需要装载的模块
     */
    pringTemp = &pmodule->EMOD_ringModules;
    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        if (pmodTemp->EMOD_ulStatus == LW_LD_STATUS_UNLOAD) {           /*  模块还没有被装载            */
            if (__elfLoad(pmodTemp, pmodTemp->EMOD_pcModulePath)) {
                if (errno == ERROR_LOADER_EACCES) {
                    fprintf(stderr, "[ld]%s insufficient permissions!\n",
                            pmodTemp->EMOD_pcModulePath);               /*  从标准错误里打印无权限信息  */
                } else {
                    fprintf(stderr, "[ld]Load file \"%s\" error %s!\n",
                            pmodTemp->EMOD_pcModulePath, lib_strerror(errno));
                    _ErrorHandle(ERROR_LOADER_NO_MODULE);
                }
                goto    __out;
            }
            pmodTemp->EMOD_ulStatus = LW_LD_STATUS_LOADED;
        }
        pringTemp = _list_ring_get_next(pringTemp);
    } while (pringTemp != &pmodule->EMOD_ringModules);

    /*
     *  然后装载所有需要装载的模块
     */
    pringTemp = &pmodule->EMOD_ringModules;
    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        if (pmodTemp->EMOD_pvFormatInfo) {                              /*  模块还没有完成重定位        */
            if (elfPhdrRelocate(pmodTemp, (ELF_DYN_DIR *)pmodTemp->EMOD_pvFormatInfo)) {
                                                                        /*  重定位                      */
                goto    __out;
            }

            if (elfPhdrBuildInitTable(pmodTemp, (ELF_DYN_DIR *)pmodTemp->EMOD_pvFormatInfo)) {
                goto    __out;
            }
        }
#if LW_CFG_CACHE_EN > 0
        API_CacheTextUpdate(pmodTemp->EMOD_pvBaseAddr, pmodTemp->EMOD_stLen);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
        pringTemp = _list_ring_get_next(pringTemp);
    } while (pringTemp != &pmodule->EMOD_ringModules);

    iError = ERROR_NONE;

__out:
    pringTemp = &pmodule->EMOD_ringModules;
    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        LW_LD_SAFEFREE(pmodTemp->EMOD_pvFormatInfo);
        pringTemp = _list_ring_get_next(pringTemp);
    } while (pringTemp != &pmodule->EMOD_ringModules);

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __elfListUnload
** 功能描述: 卸载 elf 文件, 包括依赖的 elf.
** 输　入  : pmodule       模块指针
** 输　出  : NONE
** 全局变量:
** 调用模块: 
** 注  意  : 当前仅是将卸载模块的信息记录入 monitor.
*********************************************************************************************************/
VOID  __elfListUnload (LW_LD_EXEC_MODULE *pmodule)
{
#if LW_CFG_MONITOR_EN > 0
    LW_LIST_RING      *pringTemp;
    LW_LD_EXEC_MODULE *pmodTemp;
    LW_LD_VPROC       *pvproc;
    
    pvproc    = pmodule->EMOD_pvproc;
    pringTemp = &pmodule->EMOD_ringModules;
    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        if (pmodTemp->EMOD_ulRefs == 0) {
            MONITOR_EVT_LONG2(MONITOR_EVENT_ID_LOADER, MONITOR_EVENT_LOADER_UNLOAD,
                              pmodTemp, pvproc->VP_pid, LW_NULL);
        }
        pringTemp = _list_ring_get_next(pringTemp);
    } while (pringTemp != &pmodule->EMOD_ringModules);
#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
}
/*********************************************************************************************************
** 函数名称: __elfGetMachineStr
** 功能描述: 通过 e_machine 获取处理器类型符号.
** 输　入  : ehMachine     机器编号
** 输　出  : 机器类型字符串
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static CPCHAR  __elfGetMachineStr (Elf_Half  ehMachine)
{
    switch (ehMachine) {

    case EM_NONE:
        return  ("none");

    case EM_M32:
        return  ("AT&T WE 32100");

    case EM_SPARC:
        return  ("Sun SPARC");

    case EM_386:
        return  ("Intel 80386");

    case EM_68K:
        return  ("Motorola 68000");

    case EM_88K:
        return  ("Motorola 88000");

    case EM_486:
        return  ("Intel 80486");

    case EM_860:
        return  ("Intel i860");

    case EM_MIPS:
        return  ("MIPS family");

    case EM_PARISC:
        return  ("HPPA");
        
    case EM_SPARC32PLUS:
        return  ("Sun's SPARCv8 plus");

    case EM_PPC:
        return  ("PowerPC family");

    case EM_PPC64:
        return  ("PowerPC64 family");

    case EM_S390:
        return  ("IBM S/390");
        
    case EM_SPU:
        return  ("Cell BE SPU");

    case EM_ARM:
        return  ("ARM family");

    case EM_SH:
        return  ("Hitachi SH family");
        
    case EM_SPARCV9:
        return  ("SPARC v9 64-bit");
    
    case EM_H8_300:
        return  ("Renesas H8/300,300H,H8S");
    
    case EM_IA_64:
        return  ("HP/Intel IA-64");
    
    case EM_X86_64:
        return  ("AMD x86-64");
    
    case EM_CRIS:
        return  ("Axis 32-bit processor");
    
    case EM_V850:
        return  ("NEC v850");
    
    case EM_M32R:
        return  ("Renesas M32R");
    
    case EM_MN10300:
        return  ("Panasonic/MEI MN10300, AM33");
    
    case EM_BLACKFIN:
        return  ("ADI Blackfin Processor");
    
    case EM_ALTERA_NIOS2:
        return  ("ALTERA NIOS2");

    case EM_TI_C6000:
        return  ("TI C6x DSPs");

    case EM_AARCH64:
        return  ("ARM AArch64");
    
    case EM_FRV:
        return  ("Fujitsu FR-V");
    
    case EM_AVR32:
        return  ("Atmel AVR32");
    
    case EM_ALPHA:
        return  ("Alpha");
    
    default:
        return  ("unknown");
    }
}
/*********************************************************************************************************
** 函数名称: __elfGetETypeStr
** 功能描述: 通过 e_type 获取 elf 类型.
** 输　入  : ehEType      e_type
** 输　出  : elf 类型
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static CPCHAR  __elfGetETypeStr (Elf_Half  ehEType)
{
    switch (ehEType) {
    
    case ET_NONE:
        return  ("ET_NONE");
        
    case ET_REL:
        return  ("ET_REL");
        
    case ET_EXEC:
        return  ("ET_EXEC");
    
    case ET_DYN:
        return  ("ET_DYN");
    
    case ET_CORE:
        return  ("ET_CORE");
    
    default:
        return  ("unknown");
    }
}
/*********************************************************************************************************
** 函数名称: __elfGetShTypeStr
** 功能描述: 通过 sh_type 获取分段类型.
** 输　入  : ewShType      分段类型
** 输　出  : 分段类型字符串
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static CPCHAR  __elfGetShTypeStr (Elf_Word  ewShType)
{
    switch (ewShType) {

    case SHT_NULL:
        return  ("NULL");

    case SHT_PROGBITS:
        return  ("PROGBITS");

    case SHT_SYMTAB:
        return  ("SYMTAB");

    case SHT_STRTAB:
        return  ("STRTAB");

    case SHT_RELA:
        return  ("RELA");

    case SHT_HASH:
        return  ("HASH");

    case SHT_DYNAMIC:
        return  ("DYNAMIC");

    case SHT_NOTE:
        return  ("NOTE");

    case SHT_NOBITS:
        return  ("NOBITS");

    case SHT_REL:
        return  ("REL");

    case SHT_SHLIB:
        return  ("SHLIB");

    case SHT_DYNSYM:
        return  ("DYNSYM");

    case SHT_NUM:
        return  ("NUM");

    case SHT_LOPROC:
        return  ("LOPROC");

    case SHT_HIPROC:
        return  ("HIPROC");

    case SHT_LOUSER:
        return  ("LOUSER");

    case SHT_HIUSER:
        return  ("HIUSER");

    default:
        return  ("NONE");
    }
}
/*********************************************************************************************************
** 函数名称: __elfGetShFlagStr
** 功能描述: 通过 sh_flag 获取分段flag.
** 输　入  : ewShFlag      ewShFlag
**           pcBuffer      输出缓冲
**           stSize        缓冲大小
** 输　出  : 分段flag
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static CPCHAR  __elfGetShFlagStr (Elf_Word  ewShFlag, PCHAR pcBuffer, size_t stSize)
{
    pcBuffer[0] = PX_EOS;

    if (ewShFlag & SHF_ALLOC) {
        lib_strlcat(pcBuffer, "[ALLOC]", stSize);
    }
    
    if (ewShFlag & SHF_EXECINSTR) {
        lib_strlcat(pcBuffer, "[EXEC]", stSize);
    }
    
    if (ewShFlag & SHF_WRITE) {
        lib_strlcat(pcBuffer, "[WRITE]", stSize);
    }
    
    return  (pcBuffer);
}
/*********************************************************************************************************
** 函数名称: __elfStatus
** 功能描述: 查看elf文件信息.
** 输　入  : pcPath     文件路径
**           iFd        信息输出文件句柄
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __elfStatus (CPCHAR pcPath, INT iFd)
{
    INT           iFdElf;
    Elf_Shdr     *shdr;
    PCHAR         pcBuf = LW_NULL;

    Elf_Ehdr      ehdr;

    size_t        stHdSize;
    ULONG         i;

    iFdElf = open(pcPath, O_RDONLY);
    if (iFdElf < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "open file error!\r\n");
        return  (PX_ERROR);
    }

    if (read(iFdElf, &ehdr, sizeof(ehdr)) < sizeof(ehdr)) {             /*  读取ELF头                   */
        close(iFdElf);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "read elf header error!\r\n");
        return  (PX_ERROR);
    }

    if (elfCheck(&ehdr, LW_FALSE) < 0) {                                /*  检查文件头有效性            */
        if (API_GetLastError() != ERROR_LOADER_ARCH) {                  /*  文件格式错误                */
            close(iFdElf);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "elf format error!\r\n");
            return  (PX_ERROR);
        }
    }

    fdprintf(iFd, "File Type: ELF\r\n");                                /*  打印文件信息                */
    fdprintf(iFd, "Machine:   %s\r\n", __elfGetMachineStr(ehdr.e_machine));
    fdprintf(iFd, "Type:      %s\r\n", __elfGetETypeStr(ehdr.e_type));
    fdprintf(iFd, "Entry:     %x\r\n", ehdr.e_entry);

    /*
     *  读取节区头部表
     */
    stHdSize = ehdr.e_shentsize * ehdr.e_shnum;
    pcBuf = (PCHAR)LW_LD_SAFEMALLOC(stHdSize);
    if (pcBuf == LW_NULL) {
        close(iFdElf);
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }

    if (lseek(iFdElf, ehdr.e_shoff, SEEK_SET) < 0) {
        close(iFdElf);
        LW_LD_SAFEFREE(pcBuf);
        return  (PX_ERROR);
    }
    if (read(iFdElf, pcBuf, stHdSize) < stHdSize) {
        close(iFdElf);
        LW_LD_SAFEFREE(pcBuf);
        return  (PX_ERROR);
    }

    /*
     *  打印节区头部表信息
     */
    fdprintf(iFd, "Section Headers:\r\n");
    fdprintf(iFd, "%-10s%-10s%-10s%-10s%-10s\r\n",
             "TYPE",
             "ADDRESS",
             "OFFSET",
             "SIZE",
             "FLAGS");

    shdr = (Elf_Shdr *)pcBuf;
    for (i = 0; i < ehdr.e_shnum; i++, shdr++) {
        CHAR    cBuffer[128];
        fdprintf(iFd, "%-8s  %08x  %8x  %8x  %s\r\n",
                 __elfGetShTypeStr(shdr->sh_type),
                 shdr->sh_addr,
                 shdr->sh_offset,
                 shdr->sh_size,
                 __elfGetShFlagStr(shdr->sh_flags, cBuffer, sizeof(cBuffer)));
    }

    close(iFdElf);
    LW_LD_SAFEFREE(pcBuf);

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
