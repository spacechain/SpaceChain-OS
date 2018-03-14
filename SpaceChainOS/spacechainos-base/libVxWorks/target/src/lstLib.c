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

#include <stdlib.h>
#include "lstLib.h"

/*
 * list lib init
 */
void lstLibInit (void)
{
}

/*
 * get first node
 */
NODE *lstFirst (LIST *pList)
{
    return  (pList->HEAD);
}

/*
 * get one node and unlink from list
 */
NODE *lstGet (LIST *pList)
{
    FAST NODE *pNode = pList->HEAD;

    if (pNode != NULL) {                    /* is list empty? */
        pList->HEAD = pNode->next;          /* make next node be 1st */

        if (pNode->next == NULL) {          /* is there any next node? */
            pList->TAIL = NULL;             /*   no - list is empty */
        } else {
            pNode->next->previous = NULL;   /*   yes - make it 1st node */
        }
        pList->count--;                     /* update node count */
    }

    return (pNode);
}

/*
 * get last node
 */
NODE *lstLast (LIST *pList)
{
    return  (pList->TAIL);
}

/*
 * go N step
 */
NODE *lstNStep (NODE *pNode, int nStep)
{
    int i;
    int absStep = lib_abs(nStep);

    for (i = 0; i < absStep; i++) {
        if (nStep < 0) {
            pNode = pNode->previous;
        } else if (nStep > 0) {
            pNode = pNode->next;
        }
        if (pNode == NULL) {
            break;
        }
    }

    return (pNode);
}

/*
 * get next node
 */
NODE *lstNext (NODE *pNode)
{
    return  (pNode->next);
}

/*
 * get
 */
NODE *lstNth (LIST *pList, int nodenum)
{
    FAST NODE *pNode;

    /* verify node number is in list */

    if ((nodenum < 1) || (nodenum > pList->count)) {
        return (NULL);
    }


    /* if nodenum is less than half way, look forward from beginning;
    otherwise look back from end */

    if (nodenum < (pList->count >> 1)) {
        pNode = pList->HEAD;

        while (--nodenum > 0) {
            pNode = pNode->next;
        }
    } else {
        nodenum -= pList->count;
        pNode = pList->TAIL;

        while (nodenum++ < 0) {
            pNode = pNode->previous;
        }
    }

    return (pNode);
}

/*
 * get prev
 */
NODE *lstPrevious (NODE *pNode)
{
    return  (pNode->previous);
}

/*
 * get list node num
 */
int  lstCount (LIST *pList)
{
    return  (pList->count);
}

/*
 * find node
 */
int  lstFind (LIST *pList, NODE *pNode)
{
    FAST NODE *pNextNode;
    FAST int index = 1;

    pNextNode = lstFirst(pList);

    while ((pNextNode != NULL) && (pNextNode != pNode)) {
        index++;
        pNextNode = lstNext(pNextNode);
    }

    if (pNextNode == NULL) {
        return (ERROR);
    } else {
        return (index);
    }
}

/*
 * add a node
 */
void  lstAdd (LIST *pList, NODE *pNode)
{
    lstInsert(pList, pList->TAIL, pNode);
}

/*
 * cat list
 */
void  lstConcat (LIST *pDstList, LIST *pAddList)
{
    if (pAddList->count == 0) {     /* nothing to do if AddList is empty */
        return;
    }

    if (pDstList->count == 0) {
        *pDstList = *pAddList;
    } else {
        /* both lists non-empty; update DstList pointers */

        pDstList->TAIL->next     = pAddList->HEAD;
        pAddList->HEAD->previous = pDstList->TAIL;
        pDstList->TAIL           = pAddList->TAIL;

        pDstList->count += pAddList->count;
    }

    /* make AddList empty */

    lstInit(pAddList);
}

/*
 * delete a node
 */
void  lstDelete (LIST *pList, NODE *pNode)
{
    if (pNode->previous == NULL) {
        pList->HEAD = pNode->next;
    } else {
        pNode->previous->next = pNode->next;
    }

    if (pNode->next == NULL) {
        pList->TAIL = pNode->previous;
    } else {
        pNode->next->previous = pNode->previous;
    }

    /* update node count */

    pList->count--;
}

/*
 * extract a sublist from a list
 */
void  lstExtract (LIST *pSrcList, NODE *pStartNode, NODE *pEndNode, LIST *pDstList)
{
    FAST int i;
    FAST NODE *pNode;

    /* fix pointers in original list */

    if (pStartNode->previous == NULL) {
        pSrcList->HEAD = pEndNode->next;
    } else {
        pStartNode->previous->next = pEndNode->next;
    }

    if (pEndNode->next == NULL) {
        pSrcList->TAIL = pStartNode->previous;
    } else {
        pEndNode->next->previous = pStartNode->previous;
    }


    /* fix pointers in extracted list */

    pDstList->HEAD = pStartNode;
    pDstList->TAIL = pEndNode;

    pStartNode->previous = NULL;
    pEndNode->next       = NULL;


    /* count number of nodes in extracted list and update counts in lists */

    i = 0;

    for (pNode = pStartNode; pNode != NULL; pNode = pNode->next) {
        i++;
    }

    pSrcList->count -= i;
    pDstList->count = i;
}

/*
 * free
 */
void  lstFree2 (LIST *pList, VOIDFUNCPTR freeFunc)
{
    NODE *p1, *p2;

    if (pList->count > 0) {
        p1 = pList->HEAD;
        while (p1 != NULL) {
            p2 = p1->next;
            freeFunc((char *)p1);
            p1 = p2;
        }
        pList->count = 0;
        pList->HEAD = pList->TAIL = NULL;
    }
}

/*
 * free
 */
void  lstFree (LIST *pList)
{
    lstFree2(pList, free);
}

/*
 * init list
 */
void  lstInit (LIST *pList)
{
    pList->HEAD  = NULL;
    pList->TAIL  = NULL;
    pList->count = 0;
}

/*
 * insert a node into list
 */
void  lstInsert (LIST *pList, NODE *pPrev, NODE *pNode)
{
    FAST NODE *pNext;

    if (pPrev == NULL) {    /* new node is to be first in list */
        pNext = pList->HEAD;
        pList->HEAD = pNode;
    } else {                /* make prev node point fwd to new */
        pNext = pPrev->next;
        pPrev->next = pNode;
    }

    if (pNext == NULL) {
        pList->TAIL = pNode;        /* new node is to be last in list */
    } else {
        pNext->previous = pNode;    /* make next node point back to new */
    }


    /* set pointers in new node, and update node count */

    pNode->next     = pNext;
    pNode->previous = pPrev;

    pList->count++;
}

/*
 * end
 */
