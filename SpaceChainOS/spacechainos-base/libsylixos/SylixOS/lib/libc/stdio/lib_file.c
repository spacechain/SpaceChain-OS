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
** 文   件   名: lib_file.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 09 月 04 日
**
** 描        述: 类标准 C 库支持, 标准文件创建及获取函数

** BUG:
2009.02.15  标准文件中仅仅 stdin 需要缓冲区.
2009.05.28  删除线程标准文件时, 需要将相关 TCB 指针清零, 防止 ThreadRestart 错误.
2009.07.11  在创建标准文件时必须加入删除钩子函数(不管什么条件).
2011.07.20  文件 FILE 内存必须从 malloc 开辟, 这样进程退出时, 可释放掉内存.
2012.12.07  将 FILE 加入资源管理器.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "lib_stdio.h"
#include "local.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
/*********************************************************************************************************
** 函数名称: __lib_newfile
** 功能描述: 创建一个文件结构
** 输　入  : pfFile        文件指针 (如果为 NULL 将创建)
** 输　出  : 文件指针
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
FILE  *__lib_newfile (FILE  *pfFile)
{
    BOOL    bNeedRes = LW_TRUE;

    if (pfFile == LW_NULL) { 
        pfFile = (FILE *)lib_malloc(sizeof(FILE));                      /* 分配内核文件指针             */
        if (!pfFile) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (LW_NULL);
        }
    
    } else {
        bNeedRes = LW_FALSE;                                            /* 标准 IO 文件                 */
    }
    
    lib_bzero(pfFile, sizeof(FILE));
    
    pfFile->_p        = LW_NULL;                                        /* no current pointer           */
    pfFile->_r        = 0;
    pfFile->_w        = 0;                                              /* nothing to read or write     */
    pfFile->_flags    = 1;                                              /* caller sets real flags       */
    pfFile->_file     = -1;                                             /* no file                      */
    pfFile->_bf._base = LW_NULL;                                        /* no buffer                    */
    pfFile->_bf._size = 0;
    pfFile->_lbfsize  = 0;                                              /* not line buffered            */
    pfFile->_ub._base = LW_NULL;                                        /* no ungetc buffer             */
    pfFile->_ub._size = 0;
    pfFile->_lb._base = LW_NULL;                                        /* no line buffer               */
    pfFile->_lb._size = 0;
    pfFile->_blksize  = 0;
    pfFile->_offset   = 0;
    pfFile->_cookie   = (void *)pfFile;
    
    pfFile->_close = __sclose;
    pfFile->_read  = __sread;
    pfFile->_seek  = __sseek;
    pfFile->_write = __swrite;
    
    if (bNeedRes) {
        __resAddRawHook(&pfFile->resraw, (VOIDFUNCPTR)fclose_ex, 
                        pfFile, (PVOID)LW_FALSE, (PVOID)LW_TRUE, 0, 0, 0);
    }
    
    return  (pfFile);
}
/*********************************************************************************************************
** 函数名称: __lib_delfile
** 功能描述: 删除一个文件结构
** 输　入  : 文件指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __lib_delfile (FILE  *pfFile)
{
    __resDelRawHook(&pfFile->resraw);

    if (pfFile) {
        lib_free(pfFile);
    }
}
/*********************************************************************************************************
** 函数名称: __lib_stdin
** 功能描述: 获得当前 stdin 文件指针
** 输　入  : NONE
** 输　出  : 文件指针的地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
FILE  **__lib_stdin (VOID)
{
    return  (lib_nlreent_stdfile(STDIN_FILENO));
}
/*********************************************************************************************************
** 函数名称: __lib_stdout
** 功能描述: 获得当前 stdout 文件指针
** 输　入  : NONE
** 输　出  : 文件指针的地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
FILE  **__lib_stdout (VOID)
{
    return  (lib_nlreent_stdfile(STDOUT_FILENO));
}
/*********************************************************************************************************
** 函数名称: __lib_stderr
** 功能描述: 获得当前 stderr 文件指针
** 输　入  : NONE
** 输　出  : 文件指针的地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
FILE  **__lib_stderr (VOID)
{
    return  (lib_nlreent_stdfile(STDERR_FILENO));
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_FIO_LIB_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
