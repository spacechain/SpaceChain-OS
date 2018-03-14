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
** 文   件   名: symLibc.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 03 月 04 日
**
** 描        述: gcc 编译器可能会自动生成调用 memcmp, memset, memcpy and memmove 的程序, 所以, 这些符号
                 必须添加在 sylixos 符号表中, 这样装载器才能装载这个程序, 
                 
                 例如: char  a[] = "abcd"; gcc 编译动态库后, 会自动调用 memcpy 来进行初始化.
                 这里的符号保证了在动态链接时, 不会产生一些符号的未定义错误.
                 
                 可参考: http://gcc.gnu.org/onlinedocs/gcc/Link-Options.html
                 
** BUG:
2011.05.16  加入 setjmp / longjmp 等常用函数.
2012.02.03  加入网络符号.
2012.10.23  sylixos 开始使用自身提供的 setjmp 与 longjmp 功能, 这里不需要独立加入
2012.12.18  rc36 之后, 为了兼容性, lwip_* 开头的符号使用 sylixos socket 包裹的符号 .
*********************************************************************************************************/
#define  __SYLIXOS_INTTYPES
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "setjmp.h"
#include "process.h"
#if LW_CFG_NET_EN > 0
#include "socket.h"
#include "netdb.h"
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_SYMBOL_EN > 0
static LW_SYMBOL    _G_symLibc[] = {
/*********************************************************************************************************
  string
*********************************************************************************************************/
    {   {LW_NULL, LW_NULL}, "__lib_strerror", (caddr_t)lib_strerror,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
/*********************************************************************************************************
  net (为了兼容先早直接使用 lwip socket api 的应用程序)
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
    {   {LW_NULL, LW_NULL}, "lwip_socket", (caddr_t)socket,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_accept", (caddr_t)accept,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_bind", (caddr_t)bind,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_shutdown", (caddr_t)shutdown,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_connect", (caddr_t)connect,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_getsockname", (caddr_t)getsockname,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_getpeername", (caddr_t)getpeername,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_setsockopt", (caddr_t)setsockopt,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_getsockopt", (caddr_t)getsockopt,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_listen", (caddr_t)listen,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_recv", (caddr_t)recv,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_recvfrom", (caddr_t)recvfrom,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_send", (caddr_t)send,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_sendto", (caddr_t)sendto,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_ioctl", (caddr_t)ioctl,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_gethostbyname", (caddr_t)gethostbyname,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_gethostbyname_r", (caddr_t)gethostbyname_r,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_freeaddrinfo", (caddr_t)freeaddrinfo,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_getaddrinfo", (caddr_t)getaddrinfo,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
/*********************************************************************************************************
  net 地址转换兼容
*********************************************************************************************************/
    {   {LW_NULL, LW_NULL}, "ipaddr_aton", (caddr_t)ip4addr_aton,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "ipaddr_ntoa", (caddr_t)ip4addr_ntoa,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "ipaddr_ntoa_r", (caddr_t)ip4addr_ntoa_r,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
  process
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
    {   {LW_NULL, LW_NULL}, "_execl", (caddr_t)execl,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_execle", (caddr_t)execle,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_execlp", (caddr_t)execlp,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_execv", (caddr_t)execv,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_execve", (caddr_t)execve,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_execvp", (caddr_t)execvp,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_execvpe", (caddr_t)execvpe,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    
    {   {LW_NULL, LW_NULL}, "_spawnl", (caddr_t)spawnl,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_spawnle", (caddr_t)spawnle,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_spawnlp", (caddr_t)spawnlp,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_spawnv", (caddr_t)spawnv,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_spawnve", (caddr_t)spawnve,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_spawnvp", (caddr_t)spawnvp,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_spawnvpe", (caddr_t)spawnvpe,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  longjmp setjmp
*********************************************************************************************************/
    {   {LW_NULL, LW_NULL}, "_longjmp", (caddr_t)longjmp,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_setjmp", (caddr_t)setjmp,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
};
/*********************************************************************************************************
** 函数名称: __symbolAddLibc
** 功能描述: 向符号表中添加 libc 符号 
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __symbolAddLibc (VOID)
{
    return  (API_SymbolAddStatic(_G_symLibc, (sizeof(_G_symLibc) / sizeof(LW_SYMBOL))));
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
