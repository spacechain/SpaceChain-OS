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
** 文   件   名: listOp.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 这是系统使用的所有链表低级操作定义。

** BUG
2007.11.13  加入获取链表下一个节点的内联函数, 完全封装链表操作.
2009.04.09  加入顺序资源链, 预防句柄卷绕.
2012.12.07  加入对双向链表节点是否在链表中的判断宏.
2013.11.29  指针转整型使用 size_t 类型.
*********************************************************************************************************/

#ifndef  __LISTOP_H
#define  __LISTOP_H

#include "../SylixOS/kernel/list/listType.h"

/*********************************************************************************************************
  链表结构初始化
*********************************************************************************************************/

#define	_LIST_RING_INIT(name)	 { LW_NULL, LW_NULL }
#define _LIST_LINE_INIT(name)    { LW_NULL, LW_NULL }
#define _LIST_MONO_INIT(name)    { LW_NULL }

/*********************************************************************************************************
  链表结构初始化
*********************************************************************************************************/

#define	_LIST_RING_INIT_IN_CODE(name) do {                    \
            (name).RING_plistNext = LW_NULL;                  \
            (name).RING_plistPrev = LW_NULL;                  \
        } while (0)
#define _LIST_LINE_INIT_IN_CODE(name) do {                    \
            (name).LINE_plistNext = LW_NULL;                  \
            (name).LINE_plistPrev = LW_NULL;                  \
        } while (0)
#define _LIST_MONO_INIT_IN_CODE(name) do {                    \
            (name).MONO_plistNext = LW_NULL;                  \
        } while (0)
        
/*********************************************************************************************************
  含有变量定义的链表结构初始化
*********************************************************************************************************/

#define _LIST_RING_HEAD(name)                                 \
        LW_LIST_RING name = _LIST_RING_INIT(name)
#define _LIST_LINE_HEAD(name)                                 \
        LW_LIST_LINE name = _LIST_LINE_INIT(name)
#define _LIST_MONO_HEAD(name)                                 \
        LW_LIST_MONO name = _LIST_MONO_INIT(name)
        
/*********************************************************************************************************
  链表指针初始化
*********************************************************************************************************/

#define _INIT_LIST_RING_HEAD(ptr) do {                        \
            (ptr)->RING_plistNext = LW_NULL;                  \
            (ptr)->RING_plistPrev = LW_NULL;                  \
        } while (0)
#define _INIT_LIST_LINE_HEAD(ptr) do {                        \
            (ptr)->LINE_plistNext = LW_NULL;                  \
            (ptr)->LINE_plistPrev = LW_NULL;                  \
        } while (0)
#define _INIT_LIST_MONO_HEAD(ptr) do {                        \
            (ptr)->MONO_plistNext = LW_NULL;                  \
        } while (0)
        
/*********************************************************************************************************
  偏移量计算
*********************************************************************************************************/

#define _LIST_OFFSETOF(type, member)                          \
        ((size_t)&((type *)0)->member)
        
/*********************************************************************************************************
  得到ptr的容器结构
*********************************************************************************************************/

#define _LIST_CONTAINER_OF(ptr, type, member)                 \
        ((type *)((size_t)ptr - _LIST_OFFSETOF(type, member)))
        
/*********************************************************************************************************
  获得链表母体指针结构
*********************************************************************************************************/

#define _LIST_ENTRY(ptr, type, member)                        \
        _LIST_CONTAINER_OF(ptr, type, member)
        
/*********************************************************************************************************
  判断链表是否为空
*********************************************************************************************************/

#define _LIST_RING_IS_EMPTY(ptr)                              \
        ((ptr) == LW_NULL)
#define _LIST_LINE_IS_EMPTY(ptr)                              \
        ((ptr) == LW_NULL)
#define _LIST_MONO_IS_EMPTY(ptr)                              \
        ((ptr) == LW_NULL)
        
/*********************************************************************************************************
  判断节点是否不在链表中
*********************************************************************************************************/

