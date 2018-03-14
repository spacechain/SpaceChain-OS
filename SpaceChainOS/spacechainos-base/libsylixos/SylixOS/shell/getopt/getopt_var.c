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
** 文   件   名: getopt_var.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 08 月 16 日
**
** 描        述: 兼容 POSIX 的 getopt 解析 argc argv 参数标准库.(全局变量抽象)

** BUG:
2009.12.15  修改注释.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "../SylixOS/shell/ttinyShell/ttinyShell.h"
#include "../SylixOS/shell/ttinyShell/ttinyShellLib.h"
#include "getopt_var.h"
#include "getopt.h"
/*********************************************************************************************************
  注意: 多数 posix 兼容操作系统提供的 getopt() 和 getopt_long() 是多线程不可冲入函数, 因为这两个函数只是
        为了分析进程的入口参数而设计的. 而在多线程环境中需要考虑重入性问题, 所以这里对全局变量的保护使用了
        类似 errno 的方法.
*********************************************************************************************************/
/*********************************************************************************************************
  ARGOPT 上下文
*********************************************************************************************************/
typedef struct {
    INT     SAO_iOptErr;                                                /*  错误信息                    */
    INT     SAO_iOptInd;                                                /*  argv的当前索引值            */
    INT     SAO_iOptOpt;                                                /*  无效选项字符                */
    INT     SAO_iOptReset;                                              /*  复位选项                    */
    PCHAR   SAO_pcOptArg;                                               /*  当前选项参数字串            */
    /*
     *  以下内部使用
     */
    CHAR    SAO_cEMsg[1];
    PCHAR   SAO_pcEMsg;
    PCHAR   SAO_pcPlace;
    INT     SAO_iNonoptStart;
    INT     SAO_iNonoptEnd;
} __TSHELL_ARGOPT;
typedef __TSHELL_ARGOPT     *__PTSHELL_ARGOPT;
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static const __TSHELL_ARGOPT _G_saoStd = {1, 1, '?', 0, LW_NULL, {0}, LW_NULL, LW_NULL, -1, -1}; 
                                                                        /*  标准初始化模板              */
