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
** 文   件   名: nl_reent.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 08 月 15 日
**
** 描        述: newlib fio 兼容层. (SylixOS 和 VxWorks 类似, 必须使用自己的 libc 库)
                 很多 gcc 使用 newlib 作为 libc, 其他的库也都依赖于 newlib, 例如 libstdc++ 库, 
                 SylixOS 要想使用这些库, 则必须提供一个 newlib reent 兼容的接口.
                 
2012.11.09  lib_nlreent_init() 修正错误的缓冲类型.
2014.11.08  加入 __getreent() 函数, 支持 newlib 使用 __DYNAMIC_REENT__ 编译支持 SMP 系统.
2015.06.23  减少内存使用量.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
#include "../libc/stdio/lib_file.h"
/*********************************************************************************************************
  newlib compatible reent
*********************************************************************************************************/
typedef struct __nl_reent {
    /*
     *  ._errno 不使用, 为了二进制兼容, 这里仅是占位符而已.
     *  因为 newlib errno 的定义为 *__errno() 与 SylixOS 相同, 所以如果使用了 errno 就一定会定位到 SylixOS
     *  中的 __errno() 函数.
     */
    int         _errno_notuse;                                          /*  not use!                    */
    FILE       *_stdin, *_stdout, *_stderr;                             /*  三个标准文件                */
} __NL_REENT;

typedef struct __lw_reent {
    FILE        _file[3];                                               /*  三个标准文件                */
    /*
     *  以下变量为 newlib 兼容部分.
     */
    __NL_REENT  _nl_com;
} __LW_REENT;
/*********************************************************************************************************
  newlib compatible reent for all thread
*********************************************************************************************************/
static __LW_REENT _G_lwreentTbl[LW_CFG_MAX_THREADS];                    /*  每个任务都拥有一个 reent    */
/*********************************************************************************************************
  newlib compatible reent (此方法不支持 SMP 系统)
*********************************************************************************************************/
struct __nl_reent *LW_WEAK _impure_ptr;                                 /*  当前 newlib reent 上下文    */
/*********************************************************************************************************
** 函数名称: __nlreent_swtich_hook
** 功能描述: newlib 使用 __DYNAMIC_REENT__ 编译时才能支持 SMP 系统, 这里提供必要函数.
** 输　入  : NONE
** 输　出  : newlib 线程上下文
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
struct __nl_reent *__getreent (VOID)
{
    REGISTER PLW_CLASS_TCB   ptcbCur;
    REGISTER __LW_REENT     *plwreent;

    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    plwreent = &_G_lwreentTbl[ptcbCur->TCB_usIndex];
    
    return  (&plwreent->_nl_com);
}
/*********************************************************************************************************
** 函数名称: __nlreent_swtich_hook
** 功能描述: nl reent 任务调度 hook
** 输　入  : ulOldThread   将被调度器换出的线程
**           ulNewThread   将要运行的线程.
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 适用于单核模式.
*********************************************************************************************************/
static VOID  __nlreent_swtich_hook (LW_OBJECT_HANDLE  ulOldThread, LW_OBJECT_HANDLE  ulNewThread)
{
    REGISTER __LW_REENT  *plwreent;
    
    plwreent    = &_G_lwreentTbl[_ObjectGetIndex(ulNewThread)];
    _impure_ptr = &plwreent->_nl_com;
}
/*********************************************************************************************************
** 函数名称: __nlreent_delete_hook
** 功能描述: nl reent 任务删除 hook
** 输　入  : ulThread      线程
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __nlreent_delete_hook (LW_OBJECT_HANDLE  ulThread)
{
    INT          i;
    __LW_REENT  *plwreent = &_G_lwreentTbl[_ObjectGetIndex(ulThread)];
    __NL_REENT  *pnlreent = &plwreent->_nl_com;
    
    /*
     *  注意, 当 stdin stdout 或 stderr 被重定向了, 就不用关闭了, 这里只关闭最原始的三个 std 文件.
     */
    for (i = 0; i < 3; i++) {
        if (plwreent->_file[i]._flags) {
            fclose_ex(&plwreent->_file[i], LW_TRUE, LW_FALSE);
        }
    }
    
    pnlreent->_stdin  = LW_NULL;
    pnlreent->_stdout = LW_NULL;
    pnlreent->_stderr = LW_NULL;
}
/*********************************************************************************************************
** 函数名称: lib_nlreent_init
** 功能描述: 初始化指定线程的 nl reent 结构
** 输　入  : ulThread      线程 ID
** 输　出  : newlib 兼容 reent 结构
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  lib_nlreent_init (LW_OBJECT_HANDLE  ulThread)
{
    static BOOL  bSwpHook = LW_FALSE;
    static BOOL  bDelHook = LW_FALSE;
    
    __LW_REENT  *plwreent = &_G_lwreentTbl[_ObjectGetIndex(ulThread)];
    __NL_REENT  *pnlreent = &plwreent->_nl_com;
    
    if (bSwpHook == LW_FALSE) {
        if (API_SystemHookAdd(__nlreent_swtich_hook, LW_OPTION_THREAD_SWAP_HOOK) == ERROR_NONE) {
            bSwpHook = LW_TRUE;
        }
    }
    
    if (bDelHook == LW_FALSE) {
        if (API_SystemHookAdd(__nlreent_delete_hook, LW_OPTION_THREAD_DELETE_HOOK) == ERROR_NONE) {
            bDelHook = LW_TRUE;
        }
    }
    
    pnlreent->_stdin  = &plwreent->_file[STDIN_FILENO];
    pnlreent->_stdout = &plwreent->_file[STDOUT_FILENO];
    pnlreent->_stderr = &plwreent->_file[STDERR_FILENO];
    
    __lib_newfile(pnlreent->_stdin);                                    /* 标准文件初始化不存在内存分配 */
    __lib_newfile(pnlreent->_stdout);
    __lib_newfile(pnlreent->_stderr);
    
    /*
     *  stdin init flags
     */
    pnlreent->_stdin->_flags = __SRD;
