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
** 文   件   名: ioLockF.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 01 月 04 日
**
** 描        述: IO 系统对文件记录锁的支持, 文件记录锁仅支持 NEW_1 型或更高版本的驱动程序类型.
**               这里提供的 API 函数都是供驱动程序使用的. (本程序算法来自 BSD)
**               SylixOS 提供的文件锁与 POSIX 系统兼容, 都叫做建议性锁, 对于这一种锁来说，内核只提供加锁
**               以及检测文件是否已经加锁的手段，但是内核并不参与锁的控制和协调。也就是说，如果有进程
**               不遵守“游戏规则”，不检查目标文件是否已经由别的进程加了锁就往其中写入数据，那么内核是
**               不会加以阻拦的。因此，劝告锁并不能阻止进程对文件的访问，而只能依靠各个进程在访问文件之前
**               检查该文件是否已经被其他进程加锁来实现并发控制。进程需要事先对锁的状态做一个约定，
**               并根据锁的当前状态和相互关系来确定其他进程是否能对文件执行指定的操作。从这点上来说，
**               劝告锁的工作方式与使用信号量保护临界区的方式非常类似。
**
** BUG:
2013.07.17  增加了专用的释放接口, 使文件关闭更加快捷.
2013.08.14  支持任务在等待记录锁时被删除.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  说明:
     SylixOS IO 系统驱动程序早期是为兼容 VxWorks 驱动程序设计的, 后来经过二次升级后开始设计了
     新的驱动程序模型, 文件记录锁只能用于新的驱动模型. 安装驱动时通过选项来指明是使用 VxWorks
     兼容设备驱动还是 NEW_1 型或更高版本的驱动程序类型.
     新驱动模型相比原始驱动模型加入了一层文件节点结构, 它类似于 UNIX 兼容系统的 vnode 结构, 
     它代表了在 SylixOS 操作系统中唯一的一个文件实体, 它通过 dev_t 和 ino_t 来描述, 不管一个
     文件被打开了多少次, 系统内核中就只存在与之对应唯一的一个文件节点, 所有对该文件的记录锁全
     部保存在这个结构中. 每一次打开文件生成的 fd_entry 都保存一个自己的读写指针, 每次对同一
     文件的操作, 驱动程序必须使用此指针作为当前指针. 驱动程序 open 和 creat 的返回值就保存在
     fd_entry 的 FDENTRY_lValue 指针中, 之后操作系统传递给驱动程序的第一个参数即为 fd_entry 
     驱动相关信息需要驱动程序自己从 FDENTRY_lValue 指针中获取. 这里需要说明的是:FDENTRY_lValue
     指针必须可以强制转换为文件节点类型(fd_node), 这样才能确保操作系统有效的处理文件记录锁.
     对应 NEW_1 型驱动程序结构可以参考 romfs yaffs nfs fat.
