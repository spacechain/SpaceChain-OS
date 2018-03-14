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

#ifndef __VXWORKS_AVLLIB_H
#define __VXWORKS_AVLLIB_H

#include "vxWorks.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct avl_node {
    struct avl_node *left;      /* pointer to the left subtree */
    struct avl_node *right;     /* pointer to the right subtree */
    int    height;              /* height of the subtree rooted at this node */
} AVL_NODE;

typedef AVL_NODE *AVL_TREE;     /* points to the root node of the tree */

typedef STATUS (*AVL_COMPARE)(AVL_NODE *pNode, void *pKey);

/* callback routines for avlUintTreeWalk */

typedef STATUS (*AVL_CALLBACK)(AVL_NODE *pNode, void *pArg);

/* function declarations */

STATUS     avlInsert(AVL_TREE *pRoot, AVL_NODE *pNode, void *pKey,
                     AVL_COMPARE cmpRtn);
AVL_NODE  *avlDelete(AVL_TREE *pRoot, void *pKey, AVL_COMPARE cmpRtn);
AVL_NODE  *avlSearch(AVL_TREE root, void *pKey, AVL_COMPARE cmpRtn);
AVL_NODE  *avlSuccessorGet(AVL_TREE root, void *pKey, AVL_COMPARE cmpRtn);
AVL_NODE  *avlPredecessorGet(AVL_TREE root, void *pKey, AVL_COMPARE cmpRtn);
AVL_NODE  *avlMinimumGet(AVL_TREE root);
AVL_NODE  *avlMaximumGet(AVL_TREE root);
STATUS     avlTreeWalk(AVL_TREE pRoot, AVL_CALLBACK preRtn, void *preArg,
                       AVL_CALLBACK inRtn, void *inArg,
                       AVL_CALLBACK postRtn, void *postArg);
STATUS     avlInsertInform(AVL_TREE *pRoot, void *pNewNode, void *key,
                           void **ppKeyHolder, AVL_COMPARE cmpRtn);
void      *avlRemoveInsert(AVL_TREE *pRoot, void *pNewNode, void *key,
                           AVL_COMPARE cmpRtn);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_AVLLIB_H */
