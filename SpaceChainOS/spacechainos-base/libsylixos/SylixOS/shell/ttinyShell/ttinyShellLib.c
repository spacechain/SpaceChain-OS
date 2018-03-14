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
** 文   件   名: ttinyShellLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 07 月 27 日
**
** 描        述: 一个超小型的 shell 系统, 使用 tty/pty 做接口, 主要用于调试与简单交互.

** BUG
2008.10.20  今天是微软向盗版软件宣战重要的日子, 心情十分复杂... 
            shell 系统支持背景运行能力.
2008.12.24  今天是圣诞前夜, 祝我自己圣诞快乐, 呵呵! 修改了命令提示符.
2009.02.09  修改 shell 一处小 BUG.
2009.02.13  dup2 产生的 fd 需要关闭.
2009.02.19  将命令提示符修改为与 linux bash 类似.
2009.02.23  加入 join 背景运行的符号.
2009.03.10  终端启动时修改终端窗口的标题.
2009.03.24  当使用命令连字符, 下方打印 > 提示.
2009.03.25  在调用 shell exec 前加入对 optind 和 optreset 的操作, 确保 getopt 的正确性.
            背景线程不需要, 在初始化 getopt 线程上下文时, 已经赋了初值.
2009.04.13  背景线程加入清除函数入栈, 防止内部服务函数调用 exit 直接退出, 导致无法释放缓存造成内存泄露.
2009.05.22  加入用户认证功能.
2009.05.28  加入了 shell 重启的功能.
2009.06.30  启动 shell 时不打印 tty 终端名.
            命令提示符显示全部当前路径.
2009.07.13  支持 LW_OPTION_TSHELL_NOECHO 创建 shell 线程.
2009.08.04  __tshellKeywordAdd() 加入选项参数.
            __tshellExec() 支持关键字选项功能.
2009.10.28  __tshellExec() 直接忽略注释行.
2009.11.21  shell 线程(或其衍生出的背景线程)使用私有的 io 环境. 其衍生线程将继承父系线程的 io 环境.
2010.05.06  加入 undef 接口的操作.
2010.12.27  解决 __tshellExec 执行的变量赋值语句长度大于最大关键字长度时, 出现的赋值错误 bug. 
2011.03.10  加入系统重定向输出功能.
            使用新的保存标准文件描述符的方法.
2011.04.23  重定向时, 需要修正参数列表.
2011.05.19  修正 __tshellExec() 调用 pfuncHook 时的参数前有符号错误.
2011.06.23  使用新的色彩转义.
2011.06.24  使用新的背景运行算法, 由 __tshellExec() 判断命令行是否需要背景执行.
2011.07.27  shell 背景线程使用缩短命令名作为线程名.
2011.08.06  shell 背景同步执行完毕时, 需要返回命令返回值, shell 主任务也要设置相应的返回值.
2012.03.25  shell undef 处理写入此文件, 同时 shell 读取不再使用 read 而是用一个专用函数, 这个函数将实现一
            些易用性操作, 例如历史命令, 命令随意位置修改等等.
2012.03.25  __tshellReadline() 内部已经处理了 \n, shell 任务不需要再次处理.
2012.03.27  每一个 shell 输入都要判断强制退出命令.
2012.03.30  创建背景执行线程使用 API_ThreadStartEx() 进行 join 原子操作.
2012.08.25  加入 __tshellBgCreateEx() 提供扩展文件描述符的功能.
2012.11.09  __tshellRestart() 内部需要严格判断句柄是否有效.
2012.12.07  重新设计 shell 执行背景程序的方式, 防止二次背景执行造成的浪费.
2012.12.24  shell 支持退出清除文件功能.
2012.12.25  __tshellBgCreateEx() 将自身返回值和命令返回值隔离开.
2013.01.23  shell 当系统命令和可执行文件名重复时, 优先运行文件. 
2014.07.23  shell 加入标准文件重定向支持.
2014.08.27  __tshellRestart() 如果判断是在等待其他任务, 则使用 kill 操作.
2014.04.17  对重定向描述符的回收需要在内核 IO 环境中中进行.
2017.07.19  执行完每条命令后使用 fpurge(stdin) 清除输入缓存.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "unistd.h"
/*********************************************************************************************************
  应用级 API
*********************************************************************************************************/
#include "../SylixOS/api/Lw_Api_Kernel.h"
#include "../SylixOS/api/Lw_Api_System.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "limits.h"
#include "pwd.h"
#include "ttinyShell.h"
#include "ttinyShellLib.h"
#include "ttinyShellSysCmd.h"
#include "ttinyShellSysVar.h"
#include "ttinyShellReadline.h"
#include "ttinyString.h"
#include "../SylixOS/shell/hashLib/hashHorner.h"
#include "../SylixOS/shell/ttinyVar/ttinyVarLib.h"
/*********************************************************************************************************
  背景控制 (t_shell 线程中通过接收的命令字符串末尾判断)
*********************************************************************************************************/
#define   __TTINY_SHELL_BG_ASYNC            '&'                         /*  背景异步执行                */
#define   __TTINY_SHELL_BG_JOIN             '^'                         /*  背景同步执行                */
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static PLW_LIST_LINE    _G_plineTSKeyHeader = LW_NULL;                  /*  命令链表头                  */
static PLW_LIST_LINE    _G_plineTSKeyHeaderHashTbl[LW_CFG_SHELL_KEY_HASH_SIZE];
                                                                        /*  哈希搜索表                  */
/*********************************************************************************************************
  shell 执行线程堆栈大小 (默认与 shell 相同)
*********************************************************************************************************/
extern size_t           _G_stShellStackSize;
/*********************************************************************************************************
  背景线程执行参数
*********************************************************************************************************/
typedef struct {
    INT          TSBG_iFd[3];                                           /*  文件描述符                  */
    BOOL         TSBG_bClosed[3];                                       /*  运行完毕是否关闭对应文件    */
    PCHAR        TSBG_pcDefPath;                                        /*  父系线程当前目录项          */
    BOOL         TSBG_bIsJoin;                                          /*  shell 是否在等待返回值      */
    CHAR         TSBG_cCommand[1];                                      /*  执行命令                    */
} __TSHELL_BACKGROUND;
typedef __TSHELL_BACKGROUND     *__PTSHELL_BACKGROUND;

typedef struct {
    uid_t        TPC_uid;
    gid_t        TPC_gid;
    CHAR         TPC_cUserName[MAX_FILENAME_LENGTH];
} __TSHELL_PROMPT_CTX;
typedef __TSHELL_PROMPT_CTX     *__PTSHELL_PROMPT_CTX;
/*********************************************************************************************************
  用户认证
*********************************************************************************************************/
       ULONG  __tshellUserAuthen(INT  iTtyFd, BOOL  bWaitInf);
static INT    __tshellBgCreate(INT      iFd, 
                               CPCHAR   pcCommand, 
                               size_t   stCommandLen, 
                               ULONG    ulKeywordOpt,
                               BOOL     bIsJoin, 
                               ULONG    ulMagic);
