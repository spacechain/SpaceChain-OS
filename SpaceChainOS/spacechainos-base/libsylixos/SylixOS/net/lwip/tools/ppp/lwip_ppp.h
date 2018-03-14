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
** 文   件   名: lwip_ppp.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 01 月 08 日
**
** 描        述: lwip ppp 连接管理器.
*********************************************************************************************************/

#ifndef __LWIP_PPP_H
#define __LWIP_PPP_H

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_LWIP_PPP > 0)

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  ppp 拨号串口参数
*********************************************************************************************************/

typedef struct {
    INT     baud;
    INT     stop_bits;                                                  /*  停止位数 1, 2               */
    INT     parity;                                                     /*  0:无校验 1:奇校验 2:偶校验  */
} LW_PPP_TTY;

/*********************************************************************************************************
  ppp 拨号参数
*********************************************************************************************************/

typedef struct {
    char   *user;
    char   *passwd;
} LW_PPP_DIAL;

/*********************************************************************************************************
  api
*********************************************************************************************************/

LW_API INT  API_PppOsCreate(CPCHAR  pcSerial, LW_PPP_TTY  *ptty, PCHAR  pcIfName, size_t  stMaxSize);
LW_API INT  API_PppOeCreate(CPCHAR  pcEthIf,  PCHAR  pcIfName, size_t  stMaxSize);
LW_API INT  API_PppOl2tpCreate(CPCHAR  pcEthIf, CPCHAR  pcIp, UINT16  usPort, CPCHAR  pcSecret,
                               size_t  stSecretLen, PCHAR   pcIfName, size_t  stMaxSize);
LW_API INT  API_PppDelete(CPCHAR  pcIfName);
LW_API INT  API_PppConnect(CPCHAR  pcIfName, LW_PPP_DIAL *pdial);
LW_API INT  API_PppDisconnect(CPCHAR  pcIfName, BOOL  bForce);
LW_API INT  API_PppGetPhase(CPCHAR  pcIfName, INT  *piPhase);

#define pppOsCreate     API_PppOsCreate
#define pppOeCreate     API_PppOeCreate
#define pppOl2tpCreate  API_PppOl2tpCreate
#define pppConnect      API_PppConnect
#define pppDisconnect   API_PppDisconnect
#define pppGetPhase     API_PppGetPhase

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_LWIP_PPP > 0         */
#endif                                                                  /*  __LWIP_PPPFD_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
