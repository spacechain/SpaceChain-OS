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
** 文   件   名: process.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 03 月 30 日
**
** 描        述: UNIX process.
*********************************************************************************************************/

#ifndef __PROCESS_H
#define __PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

int execl(  const char *path, const char *argv0, ...);
int execle( const char *path, const char *argv0, ... /*, char * const *envp */);
int execlp( const char *file, const char *argv0, ...);
int execv(  const char *path, char * const *argv);
int execve( const char *path, char * const *argv, char * const *envp);
int execvp( const char *file, char * const *argv);
int execvpe(const char *file, char * const *argv, char * const *envp);

int _execl(  const char *path, const char *argv0, ...);
int _execle( const char *path, const char *argv0, ... /*, char * const *envp */);
int _execlp( const char *file, const char *argv0, ...);
int _execv(  const char *path, char * const *argv);
int _execve( const char *path, char * const *argv, char * const *envp);
int _execvp( const char *file, char * const *argv);
int _execvpe(const char *file, char * const *argv, char * const *envp);

int spawnl(  int mode, const char *path, const char *argv0, ...);
int spawnle( int mode, const char *path, const char *argv0, ... /*, char * const *envp */);
int spawnlp( int mode, const char *file, const char *argv0, ...);
int spawnv(  int mode, const char *path, char * const *argv);
int spawnve( int mode, const char *path, char * const *argv, char * const *envp);
int spawnvp( int mode, const char *file, char * const *argv);
int spawnvpe(int mode, const char *file, char * const *argv, char * const *envp);

int _spawnl(  int mode, const char *path, const char *argv0, ...);
int _spawnle( int mode, const char *path, const char *argv0, ... /*, char * const *envp */);
int _spawnlp( int mode, const char *file, const char *argv0, ...);
int _spawnv(  int mode, const char *path, char * const *argv);
int _spawnve( int mode, const char *path, char * const *argv, char * const *envp);
int _spawnvp( int mode, const char *file, char * const *argv);
int _spawnvpe(int mode, const char *file, char * const *argv, char * const *envp);

#define _P_WAIT		1
#define _P_NOWAIT	2
#define _P_OVERLAY	3
#define _P_NOWAITO	4
#define _P_DETACH	5

#define P_WAIT      _P_WAIT
#define P_NOWAIT    _P_NOWAIT
#define P_OVERLAY   _P_OVERLAY
#define P_NOWAITO   _P_NOWAITO
#define P_DETACH	_P_DETACH

#ifndef _OLD_P_OVERLAY
#define _OLD_P_OVERLAY  _P_OVERLAY
#endif

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __PROCESS_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
