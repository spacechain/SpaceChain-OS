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
** 文   件   名: loader_file.c
**
** 创   建   人: Han.hui (韩辉)
**
** 文件创建日期: 2012 年 12 月 18 日
**
** 描        述: 进程文件描述符管理. (进程文件描述符从 0 开始到 LW_VP_MAX_FILES 结束)

** BUG:
2012.12.28  为了支持进程间传递文件描述符, 这里加入 vprocIoFileDupFrom 操作
2013.01.13  孤儿进程只继承系统的 0 1 2 标准文件描述符.
2013.06.11  vprocIoFileGetInherit() 继承所有文件描述符, 包括 FD_CLOEXE 属性的文件, 直到进程开始运行指定的
            可执行文件前, 在关闭 FD_CLOEXE 属性的文件.
2013.11.18  vprocIoFileDup() 加入一个参数, 可以控制最小文件描述符数值.
2015.07.24  修正 vprocIoFileDup() 循环错误.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
static PLW_FD_ENTRY  vprocIoFileGetInherit(LW_LD_VPROC *pvproc, INT  iFd, BOOL  *pbIsCloExec);
static INT           vprocIoFileDup2Ex(LW_LD_VPROC *pvproc, PLW_FD_ENTRY pfdentry, 
                                       INT  iNewFd, BOOL  bIsCloExec);
