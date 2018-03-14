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
** 文   件   名: vmmSwap.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 05 月 16 日
**
** 描        述: 平台无关内存交换区管理.
*********************************************************************************************************/

#ifndef __VMMSWAP_H
#define __VMMSWAP_H

/*********************************************************************************************************
  异常消息
*********************************************************************************************************/

typedef struct {
    PVOID               PAGEFCTX_pvStackRet;                            /*  跳跃返回堆栈的地址          */
#if defined(LW_CFG_CPU_ARCH_C6X)
    ULONG               PAGEFCTX_ulContextType;                         /*  上下文类型                  */
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */
    addr_t              PAGEFCTX_ulRetAddr;                             /*  异常返回地址                */
    addr_t              PAGEFCTX_ulAbortAddr;                           /*  内存访问失效地址            */
    LW_VMM_ABORT        PAGEFCTX_abtInfo;                               /*  异常类型                    */
    PLW_CLASS_TCB       PAGEFCTX_ptcb;                                  /*  产生缺页中断的线程          */
    
    errno_t             PAGEFCTX_iLastErrno;                            /*  返回时需要恢复的信息        */
    INT                 PAGEFCTX_iKernelSpace;
} LW_VMM_PAGE_FAIL_CTX;
typedef LW_VMM_PAGE_FAIL_CTX    *PLW_VMM_PAGE_FAIL_CTX;

#define __PAGEFAILCTX_SIZE_ALIGN    ROUND_UP(sizeof(LW_VMM_PAGE_FAIL_CTX), sizeof(LW_STACK))

#define __PAGEFAILCTX_ABORT_TYPE(pctx)      (pctx->PAGEFCTX_abtInfo.VMABT_uiType)
#define __PAGEFAILCTX_ABORT_METHOD(pctx)    (pctx->PAGEFCTX_abtInfo.VMABT_uiMethod)

/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

/*********************************************************************************************************
  缺页中断系统支持结构
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE        PAGEP_lineManage;                               /*  area 链表                   */
    PLW_VMM_PAGE        PAGEP_pvmpageVirtual;                           /*  回指虚拟页面控制块          */
    
#define LW_VMM_SHARED_CHANGE    1
#define LW_VMM_PRIVATE_CHANGE   2
    INT                 PAGEP_iFlags;                                   /*  like mmap flags             */
    
    FUNCPTR             PAGEP_pfuncFiller;                              /*  页面填充器                  */
    PVOID               PAGEP_pvArg;                                    /*  页面填充器参数              */
    
    PVOIDFUNCPTR        PAGEP_pfuncFindShare;                           /*  寻找可以共享的页面          */
    PVOID               PAGEP_pvFindArg;                                /*  参数                        */
} LW_VMM_PAGE_PRIVATE;
typedef LW_VMM_PAGE_PRIVATE *PLW_VMM_PAGE_PRIVATE;

/*********************************************************************************************************
  虚拟分页空间链表 (缺页中断查找表)
*********************************************************************************************************/
extern LW_LIST_LINE_HEADER  _K_plineVmmVAddrSpaceHeader;

/*********************************************************************************************************
  内存交换函数
*********************************************************************************************************/
BOOL            __vmmPageSwapIsNeedLoad(pid_t  pid, addr_t  ulVirtualPageAddr);
ULONG           __vmmPageSwapLoad(pid_t  pid, addr_t  ulDestVirtualPageAddr, addr_t  ulVirtualPageAddr);
PLW_VMM_PAGE    __vmmPageSwapSwitch(pid_t  pid, ULONG  ulPageNum, UINT  uiAttr);

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#endif                                                                  /*  __VMMSWAP_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
