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
** 文   件   名: loader_error.h
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2010 年 04 月 17 日
**
** 描        述: loader errno.
*********************************************************************************************************/

#ifndef __LOADER_ERROR_H
#define __LOADER_ERROR_H

/*********************************************************************************************************
   LOADER ERROR     100000 - 100500
*********************************************************************************************************/

#define ERROR_LOADER_FORMAT         100000                              /*  elf格式错误                 */
#define ERROR_LOADER_ARCH           100001                              /*  elf体系结构不匹配           */
#define ERROR_LOADER_RELOCATE       100002                              /*  重定位错误                  */
#define ERROR_LOADER_EXPORT_SYM     100003                              /*  导出符号错误                */
#define ERROR_LOADER_NO_MODULE      100004                              /*  模块未找到                  */
#define ERROR_LOADER_CREATE         100005                              /*  创建模块出错                */
#define ERROR_LOADER_NO_INIT        100006                              /*  初始化函数未找到            */
#define ERROR_LOADER_NO_ENTRY       100007                              /*  入口函数未找到              */
#define ERROR_LOADER_PARAM_NULL     100008                              /*  参数为空                    */
#define ERROR_LOADER_UNEXPECTED     100009                              /*  未知错误                    */
#define ERROR_LOADER_NO_SYMBOL      100010                              /*  符号未找到                  */
#define ERROR_LOADER_VERSION        100011                              /*  版本号不符                  */
#define ERROR_LOADER_NOT_KO         100012                              /*  不是内核模块                */
#define ERROR_LOADER_EACCES         EACCES                              /*  没有执行权限                */

#endif                                                                  /*  __LOADER_ERROR_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
