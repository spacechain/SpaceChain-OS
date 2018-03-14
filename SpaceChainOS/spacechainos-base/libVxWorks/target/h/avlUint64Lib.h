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

#ifndef __VXWORKS_AVLUINT64LIB_H
#define __VXWORKS_AVLUINT64LIB_H

#include "vxWorks.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Binary tree node definition that uses an unsigned long as the sorting
 * key.
 */

typedef struct avlUint64_node {
    struct avlUint64_node *left;    /* pointer to the left subtree */
    struct avlUint64_node *right;   /* pointer to the right subtree */
    UINT64 key;                     /* sorting key */
    int    height;                  /* height of the subtree rooted at this node */
} AVLUINT64_NODE;

typedef AVLUINT64_NODE * AVLUINT64_TREE;    /* points to the root node of the tree */

/* callback routines for avlUint64TreeWalk */

typedef STATUS (*AVLUINT64_CALLBACK)(AVLUINT64_NODE *pNode, void *pArg);

/* function declarations */

STATUS           avlUint64Insert(AVLUINT64_TREE *pRoot,
                                 AVLUINT64_NODE *pNode);
AVLUINT64_NODE  *avlUint64Delete(AVLUINT64_TREE *pRoot, UINT64 key);
AVLUINT64_NODE  *avlUint64Search(AVLUINT64_TREE root, UINT64 key);
AVLUINT64_NODE  *avlUint64SuccessorGet(AVLUINT64_TREE root, UINT64 key);
AVLUINT64_NODE  *avlUint64PredecessorGet(AVLUINT64_TREE root, UINT64 key);
AVLUINT64_NODE  *avlUint64MinimumGet(AVLUINT64_TREE root);
AVLUINT64_NODE  *avlUint64MaximumGet(AVLUINT64_TREE root);
STATUS           avlUint64TreeWalk(AVLUINT64_TREE pRoot,
                                   AVLUINT64_CALLBACK preRtn, void *preArg,
                                   AVLUINT64_CALLBACK inRtn, void *inArg,
                                   AVLUINT64_CALLBACK postRtn, void *postArg);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_AVLUINT64LIB_H */
