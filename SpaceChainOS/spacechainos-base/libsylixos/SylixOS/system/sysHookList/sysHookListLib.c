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
** 文   件   名: sysHookListLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 04 月 01 日
**
** 描        述: 系统钩子函数链内部库

** BUG
2007.11.07  _SysCreateHook() 加入建立时的 option 选项.
2007.11.13  使用链表库对链表操作进行完全封装.
2007.11.18  整理注释.
2008.03.02  加入系统重新启动回调.
2008.03.10  使用安全机制的回调链操作.
2009.04.09  修改回调参数.
2010.08.03  每个回调控制块使用独立的 spinlock.
2012.09.23  初始化时并不设置系统回调, 而是当用户第一次调用 hook add 操作时再安装.
2013.03.16  加入进程创建和删除回调.
2013.12.12  中断 hook 加入向量与嵌套层数参数.
2014.01.07  修正部分 hook 函数参数.
2015.05.15  不同的 hook 使用不同的锁.
2016.06.26  调用 hook 为反序调用, 同时 delete hook 时不产生空洞.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#define  __SYSHOOKLIST_MAIN_FILE
#include "sysHookList.h"
/*********************************************************************************************************
  HOOK 服务函数模板 (spinlock 版本)
*********************************************************************************************************/
#define __HOOK_TEMPLATE_SPIN(phookcb, param) \
        do { \
            INT             i; \
            INTREG          iregInterLevel; \
            LW_HOOK_FUNC    pfuncHook; \
             \
            LW_SPIN_LOCK_QUICK(&((phookcb)->HOOKCB_slHook), &iregInterLevel); \
            for (i = (phookcb->HOOKCB_uiCnt - 1); i >= 0; i--) { \
                if ((phookcb)->HOOKCB_pfuncnode[i]) { \
                    pfuncHook = (phookcb)->HOOKCB_pfuncnode[i]->FUNCNODE_hookfunc; \
                    LW_SPIN_UNLOCK_QUICK(&((phookcb)->HOOKCB_slHook), iregInterLevel); \
                    pfuncHook param; \
                    LW_SPIN_LOCK_QUICK(&((phookcb)->HOOKCB_slHook), &iregInterLevel); \
                } \
            } \
            LW_SPIN_UNLOCK_QUICK(&((phookcb)->HOOKCB_slHook), iregInterLevel); \
        } while (0)
/*********************************************************************************************************
  HOOK 服务函数模板 (不加锁)
*********************************************************************************************************/
#define __HOOK_TEMPLATE(phookcb, param) \
        do { \
            INT             i; \
            LW_HOOK_FUNC    pfuncHook; \
             \
            for (i = (phookcb->HOOKCB_uiCnt - 1); i >= 0; i--) { \
                if ((phookcb)->HOOKCB_pfuncnode[i]) { \
                    pfuncHook = (phookcb)->HOOKCB_pfuncnode[i]->FUNCNODE_hookfunc; \
                    pfuncHook param; \
                } \
            } \
        } while (0)
