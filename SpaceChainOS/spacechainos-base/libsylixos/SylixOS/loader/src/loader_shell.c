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
** 文   件   名: loader_shell.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 04 月 17 日
**
** 描        述: loader shell interface. 

** BUG:
2010.05.05  支持使用 PATH 环境变量查询可执行文件的路径.
2010.05.14  __ldGetFilePath() 函数与 linux 可执行文件命令格式兼容.
2010.10.24  修改注释.
2011.02.20  修正 __tshellLd() 函数, 转载模块时使用 LW_OPTION_LOADER_SYM_GLOBAL 方式.
            仅 exec 命令需要访问环境变量, 其他的不需要.
2011.03.02  淘汰 ld 命令, 加入 modulereg 和 moduleunreg, 可以向系统内核注册和解除注册模块.
            加入显示所有 module 的功能.
2011.03.26  使用 LW_LD_DEFAULT_ENTRY 替换 "main".
2011.05.18  加入 libreg 和 libunreg 命令, 注册和卸载全局公共服务库.
            初始化 symbol table 查找回调.
2011.06.09  加入 __ldPathIsFile() 来判断是否为 reg 文件.
2011.07.29  去掉 libreg 接口, 所有的动态链接库, 与 linux 相同, 都必须依附于一个进程.
2011.11.02  加入对 vp patch 进程总内存使用量的打印.
2011.11.09  对进程内存的统计信息, 使用 API_VmmPCountInArea() 接口.
2012.04.12  注册内核模块, 当为没有权限错误时, 打印相关错误.
2012.04.26  系统 reboot 时, 需要运行 moduleRebootHook() 函数.
2012.05.10  初始化时加入对内核符号遍历回调.
2012.08.24  加入 ps 命令.
2012.09.29  获得模块文件信息时, 需要先判断文件有效且是数据文件.
2012.10.18  __tshellExec() 支持运行脚本程序.
2012.12.09  ps 命令加入对进程父亲的打印.
2012.12.17  __ldGetFilePath 保存绝对路径.
2012.12.22  ps 与 modules 命令内部需要加入, 进程内模块锁, 否则装载或卸载进程时查看进程信息会出错.
2013.01.18  加入 which 内建命令.
2013.06.05  由于使用共享代码段, 显示进程内存消耗量时, 使用新的机制.
2013.06.07  加入 dlrefresh 命令, 更新动态库或者应用前, 需要运行此命令.
2013.06.13  shell 创建的进程, 初始优先级为 posix 默认优先级.
            ps 命令加入对进程组号显示.
2013.12.04  整理 loader 初始化代码.
2014.04.21  内核模块插入或卸载打印相关路径名.
2014.05.02  加入 lsmod 命令.
2017.02.06  ps加入显示进程状态. (张荣荣)
2017.12.26  调整 64 位打印宽度.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "unistd.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "pwd.h"
#include "../include/loader_lib.h"
#include "../include/loader_symbol.h"
/*********************************************************************************************************
  进程列表
*********************************************************************************************************/
extern LW_LIST_LINE_HEADER      _G_plineVProcHeader;
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_RMMOD_ATREBOOT_EN > 0
VOID          moduleRebootHook(INT  iRebootType);
#endif

PVOIDFUNCPTR  __symbolFindHookSet(PVOIDFUNCPTR  pfuncSymbolFindHook);
VOIDFUNCPTR   __symbolTraverseHookSet(VOIDFUNCPTR  pfuncSymbolTraverseHook);

