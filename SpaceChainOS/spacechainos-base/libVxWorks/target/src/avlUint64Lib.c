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

#include "avlUint64Lib.h"

/* defines */

#define AVLUINT64_MAX_HEIGHT 28 /* can't have more than 2**28 nodes of */
                /* 16 bytes in 4GB */


/* forward declarations */

LOCAL void avlUint64Rebalance(AVLUINT64_NODE ***ancestors, int count);

/*
 * avlUint64Insert - insert a node in an AVL tree
 */
STATUS avlUint64Insert (AVLUINT64_TREE *pRoot,     /* pointer to the root node ptr */
                        AVLUINT64_NODE *pNewNode)  /* ptr to the node we want to insert */
{
    AVLUINT64_NODE **ppNode;                         /* ptr to current node ptr */
    AVLUINT64_NODE **ancestor[AVLUINT64_MAX_HEIGHT]; /* ancestor list */
    int              ancestorCount;                  /* number of ancestors */
    UINT64           key;

    if ((pNewNode == NULL) || (pRoot == NULL)) {
        return  (ERROR);
    }

    key = pNewNode->key;
    ppNode = pRoot;
    ancestorCount = 0;

    /* Find the leaf node where to add the new node */

    while (ancestorCount < AVLUINT64_MAX_HEIGHT) {
        AVLUINT64_NODE * pNode; /* pointer to the current node */

        pNode = *ppNode;
        if (pNode == NULL) {
            break;      /* we can insert a leaf node here */
        }

        ancestor[ancestorCount++] = ppNode;

        if (key == pNode->key) {
            return  (ERROR);

        } else if (key < pNode->key) {
            ppNode = &(pNode->left);

        } else {
            ppNode = &(pNode->right);
        }
    }

    if (ancestorCount == AVLUINT64_MAX_HEIGHT) {
        return  (ERROR);
    }

    /* initialize pNewNode */

    ((AVLUINT64_NODE *)pNewNode)->left = NULL;
    ((AVLUINT64_NODE *)pNewNode)->right = NULL;
    ((AVLUINT64_NODE *)pNewNode)->height = 1;

    /* add as new leaf */

    *ppNode = pNewNode;

    avlUint64Rebalance(ancestor, ancestorCount);

    return  (OK);
}

/*
 * avlUint64Delete - delete a node in an AVL tree
 */
