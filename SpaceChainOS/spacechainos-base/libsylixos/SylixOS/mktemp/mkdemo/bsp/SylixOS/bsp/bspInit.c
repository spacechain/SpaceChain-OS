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
** 文   件   名: bspInit.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 06 月 25 日
**
** 描        述: BSP 用户 C 程序入口
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"                                                     /*  工程配置 & 处理器相关       */
/*********************************************************************************************************
  操作系统相关
*********************************************************************************************************/
#include "SylixOS.h"                                                    /*  操作系统                    */
#include "stdlib.h"                                                     /*  for system() function       */
#include "gdbserver.h"                                                  /*  GDB 调试器                  */
#include "sys/compiler.h"                                               /*  编译器相关                  */
#include "lwip/tcpip.h"                                                 /*  网络系统                    */
#include "netif/etharp.h"                                               /*  以太网网络接口              */
#include "netif/aodvif.h"                                               /*  aodv 多跳自组网网络接口     */
/*********************************************************************************************************
  BSP 及 驱动程序
*********************************************************************************************************/
/*
 * TODO: 包含你的 BSP 及 驱动程序的头文件
 */
/*********************************************************************************************************
  操作系统符号表
*********************************************************************************************************/
#if LW_CFG_SYMBOL_EN > 0 && defined(__GNUC__)
#include "symbol.h"
#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
                                                                        /*  defined(__GNUC__)           */
/*********************************************************************************************************
  内存初始化映射表
*********************************************************************************************************/
#define  __BSPINIT_MAIN_FILE
#include "bspMap.h"
/*********************************************************************************************************
  主线程与启动线程堆栈 (t_boot 可以大一点, startup.sh 中可能有很多消耗堆栈的操作)
*********************************************************************************************************/
#define  __LW_THREAD_BOOT_STK_SIZE      (16 * LW_CFG_KB_SIZE)
#define  __LW_THREAD_MAIN_STK_SIZE      (16 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
  主线程声明
*********************************************************************************************************/
VOID  t_main(VOID);
/*********************************************************************************************************
** 函数名称: halModeInit
** 功能描述: 初始化目标系统运行的模式
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  halModeInit (VOID)
{
    /*
     * TODO: 加入你的处理代码, 但建议不作处理
     */
}
/*********************************************************************************************************
** 函数名称: halTimeInit
** 功能描述: 初始化目标系统时间系统 (系统默认时区为: 东8区)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

static VOID  halTimeInit (VOID)
{
    /*
     * TODO: 加入你的处理代码, 参考代码如下:
     */
#if 0                                                                   /*  参考代码开始                */
    PLW_RTC_FUNCS   prtcfuncs = rtcGetFuncs();

    rtcDrv();
    rtcDevCreate(prtcfuncs);                                            /*  创建硬件 RTC 设备           */
    rtcToSys();                                                         /*  将 RTC 时间同步到系统时间   */
#endif                                                                  /*  参考代码结束                */
}

#endif                                                                  /*  LW_CFG_RTC_EN > 0           */
/*********************************************************************************************************
** 函数名称: halIdleInit
** 功能描述: 初始化目标系统空闲时间作业
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  halIdleInit (VOID)
{
    API_SystemHookAdd(armWaitForInterrupt, 
                      LW_OPTION_THREAD_IDLE_HOOK);                      /*  空闲时暂停 CPU              */
}
/*********************************************************************************************************
** 函数名称: halCacheInit
** 功能描述: 目标系统 CPU 高速缓冲初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0

static VOID  halCacheInit (VOID)
{
    API_CacheLibInit(CACHE_COPYBACK, CACHE_COPYBACK, ARM_MACHINE_920);  /*  初始化 CACHE 系统           */
    API_CacheEnable(INSTRUCTION_CACHE);
    API_CacheEnable(DATA_CACHE);                                        /*  使能 CACHE                  */
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
** 函数名称: halFpuInit
** 功能描述: 目标系统 FPU 浮点运算单元初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

