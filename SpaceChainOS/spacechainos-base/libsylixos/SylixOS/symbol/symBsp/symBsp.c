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
** 文   件   名: symBsp.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 08 月 12 日
**
** 描        述: 这里加入 BSP 中应该包含的一些重要的符号.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_SYMBOL_EN > 0
/*********************************************************************************************************
  bsp 函数. 
*********************************************************************************************************/
#define __LW_SYMBOL_ITEM__STR(s)            #s
#define __LW_SYMBOL_ITEM_STR(s)             __LW_SYMBOL_ITEM__STR(s)
#define __LW_SYMBOL_ITEM_FUNC(pcName)                                       \
        {   {LW_NULL, LW_NULL},                                             \
            __LW_SYMBOL_ITEM_STR(pcName), (caddr_t)pcName,                  \
    	    LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN \
        },
static LW_SYMBOL    _G_symBsp[] = {
/*********************************************************************************************************
  操作系统无关信息打印
*********************************************************************************************************/
#if  LW_CFG_ERRORMESSAGE_EN > 0 || LW_CFG_LOGMESSAGE_EN > 0
    {   {LW_NULL, LW_NULL}, "bspDebugMsg", (caddr_t)bspDebugMsg,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
#endif
/*********************************************************************************************************
  驱动函数使用短延时
*********************************************************************************************************/
    {   {LW_NULL, LW_NULL}, "udelay", (caddr_t)bspDelayUs,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "ndelay", (caddr_t)bspDelayNs,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "bspDelayUs", (caddr_t)bspDelayUs,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "bspDelayNs", (caddr_t)bspDelayNs,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
/*********************************************************************************************************
  驱动函数 CPU 识别
*********************************************************************************************************/
    {   {LW_NULL, LW_NULL}, "bspInfoCpu", (caddr_t)bspInfoCpu,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "bspInfoCache", (caddr_t)bspInfoCache,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "bspInfoPacket", (caddr_t)bspInfoPacket,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "bspInfoVersion", (caddr_t)bspInfoVersion,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "bspInfoHwcap", (caddr_t)bspInfoHwcap,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "bspInfoRomBase", (caddr_t)bspInfoRomBase,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "bspInfoRomSize", (caddr_t)bspInfoRomSize,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "bspInfoRamBase", (caddr_t)bspInfoRamBase,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "bspInfoRamSize", (caddr_t)bspInfoRamSize,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
/*********************************************************************************************************
  高精度时间获取
*********************************************************************************************************/
#if LW_CFG_TIME_HIGH_RESOLUTION_EN > 0
    {   {LW_NULL, LW_NULL}, "bspTickHighResolution", (caddr_t)bspTickHighResolution,
         LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    }, 
#endif
/*********************************************************************************************************
  TI C6X DSP 符号
*********************************************************************************************************/
#if defined(LW_CFG_CPU_ARCH_C6X)
    {   {LW_NULL, LW_NULL}, "_K_pheapSystem", (caddr_t)&_K_pheapSystem,
         LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN
    },
    {   {LW_NULL, LW_NULL}, "_K_cpuTable", (caddr_t)&_K_cpuTable,
         LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN
    },
    {
        {LW_NULL, LW_NULL}, "__c6xabi_DSBT_BASE", (caddr_t)__TI_STATIC_BASE,
         LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN
    },
#endif
};
/*********************************************************************************************************
** 函数名称: __symbolAddBsp
** 功能描述: 向符号表中添加 bsp 符号 
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __symbolAddBsp (VOID)
{
    return  (API_SymbolAddStatic(_G_symBsp, (sizeof(_G_symBsp) / sizeof(LW_SYMBOL))));
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
