/**
 * @file
 * double link list library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_DLLLIB_H
#define __VXWORKS_DLLLIB_H

#include "vxWorks.h"
#include "types/vxTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Vx_dlnode {     /* Node of a linked list. */
    struct _Vx_dlnode *next;    /* Points at the next node in the list */
    struct _Vx_dlnode *previous;/* Points at the previous node in the list */
} _Vx_DL_NODE;

typedef struct {        /* Header for a linked list. */
    _Vx_DL_NODE *head;  /* header of list */
    _Vx_DL_NODE *tail;  /* tail of list */
} _Vx_DL_LIST;

typedef _Vx_DL_NODE DL_NODE;
typedef _Vx_DL_LIST DL_LIST;

/*
 * basic operation
 */
#define DLL_INIT(list) {                    \
    {                                       \
        ((DL_LIST *)(list))->head = NULL;   \
        ((DL_LIST *)(list))->tail = NULL;   \
    }

#define DLL_FIRST(pList)            \
    ((((DL_LIST *)(pList))->head))

#define DLL_LAST(pList)             \
    ((((DL_LIST *)(pList))->tail))

#define DLL_NEXT(pNode)             \
    ((((DL_NODE *)(pNode))->next))

#define DLL_PREVIOUS(pNode)         \
    ((((DL_NODE *)(pNode))->previous))

#define DLL_EMPTY(pList)            \
    ((((DL_LIST *)pList)->head == NULL))

#define DLL_INSERT(list, previousNode, node)                        \
    {                                                               \
        DL_NODE *temp;                                              \
        if ((previousNode) == NULL) {                               \
            temp = ((DL_LIST *)(list))->head;                       \
            ((DL_LIST *)(list))->head = (DL_NODE *)(node);          \
        } else {                                                    \
            temp = ((DL_NODE *)(previousNode))->next;               \
            ((DL_NODE *)(previousNode))->next = (DL_NODE *)(node);  \
        }                                                           \
        if (temp == NULL) {                                         \
            ((DL_LIST *)(list))->tail = (DL_NODE *)(node);          \
        } else {                                                    \
           temp->previous = (DL_NODE *)(node);                      \
        }                                                           \
        ((DL_NODE *)(node))->next = temp;                           \
        ((DL_NODE *)(node))->previous = (DL_NODE *)(previousNode);  \
    }

#define DLL_ADD(list, node)                     \
    {                                           \
        DL_NODE *listTail = (list)->tail;       \
        DLL_INSERT ((list), listTail, (node));  \
    }

#define DLL_REMOVE(list, node)                                                      \
    {                                                                               \
        if (((DL_NODE *)(node))->previous == NULL) {                                \
            ((DL_LIST *)(list))->head = ((DL_NODE *)(node))->next;                  \
        } else {                                                                    \
            ((DL_NODE *)(node))->previous->next = ((DL_NODE *)(node))->next;        \
        }                                                                           \
        if (((DL_NODE *)(node))->next == NULL) {                                    \
            ((DL_LIST *)(list))->tail = ((DL_NODE *)(node))->previous;              \
        } else {                                                                    \
            ((DL_NODE *)(node))->next->previous = ((DL_NODE *)(node))->previous;    \
        }                                                                           \
    }

#define DLL_GET(list, node)                                         \
    {                                                               \
        (node) = (void *)((DL_LIST *)(list))->head;                 \
        if ((node) != NULL) {                                       \
            ((DL_LIST *)(list))->head = ((DL_NODE *)(node))->next;  \
            if (((DL_NODE *)(node))->next == NULL) {                \
                ((DL_LIST *)(list))->tail = NULL;                   \
            } else {                                                \
                ((DL_NODE *)(node))->next->previous = NULL;         \
            }                                                       \
        }                                                           \
    }

/* function declarations */

extern DL_LIST *dllCreate(void);
extern DL_NODE *dllEach(DL_LIST *pList,
                        BOOL (*routine)(DL_NODE * pNode, _Vx_usr_arg_t arg),
                        _Vx_usr_arg_t routineArg);
extern DL_NODE *dllGet(DL_LIST *pList);
extern STATUS   dllDelete(DL_LIST *pList);
extern STATUS   dllInit(DL_LIST *pList);
extern STATUS   dllTerminate(DL_LIST *pList);
extern int      dllCount(DL_LIST *pList);
extern void     dllAdd(DL_LIST *pList, DL_NODE *pNode);
extern void     dllInsert(DL_LIST *pList, DL_NODE *pPrev, DL_NODE *pNode);
extern void     dllRemove(DL_LIST *pList, DL_NODE *pNode);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_DLLLIB_H */
