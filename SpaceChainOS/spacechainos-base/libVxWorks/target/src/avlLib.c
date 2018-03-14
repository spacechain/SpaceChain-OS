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

#include "avlLib.h"

/* defines */

#define AVL_MAX_HEIGHT 42 /* The meaning of life, the universe and everything.
                             Plus, the nodes for a tree this high would use
                             more than 2**32 bytes anyway */

/* forward declarations */

LOCAL void avlRebalance(AVL_NODE ***ancestors, int count);

/*
 * avlInsert - insert a node in an AVL tree
 */
STATUS avlInsert (AVL_TREE   *pRoot,      /* pointer to the root node ptr */
                  AVL_NODE   *pNewNode,   /* ptr to the node we want to insert */
                  void       *pKey,       /* search key of newNode */
                  AVL_COMPARE compare)    /* comparison function */
{
    AVL_NODE **ppNode;          /* ptr to current node ptr */
    AVL_NODE **ancestor[AVL_MAX_HEIGHT]; /* ancestor list */
    int        ancestorCount;   /* number of ancestors */

    if ((pNewNode == NULL) || (pRoot == NULL)) {
        return (ERROR);
    }

    ppNode = pRoot;
    ancestorCount = 0;

    /* Find the leaf node where to add the new node */

    while (ancestorCount < AVL_MAX_HEIGHT) {
        AVL_NODE *pNode;    /* pointer to the current node */
        int       delta;    /* result of the comparison operation */

        pNode = *ppNode;
        if (pNode == NULL) {
            break;          /* we can insert a leaf node here */
        }

        ancestor[ancestorCount++] = ppNode;

        delta = compare(pNode, pKey);
        if (0 == delta) {
            return  (ERROR);

        } else if (delta < 0) {
            ppNode = &(pNode->left);

        } else {
            ppNode = &(pNode->right);
        }
    }

    if (ancestorCount == AVL_MAX_HEIGHT) {
        return (ERROR);
    }

    /* initialize pNewNode */

    ((AVL_NODE *)pNewNode)->left = NULL;
    ((AVL_NODE *)pNewNode)->right = NULL;
    ((AVL_NODE *)pNewNode)->height = 1;
    *ppNode = pNewNode;

    avlRebalance(ancestor, ancestorCount);

    return  (OK);
}

/*
 * avlDelete - delete a node in an AVL tree
 */
