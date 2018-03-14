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
** 文   件   名: ttinyShellSysCmd.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 09 月 05 日
**
** 描        述: 一个超小型的 shell 系统, 系统环境变量定义.

** BUG:
2009.12.11  加入 PATH 变量, 以供应用程序获取工作路径.
2010.05.05  PATH 初始化为 /bin 目录(以 : 作为环境变量不同值间的分隔符).
2011.03.05  更改变量名
2011.03.10  加入 syslog socket 环境变量.
2011.06.10  加入 TZ 环境变量.
2011.07.08  加入 CALIBRATE 变量.
2012.03.21  加入 NFS_CLIENT_AUTH 变量, 指定 NFS 使用 AUTH_NONE(windows) 还是 AUTH_UNIX
2012.09.21  加入对 locale 环境变量的初始化.
2013.01.23  加入对 NFS_CLIENT_PROTO 环境变量初始化.
2013.06.12  SylixOS 默认不再使用 ftk 图形界面, 转而使用 Qt 图形界面.
2015.04.06  去掉 GUILIB GUIFONT ... 默认环境变量.
2017.01.09  加入网络黑名单参数环境变量.
2017.09.14  加入 STARTUP_WAIT_SEC 环境变量.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "paths.h"
#if LW_CFG_NET_EN > 0
#include "lwip/inet.h"
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
** 函数名称: __tshellSysVarInit
** 功能描述: 初始化系统环境变量
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __tshellSysVarInit (VOID)
{
    /*
     *  系统信息
     */
    API_TShellExec("SYSTEM=\"" __SYLIXOS_VERINFO "\"");
    API_TShellExec("VERSION=\"" __SYLIXOS_VERSTR "\"");
    API_TShellExec("LICENSE=\"" __SYLIXOS_LICENSE "\"");

    API_TShellExec("TMPDIR=" LW_CFG_TMP_PATH);
    API_TShellExec("TZ=CST-8:00:00");                                   /*  默认为东8区                 */
    API_TShellExec("STARTUP_WAIT_SEC=1");                               /*  执行 startup.sh 延迟时间    */
                                                                        /*  0 ~ 10 之间                 */
    /*
     *  图形界面
     */
    API_TShellExec("KEYBOARD=/dev/input/keyboard0");                    /*  HID 设备                    */
    API_TShellExec("MOUSE=/dev/input/mouse0:/dev/input/touch0");
    
    API_TShellExec("TSLIB_TSDEVICE=/dev/input/touch0");                 /*  触摸屏校准关联设备          */
    API_TShellExec("TSLIB_CALIBFILE=/etc/pointercal");                  /*  触摸屏校准文件              */
    
    /*
     *  如果可装载的应用程序模块使用了 SylixOS 模块初始化库, 
     *  则模块可以动态分配的内存大小如下环境变量确定.
     */
#if !defined(LW_CFG_CPU_ARCH_C6X)
    API_TShellExec("SO_MEM_PAGES=8192");                                /*  动态内存虚拟页面数量        */
                                                                        /*  默认为 32 MB                */
#else
    API_TShellExec("SO_MEM_PAGES=256");                                 /*  动态内存虚拟页面数量 2MB    */
#endif

#if LW_KERN_FLOATING > 0
    API_TShellExec("KERN_FLOAT=1");                                     /*  内核支持浮点格式            */
#else
    API_TShellExec("KERN_FLOAT=0");                                     /*  内核不支持浮点格式          */
#endif                                                                  /*  LW_KERN_FLOATING            */

#if LW_CFG_POSIX_EN > 0
    API_TShellExec("SYSLOGD_HOST=0.0.0.0:514");                         /*  syslog 服务器地址           */
#endif

    API_TShellExec("NFS_CLIENT_AUTH=AUTH_UNIX");                        /*  NFS 默认使用 auth_unix      */
    API_TShellExec("NFS_CLIENT_PROTO=udp");                             /*  NFS 默认使用 udp 协议       */

    API_TShellExec("PATH=" _PATH_DEFPATH);                              /*  PATH 启动时默认路径         */
    API_TShellExec("LD_LIBRARY_PATH=" _PATH_LIBPATH);                   /*  LD_LIBRARY_PATH 默认值      */
    
    /*
     *  多国语言与编码
     */
    API_TShellExec("LANG=C");                                           /*  系统默认 locale 为 "C"      */
    API_TShellExec("LC_ALL=");                                          /*  推荐不要使用此变量          */
    API_TShellExec("PATH_LOCALE=" _PATH_LOCALE);                        /*  注意:需要从 BSD 系统将 UTF-8*/
                                                                        /*  目录拷贝到这里              */
    /*
     *  调试
     */
#if LW_CFG_GDB_EN > 0
    API_TShellExec("DEBUG_CPU=-1");                                     /*  是否将被调对象锁定到一个 CPU*/
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
    
#if LW_CFG_NET_LOGINBL_EN > 0
    API_TShellExec("LOGINBL_TO=120");                                   /*  网络登录黑名单刷新时间      */
    API_TShellExec("LOGINBL_REP=3");                                    /*  连续出现几次则加入黑名单    */
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
    
    /*
     *  进程默认控制
     */
#if LW_CFG_MODULELOADER_EN > 0
    API_TShellExec("VPROC_EXIT_FORCE=0");                               /*  1: 主线程退出自动删除子线程 */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    /*
     *  LUA 环境
     */
    API_TShellExec("LUA_PATH=?.lua;/usr/local/lib/lua/?.lua;/usr/lib/lua/?.lua;/lib/lua/?.lua");
    API_TShellExec("LUA_CPATH=?.so;/usr/local/lib/lua/?.so;/usr/lib/lua/?.so;/lib/lua/?.so");
    
    /*
     *  终端
     */                                                                    
    API_TShellExec("TERM=vt100");
    API_TShellExec("TERMCAP=/etc/termcap");                             /*  BSD 终端转义                */
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
