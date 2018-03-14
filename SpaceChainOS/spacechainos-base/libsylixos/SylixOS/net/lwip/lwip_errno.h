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
** 文   件   名: lwip_errno.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 05 月 06 日
**
** 描        述: lwip 需要的附加错误编号.
*********************************************************************************************************/

#ifndef __LWIP_ERRNO_H
#define __LWIP_ERRNO_H

#define ENSROK                  0               /* DNS server returned answer with no data              */
#define ENSRNODATA              160             /* DNS server returned answer with no data              */
#define ENSRFORMERR             161             /* DNS server claims query was misformatted             */
#define ENSRSERVFAIL            162             /* DNS server returned general failure                  */
#define ENSRNOTFOUND            163             /* Domain name not found                                */
#define ENSRNOTIMP              164             /* DNS server does not implement requested operation    */
#define ENSRREFUSED             165             /* DNS server refused query                             */
#define ENSRBADQUERY            166             /* Misformatted DNS query                               */
#define ENSRBADNAME             167             /* Misformatted domain name                             */
#define ENSRBADFAMILY           168             /* Unsupported address family                           */
#define ENSRBADRESP             169             /* Misformatted DNS reply                               */
#define ENSRCONNREFUSED         170             /* Could not contact DNS servers                        */
#define ENSRTIMEOUT             171             /* Timeout while contacting DNS servers                 */
#define ENSROF                  172             /* End of file                                          */
#define ENSRFILE                173             /* Error reading file                                   */
#define ENSRNOMEM               174             /* Out of memory                                        */
#define ENSRDESTRUCTION         175             /* Application terminated lookup                        */
#define ENSRQUERYDOMAINTOOLONG  176             /* Domain name is too long                              */
#define ENSRCNAMELOOP           177             /* Domain name is too long                              */

#endif                                          /*  __LWIP_ERRNO_H                                      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