/*********************************************************************************************************
  装载器内部函数声明
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
extern BOOL   __ldPathIsFile(CPCHAR  pcName);
/*********************************************************************************************************
** 函数名称: __tshellCheckFile
** 功能描述: shell 未定义命令检查是否存在对应命令的文件
** 输　入  : pcKeyword         当前未能识别的命令
** 输　出  : 是否有对应的文件
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  __tshellCheckFile (CPCHAR  pcKeyword)
{
    PCHAR   pcBuffer;
    PCHAR   pcPathBuffer;
    
    PCHAR   pcStart;
    PCHAR   pcDiv;
    
    if (lib_strchr(pcKeyword, PX_DIVIDER)) {                            /*  是一个路径                  */
        if (__ldPathIsFile(pcKeyword)) {
            return  (LW_TRUE);
        
        } else {
            return  (LW_FALSE);
        }
    
    } else {
        pcBuffer = (PCHAR)__SHEAP_ALLOC(MAX_FILENAME_LENGTH * 2);
        if (pcBuffer == LW_NULL) {
            return  (LW_FALSE);
        }
        pcPathBuffer = pcBuffer + MAX_FILENAME_LENGTH;
    
        if (lib_getenv_r("PATH", pcBuffer, MAX_FILENAME_LENGTH)
            != ERROR_NONE) {                                            /*  PATH 环境变量值为空         */
            __SHEAP_FREE(pcBuffer);
            return  (LW_FALSE);                                         /*  无法找到文件                */
        }
        
        pcPathBuffer[MAX_FILENAME_LENGTH - 1] = PX_EOS;
        
        pcDiv = pcBuffer;                                               /*  从第一个参数开始找          */
        do {
            pcStart = pcDiv;
            pcDiv   = lib_strchr(pcStart, ':');                         /*  查找下一个参数分割点        */
            if (pcDiv) {
                *pcDiv = PX_EOS;
                pcDiv++;
            }
            
            snprintf(pcPathBuffer, MAX_FILENAME_LENGTH, "%s/%s", 
                     pcStart, pcKeyword);                               /*  合并为完整的目录            */
            if (__ldPathIsFile(pcPathBuffer)) {                         /*  此文件可以被访问            */
                __SHEAP_FREE(pcBuffer);
                return  (LW_TRUE);
            }
        } while (pcDiv);
        
        __SHEAP_FREE(pcBuffer);
    }
    
    return  (LW_FALSE);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** 函数名称: __tshellUndef
