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
** 文   件   名: ttinyShellLib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 07 月 27 日
**
** 描        述: 一个超小型的 shell 系统, 使用 tty/pty 做接口, 主要用于调试与简单交互.

** BUG:
2009.08.04  加入了 keyword 选项功能.
2011.03.11  加入 TCB_ulReserve0 作为 shell 标准文件保存地址.
2011.06.24  TCB_ulReserve0 改为 TCB_ulShellStdFile !
*********************************************************************************************************/

#ifndef __TTINYSHELLLIB_H
#define __TTINYSHELLLIB_H

/*********************************************************************************************************
  shell 强制退出命令
*********************************************************************************************************/

#define __TTINY_SHELL_FORCE_ABORT   "`${@_force_\x82\x93\xA4\xE0_abort_@}$`"

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

/*********************************************************************************************************
  锁句柄
*********************************************************************************************************/

extern  LW_OBJECT_HANDLE            _G_hTShellLock;

/*********************************************************************************************************
  宏配置与操作
*********************************************************************************************************/

#define __TTINY_SHELL_LOCK_OPT      (LW_OPTION_WAIT_PRIORITY | \
                                     LW_OPTION_INHERIT_PRIORITY | \
                                     LW_OPTION_DELETE_SAFE | \
                                     LW_OPTION_OBJECT_GLOBAL)
#define __TTINY_SHELL_LOCK()        API_SemaphoreMPend(_G_hTShellLock, LW_OPTION_WAIT_INFINITE);
#define __TTINY_SHELL_UNLOCK()      API_SemaphoreMPost(_G_hTShellLock);

/*********************************************************************************************************
  shell std file 操作
*********************************************************************************************************/

#define __TTINY_SHELL_SET_STDFILE(ptcb, iFd)    ptcb->TCB_shc.SHC_ulShellStdFile = iFd;
#define __TTINY_SHELL_GET_STDFILE(ptcb)         (INT)ptcb->TCB_shc.SHC_ulShellStdFile

/*********************************************************************************************************
  shell 错误暂存
*********************************************************************************************************/

#define __TTINY_SHELL_SET_ERROR(ptcb, iRet)     ptcb->TCB_shc.SHC_ulShellError = (ULONG)iRet
#define __TTINY_SHELL_GET_ERROR(ptcb)           (INT)ptcb->TCB_shc.SHC_ulShellError

/*********************************************************************************************************
  shell 选项参数
*********************************************************************************************************/

#define __TTINY_SHELL_SET_OPT(ptcb, ulOption)   ptcb->TCB_shc.SHC_ulShellOption = ulOption
#define __TTINY_SHELL_GET_OPT(ptcb)             ptcb->TCB_shc.SHC_ulShellOption

/*********************************************************************************************************
  shell 回调
*********************************************************************************************************/

#define __TTINY_SHELL_SET_CALLBACK(ptcb, pfunc) ptcb->TCB_shc.SHC_pfuncShellCallback = pfunc
#define __TTINY_SHELL_GET_CALLBACK(ptcb)        ptcb->TCB_shc.SHC_pfuncShellCallback

/*********************************************************************************************************
  shell 历史输入相关
*********************************************************************************************************/

#define __TTINY_SHELL_SET_HIS(ptcb, psihc)      ptcb->TCB_shc.SHC_ulShellHistoryCtx = (addr_t)psihc
#define __TTINY_SHELL_GET_HIS(ptcb)             (__PSHELL_HISTORY_CTX)ptcb->TCB_shc.SHC_ulShellHistoryCtx

/*********************************************************************************************************
  shell 魔数
*********************************************************************************************************/

#define __TTINY_SHELL_SET_MAGIC(ptcb, ulMagic)  ptcb->TCB_shc.SHC_ulShellMagic = ulMagic
#define __TTINY_SHELL_GET_MAGIC(ptcb)           ptcb->TCB_shc.SHC_ulShellMagic

/*********************************************************************************************************
  shell 主线程
*********************************************************************************************************/

#define __TTINY_SHELL_SET_MAIN(ptcb)            ptcb->TCB_shc.SHC_ulShellMain = ptcb->TCB_ulId
#define __TTINY_SHELL_GET_MAIN(ptcb)            ptcb->TCB_shc.SHC_ulShellMain

/*********************************************************************************************************
  KEYWORD
*********************************************************************************************************/

typedef struct __shell_keyword {
    LW_LIST_LINE             SK_lineManage;                             /*  管理用双链表                */
    LW_LIST_LINE             SK_lineHash;                               /*  哈希分离链表                */

    PCOMMAND_START_ROUTINE   SK_pfuncCommand;                           /*  执行函数                    */
    ULONG                    SK_ulOption;                               /*  特殊服务选项                */
    
    PCHAR                    SK_pcKeyword;                              /*  关键字                      */
    PCHAR                    SK_pcFormatString;                         /*  格式字段                    */
    PCHAR                    SK_pcHelpString;                           /*  指向帮助字符串缓冲区        */
} __TSHELL_KEYWORD;
typedef __TSHELL_KEYWORD    *__PTSHELL_KEYWORD;                         /*  指针类型                    */

/*********************************************************************************************************
  内部函数声明
*********************************************************************************************************/

ULONG  __tshellKeywordAdd(CPCHAR  pcKeyword, size_t stStrLen, PCOMMAND_START_ROUTINE  pfuncCommand, 
                          ULONG   ulOption);
ULONG  __tshellKeywordFind(CPCHAR  pcKeyword, __PTSHELL_KEYWORD   *ppskwNode);
ULONG  __tshellKeywordList(__PTSHELL_KEYWORD   pskwNodeStart,
                           __PTSHELL_KEYWORD   ppskwNode[],
                           INT                 iMaxCounter);
PVOID  __tshellThread(PVOID  pcArg);
INT    __tshellRestartEx(LW_OBJECT_HANDLE  ulThread, BOOL  bNeedAuthen);
VOID   __tshellPreTreatedBg(PCHAR  cCommand, BOOL  *pbNeedJoin, BOOL  *pbNeedAsyn);
INT    __tshellExec(CPCHAR  pcCommandExec, VOIDFUNCPTR  pfuncHook);
INT    __tshellBgCreateEx(INT               iFd[3],
                          BOOL              bClosed[3],
                          CPCHAR            pcCommand, 
                          size_t            stCommandLen, 
                          ULONG             ulKeywordOpt,
                          BOOL              bIsJoin,
                          ULONG             ulMagic,
                          LW_OBJECT_HANDLE *pulSh,
                          INT              *piRet);

/*********************************************************************************************************
  用户与组
*********************************************************************************************************/

INT     __tshellGetUserName(uid_t  uid, PCHAR  pcName, size_t  stNSize, PCHAR  pcHome, size_t  stHSize);
INT     __tshellGetGrpName(gid_t  gid, PCHAR  pcName, size_t  stSize);
VOID    __tshellFlushCache(VOID);
ULONG   __tshellUserAuthen(INT  iTtyFd, BOOL  bWaitInf);
VOID    __tshellUserCmdInit(VOID);
                          
/*********************************************************************************************************
  变量保存文件
*********************************************************************************************************/

#define __TTINY_SHELL_VAR_FILE                  "/etc/profile"

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  __TTINYSHELLLIB_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
