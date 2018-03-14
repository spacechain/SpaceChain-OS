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
** 文   件   名: sysHookList.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 04 月 01 日
**
** 描        述: 系统钩子函数链头文件.
**
** BUG:
2015.05.16  系统回调不再采用链式结构, 转而采用数组结构.
*********************************************************************************************************/

#ifndef  __SYSHOOKLIST_H
#define  __SYSHOOKLIST_H

/*********************************************************************************************************
  FUNCTION NODE
*********************************************************************************************************/

typedef struct {
    LW_HOOK_FUNC            FUNCNODE_hookfunc;                          /*  钩子函数指针                */
    LW_RESOURCE_RAW         FUNCNODE_resraw;                            /*  资源管理节点                */
} LW_FUNC_NODE;
typedef LW_FUNC_NODE       *PLW_FUNC_NODE;

/*********************************************************************************************************
  SYSTEM HOOK NODE
*********************************************************************************************************/

#define LW_SYS_HOOK_SIZE    16

typedef struct {
    VOIDFUNCPTR             HOOKCB_pfuncCall;                           /*  调用所有的回调节点          */
    PLW_FUNC_NODE           HOOKCB_pfuncnode[LW_SYS_HOOK_SIZE];         /*  回调节点表                  */
    UINT                    HOOKCB_uiCnt;                               /*  回调节点数                  */
    LW_SPINLOCK_DEFINE     (HOOKCB_slHook);
} LW_HOOK_CB;
typedef LW_HOOK_CB         *PLW_HOOK_CB;

/*********************************************************************************************************
  HOOK CONTROL BLOCK
*********************************************************************************************************/

typedef INT               (*LW_HOOK_FUNC_ADD)(PLW_HOOK_CB, PLW_FUNC_NODE);
typedef PLW_FUNC_NODE     (*LW_HOOK_FUNC_DEL)(PLW_HOOK_CB, LW_HOOK_FUNC, BOOL *);

typedef struct {
    LW_HOOK_FUNC_ADD        HOOKTBL_pfuncAdd;                           /*  加入一个回调节点            */
    LW_HOOK_FUNC_DEL        HOOKTBL_pfuncDel;                           /*  删除一个回调节点            */

    LW_HOOK_CB              HOOKTBL_hookcbCreate;
    LW_HOOK_CB              HOOKTBL_hookcbDelete;
    LW_HOOK_CB              HOOKTBL_hookcbSwap;
    LW_HOOK_CB              HOOKTBL_hookcbTick;
    LW_HOOK_CB              HOOKTBL_hookcbInit;
    LW_HOOK_CB              HOOKTBL_hookcbIdle;
    LW_HOOK_CB              HOOKTBL_hookcbInitBegin;
    LW_HOOK_CB              HOOKTBL_hookcbInitEnd;
    LW_HOOK_CB              HOOKTBL_hookcbReboot;
    LW_HOOK_CB              HOOKTBL_hookcbWatchDog;
    LW_HOOK_CB              HOOKTBL_hookcbObjectCreate;
    LW_HOOK_CB              HOOKTBL_hookcbObjectDelete;
    LW_HOOK_CB              HOOKTBL_hookcbFdCreate;
    LW_HOOK_CB              HOOKTBL_hookcbFdDelete;
    LW_HOOK_CB              HOOKTBL_hookcbCpuIdleEnter;
    LW_HOOK_CB              HOOKTBL_hookcbCpuIdleExit;
    LW_HOOK_CB              HOOKTBL_hookcbCpuIntEnter;
    LW_HOOK_CB              HOOKTBL_hookcbCpuIntExit;
    LW_HOOK_CB              HOOKTBL_hookcbStkOverflow;
    LW_HOOK_CB              HOOKTBL_hookcbFatalError;
    LW_HOOK_CB              HOOKTBL_hookcbVpCreate;
    LW_HOOK_CB              HOOKTBL_hookcbVpDelete;
} LW_HOOK_TABLE;

/*********************************************************************************************************
  各种 HOOK 表
*********************************************************************************************************/
#ifdef   __SYSHOOKLIST_MAIN_FILE
#define  __SYSHOOK_EXT
#else
#define  __SYSHOOK_EXT           extern
#endif

__SYSHOOK_EXT LW_HOOK_TABLE _G_hookTable;

#define HOOK_F_ADD          (_G_hookTable.HOOKTBL_pfuncAdd)
#define HOOK_F_DEL          (_G_hookTable.HOOKTBL_pfuncDel)

#define HOOK_T_CREATE       (&(_G_hookTable.HOOKTBL_hookcbCreate))
#define HOOK_T_DELETE       (&(_G_hookTable.HOOKTBL_hookcbDelete))
#define HOOK_T_SWAP         (&(_G_hookTable.HOOKTBL_hookcbSwap))
#define HOOK_T_TICK         (&(_G_hookTable.HOOKTBL_hookcbTick))
#define HOOK_T_INIT         (&(_G_hookTable.HOOKTBL_hookcbInit))
#define HOOK_T_IDLE         (&(_G_hookTable.HOOKTBL_hookcbIdle))
#define HOOK_T_INITBEGIN    (&(_G_hookTable.HOOKTBL_hookcbInitBegin))
#define HOOK_T_INITEND      (&(_G_hookTable.HOOKTBL_hookcbInitEnd))
#define HOOK_T_REBOOT       (&(_G_hookTable.HOOKTBL_hookcbReboot))
#define HOOK_T_WATCHDOG     (&(_G_hookTable.HOOKTBL_hookcbWatchDog))
#define HOOK_T_OBJCREATE    (&(_G_hookTable.HOOKTBL_hookcbObjectCreate))
#define HOOK_T_OBJDELETE    (&(_G_hookTable.HOOKTBL_hookcbObjectDelete))
#define HOOK_T_FDCREATE     (&(_G_hookTable.HOOKTBL_hookcbFdCreate))
#define HOOK_T_FDDELETE     (&(_G_hookTable.HOOKTBL_hookcbFdDelete))
#define HOOK_T_IDLEENTER    (&(_G_hookTable.HOOKTBL_hookcbCpuIdleEnter))
#define HOOK_T_IDLEEXIT     (&(_G_hookTable.HOOKTBL_hookcbCpuIdleExit))
#define HOOK_T_INTENTER     (&(_G_hookTable.HOOKTBL_hookcbCpuIntEnter))
#define HOOK_T_INTEXIT      (&(_G_hookTable.HOOKTBL_hookcbCpuIntExit))
#define HOOK_T_STKOF        (&(_G_hookTable.HOOKTBL_hookcbStkOverflow))
#define HOOK_T_FATALERR     (&(_G_hookTable.HOOKTBL_hookcbFatalError))
#define HOOK_T_VPCREATE     (&(_G_hookTable.HOOKTBL_hookcbVpCreate))
#define HOOK_T_VPDELETE     (&(_G_hookTable.HOOKTBL_hookcbVpDelete))

#endif                                                                  /*  __SYSHOOKLIST_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
