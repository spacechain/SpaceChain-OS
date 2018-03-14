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
** 文   件   名: lwip_tftp.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 05 月 29 日
**
** 描        述: tftp 网络工具. (不支持选项)
*********************************************************************************************************/

#ifndef __LWIP_TFTP_H
#define __LWIP_TFTP_H

/*********************************************************************************************************
  tftp 配置
*********************************************************************************************************/

#define LWIP_TFTP_TIMEOUT           3000                                /*  超时时间                    */

/*********************************************************************************************************
  tftp 协议相关
*********************************************************************************************************/

#define LWIP_TFTP_OPC_RDREQ         1                                   /*  读请求                      */
#define LWIP_TFTP_OPC_WRREQ         2                                   /*  写请求                      */
#define LWIP_TFTP_OPC_DATA          3                                   /*  文件数据                    */
#define LWIP_TFTP_OPC_ACK           4                                   /*  数据确认                    */
#define LWIP_TFTP_OPC_ERROR         5                                   /*  error 信息                  */

#define LWIP_TFTP_DATA_HEADERLEN    4                                   /*  数据包头长度                */
#define LWIP_TFTP_ERROR_HEADERLEN   4                                   /*  错误信息包头长度            */

/*********************************************************************************************************
  tftp 错误号
*********************************************************************************************************/

#define LWIP_TFTP_EC_NONE           0                                   /*  NONE                        */
#define LWIP_TFTP_EC_NOFILE         1                                   /*  文件未找到                  */
#define LWIP_TFTP_EC_NOACCESS       2                                   /*  访问失败                    */
#define LWIP_TFTP_EC_DISKFULL       3                                   /*  磁盘满                      */
#define LWIP_TFTP_EC_TFTP           4                                   /*  TFTP 错误                   */
#define LWIP_TFTP_EC_ID             5                                   /*  错误 ID                     */
#define LWIP_TFTP_EC_FILEEXIST      6                                   /*  文件存在                    */
#define LWIP_TFTP_EC_NOUSER         7                                   /*  用户错误                    */

/*********************************************************************************************************
  tftp client API
*********************************************************************************************************/

LW_API INT          API_INetTftpReceive(CPCHAR  pcRemoteHost, 
                                        CPCHAR  pcFileName, 
                                        CPCHAR  pcLocalFileName);       /*  请求远程文件                */

LW_API INT          API_INetTftpSend(CPCHAR  pcRemoteHost, 
                                     CPCHAR  pcFileName, 
                                     CPCHAR  pcLocalFileName);          /*  发送本地文件                */
                                     
#define inetTftpReceive             API_INetTftpReceive
#define inetTftpSend                API_INetTftpSend

/*********************************************************************************************************
  tftp server API
*********************************************************************************************************/

LW_API VOID         API_INetTftpServerInit(CPCHAR  pcPath);             /*  启动 TFTP 简易服务器        */
                                                                        /*  仅支持二进制传输            */
LW_API INT          API_INetTftpServerPath(CPCHAR  pcPath);             /*  设置服务器路径              */


#define inetTftpServerInit          API_INetTftpServerInit
#define inetTftpServerPath          API_INetTftpServerPath

#endif                                                                  /*  __LWIP_TFTP_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
