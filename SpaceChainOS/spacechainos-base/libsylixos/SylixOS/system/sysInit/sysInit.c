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
** 文   件   名: sysInit.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 04 月 02 日
**
** 描        述: 系统初始化函数

** BUG
2007.04.08  iErr 初始化为零
2007.11.07  加入 select 系统初始化.
2008.03.06  加入功耗管理器初始化
2008.06.01  将 hook 放在线程建立之前.
2008.06.12  加入日志系统.
2009.10.27  加入总线系统初始化.
2013.11.18  加入 epoll 初始化.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
** 函数名称: _SysInit
** 功能描述: 系统初始化函数
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _SysInit (VOID)
{
    REGISTER INT    iErr = 0;
    
    iErr |= _excInit();                                                 /*  初始化异常处理              */
    _ErrorHandle((ULONG)iErr);

#if LW_CFG_LOG_LIB_EN > 0
    iErr |= _logInit();                                                 /*  初始化日志系统              */
    _ErrorHandle((ULONG)iErr);
#endif
    
#if LW_CFG_DEVICE_EN > 0
    iErr |= _IosInit();                                                 /*  IO 管理初始化               */
    _ErrorHandle((ULONG)iErr);
    __busSystemInit();                                                  /*  初始化总线系统              */
#endif

#if LW_CFG_MAX_VOLUMES > 0
    __blockIoDevInit();                                                 /*  块设备卷池初始化            */
#endif

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)
    iErr |= _SelectInit();                                              /*  初始化 select 系统          */
    _ErrorHandle((ULONG)iErr);
#if LW_CFG_EPOLL_EN > 0
    iErr |= _EpollInit();
    _ErrorHandle((ULONG)iErr);
#endif
#endif

#if LW_CFG_THREAD_POOL_EN > 0 && LW_CFG_MAX_THREAD_POOLS > 0
    _ThreadPoolInit();                                                  /*  线程池管理初始化            */
#endif

#if LW_CFG_POWERM_EN > 0
    _PowerMInit();                                                      /*  功耗管理器初始化            */
#endif

#if LW_CFG_HOTPLUG_EN > 0
    _hotplugInit();                                                     /*  初始化热插拔支持            */
#endif

    return  (iErr);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