AVLUINT64_NODE * avlUint64Delete (AVLUINT64_TREE *pRoot, /* pointer to the root node pointer */
                                  UINT64          key)   /* search key of node we want to delete */
{
    AVLUINT64_NODE **ppNode;            /* ptr to current node ptr */
    AVLUINT64_NODE  *pNode = NULL;      /* ptr to the current node */
    AVLUINT64_NODE **ancestor[AVLUINT64_MAX_HEIGHT];
                                        /* ancestor node pointer list */
    int              ancestorCount;     /* number of ancestors */
    AVLUINT64_NODE  *pDelete;           /* ptr to the node to be deleted */

    ppNode = pRoot;
    ancestorCount = 0;

    /* find node to be deleted */

    while (ancestorCount < AVLUINT64_MAX_HEIGHT) {
        pNode = *ppNode;
        if (pNode == NULL) {
            return  (NULL);  /* node was not in the tree ! */
        }

        ancestor[ancestorCount++] = ppNode;

        if (key == pNode->key) {
            break;      /* we found the node we have to delete */

        } else if (key < pNode->key) {
            ppNode = &(pNode->left);

        } else {
            ppNode = &(pNode->right);
        }
    }

    if (ancestorCount == AVLUINT64_MAX_HEIGHT) {
        return  (NULL);
    }

    pDelete = pNode;

    if (pNode->left == NULL) {
        /*
         * There is no node on the left subtree of delNode.
         * Either there is one (and only one, because of the balancing rules)
         * on its right subtree, and it replaces delNode, or it has no child
         * nodes at all and it just gets deleted
         */
        *ppNode = pNode->right;

        /*
         * we know that pNode->right was already balanced so we don't have to
         * check it again
         */
        ancestorCount--;

    } else {
        /*
         * We will find the node that is just before delNode in the ordering
         * of the tree and promote it to delNode's position in the tree.
         */

        AVLUINT64_NODE **ppDelete;      /* ptr to the ptr to the node
                                           we have to delete */
        int       deleteAncestorCount;  /* place where the replacing
                                           node will have to be
                                           inserted in the ancestor
                                           list */

        deleteAncestorCount = ancestorCount;
        ppDelete = ppNode;
        pDelete  = pNode;

        /* search for node just before delNode in the tree ordering */

        ppNode = &(pNode->left);

        while (ancestorCount < AVLUINT64_MAX_HEIGHT) {
            pNode = *ppNode;
            if (pNode->right == NULL) {
                break;
            }

            ancestor[ancestorCount++] = ppNode;
            ppNode = &(pNode->right);
        }

        if (ancestorCount == AVLUINT64_MAX_HEIGHT) {
            return  (NULL);
        }

        /*
         * this node gets replaced by its (unique, because of balancing rules)
         * left child, or deleted if it has no children at all.
         */

        *ppNode = pNode->left;

        /* now this node replaces delNode in the tree */

        pNode->left = pDelete->left;
        pNode->right = pDelete->right;
        pNode->height = pDelete->height;
        *ppDelete = pNode;

        /*
         * We have replaced delNode with pNode. Thus the pointer to the left
         * subtree of delNode was stored in delNode->left and it is now
         * stored in pNode->left. We have to adjust the ancestor list to
         * reflect this.
         */
        ancestor[deleteAncestorCount] = &(pNode->left);
    }

    avlUint64Rebalance((AVLUINT64_NODE ***)ancestor, ancestorCount);

    return  (pDelete);
}

/*
 * avlUint64Search - search a node in an AVL tree
 */
AVLUINT64_NODE * avlUint64Search (AVLUINT64_TREE  root,       /* root node pointer */
                                  UINT64          key)    /* search key */
{
    AVLUINT64_NODE *pNode;  /* pointer to the current node */

    pNode = root;

    /* search node that has matching key */

    while (pNode != NULL) {
        if (key == pNode->key) {
            return  (pNode); /* found the node */

        } else if (key < pNode->key) {
            pNode = pNode->left;

        } else {
            pNode = pNode->right;
        }
    }

    /* not found, return NULL */

    return  (NULL);
}

/*
 * avlUint64SuccessorGet - find node with key successor to input key
 */
AVLUINT64_NODE *avlUint64SuccessorGet (AVLUINT64_TREE  root,    /* root node pointer */
                                       UINT64          key)     /* search key */
{
    AVLUINT64_NODE *pNode;      /* pointer to the current node */
    AVLUINT64_NODE *pSuccessor; /* pointer to the current successor */

    pNode = root;
    pSuccessor = NULL;

    while (pNode != NULL) {
        if (key >= pNode->key) {
            pNode = pNode->right;

        } else {
            pSuccessor = pNode;
            pNode = pNode->left;
        }
    }

    return  (pSuccessor);
}

/*
 * avlUint64PredecessorGet - find node with key predecessor to input key
 */
AVLUINT64_NODE *avlUint64PredecessorGet (AVLUINT64_TREE  root,       /* root node pointer */
                                         UINT64          key)    /* search key */
{
    AVLUINT64_NODE *pNode;      /* pointer to the current node */
    AVLUINT64_NODE *pPred;      /* pointer to the current predecessor */

    pNode = root;
    pPred = NULL;

    while (pNode != NULL) {
        if (key <= pNode->key) {
            pNode = pNode->left;

        } else {
            pPred = pNode;
            pNode = pNode->right;
        }
    }

    return  (pPred);
}

