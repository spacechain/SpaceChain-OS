/*********************************************************************************************************
**
**                                    �й�������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: lib_hrtime.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 07 �� 04 ��
**
** ��        ��: ϵͳ�߾���ʱ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#if LW_CFG_POSIX_EN > 0
#include "sys/times.h"
#endif                                                                      /*  LW_CFG_POSIX_EN > 0     */
/*********************************************************************************************************
** ��������: lib_gethrtime
** ��������: ���ϵͳ��ȷʱ��
** �䡡��  : NONE
** �䡡��  : �� ns Ϊ��λ�ľ�ȷʱ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

hrtime_t  lib_gethrtime (VOID)
{
    struct timespec  tm;
    
    lib_clock_gettime(CLOCK_MONOTONIC, &tm);
    
    return  ((((hrtime_t)tm.tv_sec) * __TIMEVAL_NSEC_MAX) + tm.tv_nsec);
}
/*********************************************************************************************************
** ��������: lib_gethrvtime
** ��������: ����û����̾�ȷʱ��
** �䡡��  : NONE
** �䡡��  : �� ns Ϊ��λ�ľ�ȷʱ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
hrtime_t  lib_gethrvtime (VOID)
{
#if LW_CFG_POSIX_EN > 0
    struct tms tmsBuf;
    ULONG      ulTicks;
    
    times(&tmsBuf);
    
    ulTicks = (ULONG)tmsBuf.tms_utime + tmsBuf.tms_stime;
    
    return  ((hrtime_t)ulTicks * (__TIMEVAL_NSEC_MAX / LW_TICK_HZ));
#else
    return  (0);
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
}

#endif                                                                  /*  LW_CFG_RTC_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/