/*********************************************************************************************************
** 函数名称: _HookAdjTemplate
** 功能描述: 调整模板, 删除空洞项
** 输　入  : phookcb       回调控制块
**           iHole         删除的空洞位置
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _HookAdjTemplate (PLW_HOOK_CB  phookcb, INT  iHole)
{
    INT  i;
    
    for (i = (iHole + 1); i < LW_SYS_HOOK_SIZE; i++) {
        if (phookcb->HOOKCB_pfuncnode[i]) {
            phookcb->HOOKCB_pfuncnode[i - 1] = phookcb->HOOKCB_pfuncnode[i];
            phookcb->HOOKCB_pfuncnode[i]     = LW_NULL;
        } else {
            break;
        }
    }
}
/*********************************************************************************************************
** 函数名称: _HookAddTemplate
** 功能描述: HOOK 加入一个调用模板
** 输　入  : phookcb       回调控制块
**           pfuncnode     加入的新节点
** 输　出  : 是否加入成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _HookAddTemplate (PLW_HOOK_CB  phookcb, PLW_FUNC_NODE  pfuncnode)
{
    INT      i;
    INTREG   iregInterLevel;
    
    LW_SPIN_LOCK_QUICK(&phookcb->HOOKCB_slHook, &iregInterLevel);
    for (i = 0; i < LW_SYS_HOOK_SIZE; i++) {
        if (phookcb->HOOKCB_pfuncnode[i] == LW_NULL) {
            phookcb->HOOKCB_pfuncnode[i] =  pfuncnode;
            phookcb->HOOKCB_uiCnt++;
            break;
        }
    }
    LW_SPIN_UNLOCK_QUICK(&phookcb->HOOKCB_slHook, iregInterLevel);
    
    return  ((i < LW_SYS_HOOK_SIZE) ? ERROR_NONE : PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: _HookDelTemplate
** 功能描述: 任务创建 HOOK 删除一个调用
** 输　入  : pfunc         需要删除的调用
**           bEmpty        删除后是否已经没有任何节点
** 输　出  : 已经从链表中移除的节点
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_FUNC_NODE  _HookDelTemplate (PLW_HOOK_CB  phookcb, LW_HOOK_FUNC  hookfunc, BOOL  *bEmpty)
{
    INT             i;
    INTREG          iregInterLevel;
    PLW_FUNC_NODE   pfuncnode;
    
    *bEmpty = LW_FALSE;
    
    LW_SPIN_LOCK_QUICK(&phookcb->HOOKCB_slHook, &iregInterLevel);
    for (i = 0; i < LW_SYS_HOOK_SIZE; i++) {
        if (phookcb->HOOKCB_pfuncnode[i]) {
            if (phookcb->HOOKCB_pfuncnode[i]->FUNCNODE_hookfunc == hookfunc) {
                pfuncnode = phookcb->HOOKCB_pfuncnode[i];
                phookcb->HOOKCB_pfuncnode[i] = LW_NULL;
                phookcb->HOOKCB_uiCnt--;
                if (!phookcb->HOOKCB_uiCnt) {
                    *bEmpty = LW_TRUE;
                } else {
                    _HookAdjTemplate(phookcb, i);
                }
                break;
            }
        }
    }
    LW_SPIN_UNLOCK_QUICK(&phookcb->HOOKCB_slHook, iregInterLevel);
    
    return  ((i < LW_SYS_HOOK_SIZE) ? pfuncnode : LW_NULL);
}
/*********************************************************************************************************
** 函数名称: _CreateHookCall
** 功能描述: 任务创建 HOOK 调用
** 输　入  : ulId                      线程 Id
             ulOption                  建立选项
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _CreateHookCall (LW_OBJECT_HANDLE  ulId, ULONG  ulOption)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_CREATE, (ulId, ulOption));
}
/*********************************************************************************************************
** 函数名称: _DeleteHookCall
** 功能描述: 任务删除 HOOK 调用
** 输　入  : ulId                      线程 Id
**           pvReturnVal               线程返回值
**           ptcb                      线程 TCB
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _DeleteHookCall (LW_OBJECT_HANDLE  ulId, PVOID  pvReturnVal, PLW_CLASS_TCB  ptcb)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_DELETE, (ulId, pvReturnVal, ptcb));
}
/*********************************************************************************************************
** 函数名称: _SwapHookCall
** 功能描述: 任务切换 HOOK 调用
** 输　入  : hOldThread        老线程
**           hNewThread        新线程
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _SwapHookCall (LW_OBJECT_HANDLE   hOldThread, LW_OBJECT_HANDLE   hNewThread)
{
    __HOOK_TEMPLATE(HOOK_T_SWAP, (hOldThread, hNewThread));
}
/*********************************************************************************************************
** 函数名称: _TickHookCall
** 功能描述: TICK HOOK 调用
** 输　入  : i64Tick   当前 tick
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _TickHookCall (INT64   i64Tick)
{
    __HOOK_TEMPLATE(HOOK_T_TICK, (i64Tick));
}
/*********************************************************************************************************
** 函数名称: _InitHookCall
** 功能描述: 线程初始化 HOOK 调用
** 输　入  : ulId                      线程 Id
**           ptcb                      线程 TCB
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _InitHookCall (LW_OBJECT_HANDLE  ulId, PLW_CLASS_TCB  ptcb)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_INIT, (ulId, ptcb));
}
/*********************************************************************************************************
** 函数名称: _IdleHookCall
** 功能描述: 空闲线程 HOOK 调用
** 输　入  : ulCPUId                   CPU ID
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _IdleHookCall (ULONG  ulCPUId)
{
    __HOOK_TEMPLATE(HOOK_T_IDLE, (ulCPUId));
}
/*********************************************************************************************************
** 函数名称: _InitBeginHookCall
** 功能描述: 系统初始化开始 HOOK 调用
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _InitBeginHookCall (VOID)
{
    __HOOK_TEMPLATE(HOOK_T_INITBEGIN, ());
}
/*********************************************************************************************************
** 函数名称: _InitEndHookCall
** 功能描述: 系统初始化结束 HOOK 调用
** 输　入  : iError                    操作系统初始化是否出现错误   0 无错误   1 错误
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _InitEndHookCall (INT  iError)
{
    __HOOK_TEMPLATE(HOOK_T_INITEND, (iError));
}
/*********************************************************************************************************
** 函数名称: _RebootHookCall
** 功能描述: 系统重启 HOOK 调用
** 输　入  : iRebootType                系统重新启动类型
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _RebootHookCall (INT  iRebootType)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_REBOOT, (iRebootType));
}
/*********************************************************************************************************
** 函数名称: _WatchDogHookCall
** 功能描述: 线程看门狗 HOOK 调用
** 输　入  : ulId                      线程 Id
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _WatchDogHookCall (LW_OBJECT_HANDLE  ulId)
{
    __HOOK_TEMPLATE(HOOK_T_WATCHDOG, (ulId));
}
/*********************************************************************************************************
** 函数名称: _ObjCreateHookCall
** 功能描述: 创建内核对象 HOOK 调用
** 输　入  : ulId                      线程 Id
**           ulOption                  创建选项
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _ObjCreateHookCall (LW_OBJECT_HANDLE  ulId, ULONG  ulOption)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_OBJCREATE, (ulId, ulOption));
}
/*********************************************************************************************************
** 函数名称: _ObjDeleteHookCall
** 功能描述: 删除内核对象 HOOK 调用
** 输　入  : ulId                      线程 Id
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _ObjDeleteHookCall (LW_OBJECT_HANDLE  ulId)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_OBJDELETE, (ulId));
}
/*********************************************************************************************************
** 函数名称: _FdCreateHookCall
** 功能描述: 文件描述符创建 HOOK 调用
** 输　入  : iFd                       文件描述符
**           pid                       进程id
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _FdCreateHookCall (INT iFd, pid_t  pid)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_FDCREATE, (iFd, pid));
}
/*********************************************************************************************************
** 函数名称: _FdDeleteHookCall
** 功能描述: 文件描述符删除 HOOK 调用
** 输　入  : iFd                       文件描述符
**           pid                       进程id
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _FdDeleteHookCall (INT iFd, pid_t  pid)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_FDDELETE, (iFd, pid));
}
/*********************************************************************************************************
** 函数名称: _IdleEnterHookCall
** 功能描述: CPU 进入空闲模式 HOOK 调用
** 输　入  : ulIdEnterFrom             从哪个线程进入 idle
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _IdleEnterHookCall (LW_OBJECT_HANDLE  ulIdEnterFrom)
{
    __HOOK_TEMPLATE(HOOK_T_IDLEENTER, (ulIdEnterFrom));
}
/*********************************************************************************************************
** 函数名称: _IdleExitHookCall
** 功能描述: CPU 退出空闲模式 HOOK 调用
** 输　入  : ulIdExitTo                退出 idle 线程进入哪个线程
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _IdleExitHookCall (LW_OBJECT_HANDLE  ulIdExitTo)
{
    __HOOK_TEMPLATE(HOOK_T_IDLEEXIT, (ulIdExitTo));
}
/*********************************************************************************************************
** 函数名称: _IntEnterHookCall
** 功能描述: CPU 进入中断(异常)模式 HOOK 调用
** 输　入  : ulVector      中断向量
**           ulNesting     当前嵌套层数
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _IntEnterHookCall (ULONG  ulVector, ULONG  ulNesting)
{
    __HOOK_TEMPLATE(HOOK_T_INTENTER, (ulVector, ulNesting));
}
/*********************************************************************************************************
** 函数名称: _IntExitHookCall
** 功能描述: CPU 退出中断(异常)模式 HOOK 调用
** 输　入  : ulVector      中断向量
**           ulNesting     当前嵌套层数
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _IntExitHookCall (ULONG  ulVector, ULONG  ulNesting)
{
    __HOOK_TEMPLATE(HOOK_T_INTEXIT, (ulVector, ulNesting));
}
/*********************************************************************************************************
** 函数名称: _StkOverflowHookCall
** 功能描述: 系统堆栈溢出 HOOK 调用
** 输　入  : pid                       进程 id
**           ulId                      线程 id
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _StkOverflowHookCall (pid_t  pid, LW_OBJECT_HANDLE  ulId)
{
    __HOOK_TEMPLATE(HOOK_T_STKOF, (pid, ulId));
}
/*********************************************************************************************************
** 函数名称: _FatalErrorHookCall
** 功能描述: 系统致命错误 HOOK 调用
** 输　入  : pid                       进程 id
**           ulId                      线程 id
**           pinfo                     信号信息
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _FatalErrorHookCall (pid_t  pid, LW_OBJECT_HANDLE  ulId, struct siginfo *psiginfo)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_FATALERR, (pid, ulId, psiginfo));
}
/*********************************************************************************************************
** 函数名称: _VpCreateHookCall
** 功能描述: 进程建立 HOOK 调用
** 输　入  : pid                       进程 id
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _VpCreateHookCall (pid_t pid)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_VPCREATE, (pid));
}
/*********************************************************************************************************
** 函数名称: _VpDeleteHookCall
** 功能描述: 进程删除 HOOK 调用
** 输　入  : pid                       进程 id
**           iExitCode                 进程返回值
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _VpDeleteHookCall (pid_t pid, INT iExitCode)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_VPDELETE, (pid, iExitCode));
}
/*********************************************************************************************************
** 函数名称: _HookListInit
** 功能描述: 初始化 HOOK 管理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID _HookListInit (VOID)
{
    LW_SPIN_INIT(&HOOK_T_CREATE->HOOKCB_slHook);
    HOOK_T_CREATE->HOOKCB_pfuncCall = _CreateHookCall;
    
    LW_SPIN_INIT(&HOOK_T_DELETE->HOOKCB_slHook);
    HOOK_T_DELETE->HOOKCB_pfuncCall = _DeleteHookCall;
    
    LW_SPIN_INIT(&HOOK_T_SWAP->HOOKCB_slHook);
    HOOK_T_SWAP->HOOKCB_pfuncCall = _SwapHookCall;
    
    LW_SPIN_INIT(&HOOK_T_TICK->HOOKCB_slHook);
    HOOK_T_TICK->HOOKCB_pfuncCall = _TickHookCall;
    
    LW_SPIN_INIT(&HOOK_T_INIT->HOOKCB_slHook);
    HOOK_T_INIT->HOOKCB_pfuncCall = _InitHookCall;
    
    LW_SPIN_INIT(&HOOK_T_IDLE->HOOKCB_slHook);
    HOOK_T_IDLE->HOOKCB_pfuncCall = _IdleHookCall;
    
    LW_SPIN_INIT(&HOOK_T_INITBEGIN->HOOKCB_slHook);
    HOOK_T_INITBEGIN->HOOKCB_pfuncCall = _InitBeginHookCall;
    
    LW_SPIN_INIT(&HOOK_T_INITEND->HOOKCB_slHook);
    HOOK_T_INITEND->HOOKCB_pfuncCall = _InitEndHookCall;
    
    LW_SPIN_INIT(&HOOK_T_REBOOT->HOOKCB_slHook);
    HOOK_T_REBOOT->HOOKCB_pfuncCall = _RebootHookCall;
    
    LW_SPIN_INIT(&HOOK_T_WATCHDOG->HOOKCB_slHook);
    HOOK_T_WATCHDOG->HOOKCB_pfuncCall = _WatchDogHookCall;
    
    LW_SPIN_INIT(&HOOK_T_OBJCREATE->HOOKCB_slHook);
    HOOK_T_OBJCREATE->HOOKCB_pfuncCall = _ObjCreateHookCall;
    
    LW_SPIN_INIT(&HOOK_T_OBJDELETE->HOOKCB_slHook);
    HOOK_T_OBJDELETE->HOOKCB_pfuncCall = _ObjDeleteHookCall;
    
    LW_SPIN_INIT(&HOOK_T_FDCREATE->HOOKCB_slHook);
    HOOK_T_FDCREATE->HOOKCB_pfuncCall = _FdCreateHookCall;
    
    LW_SPIN_INIT(&HOOK_T_FDDELETE->HOOKCB_slHook);
    HOOK_T_FDDELETE->HOOKCB_pfuncCall = _FdDeleteHookCall;
    
    LW_SPIN_INIT(&HOOK_T_IDLEENTER->HOOKCB_slHook);
    HOOK_T_IDLEENTER->HOOKCB_pfuncCall = _IdleEnterHookCall;
    
    LW_SPIN_INIT(&HOOK_T_IDLEEXIT->HOOKCB_slHook);
    HOOK_T_IDLEEXIT->HOOKCB_pfuncCall = _IdleExitHookCall;
    
    LW_SPIN_INIT(&HOOK_T_INTENTER->HOOKCB_slHook);
    HOOK_T_INTENTER->HOOKCB_pfuncCall = _IntEnterHookCall;
    
    LW_SPIN_INIT(&HOOK_T_INTEXIT->HOOKCB_slHook);
    HOOK_T_INTEXIT->HOOKCB_pfuncCall = _IntExitHookCall;
    
    LW_SPIN_INIT(&HOOK_T_STKOF->HOOKCB_slHook);
    HOOK_T_STKOF->HOOKCB_pfuncCall = _StkOverflowHookCall;
    
    LW_SPIN_INIT(&HOOK_T_FATALERR->HOOKCB_slHook);
    HOOK_T_FATALERR->HOOKCB_pfuncCall = _FatalErrorHookCall;
    
    LW_SPIN_INIT(&HOOK_T_VPCREATE->HOOKCB_slHook);
    HOOK_T_VPCREATE->HOOKCB_pfuncCall = _VpCreateHookCall;
    
    LW_SPIN_INIT(&HOOK_T_VPDELETE->HOOKCB_slHook);
    HOOK_T_VPDELETE->HOOKCB_pfuncCall = _VpDeleteHookCall;
    
    HOOK_F_ADD = _HookAddTemplate;
    HOOK_F_DEL = _HookDelTemplate;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
