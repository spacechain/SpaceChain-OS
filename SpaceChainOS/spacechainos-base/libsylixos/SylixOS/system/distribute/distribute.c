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
** 文   件   名: distribute.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 06 月 10 日
**
** 描        述: SylixOS 发行商信息.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  默认 Logo 信息
*********************************************************************************************************/
#if LW_CFG_KERNEL_LOGO > 0
extern  const   CHAR  _K_cKernelLogo[];
#endif                                                                  /*  LW_CFG_KERNEL_LOGO > 0      */
/*********************************************************************************************************
** 函数名称: API_SystemLogoPrint
** 功能描述: 打印系统 LOGO
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_SystemLogoPrint (VOID)
{
    INT     iLogoFd;
    CHAR    cBuf[128];
    ssize_t sstNum;
    
    iLogoFd = open("/etc/oemlogo", O_RDONLY);
    if (iLogoFd < 0) {
#if LW_CFG_KERNEL_LOGO > 0
        write(STD_OUT, (CPVOID)_K_cKernelLogo, lib_strlen((PCHAR)_K_cKernelLogo));
#else
        write(STD_OUT, "\nKERNEL: Long Wing(C) 2006 - 2016\n",  \
              lib_strlen("\nKERNEL: Long Wing(C) 2006 - 2016\n"));
#endif                                                                  /*  LW_CFG_KERNEL_LOGO > 0      */
        return  (ERROR_NONE);
    }
    
    write(STD_OUT, "\n", 1);
    
    do {
        sstNum = read(iLogoFd, cBuf, sizeof(cBuf));
        if (sstNum > 0) {
            write(STD_OUT, cBuf, (size_t)sstNum);
        }
    } while (sstNum > 0);
    
    write(STD_OUT, "\n", 1);
    
    close(iLogoFd);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SystemHwInfoPrint
** 功能描述: 打印系统硬件信息
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_SystemHwInfoPrint (VOID)
{
    printf("%s\n", __SYLIXOS_LICENSE);
    printf("%s\n", __SYLIXOS_VERINFO);
    printf("\n");
    printf("CPU     : %s\n", bspInfoCpu());
    printf("CACHE   : %s\n", bspInfoCache());
    printf("PACKET  : %s\n", bspInfoPacket());
    printf("ROM SIZE: 0x%08zx Bytes (0x%08lx - 0x%08lx)\n",
           bspInfoRomSize(),
           bspInfoRomBase(),
           (bspInfoRomBase() + bspInfoRomSize() - 1));
    printf("RAM SIZE: 0x%08zx Bytes (0x%08lx - 0x%08lx)\n",
           bspInfoRamSize(),
           bspInfoRamBase(),
           (bspInfoRamBase() + bspInfoRamSize() - 1));
    printf("BSP     : %s\n", bspInfoVersion());
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
