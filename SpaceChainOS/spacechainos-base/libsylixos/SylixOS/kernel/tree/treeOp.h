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
** 文   件   名: treeOp.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 12 月 17 日
**
** 描        述: 这是系统树操作定义。
*********************************************************************************************************/

#ifndef __TREEOP_H
#define __TREEOP_H

/*********************************************************************************************************
  判断链表是否为空
*********************************************************************************************************/

#define _TREE_RB_IS_EMPTY(ptrbrRoot)                 \
        (ptrbrRoot->TRBR_ptrbnNode == LW_NULL)

/*********************************************************************************************************
  获得链表母体指针结构
*********************************************************************************************************/

#define _TREE_ENTRY(ptr, type, member)              \
        _LIST_CONTAINER_OF(ptr, type, member)

/*********************************************************************************************************
  下一个
*********************************************************************************************************/

static LW_INLINE PLW_TREE_RB_NODE   _tree_rb_get_left (PLW_TREE_RB_NODE  ptrbn)
{
    return  (ptrbn->TRBN_ptrbnLeft);
}
static LW_INLINE PLW_TREE_RB_NODE   _tree_rb_get_right (PLW_TREE_RB_NODE  ptrbn)
{
    return  (ptrbn->TRBN_ptrbnRight);
}

static LW_INLINE PLW_TREE_RB_NODE  *_tree_rb_get_left_addr (PLW_TREE_RB_NODE  ptrbn)
{
    return  (&(ptrbn->TRBN_ptrbnLeft));
}
static LW_INLINE PLW_TREE_RB_NODE  *_tree_rb_get_right_addr (PLW_TREE_RB_NODE  ptrbn)
{
    return  (&(ptrbn->TRBN_ptrbnRight));
}

/*********************************************************************************************************
  链接相关操作
*********************************************************************************************************/

static LW_INLINE VOID _tree_rb_link_node (PLW_TREE_RB_NODE  ptrbn, 
                                          PLW_TREE_RB_NODE  ptrbnParent, 
                                          PLW_TREE_RB_NODE *pptrbnLink)
{
    ptrbn->TRBN_ptrbnParent = ptrbnParent;
	ptrbn->TRBN_iColor      = LW_TREE_RB_RED;
	ptrbn->TRBN_ptrbnLeft   = ptrbn->TRBN_ptrbnRight = LW_NULL;

	*pptrbnLink = ptrbn;
}

#endif                                                                  /*  __TREEOP_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
