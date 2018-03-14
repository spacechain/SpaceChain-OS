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
** 文   件   名: loader_malloc.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2010 年 04 月 17 日
**
** 描        述: 内存管理

** BUG: 
2011.03.04  如果 vmmMalloc 没有成功则转而使用 __SHEAP_ALLOC().
2011.05.24  开始加入 loader 对代码段的共享功能.
2013.12.21  加入 LW_VMM_FLAG_EXEC 权限支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
/*********************************************************************************************************
  内核模块内存管理
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: __ldMalloc
** 功能描述: 分配内存(大容量, 装载 .ko 内核模块时使用此内存).
** 输　入  : stLen         分配内存长度
** 输　出  : 分配好的内存指针
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PVOID  __ldMalloc (size_t  stLen)
{
#if LW_CFG_VMM_EN > 0
    PVOID    pvMem = API_VmmMallocEx(stLen, LW_VMM_FLAG_EXEC | LW_VMM_FLAG_RDWR);
    
    if (pvMem == LW_NULL) {
        pvMem =  __SHEAP_ALLOC_ALIGN(stLen, LW_CFG_VMM_PAGE_SIZE);
    }
    return  (pvMem);
#else
    return  (__SHEAP_ALLOC_ALIGN(stLen, LW_CFG_VMM_PAGE_SIZE));
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: __ldMallocAlign
** 功能描述: 分配内存(大容量, 装载 .ko 内核模块时使用此内存).
** 输　入  : stLen         分配内存长度
**           stAlign       内存对齐关系
** 输　出  : 分配好的内存指针
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PVOID  __ldMallocAlign (size_t  stLen, size_t  stAlign)
{
#if LW_CFG_VMM_EN > 0
    PVOID    pvMem = API_VmmMallocAlign(stLen, stAlign, LW_VMM_FLAG_EXEC | LW_VMM_FLAG_RDWR);
    
    if (pvMem == LW_NULL) {
        pvMem =  __SHEAP_ALLOC_ALIGN(stLen, stAlign);
    }
    return  (pvMem);
#else
    return  (__SHEAP_ALLOC_ALIGN(stLen, stAlign));
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: __ldFree
** 功能描述: 释放内存(大容量, 装载 .ko 内核模块时使用此内存).
** 输　入  : pvAddr        释放内存指针
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID   __ldFree (PVOID  pvAddr)
{
#if LW_CFG_VMM_EN > 0
    if (API_VmmVirtualIsInside((addr_t)pvAddr)) {
        API_VmmFree(pvAddr);
    } else {
        __SHEAP_FREE(pvAddr);
    }
#else
    __SHEAP_FREE(pvAddr);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
  共享库内存管理 (动态链接库共享代码空间)
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
/*********************************************************************************************************
  共享区域
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE             ESHM_lineManage;                           /*  管理链表                    */
    dev_t                    ESHM_dev;                                  /*  对应文件的 dev_t            */
    ino64_t                  ESHM_ino64;                                /*  对应文件的 ino64_t          */
    PVOID                    ESHM_pvBase;                               /*  基址                        */
    size_t                   ESHM_stSize;                               /*  大小                        */
} LW_LD_EXEC_SHARE;
/*********************************************************************************************************
  共享 hash 表区域
*********************************************************************************************************/
#define EXEC_SHARE_HASH_SIZE    32
#define EXEC_SHARE_HASH_MASK    (EXEC_SHARE_HASH_SIZE - 1)
#define EXEC_SHARE_HASH(key)    (((key) >> 4) & EXEC_SHARE_HASH_MASK)
static LW_LIST_LINE_HEADER      _G_plineExecShare[EXEC_SHARE_HASH_SIZE];
/*********************************************************************************************************
  共享库内存管理锁
*********************************************************************************************************/
LW_OBJECT_HANDLE                _G_ulExecShareLock;
#define EXEC_SHARE_LOCK()       API_SemaphoreMPend(_G_ulExecShareLock, LW_OPTION_WAIT_INFINITE)
#define EXEC_SHARE_UNLOCK()     API_SemaphoreMPost(_G_ulExecShareLock)
/*********************************************************************************************************
  共享库内存共享使能
*********************************************************************************************************/
static BOOL                     _G_bShareSegEn  = LW_TRUE;              /*  默认允许共享功能            */
#define EXEC_SHARE_SET(bShare)  (_G_bShareSegEn = bShare);
#define EXEC_SHARE_EN()         (_G_bShareSegEn = LW_TRUE);
#define EXEC_SHARE_DIS()        (_G_bShareSegEn = LW_FALSE);
#define EXEC_SHARE_IS_EN()      (_G_bShareSegEn)
/*********************************************************************************************************
  共享段临时填充函数参数
*********************************************************************************************************/
typedef struct {
    INT                      EFILLA_iFd;                                /*  文件描述符                  */
    off_t                    EFILLA_oftOffset;                          /*  共享区域初始文件偏移量      */
    PVOID                    EFILLA_pvShare;                            /*  共享区域起始地址            */
    size_t                   EFILLA_stSize;                             /*  共享区域大小                */
    INT                      EFILLA_iRet;                               /*  填充返回值                  */
} LW_LD_EXEC_FILLER_ARG;
/*********************************************************************************************************
** 函数名称: __ldExecShareFindByBase
** 功能描述: 通过内存基址查询共享段信息
** 输　入  : pvBase        基址
** 输　出  : 共享段信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_LD_EXEC_SHARE *__ldExecShareFindByBase (PVOID  pvBase)
{
    INT                  iHash = EXEC_SHARE_HASH((INT)pvBase);
    PLW_LIST_LINE        plineTemp;
    LW_LD_EXEC_SHARE    *pexecshare;
    
    for (plineTemp  = _G_plineExecShare[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pexecshare = _LIST_ENTRY(plineTemp, LW_LD_EXEC_SHARE, ESHM_lineManage);
        if (pexecshare->ESHM_pvBase == pvBase) {
            break;
        }
    }
    
    if (plineTemp) {
        return  (pexecshare);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: __ldExecShareFindByBase
** 功能描述: 通过文件信息查询共享段信息
** 输　入  : dev           设备
**           ino64         ino64_t
** 输　出  : 共享段信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_LD_EXEC_SHARE *__ldExecShareFindByFile (dev_t  dev, ino64_t  ino64)
{
    INT                  i;
    PLW_LIST_LINE        plineTemp;
    LW_LD_EXEC_SHARE    *pexecshare;
    
    for (i = 0; i < EXEC_SHARE_HASH_SIZE; i++) {
        for (plineTemp  = _G_plineExecShare[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            pexecshare = _LIST_ENTRY(plineTemp, LW_LD_EXEC_SHARE, ESHM_lineManage);
            if ((pexecshare->ESHM_dev == dev) && (pexecshare->ESHM_ino64 == ino64)) {
                break;
            }
        }
        if (plineTemp) {
            break;
        }
    }
    
    if (plineTemp) {
        return  (pexecshare);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: __ldExecShareCreate
** 功能描述: 创建共享段信息
** 输　入  : pvBase        基址
**           stSize        空间大小
**           pstat64       文件属性
** 输　出  : 共享段信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_LD_EXEC_SHARE *__ldExecShareCreate (PVOID  pvBase, size_t  stSize, 
                                              dev_t  dev,    ino64_t ino64)
{
    INT                  iHash = EXEC_SHARE_HASH((INT)pvBase);
    LW_LD_EXEC_SHARE    *pexecshare;
    
    pexecshare = (LW_LD_EXEC_SHARE *)__SHEAP_ALLOC(sizeof(LW_LD_EXEC_SHARE));
    if (pexecshare == LW_NULL) {
        return  (LW_NULL);
    }
    lib_bzero(pexecshare, sizeof(LW_LD_EXEC_SHARE));
    
    pexecshare->ESHM_dev    = dev;
    pexecshare->ESHM_ino64  = ino64;
    pexecshare->ESHM_pvBase = pvBase;
    pexecshare->ESHM_stSize = stSize;
    
    _List_Line_Add_Ahead(&pexecshare->ESHM_lineManage, &_G_plineExecShare[iHash]);
    
    return  (pexecshare);
}
/*********************************************************************************************************
** 函数名称: __ldExecShareDelete
** 功能描述: 删除共享段信息
** 输　入  : pexecshare    共享段信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __ldExecShareDelete (LW_LD_EXEC_SHARE *pexecshare)
{
    INT     iHash = EXEC_SHARE_HASH((INT)pexecshare->ESHM_pvBase);
    
    _List_Line_Del(&pexecshare->ESHM_lineManage, &_G_plineExecShare[iHash]);
    
    __SHEAP_FREE(pexecshare);
}
/*********************************************************************************************************
** 函数名称: __ldExecShareDeleteAll
** 功能描述: 删除所有共享段信息
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __ldExecShareDeleteAll (VOID)
{
    INT                  i;
    PLW_LIST_LINE        plineTemp;
    LW_LD_EXEC_SHARE    *pexecshare;
    
    for (i = 0; i < EXEC_SHARE_HASH_SIZE; i++) {
        plineTemp = _G_plineExecShare[i];
        while (plineTemp) {
            pexecshare = _LIST_ENTRY(plineTemp, LW_LD_EXEC_SHARE, ESHM_lineManage);
            plineTemp  = _list_line_get_next(plineTemp);
            
            _List_Line_Del(&pexecshare->ESHM_lineManage, &_G_plineExecShare[i]);
            __SHEAP_FREE(pexecshare);
        }
    }
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
** 函数名称: __ldMallocArea
** 功能描述: 分配内存(大容量, 装载 .so 共享库时使用此内存). 此内存仅是虚拟空间, 没有对应的物理内存
** 输　入  : stLen         分配内存长度
** 输　出  : 分配好的内存指针
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PVOID  __ldMallocArea (size_t  stLen)
{
#if LW_CFG_VMM_EN > 0
    return  (API_VmmMallocAreaEx(stLen, LW_NULL, LW_NULL, 
                                 LW_VMM_PRIVATE_CHANGE, 
                                 LW_VMM_FLAG_EXEC | LW_VMM_FLAG_RDWR));
#else
    return  (__SHEAP_ALLOC_ALIGN(stLen, LW_CFG_VMM_PAGE_SIZE));
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: __ldMallocAreaAlign
** 功能描述: 分配内存(大容量, 装载 .so 共享库时使用此内存). 此内存仅是虚拟空间, 没有对应的物理内存
** 输　入  : stLen         分配内存长度
**           stAlign       内存对齐关系
** 输　出  : 分配好的内存指针
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PVOID  __ldMallocAreaAlign (size_t  stLen, size_t  stAlign)
{
#if LW_CFG_VMM_EN > 0
#ifndef LW_VMM_PRIVATE_CHANGE
#define LW_VMM_PRIVATE_CHANGE 2
#endif                                                                  /*  LW_VMM_PRIVATE_CHANGE       */
    return  (API_VmmMallocAreaAlign(stLen, stAlign, LW_NULL, LW_NULL, 
                                    LW_VMM_PRIVATE_CHANGE, 
                                    LW_VMM_FLAG_EXEC | LW_VMM_FLAG_RDWR));
