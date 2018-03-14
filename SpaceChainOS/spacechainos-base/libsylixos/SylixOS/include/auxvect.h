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
** 文   件   名: auxvect.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 05 月 04 日
**
** 描        述: Symbolic values for the entries in the auxiliary table.
*********************************************************************************************************/

#ifndef __AUXVECT_H
#define __AUXVECT_H

/*********************************************************************************************************
  Symbolic values for the entries in the auxiliary table.
*********************************************************************************************************/

#define AT_NULL             0                                           /* end of vector                */
#define AT_IGNORE           1                                           /* entry should be ignored      */
#define AT_EXECFD           2                                           /* file descriptor of program   */
#define AT_PHDR             3                                           /* program headers for program  */
#define AT_PHENT            4                                           /* size of program header entry */
#define AT_PHNUM            5                                           /* number of program headers    */
#define AT_PAGESZ           6                                           /* system page size             */
#define AT_BASE             7                                           /* base address of interpreter  */
#define AT_FLAGS            8                                           /* flags                        */
#define AT_ENTRY            9                                           /* entry point of program       */
#define AT_NOTELF           10                                          /* program is not ELF           */
#define AT_UID              11                                          /* real uid                     */
#define AT_EUID             12                                          /* effective uid                */
#define AT_GID              13                                          /* real gid                     */
#define AT_EGID             14                                          /* effective gid                */
#define AT_PLATFORM         15                                          /* string identifying CPU for   */
                                                                        /* optimizations                */
#define AT_HWCAP            16                                          /* arch dependent hints at CPU  */
                                                                        /* capabilities                 */
#define AT_CLKTCK           17                                          /* frequency at which times()   */
                                                                        /* increments                   */
                                                                        
/*********************************************************************************************************
  AT_* values 18 through 22 are reserved
*********************************************************************************************************/

#define AT_SECURE           23                                          /* secure mode boolean          */
#define AT_BASE_PLATFORM    24                                          /* str identifying real platform*/
                                                                        /* may differ from AT_PLATFORM. */
#define AT_RANDOM           25                                          /* address of 16 random bytes   */

#define AT_EXECFN           31                                          /* filename of program          */

#endif                                                                  /*  __AUXVECT_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
