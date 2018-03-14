/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: x86Pentium.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 04 月 12 日
**
** 描        述: x86 体系构架 Pentium 相关库函数.
*********************************************************************************************************/

#ifndef __X86_PENTIUM_H
#define __X86_PENTIUM_H

/*********************************************************************************************************
  MSR 定义
*********************************************************************************************************/
#define X86_MSR_APICBASE            0x001b                              /*  Local APIC base reg         */

#define X86_MSR_IA32_EFER           0xc0000080
#define X86_MSR_IA32_STAR           0xc0000081
#define X86_MSR_IA32_LSTAR          0xc0000082
#define X86_MSR_IA32_FMASK          0xc0000084
#define X86_MSR_IA32_GS_BASE        0xc0000101
#define X86_MSR_IA32_KERNEL_GS_BASE 0xc0000102
/*********************************************************************************************************
  MTRR MSR 定义
*********************************************************************************************************/
#define X86_MSR_MTRR_CAP            0x00fe

#define X86_MSR_MTRR_DEFTYPE        0x02ff

#define X86_MSR_MTRR_PHYS_BASE0     0x0200
#define X86_MSR_MTRR_PHYS_MASK0     0x0201
#define X86_MSR_MTRR_PHYS_BASE1     0x0202
#define X86_MSR_MTRR_PHYS_MASK1     0x0203
#define X86_MSR_MTRR_PHYS_BASE2     0x0204
#define X86_MSR_MTRR_PHYS_MASK2     0x0205
#define X86_MSR_MTRR_PHYS_BASE3     0x0206
#define X86_MSR_MTRR_PHYS_MASK3     0x0207
#define X86_MSR_MTRR_PHYS_BASE4     0x0208
#define X86_MSR_MTRR_PHYS_MASK4     0x0209
#define X86_MSR_MTRR_PHYS_BASE5     0x020a
#define X86_MSR_MTRR_PHYS_MASK5     0x020b
#define X86_MSR_MTRR_PHYS_BASE6     0x020c
#define X86_MSR_MTRR_PHYS_MASK6     0x020d
#define X86_MSR_MTRR_PHYS_BASE7     0x020e
#define X86_MSR_MTRR_PHYS_MASK7     0x020f

#define X86_MSR_MTRR_FIX_00000      0x0250
#define X86_MSR_MTRR_FIX_80000      0x0258
#define X86_MSR_MTRR_FIX_A0000      0x0259
#define X86_MSR_MTRR_FIX_C0000      0x0268
#define X86_MSR_MTRR_FIX_C8000      0x0269
#define X86_MSR_MTRR_FIX_D0000      0x026a
#define X86_MSR_MTRR_FIX_D8000      0x026b
#define X86_MSR_MTRR_FIX_E0000      0x026c
#define X86_MSR_MTRR_FIX_E8000      0x026d
#define X86_MSR_MTRR_FIX_F0000      0x026e
#define X86_MSR_MTRR_FIX_F8000      0x026f
/*********************************************************************************************************
  MTRR 相关宏定义
*********************************************************************************************************/
#define X86_MTRR_UC                 0x00
#define X86_MTRR_WC                 0x01
#define X86_MTRR_WT                 0x04
#define X86_MTRR_WP                 0x05
#define X86_MTRR_WB                 0x06
#define X86_MTRR_E                  0x00000800
#define X86_MTRR_FE                 0x00000400
#define X86_MTRR_VCNT               0x000000ff
#define X86_MTRR_FIX_SUPPORT        0x00000100
#define X86_MTRR_WC_SUPPORT         0x00000400
/*********************************************************************************************************
 IA32_EFER 相关宏定义
*********************************************************************************************************/
#define X86_IA32_EFER_NXE           0x00000800                      /*  Execute-disable bit enable      */
#define X86_IA32_EFER_LME           0x00000100                      /*  Enables IA-32e mode operation.  */
/*********************************************************************************************************
  数据类型定义
*********************************************************************************************************/
#ifndef ASSEMBLY

typedef struct {
    INT32           MSR_iAddr;                                      /*  Address of the MSR              */
    PCHAR           MSR_pcName;                                     /*  Name of the MSR                 */
} X86_PENTIUM_MSR;                                                  /*  Pentium MSR                     */
/*********************************************************************************************************
  MTRR 相关数据类型定义
*********************************************************************************************************/
typedef struct {
    CHAR            FIX_cType[8];
} X86_MTRR_FIX;                                                     /*  MTRR fixed range register       */

typedef struct {
    UINT64          VAR_ui64Base;
    UINT64          VAR_ui64Mask;
} X86_MTRR_VAR;                                                     /*  MTRR variable range register    */

typedef struct {
    INT             MTRR_iCap[2];                                   /*  MTRR cap register               */
    INT             MTRR_iDefType[2];                               /*  MTRR defType register           */
    X86_MTRR_FIX    MTRR_fix[11];                                   /*  MTRR fixed range registers      */
    X86_MTRR_VAR    MTRR_var[256];                                  /*  MTRR variable range registers   */
} X86_MTRR;                                                         /*  MTRR                            */
typedef X86_MTRR *PX86_MTRR;
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
VOID  x86PentiumMsrGet(UINT  uiAddr, UINT64        *pData);
VOID  x86PentiumMsrSet(UINT  uiAddr, const UINT64  *pData);
/*********************************************************************************************************
  MTRR 相关函数声明
*********************************************************************************************************/
VOID  x86PentiumMtrrEnable(VOID);
VOID  x86PentiumMtrrDisable(VOID);
INT   x86PentiumMtrrGet(PX86_MTRR  pMtrr);
INT   x86PentiumMtrrSet(PX86_MTRR  pMtrr);

#endif                                                              /*  ASSEMBLY                        */

#endif                                                              /*  __X86_PENTIUM_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
