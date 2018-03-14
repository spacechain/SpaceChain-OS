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
** 文   件   名: lwip_ftplib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 11 月 21 日
**
** 描        述: ftp 协议相关格式与代码.
*********************************************************************************************************/

#ifndef __LWIP_FTPLIB_H
#define __LWIP_FTPLIB_H

/*********************************************************************************************************
  服务器信息
*********************************************************************************************************/
#define __FTPD_SERVER_MESSAGE   "SylixOS FTP server ready." __SYLIXOS_VERINFO
#define __FTPD_SYSTYPE          "UNIX Type: L8"

/*********************************************************************************************************
  服务器响应码
*********************************************************************************************************/
#define __FTPD_RETCODE_SERVER_RESTART_ACK       110                     /*  重新启动标记应答            */
#define __FTPD_RETCODE_SERVER_READY_MIN         120                     /*  服务在nnn分钟内准备好       */
#define __FTPD_RETCODE_SERVER_DATALINK_READY    125                     /*  数据连接已准备好            */
#define __FTPD_RETCODE_SERVER_FILE_OK           150                     /*  文件状态良好，打开数据连接  */
#define __FTPD_RETCODE_SERVER_CMD_OK            200                     /*  命令成功                    */
#define __FTPD_RETCODE_SERVER_CMD_UNSUPPORT     202                     /*  命令未实现                  */
#define __FTPD_RETCODE_SERVER_STATUS_HELP       211                     /*  系统状态或系统帮助响应      */
#define __FTPD_RETCODE_SERVER_DIR_STATUS        212                     /*  目录状态                    */
#define __FTPD_RETCODE_SERVER_FILE_STATUS       213                     /*  文件状态                    */
#define __FTPD_RETCODE_SERVER_HELP_INFO         214                     /*  帮助信息, 仅对人类用户有用  */
#define __FTPD_RETCODE_SERVER_NAME_SYS_TYPE     215                     /*  名字系统类型                */
#define __FTPD_RETCODE_SERVER_READY             220                     /*  服务器就绪                  */
#define __FTPD_RETCODE_SERVER_BYEBYE            221                     /*  服务关闭控制连接可以退出登录*/
#define __FTPD_RETCODE_SERVER_DATALINK_NODATA   225                     /*  数据连接打开，无传输正在进行*/
#define __FTPD_RETCODE_SERVER_DATACLOSE_NOERR   226                     /*  关闭数据连接, 文件操作成功  */
#define __FTPD_RETCODE_SERVER_INTO_PASV         227                     /*  进入被动模式                */
#define __FTPD_RETCODE_SERVER_USER_LOGIN        230                     /*  用户登录                    */
#define __FTPD_RETCODE_SERVER_FILE_OP_OK        250                     /*  请求的文件操作完成          */
#define __FTPD_RETCODE_SERVER_MAKE_DIR_OK       257                     /*  创建目录成功                */
#define __FTPD_RETCODE_SERVER_PW_REQ            331                     /*  用户名正确，需要口令        */
#define __FTPD_RETCODE_SERVER_NEED_INFO         350                     /*  请求需要进一步的信息        */
#define __FTPD_RETCODE_SERVER_DATALINK_FAILED   425                     /*  不能打开数据连接            */
#define __FTPD_RETCODE_SERVER_DATALINK_ABORT    426                     /*  关闭连接，中止传输          */
#define __FTPD_RETCODE_SERVER_REQ_NOT_RUN       450                     /*  请求命令未执行              */
#define __FTPD_RETCODE_SERVER_REQ_ABORT         451                     /*  中止请求的操作 本地有错     */
#define __FTPD_RETCODE_SERVER_DONOT_RUN_REQ     452                     /*  未执行请求的操作,空间不足   */
#define __FTPD_RETCODE_SERVER_CMD_ERROR         500                     /*  命令不可识别                */
#define __FTPD_RETCODE_SERVER_SYNTAX_ERR        501                     /*  参数语法错误                */
#define __FTPD_RETCODE_SERVER_UNSUP_WITH_ARG    504                     /*  此参数下的命令功能未实现    */
#define __FTPD_RETCODE_SERVER_LOGIN_FAILED      530                     /*  用户登录失败                */
#define __FTPD_RETCODE_SERVER_REQ_FAILED        550                     /*  未执行请求的操作            */
#define __FTPD_RETCODE_SERVER_DREQ_ABORT        551                     /*  数据请求中止                */
#define __FTPD_RETCODE_SERVER_FILE_NAME_ERROR   553                     /*  文件名不合法                */

#endif                                                                  /*  __LWIP_FTP_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