*********************************************************************************************************/
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
#include "sys/fcntl.h"
/*********************************************************************************************************
  是否打印调试信息
*********************************************************************************************************/
#define DEBUG_PRINT_EN      0
/*********************************************************************************************************
  Maximum length of sleep chains to traverse to try and detect deadlock.
*********************************************************************************************************/
#define MAXDEPTH            50
/*********************************************************************************************************
  parameters to __fdLockFindOverlap()
*********************************************************************************************************/
#define SELF                0x1
#define OTHERS              0x2
/*********************************************************************************************************
  任务在阻塞时被删除, 这里记录任务需要释放的资源
*********************************************************************************************************/
typedef struct {
    PLW_FD_LOCKF             LFC_pfdlockf;
    PLW_FD_LOCKF             LFC_pfdlockfSpare;
    PLW_FD_NODE              LFC_pfdnode;
} LW_LOCKF_CLEANUP;
typedef LW_LOCKF_CLEANUP    *PLW_LOCKF_CLEANUP;
static LW_LOCKF_CLEANUP      _G_lfcTable[LW_CFG_MAX_THREADS];
/*********************************************************************************************************
** 函数名称: __fdLockfPrint
** 功能描述: 打印一个 lock file 节点.
** 输　入  : pcMsg         前缀信息
**           pfdlockf      lock file 节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __fdLockfPrint (CPCHAR  pcMsg, PLW_FD_LOCKF    pfdlockf)
{
#if DEBUG_PRINT_EN > 0
    PLW_FD_LOCKF    pfdlockfBlock;

    if (!pfdlockf) {
        printf(pcMsg);
        return;
    }

    printf("%s: lock %p for ", pcMsg, pfdlockf);
    printf("proc %d", pfdlockf->FDLOCK_pid);
    printf(", %s, start %llx, end %llx",
           pfdlockf->FDLOCK_usType == F_RDLCK ? "shared" :
           pfdlockf->FDLOCK_usType == F_WRLCK ? "exclusive" :
           pfdlockf->FDLOCK_usType == F_UNLCK ? "unlock" :
           "unknown", pfdlockf->FDLOCK_oftStart, pfdlockf->FDLOCK_oftEnd);
    if (pfdlockf->FDLOCK_plineBlockHd) {
        pfdlockfBlock = _LIST_ENTRY(pfdlockf->FDLOCK_plineBlockHd, LW_FD_LOCKF, FDLOCK_lineBlock);
        printf(" block %p\n", pfdlockfBlock);
    } else {
        printf("\n");
    }
#endif                                                                  /*  DEBUG_PRINT_EN              */
}
/*********************************************************************************************************
** 函数名称: __fdLockfPrintList
** 功能描述: 打印一个 lock file 序列.
** 输　入  : pcMsg         前缀信息
**           pfdlockf      lock file 节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __fdLockfPrintList (CPCHAR  pcMsg, PLW_FD_LOCKF    pfdlockf)
{
#if DEBUG_PRINT_EN > 0
    PLW_LIST_LINE   plineTmp;
    PLW_FD_LOCKF    pfdlockfTmp, pfdlockfBlock;
	PCHAR           pcOpen, pcClose;

    printf("%s: lock list:\n", pcMsg);
    for (pfdlockfTmp  = *pfdlockf->FDLOCK_pfdlockHead; 
         pfdlockfTmp != LW_NULL; 
         pfdlockfTmp  = pfdlockfTmp->FDLOCK_pfdlockNext) {
         
        printf("\tlock %p for ", pfdlockfTmp);
        printf("proc %d", pfdlockfTmp->FDLOCK_pid);
        
        printf(", %s, start %llx, end %llx",
               pfdlockfTmp->FDLOCK_usType == F_RDLCK ? "shared" :
               pfdlockfTmp->FDLOCK_usType == F_WRLCK ? "exclusive" :
               pfdlockfTmp->FDLOCK_usType == F_UNLCK ? "unlock" :
               "unknown", pfdlockfTmp->FDLOCK_oftStart, pfdlockfTmp->FDLOCK_oftEnd);
           
        pcOpen = " is blocking { ";
        pcClose = "";
        
        for (plineTmp  = pfdlockfTmp->FDLOCK_plineBlockHd;
             plineTmp != LW_NULL;
             plineTmp  = _list_line_get_next(plineTmp)) {
             
            pfdlockfBlock = _LIST_ENTRY(plineTmp, LW_FD_LOCKF, FDLOCK_lineBlock);
            
            printf("%s", pcOpen);
            pcOpen = ", ";
            pcClose = " }";
            
            printf("proc %d", pfdlockfBlock->FDLOCK_pid);
            
            printf(", %s, start %llx, end %llx",
                   pfdlockfBlock->FDLOCK_usType == F_RDLCK ? "shared" :
                   pfdlockfBlock->FDLOCK_usType == F_WRLCK ? "exclusive" :
                   pfdlockfBlock->FDLOCK_usType == F_UNLCK ? "unlock" :
                   "unknown", pfdlockfBlock->FDLOCK_oftStart, pfdlockfBlock->FDLOCK_oftEnd);
        }
        printf("%s\n", pcClose);
    }
#endif                                                                  /*  DEBUG_PRINT_EN              */
}
/*********************************************************************************************************
** 函数名称: __fdLockfCreate
** 功能描述: 创建一个 lock file 节点.
** 输　入  : NONE
** 输　出  : 创建的新 lock file 节点
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_FD_LOCKF  __fdLockfCreate (VOID)
{
    PLW_FD_LOCKF    pfdlockf;
    
    pfdlockf = (PLW_FD_LOCKF)__SHEAP_ALLOC(sizeof(LW_FD_LOCKF));
    if (pfdlockf == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    lib_bzero(pfdlockf, sizeof(LW_FD_LOCKF));
    
    pfdlockf->FDLOCK_ulBlock = API_SemaphoreBCreate("lockf_lock", LW_FALSE, 
                                                    LW_OPTION_OBJECT_GLOBAL,
                                                    LW_NULL);
    if (pfdlockf->FDLOCK_ulBlock == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pfdlockf);
        return  (LW_NULL);
    }
    
    return  (pfdlockf);
}
/*********************************************************************************************************
** 函数名称: __fdLockfDelete
** 功能描述: 删除一个 lock file 节点.
** 输　入  : pfdlockf      lock file 节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __fdLockfDelete (PLW_FD_LOCKF    pfdlockf)
{
    if (pfdlockf) {
        API_SemaphoreBDelete(&pfdlockf->FDLOCK_ulBlock);
        __SHEAP_FREE(pfdlockf);
    }
}
/*********************************************************************************************************
** 函数名称: __fdLockfBlock
** 功能描述: 将一个 lock file 节点放入阻塞链表.
** 输　入  : pfdlockfWaiter    当前需要阻塞的 lock file 节点
**           pfdlockfBlocker   阻塞者 (Waiter 就是由于这个节点才阻塞)
**           pfdnode           文件节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __fdLockfBlock (PLW_FD_LOCKF    pfdlockfWaiter, 
                             PLW_FD_LOCKF    pfdlockfBlocker, 
                             PLW_FD_NODE     pfdnode)
{
    _List_Line_Add_Tail(&pfdlockfWaiter->FDLOCK_lineBlockQ,
                        &pfdnode->FDNODE_plineBlockQ);                  /*  加入 fd_node 锁阻塞队列     */
                        
    _List_Line_Add_Tail(&pfdlockfWaiter->FDLOCK_lineBlock,
                        &pfdlockfBlocker->FDLOCK_plineBlockHd);         /*  加入 blocker 的阻塞表       */
                        
    pfdlockfWaiter->FDLOCK_pfdlockNext = pfdlockfBlocker;
}
/*********************************************************************************************************
** 函数名称: __fdLockfDUnblock
** 功能描述: 将一个 lock file 节点从阻塞链表中退出.
** 输　入  : pfdlockfWaiter    当前需要阻塞的 lock file 节点
**           pfdlockfBlocker   阻塞者 (Waiter 就是由于这个节点才阻塞)
**           pfdnode           文件节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __fdLockfUnblock (PLW_FD_LOCKF    pfdlockfWaiter, 
                               PLW_FD_LOCKF    pfdlockfBlocker,
                               PLW_FD_NODE     pfdnode)
{
    pfdlockfWaiter->FDLOCK_pfdlockNext = LW_NULL;
    
    _List_Line_Del(&pfdlockfWaiter->FDLOCK_lineBlock,
                   &pfdlockfBlocker->FDLOCK_plineBlockHd);
                   
    _List_Line_Del(&pfdlockfWaiter->FDLOCK_lineBlockQ,
                   &pfdnode->FDNODE_plineBlockQ);
}
/*********************************************************************************************************
** 函数名称: __fdLockfFindBlock
** 功能描述: 查询一个指定进程的 lock file 正在阻塞的节点.
** 输　入  : pid               进程 id
**           pfdnode           文件节点
** 输　出  : 查询到正在被阻塞的锁
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_FD_LOCKF  __fdLockfFindBlock (pid_t  pid, PLW_FD_NODE  pfdnode)
{
    PLW_LIST_LINE   plineTemp;
    PLW_FD_LOCKF    pfdlockfWaiter;
    
    for (plineTemp  = pfdnode->FDNODE_plineBlockQ;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  遍历 fd_node 所有阻塞的锁   */
        
        pfdlockfWaiter = _LIST_ENTRY(plineTemp, LW_FD_LOCKF, FDLOCK_lineBlockQ);
        if (pfdlockfWaiter->FDLOCK_pid == pid) {
            break;
        }
    }
    
    if (plineTemp != LW_NULL) {
        return  (pfdlockfWaiter);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: __fdLockfCleanupSet
** 功能描述: 设置当前线程清除 lockf 信息, 线程被杀死时将会自动调用清除函数.
** 输　入  : pfdlockf          新加锁的区域
**           pfdlockfSpare     可能产生的分离区域
**           pfdnode           文件节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __fdLockfCleanupSet (PLW_FD_LOCKF pfdlockf, 
                                  PLW_FD_LOCKF pfdlockfSpare, 
                                  PLW_FD_NODE  pfdnode)
{
    PLW_CLASS_TCB       ptcbCur;
    PLW_LOCKF_CLEANUP   plfc;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    plfc = &_G_lfcTable[ptcbCur->TCB_usIndex];
    
    plfc->LFC_pfdlockf      = pfdlockf;
    plfc->LFC_pfdlockfSpare = pfdlockfSpare;
    plfc->LFC_pfdnode       = pfdnode;
}
/*********************************************************************************************************
** 函数名称: __fdLockfCleanupHook
** 功能描述: 线程被杀死时将会自动调用清除函数.
** 输　入  : NONE
** 输　出  : 创建的新 lock file 节点
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __fdLockfCleanupHook (PLW_CLASS_TCB  ptcbDel)
{
    PLW_LOCKF_CLEANUP   plfc;
    PLW_FD_LOCKF        pfdlockfBlock;

    plfc = &_G_lfcTable[ptcbDel->TCB_usIndex];
    
    if (plfc->LFC_pfdlockf == LW_NULL) {
        return;
    }
    
    if (API_SemaphoreBPend(plfc->LFC_pfdnode->FDNODE_ulSem, LW_OPTION_WAIT_INFINITE)) {
        goto    __out;
    } else {
        /*
         * Is blocking ?
         */
        if ((pfdlockfBlock = plfc->LFC_pfdlockf->FDLOCK_pfdlockNext) != LW_NULL) {
            __fdLockfUnblock(plfc->LFC_pfdlockf, pfdlockfBlock, plfc->LFC_pfdnode);
        }
        API_SemaphoreBPost(plfc->LFC_pfdnode->FDNODE_ulSem);
    }
    
