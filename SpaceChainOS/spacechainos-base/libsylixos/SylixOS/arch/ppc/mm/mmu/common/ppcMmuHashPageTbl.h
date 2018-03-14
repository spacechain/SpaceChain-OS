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
** 文   件   名: ppcMmuHashPageTbl.h
**
** 创   建   人: Dang.YueDong (党跃东)
**
** 文件创建日期: 2016 年 01 月 15 日
**
** 描        述: PowerPC 体系构架 MMU Hashed 页表驱动.
*********************************************************************************************************/

#ifndef __ARCH_PPCMMUHASHTBL_H
#define __ARCH_PPCMMUHASHTBL_H

#define MMU_HASH_DIVISOR                    0x00800000
#define MMU_SDR1_HTABORG_MASK               0xffff0000                  /*  HTABORG mask                */
#define MMU_SDR1_HTABORG_8M                 0xffff0000                  /*  HTABORG value for 8M        */
#define MMU_SDR1_HTABORG_16M                0xfffe0000                  /*  HTABORG value for 16M       */
#define MMU_SDR1_HTABORG_32M                0xfffc0000                  /*  HTABORG value for 32M       */
#define MMU_SDR1_HTABORG_64M                0xfff80000                  /*  HTABORG value for 64M       */
#define MMU_SDR1_HTABORG_128M               0xfff00000                  /*  HTABORG value for 128M      */
#define MMU_SDR1_HTABORG_256M               0xffe00000                  /*  HTABORG value for 256M      */
#define MMU_SDR1_HTABORG_512M               0xffc00000                  /*  HTABORG value for 512M      */
#define MMU_SDR1_HTABORG_1G                 0xff800000                  /*  HTABORG value for 1G        */
#define MMU_SDR1_HTABORG_2G                 0xff000000                  /*  HTABORG value for 2G        */
#define MMU_SDR1_HTABORG_4G                 0xfe000000                  /*  HTABORG value for 4G        */

#define MMU_SDR1_HTABMASK_MASK              0x000001ff                  /*  HTABMASK mask               */
#define MMU_SDR1_HTABMASK_SHIFT             16                          /*  HTABMASK shift              */
#define MMU_SDR1_HTABMASK_8M                0x00000000                  /*  HTABMASK value for 8M       */
#define MMU_SDR1_HTABMASK_16M               0x00000001                  /*  HTABMASK value for 16M      */
#define MMU_SDR1_HTABMASK_32M               0x00000003                  /*  HTABMASK value for 32M      */
#define MMU_SDR1_HTABMASK_64M               0x00000007                  /*  HTABMASK value for 64M      */
#define MMU_SDR1_HTABMASK_128M              0x0000000f                  /*  HTABMASK value for 128M     */
#define MMU_SDR1_HTABMASK_256M              0x0000001f                  /*  HTABMASK value for 256M     */
#define MMU_SDR1_HTABMASK_512M              0x0000003f                  /*  HTABMASK value for 512M     */
#define MMU_SDR1_HTABMASK_1G                0x0000007f                  /*  HTABMASK value for 1G       */
#define MMU_SDR1_HTABMASK_2G                0x000000ff                  /*  HTABMASK value for 2G       */
#define MMU_SDR1_HTABMASK_4G                0x000001ff                  /*  HTABMASK value for 4G       */

#define MMU_PTE_MIN_SIZE_8M                 0x00010000                  /*   64k min size               */
#define MMU_PTE_MIN_SIZE_16M                0x00020000                  /*  128k min size               */
#define MMU_PTE_MIN_SIZE_32M                0x00040000                  /*  256k min size               */
#define MMU_PTE_MIN_SIZE_64M                0x00080000                  /*  512k min size               */
#define MMU_PTE_MIN_SIZE_128M               0x00100000                  /*    1M min size               */
#define MMU_PTE_MIN_SIZE_256M               0x00200000                  /*    2M min size               */
#define MMU_PTE_MIN_SIZE_512M               0x00400000                  /*    4M min size               */
#define MMU_PTE_MIN_SIZE_1G                 0x00800000                  /*    8M min size               */
#define MMU_PTE_MIN_SIZE_2G                 0x01000000                  /*   16M min size               */
#define MMU_PTE_MIN_SIZE_4G                 0x02000000                  /*   32M min size               */

