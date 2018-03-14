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
** 文   件   名: loader.h
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2010 年 04 月 17 日
**
** 描        述: loader 导出接口

** BUG: 
2011.02.20  加入 unload 接口与符号表区分.
*********************************************************************************************************/

#ifndef __LOADER_H
#define __LOADER_H

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

#include "loader_error.h"                                               /*  loader errno                */
#include "loader_module.h"                                              /*  内核模块装载器              */

/*********************************************************************************************************
  API_ModuleLoad mode
*********************************************************************************************************/

#define LW_OPTION_LOADER_SYM_LOCAL          0                           /*  装载后在局部符号表中        */
#define LW_OPTION_LOADER_SYM_GLOBAL         1                           /*  装载后在全局符号表中        */

/*********************************************************************************************************
  loader API
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL

LW_API VOID API_LoaderInit(VOID);                                       /*  初始化 loader               */

#define loaderInit          API_LoaderInit

#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  module API (pvVProc 通常为 NULL, 不属于任何进程)
*********************************************************************************************************/

LW_API PVOID    API_ModuleLoad(CPCHAR  pcFile, 
                               INT     iMode,
                               CPCHAR  pcInit,
                               CPCHAR  pcExit,
                               PVOID   pvVProc);                        /*  加载模块                    */
                               
LW_API PVOID    API_ModuleLoadEx(CPCHAR  pcFile, 
                                 INT     iMode,
                                 CPCHAR  pcInit,
                                 CPCHAR  pcExit,
                                 CPCHAR  pcEntry,
                                 CPCHAR  pcSection,
                                 PVOID   pvVProc);                      /*  仅加载指定 section 符号     */

LW_API INT      API_ModuleUnload(PVOID  pvModule);                      /*  卸载模块                    */

LW_API PVOID    API_ModuleSym(PVOID  pvModule, CPCHAR  pcName);         /*  查找装载模块的本地符号表    */
                                
LW_API INT      API_ModuleStatus(CPCHAR  pcFile, INT  iFd);             /*  查看elf文件状态             */

#define moduleLoad          API_ModuleLoad
#define moduleUnload        API_ModuleUnload
#define moduleSym           API_ModuleSym
#define moduleStatus        API_ModuleStatus

/*********************************************************************************************************
  module 内部 API 
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL

LW_API INT      API_ModuleRun(CPCHAR  pcFile, CPCHAR  pcEntry);         /*  运行模块                    */

LW_API INT      API_ModuleRunEx(CPCHAR  pcFile,
                                CPCHAR  pcEntry,
                                INT     iArgC,
                                CPCHAR  ppcArgV[],
                                CPCHAR  ppcEnv[]);                      /*  带参数运行模块              */
                                
LW_API INT      API_ModuleFinish(PVOID   pvVProc);                      /*  进程结束自清理              */

LW_API INT      API_ModuleTerminal(PVOID   pvVProc);                    /*  回收进程空间                */

#if LW_CFG_MODULELOADER_ATEXIT_EN > 0
LW_API INT      API_ModuleAtExit(VOIDFUNCPTR  pfunc);                   /*  内核模块 atexit()           */
#endif                                                                  /*  LW_CFG_MODULELOADER_ATEXI...*/

#if LW_CFG_MODULELOADER_GCOV_EN > 0
LW_API INT      API_ModuleGcov(PVOID  pvModule);                        /*  内核模块生成代码覆盖率信息  */
#endif                                                                  /*  LW_CFG_MODULELOADER_GCOV_EN */

LW_API ssize_t  API_ModuleGetName(PVOID  pvAddr, PCHAR  pcFullPath, size_t  stLen);

LW_API PVOID    API_ModuleGlobal(CPCHAR  pcFile, INT  iMode, PVOID  pvVProc);
                                                                        /*  修改已装载模块属性          */
LW_API INT      API_ModuleShareRefresh(CPCHAR  pcFileName);             /*  清除(刷新)共享空间缓冲      */

LW_API INT      API_ModuleShareConfig(BOOL  bShare);                    /*  设置共享空间功能            */

LW_API INT      API_ModuleTimes(PVOID    pvVProc, 
                                clock_t *pclockUser, 
                                clock_t *pclockSystem,
                                clock_t *pclockCUser, 
                                clock_t *pclockCSystem);
                                                                        /*  进程运行时间信息            */
LW_API INT      API_ModuleGetBase(pid_t   pid, 
                                  PCHAR   pcModPath, 
                                  addr_t *pulAddrBase,
                                  size_t *pstLen);                      /*  获得模块装载基地址          */
                                                                        
#define moduleRun           API_ModuleRun
#define moduleRunEx         API_ModuleRunEx
#define moduleFinish        API_ModuleFinish
#define moduleTerminal      API_ModuleTerminal
#define moduleAtExit        API_ModuleAtExit
#define moduleGcov          API_ModuleGcov
#define moduleGetName       API_ModuleGetName
#define moduleGlobal        API_ModuleGlobal
#define moduleShareRefresh  API_ModuleShareRefresh
#define moduleShareConfig   API_ModuleShareConfig
#define moduleTimes         API_ModuleTimes

/*********************************************************************************************************
  module API 以下 API 为补丁使用.
*********************************************************************************************************/

LW_API pid_t    API_ModulePid(PVOID  pvVProc);

#if LW_CFG_POSIX_EN > 0
LW_API INT      API_ModuleAddr(PVOID  pvAddr, PVOID  pvDlinfo, PVOID  pvVProc);
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */

#define modulePid           API_ModulePid
#define moduleAddr          API_ModuleAddr

#endif                                                                  /*  __SYLIXOS_KERNEL            */

#endif                                                                  /*  __LOADER_H                  */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