#if LW_CFG_FIO_STDIN_LINE_EN > 0
    pnlreent->_stdin->_flags |= __SLBF;
#endif                                                                  /* LW_CFG_FIO_STDIN_LINE_EN     */

    /*
     *  stdout init flags
     */
    pnlreent->_stdout->_flags = __SWR;
#if LW_CFG_FIO_STDIN_LINE_EN > 0
    pnlreent->_stdout->_flags |= __SLBF;
#endif                                                                  /* LW_CFG_FIO_STDIN_LINE_EN     */

    /*
     *  stderr init flags
     */
    pnlreent->_stderr->_flags = __SWR;
#if LW_CFG_FIO_STDERR_LINE_EN > 0
    pnlreent->_stderr->_flags |= __SNBF;
#endif                                                                  /* LW_CFG_FIO_STDERR_LINE_EN    */

    pnlreent->_stdin->_file  = STDIN_FILENO;
    pnlreent->_stdout->_file = STDOUT_FILENO;
    pnlreent->_stderr->_file = STDERR_FILENO;
}
/*********************************************************************************************************
** 函数名称: lib_nlreent_stdfile
** 功能描述: 获取指定线程的 stdfile 结构 
** 输　入  : FileNo        文件号, 0, 1, 2
** 输　出  : stdfile 指针地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
FILE **lib_nlreent_stdfile (INT  FileNo)
{
    REGISTER __LW_REENT    *plwreent;
    REGISTER __NL_REENT    *pnlreent;
    REGISTER PLW_CLASS_TCB  ptcbCur;

    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        return  (LW_NULL);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    plwreent = &_G_lwreentTbl[_ObjectGetIndex(ptcbCur->TCB_ulId)];
    pnlreent = &plwreent->_nl_com;
    
    switch (FileNo) {
    
    case STDIN_FILENO:
        return  (&pnlreent->_stdin);
        
    case STDOUT_FILENO:
        return  (&pnlreent->_stdout);
        
    case STDERR_FILENO:
        return  (&pnlreent->_stderr);
        
    default:
        return  (LW_NULL);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_FIO_LIB_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
