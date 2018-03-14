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
** 文   件   名: KernelParam.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 08 月 08 日
**
** 描        述: 这是系统内核启动参数设置文件。
**
** BUG:
2014.09.09  ncpus 取值范围为 [1 ~ LW_CFG_MAX_PROCESSORS].
2017.08.13  加入对初始状态的初始化.
*********************************************************************************************************/
#define  __KERNEL_NCPUS_SET
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  内核参数
*********************************************************************************************************/
static CHAR         _K_cKernelStartParam[1024];
#if LW_CFG_DEVICE_EN > 0
extern LW_API INT   API_RootFsMapInit(CPCHAR  pcMap);
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
/*********************************************************************************************************
** 函数名称: API_KernelStartParam
** 功能描述: 系统内核启动参数
** 输　入  : pcParam       启动参数, 是以空格分开的一个字符串列表，通常具有如下形式:
                           ncpus=1          CPU 个数 (x86/64 平台可以不设置, 操作系统会自动探测)
                           dlog=no          DEBUG LOG 信息打印
                           derror=yes       DEBUG ERROR 信息打印
                           kfpu=no          内核态对浮点支持 (推荐为 no)
                           heapchk=yes      内存堆越界检查
                           hz=100           系统 tick 频率, 默认为 100 (推荐 100 ~ 10000 中间)
                           hhz=100          高速定时器频率, 默认与 hz 相同 (需 BSP 支持)
                           irate=5          应用定时器分辨率, 默认为 5 个 tick. (推荐 1 ~ 10 中间)
                           hpsec=1          热插拔循环检测间隔时间, 单位: 秒 (推荐 1 ~ 5 秒)
                           bugreboot=yes    内核探测到 bug 时是否自动重启.
                           rebootto=10      重启超时时间.
                           fsched=no        SMP 系统内核快速调度
                           smt=no           SMT 均衡调度
                           noitmr=no        不支持 ITIMER_REAL/ITIMER_VIRTUAL/ITIMER_PROF,
                                            建议运动控制等高实时性应用, 可置为 yes 提高 tick 速度
                           tmcvtsimple=no   通过 timespec 转换 tick 超时, 是否使用简单转换法.
                                            建议 Lite 类型处理器可采用 simple 转换法.
                                            
                           rfsmap=/boot:[*],/:[*],...   这是根文件系统映射关系选项, 用逗号隔开, 
                                                        /boot /etc /tmp /apps ... 为可选映射, 
                                                        / 为必须映射.
                           
                                            例如 /boot:/media/hdd0 表示将 /boot 目录映射到 /media/hdd0
                                                 /apps:/media/hdd2 表示将 /apps 目录映射到 /media/hdd2
                                                 /:/media/hdd1 表示将根目录整体映射到 /media/hdd1
                                                 /:/dev/ram    表示将根目录整体映射到 ramfs 中.
                                                 
                                                 注意: /dev/ram 类型只能使用在 /: 映射中
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                       API 函数
*********************************************************************************************************/
LW_API
ULONG  API_KernelStartParam (CPCHAR  pcParam)
{
    CHAR        cParamBuffer[1024];                                     /*  参数长度不得超过 1024 字节  */
    PCHAR       pcDelim = " ";
    PCHAR       pcLast;
    PCHAR       pcTok;
    
    if (LW_SYS_STATUS_IS_RUNNING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel is already start.\r\n");
        _ErrorHandle(ERROR_KERNEL_RUNNING);
        return  (ERROR_KERNEL_RUNNING);
    }
    
    LW_KERN_BUG_REBOOT_EN_SET(LW_TRUE);                                 /*  初始状态                    */

    lib_strlcpy(cParamBuffer,         pcParam, sizeof(cParamBuffer));
    lib_strlcpy(_K_cKernelStartParam, pcParam, sizeof(_K_cKernelStartParam));

    pcTok = lib_strtok_r(cParamBuffer, pcDelim, &pcLast);
    while (pcTok) {
        if (lib_strncmp(pcTok, "ncpus=", 6) == 0) {                     /*  CPU 数量                    */
            INT     iCpus = lib_atoi(&pcTok[6]);
            if (iCpus > 0) {
                if (iCpus > LW_CFG_MAX_PROCESSORS) {
                    _K_ulNCpus = LW_CFG_MAX_PROCESSORS;
                } else {
                    _K_ulNCpus = (ULONG)iCpus;
                }
            }
            
#if LW_CFG_LOGMESSAGE_EN > 0
        } else if (lib_strncmp(pcTok, "kdlog=", 6) == 0) {              /*  是否使能内核 log 打印       */
            if (pcTok[6] == 'n') {
                _K_pfuncKernelDebugLog = LW_NULL;
            } else {
                _K_pfuncKernelDebugLog = bspDebugMsg;
            }
#endif                                                                  /*  LW_CFG_LOGMESSAGE_EN > 0    */

#if LW_CFG_ERRORMESSAGE_EN > 0
        } else if (lib_strncmp(pcTok, "kderror=", 8) == 0) {            /*  是否使能内核错误打印        */
            if (pcTok[8] == 'n') {
                _K_pfuncKernelDebugError = LW_NULL;
            } else {
                _K_pfuncKernelDebugError = bspDebugMsg;
            }
#endif                                                                  /*  LW_CFG_ERRORMESSAGE_EN > 0  */
        
        } else if (lib_strncmp(pcTok, "kfpu=", 5) == 0) {               /*  是否使能内核浮点支持        */
            if (pcTok[5] == 'n') {
                LW_KERN_FPU_EN_SET(LW_FALSE);
            } else {
#if LW_CFG_INTER_FPU == 0
                _BugHandle(LW_TRUE, LW_TRUE, 
                           "Please configure LW_CFG_INTER_FPU with 1 in kernel_cfg.h\r\n");
#else
#ifdef LW_CFG_CPU_ARCH_MIPS                                             /*  MIPS 平台不允许 kfpu 操作   */
                _BugHandle(LW_TRUE, LW_TRUE, 
                           "SylixOS do not support kfpu on MIPS!\r\n");
#else
                LW_KERN_FPU_EN_SET(LW_TRUE);
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS        */
#endif                                                                  /*  LW_CFG_INTER_FPU > 0        */
            }

        } else if (lib_strncmp(pcTok, "bugreboot=", 10) == 0) {         /*  探测到 bug 时是否自动重启   */
            if (pcTok[10] == 'n') {
                LW_KERN_BUG_REBOOT_EN_SET(LW_FALSE);
            } else {
                LW_KERN_BUG_REBOOT_EN_SET(LW_TRUE);
            }
            
        } else if (lib_strncmp(pcTok, "rebootto=", 9) == 0) {           /*  重启超时                    */
            INT     iToSec = lib_atoi(&pcTok[9]);
            if ((iToSec >= 0) && (iToSec < 1000)) {
                LW_REBOOT_TO_SEC = (ULONG)iToSec;
            }
            
        } else if (lib_strncmp(pcTok, "heapchk=", 8) == 0) {            /*  是否进行堆内存越界检查      */
            if (pcTok[8] == 'n') {
                _K_bHeapCrossBorderEn = LW_FALSE;
            } else {
                _K_bHeapCrossBorderEn = LW_TRUE;
            }
        
        } else if (lib_strncmp(pcTok, "hz=", 3) == 0) {                 /*  tick 频率                   */
            ULONG   ulHz = (ULONG)lib_atol(&pcTok[3]);
            if (ulHz >= 100 && ulHz <= 10000) {                         /*  10ms ~ 100us                */
                LW_TICK_HZ = ulHz;
                LW_NSEC_PER_TICK = __TIMEVAL_NSEC_MAX / ulHz;
            }
        
        } else if (lib_strncmp(pcTok, "hhz=", 4) == 0) {                /*  高度定时器频率              */
            ULONG   ulHz = (ULONG)lib_atol(&pcTok[4]);
            if (ulHz >= 100 && ulHz <= 100000) {                        /*  10ms ~ 10us                 */
                LW_HTIMER_HZ = ulHz;
            }
        
        } else if (lib_strncmp(pcTok, "irate=", 6) == 0) {              /*  应用定时器分辨率            */
            ULONG   ulRate = (ULONG)lib_atol(&pcTok[6]);
            if (ulRate >= 1 && ulRate <= 10) {                          /*  1 ~ 10 ticks                */
                LW_ITIMER_RATE = ulRate;
            }
        
        } else if (lib_strncmp(pcTok, "hpsec=", 6) == 0) {              /*  热插拔循环检测周期          */
            ULONG   ulSec = (ULONG)lib_atol(&pcTok[6]);
            if (ulSec >= 1 && ulSec <= 10) {                            /*  1 ~ 10 ticks                */
                LW_HOTPLUG_SEC = ulSec;
            }
        }
        
#if LW_CFG_SMP_EN > 0
          else if (lib_strncmp(pcTok, "fsched=", 7) == 0) {             /*  SMP 快速调度                */
            if (pcTok[7] == 'n') {
                LW_KERN_SMP_FSCHED_EN_SET(LW_FALSE);
            } else {
                LW_KERN_SMP_FSCHED_EN_SET(LW_TRUE);
            }
        
        } 
#if LW_CFG_CPU_ARCH_SMT > 0
          else if (lib_strncmp(pcTok, "smt=", 4) == 0) {                /*  smt 均衡调度                */
            if (pcTok[4] == 'n') {
                LW_KERN_SMT_BSCHED_EN_SET(LW_FALSE);
            } else {
                LW_KERN_SMT_BSCHED_EN_SET(LW_TRUE);
            }
        }
#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
          else if (lib_strncmp(pcTok, "noitmr=", 7) == 0) {             /*  不支持 itimer               */
            if (pcTok[7] == 'n') {
                LW_KERN_NO_ITIMER_EN_SET(LW_FALSE);
            } else {
                LW_KERN_NO_ITIMER_EN_SET(LW_TRUE);
            }

        } else if (lib_strncmp(pcTok, "tmcvtsimple=", 12) == 0) {       /*  是否使用简单方法转换        */
            if (pcTok[12] == 'n') {
                LW_KERN_TMCVT_SIMPLE_EN_SET(LW_FALSE);
            } else {
                LW_KERN_TMCVT_SIMPLE_EN_SET(LW_TRUE);
            }
        } 
        
#if LW_CFG_DEVICE_EN > 0
          else if (lib_strncmp(pcTok, "rfsmap=", 7) == 0) {             /*  根文件系统映射              */
            API_RootFsMapInit(&pcTok[7]);
        }
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */

#ifdef __ARCH_KERNEL_PARAM
          else {
            __ARCH_KERNEL_PARAM(pcTok);                                 /*  体系结构相关参数            */
        }
#endif                                                                  /*  __ARCH_KERNEL_PARAM         */
        
        pcTok = lib_strtok_r(LW_NULL, pcDelim, &pcLast);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_KernelStartParamGet
** 功能描述: 获得系统内核启动参数
** 输　入  : pcParam       启动参数
**           stLen         缓冲区长度
** 输　出  : 实际长度
** 全局变量: 
** 调用模块: 
                                       API 函数
*********************************************************************************************************/
LW_API
ssize_t  API_KernelStartParamGet (PCHAR  pcParam, size_t  stLen)
{
    if (!pcParam || !stLen) {
        _ErrorHandle(ERROR_KERNEL_BUFFER_NULL);
        return  (0);
    }

    return  ((ssize_t)lib_strlcpy(pcParam, _K_cKernelStartParam, stLen));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
