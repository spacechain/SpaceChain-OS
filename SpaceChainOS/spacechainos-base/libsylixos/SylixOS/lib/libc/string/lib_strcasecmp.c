/*********************************************************************************************************
**
**                                    �й�������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: lib_strcasecmp.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 25 ��
**
** ��        ��: ��
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: lib_strcasecmp
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  lib_strcasecmp (CPCHAR cpcStr1, CPCHAR  cpcStr2)
{
    REGISTER PCHAR      pcStr1 = (PCHAR)cpcStr1;
    REGISTER PCHAR      pcStr2 = (PCHAR)cpcStr2;

    while (lib_toupper((UCHAR)*pcStr1) == lib_toupper((UCHAR)*pcStr2)) {
        if (*pcStr1 == PX_EOS) {
            return  (0);
        }
        pcStr1++;
        pcStr2++;
    }
    
    return  (lib_toupper((UCHAR)*pcStr1) - lib_toupper((UCHAR)*pcStr2));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/