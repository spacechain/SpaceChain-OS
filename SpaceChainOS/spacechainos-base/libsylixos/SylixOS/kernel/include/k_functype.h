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
** 文   件   名: k_functype.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 11 月 21 日
**
** 描        述: 这是系统相关函数指针类型声明.
*********************************************************************************************************/

#ifndef __K_FUNCTYPE_H
#define __K_FUNCTYPE_H

/*********************************************************************************************************
  FUNCPTR
*********************************************************************************************************/

#ifdef __cplusplus
typedef INT         (*FUNCPTR)(...);                                    /*  function returning int      */
typedef off_t       (*OFFTFUNCPTR)(...);                                /*  function returning off_t    */
typedef size_t      (*SIZETFUNCPTR)(...);                               /*  function returning size_t   */
typedef ssize_t     (*SSIZETFUNCPTR)(...);                              /*  function returning ssize_t  */
typedef LONG        (*LONGFUNCPTR)(...);                                /*  function returning long     */
typedef ULONG       (*ULONGFUNCPTR)(...);                               /*  function returning ulong    */
typedef VOID        (*VOIDFUNCPTR)(...);                                /*  function returning void     */
typedef PVOID       (*PVOIDFUNCPTR)(...);                               /*  function returning void *   */
typedef BOOL        (*BOOLFUNCPTR)(...);                                /*  function returning bool     */

#else
typedef INT         (*FUNCPTR)();                                       /*  function returning int      */
typedef off_t       (*OFFTFUNCPTR)();                                   /*  function returning off_t    */
typedef size_t      (*SIZETFUNCPTR)();                                  /*  function returning size_t   */
typedef ssize_t     (*SSIZETFUNCPTR)();                                 /*  function returning ssize_t  */
typedef LONG        (*LONGFUNCPTR)();                                   /*  function returning long     */
typedef ULONG       (*ULONGFUNCPTR)();                                  /*  function returning ulong    */
typedef VOID        (*VOIDFUNCPTR)();                                   /*  function returning void     */
typedef PVOID       (*PVOIDFUNCPTR)();                                  /*  function returning void *   */
typedef BOOL        (*BOOLFUNCPTR)();                                   /*  function returning bool     */
#endif			                                                        /*  _cplusplus                  */

/*********************************************************************************************************
  HOOK
*********************************************************************************************************/

#ifdef __cplusplus
typedef VOID            (*LW_HOOK_FUNC)(...);                           /*  HOOK 函数指针               */

#else
typedef VOID            (*LW_HOOK_FUNC)();                              /*  HOOK 函数指针               */
#endif                                                                  /*  _cplusplus                  */

#endif                                                                  /*  __K_FUNCTYPE_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
