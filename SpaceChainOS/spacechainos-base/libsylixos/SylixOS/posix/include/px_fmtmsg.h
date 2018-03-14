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
** 文   件   名: px_fmtmsg.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 09 月 17 日
**
** 描        述: 兼容 posix fmtmsg 库.
*********************************************************************************************************/

#ifndef __PX_FMTMSG_H
#define __PX_FMTMSG_H

#include "SylixOS.h"                                                    /*  操作系统头文件              */

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

/*********************************************************************************************************
  Major Classifications: identifies the source of the condition.
*********************************************************************************************************/
#define MM_HARD		0x01L	                                            /* Hardware                     */
#define MM_SOFT		0x02L	                                            /* Software                     */
#define MM_FIRM		0x03L	                                            /* Firmware                     */

/*********************************************************************************************************
  Message Source Subclassifications: type of software. 
*********************************************************************************************************/
#define MM_APPL		0x04L	                                            /* Application                  */
#define MM_UTIL		0x08L	                                            /* Utility                      */
#define MM_OPSYS	0x0cL	                                            /* Operating system             */

/*********************************************************************************************************
  Display Subclassifications: where to display the message.
*********************************************************************************************************/
#define MM_PRINT	0x10L	                                            /* Display on standard error    */
#define MM_CONSOLE	0x20L	                                            /* Display on system console    */

/*********************************************************************************************************
  Status subclassifications: whether the application will recover.
*********************************************************************************************************/
#define MM_RECOVER	0x40L	                                            /* Recoverable                  */
#define MM_NRECOV	0x80L	                                            /* Non-recoverable              */

/*********************************************************************************************************
  Severity: seriousness of the condition.
*********************************************************************************************************/
#define MM_NOSEV	0	                                                /* No severity level provided   */
#define MM_HALT		1	                                                /* Error causing application to halt */
#define MM_ERROR	2	                                                /* Encountered a non-fatal fault*/
#define MM_WARNING	3	                                                /* Unusual non-error condition  */
#define MM_INFO		4	                                                /* Informative message          */

/*********************************************************************************************************
  `Null' values for message components.
*********************************************************************************************************/
#define MM_NULLMC	0L		                                            /* `Null' classsification component */
#define MM_NULLLBL	(char *)0	                                        /* `Null' label component       */
#define MM_NULLSEV	0		                                            /* `Null' severity component    */
#define MM_NULLTXT	(char *)0	                                        /* `Null' text component        */
#define MM_NULLACT	(char *)0	                                        /* `Null' action component      */
#define MM_NULLTAG	(char *)0	                                        /* `Null' tag component         */

/*********************************************************************************************************
  Return values for fmtmsg().
*********************************************************************************************************/
#define MM_OK		0	                                                /* Function succeeded           */
#define MM_NOTOK	(-1)	                                            /* Function failed completely   */
#define MM_NOMSG	0x01	                                            /* Unable to perform MM_PRINT   */
#define MM_NOCON	0x02	                                            /* Unable to perform MM_CONSOLE */

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  api 
*********************************************************************************************************/

LW_API int  fmtmsg(long, const char *, int, const char *, const char *, const char *);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
#endif                                                                  /*  __PX_FMTMSG_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
