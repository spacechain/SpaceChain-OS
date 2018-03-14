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
** 文   件   名: if_dl.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 01 月 15 日
**
** 描        述: posix net/if_dl.h
*********************************************************************************************************/

#ifndef __IF_DL_H
#define __IF_DL_H

#include "sys/types.h"

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

struct sockaddr_dl {
    u_char      sdl_len;                                                /* Total length of sockaddr     */
    u_char      sdl_family;                                             /* AF_LINK                      */
    u_short     sdl_index;                                              /* if != 0, system given index  */
    u_char      sdl_type;                                               /* interface type               */
    u_char      sdl_nlen;                                               /* interface name length,       */
                                                                        /* no trailing 0 reqd.          */
    u_char      sdl_alen;                                               /* link level address length    */
    u_char      sdl_slen;                                               /* link layer selector length   */
    char        sdl_data[12];                                           /* minimum work area,           */
                                                                        /* can be larger; contains both */
                                                                        /* if name and ll address       */
    u_short     sdl_rcf;                                                /* source routing control       */
    u_short     sdl_route[16];                                          /* source routing information   */
};

#define LLADDR(s)  ((caddr_t)((s)->sdl_data + (s)->sdl_nlen))
#define CLLADDR(s) ((const char *)((s)->sdl_data + (s)->sdl_nlen))

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_NET_EN               */
#endif                                                                  /*  __IF_DL_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
