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
** 文   件   名: loader_symbol.h
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2010 年 04 月 17 日
**
** 描        述: 符号管理

** BUG:
2011.02.20  改进符号表管理, 加入局部和全局的区别. (韩辉)
*********************************************************************************************************/

#ifndef __LOADER_SYMBOL_H
#define __LOADER_SYMBOL_H

/*********************************************************************************************************
  符号属性
*********************************************************************************************************/
#define LW_LD_SYM_ANY       0                                           /*  任何类型的符号              */
#define LW_LD_SYM_FUNCTION  (LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN)   /*  函数符号                    */
#define LW_LD_SYM_DATA      (LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_WEN)   /*  数据符号                    */

#define LW_LD_VER_SYM       "__sylixos_version"                         /*  版本变量                    */
#define LW_LD_DEF_VER       "0.0.0"                                     /*  默认版本                    */
#define LW_LD_VERIFY_VER(pcModuleName, pcVersion, ulType)   \
        __moduleVerifyVersion(pcModuleName, pcVersion, ulType)

/*********************************************************************************************************
  版本符号相关
*********************************************************************************************************/

INT __moduleVerifyVersion(CPCHAR  pcModuleName, 
                          CPCHAR  pcVersion, 
                          ULONG   ulType);                              /*  模块系统版本与当前系统匹配  */

/*********************************************************************************************************
  符号操作
*********************************************************************************************************/

INT __moduleFindSym(LW_LD_EXEC_MODULE  *pmodule,
                    CPCHAR              pcSymName, 
                    ULONG              *pulSymVal, INT iFlag);          /*  查找模块内符号              */

INT __moduleSymGetValue(LW_LD_EXEC_MODULE  *pmodule,
                        BOOL                bIsWeak,
                        CPCHAR              pcSymName,
                        addr_t             *pulSymVal,
                        INT                 iFlag);                     /*  从全局, 进程, 依赖链表查找  */

INT __moduleExportSymbol(LW_LD_EXEC_MODULE  *pmodule,
                         CPCHAR              pcSymName,
                         addr_t              ulSymVal,
                         INT                 iFlag,
                         ULONG               ulAllSymCnt,
                         ULONG               ulCurSymNum);              /*  导出符号                    */

VOID __moduleTraverseSym(LW_LD_EXEC_MODULE  *pmodule, 
                         BOOL (*pfuncCb)(PVOID, PLW_SYMBOL, LW_LD_EXEC_MODULE *), 
                         PVOID  pvArg);                                 /*  遍历模块链表所有符号        */

INT __moduleDeleteAllSymbol(LW_LD_EXEC_MODULE *pmodule);
                                                                        /*  删除符号                    */
                                                                        
size_t __moduleSymbolBufferSize(LW_LD_EXEC_MODULE *pmodule);            /*  获取符号表缓存大小          */

#endif                                                                  /*  __LOADER_SYMBOL_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