static       __TSHELL_ARGOPT _G_saoTmp = {1, 1, '?', 0, LW_NULL, {0}, LW_NULL, LW_NULL, -1, -1}; 
                                                                        /*  无法开辟内存使用此变量      */
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define __TTINY_SHELL_GET_ARTOPT(ptcb)              (__PTSHELL_ARGOPT)(ptcb->TCB_shc.SHC_ulGetOptCtx)
#define __TTINY_SHELL_SET_ARTOPT(ptcb, pvAddr)      ptcb->TCB_shc.SHC_ulGetOptCtx = (addr_t)pvAddr
/*********************************************************************************************************
** 函数名称: __tshellOptNonoptEnd
** 功能描述: 获得 ARGOPT 上下文中的 SAO_INonoptEnd 地址
** 输　入  : NONE
** 输　出  : SAO_INonoptEnd 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  *__tshellOptNonoptEnd (VOID)
{
    REGISTER INT                *piNonoptEnd;
    REGISTER __PTSHELL_ARGOPT    psaoGet;
             PLW_CLASS_TCB       ptcbCur;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    __TTINY_SHELL_LOCK();                                               /*  shell 互斥                  */
    if (LW_NULL == __TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        psaoGet =  (__PTSHELL_ARGOPT)__SHEAP_ALLOC(sizeof(__TSHELL_ARGOPT));
        if (!psaoGet) {
            _G_saoTmp.SAO_pcPlace = _G_saoTmp.SAO_cEMsg;
            _G_saoTmp.SAO_pcEMsg  = _G_saoTmp.SAO_cEMsg;
            piNonoptEnd = &_G_saoTmp.SAO_iNonoptEnd;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "\"int nonopt_end\" system low memory.\r\n");
        } else {
            __TTINY_SHELL_SET_ARTOPT(ptcbCur, psaoGet);
            *psaoGet = _G_saoStd;
            psaoGet->SAO_pcPlace = psaoGet->SAO_cEMsg;
            psaoGet->SAO_pcEMsg  = psaoGet->SAO_cEMsg;
            piNonoptEnd = &psaoGet->SAO_iNonoptEnd;
        }
    } else {
        psaoGet  = __TTINY_SHELL_GET_ARTOPT(ptcbCur);
        piNonoptEnd = &psaoGet->SAO_iNonoptEnd;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  解除互斥                    */
    
    return  (piNonoptEnd);
}
/*********************************************************************************************************
** 函数名称: __tshellOptNonoptStart
** 功能描述: 获得 ARGOPT 上下文中的 SAO_iNonoptStart 地址
** 输　入  : NONE
** 输　出  : SAO_iNonoptStart 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  *__tshellOptNonoptStart (VOID)
{
    REGISTER INT                *piNonoptStart;
    REGISTER __PTSHELL_ARGOPT    psaoGet;
             PLW_CLASS_TCB       ptcbCur;
             
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    __TTINY_SHELL_LOCK();                                               /*  shell 互斥                  */
    if (LW_NULL == __TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        psaoGet =  (__PTSHELL_ARGOPT)__SHEAP_ALLOC(sizeof(__TSHELL_ARGOPT));
        if (!psaoGet) {
            _G_saoTmp.SAO_pcPlace = _G_saoTmp.SAO_cEMsg;
            _G_saoTmp.SAO_pcEMsg  = _G_saoTmp.SAO_cEMsg;
            piNonoptStart = &_G_saoTmp.SAO_iNonoptStart;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "\"int nonopt_start\" system low memory.\r\n");
        } else {
            __TTINY_SHELL_SET_ARTOPT(ptcbCur, psaoGet);
            *psaoGet = _G_saoStd;
            psaoGet->SAO_pcPlace = psaoGet->SAO_cEMsg;
            psaoGet->SAO_pcEMsg  = psaoGet->SAO_cEMsg;
            piNonoptStart = &psaoGet->SAO_iNonoptStart;
        }
    } else {
        psaoGet  = __TTINY_SHELL_GET_ARTOPT(ptcbCur);
        piNonoptStart = &psaoGet->SAO_iNonoptStart;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  解除互斥                    */
    
    return  (piNonoptStart);
}
/*********************************************************************************************************
** 函数名称: __tshellOptPlace
** 功能描述: 获得 ARGOPT 上下文中的 SAO_pcPlace 地址
** 输　入  : NONE
** 输　出  : SAO_pcPlace 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PCHAR   *__tshellOptPlace (VOID)
{
    REGISTER PCHAR              *ppcPlace;
    REGISTER __PTSHELL_ARGOPT    psaoGet;
             PLW_CLASS_TCB       ptcbCur;
             
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    __TTINY_SHELL_LOCK();                                               /*  shell 互斥                  */
    if (LW_NULL == __TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        psaoGet =  (__PTSHELL_ARGOPT)__SHEAP_ALLOC(sizeof(__TSHELL_ARGOPT));
        if (!psaoGet) {
            _G_saoTmp.SAO_pcPlace = _G_saoTmp.SAO_cEMsg;
            _G_saoTmp.SAO_pcEMsg  = _G_saoTmp.SAO_cEMsg;
            ppcPlace = &_G_saoTmp.SAO_pcPlace;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "\"int place\" system low memory.\r\n");
        } else {
            __TTINY_SHELL_SET_ARTOPT(ptcbCur, psaoGet);
            *psaoGet  = _G_saoStd;
            psaoGet->SAO_pcPlace = psaoGet->SAO_cEMsg;
            psaoGet->SAO_pcEMsg  = psaoGet->SAO_cEMsg;
            ppcPlace = &psaoGet->SAO_pcPlace;
        }
    } else {
        psaoGet  = __TTINY_SHELL_GET_ARTOPT(ptcbCur);
        ppcPlace = &psaoGet->SAO_pcPlace;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  解除互斥                    */
    
    return  (ppcPlace);
}
/*********************************************************************************************************
** 函数名称: __tshellOptEMsg
** 功能描述: 获得 ARGOPT 上下文中的 SAO_cEMsg 地址
** 输　入  : NONE
** 输　出  : SAO_cEMsg 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PCHAR  *__tshellOptEMsg (VOID)
{
    REGISTER PCHAR              *ppcEMsg;
    REGISTER __PTSHELL_ARGOPT    psaoGet;
             PLW_CLASS_TCB       ptcbCur;
             
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    __TTINY_SHELL_LOCK();                                               /*  shell 互斥                  */
    if (LW_NULL == __TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        psaoGet =  (__PTSHELL_ARGOPT)__SHEAP_ALLOC(sizeof(__TSHELL_ARGOPT));
        if (!psaoGet) {
            _G_saoTmp.SAO_pcPlace = _G_saoTmp.SAO_cEMsg;
            _G_saoTmp.SAO_pcEMsg  = _G_saoTmp.SAO_cEMsg;
            ppcEMsg = &_G_saoTmp.SAO_pcEMsg;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "\"char  EMSG[1]\" system low memory.\r\n");
        } else {
            __TTINY_SHELL_SET_ARTOPT(ptcbCur, psaoGet);
            *psaoGet = _G_saoStd;
            psaoGet->SAO_pcPlace = psaoGet->SAO_cEMsg;
            psaoGet->SAO_pcEMsg  = psaoGet->SAO_cEMsg;
            ppcEMsg  = &psaoGet->SAO_pcEMsg;
        }
    } else {
        psaoGet = __TTINY_SHELL_GET_ARTOPT(ptcbCur);
        ppcEMsg = &psaoGet->SAO_pcEMsg;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  解除互斥                    */
    
    return  (ppcEMsg);
}
/*********************************************************************************************************
** 函数名称: API_TShellOptErr
** 功能描述: 相当于 int opterr 这个变量.
**           非零时，getopt()函数为“无效选项”和“缺少参数选项，并输出其错误信息.
** 输　入  : NONE
** 输　出  : getopt 上下文中相应地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  *API_TShellOptErr (VOID)
{
    REGISTER INT                *piOptErr;
    REGISTER __PTSHELL_ARGOPT    psaoGet;
             PLW_CLASS_TCB       ptcbCur;
             
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    __TTINY_SHELL_LOCK();                                               /*  shell 互斥                  */
    if (LW_NULL == __TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        psaoGet =  (__PTSHELL_ARGOPT)__SHEAP_ALLOC(sizeof(__TSHELL_ARGOPT));
        if (!psaoGet) {
            _G_saoTmp.SAO_pcPlace = _G_saoTmp.SAO_cEMsg;
            _G_saoTmp.SAO_pcEMsg  = _G_saoTmp.SAO_cEMsg;
            piOptErr = &_G_saoTmp.SAO_iOptErr;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "\"int opterr\" system low memory.\r\n");
        } else {
            __TTINY_SHELL_SET_ARTOPT(ptcbCur, psaoGet);
            *psaoGet = _G_saoStd;
            psaoGet->SAO_pcPlace = psaoGet->SAO_cEMsg;
            psaoGet->SAO_pcEMsg  = psaoGet->SAO_cEMsg;
            piOptErr = &psaoGet->SAO_iOptErr;
        }
    } else {
        psaoGet  = __TTINY_SHELL_GET_ARTOPT(ptcbCur);
        piOptErr = &psaoGet->SAO_iOptErr;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  解除互斥                    */
    
    return  (piOptErr);
}
/*********************************************************************************************************
** 函数名称: API_TShellOptInd
** 功能描述: 相当于 int optind 这个变量.
**           argv的当前索引值。当getopt()在while循环中使用时，循环结束后，剩下的字串视为操作数，
**           在argv[optind]至argv[argc-1]中可以找到。
** 输　入  : NONE
** 输　出  : getopt 上下文中相应地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  *API_TShellOptInd (VOID)
{
    REGISTER INT                *piOptInd;
    REGISTER __PTSHELL_ARGOPT    psaoGet;
             PLW_CLASS_TCB       ptcbCur;
             
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    __TTINY_SHELL_LOCK();                                               /*  shell 互斥                  */
    if (LW_NULL == __TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        psaoGet =  (__PTSHELL_ARGOPT)__SHEAP_ALLOC(sizeof(__TSHELL_ARGOPT));
        if (!psaoGet) {
            _G_saoTmp.SAO_pcPlace = _G_saoTmp.SAO_cEMsg;
            _G_saoTmp.SAO_pcEMsg  = _G_saoTmp.SAO_cEMsg;
            piOptInd = &_G_saoTmp.SAO_iOptInd;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "\"int optind\" system low memory.\r\n");
        } else {
            __TTINY_SHELL_SET_ARTOPT(ptcbCur, psaoGet);
            *psaoGet = _G_saoStd;
            psaoGet->SAO_pcPlace = psaoGet->SAO_cEMsg;
            psaoGet->SAO_pcEMsg  = psaoGet->SAO_cEMsg;
            piOptInd = &psaoGet->SAO_iOptInd;
        }
    } else {
        psaoGet  = __TTINY_SHELL_GET_ARTOPT(ptcbCur);
        piOptInd = &psaoGet->SAO_iOptInd;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  解除互斥                    */
    
    return  (piOptInd);
}
/*********************************************************************************************************
** 函数名称: API_TShellOptOpt
** 功能描述: 相当于 int optopt 这个变量.
**           当发现无效选项字符之时，getopt()函数或返回'?'字符，或返回':'字符，
**           并且optopt包含了所发现的无效选项字符。
** 输　入  : NONE
** 输　出  : getopt 上下文中相应地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  *API_TShellOptOpt (VOID)
{
    REGISTER INT                *piOptOpt;
    REGISTER __PTSHELL_ARGOPT    psaoGet;
             PLW_CLASS_TCB       ptcbCur;
             
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    __TTINY_SHELL_LOCK();                                               /*  shell 互斥                  */
    if (LW_NULL == __TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        psaoGet =  (__PTSHELL_ARGOPT)__SHEAP_ALLOC(sizeof(__TSHELL_ARGOPT));
        if (!psaoGet) {
            _G_saoTmp.SAO_pcPlace = _G_saoTmp.SAO_cEMsg;
            _G_saoTmp.SAO_pcEMsg  = _G_saoTmp.SAO_cEMsg;
            piOptOpt = &_G_saoTmp.SAO_iOptOpt;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "\"int optopt\" system low memory.\r\n");
        } else {
            __TTINY_SHELL_SET_ARTOPT(ptcbCur, psaoGet);
            *psaoGet = _G_saoStd;
            psaoGet->SAO_pcPlace = psaoGet->SAO_cEMsg;
            psaoGet->SAO_pcEMsg  = psaoGet->SAO_cEMsg;
            piOptOpt = &psaoGet->SAO_iOptOpt;
        }
    } else {
        psaoGet  = __TTINY_SHELL_GET_ARTOPT(ptcbCur);
        piOptOpt = &psaoGet->SAO_iOptOpt;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  解除互斥                    */
    
    return  (piOptOpt);
}
/*********************************************************************************************************
** 函数名称: API_TShellOptReset
** 功能描述: 相当于 int optreset 这个变量.
**           当此变量为 1 时, 表示从第一个参数开始分析.
** 输　入  : NONE
** 输　出  : getopt 上下文中相应地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  *API_TShellOptReset (VOID)
{
    REGISTER INT                *piOptReset;
    REGISTER __PTSHELL_ARGOPT    psaoGet;
             PLW_CLASS_TCB       ptcbCur;
             
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    __TTINY_SHELL_LOCK();                                               /*  shell 互斥                  */
    if (LW_NULL == __TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        psaoGet =  (__PTSHELL_ARGOPT)__SHEAP_ALLOC(sizeof(__TSHELL_ARGOPT));
        if (!psaoGet) {
            _G_saoTmp.SAO_pcPlace = _G_saoTmp.SAO_cEMsg;
            _G_saoTmp.SAO_pcEMsg  = _G_saoTmp.SAO_cEMsg;
            piOptReset = &_G_saoTmp.SAO_iOptReset;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "\"int optopt\" system low memory.\r\n");
        } else {
            __TTINY_SHELL_SET_ARTOPT(ptcbCur, psaoGet);
            *psaoGet = _G_saoStd;
            psaoGet->SAO_pcPlace = psaoGet->SAO_cEMsg;
            psaoGet->SAO_pcEMsg  = psaoGet->SAO_cEMsg;
            piOptReset = &psaoGet->SAO_iOptReset;
        }
    } else {
        psaoGet  = __TTINY_SHELL_GET_ARTOPT(ptcbCur);
        piOptReset = &psaoGet->SAO_iOptReset;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  解除互斥                    */
    
    return  (piOptReset);
}
/*********************************************************************************************************
** 函数名称: API_TShellOptArg
** 功能描述: 相当于 int optarg 这个变量.
**           当前选项参数字串 (如果有).
** 输　入  : NONE
** 输　出  : getopt 上下文中相应地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PCHAR   *API_TShellOptArg (VOID)
{
    REGISTER PCHAR              *ppcOptArg;
    REGISTER __PTSHELL_ARGOPT    psaoGet;
             PLW_CLASS_TCB       ptcbCur;
             
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    __TTINY_SHELL_LOCK();                                               /*  shell 互斥                  */
    if (LW_NULL == __TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        psaoGet =  (__PTSHELL_ARGOPT)__SHEAP_ALLOC(sizeof(__TSHELL_ARGOPT));
        if (!psaoGet) {
            _G_saoTmp.SAO_pcPlace = _G_saoTmp.SAO_cEMsg;
            _G_saoTmp.SAO_pcEMsg  = _G_saoTmp.SAO_cEMsg;
            ppcOptArg = &_G_saoTmp.SAO_pcOptArg;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "\"int optarg\" system low memory.\r\n");
        } else {
            __TTINY_SHELL_SET_ARTOPT(ptcbCur, psaoGet);
            *psaoGet  = _G_saoStd;
            psaoGet->SAO_pcPlace = psaoGet->SAO_cEMsg;
            psaoGet->SAO_pcEMsg  = psaoGet->SAO_cEMsg;
            ppcOptArg = &psaoGet->SAO_pcOptArg;
        }
    } else {
        psaoGet  = __TTINY_SHELL_GET_ARTOPT(ptcbCur);
        ppcOptArg = &psaoGet->SAO_pcOptArg;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  解除互斥                    */
    
    return  (ppcOptArg);
}
/*********************************************************************************************************
** 函数名称: API_TShellOptFree
** 功能描述: 当有调用过 getopt 或者 getopt_long 函数的, 最终需要调用这个函数释放临时内存,
**           从而不会产生内存泄露.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_TShellOptFree (VOID)
{
    PLW_CLASS_TCB       ptcbCur;
             
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    __TTINY_SHELL_LOCK();                                               /*  shell 互斥                  */
    if (__TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        __SHEAP_FREE(__TTINY_SHELL_GET_ARTOPT(ptcbCur));
        __TTINY_SHELL_SET_ARTOPT(ptcbCur, LW_NULL);
    }
    __TTINY_SHELL_UNLOCK();                                             /*  解除互斥                    */
}
/*********************************************************************************************************
** 函数名称: __tShellOptDeleteHook
** 功能描述: 线程删除 hook.
** 输　入  : ulId          线程 ID
**           pvReturnVal   线程返回值
**           ptcb          线程控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __tShellOptDeleteHook (LW_OBJECT_HANDLE  ulId, 
                             PVOID             pvReturnVal, 
                             PLW_CLASS_TCB     ptcb)
{
    __TTINY_SHELL_LOCK();                                               /*  shell 互斥                  */
    if (__TTINY_SHELL_GET_ARTOPT(ptcb)) {
        __SHEAP_FREE(__TTINY_SHELL_GET_ARTOPT(ptcb));
        __TTINY_SHELL_SET_ARTOPT(ptcb, LW_NULL);                        /*  防止重启误操作              */
    }
    __TTINY_SHELL_UNLOCK();                                             /*  解除互斥                    */
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