/*********************************************************************************************************
** 函数名称: vprocIoReclaim
** 功能描述: 回收进程打开的文件 (如果是 exec 回收, 则只回收有 FD_CLOEXEC 标志的文件)
** 输　入  : pid       进程 pid
**           bIsExec   如果为真, 则只回收 FD_CLOEXE 文件, 如果为假, 则回收全部文件.
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此程序允许被重复调用调用
*********************************************************************************************************/
VOID  vprocIoReclaim (pid_t  pid, BOOL  bIsExec)
{
    LW_LD_VPROC    *pvproc;
    PLW_FD_DESC     pfddesc;
    INT             i;
    
    pvproc = vprocGet(pid);
    if (!pvproc) {
        return;
    }
    
    LW_LD_LOCK();
    pfddesc = pvproc->VP_fddescTbl;
    for (i = 0; i < LW_VP_MAX_FILES; i++) {
        if (pfddesc[i].FDDESC_pfdentry && 
            pfddesc[i].FDDESC_ulRef) {
            if (!bIsExec || pfddesc[i].FDDESC_bCloExec) {
                API_IosFdEntryReclaim(pfddesc[i].FDDESC_pfdentry, 
                                      pfddesc[i].FDDESC_ulRef, 
                                      pid);
                pfddesc[i].FDDESC_pfdentry = LW_NULL;
                pfddesc[i].FDDESC_bCloExec = LW_FALSE;
                pfddesc[i].FDDESC_ulRef    = 0ul;
                _IosLock();
                __LW_FD_DELETE_HOOK(i, pvproc->VP_pid);
                _IosUnlock();
            }
        }
    }
    LW_LD_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: vprocIoFileDeinit
** 功能描述: 回收所有进程打开的文件
** 输　入  : pid       进程 pid
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此程序允许被重复调用调用
*********************************************************************************************************/
VOID  vprocIoFileDeinit (LW_LD_VPROC *pvproc)
{
    PLW_FD_DESC     pfddesc;
    INT             i;

    LW_LD_LOCK();
    pfddesc = pvproc->VP_fddescTbl;
    for (i = 0; i < LW_VP_MAX_FILES; i++) {
        if (pfddesc[i].FDDESC_pfdentry && 
            pfddesc[i].FDDESC_ulRef) {
            API_IosFdEntryReclaim(pfddesc[i].FDDESC_pfdentry, 
                                  pfddesc[i].FDDESC_ulRef,
                                  pvproc->VP_pid);
            pfddesc[i].FDDESC_pfdentry = LW_NULL;
            pfddesc[i].FDDESC_bCloExec = LW_FALSE;
            pfddesc[i].FDDESC_ulRef    = 0ul;
            _IosLock();
            __LW_FD_DELETE_HOOK(i, pvproc->VP_pid);
            _IosUnlock();
        }
    }
    LW_LD_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: vprocIoFileInit
** 功能描述: 初始化进程 IO 文件描述符环境, 在 vprocCreate() 中被调用.
**           如果存在父进程, 则继承父进程所有文件.
**           如果是由内核启动, 则进程继承内核所有文件描述符.
** 输　入  : pvproc    进程控制块
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  vprocIoFileInit (LW_LD_VPROC *pvproc)
{
    LW_LD_VPROC     *pvprocFather;
    PLW_FD_ENTRY     pfdentry;
    BOOL             bIsCloExec;
    INT              i;
    
    LW_LD_LOCK();
    pvprocFather = pvproc->VP_pvprocFather;
    if (pvprocFather) {                                                 /*  如果有父亲则继承父亲所有文件*/
        _IosLock();
        for (i = 0; i < LW_VP_MAX_FILES; i++) {                         /*  继承所有文件描述符          */
            pfdentry = vprocIoFileGetInherit(pvprocFather, i, &bIsCloExec);
            if (pfdentry) {
                if (vprocIoFileDup2Ex(pvproc, pfdentry, i, bIsCloExec) >= 0) {
                    __LW_FD_CREATE_HOOK(i, pvproc->VP_pid);
                }
            }
        }
        _IosUnlock();
    
    } else {                                                            /*  不存在父亲, 则继承系统的    */
        _IosLock();
        for (i = 0; i < 3; i++) {                                       /*  继承标准文件描述符          */
            pfdentry = _IosFileGetKernel(i, LW_FALSE);
            if (pfdentry) {
                if (vprocIoFileDup2Ex(pvproc, pfdentry, i, LW_FALSE) >= 0) {
                    __LW_FD_CREATE_HOOK(i, pvproc->VP_pid);
                }
            }
        }
        _IosUnlock();
    }
    LW_LD_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: vprocIoFileDupFrom
** 功能描述: 从指定进程中 dup 一个文件描述符到本进程中
** 输　入  : pidSrc        源进程pid
**           iFd           源进程文件描述符
** 输　出  : 新的文件描述符
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vprocIoFileDupFrom (pid_t  pidSrc, INT  iFd)
{
    INT             i;
    LW_LD_VPROC    *pvproc;
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;
    
    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (PX_ERROR);
    }
    
    LW_LD_LOCK();
    pvproc = vprocGet(pidSrc);
    if (!pvproc) {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }
    
    pfddesc = &pvproc->VP_fddescTbl[iFd];
    
    pfdentry = pfddesc->FDDESC_pfdentry;                                /*  获得文件结构                */
    if (!pfdentry) {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }
    
    pfddesc = &__LW_VP_GET_CUR_PROC()->VP_fddescTbl[0];                 /*  从本进程 0 开始搜索         */
    
    for (i = 0; i < LW_VP_MAX_FILES; i++, pfddesc++) {
        if (!pfddesc->FDDESC_pfdentry) {
            pfddesc->FDDESC_pfdentry = pfdentry;
            pfddesc->FDDESC_bCloExec = LW_FALSE;
            pfddesc->FDDESC_ulRef    = 1ul;                             /*  新创建的 fd 引用数为 1      */
            pfdentry->FDENTRY_ulCounter++;                              /*  总引用数++                  */
            LW_LD_UNLOCK();
            return  (i);
        }
    }
    LW_LD_UNLOCK();
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: vprocIoFileRefIncEx
** 功能描述: 通过 fd 获得设置 fd_entry 引用计数++
** 输　入  : pid           进程pid
**           iFd           文件描述符
** 输　出  : ++ 后的引用数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vprocIoFileRefIncByPid (pid_t  pid, INT  iFd)
{
    LW_LD_VPROC    *pvproc;
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;

    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (PX_ERROR);
    }
    
    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (!pvproc) {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }
    
    pfddesc = &pvproc->VP_fddescTbl[iFd];
    
    pfdentry = pfddesc->FDDESC_pfdentry;
    if (pfdentry) {
        pfddesc->FDDESC_ulRef++;
        pfdentry->FDENTRY_ulCounter++;                                  /*  总引用数++                  */
        LW_LD_UNLOCK();
        return  ((INT)pfddesc->FDDESC_ulRef);
    }
    LW_LD_UNLOCK();
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: vprocIoFileRefDecEx
** 功能描述: 通过 fd 获得设置 fd_entry 引用计数--
** 输　入  : pid           进程pid
**           iFd           文件描述符
** 输　出  : -- 后的引用数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vprocIoFileRefDecByPid (pid_t  pid, INT  iFd)
{
    LW_LD_VPROC    *pvproc;
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;

    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (PX_ERROR);
    }
    
    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (!pvproc) {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }
    
    pfddesc = &pvproc->VP_fddescTbl[iFd];
    
    pfdentry = pfddesc->FDDESC_pfdentry;
    if (pfdentry) {
        pfddesc->FDDESC_ulRef--;
        pfdentry->FDENTRY_ulCounter--;                                  /*  总引用数--                  */
        if (pfddesc->FDDESC_ulRef == 0) {
            pfddesc->FDDESC_pfdentry = LW_NULL;
            pfddesc->FDDESC_bCloExec = LW_FALSE;
        }
        LW_LD_UNLOCK();
        return  ((INT)pfddesc->FDDESC_ulRef);
    }
    LW_LD_UNLOCK();
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
  以下函数为没有加进程锁的函数
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: vprocIoFileGet
** 功能描述: 通过文件描述符获取 fd_entry
** 输　入  : iFd       文件描述符
**           bIsIgnAbn 如果是异常文件也获取
** 输　出  : fd_entry
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PLW_FD_ENTRY  vprocIoFileGet (INT  iFd, BOOL  bIsIgnAbn)
{
    PLW_FD_DESC     pfddesc;

    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (LW_NULL);
    }
    
    pfddesc = &__LW_VP_GET_CUR_PROC()->VP_fddescTbl[iFd];
    
    if (pfddesc->FDDESC_pfdentry && pfddesc->FDDESC_ulRef) {
        if (bIsIgnAbn) {                                                /*  忽略异常文件               */
            return  (pfddesc->FDDESC_pfdentry);
        
        } else if (!pfddesc->FDDESC_pfdentry->FDENTRY_iAbnormity) {
            return  (pfddesc->FDDESC_pfdentry);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: vprocIoFileGetEx
** 功能描述: 通过文件描述符获取 fd_entry (可指定进程)
** 输　入  : pvproc    进程控制块
**           iFd       文件描述符
**           bIsIgnAbn 如果是异常文件也获取
** 输　出  : fd_entry
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PLW_FD_ENTRY  vprocIoFileGetEx (LW_LD_VPROC *pvproc, INT  iFd, BOOL  bIsIgnAbn)
{
    PLW_FD_DESC     pfddesc;

    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (LW_NULL);
    }
    
    pfddesc = &pvproc->VP_fddescTbl[iFd];
    
    if (pfddesc->FDDESC_pfdentry && pfddesc->FDDESC_ulRef) {
        if (bIsIgnAbn) {                                                /*  忽略异常文件               */
            return  (pfddesc->FDDESC_pfdentry);
        
        } else if (!pfddesc->FDDESC_pfdentry->FDENTRY_iAbnormity) {
            return  (pfddesc->FDDESC_pfdentry);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: vprocIoFileGetInherit
** 功能描述: 通过文件描述符获取需要继承的 fd_entry
** 输　入  : pvproc         进程控制块
**           iFd            文件描述符
**           pbIsCloExec    是否有 cloexec 标志
** 输　出  : fd_entry
** 全局变量:
** 调用模块:
** 注  意  : 继承所有文件描述符, 包括 FD_CLOEXE 属性的文件.
*********************************************************************************************************/
static PLW_FD_ENTRY  vprocIoFileGetInherit (LW_LD_VPROC *pvproc, INT  iFd, BOOL  *pbIsCloExec)
{
    PLW_FD_DESC     pfddesc;

    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (LW_NULL);
    }
    
    pfddesc = &pvproc->VP_fddescTbl[iFd];
    
    if (pfddesc->FDDESC_pfdentry && pfddesc->FDDESC_ulRef) {
        if (!pfddesc->FDDESC_pfdentry->FDENTRY_iAbnormity) {            /*  不继承异常文件              */
            *pbIsCloExec = pfddesc->FDDESC_bCloExec;
            return  (pfddesc->FDDESC_pfdentry);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: vprocIoFileDescGet
** 功能描述: 通过 fd 获得 filedesc
** 输　入  : iFd           文件描述符
**           bIsIgnAbn     如果是异常文件也获取
** 输　出  : filedesc
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PLW_FD_DESC  vprocIoFileDescGet (INT  iFd, BOOL  bIsIgnAbn)
{
    PLW_FD_DESC     pfddesc;

    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (LW_NULL);
    }
    
    pfddesc = &__LW_VP_GET_CUR_PROC()->VP_fddescTbl[iFd];
    
    if (pfddesc->FDDESC_pfdentry && pfddesc->FDDESC_ulRef) {
        if (bIsIgnAbn) {                                                /*  忽略异常文件               */
            return  (pfddesc);
        
        } else if (!pfddesc->FDDESC_pfdentry->FDENTRY_iAbnormity) {
            return  (pfddesc);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: vprocIoFileDup
** 功能描述: 查找一个空的文件描述符, 并把文件结构 pfdentry 放入, 并且初始化文件描述符计数器为 1 
** 输　入  : pfdentry      文件结构
**           iMinFd        最小描述符数值
** 输　出  : 新的文件描述符
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vprocIoFileDup (PLW_FD_ENTRY pfdentry, INT iMinFd)
{
    INT             i;
    PLW_FD_DESC     pfddesc;
    LW_LD_VPROC    *pvproc = __LW_VP_GET_CUR_PROC();
    
    for (i = iMinFd; i < LW_VP_MAX_FILES; i++, pfddesc++) {
        pfddesc = &pvproc->VP_fddescTbl[i];
        if (!pfddesc->FDDESC_pfdentry) {
            pfddesc->FDDESC_pfdentry = pfdentry;
            pfddesc->FDDESC_bCloExec = LW_FALSE;
            pfddesc->FDDESC_ulRef    = 1ul;                             /*  新创建的 fd 引用数为 1      */
            pfdentry->FDENTRY_ulCounter++;                              /*  总引用数++                  */
            return  (i);
        }
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: vprocIoFileDup2
** 功能描述: 查找一个空的文件描述符, 并把文件结构 pfdentry 放入, 并且初始化文件描述符计数器为 1 
** 输　入  : pfdentry      文件结构
**           iNewFd        新的文件描述符
** 输　出  : 新的文件描述符
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vprocIoFileDup2 (PLW_FD_ENTRY pfdentry, INT  iNewFd)
{
    PLW_FD_DESC     pfddesc;
    
    if (iNewFd < 0 || iNewFd >= LW_VP_MAX_FILES) {
        return  (PX_ERROR);
    }
    
    pfddesc = &__LW_VP_GET_CUR_PROC()->VP_fddescTbl[iNewFd];
    
    if (!pfddesc->FDDESC_pfdentry) {
        pfddesc->FDDESC_pfdentry = pfdentry;
        pfddesc->FDDESC_bCloExec = LW_FALSE;
        pfddesc->FDDESC_ulRef    = 1ul;                                 /*  新创建的 fd 引用数为 1      */
        pfdentry->FDENTRY_ulCounter++;                                  /*  总引用数++                  */
        return  (iNewFd);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: vprocIoFileDup2Ex
** 功能描述: 查找一个空的文件描述符, 并把文件结构 pfdentry 放入, 并且初始化文件描述符计数器为 1 
** 输　入  : pvproc        进程控制块
**           pfdentry      文件结构
**           iNewFd        新的文件描述符
**           bIsCloExec    是否带有 cloexec 标志
** 输　出  : 新的文件描述符
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  vprocIoFileDup2Ex (LW_LD_VPROC *pvproc, PLW_FD_ENTRY pfdentry, INT  iNewFd, BOOL  bIsCloExec)
{
    PLW_FD_DESC     pfddesc;
    
    if (iNewFd < 0 || iNewFd >= LW_VP_MAX_FILES) {
        return  (PX_ERROR);
    }
    
    pfddesc = &pvproc->VP_fddescTbl[iNewFd];
    
    if (!pfddesc->FDDESC_pfdentry) {
        pfddesc->FDDESC_pfdentry = pfdentry;
        pfddesc->FDDESC_bCloExec = bIsCloExec;
        pfddesc->FDDESC_ulRef    = 1ul;                                 /*  新创建的 fd 引用数为 1      */
        pfdentry->FDENTRY_ulCounter++;                                  /*  总引用数++                  */
        return  (iNewFd);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: vprocIoFileRefInc
** 功能描述: 通过 fd 获得设置 fd_entry 引用计数++
** 输　入  : iFd           文件描述符
** 输　出  : ++ 后的引用数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vprocIoFileRefInc (INT  iFd)
{
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;

    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (PX_ERROR);
    }
    
    pfddesc = &__LW_VP_GET_CUR_PROC()->VP_fddescTbl[iFd];
    
    pfdentry = pfddesc->FDDESC_pfdentry;
    if (pfdentry) {
        pfddesc->FDDESC_ulRef++;
        pfdentry->FDENTRY_ulCounter++;                                  /*  总引用数++                  */
        return  ((INT)pfddesc->FDDESC_ulRef);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: vprocIoFileRefDec
** 功能描述: 通过 fd 获得设置 fd_entry 引用计数--
** 输　入  : iFd           文件描述符
** 输　出  : -- 后的引用数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vprocIoFileRefDec (INT  iFd)
{
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;

    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (PX_ERROR);
    }
    
    pfddesc = &__LW_VP_GET_CUR_PROC()->VP_fddescTbl[iFd];
    
    pfdentry = pfddesc->FDDESC_pfdentry;
    if (pfdentry) {
        pfddesc->FDDESC_ulRef--;
        pfdentry->FDENTRY_ulCounter--;                                  /*  总引用数--                  */
        if (pfddesc->FDDESC_ulRef == 0) {
            pfddesc->FDDESC_pfdentry = LW_NULL;
            pfddesc->FDDESC_bCloExec = LW_FALSE;
        }
        return  ((INT)pfddesc->FDDESC_ulRef);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: vprocIoFileRefGet
** 功能描述: 通过 fd 获得获得 fd_entry 引用计数
** 输　入  : iFd           文件描述符
** 输　出  : 引用数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vprocIoFileRefGet (INT  iFd)
{
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;

    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (PX_ERROR);
    }
    
    pfddesc = &__LW_VP_GET_CUR_PROC()->VP_fddescTbl[iFd];
    
    pfdentry = pfddesc->FDDESC_pfdentry;
    if (pfdentry) {
        return  ((INT)pfddesc->FDDESC_ulRef);
    }
    
    return  (PX_ERROR);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
