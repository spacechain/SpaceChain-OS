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
** 文   件   名: symTable.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 02 月 26 日
**
** 描        述: 系统符号表管理器 (为程序装载器提供服务).
** 注        意: 这里是系统全局符号表管理.
*********************************************************************************************************/

#ifndef __SYMTABLE_H
#define __SYMTABLE_H

/*********************************************************************************************************
  symbol flag
*********************************************************************************************************/
#define LW_SYMBOL_FLAG_STATIC           0x80000000                      /*  不能删除的静态符号          */
#define LW_SYMBOL_FLAG_REN              0x00000001                      /*  可读符号                    */
#define LW_SYMBOL_FLAG_WEN              0x00000002                      /*  可写符号                    */
#define LW_SYMBOL_FLAG_XEN              0x00000004                      /*  可执行符号                  */

#define LW_SYMBOL_IS_STATIC(flag)       ((flag) & LW_SYMBOL_FLAG_STATIC)
#define LW_SYMBOL_IS_REN(flag)          ((flag) & LW_SYMBOL_FLAG_REN)
#define LW_SYMBOL_IS_WEN(flag)          ((flag) & LW_SYMBOL_FLAG_WEN)
#define LW_SYMBOL_IS_XEN(flag)          ((flag) & LW_SYMBOL_FLAG_XEN)
/*********************************************************************************************************
  symbol 
*********************************************************************************************************/
typedef struct lw_symbol {
    LW_LIST_LINE         SYM_lineManage;                                /*  管理链表                    */
    PCHAR                SYM_pcName;                                    /*  符号名                      */
    caddr_t              SYM_pcAddr;                                    /*  对应的内存地址              */
    INT                  SYM_iFlag;                                     /*  flag                        */
} LW_SYMBOL;
typedef LW_SYMBOL       *PLW_SYMBOL;
/*********************************************************************************************************
  symbol api
  delete 函数谨慎使用, 例如遍历结束后, 用户不能再使用回调函数中的 PLW_SYMBOL 结构.
*********************************************************************************************************/
LW_API VOID         API_SymbolInit(VOID);
LW_API INT          API_SymbolAddStatic(PLW_SYMBOL  psymbol, INT  iNum);/*  静态符号表安装              */
LW_API INT          API_SymbolAdd(CPCHAR  pcName, caddr_t  pcAddr, INT  iFlag);
LW_API INT          API_SymbolDelete(CPCHAR  pcName, INT  iFlag);
LW_API PVOID        API_SymbolFind(CPCHAR  pcName, INT  iFlag);
LW_API VOID         API_SymbolTraverse(BOOL (*pfuncCb)(PVOID, PLW_SYMBOL), PVOID  pvArg);

#define symbolInit          API_SymbolInit
#define symbolAddStatic     API_SymbolAddStatic
#define symbolAdd           API_SymbolAdd
#define symbolDelete        API_SymbolDelete
#define symbolFind          API_SymbolFind
#define symbolTraverse      API_SymbolTraverse

/*********************************************************************************************************
  API_SymbolAddStatic 说明: (一般只有在系统使用模块装载器时, 才使用此功能!)
  
  此函数仅供 BSP 初始化时使用! 
  
  API_SymbolAddStatic 由 sylixos 宿主工具产生相应的代码调用, 具体请查阅 doc/SYMBOL 文件.
*********************************************************************************************************/

#endif                                                                  /*  __SYMTABLE_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
