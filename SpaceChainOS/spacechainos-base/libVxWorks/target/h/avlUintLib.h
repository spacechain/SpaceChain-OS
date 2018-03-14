/**
 * @file
 * avl tree library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_AVLUINTLIB_H
#define __VXWORKS_AVLUINTLIB_H

#include "vxWorks.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Binary tree node definition that uses an unsigned integer as the sorting
 * key.
 */

typedef struct avlu_node {
    struct avlu_node *left;     /* pointer to the left subtree */
    struct avlu_node *right;    /* pointer to the right subtree */
    int    height;              /* height of the subtree rooted at this node */
    UINT   key;                 /* sorting key */
} AVLU_NODE;

typedef AVLU_NODE * AVLU_TREE;  /* points to the root node of the tree */

/* callback routines for avlUintTreeWalk */

typedef STATUS (*AVLU_CALLBACK)(AVLU_NODE *pNode, void *pArg);

/* function declarations */

STATUS      avlUintInsert(AVLU_TREE *pRoot, AVLU_NODE *pNode);
AVLU_NODE  *avlUintDelete(AVLU_TREE *pRoot, UINT key);
AVLU_NODE  *avlUintSearch(AVLU_TREE root, UINT key);
AVLU_NODE  *avlUintSuccessorGet(AVLU_TREE root, UINT key);
AVLU_NODE  *avlUintPredecessorGet(AVLU_TREE root, UINT key);
AVLU_NODE  *avlUintMinimumGet(AVLU_TREE root);
AVLU_NODE  *avlUintMaximumGet(AVLU_TREE root);
STATUS      avlUintTreeWalk(AVLU_TREE pRoot, AVLU_CALLBACK preRtn,
                            void *preArg, AVLU_CALLBACK inRtn, void *inArg,
                            AVLU_CALLBACK postRtn, void *postArg);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_AVLUINT64LIB_H */
