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
** 文   件   名: ifaddrs.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 08 月 11 日
**
** 描        述: 网络接口地址.
*********************************************************************************************************/

#ifndef __IFADDRS_H
#define __IFADDRS_H

#include <socket.h>

struct ifaddrs {
    struct ifaddrs  *ifa_next;
    char            *ifa_name;
    u_int            ifa_flags;
    struct sockaddr *ifa_addr;
    struct sockaddr *ifa_netmask;
    struct sockaddr *ifa_dstaddr;
    void            *ifa_data;
};

/*********************************************************************************************************
  This may have been defined in <net/if.h>.  Note that if <net/if.h> is
  to be included it must be included before this header file.
*********************************************************************************************************/

#ifndef ifa_broadaddr
#define ifa_broadaddr   ifa_dstaddr                                     /* broadcast address interface  */
#endif

#ifdef __cplusplus
extern "C" {
#endif

int  getifaddrs(struct ifaddrs **);
void freeifaddrs(struct ifaddrs *);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __IFADDRS_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
