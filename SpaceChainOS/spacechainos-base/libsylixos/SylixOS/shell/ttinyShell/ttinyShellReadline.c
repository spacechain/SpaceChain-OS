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
** 文   件   名: ttinyShellReadline.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 03 月 25 日
**
** 描        述: shell 从终端读取一条命令.
                 支持目录匹配补齐, 任意位置插入, 删除, 历史操作记录.
                 
** BUG:
2012.10.19  修正 __tshellCharTab() 参数列表缓存个数错误, 应该为 LW_CFG_SHELL_MAX_PARAMNUM + 1.
2014.02.24  tab 补齐时, 进行相似度补齐.
2014.10.29  tab 补齐时, 显示的单行宽度通过 TIOCGWINSZ 获取.
2016.05.27  tab 支持匹配 HOME 目录.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
#include "sys/ioctl.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "ttinyShellLib.h"
#include "ttinyShellSysCmd.h"
#include "ttinyString.h"
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
extern VOID  __tshellShowPrompt(VOID);
/*********************************************************************************************************
  特殊功能键
*********************************************************************************************************/
#define __KEY_IS_ESC(c)             (c == 27)
#define __KEY_IS_EOT(c)             (c ==  4)
#define __KEY_IS_TAB(c)             (c ==  9)
#define __KEY_IS_LF(c)              (c == 10)
#define __KEY_IS_BS(c)              (c == 127 || c == 8)

#define __FUNC_KEY_LEFT             68
#define __FUNC_KEY_UP               65
#define __FUNC_KEY_RIGHT            67
#define __FUNC_KEY_DOWN             66
#define __FUNC_KEY_HOME             49
#define __FUNC_KEY_DELETE           51
#define __FUNC_KEY_END              52
/*********************************************************************************************************
  特殊功能键回传
*********************************************************************************************************/
#define __KEY_WRITE_LEFT(fd)        write(fd, "\x1b\x5b\x44", 3)
#define __KEY_WRITE_UP(fd)          write(fd, "\x1b\x5b\x41", 3)
#define __KEY_WRITE_RIGHT(fd)       write(fd, "\x1b\x5b\x43", 3)
#define __KEY_WRITE_DOWN(fd)        write(fd, "\x1b\x5b\x42", 3)
/*********************************************************************************************************
  简化
*********************************************************************************************************/
#define CTX_TAB                     psicContext->SIC_uiTabCnt
#define CTX_CURSOR                  psicContext->SIC_uiCursor
#define CTX_TOTAL                   psicContext->SIC_uiTotalLen
#define CTX_BUFFER                  psicContext->SIC_cInputBuffer
/*********************************************************************************************************
  当前输入上下文
*********************************************************************************************************/
typedef struct {
    UINT        SIC_uiCursor;
    UINT        SIC_uiTotalLen;
    CHAR        SIC_cInputBuffer[LW_CFG_SHELL_MAX_COMMANDLEN + 1];
} __SHELL_INPUT_CTX;
typedef __SHELL_INPUT_CTX       *__PSHELL_INPUT_CTX;
/*********************************************************************************************************
  输入历史信息
*********************************************************************************************************/
#define __INPUT_SAVE_MAX            20
typedef struct {
    LW_LIST_RING_HEADER          SIHC_pringHeader;
    UINT                         SIHC_uiCounter;
} __SHELL_HISTORY_CTX;
typedef __SHELL_HISTORY_CTX     *__PSHELL_HISTORY_CTX;

