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
** 文   件   名: lwip_ftp.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 11 月 21 日
**
** 描        述: ftp 协议工具.
** 描        述: ftp 服务器. (可以显示整个 SylixOS 目录)
** 注        意: ftp 服务器 1 分钟不访问, 将会自动断开, 以释放资源.
                 如果 ftp 服务器的个目录不是系统根目录, 有些版本 windows 访问此服务器有些功能有问题, 
                 例如: 重命名. 推荐将 ftp 根目录设置为 /
                 推荐使用 CuteFTP, FileZilla 等专业 FTP 客户端访问此服务器.
*********************************************************************************************************/

#ifndef __LWIP_FTP_H
#define __LWIP_FTP_H

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_FTPD_EN > 0)

LW_API VOID     API_INetFtpServerInit(CPCHAR  pcPath);
LW_API VOID     API_INetFtpServerShow(VOID);
LW_API INT      API_INetFtpServerPath(CPCHAR  pcPath);

#define inetFtpServerInit           API_INetFtpServerInit
#define inetFtpServerShow           API_INetFtpServerShow
#define inetFtpServerPath           API_INetFtpServerPath

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_FTPD_EN > 0      */
#endif                                                                  /*  __LWIP_FTP_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
