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
** 文   件   名: KernelObject.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 内核对象.

** BUG:
2012.03.30  将所有内核对象操作合并在这个文件.
2012.12.07  API_ObjectIsGlobal() 使用资源管理器判断.
2015.09.19  加入内核对象共享.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_OBJECT_SHARE_EN > 0
/*********************************************************************************************************
  内核对象共享池
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            OBJS_lineManage;
    UINT64                  OBJS_u64Key;
    LW_OBJECT_HANDLE        OBJS_ulHandle;
} LW_OBJECT_SHARE;
typedef LW_OBJECT_SHARE    *PLW_OBJECT_SHARE;
/*********************************************************************************************************
  内核对象共享链表
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _K_plineObjShare;
#endif                                                                  /*  LW_CFG_OBJECT_SHARE_EN > 0  */
/*********************************************************************************************************
** 函数名称: API_ObjectGetClass
** 功能描述: 获得对象类型
** 输　入  : 
** 输　出  : CLASS
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
UINT8  API_ObjectGetClass (LW_OBJECT_HANDLE  ulId)
{
    REGISTER UINT8    ucClass;

#if LW_CFG_ARG_CHK_EN > 0
    if (!ulId) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);
        return  (0);
    }
#endif
    
    ucClass = (UINT8)_ObjectGetClass(ulId);
    
    return  (ucClass);
}
/*********************************************************************************************************
** 函数名称: API_ObjectIsGlobal
** 功能描述: 获得对象是否为全局对象
** 输　入  : 
** 输　出  : 为全局对象，返回：LW_TRUE  否则，返回：LW_FALSE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
BOOL  API_ObjectIsGlobal (LW_OBJECT_HANDLE  ulId)
{
#if LW_CFG_MODULELOADER_EN > 0
    return  (__resHandleIsGlobal(ulId));
#else
    return  (LW_TRUE);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
}
/*********************************************************************************************************
** 函数名称: API_ObjectGetNode
** 功能描述: 获得对象所在的处理器号
** 输　入  : 
** 输　出  : 处理器号
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ObjectGetNode (LW_OBJECT_HANDLE  ulId)
{
    REGISTER ULONG    ulNode;
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!ulId) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);
        return  ((unsigned)(PX_ERROR));
    }
#endif

    ulNode = _ObjectGetNode(ulId);
    
    return  (ulNode);
}
/*********************************************************************************************************
** 函数名称: API_ObjectGetIndex
** 功能描述: 获得对象缓冲区内地址
** 输　入  : 
** 输　出  : 获得对象缓冲区内地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ObjectGetIndex (LW_OBJECT_HANDLE  ulId)
{
    REGISTER ULONG    ulIndex;
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!ulId) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);
        return  ((unsigned)(PX_ERROR));
    }
#endif

    ulIndex = _ObjectGetIndex(ulId);
    
    return  (ulIndex);
}
/*********************************************************************************************************
** 函数名称: API_ObjectShareAdd
** 功能描述: 将一个内核对象注册到共享池中
** 输　入  : ulId      内核对象 ID
**           u64Key    key
** 输　出  : 错误代码
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_OBJECT_SHARE_EN > 0

LW_API  
ULONG  API_ObjectShareAdd (LW_OBJECT_HANDLE  ulId, UINT64  u64Key)
{
    PLW_OBJECT_SHARE    pobjsNew;
    PLW_OBJECT_SHARE    pobjsFind;
    PLW_LIST_LINE       pline;
    ULONG               ulError = ERROR_NONE;
    
    if (ulId == LW_OBJECT_HANDLE_INVALID) {
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    pobjsNew = (PLW_OBJECT_SHARE)__KHEAP_ALLOC(sizeof(LW_OBJECT_SHARE));
    if (pobjsNew == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);
        return  (ERROR_KERNEL_LOW_MEMORY);
    }
    pobjsNew->OBJS_u64Key   = u64Key;
    pobjsNew->OBJS_ulHandle = ulId;
    
    __KERNEL_ENTER();
    for (pline  = _K_plineObjShare;
         pline != LW_NULL;
         pline  = _list_line_get_next(pline)) {
        
        pobjsFind = _LIST_ENTRY(pline, LW_OBJECT_SHARE, OBJS_lineManage);
        if (pobjsFind->OBJS_u64Key == u64Key) {
            ulError = ERROR_KERNEL_KEY_CONFLICT;
            break;
        }
    }
    if (pline == LW_NULL) {
        _List_Line_Add_Ahead(&pobjsNew->OBJS_lineManage, &_K_plineObjShare);
    }
    __KERNEL_EXIT();
    
    _ErrorHandle(ulError);
    return  (ulError);
}
/*********************************************************************************************************
** 函数名称: API_ObjectShareDelete
** 功能描述: 将一个内核对象从共享池中删除
** 输　入  : u64Key        key
** 输　出  : 错误代码
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ObjectShareDelete (UINT64  u64Key)
{
    PLW_OBJECT_SHARE    pobjsFind;
    PLW_LIST_LINE       pline;
    
    __KERNEL_ENTER();
    for (pline  = _K_plineObjShare;
         pline != LW_NULL;
         pline  = _list_line_get_next(pline)) {
        
        pobjsFind = _LIST_ENTRY(pline, LW_OBJECT_SHARE, OBJS_lineManage);
        if (pobjsFind->OBJS_u64Key == u64Key) {
            break;
        }
    }
    if (pline) {
        _List_Line_Del(&pobjsFind->OBJS_lineManage, &_K_plineObjShare);
    
    } else {
        pobjsFind = LW_NULL;
    }
    __KERNEL_EXIT();
    
    if (pobjsFind) {
        __KHEAP_FREE(pobjsFind);
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: API_ObjectShareFind
** 功能描述: 获得对象类型
** 输　入  : u64Key        key
** 输　出  : 查询到的内核对象
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
LW_OBJECT_HANDLE  API_ObjectShareFind (UINT64  u64Key)
{
    PLW_OBJECT_SHARE    pobjsFind;
    PLW_LIST_LINE       pline;
    LW_OBJECT_HANDLE    ulRet = LW_OBJECT_HANDLE_INVALID;
    
    __KERNEL_ENTER();
    for (pline  = _K_plineObjShare;
         pline != LW_NULL;
         pline  = _list_line_get_next(pline)) {
        
        pobjsFind = _LIST_ENTRY(pline, LW_OBJECT_SHARE, OBJS_lineManage);
        if (pobjsFind->OBJS_u64Key == u64Key) {
            ulRet = pobjsFind->OBJS_ulHandle;
            break;
        }
    }
    __KERNEL_EXIT();
    
    if (ulRet == LW_OBJECT_HANDLE_INVALID) {
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
    }
    
    return  (ulRet);
}

#endif                                                                  /*  LW_CFG_OBJECT_SHARE_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
