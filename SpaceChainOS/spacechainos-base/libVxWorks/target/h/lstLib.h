/**
 * @file
 * list library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_LSTLIB_H
#define __VXWORKS_LSTLIB_H

#include "vxWorksCommon.h"

typedef struct __node {         /* Node of a linked list. */
    struct __node *next;        /* Points at the next node in the list */
    struct __node *previous;    /* Points at the previous node in the list */
} NODE;

typedef struct {        /* Header for a linked list. */
    NODE *HEAD;         /* Header list node */
    NODE *TAIL;         /* Tail */
    int count;          /* Number of nodes in list */
} LIST;

#ifdef __cplusplus
extern "C" {
#endif

void    lstLibInit(void);
NODE *  lstFirst(LIST *pList);
NODE *  lstGet(LIST *pList);
NODE *  lstLast(LIST *pList);
NODE *  lstNStep(NODE *pNode, int nStep);
NODE *  lstNext(NODE *pNode);
NODE *  lstNth(LIST *pList, int nodenum);
NODE *  lstPrevious(NODE *pNode);
int     lstCount(LIST *pList);
int     lstFind(LIST *pList, NODE *pNode);
void    lstAdd(LIST *pList, NODE *pNode);
void    lstConcat(LIST *pDstList, LIST *pAddList);
void    lstDelete(LIST *pList, NODE *pNode);
void    lstExtract(LIST *pSrcList, NODE *pStartNode, NODE *pEndNode,
                   LIST *pDstList);
void    lstFree(LIST *pList);
void    lstInit(LIST *pList);
void    lstInsert(LIST *pList, NODE *pPrev, NODE *pNode);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_LSTLIB_H */