__out:
    __fdLockfDelete(plfc->LFC_pfdlockf);
    if (plfc->LFC_pfdlockfSpare) {
        __fdLockfDelete(plfc->LFC_pfdlockfSpare);
    }
    
    plfc->LFC_pfdlockf      = LW_NULL;
    plfc->LFC_pfdlockfSpare = LW_NULL;
    plfc->LFC_pfdnode       = LW_NULL;
}
/*********************************************************************************************************
** 函数名称: __fdLockfIsDeadLock
** 功能描述: 文件锁死锁检测
** 输　入  : pfdlockfWaiter    当前需要阻塞的 lock file 节点
**           pfdlockfProp      建议节点
**           pfdnode           文件节点
** 输　出  : 是否存在死锁
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  __fdLockfIsDeadLock (PLW_FD_LOCKF    pfdlockfWaiter,
                                  PLW_FD_LOCKF    pfdlockfProp,
                                  PLW_FD_NODE     pfdnode)
{
    INT             i = 0;
    PLW_FD_LOCKF    pfdlockf;
    PLW_FD_LOCKF    pfdlockfW;
    
    do {
        pfdlockf = __fdLockfFindBlock(pfdlockfProp->FDLOCK_pid, pfdnode);
        if (pfdlockf == LW_NULL) {
            break;
        }
        pfdlockfW = pfdlockf->FDLOCK_pfdlockNext;
        if (pfdlockfW == LW_NULL) {
            break;                                                      /*  不可能发生                  */
        }
        if (pfdlockfW->FDLOCK_pid == pfdlockfWaiter->FDLOCK_pid) {      /*  已经阻塞的点与Waiter同一进程*/
            return  (LW_TRUE);
        }
        if (++i >= MAXDEPTH) {
            return  (LW_TRUE);
        }
        pfdlockfProp = pfdlockfW;
    } while (1);
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: __fdLockfWakeup
** 功能描述: 唤醒阻塞的节点队列
** 输　入  : pfdlockfBlocker   阻塞者 (释放由于这个节点才阻塞的节点)
**           pfdnode           文件节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __fdLockfWakeup (PLW_FD_LOCKF  pfdlockfBlocker, PLW_FD_NODE  pfdnode)
{
    PLW_LIST_LINE   plineTemp;
    PLW_FD_LOCKF    pfdlockfWake;
    
    plineTemp = pfdlockfBlocker->FDLOCK_plineBlockHd;
    while (plineTemp) {
        pfdlockfWake = _LIST_ENTRY(plineTemp, LW_FD_LOCKF, FDLOCK_lineBlock);
        plineTemp    = _list_line_get_next(plineTemp);
        
        __fdLockfUnblock(pfdlockfWake, pfdlockfBlocker, pfdnode);
        
        __fdLockfPrint("wakelock: awakening", pfdlockfWake);
        
        API_SemaphoreBPost(pfdlockfWake->FDLOCK_ulBlock);
    }
}
/*********************************************************************************************************
** 函数名称: __fdLockfSplit
** 功能描述: 拆分锁
** 输　入  : pfdlockf1             大或者小分区
**           pfdlockf2             大或者小分区
**           ppfdlockfSpare        拆分分区信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __fdLockfSplit (PLW_FD_LOCKF  pfdlockf1, 
                             PLW_FD_LOCKF  pfdlockf2, 
                             PLW_FD_LOCKF *ppfdlockfSpare)
{
    PLW_FD_LOCKF  pfdlockfSplit;
    
    __fdLockfPrint("split", pfdlockf1);
    __fdLockfPrint("splitting from", pfdlockf2);
    
    if (pfdlockf1->FDLOCK_oftStart == pfdlockf2->FDLOCK_oftStart) {
        pfdlockf1->FDLOCK_oftStart =  pfdlockf2->FDLOCK_oftEnd + 1;
        pfdlockf2->FDLOCK_pfdlockNext = pfdlockf1;
        return;
    }
    if (pfdlockf1->FDLOCK_oftEnd == pfdlockf2->FDLOCK_oftEnd) {
        pfdlockf1->FDLOCK_oftEnd =  pfdlockf2->FDLOCK_oftStart - 1;
        pfdlockf2->FDLOCK_pfdlockNext = pfdlockf1->FDLOCK_pfdlockNext;
        pfdlockf1->FDLOCK_pfdlockNext = pfdlockf2;
        return;
    }
    
    /*
     * Make a new lock consisting of the last part of
     * the encompassing lock
     */
    pfdlockfSplit = *ppfdlockfSpare;
    *ppfdlockfSpare = LW_NULL;                                          /*  已经使用了                  */
    lib_memcpy(pfdlockfSplit, pfdlockf1, sizeof(*pfdlockfSplit));
    pfdlockfSplit->FDLOCK_oftStart = pfdlockf2->FDLOCK_oftEnd + 1;
    pfdlockfSplit->FDLOCK_plineBlockHd = LW_NULL;
    pfdlockf1->FDLOCK_oftEnd = pfdlockf2->FDLOCK_oftStart - 1;
    
    /*
     * OK, now link it in
     */
    pfdlockfSplit->FDLOCK_pfdlockNext = pfdlockf1->FDLOCK_pfdlockNext;
    pfdlockf2->FDLOCK_pfdlockNext     = pfdlockfSplit;
    pfdlockf1->FDLOCK_pfdlockNext     = pfdlockf2;
}
/*********************************************************************************************************
** 函数名称: __fdLockfFindOverlap
** 功能描述: 查找与指定范围锁相互的重叠区域 (只寻找第一个重叠区域)
** 输　入  : pfdlockf           从这个锁区域开始查找
**           pfdlockLock        指定的锁范围
**           iType              SELF / OTHERS
**           pppfdlockfPrev     保存 prev
**           ppfdlockfOverlap   如果存在重叠区域则保存 
** 输　出  : 0) no overlap
**           1) overlap == lock
**           2) overlap contains lock
**           3) lock contains overlap
**           4) overlap starts before lock
**           5) overlap ends after lock
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fdLockfFindOverlap (PLW_FD_LOCKF pfdlockf, PLW_FD_LOCKF pfdlockLock, INT  iType,
                                  PLW_FD_LOCKF **pppfdlockfPrev, PLW_FD_LOCKF *ppfdlockfOverlap)
{
    off_t   oftStart, oftEnd;
    
    *ppfdlockfOverlap = pfdlockf;
    
    if (pfdlockf == LW_NULL) {
        return  (0);
    }
    
    __fdLockfPrint("findoverlap: looking for overlap in", pfdlockLock);
    
    oftStart = pfdlockLock->FDLOCK_oftStart;
    oftEnd   = pfdlockLock->FDLOCK_oftEnd;
    
    while (pfdlockf != NULL) {
        if (((iType == SELF) && pfdlockf->FDLOCK_pid != pfdlockLock->FDLOCK_pid) ||
            ((iType == OTHERS) && pfdlockf->FDLOCK_pid == pfdlockLock->FDLOCK_pid)) {
            *pppfdlockfPrev = &pfdlockf->FDLOCK_pfdlockNext;
            *ppfdlockfOverlap = pfdlockf = pfdlockf->FDLOCK_pfdlockNext;
            continue;
        }
        
        __fdLockfPrint("\tchecking", pfdlockf);
        /*
         * OK, check for overlap
         *
         * Six cases:
         *    0) no overlap
         *    1) overlap == lock
         *    2) overlap contains lock
         *    3) lock contains overlap
         *    4) overlap starts before lock
         *    5) overlap ends after lock
         */
        if ((pfdlockf->FDLOCK_oftEnd != -1 && oftStart > pfdlockf->FDLOCK_oftEnd) ||
            (oftEnd != -1 && pfdlockf->FDLOCK_oftStart > oftEnd)) {     /*  case 0                      */
            __fdLockfPrint("no overlap\n", LW_NULL);
            if ((iType & SELF) && oftEnd != -1 && pfdlockf->FDLOCK_oftStart > oftEnd) {
                return  (0);
            }
            *pppfdlockfPrev = &pfdlockf->FDLOCK_pfdlockNext;
            *ppfdlockfOverlap = pfdlockf = pfdlockf->FDLOCK_pfdlockNext;
            continue;
        }
        
        if ((pfdlockf->FDLOCK_oftStart == oftStart) && 
            (pfdlockf->FDLOCK_oftEnd == oftEnd)) {                      /*  case 1                      */
            __fdLockfPrint("overlap == lock\n", LW_NULL);
            return  (1);
        }
        
        if ((pfdlockf->FDLOCK_oftStart <= oftStart) &&
            (oftEnd != -1) &&
            ((pfdlockf->FDLOCK_oftEnd >= oftEnd) || 
             (pfdlockf->FDLOCK_oftEnd == -1))) {                        /*  case 2                      */
            __fdLockfPrint("overlap contains lock\n", LW_NULL);
            return  (2);    
        }
        
        if (oftStart <= pfdlockf->FDLOCK_oftStart &&
            (oftEnd == -1 ||
            (pfdlockf->FDLOCK_oftEnd != -1 && 
             oftEnd >= pfdlockf->FDLOCK_oftEnd))) {                     /*  case 3                      */
            __fdLockfPrint("lock contains overlap\n", LW_NULL);
            return  (3);
        }
        
        if ((pfdlockf->FDLOCK_oftStart < oftStart) &&
            ((pfdlockf->FDLOCK_oftEnd >= oftStart) || 
             (pfdlockf->FDLOCK_oftEnd == -1))) {                        /*  case 4                      */
            __fdLockfPrint("overlap starts before lock\n", LW_NULL);
            return  (4);
        }
        
        if ((pfdlockf->FDLOCK_oftStart > oftStart) &&
            (oftEnd != -1) &&
            ((pfdlockf->FDLOCK_oftEnd > oftEnd) || 
             (pfdlockf->FDLOCK_oftEnd == -1))) {                        /*  case 5                      */
            __fdLockfPrint("overlap ends after lock\n", LW_NULL);
            return  (5);
        }
        /*
         *  不可能运行到这里
         */
    }
    
    return  (0);
}
/*********************************************************************************************************
** 函数名称: __fdLockfGetBlock
** 功能描述: 遍历记录锁表, 检查是否存在一把锁, 它将阻塞 pfdlockfLock 指定的锁范围
** 输　入  : pfdlockfLock          指定的锁范围
** 输　出  : 如果存在这样的锁则返回
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_FD_LOCKF  __fdLockfGetBlock (PLW_FD_LOCKF pfdlockfLock)
{
    PLW_FD_LOCKF *ppfdlockfPrev, pfdlockfOverlap, pfdlockf = *(pfdlockfLock->FDLOCK_ppfdlockHead);
    
    ppfdlockfPrev = pfdlockfLock->FDLOCK_ppfdlockHead;
    while (__fdLockfFindOverlap(pfdlockf, pfdlockfLock, OTHERS, &ppfdlockfPrev, &pfdlockfOverlap) != 0) {
        /*
         * We've found an overlap, see if it blocks us
         */
        if ((pfdlockfLock->FDLOCK_usType == F_WRLCK || pfdlockfOverlap->FDLOCK_usType == F_WRLCK)) {
            return  (pfdlockfOverlap);
        }
        /*
         * Nope, point to the next one on the list and
         * see if it blocks us
         */
        pfdlockf = pfdlockfOverlap->FDLOCK_pfdlockNext;
    }
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __fdLockfGetLock
** 功能描述: 检查当前是否存在一个与 pfdlockfLock 指定的空间存在重叠的锁, 
** 输　入  : pfdlockfLock          指定的空间范围
**           pfl                   如果有重叠, 则填写此信息
** 输　出  : ERRNO
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fdLockfGetLock (PLW_FD_LOCKF pfdlockfLock, struct flock *pfl)
{
    PLW_FD_LOCKF pfdlockfBlock;
    
    if ((pfdlockfBlock = __fdLockfGetBlock(pfdlockfLock)) != LW_NULL) {
        pfl->l_type   = pfdlockfBlock->FDLOCK_usType;
        pfl->l_whence = SEEK_SET;
        pfl->l_start  = pfdlockfBlock->FDLOCK_oftStart;
        if (pfdlockfBlock->FDLOCK_oftEnd == -1) {
            pfl->l_len = 0;
        } else {
            pfl->l_len = pfdlockfBlock->FDLOCK_oftEnd - pfdlockfBlock->FDLOCK_oftStart + 1;
        }
        if (pfdlockfBlock->FDLOCK_usFlags & F_POSIX) {
            pfl->l_pid = pfdlockfBlock->FDLOCK_pid;
        } else {
            pfl->l_pid = -1;
        }
    } else {
        pfl->l_type = F_UNLCK;
    }
    return  (0);
}
/*********************************************************************************************************
** 函数名称: __fdLockfClearLock
** 功能描述: 从一个文件节点中移除一个文件锁
** 输　入  : pfdlockfUnlock        被移除的文件锁
**           ppfdlockfSpare        如果产生新的分段, 保存在此
**           pfdnode               文件节点
** 输　出  : ERRNO
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fdLockfClearLock (PLW_FD_LOCKF  pfdlockfUnlock, 
                                PLW_FD_LOCKF *ppfdlockfSpare, 
                                PLW_FD_NODE   pfdnode)
{
    PLW_FD_LOCKF *ppfdlockfHead = pfdlockfUnlock->FDLOCK_ppfdlockHead;
    PLW_FD_LOCKF  pfdlockf = *ppfdlockfHead;
    PLW_FD_LOCKF  pfdlockfOverlap, *ppfdlockfPrev;
    INT           iOvcase;

    if (pfdlockf == NULL) {
        return  (0);
    }
    
    __fdLockfPrint("clearlock", pfdlockfUnlock);
    
    ppfdlockfPrev = ppfdlockfHead;
    
    while ((iOvcase = __fdLockfFindOverlap(pfdlockf, pfdlockfUnlock, SELF,
                                           &ppfdlockfPrev, &pfdlockfOverlap)) != 0) {
        /*
         * Wakeup the list of locks to be retried.
         */
        __fdLockfWakeup(pfdlockfOverlap, pfdnode);
        
        switch (iOvcase) {
        
        case 1:                                                         /* overlap == lock              */
            *ppfdlockfPrev = pfdlockfOverlap->FDLOCK_pfdlockNext;
            __fdLockfDelete(pfdlockfOverlap);
            break;
        
        case 2:                                                         /* overlap contains lock: split */
            if (pfdlockfOverlap->FDLOCK_oftStart == pfdlockfUnlock->FDLOCK_oftStart) {
                pfdlockfOverlap->FDLOCK_oftStart = pfdlockfUnlock->FDLOCK_oftEnd + 1;
                break;
            }
            __fdLockfSplit(pfdlockfOverlap, pfdlockfUnlock, ppfdlockfSpare);
            pfdlockfOverlap->FDLOCK_pfdlockNext = pfdlockfUnlock->FDLOCK_pfdlockNext;
            break;
        
        case 3:                                                         /* lock contains overlap        */
            *ppfdlockfPrev = pfdlockfOverlap->FDLOCK_pfdlockNext;
            pfdlockf = pfdlockfOverlap->FDLOCK_pfdlockNext;
            __fdLockfDelete(pfdlockfOverlap);
            continue;                                                   /* next loop                    */
        
        case 4:                                                         /* overlap starts before lock   */
            pfdlockfOverlap->FDLOCK_oftEnd = pfdlockfUnlock->FDLOCK_oftStart - 1;
            ppfdlockfPrev = &pfdlockfOverlap->FDLOCK_pfdlockNext;
            pfdlockf = pfdlockfOverlap->FDLOCK_pfdlockNext;
            continue;                                                   /* next loop                    */
        
        case 5:                                                         /* overlap ends after lock      */
            pfdlockfOverlap->FDLOCK_oftStart = pfdlockfUnlock->FDLOCK_oftEnd + 1;
            break;
        }
        
        break;
    }
    
    __fdLockfPrintList("clearlock", pfdlockfUnlock);
    
    return  (0);
}
/*********************************************************************************************************
** 函数名称: __fdLockfSetLock
** 功能描述: 设置一个记录锁
** 输　入  : pfdlockfLock      新的记录锁
**           ppfdlockfSpare    如果产生新的区域则保存在此
**           pfdnode           文件节点
** 输　出  : ERRNO
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fdLockfSetLock (PLW_FD_LOCKF  pfdlockfLock, 
                              PLW_FD_LOCKF *ppfdlockfSpare, 
                              PLW_FD_NODE   pfdnode)
{
    PLW_FD_LOCKF  pfdlockfBlock;
    PLW_FD_LOCKF *ppfdlockfPrev, pfdlockfOverlap, pfdlockfTmp;
    INT           iOvcase, iNeedToLink, iError = 0;
    
    __fdLockfPrint("setlock", pfdlockfLock);
    
    /*
     * Scan lock list for this file looking for locks that would block us.
     */
    while ((pfdlockfBlock = __fdLockfGetBlock(pfdlockfLock)) != NULL) {
        /*
         * Free the structure and return if nonblocking.
         */
        if ((pfdlockfLock->FDLOCK_usFlags & F_WAIT) == 0) {
            __fdLockfDelete(pfdlockfLock);
            return  (EAGAIN);
        }
        /*
         * We are blocked. Since flock style locks cover
         * the whole file, there is no chance for deadlock.
         * For byte-range locks we must check for deadlock.
         */
        if ((pfdlockfLock->FDLOCK_usFlags & F_POSIX) &&
            (pfdlockfBlock->FDLOCK_usFlags & F_POSIX)) {
            if (__fdLockfIsDeadLock(pfdlockfLock, pfdlockfBlock, pfdnode)) {
                __fdLockfDelete(pfdlockfLock);
                return  (EDEADLK);
            }
        }
        /*
         * For flock type locks, we must first remove
         * any shared locks that we hold before we sleep
         * waiting for an exclusive lock.
         */
        if ((pfdlockfLock->FDLOCK_usFlags & F_FLOCK) &&
            pfdlockfLock->FDLOCK_usType == F_WRLCK) {
            pfdlockfLock->FDLOCK_usType = F_UNLCK;
            __fdLockfClearLock(pfdlockfLock, LW_NULL, pfdnode);
            pfdlockfLock->FDLOCK_usType = F_WRLCK;
        }
        /*
         * Add our lock to the blocked list and sleep until we're free.
         * Remember who blocked us (for deadlock detection).
         */
        __fdLockfBlock(pfdlockfLock, pfdlockfBlock, pfdnode);
        
        __fdLockfPrint("setlock: blocking on", pfdlockfBlock);
        __fdLockfPrintList("setlock", pfdlockfBlock);
        
        /*
         * set cleanup infomation, if thread delete in pending, 
         * thread delete hook will cleanup lockf.
         */
        __fdLockfCleanupSet(pfdlockfLock, *ppfdlockfSpare, pfdnode);
        
        API_SemaphoreBPost(pfdnode->FDNODE_ulSem);
        
        __THREAD_CANCEL_POINT();                                        /*  测试取消点                  */
        
        iError = (INT)API_SemaphoreBPend(pfdlockfLock->FDLOCK_ulBlock, LW_OPTION_WAIT_INFINITE);
        
        /*
         * we whill cleanup lockf by ourself.
         */
        __fdLockfCleanupSet(LW_NULL, LW_NULL, LW_NULL);
        
        if (API_SemaphoreBPend(pfdnode->FDNODE_ulSem, LW_OPTION_WAIT_INFINITE)) {
            __fdLockfDelete(pfdlockfLock);
            return  (EBADF);
        }
        
        /*
         *  fd_node has been removed.
         */
        if (pfdlockfLock->FDLOCK_usFlags & F_ABORT) {
            __fdLockfDelete(pfdlockfLock);
            return  (EBADF);
        }
        
        /*
         * We may have been unblocked by a signal (in
         * which case we must remove ourselves from the
         * blocked list) and/or by another process
         * releasing a lock (in which case we have already
         * been removed from the blocked list and our
         * lf_next field set to NULL).
         *
         * Note that while we were waiting, lock->lf_next may
         * have been changed to some other blocker.
         */
        if ((pfdlockfBlock = pfdlockfLock->FDLOCK_pfdlockNext) != LW_NULL) {
            __fdLockfUnblock(pfdlockfLock, pfdlockfBlock, pfdnode);
        }
        if (iError) {
            __fdLockfDelete(pfdlockfLock);
            return  (iError);
        }
    }
    
    /*
     * No blocks!!  Add the lock.  Note that we will
     * downgrade or upgrade any overlapping locks this
     * process already owns.
     *
     * Skip over locks owned by other processes.
     * Handle any locks that overlap and are owned by ourselves.
     */
    ppfdlockfPrev = &pfdnode->FDNODE_pfdlockHead;
    pfdlockfBlock =  pfdnode->FDNODE_pfdlockHead;
    iNeedToLink = 1;
    
    for (;;) {
        iOvcase = __fdLockfFindOverlap(pfdlockfBlock, pfdlockfLock, SELF, 
                                       &ppfdlockfPrev, &pfdlockfOverlap);
        if (iOvcase) {
            pfdlockfBlock = pfdlockfOverlap->FDLOCK_pfdlockNext;
        }
        
        /*
         * Six cases:
         *    0) no overlap
         *    1) overlap == lock
         *    2) overlap contains lock
         *    3) lock contains overlap
         *    4) overlap starts before lock
         *    5) overlap ends after lock
         */
        switch (iOvcase) {
        
        case 0:                                                         /* no overlap                   */
            if (iNeedToLink) {
                *ppfdlockfPrev = pfdlockfLock;
                pfdlockfLock->FDLOCK_pfdlockNext = pfdlockfOverlap;
            }
            break;
            
        case 1:                                                         /* overlap == lock              */
            /*
             * If downgrading lock, others may be
             * able to acquire it.
             */
            if (pfdlockfLock->FDLOCK_usType == F_RDLCK &&
                pfdlockfOverlap->FDLOCK_usType == F_WRLCK) {
                __fdLockfWakeup(pfdlockfOverlap, pfdnode);
            }
            pfdlockfOverlap->FDLOCK_usType = pfdlockfLock->FDLOCK_usType;
            __fdLockfDelete(pfdlockfLock);
            pfdlockfLock = pfdlockfOverlap;                             /* for debug output below       */
            break;
            
        case 2:                                                         /* overlap contains lock        */
            /*
             * Check for common starting point and different types.
             */
            if (pfdlockfOverlap->FDLOCK_usType == pfdlockfLock->FDLOCK_usType) {
                __fdLockfDelete(pfdlockfLock);
                pfdlockfLock = pfdlockfOverlap; /* for debug output below */
                break;
            }
            if (pfdlockfOverlap->FDLOCK_oftStart == pfdlockfLock->FDLOCK_oftStart) {
                *ppfdlockfPrev = pfdlockfLock;
                pfdlockfLock->FDLOCK_pfdlockNext = pfdlockfOverlap;
                pfdlockfOverlap->FDLOCK_oftStart = pfdlockfLock->FDLOCK_oftEnd + 1;
            } else {
                __fdLockfSplit(pfdlockfOverlap, pfdlockfLock, ppfdlockfSpare);
            }
            __fdLockfWakeup(pfdlockfOverlap, pfdnode);
            break;
            
        case 3:                                                         /* lock contains overlap        */
            /*
             * If downgrading lock, others may be able to
             * acquire it, otherwise take the list.
             */
            if (pfdlockfLock->FDLOCK_usType == F_RDLCK &&
                pfdlockfOverlap->FDLOCK_usType == F_WRLCK) {
                __fdLockfWakeup(pfdlockfOverlap, pfdnode);
            } else {
                while (pfdlockfOverlap->FDLOCK_plineBlockHd) {
                    pfdlockfTmp = _LIST_ENTRY(pfdlockfOverlap->FDLOCK_plineBlockHd, 
                                              LW_FD_LOCKF, 
                                              FDLOCK_lineBlock);
                    __fdLockfUnblock(pfdlockfTmp, pfdlockfOverlap, pfdnode);
                    __fdLockfBlock(pfdlockfTmp, pfdlockfLock, pfdnode);
                }
            }
            /*
             * Add the new lock if necessary and delete the overlap.
             */
            if (iNeedToLink) {
                *ppfdlockfPrev = pfdlockfLock;
                pfdlockfLock->FDLOCK_pfdlockNext = pfdlockfOverlap->FDLOCK_pfdlockNext;
                ppfdlockfPrev = &pfdlockfLock->FDLOCK_pfdlockNext;
                iNeedToLink = 0;
            } else {
                *ppfdlockfPrev = pfdlockfOverlap->FDLOCK_pfdlockNext;
            }
            __fdLockfDelete(pfdlockfOverlap);
            continue;                                                   /* next loop                    */
            
        case 4:                                                         /* overlap starts before lock   */
            /*
             * Add lock after overlap on the list.
             */
            pfdlockfLock->FDLOCK_pfdlockNext = pfdlockfOverlap->FDLOCK_pfdlockNext;
            pfdlockfOverlap->FDLOCK_pfdlockNext = pfdlockfLock;
            pfdlockfOverlap->FDLOCK_oftEnd = pfdlockfLock->FDLOCK_oftStart - 1;
            ppfdlockfPrev = &pfdlockfLock->FDLOCK_pfdlockNext;
            __fdLockfWakeup(pfdlockfOverlap, pfdnode);
            iNeedToLink = 0;
            continue;                                                   /* next loop                    */
            
        case 5:                                                         /* overlap ends after lock      */
            /*
             * Add the new lock before overlap.
             */
            if (iNeedToLink) {
                *ppfdlockfPrev = pfdlockfLock;
                pfdlockfLock->FDLOCK_pfdlockNext = pfdlockfOverlap;
            }
            pfdlockfOverlap->FDLOCK_oftStart = pfdlockfLock->FDLOCK_oftEnd + 1;
            __fdLockfWakeup(pfdlockfOverlap, pfdnode);
            break;
        }
        break;
    }
    
    __fdLockfPrint("setlock: got the lock", pfdlockfLock);
    __fdLockfPrintList("setlock", pfdlockfLock);
    
    return  (0);
}
/*********************************************************************************************************
** 函数名称: __fdLockfAdvLock
** 功能描述: 处理一次建议性锁操作
** 输　入  : pfdnode       fd_node
**           pid           lockf 所属进程
**           iOp           F_SETLK / F_UNLCK / F_GETLK
**           pfl           文件锁参数
**           oftSize       文件大小
**           iFlags        F_FLOCK / F_WAIT / F_POSIX
** 输　出  : ERRNO
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fdLockfAdvLock (PLW_FD_NODE   pfdnode,
                              pid_t         pid,
                              INT           iOp,
                              struct flock *pfl,
                              off_t         oftSize,
                              INT           iFlags)
{
    PLW_FD_LOCKF    pfdlockfLock  = LW_NULL;
    PLW_FD_LOCKF    pfdlockfSpare = LW_NULL;
    off_t           oftStart, oftEnd;
    INT             iError = 0;
    
    if (pid < 0) {
        pid = __PROC_GET_PID_CUR();
    }
    
    /*
     * Convert the flock structure into a start and end.
     */
    switch (pfl->l_whence) {
    
    case SEEK_SET:
    case SEEK_CUR:
        /*
         * Caller is responsible for adding any necessary offset
         * when SEEK_CUR is used.
         */
        oftStart = pfl->l_start;
        break;

    case SEEK_END:
        oftStart = oftSize + pfl->l_start;
        break;

    default:
        return  (EINVAL);
    }
    
    if (oftStart < 0) {
        return  (EINVAL);
    }
    
    /*
     * allocate locks before acquire vnode lock.
     * we need two locks in the worst case.
     */
    switch (iOp) {
    
    case F_SETLK:
    case F_UNLCK:
        /*
         * for F_UNLCK case, we can re-use lock.
         */
        if ((pfl->l_type & F_FLOCK) == 0) {
            pfdlockfSpare = __fdLockfCreate();
            if (pfdlockfSpare == LW_NULL) {                             /*  need one more lock.         */
                return  (ENOLCK);
            }
            break;
        }
        break;

    case F_GETLK:
        break;

    default:
        return  (EINVAL);
    }
    
    pfdlockfLock = __fdLockfCreate();
    if (pfdlockfLock == LW_NULL) {
        __fdLockfDelete(pfdlockfSpare);
        return  (ENOLCK);
    }
    
    API_SemaphoreBPend(pfdnode->FDNODE_ulSem, LW_OPTION_WAIT_INFINITE);
    
    /*
     * Avoid the common case of unlocking when inode has no locks.
     */
    if (pfdnode->FDNODE_pfdlockHead == LW_NULL) {                       /*  no lock in fd_node          */
        if (iOp != F_SETLK) {
            pfl->l_type = F_UNLCK;
            goto    __lock_done;
        }
    }

    if (pfl->l_len == 0) {
        oftEnd = -1;                                                    /*  file EOF                    */
    } else {
        oftEnd = oftStart + pfl->l_len - 1;
    }
    /*
     * Create the lockf structure.
     */
    pfdlockfLock->FDLOCK_oftStart = oftStart;
    pfdlockfLock->FDLOCK_oftEnd   = oftEnd;

    pfdlockfLock->FDLOCK_ppfdlockHead = &pfdnode->FDNODE_pfdlockHead;
    pfdlockfLock->FDLOCK_usType       = pfl->l_type;
    pfdlockfLock->FDLOCK_pfdlockNext  = LW_NULL;
    pfdlockfLock->FDLOCK_plineBlockHd = LW_NULL;
    pfdlockfLock->FDLOCK_usFlags      = (INT16)iFlags;
    pfdlockfLock->FDLOCK_pid          = pid;

    /*
     * Do the requested operation.
     */
    switch (iOp) {

    case F_SETLK:
        iError = __fdLockfSetLock(pfdlockfLock, &pfdlockfSpare, pfdnode);
        pfdlockfLock = LW_NULL;                                         /*  always uses-or-frees it     */
        break;

    case F_UNLCK:
        iError = __fdLockfClearLock(pfdlockfLock, &pfdlockfSpare, pfdnode);
        break;

    case F_GETLK:
        iError = __fdLockfGetLock(pfdlockfLock, pfl);
        break;
    }
    