/*
 * avlUint64MinimumGet - find node with smallest key
 */
AVLUINT64_NODE *avlUint64MinimumGet (AVLUINT64_TREE  root)  /* root node pointer */
{
    if (NULL == root) {
        return  (NULL);
    }

    while (root->left != NULL) {
        root = root->left;
    }

    return  (root);
}

/*
 * avlUint64MaximumGet - find node with largest key
 */
AVLUINT64_NODE * avlUint64MaximumGet (AVLUINT64_TREE  root) /* root node pointer */
{
    if (NULL == root) {
        return (NULL);
    }

    while (root->right != NULL) {
        root = root->right;
    }

    return  (root);
}

/*
 * avlUint64TreeWalk - walk the tree and execute selected functions on each node
 */
STATUS avlUint64TreeWalk (AVLUINT64_TREE      root,       /* root node pointer */
                          AVLUINT64_CALLBACK  preRtn,     /* pre-order routine */
                          void               *preArg,     /* pre-order argument */
                          AVLUINT64_CALLBACK  inRtn,      /* in-order routine */
                          void               *inArg,      /* in-order argument */
                          AVLUINT64_CALLBACK  postRtn,    /* post-order routine */
                          void               *postArg)    /* post-order argument */
{
#ifndef AVLUINT64_RECURSIVE_WALK

    AVLUINT64_NODE *pNode; /* pointer to the current node */
    ULONG           nodeStack[2 * AVLUINT64_MAX_HEIGHT];
    UINT            ix = 0;

    if (NULL == root) {
        return  (OK);
    }

    /* first do the pre-order and in-order routines */

    if ((preRtn != NULL) || (inRtn != NULL))
    {
        pNode = root;

        /*
         * The following algorithm can do pre-order and in-order, but
         * not post-order.
         */

        while (ix < 2 * AVLUINT64_MAX_HEIGHT) {
            while (pNode != NULL) {
                /* call pre-order if needed */

                if (preRtn != NULL) {
                    if (preRtn(pNode, preArg) == ERROR) {
                        return  (ERROR);
                    }
                }

                /* push on the stack */

                nodeStack[ix++] = (ULONG) pNode;

                if (ix == AVLUINT64_MAX_HEIGHT) {
                    return  (ERROR);
                }

                pNode = pNode->left;
            }

            if (ix == 0) {
                break;

            } else {
                AVLUINT64_NODE *right;

                /* pop from stack */

                pNode = (AVLUINT64_NODE *)nodeStack[--ix];

                /* call in-order if needed */

                right = pNode->right;

                if (inRtn != NULL) {
                    if (inRtn(pNode, inArg) == ERROR) {
                        return  (ERROR);
                    }
                }

                pNode = right;
            }
        }
    }

    /* do post-order if needed. */

    if (postRtn != NULL) {
        /*
         * The following algorithm can do pre-order and post-order but
         * not in-order. In this case, it is only used for post-order.
         */

        ix = 0;
        pNode = root;
        nodeStack[ix++] = (ULONG) pNode;

        while (ix > 0) {
            /* pop out a node */

            ix--;
            pNode  = (AVLUINT64_NODE *) (nodeStack[ix] & -1UL);
            if ((nodeStack[ix] & 0x01) == 0) {
                /* first pass, so push it back */

                nodeStack[ix++] = (ULONG) pNode | 1;

                /* check for stack overflow in case of corrupted tree */

                if ((ix + 2) >= 2 * AVLUINT64_MAX_HEIGHT) {
                    return  (ERROR);
                }

                /* push right and left */

                if (pNode->right != NULL) {
                    nodeStack[ix++] = (ULONG) pNode->right;
                }

                if (pNode->left != NULL) {
                    nodeStack[ix++] = (ULONG) pNode->left;
                }

            } else {
            /* do post Rtn */

                if (postRtn (pNode, postArg) == ERROR) {
                    return  (ERROR);
                }
            }
        }
    }

    return  (OK);

#else
    if (NULL == root) {
        return (OK);
    }

    /* call pre-order routine */

    if (preRtn != NULL) {
        if (preRtn(root, preArg) == ERROR) {
            return  (ERROR);
        }
    }

    /* walk left side */

    if (!(NULL == root->left)) {
        if (avlUint64TreeWalk(root->left, preRtn, preArg, inRtn, inArg,
                              postRtn, postArg) == ERROR) {
            return  (ERROR);
        }
    }

    /* call in-order routine */

    if (inRtn != NULL) {
        if (inRtn(root, inArg) == ERROR) {
            return  (ERROR);
        }
    }

    /* walk right side */

    if (!(NULL == root->right)) {
        if (avlUint64TreeWalk(root->right, preRtn, preArg, inRtn, inArg,
                              postRtn, postArg) == ERROR) {
            return  (ERROR);
        }
    }

    /* call post-order routine */

    if (postRtn != NULL) {
        if (postRtn(root, postArg) == ERROR) {
            return  (ERROR);
        }
    }

    return  (OK);
#endif
}