AVL_NODE *avlDelete (AVL_TREE   *pRoot,      /* pointer to the root node pointer */
                     void       *pKey,       /* search key of node we want to delete */
                     AVL_COMPARE compare)    /* comparison function */
{
    AVL_NODE **ppNode;          /* ptr to current node ptr */
    AVL_NODE  *pNode = NULL;    /* ptr to the current node */
    AVL_NODE **ancestor[AVL_MAX_HEIGHT];
                                /* ancestor node pointer list */
    int ancestorCount;          /* number of ancestors */
    AVL_NODE  *pDelete;         /* ptr to the node to be */
                                /* deleted */

    ppNode = pRoot;
    ancestorCount = 0;

    /* find node to be deleted */

    while (ancestorCount < AVL_MAX_HEIGHT) {
        int delta;          /* result of the comparison operation */

        pNode = *ppNode;
        if (pNode == NULL) {
            return  (NULL); /* node was not in the tree ! */
        }

        ancestor[ancestorCount++] = ppNode;

        delta = compare (pNode, pKey);
        if (0 == delta) {
            break;          /* we found the node we have to delete */

        } else if (delta < 0) {
            ppNode = &(pNode->left);

        } else {
            ppNode = &(pNode->right);
        }
    }

    if (ancestorCount == AVL_MAX_HEIGHT) {
        return (NULL);
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

        AVL_NODE **ppDelete;       /* ptr to the ptr to the node
                                      we have to delete */
        int     deleteAncestorCount;    /* place where the replacing
                                           node will have to be
                                           inserted in the ancestor
                                           list */

        deleteAncestorCount = ancestorCount;
        ppDelete = ppNode;
        pDelete  = pNode;

        /* search for node just before delNode in the tree ordering */

        ppNode = &(pNode->left);

        while (ancestorCount < AVL_MAX_HEIGHT) {
            pNode = *ppNode;
            if (pNode->right == NULL) {
                break;
            }

            ancestor[ancestorCount++] = ppNode;
            ppNode = &(pNode->right);
        }

        if (ancestorCount == AVL_MAX_HEIGHT) {
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

    avlRebalance(ancestor, ancestorCount);

    return  (pDelete);
}

/*
 * avlSearch - search a node in an AVL tree
 */
AVL_NODE *avlSearch (AVL_TREE    root,       /* root node pointer */
                     void       *pKey,       /* search key */
                     AVL_COMPARE compare)    /* comparison function */
{
    AVL_NODE *pNode;        /* pointer to the current node */

    pNode = root;

    /* search node that has matching key */

    while (pNode != NULL) {
        int delta;          /* result of the comparison operation */

        delta = compare(pNode, pKey);

        if (0 == delta) {
            return (pNode); /* found the node */

        } else if (delta < 0) {
            pNode = pNode->left;

        } else {
            pNode = pNode->right;
        }
    }

    /* not found, return NULL */

    return  (NULL);
}

/*
 * avlSuccessorGet - find node with key successor to input key
 */
AVL_NODE *avlSuccessorGet (AVL_TREE    root,    /* root node pointer */
                           void       *pKey,    /* search key */
                           AVL_COMPARE compare) /* comparison function */
{
    AVL_NODE *pNode;      /* pointer to the current node */
    AVL_NODE *pSuccessor; /* pointer to the current superior*/

    pNode = root;
    pSuccessor = NULL;

    while (pNode != NULL) {
        if (compare (pNode, pKey) >= 0) {
            pNode = pNode->right;

        } else {
            pSuccessor = pNode;
            pNode = pNode->left;
        }
    }

    return  (pSuccessor);
}

/*
 * avlPredecessorGet - find node with key predecessor to input key
 */
AVL_NODE *avlPredecessorGet (AVL_TREE    root,       /* root node pointer */
                             void       *pKey,       /* search key */
                             AVL_COMPARE compare)    /* comparison function */
{
    AVL_NODE *  pNode;      /* pointer to the current node */
    AVL_NODE *  pPred;      /* pointer to the current inferior*/

    pNode = root;
    pPred = NULL;

    while (pNode != NULL) {
        if (compare (pNode, pKey) <= 0) {
            pNode = pNode->left;

        } else {
            pPred = pNode;
            pNode = pNode->right;
        }
    }

    return  (pPred);
}

/*
 * avlMinimumGet - find node with smallest key
 */
AVL_NODE *avlMinimumGet (AVL_TREE  root)  /* root node pointer */
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
 * avlMaximumGet - find node with largest key
 */
AVL_NODE *avlMaximumGet(AVL_TREE  root)  /* root node pointer */
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
 * avlTreeWalk - walk the tree and execute selected functions on each node
 */
STATUS avlTreeWalk (AVL_TREE     root,      /* root node pointer */
                    AVL_CALLBACK preRtn,    /* pre-order routine */
                    void        *preArg,    /* pre-order argument */
                    AVL_CALLBACK inRtn,     /* in-order routine */
                    void        *inArg,     /* in-order argument */
                    AVL_CALLBACK postRtn,   /* post-order routine */
                    void        *postArg)   /* post-order argument */
{
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
        if (avlTreeWalk(root->left, preRtn, preArg, inRtn, inArg,
                        postRtn, postArg) == ERROR) {
            return  (ERROR);
        }
    }

    /* call in-order routine */

    if (inRtn != NULL) {
        if (inRtn(root, inArg) == ERROR) {
            return (ERROR);
        }
    }

    /* walk right side */

    if (!(NULL == root->right)) {
        if (avlTreeWalk(root->right, preRtn, preArg, inRtn, inArg,
                        postRtn, postArg) == ERROR) {
            return  (ERROR);
        }
    }

    /* call post-order routine */

    if (postRtn != NULL) {
        if (postRtn(root, postArg) == ERROR) {
            return (ERROR);
        }
    }

    return  (OK);
}

/*******************************************************************************
*
* avlRebalance - rebalance an AVL tree
*
* This routine rebalances an AVL tree as part of the insert and delete
* operations.
*
* INTERNAL
* The AVL tree balancing rules are as follows :
* - the height of the left and right subtrees under a given node must never
*   differ by more than one
* - the height of a given subtree is defined as 1 plus the maximum height of
*   the subtrees under his root node
*
* The rebalance procedure must be called after a leaf node has been inserted
* or deleted from the tree. It checks that the AVL balancing rules are
* respected, makes local adjustments to the tree if necessary, recalculates
* the height field of the modified nodes, and repeats the process for every
* node up to the root node. This iteration is necessary because the balancing
* rules for a given node might have been broken by the modification we did on
* one of the subtrees under it.
*
* Because we need to iterate the process up to the root node, and the tree
* nodes does not contain pointers to their father node, we ask the caller of
* this procedure to keep a list of all the nodes traversed from the root node
* to the node just before the recently inserted or deleted node. This is the
* <ancestors> argument. Because each subtree might have to be re-rooted in the
* balancing operation, <ancestors> is actually a list pointers to the node
* pointers - thus if re-rooting occurs, the node pointers can be modified so
* that they keep pointing to the root of a given subtree.
*
* <count> is simply a count of elements in the <ancestors> list.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

LOCAL void avlRebalance (AVL_NODE ***ancestors,  /* list of pointers to the ancestor node pointers */
                         int         count)      /* number ancestors to rebalance */
{
    while (count > 0) {
        AVL_NODE **ppNode; /* address of the pointer to the root node of
                              the current subtree */
        AVL_NODE  *pNode;  /* points to root node of current subtree */
        AVL_NODE  *leftp;  /* points to root node of left subtree */
        int        lefth;  /* height of the left subtree */
        AVL_NODE  *rightp; /* points to root node of right subtree */
        int        righth; /* height of the right subtree */

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

            AVL_NODE *leftleftp;  /* points to root of left left subtree */
            AVL_NODE *leftrightp; /* points to root of left right subtree */
            int       leftrighth; /* height of left right subtree */

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

            AVL_NODE *rightleftp;   /* points to the root of right left subtree */
            int       rightlefth;   /* height of right left subtree */
            AVL_NODE *rightrightp;  /* points to the root of right right subtree */

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
 * avlInsertInform - insert a node in an AVL tree and report key holder
 */
STATUS avlInsertInform (AVL_TREE    *pRoot,              /* ptr to the root node pointer */
                        void        *pNewNode,           /* pointer to the candidate node */
                        void        *key,                /* unique key of new node */
                        void       **ppKeyHolder,        /* ptr to final key holder */
                        AVL_COMPARE  compare)            /* comparison function */
{
    AVL_NODE **nodepp;                     /* ptr to current node ptr */
    AVL_NODE **ancestor[AVL_MAX_HEIGHT];   /* list of pointers to all
                                              our ancestor node ptrs */
    int        ancestorCount;              /* number of ancestors */

    if (NULL == ppKeyHolder) {
        printf("invalid input data were passed to avlInsertInform\n");
        return  (ERROR);
    }

    nodepp = pRoot;
    ancestorCount = 0;

    while (TRUE) {
        AVL_NODE *nodep;  /* pointer to the current node */
        int       delta;  /* result of the comparison operation */

        nodep = *nodepp;
        if (nodep == NULL) {
            break;  /* we can insert a leaf node here ! */
        }

        ancestor[ancestorCount++] = nodepp;

        delta = compare(nodep, key);
        if (0 == delta) {
            /* we inform the caller of the key holder node and return ERROR */

            *ppKeyHolder = nodep;
            return  (ERROR);

        } else if (delta < 0) {
            nodepp = (AVL_NODE **)&(nodep->left);

        } else {
            nodepp = (AVL_NODE **)&(nodep->right);
        }
    }

    ((AVL_NODE *)pNewNode)->left = NULL;
    ((AVL_NODE *)pNewNode)->right = NULL;
    ((AVL_NODE *)pNewNode)->height = 1;
    *nodepp = pNewNode;

    *ppKeyHolder = pNewNode;

    avlRebalance (ancestor, ancestorCount);

    return  (OK);
}

/*
 * avlRemoveInsert - forcefully insert a node in an AVL tree
 */
void *avlRemoveInsert(AVL_TREE    *pRoot,              /* ptr to the root node pointer */
                      void        *pNewNode,           /* pointer to the candidate node */
                      void        *key,                /* unique key of new node */
                      AVL_COMPARE  compare)            /* comparison function */
{
    AVL_NODE **nodepp;                  /* ptr to current node ptr */
    AVL_NODE **ancestor[AVL_MAX_HEIGHT];/* list of pointers to all
                                           our ancestor node ptrs */
    int     ancestorCount;              /* number of ancestors */

    nodepp = pRoot;
    ancestorCount = 0;

    while (TRUE) {
        AVL_NODE *nodep;  /* pointer to the current node */
        int       delta;  /* result of the comparison operation */

        nodep = *nodepp;
        if (nodep == NULL) {
            break;  /* we can insert a leaf node here ! */
        }

        ancestor[ancestorCount++] = nodepp;

        delta = compare (nodep, key);
        if (0 == delta) {
            /* we copy the tree data from the old node to the new node */

            ((AVL_NODE *)pNewNode)->left = nodep->left;
            ((AVL_NODE *)pNewNode)->right = nodep->right;
            ((AVL_NODE *)pNewNode)->height = nodep->height;

            /* and we make the new node child of the old node's parent */

            *nodepp = pNewNode;

            /* before we return it we sterilize the old node */
            nodep->left = NULL;
            nodep->right = NULL;
            nodep->height = 1;

            return  (nodep);

        } else if (delta < 0) {
            nodepp = (AVL_NODE **)&(nodep->left);

        } else {
            nodepp = (AVL_NODE **)&(nodep->right);
        }
    }

    ((AVL_NODE *)pNewNode)->left = NULL;
    ((AVL_NODE *)pNewNode)->right = NULL;
    ((AVL_NODE *)pNewNode)->height = 1;
    *nodepp = pNewNode;

    avlRebalance(ancestor, ancestorCount);

    return  (NULL);
}

/*
 * end
 */
