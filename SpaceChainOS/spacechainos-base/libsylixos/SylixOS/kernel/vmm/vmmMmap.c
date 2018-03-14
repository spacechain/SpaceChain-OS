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
** 文   件   名: vmmMmap.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2015 年 05 月 26 日
**
** 描        述: 进程内存映射管理.
**
** BUG:
2015.07.20  msync() 仅针对 SHARED 类型映射才回写文件.
2017.06.08  修正 mmap() 针对 AF_PACKET 映射错误问题.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0 && LW_CFG_DEVICE_EN > 0
#include "vmmMmap.h"
/*********************************************************************************************************
  全局变量声明
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_ulVmmMapLock;
#define __VMM_MMAP_LOCK()   API_SemaphoreMPend(_G_ulVmmMapLock, LW_OPTION_WAIT_INFINITE)
#define __VMM_MMAP_UNLOCK() API_SemaphoreMPost(_G_ulVmmMapLock)
/*********************************************************************************************************
  如果没有 VPROC 支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
extern LW_LIST_LINE_HEADER  _G_plineVProcHeader;                        /*  进程链表                    */
#else
static LW_LIST_LINE_HEADER  _K_plineMapnVHeader;                        /*  排序管理链表                */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
static LW_LIST_LINE_HEADER  _K_plineMapnMHeader;                        /*  统一管理链表                */
/*********************************************************************************************************
** 函数名称: __vmmMapnVHeader
** 功能描述: 获得当前进程 MAP NODE 控制块表头地址.
** 输　入  : NONE
** 输　出  : MAP NODE 控制块进程表头地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __vmmMapInit (VOID)
{
    _G_ulVmmMapLock = API_SemaphoreMCreate("vmmap_lock", LW_PRIO_DEF_CEILING, 
                                           LW_OPTION_INHERIT_PRIORITY |
                                           LW_OPTION_WAIT_PRIORITY | 
                                           LW_OPTION_DELETE_SAFE |
                                           LW_OPTION_OBJECT_GLOBAL,     /*  基于优先级等待              */
                                           LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __vmmMapnVHeader
** 功能描述: 获得当前进程 MAP NODE 控制块表头地址.
** 输　入  : NONE
** 输　出  : MAP NODE 控制块进程表头地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  *__vmmMapnVHeader (VOID)
{
#if LW_CFG_MODULELOADER_EN > 0
    REGISTER PLW_CLASS_TCB   ptcbCur;
    REGISTER LW_LD_VPROC    *pvproc;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (ptcbCur->TCB_pvVProcessContext) {
        pvproc = (LW_LD_VPROC *)ptcbCur->TCB_pvVProcessContext;
        return  (&(pvproc->VP_plineMap));                               /*  进程 MAP NODE               */
    
    } else {
        return  (&(_G_vprocKernel.VP_plineMap));                        /*  内核 MAP NODE               */
    }
#else
    return  (&_K_plineMapnVHeader);                                     /*  统一的 MAP NODE 排序头      */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
}
/*********************************************************************************************************
** 函数名称: __vmmMapnFindCur
** 功能描述: 通过虚拟地址查找当前进程 MAP NODE 控制块.
** 输　入  : pvAddr        起始地址
**           stLen         内存空间长度
** 输　出  : MAP NODE 控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_VMM_MAP_NODE  __vmmMapnFindCur (PVOID  pvAddr)
{
    LW_LIST_LINE_HEADER  *pplineVHeader = __vmmMapnVHeader();
    PLW_LIST_LINE         plineTemp;
    PLW_VMM_MAP_NODE      pmapn;
    
    for (plineTemp  = *pplineVHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pmapn = _LIST_ENTRY(plineTemp, LW_VMM_MAP_NODE, MAPN_lineVproc);
        if (((addr_t)pvAddr >= (addr_t)pmapn->MAPN_pvAddr) && 
            ((addr_t)pvAddr <  ((addr_t)pmapn->MAPN_pvAddr + pmapn->MAPN_stLen))) {
            return  (pmapn);
        
        } else if ((addr_t)pvAddr < (addr_t)pmapn->MAPN_pvAddr) {       /*  后面不可能再有了            */
            return  (LW_NULL);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __vmmMapnFindShare
