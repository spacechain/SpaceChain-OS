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
** 文   件   名: ioFormat.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 09 月 19 日
**
** 描        述: 磁盘格式化命令.

** BUG
2008.12.02  今天是母亲的生日, 祝母亲生日快乐! 将相关操作加入 FIOFLUSH 操作.
2011.08.15  重大决定: 将所有 posix 定义的函数以函数方式(非宏)引出.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
** 函数名称: diskformat
** 功能描述: 格式化指定的设备.
** 输　入  : pcDevName     设备名, NULL 表示格式化当前设备
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  diskformat (CPCHAR  pcDevName)
{
    REGISTER INT         iError;
    REGISTER INT         iFd;
    REGISTER CPCHAR      pcName;
    
    pcName = (pcDevName == LW_NULL) ? "." : pcDevName;
    
    if (geteuid() != 0) {                                               /*  必须具有 root 权限          */
        errno = EACCES;
        return  (PX_ERROR);
    }
    
    iFd = open(pcName, O_WRONLY);                                       /*  打开设备                    */
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = ioctl(iFd, FIODISKFORMAT, LW_OSIOD_LARG(0));               /*  格式化文件                  */
    if (iError < 0) {
        close(iFd);
        return  (PX_ERROR);
    }
    
    iError = ioctl(iFd, FIOSYNC);                                       /*  清空缓存                    */
    if (iError < 0) {
        close(iFd);
        return  (PX_ERROR);
    }
    
    iError = ioctl(iFd, FIODISKINIT, LW_OSIOD_LARG(0));                 /*  重新初始化设备              */
    if (iError < 0) {
        close(iFd);
        return  (PX_ERROR);
    }
    
    close(iFd);                                                         /*  关闭设备                    */
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "disk \"%s\" format ok.\r\n", pcDevName);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: diskinit
** 功能描述: 初始化指定的磁盘设备.
** 输　入  : pcDevName     设备名, NULL 表示初始化当前磁盘
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  diskinit (CPCHAR  pcDevName)
{
    REGISTER INT         iError;
    REGISTER INT         iFd;
             CHAR        cName[MAX_FILENAME_LENGTH];
    
    if (pcDevName == LW_NULL) {
        ioDefPathGet(cName);
    } else {
        lib_strlcpy(cName, pcDevName, MAX_FILENAME_LENGTH);
    }
    
    iFd = open(cName, O_WRONLY, 0);                                     /*  打开设备                    */
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = ioctl(iFd, FIOFLUSH);                                      /*  清空缓存                    */
    if (iError < 0) {
        close(iFd);
        return  (PX_ERROR);
    }
    
    iError = ioctl(iFd, FIODISKINIT, LW_OSIOD_LARG(0));                 /*  重新初始化设备              */
    if (iError < 0) {
        close(iFd);
        return  (PX_ERROR);
    }
    
    close(iFd);                                                         /*  关闭设备                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