static VOID  halFpuInit (VOID)
{
    API_KernelFpuInit(ARM_MACHINE_920, ARM_FPU_NONE);
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
** 函数名称: halPmInit
** 功能描述: 初始化目标系统电源管理系统
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0

static VOID  halPmInit (VOID)
{
    /*
     * TODO: 加入你的处理代码, 参考代码如下:
     */
#if 0                                                                   /*  参考代码开始                */
    PLW_PMA_FUNCS  pmafuncs = pmGetFuncs();

    pmAdapterCreate("inner_pm", 21, pmafuncs);
#endif                                                                  /*  参考代码结束                */
}

#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */
/*********************************************************************************************************
** 函数名称: halBusInit
** 功能描述: 初始化目标系统总线系统
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halBusInit (VOID)
{
    /*
     * TODO: 加入你的处理代码, 参考代码如下:
     */
#if 0                                                                   /*  参考代码开始                */
    PLW_I2C_FUNCS    pi2cfuns  = i2cBusFuns();

    API_I2cLibInit();                                                   /*  初始化 i2c 子系统           */

    API_I2cAdapterCreate("/bus/i2c/0", pi2cfuns, 10, 1);                /*  创建 i2c 总线适配器         */
#endif                                                                  /*  参考代码结束                */
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** 函数名称: halDrvInit
** 功能描述: 初始化目标系统静态驱动程序
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halDrvInit (VOID)
{
    /*
     *  standard device driver (rootfs and procfs need install first.)
     */
    rootFsDrv();                                                        /*  ROOT   device driver        */
    procFsDrv();                                                        /*  proc   device driver        */
    shmDrv();                                                           /*  shm    device driver        */
    randDrv();                                                          /*  random device driver        */
    ptyDrv();                                                           /*  pty    device driver        */
    ttyDrv();                                                           /*  tty    device driver        */
    memDrv();                                                           /*  mem    device driver        */
    pipeDrv();                                                          /*  pipe   device driver        */
    spipeDrv();                                                         /*  spipe  device driver        */
    tpsFsDrv();                                                         /*  TPS FS device driver        */
    fatFsDrv();                                                         /*  FAT FS device driver        */
    ramFsDrv();                                                         /*  RAM FS device driver        */
    romFsDrv();                                                         /*  ROM FS device driver        */
    nfsDrv();                                                           /*  nfs    device driver        */
    yaffsDrv();                                                         /*  yaffs  device driver        */
    canDrv();                                                           /*  CAN    device driver        */

    /*
     * TODO: 加入你的处理代码, 参考代码如下:
     */
#if 0                                                                   /*  参考代码开始                */
    INT              i;
    ULONG            ulMaxBytes;
    PLW_DMA_FUNCS    pdmafuncs;

    for (i = 0; i < 4; i++) {                                           /*  安装 2440 4 个通用 DMA 通道 */
        pdmafuncs = dmaGetFuncs(LW_DMA_CHANNEL0 + i, &ulMaxBytes);
        dmaDrv((UINT)i, pdmafuncs, (size_t)ulMaxBytes);                 /*  安装 DMA 控制器驱动         */
    }
#endif                                                                  /*  参考代码结束                */
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** 函数名称: halDevInit
** 功能描述: 初始化目标系统静态设备组件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halDevInit (VOID)
{
    /*
     *  创建根文件系统时, 将自动创建 dev, mnt, var 目录.
     */
    rootFsDevCreate();                                                  /*  创建根文件系统              */
    procFsDevCreate();                                                  /*  创建 proc 文件系统          */
    shmDevCreate();                                                     /*  创建共享内存设备            */
    randDevCreate();                                                    /*  创建随机数文件              */

    /*
     * TODO: 加入你的处理代码, 参考代码如下:
     */
#if 0                                                                   /*  参考代码开始                */
    SIO_CHAN    *psio0 = sioChanCreate(0);                              /*  创建串口 0 通道             */
    ttyDevCreate("/dev/ttyS0", psio0, 30, 50);                          /*  add    tty   device         */
#endif                                                                  /*  参考代码结束                */

    yaffsDevCreate("/yaffs2");                                          /*  create yaffs device(only fs)*/
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** 函数名称: halLogInit
** 功能描述: 初始化目标系统日志系统
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_LOG_LIB_EN > 0

static VOID  halLogInit (VOID)
{
    fd_set      fdLog;

    FD_ZERO(&fdLog);
    FD_SET(STD_OUT, &fdLog);
    API_LogFdSet(STD_OUT + 1, &fdLog);                                  /*  初始化日志                  */
}

#endif                                                                  /*  LW_CFG_LOG_LIB_EN > 0       */
/*********************************************************************************************************
** 函数名称: halStdFileInit
** 功能描述: 初始化目标系统标准文件系统
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halStdFileInit (VOID)
{
    int     iFd = open("/dev/ttyS0", O_RDWR, 0);

    if (iFd >= 0) {
        ioctl(iFd, FIOBAUDRATE,   SIO_BAUD_115200);
        ioctl(iFd, FIOSETOPTIONS, (OPT_TERMINAL & (~OPT_7_BIT)));       /*  system terminal 8 bit mode  */

        ioGlobalStdSet(STD_IN,  iFd);
        ioGlobalStdSet(STD_OUT, iFd);
        ioGlobalStdSet(STD_ERR, iFd);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** 函数名称: halShellInit
** 功能描述: 初始化目标系统 shell 环境, (getopt 使用前一定要初始化 shell 环境)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static VOID  halShellInit (VOID)
{
    API_TShellInit();

    /*
     *  初始化 appl 中间件 shell 接口
     */
    zlibShellInit();
    viShellInit();

    /*
     *  初始化 GDB 调试器
     */
    gdbInit();
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** 函数名称: halVmmInit
** 功能描述: 初始化目标系统虚拟内存管理组件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

static VOID  halVmmInit (VOID)
{
    API_VmmLibInit(_G_physicalDesc, _G_virtualDesc, ARM_MACHINE_920);
    API_VmmMmuEnable();
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
** 函数名称: halNetInit
** 功能描述: 网络组件初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

static VOID  halNetInit (VOID)
{
    API_NetInit();                                                      /*  初始化网络系统              */

    /*
     *  初始化网络附加工具
     */
#if LW_CFG_NET_PING_EN > 0
    API_INetPingInit();
    API_INetPing6Init();
#endif                                                                  /*  LW_CFG_NET_PING_EN > 0      */

#if LW_CFG_NET_NETBIOS_EN > 0
    API_INetNetBiosInit();
    API_INetNetBiosNameSet("sylixos");
#endif                                                                  /*  LW_CFG_NET_NETBIOS_EN > 0   */

#if LW_CFG_NET_TFTP_EN > 0
    API_INetTftpServerInit("/tmp");
#endif                                                                  /*  LW_CFG_NET_TFTP_EN > 0      */

#if LW_CFG_NET_FTPD_EN > 0
    API_INetFtpServerInit("/");
#endif                                                                  /*  LW_CFG_NET_FTP_EN > 0       */

#if LW_CFG_NET_TELNET_EN > 0
    API_INetTelnetInit(LW_NULL);
#endif                                                                  /*  LW_CFG_NET_TELNET_EN > 0    */

#if LW_CFG_NET_NAT_EN > 0
    API_INetNatInit();
#endif                                                                  /*  LW_CFG_NET_NAT_EN > 0       */

#if LW_CFG_NET_NPF_EN > 0
    API_INetNpfInit();
#endif                                                                  /*  LW_CFG_NET_NPF_EN > 0       */

#if LW_CFG_NET_VPN_EN > 0
    API_INetVpnInit();
#endif                                                                  /*  LW_CFG_NET_VPN_EN > 0       */
}
/*********************************************************************************************************
** 函数名称: halNetifAttch
** 功能描述: 网络接口连接
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  halNetifAttch (VOID)
{
    /*
     * TODO: 加入你的处理代码, 参考代码如下:
     */
#if 0                                                                   /*  参考代码开始                */
    static struct netif  ethernetif;
    ip_addr_t            ip, submask, gateway;

    IP4_ADDR(&ip,       192, 168,   7,  30);
    IP4_ADDR(&submask,  255, 255, 255,   0);
    IP4_ADDR(&gateway,  192, 168,   7,   1);

    netif_add(&ethernetif, &ip, &submask, &gateway,
              NULL, dm9000_netif_init,
              tcpip_input);

    ethernetif.ip6_autoconfig_enabled = 1;                              /*  允许 IPv6 地址自动配置      */

    netif_set_up(&ethernetif);

    netif_set_default(&ethernetif);

    netif_create_ip6_linklocal_address(&ethernetif, 1);                 /*  构造 ipv6 link address      */

    lib_srand((ethernetif.hwaddr[0] << 24) |
              (ethernetif.hwaddr[1] << 16) |
              (ethernetif.hwaddr[2] <<  8) |
              (ethernetif.hwaddr[3]));                                  /*  可以用 mac 设置随机数种子   */
#endif                                                                  /*  参考代码结束                */
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
** 函数名称: halMonitorInit
** 功能描述: 内核监控器上传初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_MONITOR_EN > 0

static VOID  halMonitorInit (VOID)
{
    /*
     *  可以再这里创建内核监控器上传通道, 也可以使用 shell 命令操作.
     */
}

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
/*********************************************************************************************************
** 函数名称: halPosixInit
** 功能描述: 初始化 posix 子系统 (如果系统支持 proc 文件系统, 则必须放在 proc 文件系统安装之后!)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

static VOID  halPosixInit (VOID)
{
    API_PosixInit();
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
** 函数名称: halSymbolInit
** 功能描述: 初始化目标系统符号表环境, (为 module loader 提供环境)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_SYMBOL_EN > 0

static VOID  halSymbolInit (VOID)
{
#ifdef __GNUC__
    void *__aeabi_read_tp();
#endif                                                                  /*  __GNUC__                    */

    API_SymbolInit();

#ifdef __GNUC__
    symbolAddAll();

    /*
     *  GCC will emit calls to this routine under -mtp=soft.
     */
    API_SymbolAdd("__aeabi_read_tp", (caddr_t)__aeabi_read_tp, LW_SYMBOL_FLAG_XEN);
#endif                                                                  /*  __GNUC__                    */
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
/*********************************************************************************************************
** 函数名称: halLoaderInit
** 功能描述: 初始化目标系统程序或模块装载器
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

static VOID  halLoaderInit (VOID)
{
    API_LoaderInit();
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
/*********************************************************************************************************
** 函数名称: halStdDirInit
** 功能描述: 设备标准文件系统创建
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  halStdDirInit (VOID)
{
    /*
     *  当 LW_CFG_PATH_VXWORKS == 0 时, 操作系统会在根目录下自动创建三个挂载点 /dev /mnt /var
     *  其余挂载点需要通过手动创建, 例如: /etc /bin /sbin /tmp 等等,
     *  一般来说 /tmp /bin /sbin /ftk /etc ... 可以做成链接文件, 链接到指定的文件系统.
     */
    mkdir("/usb", DEFAULT_DIR_PERM);
    if (access("/yaffs2/n0/boot", R_OK) < 0) {
        mkdir("/yaffs2/n0/boot", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n0/etc", R_OK) < 0) {
        mkdir("/yaffs2/n0/etc", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/ftk", R_OK) < 0) {
        mkdir("/yaffs2/n1/ftk", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/qt", R_OK) < 0) {
        mkdir("/yaffs2/n1/qt", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/lib", R_OK) < 0) {
        mkdir("/yaffs2/n1/lib", DEFAULT_DIR_PERM);
        mkdir("/yaffs2/n1/lib/modules", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/usr", R_OK) < 0) {
        mkdir("/yaffs2/n1/usr", DEFAULT_DIR_PERM);
        mkdir("/yaffs2/n1/usr/lib", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/bin", R_OK) < 0) {
        mkdir("/yaffs2/n1/bin", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/tmp", R_OK) < 0) {
        mkdir("/yaffs2/n1/tmp", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/sbin", R_OK) < 0) {
        mkdir("/yaffs2/n1/sbin", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/apps", R_OK) < 0) {
        mkdir("/yaffs2/n1/apps", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/home", R_OK) < 0) {
        mkdir("/yaffs2/n1/home", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/root", R_OK) < 0) {
        mkdir("/yaffs2/n1/root", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/var", R_OK) < 0) {
        mkdir("/yaffs2/n1/var", DEFAULT_DIR_PERM);
    }

    symlink("/yaffs2/n0/boot", "/boot");
    symlink("/yaffs2/n0/etc",  "/etc");                                 /*  创建根目录符号链接          */
    symlink("/yaffs2/n1/ftk",  "/ftk");                                 /*  创建 FTK 图形系统符号链接   */
    symlink("/yaffs2/n1/qt",   "/qt");                                  /*  创建 Qt 图形系统符号链接    */
    symlink("/yaffs2/n1/lib",  "/lib");
    symlink("/yaffs2/n1/usr",  "/usr");
    symlink("/yaffs2/n1/bin",  "/bin");
    symlink("/yaffs2/n1/sbin", "/sbin");
    symlink("/yaffs2/n1/apps", "/apps");
    symlink("/yaffs2/n1/home", "/home");
    symlink("/yaffs2/n1/root", "/root");
    symlink("/yaffs2/n1/var",  "/var");
    symlink("/yaffs2/n1/tmp",  "/var/tmp");
    symlink("/yaffs2/n1/tmp",  "/tmp");
}
/*********************************************************************************************************
** 函数名称: halBootThread
** 功能描述: 多任务状态下的初始化启动任务
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PVOID  halBootThread (PVOID  pvBootArg)
{
    LW_CLASS_THREADATTR     threakattr = API_ThreadAttrGetDefault();    /*  使用默认属性                */

    (VOID)pvBootArg;

#if LW_CFG_SHELL_EN > 0
    halShellInit();
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

#if LW_CFG_POWERM_EN > 0
    halPmInit();
#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */

#if LW_CFG_DEVICE_EN > 0
    halBusInit();
    halDrvInit();
    halDevInit();
    halStdFileInit();
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */

#if LW_CFG_LOG_LIB_EN > 0
    halLogInit();
    console_loglevel = default_message_loglevel;                        /*  设置 printk 打印信息等级    */
#endif                                                                  /*  LW_CFG_LOG_LIB_EN > 0       */

    /*
     *  以为 yaffs 挂载物理卷时, 需要 stdout 打印信息, 如果在 halDevInit() 中被调用, 由于没有创建
     *  标准文件, 所以会打印警告错误信息, 所以将此函数放在这里!
     *  如果未初始化标准文件会提示错误信息
     */
    /*
     * TODO: 加入你的处理代码, 参考代码如下:
     */
#if 0                                                                   /*  参考代码开始                */
#ifdef __GNUC__
    nand_init();
    mtdDevCreateEx("/n");                                               /*  mount mtddevice             */
#else
    nandDevCreateEx("/n");                                              /*  mount nandflash disk(yaffs) */
#endif
#endif                                                                  /*  参考代码结束                */

    halStdDirInit();                                                    /*  创建标准目录                */

    /*
     *  只有初始化了 shell 并获得了 TZ 环境变量标示的时区, 才可以调用 rtcToRoot()
     */
    system("varload");                                                  /*  从/etc/profile中读取环境变量*/
    lib_tzset();                                                        /*  通过 TZ 环境变量设置时区    */
    rtcToRoot();                                                        /*  将 RTC 时间同步到根文件系统 */

    /*
     *  网络初始化一般放在 shell 初始化之后, 因为初始化网络组件时, 会自动注册 shell 命令.
     */
#if LW_CFG_NET_EN > 0
    halNetInit();
    halNetifAttch();                                                    /*  wlan 网卡需要下载固件       */
#endif                                                                  /*  LW_CFG_NET_EN > 0           */

#if LW_CFG_POSIX_EN > 0
    halPosixInit();
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */

#if LW_CFG_SYMBOL_EN > 0
    halSymbolInit();
#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */

#if LW_CFG_MODULELOADER_EN > 0
    halLoaderInit();
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

#if LW_CFG_MONITOR_EN > 0
    halMonitorInit();
#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */

    system("shfile /yaffs2/n0/etc/startup.sh");                         /*  执行启动脚本                */
                                                                        /*  必须在初始化 shell 后调用!! */

    API_ThreadAttrSetStackSize(&threakattr, __LW_THREAD_MAIN_STK_SIZE); /*  设置 main 线程的堆栈大小    */
    API_ThreadCreate("t_main",
                     (PTHREAD_START_ROUTINE)t_main,
                     &threakattr,
                     LW_NULL);                                          /*  Create "t_main()" thread    */

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: usrStartup
** 功能描述: 初始化应用相关组件, 创建操作系统的第一个任务.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  usrStartup (VOID)
{
    LW_CLASS_THREADATTR     threakattr;

    /*
     *  注意, 不要修改该初始化顺序 (必须先初始化 vmm 才能正确的初始化 cache,
     *                              网络需要其他资源必须最后初始化)
     */
    halIdleInit();
#if LW_CFG_CPU_FPU_EN > 0
    halFpuInit();
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if LW_CFG_RTC_EN > 0
    halTimeInit();
#endif                                                                  /*  LW_CFG_RTC_EN > 0           */

#if LW_CFG_VMM_EN > 0
    halVmmInit();
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

#if LW_CFG_CACHE_EN > 0
    halCacheInit();
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    API_ThreadAttrBuild(&threakattr,
                        __LW_THREAD_BOOT_STK_SIZE,
                        LW_PRIO_CRITICAL,
                        LW_OPTION_THREAD_STK_CHK,
                        LW_NULL);
    API_ThreadCreate("t_boot",
                     (PTHREAD_START_ROUTINE)halBootThread,
                     &threakattr,
                     LW_NULL);                                          /*  Create boot thread          */
}
/*********************************************************************************************************
** 函数名称: main
** 功能描述: C 入口
** 输　入  : NONE
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT bspInit (VOID)
{
    /*
     *  系统内核堆与系统堆
     */
    extern UCHAR  __heap_start, __heap_end;

    halModeInit();                                                      /*  初始化硬件                  */

    /*
     *  这里的调试端口是脱离操作系统的, 所以他应该不依赖于操作系统而存在.
     *  当系统出现错误时, 这个端口显得尤为关键. (项目成熟后可以通过配置关掉)
     */
    /*
     * TODO: 加入你的处理代码, 参考代码如下:
     */
#if 0                                                                   /*  参考代码开始                */
    debugChannelInit(0);                                                /*  初始化调试接口              */
#endif                                                                  /*  参考代码结束                */

    /*
     *  这里使用 bsp 设置启动参数, 如果 bootloader 支持, 可使用 bootloader 设置.
     *  为了兼容以前的项目, 这里 kfpu=yes 允许内核中(包括中断)使用 FPU.
     *
     *  TODO: 可以修改内核启动参数
     */
    API_KernelStartParam("ncpus=1 kdlog=no kderror=yes kfpu=no heapchk=yes");
                                                                        /*  操作系统启动参数设置        */
    API_KernelStart(usrStartup,
                    (PVOID)&__heap_start,
                    (size_t)&__heap_end - (size_t)&__heap_start,
                    LW_NULL, 0);                                        /*  启动内核                    */

    return  (0);                                                        /*  不会执行到这里              */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
