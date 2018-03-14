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
** 文   件   名: procHook.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 04 月 1 日
**
** 描        述: proc 文件系统 hook 信息文件.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/symbol/include/sym_sym.h"
#include "../SylixOS/system/sysHookList/sysHookList.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_PROCFS_EN > 0) && (LW_CFG_PROCFS_HOOK_INFO > 0)
#include "../procFs.h"
/*********************************************************************************************************
  HOOK proc 文件函数声明
*********************************************************************************************************/
static ssize_t  __procFsHookRead(PLW_PROCFS_NODE  p_pfsn, 
                                 PCHAR            pcBuffer, 
                                 size_t           stMaxBytes,
                                 off_t            oft);
/*********************************************************************************************************
  HOOK proc 文件操作函数组
*********************************************************************************************************/
static LW_PROCFS_NODE_OP    _G_pfsnoHookFuncs = {
    __procFsHookRead, LW_NULL
};
/*********************************************************************************************************
  内核 proc 文件目录树
*********************************************************************************************************/
#define __PROCFS_BUFFER_SIZE_HOOK   (64 + (LW_SYS_HOOK_SIZE * 32))

static LW_PROCFS_NODE       _G_pfsnHook[] = 
{
    LW_PROCFS_INIT_NODE("hook",  
                        (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH), 
                        LW_NULL, 
                        LW_NULL,  
                        0),
                        
    LW_PROCFS_INIT_NODE("tcreate", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_THREAD_CREATE_HOOK,
                        __PROCFS_BUFFER_SIZE_HOOK),
                        
    LW_PROCFS_INIT_NODE("tdelete", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_THREAD_DELETE_HOOK,
                        __PROCFS_BUFFER_SIZE_HOOK),
                        
    LW_PROCFS_INIT_NODE("tswap", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_THREAD_SWAP_HOOK,
                        __PROCFS_BUFFER_SIZE_HOOK),
                        
    LW_PROCFS_INIT_NODE("tick", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_THREAD_TICK_HOOK,
                        __PROCFS_BUFFER_SIZE_HOOK),
    
    LW_PROCFS_INIT_NODE("tinit", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_THREAD_INIT_HOOK,
                        __PROCFS_BUFFER_SIZE_HOOK),
    
    LW_PROCFS_INIT_NODE("idle", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_THREAD_IDLE_HOOK,
                        __PROCFS_BUFFER_SIZE_HOOK),
    
    LW_PROCFS_INIT_NODE("init", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_KERNEL_INITEND,
                        __PROCFS_BUFFER_SIZE_HOOK),
    
    LW_PROCFS_INIT_NODE("reboot", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_KERNEL_REBOOT,
                        __PROCFS_BUFFER_SIZE_HOOK),
    
    LW_PROCFS_INIT_NODE("wdtimer", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_WATCHDOG_TIMER,
                        __PROCFS_BUFFER_SIZE_HOOK),
    
    LW_PROCFS_INIT_NODE("objcreate", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_OBJECT_CREATE_HOOK,
                        __PROCFS_BUFFER_SIZE_HOOK),
    
    LW_PROCFS_INIT_NODE("objdelete", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_OBJECT_DELETE_HOOK,
                        __PROCFS_BUFFER_SIZE_HOOK),
    
    LW_PROCFS_INIT_NODE("fdcreate", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_FD_CREATE_HOOK,
                        __PROCFS_BUFFER_SIZE_HOOK),
    
    LW_PROCFS_INIT_NODE("fddelete", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_FD_DELETE_HOOK,
                        __PROCFS_BUFFER_SIZE_HOOK),
    
    LW_PROCFS_INIT_NODE("idleenter", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_CPU_IDLE_ENTER,
                        __PROCFS_BUFFER_SIZE_HOOK),
    
    LW_PROCFS_INIT_NODE("idleexit", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_CPU_IDLE_EXIT,
                        __PROCFS_BUFFER_SIZE_HOOK),
    
    LW_PROCFS_INIT_NODE("intenter", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_CPU_INT_ENTER,
                        __PROCFS_BUFFER_SIZE_HOOK),
    
    LW_PROCFS_INIT_NODE("intexit", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_CPU_INT_EXIT,
                        __PROCFS_BUFFER_SIZE_HOOK),
    
    LW_PROCFS_INIT_NODE("stkoverflow", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_STACK_OVERFLOW_HOOK,
                        __PROCFS_BUFFER_SIZE_HOOK),
    
    LW_PROCFS_INIT_NODE("fatal", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_FATAL_ERROR_HOOK,
                        __PROCFS_BUFFER_SIZE_HOOK),
    
    LW_PROCFS_INIT_NODE("pcreate", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_VPROC_CREATE_HOOK,
                        __PROCFS_BUFFER_SIZE_HOOK),
    
    LW_PROCFS_INIT_NODE("pdelete", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoHookFuncs, 
                        (PVOID)LW_OPTION_VPROC_DELETE_HOOK,
                        __PROCFS_BUFFER_SIZE_HOOK),
};
/*********************************************************************************************************
  遍历参数
*********************************************************************************************************/
typedef struct lw_hook_tprint {
    LW_HOOK_FUNC  pfuncHook;
    BOOL          bInSym;
    PCHAR         pcFileBuffer;
    size_t       *pstRealSize;
} LW_HOOK_TPRINT;
/*********************************************************************************************************
** 函数名称: __procFsHookRead
** 功能描述: 打印一个符号
** 输　入  : pvArg         打印参数
**           psymbol       符号
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SYMBOL_EN > 0

static BOOL  __procFsHookPrint (PVOID  pvArg, PLW_SYMBOL  psymbol)
{
    LW_HOOK_TPRINT  *phooktp = (LW_HOOK_TPRINT *)pvArg;

    if ((caddr_t)phooktp->pfuncHook != psymbol->SYM_pcAddr) {
        return  (LW_FALSE);
    }
    
    *(phooktp->pstRealSize) = bnprintf(phooktp->pcFileBuffer, __PROCFS_BUFFER_SIZE_HOOK, 
                                       *(phooktp->pstRealSize), "%s\n", psymbol->SYM_pcName);
    
    phooktp->bInSym = LW_TRUE;
    
    return  (LW_TRUE);
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
/*********************************************************************************************************
** 函数名称: __procFsHookRead
** 功能描述: procfs 读一个内核 version proc 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsHookRead (PLW_PROCFS_NODE  p_pfsn, 
                                  PCHAR            pcBuffer, 
                                  size_t           stMaxBytes,
                                  off_t            oft)
{
    const    CHAR       cHookInfoHdr[] = 
    "         FUNCTION\n"
    "--------------------------\n";
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  实际的文件内容大小          */
             size_t     stCopeBytes;

    /*
     *  程序运行到这里, 文件缓冲一定已经分配了预置的内存大小(创建节点时预置大小为 64 字节).
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  需要生成文件                */
        INT             i;
        PLW_HOOK_CB     phookcb;
        LW_HOOK_TPRINT  hooktp;
        
        switch ((ULONG)p_pfsn->PFSN_pfsnmMessage.PFSNM_pvValue) {
        
        case LW_OPTION_THREAD_CREATE_HOOK:
            phookcb = HOOK_T_CREATE;
            break;
            
        case LW_OPTION_THREAD_DELETE_HOOK:
            phookcb = HOOK_T_DELETE;
            break;
        
        case LW_OPTION_THREAD_SWAP_HOOK:
            phookcb = HOOK_T_SWAP;
            break;
        
        case LW_OPTION_THREAD_TICK_HOOK:
            phookcb = HOOK_T_TICK;
            break;
        
        case LW_OPTION_THREAD_INIT_HOOK:
            phookcb = HOOK_T_INIT;
            break;
        
        case LW_OPTION_THREAD_IDLE_HOOK:
            phookcb = HOOK_T_IDLE;
            break;
        
        case LW_OPTION_KERNEL_INITEND:
            phookcb = HOOK_T_INITEND;
            break;
        
        case LW_OPTION_KERNEL_REBOOT:
            phookcb = HOOK_T_REBOOT;
            break;
        
        case LW_OPTION_WATCHDOG_TIMER:
            phookcb = HOOK_T_WATCHDOG;
            break;
        
        case LW_OPTION_OBJECT_CREATE_HOOK:
            phookcb = HOOK_T_OBJCREATE;
            break;
        
        case LW_OPTION_OBJECT_DELETE_HOOK:
            phookcb = HOOK_T_OBJDELETE;
            break;
        
        case LW_OPTION_FD_CREATE_HOOK:
            phookcb = HOOK_T_FDCREATE;
            break;
        
        case LW_OPTION_FD_DELETE_HOOK:
            phookcb = HOOK_T_FDDELETE;
            break;
        
        case LW_OPTION_CPU_IDLE_ENTER:
            phookcb = HOOK_T_IDLEENTER;
            break;
        
        case LW_OPTION_CPU_IDLE_EXIT:
            phookcb = HOOK_T_IDLEEXIT;
            break;
        
        case LW_OPTION_CPU_INT_ENTER:
            phookcb = HOOK_T_INTENTER;
            break;
        
        case LW_OPTION_CPU_INT_EXIT:
            phookcb = HOOK_T_INTEXIT;
            break;
        
        case LW_OPTION_STACK_OVERFLOW_HOOK:
            phookcb = HOOK_T_STKOF;
            break;
        
        case LW_OPTION_FATAL_ERROR_HOOK:
            phookcb = HOOK_T_FATALERR;
            break;
        
        case LW_OPTION_VPROC_CREATE_HOOK:
            phookcb = HOOK_T_VPCREATE;
            break;
        
        case LW_OPTION_VPROC_DELETE_HOOK:
            phookcb = HOOK_T_VPDELETE;
            break;
        
        default:
            phookcb = LW_NULL;
            break;
        }
        
        if (phookcb) {
            hooktp.pcFileBuffer = pcFileBuffer;
            hooktp.pstRealSize  = &stRealSize;
        
            stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_HOOK, 0, cHookInfoHdr); 
                                                                        /*  打印头信息                  */
            for (i = (LW_SYS_HOOK_SIZE - 1); i >= 0; i--) {
                INTREG  iregInterLevel;
                
                LW_SPIN_LOCK_QUICK(&phookcb->HOOKCB_slHook, &iregInterLevel);
                if (phookcb->HOOKCB_pfuncnode[i]) {
                    hooktp.pfuncHook = phookcb->HOOKCB_pfuncnode[i]->FUNCNODE_hookfunc;
                } else {
                    hooktp.pfuncHook = LW_NULL;
                }
                LW_SPIN_UNLOCK_QUICK(&phookcb->HOOKCB_slHook, iregInterLevel);
                
                if (hooktp.pfuncHook) {
                    hooktp.bInSym = LW_FALSE;
                    
#if LW_CFG_SYMBOL_EN > 0
                    API_SymbolTraverse(__procFsHookPrint, &hooktp);
#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
                    if (hooktp.bInSym == LW_FALSE) {
                        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_HOOK, stRealSize,
                                              "0x%08lx\n", (addr_t)hooktp.pfuncHook);
                    }
                }
            }
        } else {
            stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_HOOK, 0, "hook option error!\n");
        }
                                                                        
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  计算实际拷贝的字节数        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** 函数名称: __procFsHookInit
** 功能描述: procfs 初始化 hook proc 文件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __procFsHookInit (VOID)
{
    INT     i;
    
    API_ProcFsMakeNode(&_G_pfsnHook[0],  "/");
    
    for (i = 1; 
         i < (sizeof(_G_pfsnHook) / sizeof(LW_PROCFS_NODE)); 
         i++) {
        API_ProcFsMakeNode(&_G_pfsnHook[i],  "/hook");
    }
}

#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
                                                                        /*  LW_CFG_PROCFS_HOOK_INFO     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