/*
 * avlUint64Rebalance - rebalance an AVL tree
 */
LOCAL void avlUint64Rebalance (AVLUINT64_NODE ***ancestors,  /* ancestor list */
                               int               count)      /* number ancestors to rebalance */
{
    while (count > 0)
    {
        AVLUINT64_NODE **ppNode; /* address of the pointer to the root node of
                                    the current subtree */
        AVLUINT64_NODE  *pNode;  /* points to root node of current subtree */
        AVLUINT64_NODE  *leftp;  /* points to root node of left subtree */
        int              lefth;  /* height of the left subtree */
        AVLUINT64_NODE  *rightp; /* points to root node of right subtree */
        int              righth; /* height of the right subtree */

        /*
         * Find the current root node and its two subtrees. By construction,
         * we know that both of them conform to the AVL balancing rules.
         */

        ppNode = ancestors[--count];
        pNode = *ppNode;
        leftp = pNode->left;
        lefth = (leftp != NULL) ? leftp->height : 0;
        rightp = pNode->right;
        righth = (rightp != NULL) ? rightp->height : 0;

        if (righth - lefth < -1) {
            /*
             *         *
             *       /   \
             *    n+2      n
             *
             * The current subtree violates the balancing rules by beeing too
             * high on the left side. We must use one of two different
             * rebalancing methods depending on the configuration of the left
             * subtree.
             *
             * Note that leftp cannot be NULL or we would not pass there !
             */

            AVLUINT64_NODE * leftleftp;  /* points to root of left left
                            subtree */
            AVLUINT64_NODE * leftrightp; /* points to root of left right
                            subtree */
            int          leftrighth; /* height of left right subtree */

            leftleftp = leftp->left;
            leftrightp = leftp->right;
            leftrighth = (leftrightp != NULL) ? leftrightp->height : 0;

            if ((leftleftp != NULL) && (leftleftp->height >= leftrighth)) {
                /*
                 *            <D>                     <B>
                 *             *                    n+2|n+3
                 *           /   \                   /   \
                 *        <B>     <E>    ---->    <A>     <D>
                 *        n+2      n              n+1   n+1|n+2
                 *       /   \                           /   \
                 *    <A>     <C>                     <C>     <E>
                 *    n+1    n|n+1                   n|n+1     n
                 */

                pNode->left = leftrightp;   /* D.left = C */
                pNode->height = leftrighth + 1;
                leftp->right = pNode;       /* B.right = D */
                leftp->height = leftrighth + 2;
                *ppNode = leftp;        /* B becomes root */

            } else {
                /*
                 *           <F>
                 *            *
                 *          /   \                        <D>
                 *       <B>     <G>                     n+2
                 *       n+2      n                     /   \
                 *      /   \           ---->        <B>     <F>
                 *   <A>     <D>                     n+1     n+1
                 *    n      n+1                    /  \     /  \
                 *          /   \                <A>   <C> <E>   <G>
                 *       <C>     <E>              n  n|n-1 n|n-1  n
                 *      n|n-1   n|n-1
                 *
                 * We can assume that leftrightp is not NULL because we expect
                 * leftp and rightp to conform to the AVL balancing rules.
                 * Note that if this assumption is wrong, the algorithm will
                 * crash here.
                 */

                leftp->right = leftrightp->left;    /* B.right = C */
                leftp->height = leftrighth;
                pNode->left = leftrightp->right;    /* F.left = E */
                pNode->height = leftrighth;
                leftrightp->left = leftp;       /* D.left = B */
                leftrightp->right = pNode;      /* D.right = F */
                leftrightp->height = leftrighth + 1;
                *ppNode = leftrightp;           /* D becomes root */
            }

        } else if (righth - lefth > 1) {
            /*
             *        *
             *      /   \
             *    n      n+2
             *
             * The current subtree violates the balancing rules by beeing too
             * high on the right side. This is exactly symmetric to the
             * previous case. We must use one of two different rebalancing
             * methods depending on the configuration of the right subtree.
             *
             * Note that rightp cannot be NULL or we would not pass there !
             */

            AVLUINT64_NODE *rightleftp;  /* points to the root of right left
                                            subtree */
            int             rightlefth;  /* height of right left subtree */
            AVLUINT64_NODE *rightrightp; /* points to the root of right right
                                            subtree */

            rightleftp = rightp->left;
            rightlefth = (rightleftp != NULL) ? rightleftp->height : 0;
            rightrightp = rightp->right;

            if ((rightrightp != NULL) && (rightrightp->height >= rightlefth)) {
                /*        <B>                             <D>
                 *         *                            n+2|n+3
                 *       /   \                           /   \
                 *    <A>     <D>        ---->        <B>     <E>
                 *     n      n+2                   n+1|n+2   n+1
                 *           /   \                   /   \
                 *        <C>     <E>             <A>     <C>
                 *       n|n+1    n+1              n     n|n+1
                 */

                pNode->right = rightleftp;  /* B.right = C */
                pNode->height = rightlefth + 1;
                rightp->left = pNode;       /* D.left = B */
                rightp->height = rightlefth + 2;
                *ppNode = rightp;       /* D becomes root */

            } else {
                /*        <B>
                 *         *
                 *       /   \                            <D>
                 *    <A>     <F>                         n+2
                 *     n      n+2                        /   \
                 *           /   \       ---->        <B>     <F>
                 *        <D>     <G>                 n+1     n+1
                 *        n+1      n                 /  \     /  \
                 *       /   \                    <A>   <C> <E>   <G>
                 *    <C>     <E>                  n  n|n-1 n|n-1  n
                 *   n|n-1   n|n-1
                 *
                 * We can assume that rightleftp is not NULL because we expect
                 * leftp and rightp to conform to the AVL balancing rules.
                 * Note that if this assumption is wrong, the algorithm will
                 * crash here.
                 */

                pNode->right = rightleftp->left;    /* B.right = C */
                pNode->height = rightlefth;
                rightp->left = rightleftp->right;   /* F.left = E */
                rightp->height = rightlefth;
                rightleftp->left = pNode;       /* D.left = B */
                rightleftp->right = rightp;     /* D.right = F */
                rightleftp->height = rightlefth + 1;
                *ppNode = rightleftp;           /* D becomes root */
            }

        } else {
            /*
             * No rebalancing, just set the tree height
             *
             * If the height of the current subtree has not changed, we can
             * stop here because we know that we have not broken the AVL
             * balancing rules for our ancestors.
             */

            int height;

            height = ((righth > lefth) ? righth : lefth) + 1;
            if (pNode->height == height) {
                break;
            }
            pNode->height = height;
        }
    }
}

/*
 * end
 */