__lock_done:
    API_SemaphoreBPost(pfdnode->FDNODE_ulSem);
    __fdLockfDelete(pfdlockfLock);
    __fdLockfDelete(pfdlockfSpare);
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: _FdLockfAbortAdvLocks
** 功能描述: 清空所有 fdnode 对应的记录锁
** 输　入  : pfdnode       fd_node
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __fdLockAbortAdvLocks (PLW_FD_NODE  pfdnode)
{
    PLW_FD_LOCKF    pfdlockf, pfdlockfDel, pfdlockfBlock;
    PLW_LIST_LINE   plineTmp;
    
    API_SemaphoreBPend(pfdnode->FDNODE_ulSem, LW_OPTION_WAIT_INFINITE);
    
    pfdlockf = pfdnode->FDNODE_pfdlockHead;
    while (pfdlockf) {
        for (plineTmp  = pfdlockf->FDLOCK_plineBlockHd;
             plineTmp != LW_NULL;
             plineTmp  = _list_line_get_next(plineTmp)) {
             
            pfdlockfBlock = _LIST_ENTRY(plineTmp, LW_FD_LOCKF, FDLOCK_lineBlock);
            
            pfdlockfBlock->FDLOCK_usFlags |= F_ABORT;
            
            __fdLockfWakeup(pfdlockfBlock, pfdnode);                    /*  wakeup                      */
        }
        
        pfdlockfDel = pfdlockf;
        pfdlockf    = pfdlockf->FDLOCK_pfdlockNext;
        __fdLockfDelete(pfdlockfDel);
    }
    
    pfdnode->FDNODE_pfdlockHead = LW_NULL;
    
    API_SemaphoreBPost(pfdnode->FDNODE_ulSem);
}
/*********************************************************************************************************
** 函数名称: _FdLockfIoctl
** 功能描述: fcntl 函数将通过此函数操作文件锁
** 输　入  : pfdentry      fd_entry
**           iCmd          ioctl 命令
**           pfl           文件锁参数
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _FdLockfIoctl (PLW_FD_ENTRY   pfdentry, INT  iCmd, struct flock *pfl)
{
    PLW_FD_NODE   pfdnode;
    struct flock  fl;
    INT           iError;
    INT           iOpt;
    INT           iFlag = F_POSIX;
    
    if (pfdentry->FDENTRY_iType != LW_DRV_TYPE_NEW_1) {                 /*  目前只有 NEW_1 类型支持     */
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
    
    pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_lValue;
    if (pfdnode == LW_NULL) {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
    
    if (iCmd == FIOGETLK) {                                             /*  获取                        */
        if (pfl->l_whence == SEEK_CUR) {
            pfl->l_start  += pfdentry->FDENTRY_oftPtr;
        }
        iError = __fdLockfAdvLock(pfdnode, -1, F_GETLK, pfl, pfdnode->FDNODE_oftSize, iFlag);
    
    } else {
        fl = *pfl;                                                      /*  拷贝                        */
    
        if (iCmd == FIOSETLKW) {
            iFlag |= F_WAIT;
        }
        if (fl.l_type == F_UNLCK) {
            iOpt = F_UNLCK;
        } else {
            iOpt = F_SETLK;
        }
        
        if (fl.l_whence == SEEK_CUR) {
            fl.l_start  += pfdentry->FDENTRY_oftPtr;
        }
        iError = __fdLockfAdvLock(pfdnode, -1, iOpt, &fl, pfdnode->FDNODE_oftSize, iFlag);
    }
    
    if (iError) {
        _ErrorHandle(iError);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _FdLockfProc
** 功能描述: flock 函数将通过此锁定整个文件 (此函数也可用作进程结束时的锁回收)
** 输　入  : pfdentry      fd_entry
**           iType         锁定方式 LOCK_SH / LOCK_EX / LOCK_UN
**           pid           进程 id
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _FdLockfProc (PLW_FD_ENTRY   pfdentry, INT  iType, pid_t  pid)
{
    PLW_FD_NODE   pfdnode;
    struct flock  fl;
    INT           iError;
    INT           iOpt;
    INT           iFlag;
    
    if (pfdentry->FDENTRY_iType != LW_DRV_TYPE_NEW_1) {                 /*  目前只有 NEW_1 类型支持     */
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
    
    pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_lValue;
    if (pfdnode == LW_NULL) {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
    
    iFlag = F_FLOCK;                                                    /*  BSD file lock               */
    if (iType & LOCK_NB) {                                              /*  NON_BLOCK                   */
        iType &= ~LOCK_NB;
    } else {
        iFlag |= F_WAIT;
    }
    
    if ((iType == LOCK_SH) ||
        (iType == LOCK_EX)) {
        iOpt = F_SETLK;
    
    } else if (iType == LOCK_UN) {
        iOpt = F_UNLCK;
    
    } else {
        _ErrorHandle(EINVAL);                                           /*  参数错误                    */
        return  (PX_ERROR);
    }
    
    fl.l_type   = (short)iType;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;                                                    /*  whole file                  */
    fl.l_len    = 0;
    fl.l_pid    = pid;
    
    iError = __fdLockfAdvLock(pfdnode, pid, iOpt, &fl, pfdnode->FDNODE_oftSize, iFlag);
    if (iError) {
        _ErrorHandle(iError);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _FdLockfClearAll
** 功能描述: 一个 fd_node 移除时需要通过此函数回收所有的锁
** 输　入  : pfdnode       fd_node
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _FdLockfClearFdNode (PLW_FD_NODE  pfdnode)
{
    __fdLockAbortAdvLocks(pfdnode);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _FdLockfClearFdEntry
** 功能描述: 一个 fd_entry 移除时需要通过此函数回收所有的锁
** 输　入  : pfdentry      fd_entry
**           pid           进程 pid
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _FdLockfClearFdEntry (PLW_FD_ENTRY  pfdentry, pid_t  pid)
{
    PLW_FD_NODE   pfdnode;
    
    if (pfdentry->FDENTRY_iType != LW_DRV_TYPE_NEW_1) {                 /*  目前只有 NEW_1 类型支持     */
        return  (ERROR_NONE);                                           /*  不需要清理                  */
    }
    
    pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_lValue;
    if (pfdnode == LW_NULL) {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
    
    API_SemaphoreBPend(pfdnode->FDNODE_ulSem, LW_OPTION_WAIT_INFINITE);
    if (pfdnode->FDNODE_pfdlockHead == LW_NULL) {                       /*  no lock in fd_node          */
        API_SemaphoreBPost(pfdnode->FDNODE_ulSem);
        return  (ERROR_NONE);                                           /*  不存在任何记录锁            */
    }
    API_SemaphoreBPost(pfdnode->FDNODE_ulSem);
    
    return  (_FdLockfProc(pfdentry, LOCK_UN, pid));                     /*  移除与本进程相关的锁        */
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
