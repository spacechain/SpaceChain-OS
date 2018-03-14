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
** 文   件   名: loader_exec.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 03 月 26 日
**
** 描        述: unistd exec family of functions.
**
** 注        意: sylixos 所有环境变量均为全局属性, 所以当使用 envp 时, 整个系统的环境变量都会随之改变.
*********************************************************************************************************/

#ifndef __LOADER_EXEC_H
#define __LOADER_EXEC_H

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

/*********************************************************************************************************
  进程启动参数
*********************************************************************************************************/
#if defined(__SYLIXOS_KERNEL) && defined(__SYLIXOS_SPAWN)

#include "spawn.h"

typedef struct {
    LW_LD_VPROC            *SA_pvproc;
    INT                     SA_iArgs;
    INT                     SA_iEvns;
    PCHAR                   SA_pcPath;
    PCHAR                   SA_pcWd;
    PCHAR                   SA_pcParamList[LW_CFG_SHELL_MAX_PARAMNUM];
    PCHAR                   SA_pcpcEvn[LW_CFG_SHELL_MAX_PARAMNUM];
    LW_LIST_LINE_HEADER     SA_plineActions;
    posix_spawnattr_t       SA_spawnattr;                               /*  posix spawn attribute       */
} __SPAWN_ARG;
typedef __SPAWN_ARG        *__PSPAWN_ARG;

__PSPAWN_ARG        __spawnArgCreate(VOID);
VOID                __spawnArgFree(__PSPAWN_ARG  psarg);
LW_LIST_LINE_HEADER __spawnArgActionDup(LW_LIST_LINE_HEADER  plineAction);
INT                 __processStart(INT  mode, __PSPAWN_ARG  psarg);

#endif                                                                  /*  KERNEL && SPAWN             */
/*********************************************************************************************************
  api
*********************************************************************************************************/

LW_API int execl(  const char *path, const char *argv0, ...);
LW_API int execle( const char *path, const char *argv0, ... /*, char * const *envp */);
LW_API int execlp( const char *file, const char *argv0, ...);
LW_API int execv(  const char *path, char * const *argv);
LW_API int execve( const char *path, char * const *argv, char * const *envp);
LW_API int execvp( const char *file, char * const *argv);
LW_API int execvpe(const char *file, char * const *argv, char * const *envp);

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  __LOADER_EXEC_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