PVOID         __moduleFindKernelSymHook(CPCHAR  pcSymName, INT  iFlag);
VOID          __moduleTraverseKernelSymHook(BOOL (*pfuncCb)(PVOID, PLW_SYMBOL), PVOID  pvArg);
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#if LW_CFG_CPU_WORD_LENGHT == 64
static const CHAR               _G_cModuleInfoHdr[] = "\n\
            NAME               HANDLE       TYPE  GLB       BASE         SIZE   SYMCNT\n\
------------------------- ---------------- ------ --- ---------------- -------- ------\n";
#else
static const CHAR               _G_cModuleInfoHdr[] = "\n\
            NAME           HANDLE   TYPE  GLB   BASE     SIZE   SYMCNT\n\
------------------------- -------- ------ --- -------- -------- ------\n";
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */
static const CHAR               _G_cVProcInfoHdr[] = "\n\
      NAME            FATHER      STAT  PID   GRP    MEMORY    UID   GID   USER\n\
---------------- ---------------- ---- ----- ----- ---------- ----- ----- ------\n";
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** 函数名称: __ldPathIsFile
** 功能描述: 判断文件的路径是否为文件
** 输　入  : pcParam       用户路径参数
** 输　出  : BOOL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL __ldPathIsFile (CPCHAR  pcName)
{
    struct stat     statFs;
    
    if (stat(pcName, &statFs) >= 0) {
        if (S_ISREG(statFs.st_mode)) {
            return  (LW_TRUE);
        }
    }
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: __ldGetFilePath
** 功能描述: 获得可执行文件的路径
** 输　入  : pcParam       用户路径参数
**           pcPathBuffer  查找到的文件路径缓冲
**           stMaxLen      缓冲区大小
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  __ldGetFilePath (CPCHAR  pcParam, PCHAR  pcPathBuffer, size_t  stMaxLen)
{
    CHAR    cBuffer[MAX_FILENAME_LENGTH];
    
    PCHAR   pcStart;
    PCHAR   pcDiv;
    
    if (stMaxLen < 2) {                                                 /*  缓冲区大小错误              */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (lib_strchr(pcParam, '/')) {                                     /*  是一个路径                  */
        if (__ldPathIsFile(pcParam)) {                                  /*  直接使用参数即可            */
            _PathGetFull(pcPathBuffer, stMaxLen, pcParam);              /*  保存绝对路径                */
            return  (ERROR_NONE);
        }
    
    } else {                                                            /*  不是一个路径                */
        if (lib_getenv_r("PATH", cBuffer, MAX_FILENAME_LENGTH)
            != ERROR_NONE) {                                            /*  PATH 环境变量值为空         */
            _ErrorHandle(ENOENT);                                       /*  无法找到文件                */
            return  (PX_ERROR);
        }
        
        pcPathBuffer[stMaxLen - 1] = PX_EOS;
        
        pcDiv = cBuffer;                                                /*  从第一个参数开始找          */
        do {
            pcStart = pcDiv;
            pcDiv   = lib_strchr(pcStart, ':');                         /*  查找下一个参数分割点        */
            if (pcDiv) {
                *pcDiv = PX_EOS;
                pcDiv++;
            }
            
            snprintf(pcPathBuffer, stMaxLen, "%s/%s", pcStart, pcParam);/*  合并为完整的目录            */
            if (__ldPathIsFile(pcPathBuffer)) {                         /*  此文件可以被访问            */
                return  (ERROR_NONE);
            }
        } while (pcDiv);
    }
    
    _ErrorHandle(ENOENT);                                               /*  无法找到文件                */
    return  (PX_ERROR);
}
/*********************************************************************************************************
  以下函数需要 shell 支持
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
/*********************************************************************************************************
** 函数名称: __tshellWhich
** 功能描述: 系统命令 "which"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellWhich (INT  iArgC, PCHAR  *ppcArgV)
{
    CHAR    cFilePath[MAX_FILENAME_LENGTH];

    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (__ldGetFilePath(ppcArgV[1], cFilePath, MAX_FILENAME_LENGTH) != ERROR_NONE) {
        fprintf(stderr, "can not find file!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    printf("%s\n", cFilePath);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ldRunScript
** 功能描述: 获得可执行文件的路径
** 输　入  : pcSheBangLine shebang line (已经去掉 #! 符号)
**           iArgC         参数个数
**           ppcArgV       参数表 
                           此参数由 shell 系统给出, 且最大数组长度为 LW_CFG_SHELL_MAX_PARAMNUM + 1
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ldRunScript (PCHAR   pcSheBangLine, INT  iArgC, PCHAR  *ppcArgV)
{
#define __IS_WHITE(c)       (c == ' ' || c == '\t' || c == '\r' || c == '\n')
#define __IS_END(c)         (c == PX_EOS)
#define __SKIP_WHITE(str)   while (__IS_WHITE(*str)) {  \
                                str++;  \
                            }
#define __NEXT_WHITE(str)   while (!__IS_WHITE(*str) && !__IS_END(*str)) { \
                                str++;  \
                            }
#define __LAST_WHITE(str)   while (!__IS_END(*str)) {   \
                                str++;  \
                            }   \
                            str--;  \
                            while (__IS_WHITE(*str)) {  \
                                str--;  \
                            }   \
                            str++;
                            
    PCHAR      pcTemp = pcSheBangLine;
    PCHAR      pcCmd;
    PCHAR      pcArg;
    INT        i;
    
    /*
     *  此时的 argv 结构如下:
     *
     *  +-------------+
     *  |    exec     |  0
     *  +-------------+
     *  | script file |  1
     *  +-------------+
     *  |    <arg>    |  2
     *  +-------------+
     *  |     ...     |  ...
     *  +-------------+
     *  |     NULL    |  ...
     *  +-------------+
     */
     
    pcTemp = pcSheBangLine;
    
    __SKIP_WHITE(pcTemp);                                               /*  忽略空格                    */
    if (__IS_END(*pcTemp)) {
        return  (-ERROR_TSHELL_CMDNOTFUND);                             /*  无法找到命令                */
    }
    
    pcCmd = pcTemp;                                                     /*  命令名起始                  */
    ppcArgV[0] = pcCmd;                                                 /*  替换参数 0 为 cmd           */
    
    /*
     *  此时的 argv 结构如下:
     *
     *  +-------------+
     *  |    pcCmd    |  0
     *  +-------------+
     *  | script file |  1
     *  +-------------+
     *  |    <arg>    |  2
     *  +-------------+
     *  |     ...     |  ...
     *  +-------------+
     *  |     NULL    |  ...
     *  +-------------+
     */
    
    __NEXT_WHITE(pcTemp);
    if (__IS_END(*pcTemp)) {                                            /*  没有绑定参数                */
        return  (moduleRunEx(pcCmd, LW_LD_DEFAULT_ENTRY, 
                             iArgC, (CPCHAR *)ppcArgV, LW_NULL));
    }
    
    *pcTemp = PX_EOS;                                                   /*  命令结束                    */
    pcTemp++;
    __SKIP_WHITE(pcTemp);                                               /*  忽略空格                    */
    if (__IS_END(*pcTemp)) {                                            /*  没有绑定参数                */
        return  (moduleRunEx(pcCmd, LW_LD_DEFAULT_ENTRY, 
                             iArgC, (CPCHAR *)ppcArgV, LW_NULL));
    }
    
    pcArg = pcTemp;                                                     /*  绑定参数起始                */
    __LAST_WHITE(pcTemp);
    *pcTemp = PX_EOS;                                                   /*  绑定参数结束                */
    
    if ((iArgC + 1) > LW_CFG_SHELL_MAX_PARAMNUM) {
        return  (-ERROR_TSHELL_EPARAM);                                 /*  总参数太多                  */
    }
    
    for (i = iArgC - 1; i > 0; i--) {                                   /*  从第2个参数开始, 向后推移   */
        ppcArgV[i + 1] = ppcArgV[i];
    }
    iArgC++;
    ppcArgV[iArgC] = LW_NULL;
    
    /*
     *  此时的 argv 结构如下:
     *
     *  +-------------+
     *  |    pcCmd    |  0
     *  +-------------+
     *  |             |  1
     *  +-------------+
     *  | script file |  2
     *  +-------------+
     *  |    <arg>    |  3
     *  +-------------+
     *  |     ...     |  ...
     *  +-------------+
     *  |     NULL    |  ...
     *  +-------------+
     */
     
    ppcArgV[1] = pcArg;                                                 /*  替换参数 1 为绑定参数       */
    
    /*
     *  此时的 argv 结构如下:
     *
     *  +-------------+
     *  |    pcCmd    |  0
     *  +-------------+
     *  |  bang arg   |  1
     *  +-------------+
     *  | script file |  2
     *  +-------------+
     *  |    <arg>    |  3
     *  +-------------+
     *  |     ...     |  ...
     *  +-------------+
     *  |     NULL    |  ...
     *  +-------------+
     */
     
    return  (moduleRunEx(pcCmd, LW_LD_DEFAULT_ENTRY, iArgC, (CPCHAR *)ppcArgV, LW_NULL));
}
/*********************************************************************************************************
** 函数名称: __tshellExec
** 功能描述: 系统命令 "exec" 这个命令一定是背景运行的, 此线程将变为进程的主线程.
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellExec (INT  iArgC, PCHAR  *ppcArgV)
{
    CHAR          cFilePath[MAX_FILENAME_LENGTH];
    CHAR          cSheBangBuffer[MAX_FILENAME_LENGTH];
    PCHAR         pcSheBangLine;
    FILE         *pfile;
    struct stat   statBuf;
    
    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (__ldGetFilePath(ppcArgV[1], cFilePath, MAX_FILENAME_LENGTH) != ERROR_NONE) {
        return  (-ERROR_TSHELL_CMDNOTFUND);                             /*  无法找到命令                */
    }
    
    pfile = fopen(cFilePath, "r");
    if (!pfile) {
        return  (-ERROR_TSHELL_CMDNOTFUND);                             /*  无法找到命令                */
    }
    
    if (fstat(fileno(pfile), &statBuf) < ERROR_NONE) {                  /*  获得文件信息                */
        return  (-ERROR_TSHELL_CMDNOTFUND);                             /*  无法找到命令                */
    }
    
    if (_IosCheckPermissions(O_RDONLY, LW_TRUE, statBuf.st_mode,        /*  检查文件执行权限            */
                             statBuf.st_uid, statBuf.st_gid) < ERROR_NONE) {
        fclose(pfile);
        fprintf(stderr, "insufficient permissions!\n");
        return  (PX_ERROR);
    }
    
