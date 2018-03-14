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
** 文   件   名: zlib_sylixos.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 01 月 16 日
**
** 描        述: 操作系统 zlib 测试接口.

** BUG:
2010.02.08  使用同步背景模式执行.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  函数声明 (注意, zlib 对堆栈的消耗非常大, 所以使用时一定要加大堆栈)
*********************************************************************************************************/
int minigzip_main(int argc, char **argv);
int zlib_main(int argc, char* argv[]);
/*********************************************************************************************************
** 函数名称: luaShellInit
** 功能描述: 初始化 zlib shell 接口
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  zlibShellInit (VOID)
{
#if LW_CFG_SHELL_EN > 0
    /*
     *  注册 lua 虚拟机及其编译器.
     */
    API_TShellKeywordAddEx("gzip",  minigzip_main, LW_OPTION_KEYWORD_SYNCBG);
    API_TShellFormatAdd("gzip",     " [-c] [-d] [-f] [-h] [-r] [-1 to -9] [files...]");
    API_TShellHelpAdd("gzip",       "Usage:  gzip [-d] [-f] [-h] [-r] [-1 to -9] [files...]\n"
                                    "       -c : write to standard output\n"
                                    "       -d : decompress\n"
                                    "       -f : compress with Z_FILTERED\n"
                                    "       -h : compress with Z_HUFFMAN_ONLY\n"
                                    "       -r : compress with Z_RLE\n"
                                    "       -1 to -9 : compression level\n");
    
    API_TShellKeywordAddEx("zlib", zlib_main, LW_OPTION_KEYWORD_SYNCBG);
    API_TShellFormatAdd("zlib",    " [output.gz  [input.gz]]");
    API_TShellHelpAdd("zlib",      "zlib test.");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
