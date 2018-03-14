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
** 文   件   名: s_globalvar.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 13 日
**
** 描        述: 这是系统全局变量定义文件。

** BUG
2007.12.06  系统异常处理线程需要进入安全模式,不能被删除.
2008.06.12  加入日志系统.
2012.08.25  _S_pfileentryTbl 大小不需要 + 3
2012.12.21  内核文件描述符表放在IoFile.c 中
*********************************************************************************************************/

#ifndef __S_GLOBALVAR_H
#define __S_GLOBALVAR_H

#ifdef  __SYSTEM_MAIN_FILE
#define __SYSTEM_EXT
#else
#define __SYSTEM_EXT    extern
#endif

/*********************************************************************************************************
    IO SYSTEM
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
__SYSTEM_EXT  LW_DEV_ENTRY          _S_deventryTbl[LW_CFG_MAX_DRIVERS]; /*  驱动程序表                  */
__SYSTEM_EXT  LW_LIST_LINE_HEADER   _S_plineDevHdrHeader;               /*  设备表表头                  */

__SYSTEM_EXT  LW_LIST_LINE_HEADER   _S_plineFileEntryHeader;            /*  文件结构表头                */
__SYSTEM_EXT  atomic_t              _S_atomicFileLineOp;                /*  遍历文件结构表锁            */
__SYSTEM_EXT  BOOL                  _S_bFileEntryRemoveReq;             /*  文件结构链表有请求删除的节点*/

__SYSTEM_EXT  LW_IO_ENV             _S_ioeIoGlobalEnv;                  /*  全局 io 环境                */
__SYSTEM_EXT  INT                   _S_iIoMaxLinkLevels;                /*  maximum number of symlinks  */
                                                                        /*  to traverse                 */
/*********************************************************************************************************
  IO 回调函数组
*********************************************************************************************************/

__SYSTEM_EXT  pid_t               (*_S_pfuncGetCurPid)();               /*  获得当前 pid                */
__SYSTEM_EXT  PLW_FD_ENTRY        (*_S_pfuncFileGet)();                 /*  获得对应 fd 的 fdentry      */
__SYSTEM_EXT  PLW_FD_DESC         (*_S_pfuncFileDescGet)();             /*  获得对应 fd 的 filedesc     */
__SYSTEM_EXT  INT                 (*_S_pfuncFileDup)();                 /*  dup                         */
__SYSTEM_EXT  INT                 (*_S_pfuncFileDup2)();                /*  dup2                        */
__SYSTEM_EXT  INT                 (*_S_pfuncFileRefInc)();              /*  将文件描述符引用计数++      */
__SYSTEM_EXT  INT                 (*_S_pfuncFileRefDec)();              /*  将文件描述符引用计数--      */
__SYSTEM_EXT  INT                 (*_S_pfuncFileRefGet)();              /*  获得文件描述符引用计数      */

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
    THREAD POOL SYSTEM
*********************************************************************************************************/
#if LW_CFG_THREAD_POOL_EN > 0 && LW_CFG_MAX_THREAD_POOLS > 0
__SYSTEM_EXT  LW_CLASS_THREADPOOL   _S_threadpoolBuffer[LW_CFG_MAX_THREAD_POOLS];  
                                                                        /*  线程池控制块缓冲区          */
__SYSTEM_EXT  LW_CLASS_OBJECT_RESRC _S_resrcThreadPool;                 /*  线程池对象资源结构          */
#endif                                                                  /*  LW_CFG_THREAD_POOL_EN > 0   */
/*********************************************************************************************************
    服务线程相关
*********************************************************************************************************/
__SYSTEM_EXT  LW_OBJECT_ID          _S_ulThreadExceId;                  /*  信号服务线程                */

#if LW_CFG_LOG_LIB_EN > 0
__SYSTEM_EXT  LW_OBJECT_ID          _S_ulThreadLogId;                   /*  日志服务线程                */
#endif

#if LW_CFG_POWERM_EN > 0
__SYSTEM_EXT  LW_OBJECT_ID          _S_ulPowerMId;                      /*  功耗管理线程                */
#endif

#endif                                                                  /*  __S_GLOBALVAR_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
