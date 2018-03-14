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

#include <stdlib.h>
#include "dllLib.h"

/*
 * dll create
 */
DL_LIST *dllCreate (void)
{
    FAST DL_LIST *pList = (DL_LIST *)malloc(sizeof (DL_LIST));

    dllInit(pList);

    return  (pList);
}

/*
 * dll delete
 */
STATUS dllDelete (DL_LIST *pList)    /* pointer to list head to be initialized */
{
    free(pList);    /* free list */
    return  (OK);
}

/*
 * dllInit - initialize doubly linked list descriptor
 */
STATUS dllInit (FAST DL_LIST *pList)    /* pointer to list descriptor to be initialized */
{
    pList->head = NULL;
    pList->tail = NULL;

    return  (OK);
}

/*
 * dllTerminate - terminate doubly linked list head
 */
STATUS dllTerminate (DL_LIST *pList)    /* pointer to list head to be initialized */
{
    return  (OK);
}

/*
 * dllInsert - insert node in list after specified node
 */
void dllInsert (FAST DL_LIST *pList,        /* pointer to list descriptor */
                FAST DL_NODE *pPrev,        /* pointer to node after which to insert */
                FAST DL_NODE *pNode)        /* pointer to node to be inserted */
{
    FAST DL_NODE *pNext;

    /* Verify arguments */

    if ((pNode == NULL) || (pList == NULL)) {
        return;
    }

    if (pPrev == NULL) {    /* new node is to be first in list */
        pNext = pList->head;
        pList->head = pNode;

    } else {                /* make prev node point fwd to new */
        pNext = pPrev->next;
        pPrev->next = pNode;
    }

    if (pNext == NULL) {
        pList->tail = pNode;        /* new node is to be last in list */

    } else {
        pNext->previous = pNode;    /* make next node point back to new */
    }


    /* set pointers in new node */
    pNode->next     = pNext;
    pNode->previous = pPrev;
}

/*
 * dllAdd - add node to end of list
 */
void dllAdd (DL_LIST *pList,     /* pointer to list descriptor */
             DL_NODE *pNode)     /* pointer to node to be added */
{
    dllInsert(pList, pList->tail, pNode);
}

/*
 * dllRemove - remove specified node in list
 */
void dllRemove (DL_LIST *pList,             /* pointer to list descriptor */
                DL_NODE *pNode)             /* pointer to node to be deleted */
{
    if (pNode->previous == NULL) {
        pList->head = pNode->next;

    } else {
        pNode->previous->next = pNode->next;
    }

    if (pNode->next == NULL) {
        pList->tail = pNode->previous;

    } else {
        pNode->next->previous = pNode->previous;
    }

    pNode->next = pNode->previous = NULL;
}
/*
 * dllGet - get (delete and return) first node from list
 */
DL_NODE *dllGet (FAST DL_LIST *pList)        /* pointer to list from which to get node */
{
    FAST DL_NODE *pNode = pList->head;

    if (pNode != NULL) {                    /* is list empty? */
        pList->head = pNode->next;          /* make next node be 1st */

        if (pNode->next == NULL) {          /* is there any next node? */
            pList->tail = NULL;             /*   no - list is empty */

        } else {
            pNode->next->previous = NULL;   /*   yes - make it 1st node */
        }
    }

    return  (pNode);
}

/*
 * dllCount - report number of nodes in list
 */
int dllCount (DL_LIST *pList)     /* pointer to list descriptor */
{
    FAST DL_NODE *pNode = DLL_FIRST(pList);
    FAST int count = 0;

    while (pNode != NULL) {
        count++;
        pNode = DLL_NEXT (pNode);
    }

    return  (count);
}

/*
 * dllEach - call a routine for each node in a linked list
 */
DL_NODE *dllEach (DL_LIST *pList,           /* linked list of nodes to call routine for */
                  BOOL   (*routine)(        /* the routine to call for each list node */
                  DL_NODE *pNode,           /* pointer to a linked list node */
                  _Vx_usr_arg_t arg),       /* arbitrary user-supplied argument */
                  _Vx_usr_arg_t routineArg) /* arbitrary user-supplied argument */
{
    FAST DL_NODE *pNode = DLL_FIRST(pList);

    while ((pNode != NULL) && ((*routine)(pNode, routineArg))) {
        pNode = DLL_NEXT(pNode);
    }

    return  (pNode);    /* return node we ended with */
}

/*
 * end
 */
