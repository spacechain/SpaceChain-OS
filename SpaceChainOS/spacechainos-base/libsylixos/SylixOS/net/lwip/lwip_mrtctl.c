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
** 文   件   名: lwip_mrtctl.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2018 年 01 月 04 日
**
** 描        述: ioctl 组播路由表支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_MROUTER > 0
#include "sys/socket.h"
#include "lwip/tcpip.h"
#include "mroute/ip4_mrt.h"
#include "mroute/ip6_mrt.h"
/*********************************************************************************************************
** 函数名称: __mrtGetVifCnt4
** 功能描述: 获得 IPv4 vif cnt 信息
** 输　入  : pvArg     信息结构
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __mrtGetVifCnt4 (PVOID  pvArg)
{
    INT  iErrNo;
    
    MRT_LOCK();
    iErrNo = ip4_mrt_ioctl(SIOCGETVIFCNT, pvArg);
    if (iErrNo) {
        MRT_UNLOCK();
        _ErrorHandle(iErrNo);
        return  (PX_ERROR);
    }
    MRT_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __mrtGetVifCnt6
** 功能描述: 获得 IPv6 vif cnt 信息
** 输　入  : pvArg     信息结构
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_IPV6

static INT  __mrtGetVifCnt6 (PVOID  pvArg)
{
    INT  iErrNo;
    
    MRT6_LOCK();
    iErrNo = ip6_mrt_ioctl(SIOCGETVIFCNT, pvArg);
    if (iErrNo) {
        MRT6_UNLOCK();
        _ErrorHandle(iErrNo);
        return  (PX_ERROR);
    }
    MRT6_UNLOCK();
    
    return  (ERROR_NONE);
}

#endif
/*********************************************************************************************************
** 函数名称: __mrtGetSgCnt4
** 功能描述: 获得 IPv4 sg cnt 信息
** 输　入  : pvArg     信息结构
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __mrtGetSgCnt4 (PVOID  pvArg)
{
    INT  iErrNo;
    
    MRT_LOCK();
    iErrNo = ip4_mrt_ioctl(SIOCGETSGCNT, pvArg);
    if (iErrNo) {
        MRT_UNLOCK();
        _ErrorHandle(iErrNo);
        return  (PX_ERROR);
    }
    MRT_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __mrtGetSgCnt6
** 功能描述: 获得 IPv6 sg cnt 信息
** 输　入  : pvArg     信息结构
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_IPV6

static INT  __mrtGetSgCnt6 (PVOID  pvArg)
{
    INT  iErrNo;
    
    MRT6_LOCK();
    iErrNo = ip6_mrt_ioctl(SIOCGETSGCNT, pvArg);
    if (iErrNo) {
        MRT6_UNLOCK();
        _ErrorHandle(iErrNo);
        return  (PX_ERROR);
    }
    MRT6_UNLOCK();
    
    return  (ERROR_NONE);
}

#endif
/*********************************************************************************************************
** 函数名称: __mrtIoctlInet
** 功能描述: SIOCGETVIFCNT / SIOCGETSGCNT ... 命令处理接口
** 输　入  : iFamily    AF_INET / AF_INET6
**           iCmd       SIOCADDRT / SIOCDELRT
**           pvArg      struct arpreq
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __mrtIoctlInet (INT  iFamily, INT  iCmd, PVOID  pvArg)
{
    INT  iRet = PX_ERROR;

    if (!pvArg) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    switch (iCmd) {
    
    case SIOCGETVIFCNT:
        if (iFamily == AF_INET) {
            iRet = __mrtGetVifCnt4(pvArg);
        } 
#if LWIP_IPV6
          else {
            iRet = __mrtGetVifCnt6(pvArg);
        }
#endif
        break;
        
    case SIOCGETSGCNT:
        if (iFamily == AF_INET) {
            iRet = __mrtGetSgCnt4(pvArg);
        } 
#if LWIP_IPV6
          else {
            iRet = __mrtGetSgCnt6(pvArg);
        }
#endif
        break;
        
    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_MROUTER > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