** 功能描述: 查找 mmap 可以 share 的节点.
** 输　入  : pmapnAbort         mmap node
**           ulAbortAddr        发生缺页中断的地址 (页面对齐)
**           pfuncCallback      回调函数
** 输　出  : 回调函数返回值
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PVOID  __vmmMapnFindShare (PLW_VMM_MAP_NODE pmapnAbort,
                                  addr_t           ulAbortAddr,
                                  PVOID          (*pfuncCallback)(PVOID  pvStartAddr, size_t  stOffset))
{
    PLW_VMM_MAP_NODE    pmapn;
    PLW_LIST_LINE       plineTemp;
    PVOID               pvRet = LW_NULL;
    off_t               oft;                                            /*  一定是 VMM 页面对齐         */
    
    if (pmapnAbort->MAPN_iFd < 0) {                                     /*  不能共享                    */
        return  (LW_NULL);
    }
    
    oft = ((off_t)(ulAbortAddr - (addr_t)pmapnAbort->MAPN_pvAddr) + pmapnAbort->MAPN_off);
    
    for (plineTemp  = _K_plineMapnMHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pmapn = _LIST_ENTRY(plineTemp, LW_VMM_MAP_NODE, MAPN_lineManage);
        if (pmapn != pmapnAbort) {
            if ((pmapn->MAPN_iFd   >= 0) &&
                (pmapn->MAPN_dev   == pmapnAbort->MAPN_dev) &&
                (pmapn->MAPN_ino64 == pmapnAbort->MAPN_ino64)) {        /*  映射的文件相同              */
                
                if ((oft >= pmapn->MAPN_off) &&
                    (oft <  (pmapn->MAPN_off + pmapn->MAPN_stLen))) {   /*  范围以内                    */
                    pvRet = pfuncCallback(pmapn->MAPN_pvAddr,
                                          (size_t)(oft - pmapn->MAPN_off));
                    if (pvRet) {
                        break;
                    }
                }
            }
        }
    }
    
    return  (pvRet);
}
/*********************************************************************************************************
** 函数名称: __vmmMapnFill
** 功能描述: 缺页中断分配内存后, 将通过此函数填充文件数据 (注意, 此函数在 vmm lock 中执行!)
** 输　入  : pmapn              mmap node
**           ulDestPageAddr     拷贝目标地址 (页面对齐)
**           ulMapPageAddr      最终会被映射的目标地址 (页面对齐)
**           ulPageNum          新分配出的页面个数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 暂时不处理文件读取错误的情况. (最终是将文件内容拷入 ulDestPageAddr)
*********************************************************************************************************/
static INT  __vmmMapnFill (PLW_VMM_MAP_NODE    pmapn, 
                           addr_t              ulDestPageAddr,
                           addr_t              ulMapPageAddr, 
                           ULONG               ulPageNum)
{
#if LW_CFG_MODULELOADER_EN > 0
    PLW_CLASS_TCB   ptcbCur;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    off_t           offtRead;
    size_t          stReadLen;
    addr_t          ulMapStartAddr = (addr_t)pmapn->MAPN_pvAddr;
    
#if LW_CFG_MODULELOADER_EN > 0
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    if (pmapn->MAPN_pid != vprocGetPidByTcbNoLock(ptcbCur)) {           /*  如果不是创建进程            */
        goto    __full_with_zero;
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    if ((pmapn->MAPN_iFd < 0) || !S_ISREG(pmapn->MAPN_mode)) {          /*  非数据文件类型              */
        goto    __full_with_zero;
    }
    
    if ((ulMapPageAddr < ulMapStartAddr) || 
        (ulMapPageAddr >= (ulMapStartAddr + pmapn->MAPN_stLen))) {      /*  致命错误, 页面内存地址错误  */
        goto    __full_with_zero;
    }
    
    offtRead  = (off_t)(ulMapPageAddr - ulMapStartAddr);                /*  内存地址偏移                */
    offtRead += pmapn->MAPN_off;                                        /*  加上文件起始偏移            */
    
    stReadLen = (size_t)(ulPageNum << LW_CFG_VMM_PAGE_SHIFT);           /*  需要获取的数据大小          */
    
    {
        size_t   stZNum;
        ssize_t  sstNum = API_IosPRead(pmapn->MAPN_iFd, 
                                       (PCHAR)ulDestPageAddr, stReadLen,
                                       offtRead);                       /*  读取文件内容                */
        
        sstNum = (sstNum >= 0) ? sstNum : 0;
        stZNum = (size_t)(stReadLen - sstNum);
        
        if (stZNum > 0) {
            lib_bzero((PVOID)(ulDestPageAddr + sstNum), stZNum);        /*  未使用部分清零              */
        }
    }
    
    return  (ERROR_NONE);
    
__full_with_zero:
    lib_bzero((PVOID)ulDestPageAddr, 
              (INT)(ulPageNum << LW_CFG_VMM_PAGE_SHIFT));               /*  全部清零                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __vmmMapnMalloc
** 功能描述: 分配虚拟空间
** 输　入  : pmapn         mmap node
**           stLen         内存大小
**           iFlags        LW_VMM_SHARED_CHANGE / LW_VMM_PRIVATE_CHANGE
**           ulFlag        虚拟空间内存属性
** 输　出  : 内存地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PVOID  __vmmMapnMalloc (PLW_VMM_MAP_NODE  pmapn, size_t  stLen, INT  iFlags, ULONG  ulFlag)
{
    REGISTER PVOID  pvMem;
    
    pvMem = API_VmmMallocAreaEx(stLen, __vmmMapnFill, pmapn, iFlags, ulFlag);
    if (pvMem) {
        API_VmmSetFindShare(pvMem, __vmmMapnFindShare, pmapn);
    }
    
    return  (pvMem);
}
/*********************************************************************************************************
** 函数名称: __vmmMapnFree
** 功能描述: 释放虚拟空间
** 输　入  : pvAddr        虚拟地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __vmmMapnFree (PVOID  pvAddr)
{
    API_VmmFreeArea(pvAddr);
}
/*********************************************************************************************************
** 函数名称: __vmmMapnLink
** 功能描述: 将 MAP NODE 插入管理空间
** 输　入  : pmapn         mmap node
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __vmmMapnLink (PLW_VMM_MAP_NODE  pmapn)
{
    LW_LIST_LINE_HEADER  *pplineVHeader = __vmmMapnVHeader();
    PLW_LIST_LINE         plineTemp;
    PLW_VMM_MAP_NODE      pmapnTemp;
    BOOL                  bLast = LW_FALSE;
    
    plineTemp = *pplineVHeader;
    
    while (plineTemp) {
        pmapnTemp = _LIST_ENTRY(plineTemp, LW_VMM_MAP_NODE, MAPN_lineVproc);
        if (pmapnTemp->MAPN_pvAddr > pmapn->MAPN_pvAddr) {
            break;
        }
        if (_list_line_get_next(plineTemp)) {
            plineTemp = _list_line_get_next(plineTemp);
        
        } else {
            bLast = LW_TRUE;
            break;
        }
    }
    
    if (bLast) {
        _List_Line_Add_Right(&pmapn->MAPN_lineVproc, plineTemp);
    
    } else if (plineTemp == *pplineVHeader) {
        _List_Line_Add_Ahead(&pmapn->MAPN_lineVproc, pplineVHeader);
    
    } else {
        _List_Line_Add_Left(&pmapn->MAPN_lineVproc, plineTemp);
    }
    
    _List_Line_Add_Ahead(&pmapn->MAPN_lineManage, &_K_plineMapnMHeader);
}
/*********************************************************************************************************
** 函数名称: __vmmMapnUnlink
** 功能描述: 将 MAP NODE 从管理空间卸载
** 输　入  : pmapn         mmap node
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __vmmMapnUnlink (PLW_VMM_MAP_NODE  pmapn)
{
    LW_LIST_LINE_HEADER  *pplineVHeader = __vmmMapnVHeader();
    
    _List_Line_Del(&pmapn->MAPN_lineVproc,  pplineVHeader);             /*  从进程链表中删除            */
    _List_Line_Del(&pmapn->MAPN_lineManage, &_K_plineMapnMHeader);      /*  从管理链表中删除            */
}
/*********************************************************************************************************
** 函数名称: __vmmMapnUnlink
** 功能描述: 将 MAP NODE 从管理空间卸载
** 输　入  : pmapn         mmap node
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

static VOID  __vmmMapnReclaim (PLW_VMM_MAP_NODE  pmapn, LW_LD_VPROC  *pvproc)
{
    _List_Line_Del(&pmapn->MAPN_lineVproc,  &pvproc->VP_plineMap);      /*  从进程链表中删除            */
    _List_Line_Del(&pmapn->MAPN_lineManage, &_K_plineMapnMHeader);      /*  从管理链表中删除            */
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** 函数名称: __vmmMmapNew
** 功能描述: 分配一段新的虚拟内存
** 输　入  : stLen         映射长度
**           iFlags        LW_VMM_SHARED_CHANGE / LW_VMM_PRIVATE_CHANGE
**           ulFlag        LW_VMM_FLAG_READ | LW_VMM_FLAG_RDWR | LW_VMM_FLAG_EXEC
**           iFd           文件描述符
**           off           文件偏移量
** 输　出  : 分配的虚拟内存地址, LW_VMM_MAP_FAILED 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PVOID  __vmmMmapNew (size_t  stLen, INT  iFlags, ULONG  ulFlag, int  iFd, off_t  off)
{
    PLW_VMM_MAP_NODE    pmapn;
    struct stat64       stat64Fd;
    INT                 iErrLevel = 0;
    INT                 iFileFlag;
    
    LW_DEV_MMAP_AREA    dmap;
    INT                 iError;
    
#if LW_CFG_MODULELOADER_EN > 0
    LW_LD_VPROC        *pvproc;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    if (iFd >= 0) {
        if (iosFdGetFlag(iFd, &iFileFlag) < 0) {                        /*  获得文件权限                */
            _ErrorHandle(EBADF);
            return  (LW_VMM_MAP_FAILED);
        } 
        
        iFileFlag &= O_ACCMODE;
        if (iFlags == LW_VMM_SHARED_CHANGE) {
            if ((ulFlag & LW_VMM_FLAG_WRITABLE) &&
                (iFileFlag == O_RDONLY)) {                              /*  带有写权限映射只读文件      */
                _ErrorHandle(EACCES);
                return  (LW_VMM_MAP_FAILED);
            }
        }
        
        if (fstat64(iFd, &stat64Fd) < 0) {                              /*  获得文件 stat               */
            _ErrorHandle(EBADF);
            return  (LW_VMM_MAP_FAILED);
        }
        
        if (S_ISDIR(stat64Fd.st_mode)) {                                /*  不能映射目录                */
            _ErrorHandle(ENODEV);
            return  (LW_VMM_MAP_FAILED);
        }
        
        if (off > stat64Fd.st_size) {                                   /*  off 越界                    */
            _ErrorHandle(ENXIO);
            return  (LW_VMM_MAP_FAILED);
        }
    }
    
    pmapn = (PLW_VMM_MAP_NODE)__SHEAP_ALLOC(sizeof(LW_VMM_MAP_NODE));   /*  创建控制块                  */
    if (pmapn == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_VMM_MAP_FAILED);
    }
    
    pmapn->MAPN_pvAddr = __vmmMapnMalloc(pmapn, stLen, iFlags, ulFlag);
    if (pmapn->MAPN_pvAddr == LW_NULL) {                                /*  申请映射内存                */
        _ErrorHandle(ENOMEM);
        iErrLevel = 1;
        goto    __error_handle;
    }
    
    pmapn->MAPN_stLen  = stLen;
    pmapn->MAPN_ulFlag = ulFlag;
    
    pmapn->MAPN_iFd      = iFd;
    pmapn->MAPN_mode     = stat64Fd.st_mode;
    pmapn->MAPN_off      = off;
    pmapn->MAPN_offFSize = stat64Fd.st_size;
    pmapn->MAPN_dev      = stat64Fd.st_dev;
    pmapn->MAPN_ino64    = stat64Fd.st_ino;
    pmapn->MAPN_iFlags   = iFlags;
    
#if LW_CFG_MODULELOADER_EN > 0
    pvproc = __LW_VP_GET_CUR_PROC();
    if (pvproc) {
        pmapn->MAPN_pid = pvproc->VP_pid;
    } else 
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    {
        pmapn->MAPN_pid = 0;
    }
    
    if (iFd >= 0) {
        API_IosFdRefInc(iFd);                                           /*  对文件描述符引用 ++         */
        
        dmap.DMAP_pvAddr   = pmapn->MAPN_pvAddr;
        dmap.DMAP_stLen    = pmapn->MAPN_stLen;
        dmap.DMAP_offPages = (pmapn->MAPN_off >> LW_CFG_VMM_PAGE_SHIFT);
        dmap.DMAP_ulFlag   = ulFlag;
        
        iError = API_IosMmap(iFd, &dmap);                               /*  尝试调用设备驱动            */
        if (iError < ERROR_NONE) {
            if (errno == ERROR_IOS_DRIVER_NOT_SUP) {
                if (S_ISFIFO(stat64Fd.st_mode) || 
                    S_ISSOCK(stat64Fd.st_mode)) {                       /*  不支持 MMAP 的 FIFO 与设备  */
                    iErrLevel = 2;
                    goto    __error_handle;
                }
            } else {
                iErrLevel = 2;
                goto    __error_handle;
            }
        }
    }
    
    __vmmMapnLink(pmapn);                                               /*  加入管理链表                */
    
    MONITOR_EVT_LONG5(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_MMAP,
                      pmapn->MAPN_pvAddr, 
                      iFd, stLen, iFlags, ulFlag, LW_NULL);

    return  (pmapn->MAPN_pvAddr);
    
__error_handle:
    if (iErrLevel > 1) {
        API_IosFdRefDec(iFd);                                           /*  对文件描述符引用 --         */
        __vmmMapnFree(pmapn->MAPN_pvAddr);
    }
    if (iErrLevel > 0) {
        __SHEAP_FREE(pmapn);
    }
    
    return  (LW_VMM_MAP_FAILED);
}
/*********************************************************************************************************
** 函数名称: __vmmMmapDelete
** 功能描述: 删除一段虚拟内存
** 输　入  : pmapn         mmap node
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __vmmMmapDelete (PLW_VMM_MAP_NODE    pmapn)
{
    LW_DEV_MMAP_AREA    dmap;
    
    if (pmapn->MAPN_iFd >= 0) {
        dmap.DMAP_pvAddr   = pmapn->MAPN_pvAddr;
        dmap.DMAP_stLen    = pmapn->MAPN_stLen;
        dmap.DMAP_offPages = (pmapn->MAPN_off >> LW_CFG_VMM_PAGE_SHIFT);
        dmap.DMAP_ulFlag   = pmapn->MAPN_ulFlag;
        
        API_IosUnmap(pmapn->MAPN_iFd, &dmap);                           /*  尝试调用设备驱动            */
        API_IosFdRefDec(pmapn->MAPN_iFd);                               /*  对文件描述符引用 --         */
    }
    
    __vmmMapnUnlink(pmapn);                                             /*  从管理链表中删除            */
    __vmmMapnFree(pmapn->MAPN_pvAddr);
    __SHEAP_FREE(pmapn);
}
/*********************************************************************************************************
** 函数名称: __vmmMmapChange
** 功能描述: 修改一片虚拟内存属性
** 输　入  : stLen         映射长度
**           iFlags        LW_VMM_SHARED_CHANGE / LW_VMM_PRIVATE_CHANGE
**           ulFlag        LW_VMM_FLAG_FAIL | LW_VMM_FLAG_READ | LW_VMM_FLAG_RDWR | LW_VMM_FLAG_EXEC
**           iFd           文件描述符
**           off           文件偏移量
** 输　出  : 分配的虚拟内存地址, LW_VMM_MAP_FAILED 表示错误
** 全局变量: 
** 调用模块: 
** 注  意  : 这里只允许修改内存 ulFlag 属性.
*********************************************************************************************************/
static PVOID  __vmmMmapChange (PVOID  pvAddr, size_t  stLen, INT  iFlags, 
                               ULONG  ulFlag, int  iFd, off_t  off)
{
    PLW_VMM_MAP_NODE    pmapn;
    
    pmapn = __vmmMapnFindCur(pvAddr);
    if (pmapn == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (LW_VMM_MAP_FAILED);
    }
    
    if ((pmapn->MAPN_iFd    != iFd)    ||
        (pmapn->MAPN_off    != off)    ||
        (pmapn->MAPN_iFlags != iFlags) ||
        (pmapn->MAPN_pvAddr != pvAddr) ||
        (pmapn->MAPN_stLen  != stLen)) {
        _ErrorHandle(EINVAL);
        return  (LW_VMM_MAP_FAILED);
    }
    
    if (pmapn->MAPN_ulFlag == ulFlag) {
        return  (pvAddr);
    }
    
    pmapn->MAPN_ulFlag = ulFlag;
    
    if (!(ulFlag & LW_VMM_FLAG_ACCESS)) {                               /*  此段内存不允许访问          */
        API_VmmInvalidateArea(pvAddr, pvAddr, stLen);                   /*  释放所有物理页面            */
        return  (pvAddr);
    
    } else {
        API_VmmSetFlag(pvAddr, ulFlag);
        return  (pvAddr);
    }
}
/*********************************************************************************************************
** 函数名称: __vmmMmapShrink
** 功能描述: 缩小一片虚拟内存大小
** 输　入  : pmapn         mmap node
**           stNewSize     需要设置的内存区域新大小
** 输　出  : 分配的虚拟内存地址, LW_VMM_MAP_FAILED 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PVOID  __vmmMmapShrink (PLW_VMM_MAP_NODE  pmapn, size_t stNewSize)
{
    PVOID   pvSplit;
    
    pvSplit = API_VmmSplitArea(pmapn->MAPN_pvAddr, stNewSize);
    if (pvSplit == LW_NULL) {
        return  (LW_VMM_MAP_FAILED);
    }
    
    pmapn->MAPN_stLen = stNewSize;
    API_VmmFreeArea(pvSplit);
    
    return  (pmapn->MAPN_pvAddr);
}
/*********************************************************************************************************
** 函数名称: __vmmMmapExpand
** 功能描述: 扩展一片虚拟内存大小
** 输　入  : pmapn         mmap node
**           stNewSize     需要设置的内存区域新大小
**           iMoveEn       是否可以重新分配.
** 输　出  : 分配的虚拟内存地址, LW_VMM_MAP_FAILED 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PVOID  __vmmMmapExpand (PLW_VMM_MAP_NODE  pmapn, size_t stNewSize, INT  iMoveEn)
{
    PVOID               pvRetAddr;
    PVOID               pvOldAddr;
    LW_DEV_MMAP_AREA    dmap;
    INT                 iError;
    
    if (API_VmmExpandArea(pmapn->MAPN_pvAddr, (stNewSize - pmapn->MAPN_stLen)) == ERROR_NONE) {
        pmapn->MAPN_stLen = stNewSize;
        return  (pmapn->MAPN_pvAddr);
    }
    
    if (!iMoveEn) {
        _ErrorHandle(ENOMEM);
        return  (LW_VMM_MAP_FAILED);
    }
    
    pvRetAddr = __vmmMapnMalloc(pmapn, stNewSize, 
                                pmapn->MAPN_iFlags, 
                                pmapn->MAPN_ulFlag);
    if (pvRetAddr == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_VMM_MAP_FAILED);
    }
    
    if (pmapn->MAPN_iFd >= 0) {
        dmap.DMAP_pvAddr   = pvRetAddr;
        dmap.DMAP_stLen    = stNewSize;
        dmap.DMAP_offPages = (pmapn->MAPN_off >> LW_CFG_VMM_PAGE_SHIFT);
        dmap.DMAP_ulFlag   = pmapn->MAPN_ulFlag;
        
        iError = API_IosMmap(pmapn->MAPN_iFd, &dmap);
        if ((iError < ERROR_NONE) && 
            (errno != ERROR_IOS_DRIVER_NOT_SUP)) {                      /*  驱动程序报告错误            */
            __vmmMapnFree(pvRetAddr);
            return  (LW_VMM_MAP_FAILED);
        }
        
        dmap.DMAP_pvAddr   = pmapn->MAPN_pvAddr;
        dmap.DMAP_stLen    = pmapn->MAPN_stLen;
        dmap.DMAP_offPages = (pmapn->MAPN_off >> LW_CFG_VMM_PAGE_SHIFT);
        dmap.DMAP_ulFlag   = pmapn->MAPN_ulFlag;
        
        API_IosUnmap(pmapn->MAPN_iFd, &dmap);                           /*  调用设备驱动                */
    }
    
    pvOldAddr = pmapn->MAPN_pvAddr;
    pmapn->MAPN_pvAddr = pvRetAddr;
    pmapn->MAPN_stLen  = stNewSize;
    
    API_VmmMoveArea(pvRetAddr, pvOldAddr);                              /*  将之前的物理页面映射到新内存*/
    
    __vmmMapnFree(pvOldAddr);                                           /*  释放之前的内存              */
    
    __vmmMapnUnlink(pmapn);                                             /*  重新插入管理链表            */
    
    __vmmMapnLink(pmapn);

    return  (pvRetAddr);
}
/*********************************************************************************************************
** 函数名称: __vmmMmapSplit
** 功能描述: 分割一片虚拟内存
** 输　入  : pmapn         mmap node
**           stSplit       分割位置
** 输　出  : 分割后新的 map node
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_VMM_MAP_NODE  __vmmMmapSplit (PLW_VMM_MAP_NODE  pmapn, size_t  stSplit)
{
    PLW_VMM_MAP_NODE    pmapnR;
    PVOID               pvR;
    
    pmapnR = (PLW_VMM_MAP_NODE)__SHEAP_ALLOC(sizeof(LW_VMM_MAP_NODE));  /*  创建控制块                  */
    if (pmapnR == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    
    pvR = API_VmmSplitArea(pmapn->MAPN_pvAddr, stSplit);
    if (pvR == LW_NULL) {
        __SHEAP_FREE(pmapnR);
        return  (LW_NULL);
    }
    
    if (pmapn->MAPN_iFd >= 0) {
        API_IosFdRefInc(pmapn->MAPN_iFd);                               /*  对文件描述符引用 ++         */
    }
    
    pmapnR->MAPN_pvAddr   = pvR;
    pmapnR->MAPN_stLen    = pmapn->MAPN_stLen - stSplit;
    pmapnR->MAPN_ulFlag   = pmapn->MAPN_ulFlag;
    pmapnR->MAPN_iFd      = pmapn->MAPN_iFd;
    pmapnR->MAPN_mode     = pmapn->MAPN_mode;
    pmapnR->MAPN_off      = pmapn->MAPN_off + stSplit;
    pmapnR->MAPN_offFSize = pmapn->MAPN_offFSize;
    pmapnR->MAPN_dev      = pmapn->MAPN_dev;
    pmapnR->MAPN_ino64    = pmapn->MAPN_ino64;
    pmapnR->MAPN_iFlags   = pmapn->MAPN_iFlags;
    pmapnR->MAPN_pid      = pmapn->MAPN_pid;
    
    pmapn->MAPN_stLen = stSplit;
    
    __vmmMapnLink(pmapnR);                                              /*  加入管理链表                */
    
    return  (pmapnR);
}
/*********************************************************************************************************
** 函数名称: __vmmMmapMerge
** 功能描述: 合并一片虚拟内存
** 输　入  : pmapnL        左侧 mmap node
**           pmapnR        右侧 mmap node
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __vmmMmapMerge (PLW_VMM_MAP_NODE  pmapnL, PLW_VMM_MAP_NODE  pmapnR)
{
    API_VmmMergeArea(pmapnL->MAPN_pvAddr, pmapnR->MAPN_pvAddr);
    
    pmapnL->MAPN_stLen += pmapnR->MAPN_stLen;
    
    __vmmMmapDelete(pmapnR);
}
/*********************************************************************************************************
** 函数名称: API_VmmMmap
** 功能描述: 内存文件映射.
** 输　入  : pvAddr        虚拟地址
**           stLen         映射长度
**           iFlags        LW_VMM_SHARED_CHANGE / LW_VMM_PRIVATE_CHANGE
**           ulFlag        LW_VMM_FLAG_READ | LW_VMM_FLAG_RDWR | LW_VMM_FLAG_EXEC
**           iFd           文件描述符
**           off           文件偏移量
** 输　出  : 分配的虚拟内存地址, LW_VMM_MAP_FAILED 表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
PVOID  API_VmmMmap (PVOID  pvAddr, size_t  stLen, INT  iFlags, ULONG  ulFlag, INT  iFd, off_t  off)
{
    PVOID   pvRet;
    
    if ((iFlags != LW_VMM_SHARED_CHANGE) && 
        (iFlags != LW_VMM_PRIVATE_CHANGE)) {
        _ErrorHandle(EINVAL);
        return  (LW_VMM_MAP_FAILED);
    }
    
    if (!ALIGNED(off, LW_CFG_VMM_PAGE_SIZE) || (stLen == 0)) {          /*  必须页对齐                  */
        _ErrorHandle(EINVAL);
        return  (LW_VMM_MAP_FAILED);
    }
    
    stLen = ROUND_UP(stLen, LW_CFG_VMM_PAGE_SIZE);                      /*  变成页面对齐大小            */
    