** 功能描述: shell 未定义命令处理 (将可执行文件转交给 exec 命令执行)
** 输　入  : pcCmd             完整命令
**           pcKeyword         是否可以检测出关键字, 如果不能 pcKeyword 为空字串
**           ppcParam          重新定位的参数头
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __tshellUndef (PCHAR  pcCmd, PCHAR  pcKeyword, PCHAR  *ppcParam)
{
    *pcKeyword = PX_EOS;                                                /*  变量定义表达式              */
    return  (__tshellVarDefine(pcCmd));
}
/*********************************************************************************************************
** 函数名称: __tshellCloseRedir
** 功能描述: ttiny shell 关闭 wrapper 创建的重定向描述符
** 输　入  : iFd               文件描述符
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellCloseRedir (INT  iFd)
{
    if (iFd >= 0) {
        __KERNEL_SPACE_ENTER();                                         /*  这些文件创建时保证是内核里的*/
        close(iFd);
        __KERNEL_SPACE_EXIT();
    }
}
/*********************************************************************************************************
** 函数名称: __tshellRedir
** 功能描述: 分析重定向字串并打卡相关的文件
** 输　入  : pcString     重定向字串
**           pcRedir      重定向点
**           ulMe         当前线程
**           piPopCnt     POP 操作的个数
** 输　出  : 是否分析成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellRedir (PCHAR  pcString, PCHAR  pcRedir, LW_OBJECT_HANDLE ulMe, INT *piPopCnt)
{
    INT         iFd;
    INT         iFlag = O_CREAT | O_WRONLY;
    INT         iOutOpt;
    PCHAR       pcFile;
    
    if (*pcRedir == *(pcRedir + 1)) {                                   /*  追加打开                    */
        iFlag |= O_APPEND;
        pcFile = pcRedir + 2;                                           /*  定位文件                    */
        
    } else {
        iFlag |= O_TRUNC;
        pcFile = pcRedir + 1;
    }
    
    if (*pcFile == '&') {
        pcFile++;
    }
    
    if (*pcFile == PX_EOS) {                                            /*  定位文件错误                */
        return  (PX_ERROR);
    }
    
    if (*pcRedir == '>') {                                              /*  标准输出或标准错误          */
        *pcRedir =  PX_EOS;
        
        if (pcString == pcRedir) {
            iOutOpt  =  STD_OUT;
            
        } else if (*pcString == '&') {
            iOutOpt  =  STD_OUT | STD_ERR;
        
        } else if (*pcString == '1') {
            iOutOpt  =  STD_OUT;
        
        } else if (*pcString == '2') {
            iOutOpt  =  STD_ERR;
        
        } else {
            iOutOpt  =  STD_OUT;
        }
        
        if ((lib_strlen(pcFile) == 1) && (*pcFile >= '0') && (*pcFile <= '2')) {
            iFd = API_IoTaskStdGet(ulMe, (*pcFile - '0'));
            if (iOutOpt & STD_OUT) {
                API_IoTaskStdSet(ulMe, STD_OUT, iFd);
            }
            if (iOutOpt & STD_ERR) {
                API_IoTaskStdSet(ulMe, STD_ERR, iFd);
            }
        
        } else {
            iFd = open(pcFile, iFlag, DEFAULT_FILE_PERM);
            if (iFd < 0) {
                return  (PX_ERROR);
            }
            
            API_ThreadCleanupPush(__tshellCloseRedir, (PVOID)iFd);
            (*piPopCnt)++;
            
            if (iOutOpt & STD_OUT) {
                API_IoTaskStdSet(ulMe, STD_OUT, iFd);
            }
            if (iOutOpt & STD_ERR) {
                API_IoTaskStdSet(ulMe, STD_ERR, iFd);
            }
        }
    } else {                                                            /*  标准输入定位                */
        iFlag &= ~O_TRUNC;                                              /*  输入没有 O_TRUNC 操作       */
        iFd = open(pcFile, iFlag, DEFAULT_FILE_PERM);
        if (iFd < 0) {
            return  (PX_ERROR);
        }
        
        API_ThreadCleanupPush(__tshellCloseRedir, (PVOID)iFd);
        (*piPopCnt)++;
        
        API_IoTaskStdSet(ulMe, STD_IN, iFd);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellWrapper
** 功能描述: 运行 shell 命令的外壳
** 输　入  : pfuncCommand 系统命令指针
**           iArgC        参数个数
**           ppcArgV 参数表
** 输　出  : 命令返回值
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellWrapper (FUNCPTR  pfuncCommand, INT  iArgC, PCHAR  ppcArgV[])
{
    INT         i;
    INT         iPopCnt    = 0;
    INT         iOldStd[3] = {PX_ERROR, PX_ERROR, PX_ERROR};
    INT         iRealArgc  = iArgC;
    INT         iRet       = PX_ERROR;
    
    LW_OBJECT_HANDLE  ulMe = API_ThreadIdSelf();

    if (iArgC < 2) {                                                    /*  不包含重定向                */
        return  pfuncCommand(iArgC, ppcArgV);
    }
    
    for (i = 0; i < 3; i++) {                                           /*  保存旧的                    */
        iOldStd[i] = API_IoTaskStdGet(ulMe, i);
    }
    
    for (i = 1; i < iArgC; i++) {                                       /*  遍历参数                    */
        PCHAR   pcRedir = lib_strchr(ppcArgV[i], '<');
        
        if (pcRedir == LW_NULL) {
            pcRedir = lib_strchr(ppcArgV[i], '>');
            if (pcRedir == LW_NULL) {
                continue;
            }
        }
        
        if (__tshellRedir(ppcArgV[i], pcRedir, ulMe, &iPopCnt)) {       /*  分析并执行重定位操作        */
            fprintf(stderr, "can not process redirect.\n");
            goto    __ret;
        
        } else if (iRealArgc > i) {
            iRealArgc = i;                                              /*  重定位分析成功              */
        }
    }
    
    ppcArgV[iRealArgc] = LW_NULL;
    iRet = pfuncCommand(iRealArgc, ppcArgV);                            /*  执行命令                    */
    
__ret:
    for (i = 0; i < 3; i++) {                                           /*  还原旧的                    */
        API_IoTaskStdSet(ulMe, i, iOldStd[i]);
    }

    for (i = 0; i < iPopCnt; i++) {
        API_ThreadCleanupPop(LW_TRUE);
    }
    
    return  (iRet);
    
}
/*********************************************************************************************************
** 函数名称: __tshellChkBg
** 功能描述: 预处理背景执行相关参数 shell 命令背景执行参数
** 输　入  : pcCommand    命令字符串
**           pbNeedJoin   是否需要 JOIN.
**           pbNeedAsyn   使用需要异步执行.
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __tshellPreTreatedBg (PCHAR  cCommand, BOOL  *pbNeedJoin, BOOL  *pbNeedAsyn)
{
    size_t     stStrLen;
    PCHAR      pcTail = cCommand;
    
    if (pbNeedJoin) {
        *pbNeedJoin = LW_FALSE;
    }
    if (pbNeedAsyn) {
        *pbNeedAsyn = LW_FALSE;
    }

    __tshellStrDelCRLF(cCommand);                                       /*  删除 CR 与 LF 字符          */
    __tshellStrFormat(cCommand, cCommand);                              /*  整理 shell 命令             */
    
    stStrLen = lib_strlen(cCommand);
    if (stStrLen < 1) {
        return;
    }
    
    if (cCommand[stStrLen - 1] == __TTINY_SHELL_BG_JOIN) {              /*  命令行同步背景执行          */
        if (pbNeedJoin) {
            *pbNeedJoin = LW_TRUE;
        }
        pcTail = &cCommand[stStrLen - 1];
        
    } else if (cCommand[stStrLen - 1] == __TTINY_SHELL_BG_ASYNC) {      /*  命令行异步背景执行          */
        if (pbNeedAsyn) {
            *pbNeedAsyn = LW_TRUE;
        }
        pcTail = &cCommand[stStrLen - 1];
    }
    
    while (pcTail != cCommand) {                                        /*  去掉背景运行符号            */
        if ((*pcTail == __TTINY_SHELL_BG_JOIN) ||
            (*pcTail == __TTINY_SHELL_BG_ASYNC)) {
            *pcTail = PX_EOS;
            pcTail--;
        
        } else if (*pcTail == PX_EOS) {
            pcTail--;
        
        } else {
            break;
        }
    }
}
/*********************************************************************************************************
** 函数名称: __tshellExec
** 功能描述: ttiny shell 系统, 执行一条 shell 命令
** 输　入  : pcCommand    命令字符串
**           pfuncHook    提取关键字后调用的回调函数.
** 输　出  : 命令返回值(当发生错误时, 返回值为负值)
** 全局变量: 
** 调用模块: 
** 注  意  : 当 shell 检测出命令字符串错误时, 将会返回负值, 此值取相反数后即为错误编号.
*********************************************************************************************************/
INT  __tshellExec (CPCHAR  pcCommandExec, VOIDFUNCPTR  pfuncHook)
{
#define __TTINY_SHELL_CMD_ISWHITE(pcCmd)    \
        ((*(pcCmd) == ' ') || (*(pcCmd) == '\t') || (*(pcCmd) == '\r') || (*(pcCmd) == '\n'))

#define __TTINY_SHELL_CMD_ISEND(pcCmd)      (*(pcCmd) == PX_EOS)

    REGISTER INT        i;
    REGISTER PCHAR      pcCmd = (PCHAR)pcCommandExec;
    REGISTER size_t     stStrLen;
    REGISTER ULONG      ulError;
    
    PLW_CLASS_TCB       ptcbCur;
    __PTSHELL_KEYWORD   pskwNode = LW_NULL;
    
             CHAR       cCommandBuffer[LW_CFG_SHELL_MAX_COMMANDLEN + 1];/*  临时缓冲区                  */
             CHAR       cCommandBg[LW_CFG_SHELL_MAX_COMMANDLEN + 1];    /*  背景运行缓冲区              */
             
             CHAR       cKeyword[LW_CFG_SHELL_MAX_KEYWORDLEN + 1];      /*  关键字                      */
             PCHAR      pcParamList[LW_CFG_SHELL_MAX_PARAMNUM + 1];     /*  参数列表                    */
             
             PCHAR      pcParam = LW_NULL;                              /*  指向第一个参数的指针        */
             
             INT        iStdFd;                                         /*  背景执行标准文件            */
             BOOL       bIsJoin;                                        /*  背景执行是否同步            */
             CPCHAR     pcBgCmd;                                        /*  背景执行命令                */
             size_t     stBgCmdLen;                                     /*  背景执行长度                */
             ULONG      ulMagic;                                        /*  背景 magic 码               */
             
             BOOL       bCmdLineNeedJoin;                               /*  命令行是否要求同步背景执行  */
             BOOL       bCmdLineNeedAsyn;                               /*  命令行是否要求异步背景执行  */
             
    
    if (!pcCmd || __TTINY_SHELL_CMD_ISEND(pcCmd)) {                     /*  命令错误                    */
        return  (ERROR_NONE);
    }
    
    while (__TTINY_SHELL_CMD_ISWHITE(pcCmd)) {                          /*  过滤前面的不可见字符        */
        pcCmd++;
        if (__TTINY_SHELL_CMD_ISEND(pcCmd)) {
            return  (ERROR_NONE);                                       /*  不是有效的命令字            */
        }
    }
    
    if (*pcCmd == '#') {                                                /*  注释行直接忽略              */
        return  (ERROR_NONE);
    }
    
    stStrLen = lib_strnlen(pcCmd, LW_CFG_SHELL_MAX_COMMANDLEN + 1);     /*  计算字符串长短              */
    if ((stStrLen > LW_CFG_SHELL_MAX_COMMANDLEN - 1) || (stStrLen < 1)) {
        return  (-ERROR_TSHELL_EPARAM);                                 /*  字符串长度错误              */
    }
    
    ulError = __tshellStrConvertVar(pcCmd, cCommandBuffer);             /*  变量替换                    */
    if (ulError) {
        return  ((INT)(-ulError));                                      /*  替换错误                    */
    }
    
    __tshellPreTreatedBg(cCommandBuffer, 
                         &bCmdLineNeedJoin, &bCmdLineNeedAsyn);         /*  预处理背景执行相关参数      */
    
    lib_strcpy(cCommandBg, cCommandBuffer);                             /*  拷贝可能需要的背景执行命令  */
    
    ulError = __tshellStrKeyword(cCommandBuffer, cKeyword, &pcParam);   /*  提取关键字                  */
    if (ulError) {
        /*
         *  如果关键字过长, 这里可能为变量定义
         */
        ulError = __tshellVarDefine(cCommandBuffer);
        return  ((INT)(-ulError));                                      /*  提取错误                    */
    }
    
    /*
     *  如果需要, 调用 pfuncHook 函数.
     */
    if (pfuncHook) {
        PCHAR   pcStartAlpha = cKeyword;
        while ((lib_isalpha(*pcStartAlpha) == 0) && 
               (*pcStartAlpha != PX_EOS)) {                             /*  从第一个字符开始            */
            pcStartAlpha++;
        }
        if (*pcStartAlpha != PX_EOS) {
            pfuncHook(API_ThreadIdSelf(), pcStartAlpha);                /*  调用回调函数                */
        }
    }
    
    /*
     *  如果存在可执行文件则, 则优先运行文件.
     */
#if LW_CFG_MODULELOADER_EN > 0                                          /*  如果存在文件, 则优先运行    */
    if (__tshellCheckFile(cKeyword)) {                                  /*  有对应文件                  */
        lib_strcpy(cKeyword, "exec");                                   /*  使用 exec 转义命令序列      */
        pcParam = (PCHAR)cCommandBuffer;                                /*  从命令头起开始计算参数      */
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    /*
     *  查找关键字.
     */
    if (ERROR_NONE != __tshellKeywordFind(cKeyword, &pskwNode)) {       /*  查找关键字                  */
        ulError = __tshellUndef(cCommandBuffer, cKeyword, &pcParam);
        if (ulError != ERROR_NONE) {
            return  (-ERROR_TSHELL_CMDNOTFUND);                         /*  命令未找到                  */
        }
        return  (ERROR_NONE);                                           /*  变量定义或者赋值成功        */
    }
    
    /*
     *  检查注册命令是否需要背景执行
     */
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (((pskwNode->SK_ulOption & LW_OPTION_KEYWORD_SYNCBG) ||
         (pskwNode->SK_ulOption & LW_OPTION_KEYWORD_ASYNCBG)) &&
        __TTINY_SHELL_GET_MAGIC(ptcbCur) != (ULONG)pskwNode) {          /*  判断是否需要背景执行        */
        
        pcBgCmd    = cCommandBg;
        stBgCmdLen = lib_strlen(cCommandBg);
        ulMagic    = (ULONG)pskwNode;
        iStdFd     = API_IoTaskStdGet(API_ThreadIdSelf(), STD_OUT);     /*  获得标准文件                */
        
        /*
         *  如果背景执行方式与 shell 命令行方式冲突, 则优先使用命令行的方式
         */
        if (bCmdLineNeedJoin) {
            bIsJoin = LW_TRUE;
        
        } else if (bCmdLineNeedAsyn) {
            bIsJoin = LW_FALSE;
        
        } else {                                                        /*  命令行没有背景执行参数      */
            if (pskwNode->SK_ulOption & LW_OPTION_KEYWORD_SYNCBG) {
                bIsJoin = LW_TRUE;
            
            } else {
                bIsJoin = LW_FALSE;
            }
        }
        goto    __bg_run;
    
    } else {                                                            /*  命令本身非要求背景执行      */
        __TTINY_SHELL_SET_MAGIC(ptcbCur, 0ul);
        
        if (bCmdLineNeedJoin || bCmdLineNeedAsyn) {                     /*  命令行带有背景执行参数      */
            
            pcBgCmd    = cCommandBg;
            stBgCmdLen = lib_strlen(cCommandBg);
            ulMagic    = 0ul;
            iStdFd     = API_IoTaskStdGet(API_ThreadIdSelf(), STD_OUT); /*  获得标准文件                */
            
            if (bCmdLineNeedJoin) {
                bIsJoin = LW_TRUE;
            
            } else {
                bIsJoin = LW_FALSE;
            }
            
            goto    __bg_run;
        }
    }
    
    pcParamList[0] = cKeyword;
    if (!pcParam) {                                                     /*  检查是否存在参数            */
        REGISTER INT iRet = pskwNode->SK_pfuncCommand(1, pcParamList);  /*  执行命令,没有参数           */
        return  (iRet);
    }
    
    pcParamList[1] = pcParam;                                           /*  第一个参数地址              */
    for (i = 1; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {                   /*  开始查询参数                */
        __tshellStrGetToken(pcParamList[i], 
                            &pcParamList[i + 1]);
        __tshellStrDelTransChar(pcParamList[i], pcParamList[i]);        /*  删除转义字符与非转义引号    */
        if (pcParamList[i + 1] == LW_NULL) {                            /*  参数结束                    */
            break;
        }
    }
                                                                        /*  执行命令                    */
    return  (__tshellWrapper(pskwNode->SK_pfuncCommand, i + 1, pcParamList));
    
__bg_run:                                                               /*  异步执行命令                */
    return  (__tshellBgCreate(iStdFd, pcBgCmd, stBgCmdLen, 
                              pskwNode->SK_ulOption,
                              bIsJoin, ulMagic));                       /*  创建背景线程执行命令        */
}
/*********************************************************************************************************
** 函数名称: __tshellPromptFree
** 功能描述: ttiny shell 删除命令提示符缓存
** 输　入  : ptpc      命令提示符上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID __tshellPromptFree (__PTSHELL_PROMPT_CTX  ptpc)
{
    __SHEAP_FREE(ptpc);
}
/*********************************************************************************************************
** 函数名称: __tshellShowPrompt
** 功能描述: ttiny shell 显示命令提示符
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __tshellShowPrompt (VOID)
{
    CHAR                    cPrompt[MAX_FILENAME_LENGTH + 3];
    PCHAR                   pcSeparator;
    PLW_CLASS_TCB           ptcbCur;
    __PTSHELL_PROMPT_CTX    ptpc;
    CHAR                    cPrivilege;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    getwd(cPrompt);
    
    if (geteuid() == 0) {
        cPrivilege = '#';                                           /*  特权用户                        */
    } else {
        cPrivilege = '$';                                           /*  非特权用户                      */
    }
    
    if (__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_PROMPT_FULL) {
        PCHAR           pcUserName;
        CHAR            cHostName[HOST_NAME_MAX + 1] = "(none)";
    
        ptpc = (__PTSHELL_PROMPT_CTX)API_ThreadGetNotePad(API_ThreadIdSelf(), 0);
        if (ptpc == LW_NULL) {
            ptpc =  (__PTSHELL_PROMPT_CTX)__SHEAP_ALLOC(sizeof(__TSHELL_PROMPT_CTX));
            if (ptpc == LW_NULL) {
                goto    __simple;
            }
            API_ThreadCleanupPush(__tshellPromptFree, ptpc);
            API_ThreadSetNotePad(API_ThreadIdSelf(), 0, (ULONG)ptpc);
            
            ptpc->TPC_uid = getuid();
            ptpc->TPC_gid = getgid();
            
            if (API_TShellGetUserName(ptpc->TPC_uid, 
                                      ptpc->TPC_cUserName, 
                                      sizeof(ptpc->TPC_cUserName))) {
                lib_strcpy(ptpc->TPC_cUserName, "unknown");
            }
        }
        
        if (ptpc->TPC_uid != getuid()) {                                /*  有用户改变                  */
            ptpc->TPC_uid  = getuid();
            
            if (API_TShellGetUserName(ptpc->TPC_uid, 
                                      ptpc->TPC_cUserName, 
                                      sizeof(ptpc->TPC_cUserName))) {
                lib_strcpy(ptpc->TPC_cUserName, "unknown");
            }
        }
        pcUserName = ptpc->TPC_cUserName;
        gethostname(cHostName, sizeof(cHostName));
        
        pcSeparator = lib_strrchr(cPrompt, PX_DIVIDER);
        if (pcSeparator) {
            printf("[%s@%s:%s]%c ", pcUserName, cHostName, 
                                    cPrompt, cPrivilege);               /*  显示当前路径                */
        } else {
            printf("[%s@%s:/]%c ", pcUserName, cHostName, cPrivilege);
        }
    
    } else {
__simple:
        pcSeparator = lib_strrchr(cPrompt, PX_DIVIDER);
        if (pcSeparator) {
            printf("[%s]%c ", cPrompt, cPrivilege);                     /*  显示当前路径                */
        } else {
            printf("[/]%c ", cPrivilege);
        }
    }
    
    fflush(stdout);                                                     /*  保证 stdout 的输出          */
}
/*********************************************************************************************************
** 函数名称: __tshellRestartEx
** 功能描述: ttiny shell 线程重启
** 输　入  : ulThread      线程句柄
**           bNeedAuthen   是否需要登录
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __tshellRestartEx (LW_OBJECT_HANDLE  ulThread, BOOL  bNeedAuthen)
{
    REGISTER ULONG            ulOption;
    REGISTER PLW_CLASS_TCB    ptcbShell;
    REGISTER PLW_CLASS_TCB    ptcbJoin;
             LW_OBJECT_HANDLE ulJoin = LW_OBJECT_HANDLE_INVALID;
             INT              iMsg;
             UINT16           usIndex;

    if (LW_CPU_GET_CUR_NESTING()) {
        return  (_excJobAdd((VOIDFUNCPTR)__tshellRestartEx, 
                            (PVOID)ulThread, (PVOID)bNeedAuthen, 0, 0, 0, 0));
    }
    
    usIndex = _ObjectGetIndex(ulThread);
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    ptcbShell = __GET_TCB_FROM_INDEX(usIndex);
    ptcbJoin  = ptcbShell->TCB_ptcbJoin;
    if (ptcbJoin) {                                                     /*  等待其他线程结束            */
        if (bNeedAuthen) {
            __KERNEL_EXIT();                                            /*  退出内核                    */
            _ErrorHandle(EBUSY);
            return  (PX_ERROR);
        }
        ulJoin = ptcbJoin->TCB_ulId;
    
    } else {
        ulOption  = __TTINY_SHELL_GET_OPT(ptcbShell);
        if (bNeedAuthen) {
            ulOption &= ~LW_OPTION_TSHELL_NOLOGO;
            ulOption |= LW_OPTION_TSHELL_AUTHEN | LW_OPTION_TSHELL_LOOPLOGIN;
        
        } else {
            ulOption |= LW_OPTION_TSHELL_NOLOGO;                        /*  重启时不需要显示 logo       */
            ulOption &= ~LW_OPTION_TSHELL_AUTHEN;                       /*  重启时不需要用户认证        */
        }
        __TTINY_SHELL_SET_OPT(ptcbShell, ulOption);
    }
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    if (ulJoin) {
#if LW_CFG_SIGNAL_EN > 0
        kill(ulJoin, SIGKILL);                                          /*  杀死等待的线程/进程         */
#else
        API_ThreadDelete(&ulJoin, LW_NULL);
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
        iMsg = API_IoTaskStdGet(ulThread, STD_OUT);
        if (iMsg >= 0) {
            fdprintf(iMsg, "[sh]Warning: Program is killed (SIGKILL) by shell.\n"
                           "    Restart SylixOS is recommended!\n");
        }
    } else {                                                            /*  重启线程                    */
        API_ThreadRestart(ulThread, (PVOID)__TTINY_SHELL_GET_STDFILE(ptcbShell));
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellRestart
** 功能描述: ttiny shell 线程重启 (control-C)
** 输　入  : ulThread   线程句柄
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellRestart (LW_OBJECT_HANDLE  ulThread)
{
    __tshellRestartEx(ulThread, LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: __tshellRename
** 功能描述: ttiny shell 线程设置名字
** 输　入  : ulThread   线程句柄
**           pcName     新名字
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellRename (LW_OBJECT_HANDLE  ulThread, CPCHAR  pcName)
{
    CHAR    cNewName[LW_CFG_OBJECT_NAME_SIZE];
    PCHAR   pcNewStart = lib_rindex(pcName, PX_DIVIDER);
    
    if (pcNewStart) {
        pcNewStart++;
        lib_strlcpy(cNewName, pcNewStart, LW_CFG_OBJECT_NAME_SIZE);
    } else {
        lib_strlcpy(cNewName, pcName, LW_CFG_OBJECT_NAME_SIZE);
    }
    
    API_ThreadSetName(ulThread, cNewName);
}
/*********************************************************************************************************
** 函数名称: __tshellBackgroundCleanup
** 功能描述: ttiny shell 背景运行服务线程清除缓存信息函数
** 输　入  : pvArg      需要清除的内存
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellBackgroundCleanup (PVOID  pvArg)
{
    REGISTER __PTSHELL_BACKGROUND   tsbg = (__PTSHELL_BACKGROUND)pvArg;
             INT                    i;
             
    if (tsbg) {
        __KERNEL_SPACE_ENTER();                                         /*  这些文件创建时保证是内核里的*/
        for (i = 0; i < 3; i++) {
            if (tsbg->TSBG_bClosed[i]) {
                close(tsbg->TSBG_iFd[i]);                               /*  关闭对应的标准文件          */
            }
        }
        __KERNEL_SPACE_EXIT();
    
        if (tsbg->TSBG_pcDefPath) {
            __SHEAP_FREE(tsbg->TSBG_pcDefPath);                         /*  释放目录缓存                */
        }
        __SHEAP_FREE(tsbg);                                             /*  释放内存                    */
    }
}
/*********************************************************************************************************
** 函数名称: __tshellBackground
** 功能描述: ttiny shell 背景运行服务线程
** 输　入  : pcArg      服务终端文件
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PVOID  __tshellBackground (PVOID  pvArg)
{
    REGISTER __PTSHELL_BACKGROUND       tsbg = (__PTSHELL_BACKGROUND)pvArg;
             INT                        iRetValue;
             LW_OBJECT_HANDLE           ulMe = API_ThreadIdSelf();
    
    /*
     *  在运行到清除函数之前, 本线程不能被删除, 否则 pvArg 分配的内存将无法回收.
     */
    API_ThreadCleanupPush(__tshellBackgroundCleanup, pvArg);            /*  建立清除函数                */
    
    if (ioPrivateEnv() < 0) {                                           /*  使用私有线程 io 环境        */
        return  (LW_NULL);
    } else {
        chdir(tsbg->TSBG_pcDefPath);                                    /*  继承父系 io 当前目录        */
    }
    
    API_IoTaskStdSet(ulMe, STD_IN,  tsbg->TSBG_iFd[0]);                 /*  I/O 重定向                  */
    API_IoTaskStdSet(ulMe, STD_OUT, tsbg->TSBG_iFd[1]);
    API_IoTaskStdSet(ulMe, STD_ERR, tsbg->TSBG_iFd[2]);
    
    iRetValue = __tshellExec(tsbg->TSBG_cCommand,                       /*  执行 shell 指令             */
                             __tshellRename);                           /*  同时设置线程的名字          */
    if ((iRetValue < 0) && !tsbg->TSBG_bIsJoin) {                       /*  出现错误且sh没有等待此线程  */
        
        switch (iRetValue) {                                            /*  系统级错误提示              */
        
        case -ERROR_TSHELL_EPARAM:
            fprintf(stderr, "parameter(s) error.\n");
            break;
        
        case -ERROR_TSHELL_CMDNOTFUND:
            fprintf(stderr, "sh: command not found.\n");
            break;
            
        case -ERROR_TSHELL_EVAR:
            fprintf(stderr, "sh: variable error.\n");
            break;
            
        case -ERROR_SYSTEM_LOW_MEMORY:
            fprintf(stderr, "sh: no memory.\n");
            break;
        }
        
        API_TShellTermAlert(STD_OUT);                                   /*  产生响铃                    */
    }
    
    /*
     *  记录当前 shell 命令产生的错误.
     */
    __TTINY_SHELL_SET_ERROR(API_ThreadTcbSelf(), iRetValue);
    
    API_ThreadCleanupPop(LW_TRUE);                                      /*  运行清除函数                */
    
    return  ((PVOID)iRetValue);                                         /*  返回命令执行结果            */
}
/*********************************************************************************************************
** 函数名称: __tshellBgCreateEx
** 功能描述: ttiny shell 创建背景运行服务线程 (pid 继承调用者)
** 输　入  : iFd[3]         标准文件
**           pcCommand      需要执行的命令
**           stCommandLen   命令长度
**           ulKeywordOpt   关键字选项
**           bIsJoin        是否等待命令结束.
**           ulMagic        识别号
**           pulSh          背景线程句柄 (仅当 bIsJoin = LW_FALSE 时返回)
**           piRet          命令返回值
** 输　出  : 如果是同步执行, 则返回执行命令结果.
** 全局变量: 
** 调用模块: 
** 注  意  : 这里启动时为内核线程, 如果运行的命令需要转换为进程, 这里没有问题.
*********************************************************************************************************/
INT    __tshellBgCreateEx (INT               iFd[3],
                           BOOL              bClosed[3],
                           CPCHAR            pcCommand, 
                           size_t            stCommandLen, 
                           ULONG             ulKeywordOpt,
                           BOOL              bIsJoin,
                           ULONG             ulMagic,
                           LW_OBJECT_HANDLE *pulSh,
                           INT              *piRet)
{
    REGISTER __PTSHELL_BACKGROUND       tsbg;
             LW_CLASS_THREADATTR        threadattrTShell;
             LW_OBJECT_HANDLE           hTShellHandle;
             INT                        iRetValue = ERROR_NONE;
             
             PLW_CLASS_TCB              ptcbShellBg;
             PLW_CLASS_TCB              ptcbCur;
    REGISTER ULONG                      ulOption;
             ULONG                      ulTaskOpt = LW_CFG_SHELL_THREAD_OPTION | LW_OPTION_OBJECT_GLOBAL;
             size_t                     stSize;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    ulOption = __TTINY_SHELL_GET_OPT(ptcbCur);
    
    tsbg = (__PTSHELL_BACKGROUND)__SHEAP_ALLOC(sizeof(__TSHELL_BACKGROUND) + stCommandLen);
    if (!tsbg) {
        fprintf(stderr, "system low memory, can not create background thread.\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    stSize = lib_strlen(_PathGetDef()) + 1;
    tsbg->TSBG_pcDefPath = (PCHAR)__SHEAP_ALLOC(stSize);
    if (!tsbg->TSBG_pcDefPath) {
        __SHEAP_FREE(tsbg);
        fprintf(stderr, "system low memory, can not create background thread.\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_strlcpy(tsbg->TSBG_pcDefPath, _PathGetDef(), stSize);           /*  拷贝父系当前目录            */
    
    tsbg->TSBG_iFd[0] = iFd[0];
    tsbg->TSBG_iFd[1] = iFd[1];
    tsbg->TSBG_iFd[2] = iFd[2];
    
    tsbg->TSBG_bClosed[0] = bClosed[0];
    tsbg->TSBG_bClosed[1] = bClosed[1];
    tsbg->TSBG_bClosed[2] = bClosed[2];
    
    tsbg->TSBG_bIsJoin = bIsJoin;
    
    lib_strlcpy(tsbg->TSBG_cCommand, pcCommand, stCommandLen + 1);
    
#if LW_CFG_VMM_EN > 0
    if (ulKeywordOpt & LW_OPTION_KEYWORD_STK_MAIN) {
        ulTaskOpt |= LW_OPTION_THREAD_STK_MAIN;
    }
#endif
    
    API_ThreadAttrBuild(&threadattrTShell,
                        _G_stShellStackSize,                            /*  shell 堆栈大小              */
                        LW_PRIO_T_SHELL,
                        ulTaskOpt,
                        (PVOID)tsbg);                                   /*  构建属性块                  */
                        
    hTShellHandle = API_ThreadInit("t_tshellbg", __tshellBackground,
                                   &threadattrTShell, LW_NULL);         /*  创建 tshell 线程            */
    if (!hTShellHandle) {
        __SHEAP_FREE(tsbg->TSBG_pcDefPath);
        __SHEAP_FREE(tsbg);
        _DebugHandle(__ERRORMESSAGE_LEVEL, 
                     "tshellbg thread can not create.\r\n");
        _DebugHandle(__LOGMESSAGE_LEVEL, 
                     "ttniy shell system is not initialize.\r\n");
        return  (PX_ERROR);
    }
    
    ptcbShellBg = __GET_TCB_FROM_INDEX(_ObjectGetIndex(hTShellHandle));
    __TTINY_SHELL_SET_OPT(ptcbShellBg, ulOption);
    __TTINY_SHELL_SET_MAGIC(ptcbShellBg, ulMagic);                      /*  记录识别号                  */
    
    if (pulSh && (bIsJoin == LW_FALSE)) {
        *pulSh = hTShellHandle;
    
    } else if (pulSh) {
        *pulSh = LW_OBJECT_HANDLE_INVALID;
    }
    
    API_ThreadStartEx(hTShellHandle, bIsJoin, (PVOID *)&iRetValue);     /*  启动 shell 线程             */
    __TTINY_SHELL_SET_ERROR(ptcbCur, iRetValue);                        /*  记录 shell 命令产生的错误   */
    
    if (piRet) {
        *piRet = iRetValue;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellBgCreate
** 功能描述: ttiny shell 创建背景运行服务线程
** 输　入  : iFd            文件描述符
**           pcCommand      需要执行的命令
**           stCommandLen   命令长度
**           ulKeywordOpt   关键字选项
**           bIsJoin        是否等待命令结束.
**           ulMagic        识别号
** 输　出  : 如果是同步执行, 则返回执行命令结果.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT    __tshellBgCreate (INT     iFd, 
                                CPCHAR  pcCommand, 
                                size_t  stCommandLen, 
                                ULONG   ulKeywordOpt,
                                BOOL    bIsJoin,
                                ULONG   ulMagic)
{
    INT  iError;
    INT  iRet;
    INT  iFdArry[3];
    BOOL bClosed[3] = {LW_FALSE, LW_FALSE, LW_FALSE};                   /*  不关闭标准版文件            */
    
    iFdArry[0] = iFd;
    iFdArry[1] = iFd;
    iFdArry[2] = iFd;
    
    iError = __tshellBgCreateEx(iFdArry, bClosed, pcCommand, stCommandLen, 
                                ulKeywordOpt, bIsJoin, ulMagic, LW_NULL, &iRet);
    if (iError < 0) {
        return  (iError);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __tshellCloseFd
** 功能描述: ttiny shell 内核线程退出时需要检查是否需要关闭文件
** 输　入  : ptcb          shell TCB
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellCloseFd (PLW_CLASS_TCB  ptcb)
{
    INT     iFd;

    if (ptcb->TCB_iDeleteProcStatus == LW_TCB_DELETE_PROC_DEL) {        /*  正在被删除                  */
        if (__TTINY_SHELL_GET_OPT(ptcb) & LW_OPTION_TSHELL_CLOSE_FD) {
            iFd = __TTINY_SHELL_GET_STDFILE(ptcb);
            if (iFd >= 0) {
                close(iFd);
            }
        }
    }
}
/*********************************************************************************************************
** 函数名称: __tshellThread
** 功能描述: ttiny shell 服务线程
** 输　入  : pcArg      服务终端文件
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID   __tshellThread (PVOID  pcArg)
{
             FUNCPTR        pfuncRunCallback = LW_NULL;
             PLW_CLASS_TCB  ptcbCur;
    REGISTER INT            iTtyFd = (INT)pcArg;
             INT            iRetValue;
             CHAR           cRecvBuffer[LW_CFG_SHELL_MAX_COMMANDLEN + 1];
             CHAR           cCtrl[NCCS];
             INT            iReadNum;
             INT            iTotalNum = 0;
             BOOL           bIsCommandOver = LW_TRUE;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    __TTINY_SHELL_SET_STDFILE(ptcbCur, iTtyFd);                         /*  任务上下文中保存标准文件    */

    API_ThreadCleanupPush(__tshellCloseFd, ptcbCur);                    /*  初始化清除函数              */

    if (ioPrivateEnv() < 0) {                                           /*  使用私有线程 io 环境        */
        exit(-1);
    
    } else {
        /*
         *  TODO: control-C 重启 shell 线程后, 理论应该保持重启前的当前目录, 这里还没有支持.
         */
        if (API_TShellGetUserHome(getuid(), 
                                  cRecvBuffer, 
                                  LW_CFG_SHELL_MAX_COMMANDLEN + 1) == ERROR_NONE) {
            if (chdir(cRecvBuffer) < ERROR_NONE) {
                chdir(PX_STR_ROOT);
            }
        } else {
            chdir(PX_STR_ROOT);                                         /*  初始化为根目录              */
        }
    }
    
    if (iTtyFd >= 0) {
        LW_OBJECT_HANDLE  ulMe = API_ThreadIdSelf();
        API_IoTaskStdSet(ulMe, STD_IN,  iTtyFd);                        /*  I/O 重定向                  */
        API_IoTaskStdSet(ulMe, STD_OUT, iTtyFd);
        API_IoTaskStdSet(ulMe, STD_ERR, iTtyFd);
    }
    
    if (!isatty(iTtyFd)) {                                              /*  检测是否为终端设备          */
        exit(-1);
    }
    
    ioctl(iTtyFd, FIOSYNC);                                             /*  等待之前所有数据发送完毕    */
    ioctl(iTtyFd, FIORTIMEOUT, LW_NULL);                                /*  读取无超时时间              */
    
    if (__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_AUTHEN) {     /*  是否需要明文用户认证        */
        if (__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_LOOPLOGIN) {
__reauthen:
            if (__tshellUserAuthen(iTtyFd, LW_TRUE)) {                  /*  进行认证                    */
                goto    __reauthen;
            }
        } else {
            if (__tshellUserAuthen(iTtyFd, LW_FALSE)) {                 /*  进行认证                    */
                exit(-ERROR_TSHELL_EUSER);
            }
        }
    } else {
        printf("\n");
    }
    
    if (__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_NOECHO) {     /*  不需要回显                  */
        iRetValue = ioctl(iTtyFd, FIOSETOPTIONS, 
                          (OPT_TERMINAL & (~OPT_ECHO)));                /*  设置为终端模式              */
    } else {
        iRetValue = ioctl(iTtyFd, FIOSETOPTIONS, OPT_TERMINAL);         /*  设置为终端模式              */
    }
    if (iRetValue < 0) {
        perror("shell can not change into a TERMINAL mode");
        exit(-1);
    }
    
    iRetValue = ioctl(iTtyFd, FIORBUFSET, 
                      LW_OSIOD_LARG(LW_CFG_SHELL_MAX_COMMANDLEN + 2));  /*  设置接收缓冲区大小          */
                                                                        /*  为 ty 计数器保留 2 字节空间 */
    if (iRetValue < 0) {
        perror("shell can not change set read buffer size");
        exit(-1);
    }
    
    iRetValue = ioctl(iTtyFd, FIOWBUFSET, 
                      LW_OSIOD_LARG(LW_CFG_SHELL_MAX_COMMANDLEN));      /*  设置发送缓冲                */
    if (iRetValue < 0) {
        perror("shell can not change set write buffer size");
        exit(-1);
    }
    
    pfuncRunCallback = __TTINY_SHELL_GET_CALLBACK(ptcbCur);
    if (pfuncRunCallback) {
        iRetValue = pfuncRunCallback(iTtyFd);                           /*  调用启动回调函数            */
        if (iRetValue < 0) {
            perror("shell run callback fail");
            exit(-1);
        }
    }
    
    API_TShellSetTitel(STD_OUT, "SylixOS Terminal [t-tiny-shell]");     /*  修改标题设置                */
    API_TShellColorEnd(STD_OUT);
    
    if ((__TTINY_SHELL_GET_OPT(ptcbCur) & 
         LW_OPTION_TSHELL_NOLOGO) == 0) {                               /*  是否需要打印 logo           */
        API_SystemLogoPrint();                                          /*  打印 LOGO 信息              */
        API_SystemHwInfoPrint();
    }
    
    __TTINY_SHELL_SET_ERROR(ptcbCur, 0);                                /*  清空错误标记                */
    
#if LW_CFG_THREAD_CANCEL_EN > 0
    API_ThreadSetCancelType(LW_THREAD_CANCEL_ASYNCHRONOUS, LW_NULL);
#endif                                                                  /*  LW_CFG_THREAD_CANCEL_EN     */
    
    __TTINY_SHELL_SET_MAIN(ptcbCur);
    
    ioctl(iTtyFd, FIOABORTARG,  API_ThreadIdSelf());                    /*  control-C 参数              */
    ioctl(iTtyFd, FIOABORTFUNC, __tshellRestart);                       /*  control-C 行为              */
    ioctl(iTtyFd, FIOGETCC, cCtrl);                                     /*  获得控制字符                */
    
    for (;;) {
        if (bIsCommandOver) {                                           /*  单条命令结束后发送命令提示符*/
            __tshellShowPrompt();                                       /*  显示命令提示符              */
        
        } else {
            printf(">");                                                /*  链接命令符                  */
            fflush(stdout);                                             /*  保证 stdout 的输出          */
        }
        
        if (__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_NOECHO) {
            iReadNum = (INT)read(0, &cRecvBuffer[iTotalNum], 
                                 LW_CFG_SHELL_MAX_COMMANDLEN - 
                                 iTotalNum);                            /*  接收字符串                  */
        } else {
            iReadNum = (INT)__tshellReadline(0, &cRecvBuffer[iTotalNum], 
                                             LW_CFG_SHELL_MAX_COMMANDLEN - 
                                             iTotalNum);                /*  接收字符串                  */
        }
        
        if (iReadNum > 0) {
            iTotalNum += iReadNum;                                      /*  记录总个数                  */
            if (cRecvBuffer[iTotalNum - 1] == '\n') {
                cRecvBuffer[iTotalNum - 1] =  PX_EOS;
                iTotalNum -= 1;
                if (cRecvBuffer[iTotalNum - 1] == '\r') {               /*  过滤 \r\n                   */
                    cRecvBuffer[iTotalNum - 1] =  PX_EOS;
                    iTotalNum -= 1;
                }
            }
            if (cRecvBuffer[iTotalNum - 1] == '\\') {                   /*  用户需要续写命令            */
                iTotalNum -= 1;
                bIsCommandOver = LW_FALSE;
                if ((LW_CFG_SHELL_MAX_COMMANDLEN - iTotalNum) <= 1) {   /*  没有空闲, 不能接收          */
                    printf("command is too long.\n");
                    iTotalNum      = 0;                                 /*  该命令失效                  */
                    bIsCommandOver = LW_TRUE;
                } else {
                    continue;                                           /*  等待继续输入                */
                }
            }
        }
        
        if (iTotalNum > 0) {
            if (lib_strstr(cRecvBuffer, __TTINY_SHELL_FORCE_ABORT)) {
                break;                                                  /*  强制退出 shell              */
            }
            
            optind   = 1;                                               /*  确保 getopt() 执行正确      */
            optreset = 1;
            
            iRetValue = API_TShellExec(cRecvBuffer);                    /*  执行 shell 指令             */
            if (iRetValue < 0) {
                switch (iRetValue) {                                    /*  系统级错误提示              */
                
                case -ERROR_TSHELL_EPARAM:
                    fprintf(stderr, "parameter(s) error.\n");
                    break;
                
                case -ERROR_TSHELL_CMDNOTFUND:
                    fprintf(stderr, "sh: command not found.\n");
                    break;
                    
                case -ERROR_TSHELL_EVAR:
                    fprintf(stderr, "sh: variable error.\n");
                    break;
                    
                case -ERROR_SYSTEM_LOW_MEMORY:
                    fprintf(stderr, "sh: no memory.\n");
                    break;
                }
                
                API_TShellTermAlert(STD_OUT);                           /*  产生响铃                    */
            }
            __TTINY_SHELL_SET_ERROR(ptcbCur, iRetValue);                /*  记录当前命令产生的错误.     */

            fpurge(stdin);                                              /*  清空输入缓存                */

            if (__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_NOECHO) {
                ioctl(iTtyFd, FIOSETOPTIONS, (OPT_TERMINAL & (~OPT_ECHO)));
                
            } else {                                                    /*  重新恢复为行模式            */
                ioctl(iTtyFd, FIOSETOPTIONS, OPT_TERMINAL);
            }
            
            ioctl(iTtyFd, FIOSETCC, cCtrl);                             /*  恢复之前的控制字符          */
        }
        
        iTotalNum      = 0;
        bIsCommandOver = LW_TRUE;
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __tshellKeywordAdd
** 功能描述: 向 ttiny shell 系统添加一个关键字.
** 输　入  : pcKeyword     关键字
**           stStrLen      关键字的长度
**           pfuncCommand  执行的 shell 函数
**           ulOption      选项
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __tshellKeywordAdd (CPCHAR  pcKeyword, size_t  stStrLen, 
                           PCOMMAND_START_ROUTINE  pfuncCommand, ULONG   ulOption)
{
    REGISTER __PTSHELL_KEYWORD    pskwNode         = LW_NULL;           /*  关键字节点                  */
    REGISTER PLW_LIST_LINE       *pplineHashHeader = LW_NULL;
    REGISTER INT                  iHashVal;
    
    /*
     *  分配内存
     */
    pskwNode = (__PTSHELL_KEYWORD)__SHEAP_ALLOC(sizeof(__TSHELL_KEYWORD));
    if (!pskwNode) {                                                    /*  分配失败                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (ERROR_SYSTEM_LOW_MEMORY);                              /*  缺少内存                    */
    }
    
    pskwNode->SK_pcKeyword = (PCHAR)__SHEAP_ALLOC(stStrLen + 1);
    if (!pskwNode->SK_pcKeyword) {
        __SHEAP_FREE(pskwNode);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (ERROR_SYSTEM_LOW_MEMORY);                              /*  缺少内存                    */
    }
    
    iHashVal = __hashHorner(pcKeyword, LW_CFG_SHELL_KEY_HASH_SIZE);     /*  确定一阶散列的位置          */
    
    __TTINY_SHELL_LOCK();                                               /*  互斥访问                    */
    /*
     *  填写信息
     */
    pskwNode->SK_pfuncCommand   = pfuncCommand;                         /*  记录回调函数                */
    pskwNode->SK_ulOption       = ulOption;                             /*  记录命令选项                */
    pskwNode->SK_pcFormatString = LW_NULL;                              /*  格式化字串                  */
    pskwNode->SK_pcHelpString   = LW_NULL;                              /*  帮助字段                    */
    lib_strlcpy(pskwNode->SK_pcKeyword, pcKeyword, 
                (LW_CFG_SHELL_MAX_KEYWORDLEN + 1));                     /*  复制关键字                  */

    /*
     *  插入管理链表
     */
    _List_Line_Add_Ahead(&pskwNode->SK_lineManage, 
                         &_G_plineTSKeyHeader);                         /*  插入管理表头                */
     
    /*
     *  插入哈希表
     */
    pplineHashHeader = &_G_plineTSKeyHeaderHashTbl[iHashVal];
    
    _List_Line_Add_Ahead(&pskwNode->SK_lineHash, 
                         pplineHashHeader);                             /*  插入相应的表                */
    
    __TTINY_SHELL_UNLOCK();                                             /*  释放资源                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellKeywordFine
** 功能描述: 在 ttiny shell 系统查找一个关键字.
** 输　入  : pcKeyword     关键字
**           ppskwNode     关键字节点双指针
** 输　出  : 0: 表示执行成功 -1: 表示执行失败
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __tshellKeywordFind (CPCHAR  pcKeyword, __PTSHELL_KEYWORD   *ppskwNode)
{
    REGISTER PLW_LIST_LINE        plineHash;
    REGISTER __PTSHELL_KEYWORD    pskwNode = LW_NULL;                   /*  关键字节点                  */
    REGISTER INT                  iHashVal;

    iHashVal = __hashHorner(pcKeyword, LW_CFG_SHELL_KEY_HASH_SIZE);     /*  确定一阶散列的位置          */
    
    __TTINY_SHELL_LOCK();                                               /*  互斥访问                    */
    plineHash = _G_plineTSKeyHeaderHashTbl[iHashVal];
    for (;
         plineHash != LW_NULL;
         plineHash  = _list_line_get_next(plineHash)) {
        
        pskwNode = _LIST_ENTRY(plineHash, __TSHELL_KEYWORD, 
                               SK_lineHash);                            /*  获得控制块                  */
        
        if (lib_strcmp(pcKeyword, pskwNode->SK_pcKeyword) == 0) {       /*  关键字相同                  */
            break;
        }
    }

    if (plineHash == LW_NULL) {
        __TTINY_SHELL_UNLOCK();                                         /*  释放资源                    */
        _ErrorHandle(ERROR_TSHELL_EKEYWORD);
        return  (ERROR_TSHELL_EKEYWORD);                                /*  没有找到关键字              */
    }

    *ppskwNode = pskwNode;

    __TTINY_SHELL_UNLOCK();                                             /*  释放资源                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellKeywordList
** 功能描述: 获得链表中的所有关键字控制块地址
** 输　入  : pskwNodeStart   起始节点地址, NULL 表示从头开始
**           ppskwNode[]     节点列表
**           iMaxCounter     列表中可以存放的最大节点数量
** 输　出  : 真实获取的节点数量
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __tshellKeywordList (__PTSHELL_KEYWORD   pskwNodeStart,
                            __PTSHELL_KEYWORD   ppskwNode[],
                            INT                 iMaxCounter)
{
    REGISTER INT                  i = 0;
    
    REGISTER PLW_LIST_LINE        plineNode;
    REGISTER __PTSHELL_KEYWORD    pskwNode = pskwNodeStart;             /*  关键字节点                  */
    
    __TTINY_SHELL_LOCK();                                               /*  互斥访问                    */
    if (pskwNode == LW_NULL) {
        plineNode = _G_plineTSKeyHeader;
    } else {
        plineNode = _list_line_get_next(&pskwNode->SK_lineManage);
    }
    
    for (;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode)) {                 /*  开始搜索                    */
        
        pskwNode = _LIST_ENTRY(plineNode, __TSHELL_KEYWORD, 
                               SK_lineManage);                          /*  获得控制块                  */
        
        ppskwNode[i++] = pskwNode;                                      /*  保存                        */
        
        if (i >= iMaxCounter) {                                         /*  已经保存满了                */
            break;
        }
    }
    __TTINY_SHELL_UNLOCK();                                             /*  释放资源                    */
    
    return  (i);
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
