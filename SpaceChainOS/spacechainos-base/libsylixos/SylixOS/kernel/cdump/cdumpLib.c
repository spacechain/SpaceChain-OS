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
** 文   件   名: cdumpLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 01 日
**
** 描        述: 系统/应用崩溃信息记录.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_CDUMP_EN > 0) && (LW_CFG_DEVICE_EN > 0)
/*********************************************************************************************************
  缓冲信息
*********************************************************************************************************/
static PVOID  _K_pvCrashDumpBuffer = (PVOID)PX_ERROR;
static size_t _K_stCrashDumpSize   = LW_CFG_CDUMP_BUF_SIZE;
/*********************************************************************************************************
  最长信息长度
*********************************************************************************************************/
#define LW_CDUMP_BUF_SIZE   (_K_stCrashDumpSize)
#define LW_CDUMP_MAX_LEN    (LW_CDUMP_BUF_SIZE - 1)
/*********************************************************************************************************
** 函数名称: _CrashDumpAbortStkOf
** 功能描述: 堆栈溢出崩溃信息记录.
** 输　入  : ulRetAddr     异常返回 PC 地址
**           ulAbortAddr   异常地址
**           pcInfo        异常信息
**           ptcb          异常任务
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _CrashDumpAbortStkOf (addr_t  ulRetAddr, addr_t  ulAbortAddr, CPCHAR  pcInfo, PLW_CLASS_TCB  ptcb)
{
    PCHAR   pcCdump = (PCHAR)_K_pvCrashDumpBuffer;
    size_t  stOft   = 4;
    
    if (!pcCdump || (pcCdump == (PCHAR)PX_ERROR)) {
        return;
    }
    
    pcCdump[0] = LW_CDUMP_MAGIC_0;
    pcCdump[1] = LW_CDUMP_MAGIC_1;
    pcCdump[2] = LW_CDUMP_MAGIC_2;
    pcCdump[3] = LW_CDUMP_MAGIC_3;
    
    stOft = bnprintf(pcCdump, LW_CDUMP_MAX_LEN, stOft, 
                     "FATAL ERROR: thread %lx[%s] stack overflow. "
                     "ret_addr: 0x%08lx abt_addr: 0x%08lx abt_type: %s\n"
                     "rebooting...\n",
                     ptcb->TCB_ulId, ptcb->TCB_cThreadName,
                     ulRetAddr, ulAbortAddr, pcInfo);
    
    pcCdump[stOft] = PX_EOS;
}
/*********************************************************************************************************
** 函数名称: _CrashDumpAbortFatal
** 功能描述: 崩溃信息记录.
** 输　入  : ulRetAddr     异常返回 PC 地址
**           ulAbortAddr   异常地址
**           pcInfo        异常信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _CrashDumpAbortFatal (addr_t  ulRetAddr, addr_t  ulAbortAddr, CPCHAR  pcInfo)
{
    PCHAR   pcCdump = (PCHAR)_K_pvCrashDumpBuffer;
    size_t  stOft   = 4;
    
    if (!pcCdump || (pcCdump == (PCHAR)PX_ERROR)) {
        return;
    }
    
    pcCdump[0] = LW_CDUMP_MAGIC_0;
    pcCdump[1] = LW_CDUMP_MAGIC_1;
    pcCdump[2] = LW_CDUMP_MAGIC_2;
    pcCdump[3] = LW_CDUMP_MAGIC_3;
    
    stOft = bnprintf(pcCdump, LW_CDUMP_MAX_LEN, stOft, 
                     "FATAL ERROR: abort occur in exception mode. "
                     "ret_addr: 0x%08lx abt_addr: 0x%08lx abt_type: %s\n"
                     "rebooting...\n",
                     ulRetAddr, ulAbortAddr, pcInfo);
    
    pcCdump[stOft] = PX_EOS;
}
/*********************************************************************************************************
** 函数名称: _CrashDumpAbortKernel
** 功能描述: 内核崩溃信息记录.
** 输　入  : ulOwner       占用内核的任务
**           pcKernelFunc  进入内核的函数
**           pvCtx         异常信息结构
**           pcInfo        异常信息
**           pcTail        附加信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _CrashDumpAbortKernel (LW_OBJECT_HANDLE   ulOwner, 
                             CPCHAR             pcKernelFunc, 
                             PVOID              pvCtx,
                             CPCHAR             pcInfo, 
                             CPCHAR             pcTail)
{
    PLW_VMM_PAGE_FAIL_CTX  pvmpagefailctx = (PLW_VMM_PAGE_FAIL_CTX)pvCtx;
    PCHAR                  pcCdump        = (PCHAR)_K_pvCrashDumpBuffer;
    size_t                 stOft;
    
    if (!pcCdump || (pcCdump == (PCHAR)PX_ERROR)) {
        return;
    }
    
    lib_bzero(pcCdump, LW_CDUMP_BUF_SIZE);
    
    pcCdump[0] = LW_CDUMP_MAGIC_0;
    pcCdump[1] = LW_CDUMP_MAGIC_1;
    pcCdump[2] = LW_CDUMP_MAGIC_2;
    pcCdump[3] = LW_CDUMP_MAGIC_3;
    
    archTaskCtxPrint(&pcCdump[4], (LW_CDUMP_MAX_LEN - LW_CDUMP_MAGIC_LEN), 
                     (PLW_STACK)pvmpagefailctx->PAGEFCTX_pvStackRet);
                     
    stOft = lib_strlen(pcCdump);
    
    stOft = bnprintf(pcCdump, LW_CDUMP_MAX_LEN, stOft, 
                     "FATAL ERROR: abort in kernel status. "
                     "kowner: 0x%08lx, kfunc: %s, "
                     "ret_addr: 0x%08lx abt_addr: 0x%08lx, abt_type: %s, %s.\n",
                     ulOwner, pcKernelFunc, 
                     pvmpagefailctx->PAGEFCTX_ulRetAddr,
                     pvmpagefailctx->PAGEFCTX_ulAbortAddr, pcInfo, pcTail);
}
/*********************************************************************************************************
** 函数名称: _CrashDumpAbortAccess
** 功能描述: 崩溃信息记录.
** 输　入  : pvCtx            异常信息结构
**           pcInfo           异常信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _CrashDumpAbortAccess (PVOID  pvCtx, CPCHAR  pcInfo)
{
    PLW_VMM_PAGE_FAIL_CTX  pvmpagefailctx = (PLW_VMM_PAGE_FAIL_CTX)pvCtx;
    PCHAR                  pcCdump        = (PCHAR)_K_pvCrashDumpBuffer;
    size_t                 stOft;
    
    if (!pcCdump || (pcCdump == (PCHAR)PX_ERROR)) {
        return;
    }
    
    lib_bzero(pcCdump, LW_CDUMP_BUF_SIZE);
    
    pcCdump[0] = LW_CDUMP_MAGIC_0;
    pcCdump[1] = LW_CDUMP_MAGIC_1;
    pcCdump[2] = LW_CDUMP_MAGIC_2;
    pcCdump[3] = LW_CDUMP_MAGIC_3;
    
    archTaskCtxPrint(&pcCdump[4], (LW_CDUMP_MAX_LEN - LW_CDUMP_MAGIC_LEN), 
                     (PLW_STACK)pvmpagefailctx->PAGEFCTX_pvStackRet);
    
    stOft = lib_strlen(pcCdump);
    
    switch (__PAGEFAILCTX_ABORT_TYPE(pvmpagefailctx)) {
    
    case LW_VMM_ABORT_TYPE_UNDEF:
        stOft = bnprintf(pcCdump, LW_CDUMP_MAX_LEN, stOft, 
                         "UNDEF ERROR: abort in thread %lx[%s]. "
                         "ret_addr: 0x%08lx abt_addr: 0x%08lx, abt_type: %s.\n",
                         pvmpagefailctx->PAGEFCTX_ptcb->TCB_ulId,
                         pvmpagefailctx->PAGEFCTX_ptcb->TCB_cThreadName,
                         pvmpagefailctx->PAGEFCTX_ulRetAddr,
                         pvmpagefailctx->PAGEFCTX_ulAbortAddr, pcInfo);
        break;
    
    case LW_VMM_ABORT_TYPE_FPE:
        stOft = bnprintf(pcCdump, LW_CDUMP_MAX_LEN, stOft, 
                         "FPU ERROR: abort in thread %lx[%s]. "
                         "ret_addr: 0x%08lx abt_addr: 0x%08lx, abt_type: %s.\n",
                         pvmpagefailctx->PAGEFCTX_ptcb->TCB_ulId,
                         pvmpagefailctx->PAGEFCTX_ptcb->TCB_cThreadName,
                         pvmpagefailctx->PAGEFCTX_ulRetAddr,
                         pvmpagefailctx->PAGEFCTX_ulAbortAddr, pcInfo);
        break;
        
    default:
        stOft = bnprintf(pcCdump, LW_CDUMP_MAX_LEN, stOft, 
                         "ACCESS ERROR: abort in thread %lx[%s]. "
                         "ret_addr: 0x%08lx abt_addr: 0x%08lx, abt_type: %s.\n",
                         pvmpagefailctx->PAGEFCTX_ptcb->TCB_ulId,
                         pvmpagefailctx->PAGEFCTX_ptcb->TCB_cThreadName,
                         pvmpagefailctx->PAGEFCTX_ulRetAddr,
                         pvmpagefailctx->PAGEFCTX_ulAbortAddr, pcInfo);
        break;
    }
    
    API_BacktracePrint(&pcCdump[stOft], (LW_CDUMP_MAX_LEN - stOft), LW_CFG_CDUMP_CALL_STACK_DEPTH);
}
/*********************************************************************************************************
** 函数名称: _CrashDumpSet
** 功能描述: 设置系统/应用崩溃信息记录位置.
** 输　入  : pvCdump           缓冲地址
**           stSize            缓冲大小
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _CrashDumpSet (PVOID  pvCdump, size_t  stSize)
{
    _K_pvCrashDumpBuffer = pvCdump;
    _K_stCrashDumpSize   = stSize;
}
/*********************************************************************************************************
** 函数名称: _CrashDumpGet
** 功能描述: 获取系统/应用崩溃信息记录位置.
** 输　入  : pstSize           缓冲大小
** 输　出  : 缓冲地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  _CrashDumpGet (size_t  *pstSize)
{
    *pstSize = _K_stCrashDumpSize;
    
    return  (_K_pvCrashDumpBuffer);
}

#endif                                                                  /*  LW_CFG_CDUMP_EN > 0         */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