#else
    return  (__SHEAP_ALLOC_ALIGN(stLen, stAlign));
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: __ldFreeArea
** 功能描述: 释放内存(大容量, 装载 .so 共享库时使用此内存). 仅仅释放虚拟空间
** 输　入  : pvAddr        释放内存指针
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID   __ldFreeArea (PVOID  pvAddr)
{
#if LW_CFG_VMM_EN > 0
    LW_LD_EXEC_SHARE    *pexecshare;

    EXEC_SHARE_LOCK();
    pexecshare = __ldExecShareFindByBase(pvAddr);
    if (pexecshare) {
        __ldExecShareDelete(pexecshare);
    }
    EXEC_SHARE_UNLOCK();
    
    API_VmmFreeArea(pvAddr);
#else
    __SHEAP_FREE(pvAddr);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: __ldPrealloc
** 功能描述: 预先填充内存区域
** 输　入  : pvBase         __ldMallocArea 或者 __ldMallocAreaAlign 返回的地址
**           stAddrOft      地址偏移量
**           iFd            文件描述符
**           pstat64        文件属性
**           oftOffset      文件偏移量
**           stLen          长度
**           bCanShared     此段是否允许共享, 如果允许, 则需要将此段设为只读, 这样当修改此段时将会产生异常
**                          这时异常程序会将异常的段标示为 private 属性. private 属性的物理页面是不允许
                            共享的.
**           bCanExec       是否可以执行.
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

static INT  __ldPrealloc (PVOID  pvBase, size_t  stAddrOft, 
                          INT  iFd, off_t  oftOffset, size_t  stLen, 
                          BOOL  bCanShared, BOOL  bCanExec)
{
    PVOID       pvStart = (PVOID)((PCHAR)pvBase + stAddrOft);
    ssize_t     sstRet;
    INT         iRet;
    ULONG       ulFlag;
    
    if (bCanExec) {
        ulFlag = LW_VMM_FLAG_EXEC;
    
    } else {
        ulFlag = 0ul;
    }
    
    if (API_VmmPreallocArea(pvBase, pvStart, stLen, LW_NULL, LW_NULL, 
                            ulFlag | LW_VMM_FLAG_RDWR)) {
        return  (PX_ERROR);
    }
    
    if (iFd >= 0) {
        sstRet = pread(iFd, pvStart, stLen, oftOffset);
        iRet   = (sstRet == (ssize_t)stLen) ? (ERROR_NONE) : (PX_ERROR);
    
    } else {
        lib_bzero(pvStart, stLen);
        iRet   = ERROR_NONE;
    }
    
    /*
     *  如果为共享代码段, 则此区域为只读/可执行权限, 一旦有修改操作, 因为此段内存对修改私有化(MAP_PRIVATE)
     *  则异常处理程序会将相应的位设置为已被修改状态, 以后的 share 操作将不会再共享相应的物理页面.
     */
    if (bCanShared && EXEC_SHARE_IS_EN()) {                             /*  需要判断 share 是否使能     */
        API_VmmPreallocArea(pvBase, pvStart, stLen, LW_NULL, LW_NULL, 
                            ulFlag | LW_VMM_FLAG_READ);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __ldTempFiller
** 功能描述: 共享区域临时文件内容填充函数
** 输　入  : pearg              参数
**           ulDestPageAddr     拷贝目标地址 (页面对齐)
**           ulMapPageAddr      最终会被映射的目标地址 (页面对齐)
**           ulPageNum          新分配出的页面个数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ldTempFiller (LW_LD_EXEC_FILLER_ARG *pearg, 
                            addr_t                 ulDestPageAddr, 
                            addr_t                 ulMapPageAddr,
                            ULONG                  ulPageNum)
{
    off_t       offtRead;
    size_t      stReadLen;
    addr_t      ulMapStartAddr = (addr_t)pearg->EFILLA_pvShare;
    
    if ((ulMapPageAddr < ulMapStartAddr) || 
        (ulMapPageAddr > (ulMapStartAddr + pearg->EFILLA_stSize))) {    /*  致命错误, 页面内存地址错误  */
        pearg->EFILLA_iRet = PX_ERROR;
        return  (PX_ERROR);
    }
    
    offtRead  = (off_t)(ulMapPageAddr - ulMapStartAddr);                /*  内存地址偏移                */
    offtRead += pearg->EFILLA_oftOffset;                                /*  加上文件起始偏移            */
    
    stReadLen = (size_t)(ulPageNum << LW_CFG_VMM_PAGE_SHIFT);           /*  需要获取的数据大小          */
    
    if (pearg->EFILLA_iFd >= 0) {
        ssize_t  sstNum = pread(pearg->EFILLA_iFd, 
                                (PCHAR)ulDestPageAddr, stReadLen,
                                offtRead);
        if (sstNum != (ssize_t)stReadLen) {
            pearg->EFILLA_iRet = PX_ERROR;
        }
    } else {
        lib_bzero((PCHAR)ulDestPageAddr, stReadLen);
    }
    
    return  (pearg->EFILLA_iRet);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
** 函数名称: __ldMmap
** 功能描述: 装载共享库时使用映射方式获取可执行文件信息, 如果已经存在这样的共享库, 则只读段共享, 如果不存
             在则新建共享区, 可写区域则新建
** 输　入  : pvBase         __ldMallocArea 或者 __ldMallocAreaAlign 返回的地址
**           stAddrOft      地址偏移量
**           iFd            文件描述符
**           pstat64        文件属性
**           oftOffset      文件偏移量
**           stLen          长度
**           bCanShare      LW_TRUE 表示是可以共享, LW_FALSE 不可共享数据段.
**           bCanExec       是否可以执行.
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  __ldMmap (PVOID  pvBase, size_t  stAddrOft, INT  iFd, struct stat64 *pstat64,
               off_t  oftOffset, size_t  stLen,  BOOL  bCanShare, BOOL  bCanExec)
{
#if LW_CFG_VMM_EN > 0
    LW_LD_EXEC_SHARE    *pexecshare;
    INT                  iRet;
    ULONG                ulError;
    
    if (bCanShare == LW_FALSE) {                                        /*  私有数据区                  */
        return  (__ldPrealloc(pvBase, stAddrOft, 
                              iFd, oftOffset, stLen, 
                              bCanShare, bCanExec));                    /*  预分配物理内存且填充数据    */
    
    } else {                                                            /*  共享代码区                  */
        EXEC_SHARE_LOCK();
        pexecshare = __ldExecShareFindByFile(pstat64->st_dev, pstat64->st_ino);
        if (pexecshare && EXEC_SHARE_IS_EN()) {                         /*  存在可共享区域              */
            size_t      stAddrOftAlign = ROUND_UP(stAddrOft, LW_CFG_VMM_PAGE_SIZE);
            size_t      stExcess       = stAddrOftAlign - stAddrOft;
            size_t      stLenAlign;
            
            if (stExcess) {                                             /*  起始地址页面不对齐          */
                iRet = __ldPrealloc(pvBase, stAddrOft, 
                                    iFd, oftOffset, stExcess, 
                                    bCanShare, bCanExec);
                if (iRet) {
                    EXEC_SHARE_UNLOCK();
                    return  (PX_ERROR);
                }
                stAddrOft  = stAddrOftAlign;
                oftOffset += stExcess;
                stLen     -= stExcess;
            }
            
            stLenAlign = ROUND_DOWN(stLen, LW_CFG_VMM_PAGE_SIZE);
            stExcess   = stLen - stLenAlign;
            
            if (stLenAlign) {                                           /*  至少存在一页可以共享的内存  */
                LW_LD_EXEC_FILLER_ARG   earg;
                
                earg.EFILLA_iFd       = iFd;
                earg.EFILLA_oftOffset = oftOffset;
                earg.EFILLA_pvShare   = (PCHAR)pvBase + stAddrOftAlign;
                earg.EFILLA_stSize    = stLenAlign;
                earg.EFILLA_iRet      = ERROR_NONE;
                
                ulError = API_VmmShareArea(pexecshare->ESHM_pvBase, pvBase, 
                                           stAddrOftAlign, stAddrOftAlign, 
                                           stLenAlign,
                                           bCanExec,
                                           __ldTempFiller, 
                                           (PVOID)&earg);               /*  对齐的一段内的内存共享      */
                
                if (ulError || (earg.EFILLA_iRet != ERROR_NONE)) {      /*  共享或者填充错误            */
                    EXEC_SHARE_UNLOCK();
                    return  (PX_ERROR);
                }
            }
            EXEC_SHARE_UNLOCK();
            
            stAddrOft += stLenAlign;
            oftOffset += stLenAlign;
            
            if (stExcess) {                                             /*  结束地址页面不对齐          */
                return  (__ldPrealloc(pvBase, stAddrOft, 
                                      iFd, oftOffset, stExcess, 
                                      bCanShare, bCanExec));
            } else {
                return  (ERROR_NONE);
            }
        } else {                                                        /*  需要自行加载                */
            EXEC_SHARE_UNLOCK();                                        /*  预分配物理内存且填充数据    */
            return  (__ldPrealloc(pvBase, stAddrOft, 
                                  iFd, oftOffset, stLen, 
                                  bCanShare, bCanExec));
        }
    }
#else
    ssize_t     sstRet;
    PVOID       pvStart;
    
    (VOID)pstat64;
    (VOID)bCanShare;
    pvStart = (PVOID)((PCHAR)pvBase + stAddrOft);
    if (iFd >= 0) {
        sstRet  = pread(iFd, pvStart, stLen, oftOffset);
        return  ((sstRet == (ssize_t)stLen) ? (ERROR_NONE) : (PX_ERROR));
    
    } else {
        lib_bzero(pvStart, stLen);
        return  (ERROR_NONE);
    }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: __ldShare
** 功能描述: 允许共享当前共享库
** 输　入  : pvBase         __ldMallocArea 或者 __ldMallocAreaAlign 返回的地址 (__ldMmap 已经装载完成)
**           stLen          空间大小
**           dev            dev_t
**           ino64          ino64_t
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __ldShare (PVOID  pvBase, size_t  stLen, dev_t  dev, ino64_t ino64)
{
#if LW_CFG_VMM_EN > 0
    EXEC_SHARE_LOCK();
    if (EXEC_SHARE_IS_EN()) {
        __ldExecShareCreate(pvBase, stLen, dev, ino64);
    }
    EXEC_SHARE_UNLOCK();
#else
    (VOID)pvBase;
    (VOID)stLen;
    (VOID)dev;
    (VOID)ino64;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: __ldShareAbort
** 功能描述: 禁止共享指定文件的共享库 
             文件可能已经发生改变, 这时需要停止共享相关的库, 
             仅针对 new1 型文件系统有效, 所以应用程序包括动态库, 最好放在 New1 型文件系统上, 以防止升级对
             应用程序的破坏.
** 输　入  : dev            dev_t (-1 表示清除所有缓冲区)
**           ino64          ino64_t
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __ldShareAbort (dev_t  dev, ino64_t  ino64)
{
#if LW_CFG_VMM_EN > 0
    LW_LD_EXEC_SHARE    *pexecshare;
    
    EXEC_SHARE_LOCK();
    if ((dev == (dev_t)-1) && (ino64 == (ino64_t)-1)) {                 /*  清除所有共享区              */
        __ldExecShareDeleteAll();
    } else {
        pexecshare = __ldExecShareFindByFile(dev, ino64);
        while (pexecshare) {
            __ldExecShareDelete(pexecshare);
            pexecshare = __ldExecShareFindByFile(dev, ino64);
        }
    }
    EXEC_SHARE_UNLOCK();
#else
    (VOID)dev;
    (VOID)ino64;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: __ldShareConfig
** 功能描述: 设置共享功能的使能, 
             由于有些仿真器对于 MMU 只读段调试断点调试可能存在问题, 此时可以暂时关闭这个此功能.
** 输　入  : bShareEn       是否使能 share 功能
**           pbPrev         之前的状态
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  __ldShareConfig (BOOL  bShareEn, BOOL  *pbPrev)
{
#if LW_CFG_VMM_EN > 0
    EXEC_SHARE_LOCK();
    if (pbPrev) {
        *pbPrev = EXEC_SHARE_IS_EN();
    }
    EXEC_SHARE_SET(bShareEn);
    EXEC_SHARE_UNLOCK();
#else
    if (pbPrev) {
        *pbPrev = LW_FALSE;
    }
    if (bShareEn) {
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
