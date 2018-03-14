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
** 文   件   名: execinfo.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 10 日
**
** 描        述: execinfo 兼容库.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../include/px_execinfo.h"                                     /*  已包含操作系统头文件        */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0 && LW_CFG_MODULELOADER_EN > 0
#include "assert.h"
#include "dlfcn.h"
/*********************************************************************************************************
** 函数名称: backtrace_symbols
** 功能描述: 通过调用栈地址信息, 获取对应符号
** 输　入  : array     调用栈地址信息
**           size      数组大小
** 输　出  : 符号列表
** 全局变量: 
** 调用模块: 
** 注  意  : 必须使用 free() 释放此函数返回的缓冲区.
                                           API 函数
*********************************************************************************************************/
LW_API 
char  **backtrace_symbols (void *const *array, int size)
{
#define WORD_WIDTH 16

    Dl_info info[size];
    int status[size];
    int cnt;
    size_t total = 0;
    char **result;

    /* Fill in the information we can get from `dladdr'.  */
    for (cnt = 0; cnt < size; ++cnt) {
        status[cnt] = dladdr (array[cnt], &info[cnt]);
        if (status[cnt] && info[cnt].dli_fname && info[cnt].dli_fname[0] != '\0') {
            /* We have some info, compute the length of the string which will be
               "<file-name>(<sym-name>+offset) [address].  */
            total += (lib_strlen (info[cnt].dli_fname ?: "")
                  + lib_strlen (info[cnt].dli_sname ?: "")
                  + 3 + WORD_WIDTH + 3 + WORD_WIDTH + 5);
        } else {
            total += 5 + WORD_WIDTH;
        }
    }

    /* Allocate memory for the result.  */
    result = (char **) lib_malloc (size * sizeof (char *) + total);
    if (result != NULL) {
        char *last = (char *) (result + size);

        for (cnt = 0; cnt < size; ++cnt) {
            result[cnt] = last;

            if (status[cnt]
                && info[cnt].dli_fname != NULL && info[cnt].dli_fname[0] != '\0') {
                if (info[cnt].dli_sname == NULL) {
                    /* We found no symbol name to use, so describe it as
                       relative to the file.  */
                    info[cnt].dli_saddr = info[cnt].dli_fbase;
                }

                if (info[cnt].dli_sname == NULL && info[cnt].dli_saddr == 0) {
                    last += 1 + sprintf (last, "%s(%s) [%p]",
                                 info[cnt].dli_fname ?: "",
                                 info[cnt].dli_sname ?: "",
                                 array[cnt]);
                } else {
                    char sign;
                    ptrdiff_t offset;
                    if (array[cnt] >= (void *) info[cnt].dli_saddr) {
                        sign = '+';
                        offset = array[cnt] - info[cnt].dli_saddr;
                    
                    } else {
                        sign = '-';
                        offset = info[cnt].dli_saddr - array[cnt];
                    }

                    last += 1 + sprintf (last, "%s(%s%c%#tx) [%p]",
                                 info[cnt].dli_fname ?: "",
                                 info[cnt].dli_sname ?: "",
                                 sign, offset, array[cnt]);
                }
            } else {
                last += 1 + sprintf (last, "[%p]", array[cnt]);
            }
        }
        assert (last <= (char *) result + size * sizeof (char *) + total);
    }

    return result;
}
/*********************************************************************************************************
** 函数名称: backtrace_symbols
** 功能描述: 通过调用栈地址信息, 获取对应符号
** 输　入  : array     调用栈地址信息
**           size      数组大小
** 输　出  : 符号列表
** 全局变量: 
** 调用模块: 
** 注  意  : 此函数放在进程补丁中实现支持.
                                           API 函数
*********************************************************************************************************/
LW_API 
void  backtrace_symbols_fd (void *const *array, int size, int fd)
{
    struct iovec iov[9];
    int cnt;

    for (cnt = 0; cnt < size; ++cnt) {
        char buf[WORD_WIDTH];
        char buf2[WORD_WIDTH];
        Dl_info info;
        size_t last = 0;

        if (dladdr (array[cnt], &info)
            && info.dli_fname != NULL && info.dli_fname[0] != '\0') {
            /* Name of the file.  */
            iov[0].iov_base = (void *) info.dli_fname;
            iov[0].iov_len = lib_strlen (info.dli_fname);
            last = 1;

            if (info.dli_sname != NULL) {
                size_t diff;

                iov[last].iov_base = (void *) "(";
                iov[last].iov_len = 1;
                ++last;

                if (info.dli_sname != NULL) {
                    /* We have a symbol name.  */
                    iov[last].iov_base = (void *) info.dli_sname;
                    iov[last].iov_len = lib_strlen (info.dli_sname);
                    ++last;
                }

                if (array[cnt] >= (void *) info.dli_saddr) {
                    iov[last].iov_base = (void *) "+0x";
                    diff = array[cnt] - info.dli_saddr;
                
                } else {
                    iov[last].iov_base = (void *) "-0x";
                    diff = info.dli_saddr - array[cnt];
                }
                iov[last].iov_len = 3;
                ++last;

                iov[last].iov_base = lib_itoa((int)diff, buf2, 16);
                iov[last].iov_len  = (&buf2[WORD_WIDTH]
                                   - (char *) iov[last].iov_base);
                ++last;

                iov[last].iov_base = (void *) ")";
                iov[last].iov_len = 1;
                ++last;
            }
        }

        iov[last].iov_base = (void *) "[0x";
        iov[last].iov_len = 3;
        ++last;

        iov[last].iov_base = lib_itoa((int)array[cnt], buf, 16);
        iov[last].iov_len = &buf[WORD_WIDTH] - (char *) iov[last].iov_base;
        ++last;

        iov[last].iov_base = (void *) "]\n";
        iov[last].iov_len = 2;
        ++last;

        writev(fd, iov, last);
    }
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
