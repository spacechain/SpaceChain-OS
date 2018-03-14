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
** 文   件   名: if_vnd.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 01 月 15 日
**
** 描        述: virtual netdev.
*********************************************************************************************************/

#ifndef __IF_VND_H
#define __IF_VND_H

#include "sys/types.h"

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_VNETDEV_EN > 0

#include "if.h"

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

#define IF_VND_PATH     "/dev/net/vnd"

#define IF_VND_TYPE_RAW         0
#define IF_VND_TYPE_ETHERNET    1

struct ifvnd {
    int     ifvnd_id;                                                   /*  Identification              */
    int     ifvnd_type;                                                 /*  0: raw net 1: ethernet      */
    size_t  ifvnd_bsize;                                                /*  buffer size                 */
    char    ifvnd_ifname[IF_NAMESIZE];                                  /*  ioctl return                */
    UINT8   ifvnd_hwaddr[IFHWADDRLEN];                                  /*  hard address                */
};

/*********************************************************************************************************
  ioctl of IF_VND_PATH
  
  int  fd;
  ssize_t num;
  unsigned char buf[1514];
  struct ifvnd vnd_desc;
  
  fd = open(IF_VND_PATH, O_RDWR);
  
  bzero(&vnd_desc, sizeof(struct ifvnd));
  vnd_desc.ifvnd_id = 100;
  vnd_desc.ifvnd_type = IF_VND_TYPE_ETHERNET;
  vnd_desc.ifvnd_bsize = 128K;
  
  if (no hwaddr config in /etc/ifparam.ini) {
      memcpy(vnd_desc.ifvnd_hwaddr, ..., IFHWADDRLEN);
  }
  
  ioctl(fd, SIOCVNDADD, &vnd_desc);    create a virtual net device
  ioctl(fd, SIOCVNDSEL, &vnd_desc);    select this device
  
  for (;;) {
      num = read(fd, buf, 1514);
      if (num > 0) {
          write(fd, buf, num);         net loop
      }
  }
  
  close(fd);
*********************************************************************************************************/

#define SIOCVNDADD      _IOWR('v', 1, struct ifvnd)                     /*  add a vnd device            */
#define SIOCVNDDEL      _IOWR('v', 2, struct ifvnd)                     /*  delete a vnd device         */
#define SIOCVNDSEL      _IOWR('v', 3, struct ifvnd)                     /*  select specify id vnd device*/

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_NET_VNETDEV_EN       */
#endif                                                                  /*  __IF_VND_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
