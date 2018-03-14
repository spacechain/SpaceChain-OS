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
** 文   件   名: times.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 10 月 23 日
**
** 描        述: posix times 兼容库.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_times.h"                                        /*  已包含操作系统头文件        */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** 函数名称: times
** 功能描述: shall fill the tms structure pointed to by buffer with time-accounting information. 
             The tms structure is defined in <sys/times.h>.
** 输　入  : tms            struct tms 参数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
clock_t times (struct tms *ptms)
{
#if LW_CFG_MODULELOADER_EN > 0
    PVOID   pvVProc = (PVOID)__LW_VP_GET_CUR_PROC();

    if (!ptms) {
        errno = EINVAL;
        return  (lib_clock());
    }

    if (pvVProc) {
        API_ModuleTimes(pvVProc, 
                        &ptms->tms_utime, 
                        &ptms->tms_stime,
                        &ptms->tms_cutime,
                        &ptms->tms_cstime);
        return  (lib_clock());
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    lib_bzero(ptms, sizeof(struct tms));
    
    return  (lib_clock());
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
