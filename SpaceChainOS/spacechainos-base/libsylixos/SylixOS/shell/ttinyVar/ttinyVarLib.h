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
** 文   件   名: ttinyVarLib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 07 月 27 日
**
** 描        述: 一个超小型的 shell 系统的变量管理器内部头文件.
*********************************************************************************************************/

#ifndef __TTINYVARLIB_H
#define __TTINYVARLIB_H

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

/*********************************************************************************************************
  VAR 结构
  注意 : SV_ulRefCnt 引用计数表明通过 __tshellVarGet() 直接获得 SV_pcVarValue 的次数, 
         SV_ulRefCnt 不为零, 则设置此环境变量时不释放老的缓冲区 (切记切记!)
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE             SV_lineManage;                             /*  管理用双链表                */
    LW_LIST_LINE             SV_lineHash;                               /*  哈希分离链表                */

    PCHAR                    SV_pcVarName;                              /*  变量名                      */
    PCHAR                    SV_pcVarValue;                             /*  变量的值                    */
    
    ULONG                    SV_ulRefCnt;                               /*  引用计数                    */
} __TSHELL_VAR;
typedef __TSHELL_VAR        *__PTSHELL_VAR;                             /*  指针类型                    */

/*********************************************************************************************************
  内部操作函数
*********************************************************************************************************/

ULONG  __tshellVarAdd(CPCHAR  pcVarName, CPCHAR  pcVarValue, size_t  stNameStrLen);
ULONG  __tshellVarDelete(__PTSHELL_VAR  pskvNode);
ULONG  __tshellVarDeleteByName(CPCHAR  pcVarName);
ULONG  __tshellVarFind(char  *pcVarName, __PTSHELL_VAR   *ppskvNode);
ULONG  __tshellVarList(__PTSHELL_VAR   pskvNodeStart,
                       __PTSHELL_VAR   ppskvNode[],
                       INT             iMaxCounter);
INT    __tshellVarGetRt(CPCHAR  pcVarName, 
                        PCHAR   pcVarValue,
                        INT     iMaxLen);
ULONG  __tshellVarGet(CPCHAR  pcVarName, PCHAR  *ppcVarValue);
ULONG  __tshellVarSet(CPCHAR  pcVarName, CPCHAR       pcVarValue, INT  iIsOverwrite);
ULONG  __tshellVarDefine(PCHAR  pcCmd);

INT    __tshellVarSave(CPCHAR  pcFileName);
INT    __tshellVarLoad(CPCHAR  pcFileName);

/*********************************************************************************************************
  进程补丁使用下列函数初始化环境变量组
*********************************************************************************************************/

INT    __tshellVarNum(VOID);
INT    __tshellVarDup(PVOID (*pfuncMalloc)(size_t stSize), PCHAR  ppcEvn[], ULONG  ulMax);

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  __TTINYVARLIB_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