#if LW_CFG_CACHE_EN > 0
    if (iFlags == LW_VMM_SHARED_CHANGE) {
        if (API_CacheAliasProb()) {                                     /*  如果有 CACHE 别名可能       */
            ulFlag &= ~(LW_VMM_FLAG_CACHEABLE | LW_VMM_FLAG_BUFFERABLE);/*  共享内存不允许 CACHE        */
        }
    }
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    
    __VMM_MMAP_LOCK();
    if (pvAddr == LW_NULL) {                                            /*  分配新的内存                */
        pvRet = __vmmMmapNew(stLen, iFlags, ulFlag, iFd, off);
    
    } else {                                                            /*  修改之前映射的内存          */
        pvRet = __vmmMmapChange(pvAddr, stLen, iFlags, ulFlag, iFd, off);
    }
    __VMM_MMAP_UNLOCK();
    
    return  (pvRet);
}
/*********************************************************************************************************
** 函数名称: API_VmmMremap
** 功能描述: 重新设置内存区域的大小
** 输　入  : pvAddr        已经分配的虚拟内存地址
**           stOldSize     当前的内存区域大小
**           stNewSize     需要设置的内存区域新大小
**           iMoveEn       是否可以重新分配.
** 输　出  : 重新设置后的内存区域地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PVOID  API_VmmMremap (PVOID  pvAddr, size_t stOldSize, size_t stNewSize, INT  iMoveEn)
{
    PLW_VMM_MAP_NODE    pmapn;
    PVOID               pvRet;
    
    if (!ALIGNED(pvAddr, LW_CFG_VMM_PAGE_SIZE) || (stNewSize == 0)) {
        _ErrorHandle(EINVAL);
        return  (LW_VMM_MAP_FAILED);
    }
    
    stNewSize = ROUND_UP(stNewSize, LW_CFG_VMM_PAGE_SIZE);              /*  变成页面对齐大小            */
    
    __VMM_MMAP_LOCK();
    pmapn = __vmmMapnFindCur(pvAddr);
    if (pmapn == LW_NULL) {
        __VMM_MMAP_UNLOCK();
        _ErrorHandle(EINVAL);
        return  (LW_VMM_MAP_FAILED);
    }
    
    if ((pmapn->MAPN_pvAddr != pvAddr) ||
        (pmapn->MAPN_stLen  != stOldSize)) {                            /*  只支持完整空间操作          */
        __VMM_MMAP_UNLOCK();
        _ErrorHandle(ENOTSUP);
        return  (LW_VMM_MAP_FAILED);
    }
    
    if (pmapn->MAPN_stLen > stNewSize) {                                /*  进行拆分                    */
        pvRet = __vmmMmapShrink(pmapn, stNewSize);
        
    } else if (pmapn->MAPN_stLen < stNewSize) {                         /*  进行扩展                    */
        pvRet = __vmmMmapExpand(pmapn, stNewSize, iMoveEn);
    
    } else {
        pvRet = pvAddr;
    }
    __VMM_MMAP_UNLOCK();
    
    return  (pvRet);
}
/*********************************************************************************************************
** 函数名称: API_VmmMunmap
** 功能描述: 取消内存文件映射
** 输　入  : pvAddr        起始地址
**           stLen         映射长度
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_VmmMunmap (PVOID  pvAddr, size_t  stLen)
{
    PLW_VMM_MAP_NODE    pmapn, pmapnR, pmapnR2;
    size_t              stSplit;
    
    if (!ALIGNED(pvAddr, LW_CFG_VMM_PAGE_SIZE) || (stLen == 0)) {       /*  必须页对齐                  */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    stLen = ROUND_UP(stLen, LW_CFG_VMM_PAGE_SIZE);                      /*  变成页面对齐大小            */
    
    __VMM_MMAP_LOCK();
    pmapn = __vmmMapnFindCur(pvAddr);
    if (pmapn == LW_NULL) {
        __VMM_MMAP_UNLOCK();
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (((addr_t)pmapn->MAPN_pvAddr + pmapn->MAPN_stLen) <
        ((addr_t)pvAddr + stLen)) {                                     /*  内存越界                    */
        __VMM_MMAP_UNLOCK();
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pmapn->MAPN_pvAddr == pvAddr) {                                 /*  从起始地址开始释放          */
        if (pmapn->MAPN_stLen != stLen) {
            pmapnR = __vmmMmapSplit(pmapn, stLen);                      /*  建立一个右侧分段            */
            if (pmapnR == LW_NULL) {
                __VMM_MMAP_UNLOCK();
                return  (PX_ERROR);
            }
        }
        __vmmMmapDelete(pmapn);                                         /*  删除分段                    */
        
    } else {                                                            /*  从中间地址开始删除          */
        if (((addr_t)pmapn->MAPN_pvAddr + pmapn->MAPN_stLen) == 
            ((addr_t)pvAddr + stLen)) {                                 /*  删除后半段                  */
            stSplit = (addr_t)pvAddr - (addr_t)pmapn->MAPN_pvAddr;
            pmapnR = __vmmMmapSplit(pmapn, stSplit);                    /*  建立一个右侧分段            */
            if (pmapnR == LW_NULL) {
                __VMM_MMAP_UNLOCK();
                return  (PX_ERROR);
            }
            __vmmMmapDelete(pmapnR);                                    /*  删除右侧分段                */
        
        } else {                                                        /*  删除中段                    */
            stSplit = (addr_t)pvAddr - (addr_t)pmapn->MAPN_pvAddr;
            pmapnR  = __vmmMmapSplit(pmapn, stSplit);                   /*  建立一个右侧分段            */
            if (pmapnR == LW_NULL) {
                __VMM_MMAP_UNLOCK();
                return  (PX_ERROR);
            }
            
            pmapnR2 = __vmmMmapSplit(pmapnR, stLen);                    /*  建立一个右侧分段            */
            if (pmapnR2 == LW_NULL) {
                __vmmMmapMerge(pmapn, pmapnR);                          /*  分裂失败, 回复成之前的状态  */
                __VMM_MMAP_UNLOCK();
                return  (PX_ERROR);
            }
            __vmmMmapDelete(pmapnR);                                    /*  删除分段                    */
        }
    }
    __VMM_MMAP_UNLOCK();
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_MUNMAP,
                      pvAddr, stLen, LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_VmmMsync
** 功能描述: 将内存中映射的文件数据回写文件
** 输　入  : pvAddr        起始地址
**           stLen         映射长度
**           iInval        是否放弃文件回写
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_VmmMsync (PVOID  pvAddr, size_t  stLen, INT  iInval)
{
    PLW_VMM_MAP_NODE    pmapn;
    off_t               off;
    size_t              stWrite;
    
    if (!ALIGNED(pvAddr, LW_CFG_VMM_PAGE_SIZE) || (stLen == 0)) {       /*  必须页对齐                  */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __VMM_MMAP_LOCK();
    pmapn = __vmmMapnFindCur(pvAddr);
    if (pmapn == LW_NULL) {
        __VMM_MMAP_UNLOCK();
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!S_ISREG(pmapn->MAPN_mode)) {
        __VMM_MMAP_UNLOCK();
        return  (ERROR_NONE);                                           /*  非数据文件直接退出          */
    }
    
    if (((addr_t)pmapn->MAPN_pvAddr + pmapn->MAPN_stLen) <
        ((addr_t)pvAddr + stLen)) {                                     /*  内存越界                    */
        __VMM_MMAP_UNLOCK();
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    __VMM_MMAP_UNLOCK();                                                /*  直接释放锁                  */
    
    if (iInval) {
        API_VmmInvalidateArea(pmapn->MAPN_pvAddr, pvAddr, stLen);       /*  释放对应的物理内存          */
        
    } else if (pmapn->MAPN_iFlags == LW_VMM_SHARED_CHANGE) {            /*  SHARED 类型可以回写文件     */
        INT        i;
        ULONG      ulPageNum;
        size_t     stExcess;
        addr_t     ulAddr = (addr_t)pvAddr;
        ULONG      ulFlag;
        
        off = pmapn->MAPN_off 
            + (off_t)((addr_t)pvAddr - (addr_t)pmapn->MAPN_pvAddr);     /*  计算写入/无效文件偏移量     */
        
        stWrite = (size_t)(pmapn->MAPN_offFSize - off);
        stWrite = (stWrite > stLen) ? stLen : stWrite;                  /*  确定写入/无效文件的长度     */
        
        ulPageNum = (ULONG) (stWrite >> LW_CFG_VMM_PAGE_SHIFT);
        stExcess  = (size_t)(stWrite & ~LW_CFG_VMM_PAGE_MASK);
        
        for (i = 0; i < ulPageNum; i++) {
            if ((API_VmmGetFlag((PVOID)ulAddr, &ulFlag) == ERROR_NONE) &&
                (ulFlag & LW_VMM_FLAG_WRITABLE)) {                      /*  内存必须有效才可写          */
                if (pwrite(pmapn->MAPN_iFd, (CPVOID)ulAddr, 
                           LW_CFG_VMM_PAGE_SIZE, off) != 
                           LW_CFG_VMM_PAGE_SIZE) {
                    return  (PX_ERROR);
                }
            }
            ulAddr += LW_CFG_VMM_PAGE_SIZE;
            off    += LW_CFG_VMM_PAGE_SIZE;
        }
        
        if (stExcess) {
            if ((API_VmmGetFlag((PVOID)ulAddr, &ulFlag) == ERROR_NONE) &&
                (ulFlag & LW_VMM_FLAG_WRITABLE)) {                      /*  内存必须有效才可写          */
                if (pwrite(pmapn->MAPN_iFd, (CPVOID)ulAddr, 
                           stExcess, off) != stExcess) {                /*  写入文件                    */
                    return  (PX_ERROR);
                }
            }
        }
        
        ioctl(pmapn->MAPN_iFd, FIOSYNC);
    }
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_MSYNC,
                      pvAddr, stLen, iInval, LW_NULL);
                      
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_VmmMmapShow
** 功能描述: 显示当前系统映射的所有文件内存.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_VmmMmapShow (VOID)
{
#if LW_CFG_CPU_WORD_LENGHT == 64
    const CHAR          cMmapInfoHdr[] = 
    "      ADDR             SIZE            OFFSET      WRITE SHARE  PID   FD\n"
    "---------------- ---------------- ---------------- ----- ----- ----- ----\n";
#else
    const CHAR          cMmapInfoHdr[] = 
    "  ADDR     SIZE        OFFSET      WRITE SHARE  PID   FD\n"
    "-------- -------- ---------------- ----- ----- ----- ----\n";
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */

    PLW_VMM_MAP_NODE    pmapn;
    PLW_LIST_LINE       plineTemp;
    PCHAR               pcWrite;
    PCHAR               pcShare;
    
    printf(cMmapInfoHdr);
    
    __VMM_MMAP_LOCK();
    for (plineTemp  = _K_plineMapnMHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pmapn = _LIST_ENTRY(plineTemp, LW_VMM_MAP_NODE, MAPN_lineManage);
        
        if (pmapn->MAPN_ulFlag & LW_VMM_FLAG_WRITABLE) {
            pcWrite = "true ";
        } else {
            pcWrite = "false";
        }
        
        if (pmapn->MAPN_iFlags == LW_VMM_SHARED_CHANGE) {
            pcShare = "true ";
        } else {
            pcShare = "false";
        }
        
        printf("%08lx %8lx %16llx %s %s %5d %4d\n",
               (addr_t)pmapn->MAPN_pvAddr,
               (ULONG)pmapn->MAPN_stLen,
               pmapn->MAPN_off,
               pcWrite,
               pcShare,
               pmapn->MAPN_pid,
               pmapn->MAPN_iFd);
    }
    __VMM_MMAP_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: API_VmmMmapPCount
** 功能描述: 统计一个进程使用 mmap 物理内存的用量.
** 输　入  : pid          进程 ID 0 为内核
**           pstPhySize   物理内存使用大小
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_VmmMmapPCount (pid_t  pid, size_t  *pstPhySize)
{
             PLW_VMM_MAP_NODE    pmapn;
             PLW_LIST_LINE       plineTemp;
             ULONG               ulPageNum;
    REGISTER size_t              stPhySize = 0;
    
    if ((pid < 0) || (pstPhySize == LW_NULL)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __VMM_MMAP_LOCK();
    for (plineTemp  = _K_plineMapnMHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pmapn = _LIST_ENTRY(plineTemp, LW_VMM_MAP_NODE, MAPN_lineManage);
        if (pmapn->MAPN_pid == pid) {
            if (API_VmmPCountInArea(pmapn->MAPN_pvAddr, 
                                    &ulPageNum) == ERROR_NONE) {
                stPhySize += (size_t)(ulPageNum << LW_CFG_VMM_PAGE_SHIFT);
            }
        }
    }
    __VMM_MMAP_UNLOCK();
    
    *pstPhySize = stPhySize;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_VmmMmapReclaim
** 功能描述: 回收进程 mmap 内存
** 输　入  : pid       进程 id
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

LW_API 
VOID  API_VmmMmapReclaim (pid_t  pid)
{
    LW_LD_VPROC        *pvproc;
    PLW_VMM_MAP_NODE    pmapn;
    
    pvproc = vprocGet(pid);
    if (pvproc == LW_NULL) {
        return;
    }
    
    while (pvproc->VP_plineMap) {
        __VMM_MMAP_LOCK();
        pmapn = _LIST_ENTRY(pvproc->VP_plineMap, LW_VMM_MAP_NODE, MAPN_lineVproc);
        __vmmMapnReclaim(pmapn, pvproc);
        __vmmMapnFree(pmapn->MAPN_pvAddr);
        __VMM_MMAP_UNLOCK();
        
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_MUNMAP,
                          pmapn->MAPN_pvAddr, pmapn->MAPN_stLen, LW_NULL);
        __SHEAP_FREE(pmapn);
    }
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