#if LW_CFG_POSIX_EN > 0
    API_ThreadSetPriority(API_ThreadIdSelf(), LW_PRIO_NORMAL);
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
    
    pcSheBangLine = fgets(cSheBangBuffer, MAX_FILENAME_LENGTH, pfile);  /*  获得文件的第一行            */
    fclose(pfile);
    
    if (pcSheBangLine) {
        if (pcSheBangLine[0] == '#' && pcSheBangLine[1] == '!') {       /*  是脚本程序文件              */
            return  (__ldRunScript(&pcSheBangLine[2], iArgC, ppcArgV)); /*  执行脚本文件                */
        }
    }
    
    return  (moduleRunEx(cFilePath, LW_LD_DEFAULT_ENTRY, 
                         iArgC - 1, (CPCHAR *)&ppcArgV[1], LW_NULL));
}
/*********************************************************************************************************
** 函数名称: __tshellDlConfig
** 功能描述: 系统命令 "dlconfig"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellDlConfig (INT  iArgC, PCHAR  *ppcArgV)
{
    PCHAR   pcFileName = LW_NULL;
    BOOL    bShare;

    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (lib_strcmp(ppcArgV[1], "refresh") == 0) {
        if (iArgC >= 3) {
            pcFileName = ppcArgV[2];
        }
        
        return  (moduleShareRefresh(pcFileName));
    
    } else if (lib_strcmp(ppcArgV[1], "share") == 0) {
        if (iArgC < 3) {
            fprintf(stderr, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        if ((ppcArgV[2][0] == 'e') ||
            (ppcArgV[2][0] == 'y') ||
            (ppcArgV[2][0] == '1')) {
            bShare = LW_TRUE;
        } else {
            bShare = LW_FALSE;
        }
        
        return  (moduleShareConfig(bShare));
    
    } else {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
}
/*********************************************************************************************************
** 函数名称: __tshellModuleReg
** 功能描述: 系统命令 "modulereg"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellModuleReg (INT  iArgC, PCHAR  *ppcArgV)
{
    PVOID   pvModule;

    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    pvModule = moduleRegister(ppcArgV[1]);                              /*  向内核注册模块              */
    if (pvModule) {
        printf("module %s register ok, handle: 0x%lx\n", ppcArgV[1], (addr_t)pvModule);
        return  (ERROR_NONE);
    
    } else {
        if (errno == EACCES) {
            fprintf(stderr, "insufficient permissions.\n");
        } else {
            fprintf(stderr, "can not register module, error: %s\n", lib_strerror(errno));
        }
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __tshellModuleUnReg
** 功能描述: 系统命令 "moduleunreg"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellModuleUnreg (INT  iArgC, PCHAR  *ppcArgV)
{
    ULONG               ulModule = 0ul;
    LW_LD_EXEC_MODULE  *pmod     = LW_NULL; 
    CHAR                cModule[MAX_FILENAME_LENGTH];
    INT                 iError;

    BOOL                bStart;

    LW_LIST_RING       *pringTemp;
    LW_LD_VPROC        *pvproc;
    LW_LD_EXEC_MODULE  *pmodTemp;

    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (sscanf(ppcArgV[1], "%lx", &ulModule) != 1) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    /*
     *  判断参数是否为有效的内核模块句柄
     */
    pvproc = _LIST_ENTRY(_G_plineVProcHeader, LW_LD_VPROC, VP_lineManage);
    LW_VP_LOCK(pvproc);
    for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
         pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
         pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        if (pmodTemp == (LW_LD_EXEC_MODULE *)ulModule) {
            pmod = (LW_LD_EXEC_MODULE *)ulModule;
            break;
        }
    }
    LW_VP_UNLOCK(pvproc);

    if ((pmod == LW_NULL) ||
        (pmod->EMOD_ulMagic != __LW_LD_EXEC_MODULE_MAGIC)) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    lib_strlcpy(cModule, pmod->EMOD_pcModulePath, MAX_FILENAME_LENGTH);
    
    iError = moduleUnregister((PVOID)ulModule);                         /*  从内核卸载模块              */
    if (iError == ERROR_NONE) {
        printf("module %s unregister ok.\n", cModule);
        return  (ERROR_NONE);
    
    } else {
        if (errno == EACCES) {
            fprintf(stderr, "insufficient permissions.\n");
        } else {
            fprintf(stderr, "can not unregister module, error: %s\n", lib_strerror(errno));
        }
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __tshellInsModule
** 功能描述: 系统命令 "insmod"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellInsModule (INT  iArgC, PCHAR  *ppcArgV)
{
    PVOID   pvModule;

    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (iArgC == 2) {
        return  (__tshellModuleReg(iArgC, ppcArgV));
    
    } else if (!lib_strcmp(ppcArgV[1], "-x")) {
        pvModule = API_ModuleLoadEx(ppcArgV[2],
                                    LW_OPTION_LOADER_SYM_GLOBAL,
                                    LW_MODULE_INIT,
                                    LW_MODULE_EXIT,
                                    LW_NULL,
                                    ".????????????????",
                                    LW_NULL);
        if (pvModule) {
            printf("module %s register ok, handle: 0x%lx\n", ppcArgV[1], (addr_t)pvModule);
            return  (ERROR_NONE);
        
        } else {
            if (errno == EACCES) {
                fprintf(stderr, "insufficient permissions.\n");
            } else {
                fprintf(stderr, "can not register module, error: %s\n", lib_strerror(errno));
            }
            return  (PX_ERROR);
        }
    
    } else {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
}
/*********************************************************************************************************
** 函数名称: __tshellRmModule
** 功能描述: 系统命令 "rmmod"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellRmModule (INT  iArgC, PCHAR  *ppcArgV)
{
    LW_LD_EXEC_MODULE  *pmod = LW_NULL; 
    CHAR                cModule[MAX_FILENAME_LENGTH];
    INT                 iError;

    BOOL                bStart;

    LW_LIST_RING       *pringTemp;
    LW_LD_VPROC        *pvproc;
    LW_LD_EXEC_MODULE  *pmodTemp;

    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    /*
     *  判断参数是否为有效的内核模块句柄
     */
    pvproc = _LIST_ENTRY(_G_plineVProcHeader, LW_LD_VPROC, VP_lineManage);
    LW_VP_LOCK(pvproc);
    for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
         pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
         pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        
        if (!lib_strcmp(_PathLastNamePtr(pmodTemp->EMOD_pcModulePath),
                        _PathLastNamePtr(ppcArgV[1]))) {
            pmod = pmodTemp;
            break;
        }
    }
    LW_VP_UNLOCK(pvproc);

    if ((pmod == LW_NULL) ||
        (pmod->EMOD_ulMagic != __LW_LD_EXEC_MODULE_MAGIC)) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    lib_strlcpy(cModule, pmod->EMOD_pcModulePath, MAX_FILENAME_LENGTH);
    
    iError = moduleUnregister((PVOID)pmod);                             /*  从内核卸载模块              */
    if (iError == ERROR_NONE) {
        printf("module %s unregister ok.\n", cModule);
        return  (ERROR_NONE);
    
    } else {
        if (errno == EACCES) {
            fprintf(stderr, "insufficient permissions.\n");
        } else {
            fprintf(stderr, "can not unregister module, error: %s\n", lib_strerror(errno));
        }
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __tshellModulestat
** 功能描述: 系统命令 "modulestat"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellModulestat (INT  iArgC, PCHAR  *ppcArgV)
{
    struct stat  statGet;

    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (stat(ppcArgV[1], &statGet) < 0) {
        fprintf(stderr, "can not open %s : %s\n", ppcArgV[1], lib_strerror(errno));
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (!S_ISREG(statGet.st_mode)) {
        fprintf(stderr, "not a REG file.\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    return  (moduleStatus(ppcArgV[1], STD_OUT));
}
/*********************************************************************************************************
** 函数名称: __tshellModuleShow
** 功能描述: 系统命令 "modules"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellModuleShow (INT  iArgC, PCHAR  *ppcArgV)
{
    INT                 iCnt = 0;
    PCHAR               pcFileName;
    PCHAR               pcProcessName;
    PCHAR               pcModuleName;
    BOOL                bStart;
    
    LW_LIST_LINE       *plineTemp;
    LW_LIST_RING       *pringTemp;
    LW_LD_VPROC        *pvproc;
    LW_LD_EXEC_MODULE  *pmodTemp;
    
#if LW_CFG_VMM_EN == 0
    PLW_CLASS_HEAP      pheapVpPatch;
#endif                                                                  /*  LW_CFG_VMM_EN == 0          */
    
    CHAR                cVpVersion[128] = "";
    
    INT                 i, iNum;
    ULONG               ulPages;
    size_t              stTotalMem;
    PVOID               pvVmem[LW_LD_VMEM_MAX];
    
    if (iArgC < 2) {
        pcFileName = LW_NULL;
    } else {
        pcFileName = ppcArgV[1];
    }
    
    printf(_G_cModuleInfoHdr);
    LW_LD_LOCK();
    for (plineTemp  = _G_plineVProcHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pvproc = _LIST_ENTRY(plineTemp, LW_LD_VPROC, VP_lineManage);
        
        _PathLastName((PCHAR)pvproc->VP_pcName, &pcProcessName);
        
        if (pcFileName) {
            if (lib_strcmp(pcFileName, pcProcessName) != 0) {
                continue;
            }
        }
        
        /*
         *  计算 vp 进程总内存消耗.
         */
        LW_VP_LOCK(pvproc);
        stTotalMem    = 0;
        for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
             pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
             pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {

            pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
            
#if LW_CFG_VMM_EN > 0
            ulPages = 0;
            if (API_VmmPCountInArea(pmodTemp->EMOD_pvBaseAddr, &ulPages) == ERROR_NONE) {
                stTotalMem += (size_t)(ulPages << LW_CFG_VMM_PAGE_SHIFT);
            }
#else
            stTotalMem += pmodTemp->EMOD_stLen;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
            stTotalMem += __moduleSymbolBufferSize(pmodTemp);
        }
        
        if (stTotalMem) {                                               /*  至少存在一个模块            */
            pmodTemp = _LIST_ENTRY(pvproc->VP_ringModules, LW_LD_EXEC_MODULE, EMOD_ringModules);
            ulPages  = 0;
            
#if LW_CFG_VMM_EN > 0
            iNum = __moduleVpPatchVmem(pmodTemp, pvVmem, LW_LD_VMEM_MAX);
            if (iNum > 0) {
                for (i = 0; i < iNum; i++) {
                    if (API_VmmPCountInArea(pvVmem[i], 
                                            &ulPages) == ERROR_NONE) {
                        stTotalMem += (size_t)(ulPages 
                                   << LW_CFG_VMM_PAGE_SHIFT);
                    }
                }
                lib_strlcpy(cVpVersion, __moduleVpPatchVersion(pmodTemp), sizeof(cVpVersion));
            }
#else
            pheapVpPatch = (PLW_CLASS_HEAP)__moduleVpPatchHeap(pmodTemp);
            if (pheapVpPatch) {                                         /*  获得 vp 进程私有 heap       */
                stTotalMem += (size_t)(pheapVpPatch->HEAP_stTotalByteSize);
                lib_strlcpy(cVpVersion, __moduleVpPatchVersion(pmodTemp), sizeof(cVpVersion));
            }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
        }
        LW_VP_UNLOCK(pvproc);
        
#if LW_CFG_VMM_EN > 0
        {
            size_t  stMmapSize = 0;
            API_VmmMmapPCount(pvproc->VP_pid, &stMmapSize);             /*  计算 mmap 内存实际消耗量    */
            stTotalMem += stMmapSize;
        }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

        printf("VPROC: %-18s pid:%4d TOTAL MEM: %zu ",
               pcProcessName,
               pvproc->VP_pid,
               stTotalMem);
               
        if (cVpVersion[0] != PX_EOS) {
            printf("<vp ver:%s>\n", cVpVersion);
        
        } else {
            printf("\n");
        }
               
        /*
         *  打印模块信息
         */
        LW_VP_LOCK(pvproc);
        for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
             pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
             pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {

            pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

            _PathLastName(pmodTemp->EMOD_pcModulePath, &pcModuleName);

#if LW_CFG_CPU_WORD_LENGHT == 64
            printf("+ %-23s %16lx %-6s %-3s %16lx %8lx %6ld\n",
#else
            printf("+ %-23s %08lx %-6s %-3s %08lx %8lx %6ld\n",
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */
                   pcModuleName,
                   (addr_t)pmodTemp,
                   (((pmodTemp->EMOD_bIsGlobal) && (pmodTemp->EMOD_pcSymSection)) ? "KERNEL" : "USER"),
                   ((pmodTemp->EMOD_bIsGlobal) ? "YES" : "NO"),
                   (addr_t)pmodTemp->EMOD_pvBaseAddr,
                   (ULONG)pmodTemp->EMOD_stLen,
                   (ULONG)pmodTemp->EMOD_ulSymCount);
            iCnt++;
        }
        LW_VP_UNLOCK(pvproc);
    }
    LW_LD_UNLOCK();

    printf("\n");
    printf("total modules: %d\n", iCnt);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellVProcShow
** 功能描述: 系统命令 "ps"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellVProcShow (INT  iArgC, PCHAR  *ppcArgV)
{
    INT                 iCnt = 0;
    PCHAR               pcProcessName;
    PCHAR               pcFatherName;
    BOOL                bStart;
    
    CHAR                cVprocStat;                                     /*  进程状态                    */

    LW_LIST_LINE       *plineTemp;
    LW_LIST_RING       *pringTemp;
    LW_LD_VPROC        *pvproc;
    LW_LD_EXEC_MODULE  *pmodTemp;
    
#if LW_CFG_VMM_EN == 0
    PLW_CLASS_HEAP      pheapVpPatch;
#endif                                                                  /*  LW_CFG_VMM_EN == 0          */
    
    INT                 i, iNum;
    ULONG               ulPages;
    size_t              stTotalMem;
    PVOID               pvVmem[LW_LD_VMEM_MAX];
    
    struct passwd       passwd;
    struct passwd      *ppasswd = LW_NULL;
    
    CHAR                cUserName[MAX_FILENAME_LENGTH];
    PCHAR               pcUser;
    
    uid_t               uid;
    gid_t               gid;
    
    printf(_G_cVProcInfoHdr);
    LW_LD_LOCK();
    for (plineTemp  = _G_plineVProcHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pvproc = _LIST_ENTRY(plineTemp, LW_LD_VPROC, VP_lineManage);
        
        _PathLastName((PCHAR)pvproc->VP_pcName, &pcProcessName);
        
        if (pvproc->VP_pvprocFather) {
            _PathLastName((PCHAR)pvproc->VP_pvprocFather->VP_pcName, &pcFatherName);
        } else {
            pcFatherName = "<orphan>";
        }
        
        if (pvproc->VP_pid == 0) {
            /*
             *  kernel 进程其实不是真正意义上的进程, 只是对内核整个资源的标识, 不具备进程的特性.
             *  所以在显示进程状态时会一直显示处于初始化状态, 为了让用户便于理解ps出进程状态时
             *  kernel 进程显示运行状态 ('R')
             */
            cVprocStat = 'R';

        } else {
            /*
             *  判断进程进程状态:
             *  1. 进程运行状态: R (TASK_RUNNING), 可执行状态&运行状态 (在 run_queue 队列里的状态)
             *  2. 进程停止状态: T (TASK_STOPPED or TASK_TRACED), 停止态
             *  3. 进程僵尸状态: Z (TASK_DEAD - EXIT_ZOMBIE), 退出状态, 进程成为僵尸进程
             *  4. 进程初始状态: I (TASK_INIT), 运行状态
             */
            if (pvproc->VP_iStatus == __LW_VP_EXIT) {
                cVprocStat = 'Z';
            } else if (pvproc->VP_iStatus == __LW_VP_STOP) {
                cVprocStat = 'T';
            } else if (pvproc->VP_iStatus == __LW_VP_INIT) {
                cVprocStat = 'I';
            } else {
                cVprocStat = 'R';
            }
        }

        /*
         *  计算 vp 进程总内存消耗.
         */
        LW_VP_LOCK(pvproc);
        stTotalMem    = 0;
        for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
             pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
             pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {

            pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
            
#if LW_CFG_VMM_EN > 0
            ulPages = 0;
            if (API_VmmPCountInArea(pmodTemp->EMOD_pvBaseAddr, &ulPages) == ERROR_NONE) {
                stTotalMem += (size_t)(ulPages << LW_CFG_VMM_PAGE_SHIFT);
            }
#else
            stTotalMem += pmodTemp->EMOD_stLen;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
            stTotalMem += __moduleSymbolBufferSize(pmodTemp);
        }
        
        if (stTotalMem) {                                               /*  至少存在一个模块            */
            pmodTemp = _LIST_ENTRY(pvproc->VP_ringModules, LW_LD_EXEC_MODULE, EMOD_ringModules);
            ulPages  = 0;
            
#if LW_CFG_VMM_EN > 0
            iNum = __moduleVpPatchVmem(pmodTemp, pvVmem, LW_LD_VMEM_MAX);
            if (iNum > 0) {
                for (i = 0; i < iNum; i++) {
                    if (API_VmmPCountInArea(pvVmem[i], 
                                            &ulPages) == ERROR_NONE) {
                        stTotalMem += (size_t)(ulPages 
                                   <<  LW_CFG_VMM_PAGE_SHIFT);
                    }
                }
            }
#else
            pheapVpPatch = (PLW_CLASS_HEAP)__moduleVpPatchHeap(pmodTemp);
            if (pheapVpPatch) {                                         /*  获得 vp 进程私有 heap       */
                stTotalMem += (size_t)(pheapVpPatch->HEAP_stTotalByteSize);
            }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
        }
        LW_VP_UNLOCK(pvproc);
        
#if LW_CFG_VMM_EN > 0
        {
            size_t  stMmapSize = 0;
            API_VmmMmapPCount(pvproc->VP_pid, &stMmapSize);             /*  计算 mmap 内存实际消耗量    */
            stTotalMem += stMmapSize;
        }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
        
        uid = 0;
        gid = 0;
        if (pvproc->VP_ulMainThread) {
            _ThreadUserGet(pvproc->VP_ulMainThread, &uid, &gid);
        }
        
        getpwuid_r(uid, &passwd, cUserName, sizeof(cUserName), &ppasswd);
        if (ppasswd) {
            pcUser = cUserName;
        } else {
            pcUser = "<unknown>";
        }
        
        printf("%-16s %-16s %-4c %5d %5d %8zuKB %5d %5d %s\n",
                      pcProcessName, pcFatherName, cVprocStat, pvproc->VP_pid, pvproc->VP_pidGroup,
                      stTotalMem / LW_CFG_KB_SIZE, uid, gid, pcUser);
        
        iCnt++;
    }
    LW_LD_UNLOCK();

    printf("\n");
    printf("total vprocess: %d\n", iCnt);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellLsmod
** 功能描述: 系统命令 "ps"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellLsmod (INT  iArgC, PCHAR  *ppcArgV)
{
    PCHAR   pcArgV[3] = {"modules", "kernel", LW_NULL};

    return  (__tshellModuleShow(2, pcArgV));
}
/*********************************************************************************************************
** 函数名称: __tshellModuleGcov
** 功能描述: 系统命令 "modulegcov"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_GCOV_EN > 0

static INT  __tshellModuleGcov (INT  iArgC, PCHAR  *ppcArgV)
{
    ULONG   ulModule = (ULONG)LW_NULL;
    INT     iRet;

    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (sscanf(ppcArgV[1], "%lx", &ulModule) != 1) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    iRet = moduleGcov((PVOID)ulModule);
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_GCOV_EN */
/*********************************************************************************************************
** 函数名称: __ldShellInit
** 功能描述: 初始化 loader 内部 shell 命令.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块: 
*********************************************************************************************************/
static VOID  __ldShellInit (VOID)
{
    API_TShellKeywordAdd("which", __tshellWhich);
    API_TShellFormatAdd("which", " [program file]");
    API_TShellHelpAdd("which",   "find a program file real path.\n");
     
    API_TShellKeywordAddEx("exec", __tshellExec, LW_OPTION_KEYWORD_SYNCBG | LW_OPTION_KEYWORD_STK_MAIN);
    API_TShellFormatAdd("exec", " [program file] [arguments...]");
    API_TShellHelpAdd("exec",   "execute a program.\n");
    
    API_TShellKeywordAdd("dlconfig", __tshellDlConfig);
    API_TShellFormatAdd("dlconfig",  " {[share {en | dis}] | [refresh [*]]}");
    API_TShellHelpAdd("dlconfig",    "config dynamic loader.\n"
                                     "dlconfig share en     enable share code.\n"
                                     "dlconfig share dis    disable share code.\n"
                                     "dlconfig refresh *    clean system share segment info (* mean a file).\n"
                                     "dlconfig refresh      clean all system share segment info.\n"
                                     "NOTICE: before update share library or application\n"
                                     "        you must first run 'dlconfig refresh' command.\n");
    
    API_TShellKeywordAdd("modulereg", __tshellModuleReg);
    API_TShellFormatAdd("modulereg",  " [kernel module file *.ko]");
    API_TShellHelpAdd("modulereg",    "register a kernel module into system.\n"
                                      "NOTICE: only import LW_SYMBOL_EXPORT attribute\n"
                                      "        symbol(s) to kernel symbol table.\n");

    API_TShellKeywordAdd("moduleunreg", __tshellModuleUnreg);
    API_TShellFormatAdd("moduleunreg", " [kernel module handle]");
    API_TShellHelpAdd("moduleunreg",   "unregister a kernel module from system [DANGER! Not Safety].\n");
    
    API_TShellKeywordAdd("insmod", __tshellInsModule);
    API_TShellFormatAdd("insmod",  " [-x] [kernel module file *.ko]");
    API_TShellHelpAdd("insmod",    "register a kernel module into system.\n"
                                   "-x do not export global symbol.\n");
    
    API_TShellKeywordAdd("rmmod", __tshellRmModule);
    API_TShellFormatAdd("rmmod",  " [kernel module file *.ko]");
    API_TShellHelpAdd("rmmod",    "unregister a kernel module from system [DANGER! Not Safety].\n");
    
    API_TShellKeywordAdd("modulestat", __tshellModulestat);
    API_TShellFormatAdd("modulestat", " [program file]");
    API_TShellHelpAdd("modulestat",   "show a module file information.\n");
    
    API_TShellKeywordAdd("modules", __tshellModuleShow);
    API_TShellFormatAdd("modules", " [module name]");
    API_TShellHelpAdd("modules",   "show module information.\n");
    
#if LW_CFG_MODULELOADER_GCOV_EN > 0
    API_TShellKeywordAdd("modulegcov", __tshellModuleGcov);
    API_TShellFormatAdd("modulegcov",  " [kernel module handle]");
    API_TShellHelpAdd("modulegcov",    "generate kernel module code coverage file(*.gcda).\n");
#endif                                                                  /*  LW_CFG_MODULELOADER_GCOV_EN */

    API_TShellKeywordAdd("lsmod", __tshellLsmod);
    API_TShellHelpAdd("lsmod",    "show all kernel module.\n");
    
    API_TShellKeywordAdd("ps", __tshellVProcShow);
    API_TShellHelpAdd("ps",   "show vprocess information.\n");
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** 函数名称: API_LoaderInit
** 功能描述: 初始化 loader 组件.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_LoaderInit (VOID)
{
    static BOOL     bIsInit = LW_FALSE;
    
    if (bIsInit) {
        return;
    }
    
    bIsInit = LW_TRUE;

    _G_ulVProcMutex    = API_SemaphoreMCreate("loader_lock", LW_PRIO_DEF_CEILING, LW_OPTION_WAIT_PRIORITY |
                                              LW_OPTION_INHERIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                              LW_OPTION_OBJECT_GLOBAL, LW_NULL);
#if LW_CFG_VMM_EN > 0
    _G_ulExecShareLock = API_SemaphoreMCreate("execshare_lock", LW_PRIO_DEF_CEILING, LW_OPTION_WAIT_PRIORITY |
                                              LW_OPTION_INHERIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                              LW_OPTION_OBJECT_GLOBAL, LW_NULL);
#endif                                                                  /*  LW_CFG_VMM_EN > 0          */

    lib_bzero(&_G_vprocKernel, sizeof(_G_vprocKernel));
    
    _G_vprocKernel.VP_pcName = "kernel";
    _G_vprocKernel.VP_ulModuleMutex = API_SemaphoreMCreate("kvproc_lock",
                                                           LW_PRIO_DEF_CEILING,
                                                           LW_OPTION_WAIT_PRIORITY |
                                                           LW_OPTION_INHERIT_PRIORITY | 
                                                           LW_OPTION_DELETE_SAFE |
                                                           LW_OPTION_OBJECT_GLOBAL,
                                                           LW_NULL);
                                                            
    _List_Line_Add_Ahead(&_G_vprocKernel.VP_lineManage, &_G_plineVProcHeader);

    __symbolFindHookSet(__moduleFindKernelSymHook);                     /*  安装符号表查询回调          */
    __symbolTraverseHookSet(__moduleTraverseKernelSymHook);
    
    _S_pfuncGetCurPid   = getpid;                                       /*  安装 I/O 系统回调组         */
    _S_pfuncFileGet     = vprocIoFileGet;
    _S_pfuncFileDescGet = vprocIoFileDescGet;
    _S_pfuncFileDup     = vprocIoFileDup;
    _S_pfuncFileDup2    = vprocIoFileDup2;
    _S_pfuncFileRefInc  = vprocIoFileRefInc;
    _S_pfuncFileRefDec  = vprocIoFileRefDec;
    _S_pfuncFileRefGet  = vprocIoFileRefGet;
    
#if LW_CFG_MODULELOADER_RMMOD_ATREBOOT_EN > 0
    API_SystemHookAdd(moduleRebootHook, LW_OPTION_KERNEL_REBOOT);       /*  安装系统重启回调            */
#endif
    
#if LW_CFG_SHELL_EN > 0
    __ldShellInit();
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

#if LW_CFG_TRUSTED_COMPUTING_EN > 0
    bspTrustedModuleInit();                                             /*  初始化可信计算模块          */
#endif
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