typedef struct {
    LW_LIST_RING                 SIH_ringManage;
    CHAR                         SIH_cInputSave[1];
} __SHELL_HISTORY;
typedef __SHELL_HISTORY         *__PSHELL_HISTORY;
/*********************************************************************************************************
** 函数名称: __tshellReadlineClean
** 功能描述: shell 退出时清除 readline 信息.
** 输　入  : ulId                          线程 ID
**           pvRetVal                      返回值
**           ptcbDel                       删除的 TCB
** 输　出  : 读取个数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __tshellReadlineClean (LW_OBJECT_HANDLE  ulId, PVOID  pvRetVal, PLW_CLASS_TCB  ptcbDel)
{
    __PSHELL_HISTORY_CTX    psihc = __TTINY_SHELL_GET_HIS(ptcbDel);
    __PSHELL_HISTORY        psihHistory;
    
    if (psihc) {
        while (psihc->SIHC_pringHeader) {
            psihHistory = _LIST_ENTRY(psihc->SIHC_pringHeader, __SHELL_HISTORY, SIH_ringManage);
            _List_Ring_Del(&psihHistory->SIH_ringManage, 
                           &psihc->SIHC_pringHeader);
            __SHEAP_FREE(psihHistory);
        }
        
        __SHEAP_FREE(psihc);
        __TTINY_SHELL_SET_HIS(ptcbDel, LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: __tshellReadlineInit
** 功能描述: shell 初始化 readline.
** 输　入  : NONE
** 输　出  : 初始化是否成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellReadlineInit (VOID)
{
    PLW_CLASS_TCB           ptcbCur;
    __PSHELL_HISTORY_CTX    psihc;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psihc = __TTINY_SHELL_GET_HIS(ptcbCur);
    
    if (psihc == LW_NULL) {
        psihc = (__PSHELL_HISTORY_CTX)__SHEAP_ALLOC(sizeof(__SHELL_HISTORY_CTX));
        if (psihc == LW_NULL) {
            fprintf(stderr, "read line tool no memory!\n");
            return  (PX_ERROR);
        }
        lib_bzero(psihc, sizeof(__SHELL_HISTORY_CTX));
        __TTINY_SHELL_SET_HIS(ptcbCur, psihc);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellTtyInputHistorySave
** 功能描述: shell 记录一条输入.
** 输　入  : psicContext           输入上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellHistorySave (__PSHELL_INPUT_CTX  psicContext)
{
    PLW_CLASS_TCB           ptcbCur;
    __PSHELL_HISTORY_CTX    psihc;
    __PSHELL_HISTORY        psihHistory;
    PLW_LIST_RING           pring;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psihc = __TTINY_SHELL_GET_HIS(ptcbCur);
    
    pring = psihc->SIHC_pringHeader;
    while (pring) {                                                     /*  循环查找是否与以前的相同    */
        psihHistory = _LIST_ENTRY(pring, __SHELL_HISTORY, SIH_ringManage);
        
        if (lib_strcmp(psihHistory->SIH_cInputSave, CTX_BUFFER) == 0) { /*  如果相同                    */
            
            if (pring == psihc->SIHC_pringHeader) {
                return;                                                 /*  如果是表头, 则不需要处理    */
            
            } else {
                _List_Ring_Del(&psihHistory->SIH_ringManage,
                               &psihc->SIHC_pringHeader);
                _List_Ring_Add_Ahead(&psihHistory->SIH_ringManage,
                                     &psihc->SIHC_pringHeader);         /*  放在表头位置                */
                return;
            }
        }
        pring = _list_ring_get_next(pring);
        if (pring == psihc->SIHC_pringHeader) {
            break;
        }
    }
    
    psihHistory = (__PSHELL_HISTORY)__SHEAP_ALLOC(sizeof(__SHELL_HISTORY) + 
                                                  lib_strlen(CTX_BUFFER));
    if (psihHistory) {
        lib_strcpy(psihHistory->SIH_cInputSave, CTX_BUFFER);
        _List_Ring_Add_Ahead(&psihHistory->SIH_ringManage,
                             &psihc->SIHC_pringHeader);                 /*  加入新的输入                */
        psihc->SIHC_uiCounter++;
        
        if (psihc->SIHC_uiCounter > __INPUT_SAVE_MAX) {                 /*  需要删除最老的一条          */
            PLW_LIST_RING   pringPrev = _list_ring_get_prev(psihc->SIHC_pringHeader);
            psihHistory = _LIST_ENTRY(pringPrev, __SHELL_HISTORY, SIH_ringManage);
            _List_Ring_Del(&psihHistory->SIH_ringManage,
                           &psihc->SIHC_pringHeader);
            __SHEAP_FREE(psihHistory);
            psihc->SIHC_uiCounter--;
        }
    }
}
/*********************************************************************************************************
** 函数名称: __tshellTtyInputHistoryGet
** 功能描述: shell 获取一条输入历史.
** 输　入  : bNext                 向前还是向后
**           ppvCookie             上次获取的位置
**           psicContext           当前输入相上下文
** 输　出  : 是否获取成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  __tshellHistoryGet (BOOL  bNext, PVOID  *ppvCookie, __PSHELL_INPUT_CTX  psicContext)
{
    PLW_CLASS_TCB           ptcbCur;
    __PSHELL_HISTORY_CTX    psihc;
    __PSHELL_HISTORY        psihHistory = (__PSHELL_HISTORY)*ppvCookie;
    PLW_LIST_RING           pringGet;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psihc = __TTINY_SHELL_GET_HIS(ptcbCur);

    if (psihc->SIHC_pringHeader == LW_NULL) {                           /*  没有历史记录                */
        return  (LW_FALSE);
    }

    if (psihHistory == LW_NULL) {
        pringGet = psihc->SIHC_pringHeader;
        
    } else {
        if (bNext) {
            pringGet = _list_ring_get_next(&psihHistory->SIH_ringManage);
        } else {
            pringGet = _list_ring_get_prev(&psihHistory->SIH_ringManage);
        }
    }

    psihHistory = _LIST_ENTRY(pringGet, __SHELL_HISTORY, SIH_ringManage);
    lib_strlcpy(CTX_BUFFER, psihHistory->SIH_cInputSave, LW_CFG_SHELL_MAX_COMMANDLEN);
    *ppvCookie = (PVOID)psihHistory;
    
    return  (LW_TRUE);
}
/*********************************************************************************************************
** 函数名称: __tshellTtyCursorMoveLeft
** 功能描述: shell 光标左移.
** 输　入  : iFd                           文件描述符
**           iNum                          左移位数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellTtyCursorMoveLeft (INT  iFd, INT  iNum)
{
    fdprintf(iFd, "\x1b[%dD", iNum);
}
/*********************************************************************************************************
** 函数名称: __tshellTtyCursorMoveRight
** 功能描述: shell 光标右移.
** 输　入  : iFd                           文件描述符
**           iNum                          右移位数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellTtyCursorMoveRight (INT  iFd, INT  iNum)
{
    fdprintf(iFd, "\x1b[%dC", iNum);
}
/*********************************************************************************************************
** 函数名称: __tshellTtyClearEndLine
** 功能描述: shell 从光标处删除本行.
** 输　入  : iFd                           文件描述符
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellTtyClearEndLine (INT  iFd)
{
    fdprintf(iFd, "\x1b[K");
}
/*********************************************************************************************************
** 函数名称: __tshellCharLeft
** 功能描述: shell 收到左方向键.
** 输　入  : iFd                           文件描述符
**           psicContext                   当前输入上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellCharLeft (INT  iFd, __PSHELL_INPUT_CTX  psicContext)
{
    if (CTX_CURSOR > 0) {
        CTX_CURSOR--;
        __KEY_WRITE_LEFT(iFd);
    }
}
/*********************************************************************************************************
** 函数名称: __tshellCharRight
** 功能描述: shell 收到右方向键.
** 输　入  : iFd                           文件描述符
**           psicContext                   当前输入上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellCharRight (INT  iFd, __PSHELL_INPUT_CTX  psicContext)
{
    if (CTX_CURSOR < CTX_TOTAL) {
        CTX_CURSOR++;
        __KEY_WRITE_RIGHT(iFd);
    }
}
/*********************************************************************************************************
** 函数名称: __tshellCharUp
** 功能描述: shell 收到上方向键.
** 输　入  : iFd                           文件描述符
**           ppvCookie                     上一次的位置
**           psicContext                   当前输入上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellCharUp (INT  iFd, PVOID *ppvCookie, __PSHELL_INPUT_CTX  psicContext)
{
    BOOL    bGetOk = __tshellHistoryGet(LW_TRUE, ppvCookie, psicContext);
                                                                        /*  最近的在表头, 后面的按顺序  */
    if (bGetOk) {
        if (CTX_CURSOR > 0) {
            __tshellTtyCursorMoveLeft(iFd, CTX_CURSOR);
        }
        __tshellTtyClearEndLine(iFd);
        CTX_TOTAL  = (UINT)lib_strlen(CTX_BUFFER);
        write(iFd, CTX_BUFFER, CTX_TOTAL);
        CTX_CURSOR = CTX_TOTAL;
    }
}
/*********************************************************************************************************
** 函数名称: __tshellCharDown
** 功能描述: shell 收到下方向键.
** 输　入  : iFd                           文件描述符
**           ppvCookie                     上一次的位置
**           psicContext                   当前输入上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellCharDown (INT  iFd, PVOID *ppvCookie, __PSHELL_INPUT_CTX  psicContext)
{
    BOOL    bGetOk = __tshellHistoryGet(LW_FALSE, ppvCookie, psicContext);
                                                                        /*  反序                        */
    if (bGetOk) {
        if (CTX_CURSOR > 0) {
            __tshellTtyCursorMoveLeft(iFd, CTX_CURSOR);
        }
        __tshellTtyClearEndLine(iFd);
        CTX_TOTAL  = (UINT)lib_strlen(CTX_BUFFER);
        write(iFd, CTX_BUFFER, CTX_TOTAL);
        CTX_CURSOR = CTX_TOTAL;
    }
}
/*********************************************************************************************************
** 函数名称: __tshellCharBackspace
** 功能描述: shell 收到退格键.
** 输　入  : iFd                           文件描述符
**           cChar                         键值
**           psicContext                   当前输入上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellCharBackspace (INT  iFd, CHAR  cChar, __PSHELL_INPUT_CTX  psicContext)
{
    CHAR    cBsCharList[3];
    
    if (CTX_CURSOR == 0) {
        return;
    }
    
    if (CTX_CURSOR == CTX_TOTAL) {
        cBsCharList[0] = cChar;
        cBsCharList[1] = ' ';
        cBsCharList[2] = cChar;
        write(iFd, cBsCharList, 3);
        CTX_CURSOR--;
        CTX_TOTAL--;
    
    } else if (CTX_CURSOR < CTX_TOTAL) {
        write(iFd, &cChar, 1);
        lib_memcpy(&CTX_BUFFER[CTX_CURSOR - 1], 
                   &CTX_BUFFER[CTX_CURSOR], 
                   CTX_TOTAL - CTX_CURSOR);
        CTX_BUFFER[CTX_TOTAL - 1] = ' ';
        CTX_CURSOR--;
        write(iFd, &CTX_BUFFER[CTX_CURSOR], CTX_TOTAL - CTX_CURSOR);
        __tshellTtyCursorMoveLeft(iFd, CTX_TOTAL - CTX_CURSOR);
        CTX_TOTAL--;
    }
}
/*********************************************************************************************************
** 函数名称: __tshellCharDelete
** 功能描述: shell 收到一个 del 键值.
** 输　入  : iFd                           文件描述符
**           psicContext                   当前输入上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellCharDelete (INT  iFd, __PSHELL_INPUT_CTX  psicContext)
{
    if (CTX_CURSOR < CTX_TOTAL) {
        lib_memcpy(&CTX_BUFFER[CTX_CURSOR], 
                   &CTX_BUFFER[CTX_CURSOR + 1], 
                   CTX_TOTAL - CTX_CURSOR);
        CTX_BUFFER[CTX_TOTAL - 1] = ' ';
        write(iFd, &CTX_BUFFER[CTX_CURSOR], CTX_TOTAL - CTX_CURSOR);
        __tshellTtyCursorMoveLeft(iFd, CTX_TOTAL - CTX_CURSOR);
        CTX_TOTAL--;
    }
}
/*********************************************************************************************************
** 函数名称: __tshellCharHome
** 功能描述: shell 收到一个 home 键值.
** 输　入  : iFd                           文件描述符
**           psicContext                   当前输入上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellCharHome (INT  iFd, __PSHELL_INPUT_CTX  psicContext)
{
    if (CTX_CURSOR > 0) {
        __tshellTtyCursorMoveLeft(iFd, CTX_CURSOR);
        CTX_CURSOR = 0;
    }
}
/*********************************************************************************************************
** 函数名称: __tshellCharEnd
** 功能描述: shell 收到一个 end 键值.
** 输　入  : iFd                           文件描述符
**           psicContext                   当前输入上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellCharEnd (INT  iFd, __PSHELL_INPUT_CTX  psicContext)
{
    if (CTX_CURSOR < CTX_TOTAL) {
        __tshellTtyCursorMoveRight(iFd, CTX_TOTAL - CTX_CURSOR);
        CTX_CURSOR = CTX_TOTAL;
    }
}
/*********************************************************************************************************
** 函数名称: __tshellCharInster
** 功能描述: shell 收到一个普通键值.
** 输　入  : iFd                           文件描述符
**           cChar                         键值
**           psicContext                   当前输入上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellCharInster (INT  iFd, CHAR  cChar, __PSHELL_INPUT_CTX  psicContext)
{
    if (CTX_CURSOR == CTX_TOTAL) {
        write(iFd, &cChar, 1);
        CTX_BUFFER[CTX_CURSOR] = cChar;
        CTX_CURSOR++;
        CTX_TOTAL++;

    } else if (CTX_CURSOR < CTX_TOTAL) {
        lib_memcpy(&CTX_BUFFER[CTX_CURSOR + 1], 
                   &CTX_BUFFER[CTX_CURSOR], 
                   CTX_TOTAL - CTX_CURSOR);
                   
        CTX_BUFFER[CTX_CURSOR] = cChar;
        CTX_TOTAL++;
        write(iFd, &CTX_BUFFER[CTX_CURSOR], CTX_TOTAL - CTX_CURSOR);
        CTX_CURSOR++;
        __tshellTtyCursorMoveLeft(iFd, CTX_TOTAL - CTX_CURSOR);
    }
}
/*********************************************************************************************************
** 函数名称: __fillWhite
** 功能描述: 补充若干个空格
** 输　入  : stLen      空格的长度
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __fillWhite (size_t  stLen)
{
    CHAR    cFmt[16];
    
    sprintf(cFmt, "%%-%zds", stLen);
    printf(cFmt, "");                                                   /*  补充空格                    */
}
/*********************************************************************************************************
** 函数名称: __similarLen
** 功能描述: 从左侧开始查询两个字符串相似的字符数
** 输　入  : pcStr1     字符串1
**           pcStr2     字符串2
** 输　出  : 相似的个数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static size_t  __similarLen (CPCHAR  pcStr1, CPCHAR  pcStr2)
{
    size_t  stSimilar = 0;

    while (*pcStr1 == *pcStr2) {
        if (*pcStr1 == PX_EOS) {
            break;
        }
        pcStr1++;
        pcStr2++;
        stSimilar++;
    }
    
    return  (stSimilar);
}
/*********************************************************************************************************
** 函数名称: __tshellFileMatch
** 功能描述: shell 根据当前输入情况进行匹配.
** 输　入  : iFd                           文件描述符
**           pcDir                         文件夹
**           pcFileName                    文件名
**           psicContext                   当前输入上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellFileMatch (INT  iFd, PCHAR  pcDir, PCHAR  pcFileName, 
                                __PSHELL_INPUT_CTX  psicContext)
{
#define __TSHELL_BYTES_PERLINE          80                              /*  单行 80 字符                */
#define __TSHELL_BYTES_PERFILE          16                              /*  每个文件名显示格长度        */

    UINT             uiMath = 0;
    BOOL             bMathPrint = LW_FALSE;
    
    INT              iError;
    
    size_t           stPrintLen;
    size_t           stTotalLen = 0;
    size_t           stPad;
    size_t           stFileNameLen = lib_strlen(pcFileName);
    
    CHAR             cStat[MAX_FILENAME_LENGTH];
    CHAR             cHome[MAX_FILENAME_LENGTH];
    
    struct stat      statGet;
    struct winsize   winsz;
    
    struct dirent    direntcMatch;
    struct dirent   *pdirent;
    DIR             *pdir;
    size_t           stDirLen;
    
    size_t           stMinSimilar = 0;
    size_t           stSimilar;
    
    
    if (*pcDir == '~') {                                                /*  需要使用 HOME 目录替代      */
        if (API_TShellGetUserHome(getuid(), cHome, 
                                  MAX_FILENAME_LENGTH) == ERROR_NONE) {
            lib_strlcat(cHome, pcDir + 1, MAX_FILENAME_LENGTH);
            pcDir = cHome;
        }
    }
    
    pdir = opendir(pcDir);                                              /*  无法打开目录                */
    if (pdir == LW_NULL) {
        return;
    }
    
    if (ioctl(STD_OUT, TIOCGWINSZ, &winsz)) {                           /*  获得窗口信息                */
        winsz.ws_col = __TSHELL_BYTES_PERLINE;
    } else {
        winsz.ws_col = (unsigned short)ROUND_DOWN(winsz.ws_col, __TSHELL_BYTES_PERFILE);
    }
    
    stDirLen = lib_strlen(pcDir);
    
    do {
        pdirent = readdir(pdir);
        if (pdirent) {
            if ((stFileNameLen == 0) ||
                (lib_strncmp(pcFileName, pdirent->d_name, stFileNameLen) == 0)) {
                
                uiMath++;
                if (uiMath > 1) {                                       /*  只打印匹配结果              */
                    if (uiMath == 2) {
                        printf("\n");                                   /*  换行                        */
                    }
                    
                    stSimilar = __similarLen(direntcMatch.d_name, pdirent->d_name);
                    if (stSimilar < stMinSimilar) {
                        stMinSimilar = stSimilar;                       /*  保存最小相似度              */
                    }
                    
__print_dirent:
                    if (pcDir[stDirLen - 1] != PX_DIVIDER) {
                        snprintf(cStat, MAX_FILENAME_LENGTH, "%s/%s", pcDir, pdirent->d_name);
                    } else {
                        snprintf(cStat, MAX_FILENAME_LENGTH, "%s%s", pcDir, pdirent->d_name);
                    }
                    iError = stat(cStat, &statGet);
                    if (iError < 0) {
                        statGet.st_mode = DEFAULT_FILE_PERM | S_IFCHR;  /*  默认属性                    */
                    }
                    
                    if (S_ISDIR(statGet.st_mode)) {
                        lib_strlcat(pdirent->d_name, PX_STR_DIVIDER, MAX_FILENAME_LENGTH);
                    }
                    
                    API_TShellColorStart(pdirent->d_name, "", statGet.st_mode, STD_OUT);
                    stPrintLen = printf("%-15s ", pdirent->d_name);     /*  打印文件名                  */
                    if (stPrintLen > __TSHELL_BYTES_PERFILE) {
                        stPad = ROUND_UP(stPrintLen, __TSHELL_BYTES_PERFILE)
                              - stPrintLen;                             /*  计算填充数量                */
                        __fillWhite(stPad);
                    } else {
                        stPad = 0;
                    }
                    stTotalLen += stPrintLen + stPad;
                    API_TShellColorEnd(STD_OUT);
                    
                    if (stTotalLen >= winsz.ws_col) {
                        printf("\n");                                   /*  换行                        */
                        stTotalLen = 0;
                    }
                
                    if (bMathPrint == LW_FALSE) {                       /*  是否也需要把第一次匹配的打印*/
                        bMathPrint =  LW_TRUE;
                        pdirent    = &direntcMatch;
                        goto    __print_dirent;
                    
                    } else {                                            /*  否则记录上一次的结果        */
                        lib_strcpy(direntcMatch.d_name, pdirent->d_name);
                    }
                
                } else {
                    direntcMatch = *pdirent;                            /*  记录第一次 math 的节点      */
                    stMinSimilar = lib_strlen(pdirent->d_name);
                }
            }
        }
    } while (pdirent);
    
    closedir(pdir);
    
    if (uiMath > 1) {                                                   /*  多个匹配                    */
        if (stTotalLen) {
            printf("\n");                                               /*  结束本行                    */
        }
        __tshellShowPrompt();                                           /*  显示命令提示符              */
        
        if (stMinSimilar > stFileNameLen) {
            direntcMatch.d_name[stMinSimilar] = PX_EOS;                 /*  自动匹配输入至相似处        */
            lib_strlcat(CTX_BUFFER, 
                        &direntcMatch.d_name[stFileNameLen],
                        LW_CFG_SHELL_MAX_COMMANDLEN);
            CTX_TOTAL  = lib_strlen(CTX_BUFFER);
            CTX_CURSOR = CTX_TOTAL;
        }
        write(iFd, CTX_BUFFER, CTX_TOTAL);
        
    } else if (uiMath == 1) {                                           /*  仅有一个匹配                */
        size_t  stCatLen;
        
        if (pcDir[stDirLen - 1] != PX_DIVIDER) {
            snprintf(cStat, MAX_FILENAME_LENGTH, "%s/%s", pcDir, direntcMatch.d_name);
        } else {
            snprintf(cStat, MAX_FILENAME_LENGTH, "%s%s", pcDir, direntcMatch.d_name);
        }
        iError = stat(cStat, &statGet);
        if (iError < 0) {
            statGet.st_mode = DEFAULT_FILE_PERM | S_IFCHR;              /*  默认属性                    */
        }
        
        if (S_ISDIR(statGet.st_mode)) {                                 /*  如果是目录                  */
            lib_strlcat(direntcMatch.d_name, PX_STR_DIVIDER, 
                        MAX_FILENAME_LENGTH);                           /*  加一个目录结束符            */
        }
        stCatLen = lib_strlen(direntcMatch.d_name) - stFileNameLen;
        
        write(iFd, &direntcMatch.d_name[stFileNameLen], stCatLen);
        
        lib_strlcat(CTX_BUFFER, &direntcMatch.d_name[stFileNameLen], LW_CFG_SHELL_MAX_COMMANDLEN);
        CTX_TOTAL  += (UINT)stCatLen;
        CTX_CURSOR  = CTX_TOTAL;
    }
}
/*********************************************************************************************************
** 函数名称: __tshellCharTab
** 功能描述: shell 收到一个 tab 按键.
** 输　入  : iFd                           文件描述符
**           psicContext                   当前输入上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellCharTab (INT  iFd, __PSHELL_INPUT_CTX  psicContext)
{
#define __TTINY_SHELL_CMD_ISWHITE(pcCmd)    \
        ((*(pcCmd) == ' ') || (*(pcCmd) == '\t') || (*(pcCmd) == '\r') || (*(pcCmd) == '\n'))

#define __TTINY_SHELL_CMD_ISEND(pcCmd)      (*(pcCmd) == PX_EOS)

             INT         i;
             PCHAR       pcCmd;
             size_t      stStrLen;
             CHAR        cCommandBuffer[MAX_FILENAME_LENGTH];
             PCHAR       pcParamList[LW_CFG_SHELL_MAX_PARAMNUM + 1];    /*  参数列表                    */
             
             PCHAR       pcDir;
             PCHAR       pcFileName;
    REGISTER ULONG       ulError;
    
    if (CTX_CURSOR < CTX_TOTAL) {                                       /*  将光标移动到行末            */
        __tshellTtyCursorMoveRight(iFd, CTX_TOTAL - CTX_CURSOR);
        CTX_CURSOR = CTX_TOTAL;
    }
    CTX_BUFFER[CTX_TOTAL] = PX_EOS;                                     /*  假设命令结束                */
    
    pcCmd = CTX_BUFFER;
    
    if (!pcCmd || __TTINY_SHELL_CMD_ISEND(pcCmd)) {                     /*  命令错误                    */
        return;
    }
    
    while (__TTINY_SHELL_CMD_ISWHITE(pcCmd)) {                          /*  过滤前面的不可见字符        */
        pcCmd++;
        if (__TTINY_SHELL_CMD_ISEND(pcCmd)) {
            return;                                                     /*  不是有效的命令字            */
        }
    }
    
    if (*pcCmd == '#') {                                                /*  注释行直接忽略              */
        return;
    }
    
    stStrLen = lib_strnlen(pcCmd, LW_CFG_SHELL_MAX_COMMANDLEN + 1);     /*  计算字符串长短              */
    if ((stStrLen > LW_CFG_SHELL_MAX_COMMANDLEN - 1) ||
        (stStrLen < 1)) {                                               /*  字符串长度错误              */
        return;
    }
    
    lib_bzero(cCommandBuffer, LW_CFG_SHELL_MAX_COMMANDLEN);             /*  清空 cCommandBuffer 缓冲区  */
    
    ulError = __tshellStrConvertVar(CTX_BUFFER, cCommandBuffer);        /*  变量替换                    */
    if (ulError) {
        return;
    }
    __tshellStrFormat(cCommandBuffer, cCommandBuffer);                  /*  整理 shell 命令             */
    
    pcParamList[0] = cCommandBuffer;
    for (i = 0; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {                   /*  开始查询参数                */
        __tshellStrGetToken(pcParamList[i], 
                            &pcParamList[i + 1]);
        __tshellStrDelTransChar(pcParamList[i], pcParamList[i]);        /*  删除转义字符与非转义引号    */
        if (pcParamList[i + 1] == LW_NULL) {                            /*  参数结束                    */
            break;
        }
    }
    
    pcDir = pcParamList[i];                                             /*  仅分析最后一个字段          */
    if (pcDir == LW_NULL) {
        return;
    }
    
    if (lib_strlen(pcDir) == 0) {                                       /*  没有内容, 当前目录          */
        pcDir = ".";
        pcFileName = "";
        __tshellFileMatch(iFd, pcDir, pcFileName, psicContext);         /*  显示目录下指定匹配的文件    */
        return;
    }
    
    pcFileName = lib_rindex(pcDir, PX_DIVIDER);
    if (pcFileName == LW_NULL) {                                        /*  当前目录                    */
        pcFileName = pcDir;
        pcDir = ".";

    } else {
        if (pcFileName == pcDir) {                                      /*  根目录下第一级目录          */
            pcDir = PX_STR_DIVIDER;
            pcFileName++;
        
        } else {
            *pcFileName = PX_EOS;
            pcFileName++;
        }
    }
    
    __tshellFileMatch(iFd, pcDir, pcFileName, psicContext);             /*  显示目录下指定匹配的文件    */
}
/*********************************************************************************************************
** 函数名称: __tshellReadline
** 功能描述: shell 从终端读取一条命令.
** 输　入  : iFd                           文件描述符
**           pvBuffer                      接收缓冲区
**           stMaxBytes                    接收缓冲区大小
** 输　出  : 读取个数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  __tshellReadline (INT  iFd, PVOID  pcBuffer, size_t  stSize)
{
#define __CHK_READ_OUT(res)     if (sstReadNum <= 0) {  \
                                    goto    __out;  \
                                }

    INT                         iOldOption = OPT_TERMINAL;
    INT                         iNewOption;
    ssize_t                     sstReadNum;
    __SHELL_INPUT_CTX           sicContext;
    CHAR                        cRead;
    PVOID                       pvCookie = LW_NULL;
    
    if (__tshellReadlineInit() < ERROR_NONE) {                          /*  初始化当前线程 Readline相关 */
        return  (0);
    }
    
    sicContext.SIC_uiCursor   = 0;
    sicContext.SIC_uiTotalLen = 0;
    sicContext.SIC_cInputBuffer[0] = PX_EOS;

    ioctl(iFd, FIOGETOPTIONS, &iOldOption);
    iNewOption = iOldOption & ~(OPT_ECHO | OPT_LINE);                   /*  no echo no line mode        */
    ioctl(iFd, FIOSETOPTIONS, iNewOption);

    while (sicContext.SIC_uiTotalLen < LW_CFG_SHELL_MAX_COMMANDLEN) {
        sstReadNum = read(iFd, &cRead, 1);
        __CHK_READ_OUT(sstReadNum);
        
__re_check_key:
        if (__KEY_IS_ESC(cRead)) {                                      /*  功能键起始                  */
            sstReadNum = read(iFd, &cRead, 1);
            __CHK_READ_OUT(sstReadNum);
            
            if (cRead == 91) {                                          /*  方向键中间键                */
                sstReadNum = read(iFd, &cRead, 1);
                __CHK_READ_OUT(sstReadNum);
                
                switch (cRead) {
                
                case __FUNC_KEY_LEFT:                                   /*  ←                          */
                    __tshellCharLeft(iFd, &sicContext);
                    break;
                
                case __FUNC_KEY_UP:                                     /*  ↑                          */
                    __tshellCharUp(iFd, &pvCookie, &sicContext);
                    break;
                    
                case __FUNC_KEY_RIGHT:                                  /*  →                          */
                    __tshellCharRight(iFd, &sicContext);
                    break;
                
                case __FUNC_KEY_DOWN:                                   /*  ↓                          */
                    __tshellCharDown(iFd, &pvCookie, &sicContext);
                    break;
                    
                case __FUNC_KEY_HOME:                                   /*  home key                    */
                    __tshellCharHome(iFd, &sicContext);
                    sstReadNum = read(iFd, &cRead, 1);
                    __CHK_READ_OUT(sstReadNum);
                    if (cRead != 126) {
                        goto    __re_check_key;
                    }
                    break;
                
                case __FUNC_KEY_DELETE:                                 /*  delete key                  */
                    __tshellCharDelete(iFd, &sicContext);
                    sstReadNum = read(iFd, &cRead, 1);
                    __CHK_READ_OUT(sstReadNum);
                    if (cRead != 126) {
                        goto    __re_check_key;
                    }
                    break;
                
                case __FUNC_KEY_END:                                    /*  end key                     */
                    __tshellCharEnd(iFd, &sicContext);
                    sstReadNum = read(iFd, &cRead, 1);
                    __CHK_READ_OUT(sstReadNum);
                    if (cRead != 126) {
                        goto    __re_check_key;
                    }
                    break;
                }
            } else {
                goto    __re_check_key;
            }
        
        } else if (__KEY_IS_TAB(cRead)) {                               /*  tab                         */
            __tshellCharTab(iFd, &sicContext);
        
        } else if (__KEY_IS_BS(cRead)) {                                /*  backspace                   */
            __tshellCharBackspace(iFd, cRead, &sicContext);
        
        } else if (__KEY_IS_LF(cRead)) {                                /*  结束                        */
            write(iFd, &cRead, 1);                                      /*  echo                        */
            break;
        
        } else if (__KEY_IS_EOT(cRead)) {                               /*  CTL+D 强制退出              */
            lib_strcpy(sicContext.SIC_cInputBuffer, 
                       __TTINY_SHELL_FORCE_ABORT);
            sicContext.SIC_uiTotalLen = (UINT)lib_strlen(__TTINY_SHELL_FORCE_ABORT);
            break;
        
        } else {
            __tshellCharInster(iFd, cRead, &sicContext);
        }
    }
    sicContext.SIC_cInputBuffer[sicContext.SIC_uiTotalLen] = PX_EOS;    /*  命令结束                    */
    
    if (lib_strlen(sicContext.SIC_cInputBuffer)) {
        __tshellHistorySave(&sicContext);                               /*  记录历史命令                */
    }

__out:
    ioctl(iFd, FIOSETOPTIONS, iOldOption);                              /*  恢复以前终端状态            */
    
    lib_strlcpy((PCHAR)pcBuffer, sicContext.SIC_cInputBuffer, stSize);
    
    return  ((ssize_t)sicContext.SIC_uiTotalLen);
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