#define _LIST_RING_IS_NOTLNK(ptr)                             \
        (((ptr)->RING_plistNext == LW_NULL) || ((ptr)->RING_plistPrev == LW_NULL))
#define _LIST_LINE_IS_NOTLNK(ptr)                             \
        (((ptr)->LINE_plistNext == LW_NULL) && ((ptr)->LINE_plistPrev == LW_NULL))

/*********************************************************************************************************
  资源表初始化连接
*********************************************************************************************************/

#define _LIST_MONO_LINK(ptr, ptrnext)                         \
        ((ptr)->MONO_plistNext = (ptrnext))

/*********************************************************************************************************
  下一个
*********************************************************************************************************/

static LW_INLINE VOID _list_ring_next (PLW_LIST_RING  *phead)
{
    *phead = (*phead)->RING_plistNext;
}
static LW_INLINE VOID _list_line_next (PLW_LIST_LINE  *phead)
{
    *phead = (*phead)->LINE_plistNext;
}
static LW_INLINE VOID _list_mono_next (PLW_LIST_MONO  *phead)
{
    *phead = (*phead)->MONO_plistNext;
}

/*********************************************************************************************************
  获取下一个
*********************************************************************************************************/

static LW_INLINE PLW_LIST_RING    _list_ring_get_next (PLW_LIST_RING  pring)
{
    return  (pring->RING_plistNext);
}
static LW_INLINE PLW_LIST_LINE    _list_line_get_next (PLW_LIST_LINE  pline)
{
    return  (pline->LINE_plistNext);
}
static LW_INLINE PLW_LIST_MONO    _list_mono_get_next (PLW_LIST_MONO  pmono)
{
    return  (pmono->MONO_plistNext);
}

/*********************************************************************************************************
  获取上一个
*********************************************************************************************************/

static LW_INLINE PLW_LIST_RING    _list_ring_get_prev (PLW_LIST_RING  pring)
{
    return  (pring->RING_plistPrev);
}
static LW_INLINE PLW_LIST_LINE    _list_line_get_prev (PLW_LIST_LINE  pline)
{
    return  (pline->LINE_plistPrev);
}

/*********************************************************************************************************
  资源缓冲池分配与回收操作
*********************************************************************************************************/

static LW_INLINE PLW_LIST_MONO _list_mono_allocate (PLW_LIST_MONO  *phead)
{
    REGISTER  PLW_LIST_MONO  pallo;
    
    pallo  = *phead;
    *phead = (*phead)->MONO_plistNext;
    
    return  (pallo);
}
static LW_INLINE VOID _list_mono_free (PLW_LIST_MONO  *phead, PLW_LIST_MONO  pfree)
{
    pfree->MONO_plistNext = *phead;
    *phead = pfree;
}

/*********************************************************************************************************
  资源缓冲池分配与回收操作 (顺序资源链, 预防句柄卷绕)
*********************************************************************************************************/

static LW_INLINE PLW_LIST_MONO _list_mono_allocate_seq (PLW_LIST_MONO  *phead, PLW_LIST_MONO  *ptail)
{
    REGISTER  PLW_LIST_MONO  pallo;
    
    pallo = *phead;
    if (*ptail == *phead) {
        *ptail = (*phead)->MONO_plistNext;
    }
    *phead = (*phead)->MONO_plistNext;
    
    return  (pallo);
}
static LW_INLINE VOID _list_mono_free_seq (PLW_LIST_MONO  *phead, PLW_LIST_MONO  *ptail, PLW_LIST_MONO  pfree)
{
    pfree->MONO_plistNext = LW_NULL;
    if (*ptail) {                                                       /*  存在有空闲节点时            */
        (*ptail)->MONO_plistNext = pfree;
    } else {
        *phead = pfree;                                                 /*  没有空闲节点                */
    }
    *ptail = pfree;
}

#endif                                                                  /*  __LISTOP_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
