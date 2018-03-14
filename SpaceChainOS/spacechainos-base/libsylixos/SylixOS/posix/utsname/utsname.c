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
** 文   件   名: utsname.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 03 月 10 日
**
** 描        述: posix utsname 兼容库.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_utsname.h"                                      /*  已包含操作系统头文件        */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
  host name
*********************************************************************************************************/
static CHAR     _G_cHostName[HOST_NAME_MAX + 1] = "sylixos";
/*********************************************************************************************************
** 函数名称: uname
** 功能描述: get the name of the current system
** 输　入  : name          system name arry
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  uname (struct utsname *pname)
{
    if (!pname) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    lib_strlcpy(pname->sysname, "sylixos", __PX_UTSNAME_SYSNAME_SIZE);
    lib_strlcpy(pname->nodename, _G_cHostName, __PX_UTSNAME_NODENAME_SIZE);
    lib_strlcpy(pname->release, __SYLIXOS_RELSTR, __PX_UTSNAME_RELEASE_SIZE);
    lib_strlcpy(pname->version, __SYLIXOS_VERSTR, __PX_UTSNAME_VERSION_SIZE);
    lib_strlcpy(pname->machine, bspInfoCpu(), __PX_UTSNAME_MACHINE_SIZE);
    
    return  (0);
}
/*********************************************************************************************************
** 函数名称: gethostname
** 功能描述: return the standard host name for the current machine.
** 输　入  : Host names are limited to {HOST_NAME_MAX} bytes.
             The namelen argument shall specify the size of the array pointed to by the name argument
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int	 gethostname (char *name, size_t namelen)
{
    if (!name || !namelen) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    lib_strlcpy(name, _G_cHostName, namelen);
    
    return  (0);
}
/*********************************************************************************************************
** 函数名称: sethostname
** 功能描述: set the standard host name for the current machine.
** 输　入  : Host names are limited to {HOST_NAME_MAX} bytes.
             The namelen argument shall specify the size of the array pointed to by the name argument
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int	 sethostname (const char *name, size_t namelen)
{
    if (!name || !namelen) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (namelen > HOST_NAME_MAX) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    lib_strlcpy(_G_cHostName, name, HOST_NAME_MAX + 1);
    
    return  (0);
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