#define MMU_EA_SR                           0xf0000000                  /*  SR field in E.A             */
#define MMU_EA_SR_SHIFT                     28                          /*                              */
#define MMU_EA_PAGE_INDEX                   0x0ffff000                  /*  Page Index in E.A           */
#define MMU_EA_PAGE_INDEX_SHIFT             12                          /*  Page Index shift            */
#define MMU_EA_BYTE_OFFSET                  0x00000fff                  /*  Byte Offset in E.A          */
#define MMU_EA_API                          0x0000fc00                  /*  Abbreviated page index      */
#define MMU_EA_API_SHIFT                    10                          /*  API shift in page index     */

#define MMU_SR_VSID_MASK                    0x00ffffff                  /*  Virtual segment ID          */
#define MMU_SR_NB_SHIFT                     28

#define MMU_VSID_PRIM_HASH                  0x000fffff                  /*  Primary hash value in VSID  */

#define MMU_HASH_VALUE_LOW                  0x000003ff

#define MMU_HASH_VALUE_HIGH                 0x0007fc00

#define MMU_HASH_VALUE_HIGH_SHIFT           10

#define MMU_PTE_HASH_VALUE_HIGH_SHIFT       16
#define MMU_PTE_HASH_VALUE_LOW_SHIFT        6

#define MMU_PTE_VSID_SHIFT                  7
#define MMU_PTE_V                           0x80000000
#define MMU_PTE_H                           0x00000040
#define MMU_PTE_H_SHIFT                     6
#define MMU_PTE_API                         0x0000003f                  /*  API field in PTE            */
#define MMU_PTE_API_SHIFT                   0                           /*  API shift in PTE            */

#define MMU_PTES_IN_PTEG                    8

/*********************************************************************************************************
  PTE for 32-bit implementations
*********************************************************************************************************/

typedef union {
    struct {
        UINT    PTE_bV              :  1;                               /*  Entry valid or invalid(v=0) */
        UINT    PTE_uiVSID          : 24;                               /*  Virtual Segment ID          */
        UINT    PTE_bH              :  1;                               /*  Hash Function identifier bit*/
        UINT    PTE_ucAPI           :  6;                               /*  Abbreviated page index      */

        UINT    PTE_uiRPN           : 20;                               /*  Physical(real) page number  */
        UINT    PTE_ucReserved1     :  3;                               /*  Reserved                    */
        UINT    PTE_bR              :  1;                               /*  Referenced bit              */
        UINT    PTE_bC              :  1;                               /*  Changed bit                 */
        UINT    PTE_ucWIMG          :  4;                               /*  Memory/cache control bit    */
        UINT    PTE_bReserved2      :  1;                               /*  Reserved                    */
        UINT    PTE_ucPP            :  2;                               /*  Page protection bits        */
    } field;
    struct {
        UINT32  PTE_uiWord0;                                            /*  Word 0                      */
        UINT32  PTE_uiWord1;                                            /*  Word 1                      */
    } words;
} PTE;

typedef struct {
    PTE         PTEG_PTEs[MMU_PTES_IN_PTEG];
} PTEG;

typedef union {
    UINT32      EA_uiValue;                                             /*  Effective address           */
    struct {
        UINT    EA_ucSRn            :  4;                               /*  SR select                   */
        UINT    EA_uiPageIndex      : 16;                               /*  Page index                  */
        UINT    EA_uiPageOffset     : 12;                               /*  Byte offset                 */
    } field;
} EA;

typedef union {
    UINT32      SR_uiValue;                                             /*  SR value                    */
    struct {
        UINT    SR_bT               :  1;                               /*  SR format flag              */
        UINT    SR_bKS              :  1;                               /*  Supervisor-state protect key*/
        UINT    SR_bKP              :  1;                               /*  User-state protection key   */
        UINT    SR_bN               :  1;                               /*  No-execute protection       */
        UINT    SR_ucReserved       :  4;                               /*  Reserved                    */
        UINT    SR_uiVSID           : 24;                               /*  Virtual segment Id          */
    } field;
} SR;

/*********************************************************************************************************
  PPC MMU HASH
*********************************************************************************************************/

INT   ppcMmuHashPageTblInit(UINT32  uiMemSize);
VOID  ppcMmuHashPageTblMakeTrans(addr_t  ulEffectiveAddr,
                                 UINT32  uiPteValue1);
VOID  ppcMmuHashPageTblFlagSet(addr_t  ulEffectiveAddr,
                               UINT32  uiPteValue1);
VOID  ppcMmuHashPageTblPteMiss(addr_t  ulEffectiveAddr,
                               UINT32  uiPteValue1);
VOID  ppcMmuHashPageTblPtePreLoad(addr_t  ulEffectiveAddr,
                                  UINT32  uiPteValue1);

#endif                                                                  /*  __ARCH_PPCMMUHASHTBL_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
