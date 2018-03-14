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
** 文   件   名: treeRb.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 12 月 17 日
**
** 描        述: 这是系统红黑树操作定义。(from linux)
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: __tree_rb_rotate_left
** 功能描述: 红黑树指定节点左旋
** 输　入  : ptrbn     指定节点
**           ptrbr     根节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tree_rb_rotate_left (PLW_TREE_RB_NODE  ptrbn, PLW_TREE_RB_ROOT  ptrbr)
{
    REGISTER PLW_TREE_RB_NODE   ptrbnRight = ptrbn->TRBN_ptrbnRight;
    
    if ((ptrbn->TRBN_ptrbnRight = ptrbnRight->TRBN_ptrbnLeft) != LW_NULL) {
        ptrbnRight->TRBN_ptrbnLeft->TRBN_ptrbnParent = ptrbn;
    }
    
    ptrbnRight->TRBN_ptrbnLeft = ptrbn;
    
    if ((ptrbnRight->TRBN_ptrbnParent = ptrbn->TRBN_ptrbnParent) != LW_NULL) {
        if (ptrbn == ptrbn->TRBN_ptrbnParent->TRBN_ptrbnLeft) {
            ptrbn->TRBN_ptrbnParent->TRBN_ptrbnLeft = ptrbnRight;
        
        } else {
            ptrbn->TRBN_ptrbnParent->TRBN_ptrbnRight = ptrbnRight;
        }
    } else {
        ptrbr->TRBR_ptrbnNode = ptrbnRight;
    }
    
    ptrbn->TRBN_ptrbnParent = ptrbnRight;
}
/*********************************************************************************************************
** 函数名称: __tree_rb_rotate_right
** 功能描述: 红黑树指定节点右旋
** 输　入  : ptrbn     指定节点
**           ptrbr     根节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tree_rb_rotate_right (PLW_TREE_RB_NODE  ptrbn, PLW_TREE_RB_ROOT  ptrbr)
{
    REGISTER PLW_TREE_RB_NODE ptrbnLeft = ptrbn->TRBN_ptrbnLeft;
    
    if ((ptrbn->TRBN_ptrbnLeft = ptrbnLeft->TRBN_ptrbnRight) != LW_NULL) {
        ptrbnLeft->TRBN_ptrbnRight->TRBN_ptrbnParent = ptrbn;
    }
    
    ptrbnLeft->TRBN_ptrbnRight = ptrbn;
    
    if ((ptrbnLeft->TRBN_ptrbnParent = ptrbn->TRBN_ptrbnParent) != LW_NULL) {
        if (ptrbn == ptrbn->TRBN_ptrbnParent->TRBN_ptrbnRight) {
            ptrbn->TRBN_ptrbnParent->TRBN_ptrbnRight = ptrbnLeft;
        
        } else {
            ptrbn->TRBN_ptrbnParent->TRBN_ptrbnLeft = ptrbnLeft;
        }
    } else {
        ptrbr->TRBR_ptrbnNode = ptrbnLeft;
    }
    
    ptrbn->TRBN_ptrbnParent = ptrbnLeft;
}
/*********************************************************************************************************
** 函数名称: __tree_rb_erase_color
** 功能描述: 删除节点同时处理颜色规则
** 输　入  : ptrbn             指定节点
**           ptrbnParent       父节点
**           ptrbr             根节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tree_rb_erase_color (PLW_TREE_RB_NODE  ptrbn, 
                                    PLW_TREE_RB_NODE  ptrbnParent, 
                                    PLW_TREE_RB_ROOT  ptrbr)
{
    REGISTER PLW_TREE_RB_NODE  ptrbnOther;
    
    while ((!ptrbn || ptrbn->TRBN_iColor == LW_TREE_RB_BLACK) && 
           ptrbn != ptrbr->TRBR_ptrbnNode) {
           
        if (ptrbnParent->TRBN_ptrbnLeft == ptrbn) {
            ptrbnOther = ptrbnParent->TRBN_ptrbnRight;
            if (ptrbnOther->TRBN_iColor == LW_TREE_RB_RED) {
                ptrbnOther->TRBN_iColor = LW_TREE_RB_BLACK;
                ptrbnParent->TRBN_iColor = LW_TREE_RB_RED;
                __tree_rb_rotate_left(ptrbnParent, ptrbr);
                ptrbnOther = ptrbnParent->TRBN_ptrbnRight;
            }
            
            if ((!ptrbnOther->TRBN_ptrbnLeft ||
                ptrbnOther->TRBN_ptrbnLeft->TRBN_iColor == LW_TREE_RB_BLACK) && 
                (!ptrbnOther->TRBN_ptrbnRight ||
                ptrbnOther->TRBN_ptrbnRight->TRBN_iColor == LW_TREE_RB_BLACK)) {
                ptrbnOther->TRBN_iColor = LW_TREE_RB_RED;
                ptrbn = ptrbnParent;
                ptrbnParent = ptrbn->TRBN_ptrbnParent;
            
            } else {
                if (!ptrbnOther->TRBN_ptrbnRight ||
                    ptrbnOther->TRBN_ptrbnRight->TRBN_iColor == LW_TREE_RB_BLACK) {
                    REGISTER PLW_TREE_RB_NODE  ptrbnOtherLeft;
                    
                    if ((ptrbnOtherLeft = ptrbnOther->TRBN_ptrbnLeft) != LW_NULL) {
                        ptrbnOtherLeft->TRBN_iColor = LW_TREE_RB_BLACK;
                    }
                    ptrbnOther->TRBN_iColor = LW_TREE_RB_RED;
                    __tree_rb_rotate_right(ptrbnOther, ptrbr);
                    ptrbnOther = ptrbnParent->TRBN_ptrbnRight;
                }
                
                ptrbnOther->TRBN_iColor = ptrbnParent->TRBN_iColor;
                ptrbnParent->TRBN_iColor = LW_TREE_RB_BLACK;
                
                if (ptrbnOther->TRBN_ptrbnRight) {
                    ptrbnOther->TRBN_ptrbnRight->TRBN_iColor = LW_TREE_RB_BLACK;
                }
                
                __tree_rb_rotate_left(ptrbnParent, ptrbr);
                ptrbn = ptrbr->TRBR_ptrbnNode;
                break;
            }
        } else {
            ptrbnOther = ptrbnParent->TRBN_ptrbnLeft;
            
            if (ptrbnOther->TRBN_iColor == LW_TREE_RB_RED) {
                ptrbnOther->TRBN_iColor = LW_TREE_RB_BLACK;
                ptrbnParent->TRBN_iColor = LW_TREE_RB_RED;
                __tree_rb_rotate_right(ptrbnParent, ptrbr);
                ptrbnOther = ptrbnParent->TRBN_ptrbnLeft;
            }
            
            if ((!ptrbnOther->TRBN_ptrbnLeft ||
                ptrbnOther->TRBN_ptrbnLeft->TRBN_iColor == LW_TREE_RB_BLACK) && 
                (!ptrbnOther->TRBN_ptrbnRight ||
                ptrbnOther->TRBN_ptrbnRight->TRBN_iColor == LW_TREE_RB_BLACK)) {
                ptrbnOther->TRBN_iColor = LW_TREE_RB_RED;
                ptrbn = ptrbnParent;
                ptrbnParent = ptrbn->TRBN_ptrbnParent;
            
            } else {
                if (!ptrbnOther->TRBN_ptrbnLeft ||
                    ptrbnOther->TRBN_ptrbnLeft->TRBN_iColor == LW_TREE_RB_BLACK) {
                    REGISTER PLW_TREE_RB_NODE  ptrbnOtherRight;
                    if ((ptrbnOtherRight = ptrbnOther->TRBN_ptrbnRight) != LW_NULL) {
                        ptrbnOtherRight->TRBN_iColor = LW_TREE_RB_BLACK;
                    }
                    
                    ptrbnOther->TRBN_iColor = LW_TREE_RB_RED;
                    __tree_rb_rotate_left(ptrbnOther, ptrbr);
                    ptrbnOther = ptrbnParent->TRBN_ptrbnLeft;
                }
                
                ptrbnOther->TRBN_iColor = ptrbnParent->TRBN_iColor;
                ptrbnParent->TRBN_iColor = LW_TREE_RB_BLACK;
                
                if (ptrbnOther->TRBN_ptrbnLeft) {
                    ptrbnOther->TRBN_ptrbnLeft->TRBN_iColor = LW_TREE_RB_BLACK;
                }
                
                __tree_rb_rotate_right(ptrbnParent, ptrbr);
                ptrbn = ptrbr->TRBR_ptrbnNode;
                break;
            }
        }
    }
    
    if (ptrbn) {
        ptrbn->TRBN_iColor = LW_TREE_RB_BLACK;
    }
}
/*********************************************************************************************************
** 函数名称: _Tree_Rb_Insert_Color
** 功能描述: 插入节点同时处理颜色规则
** 输　入  : ptrbn     指定节点
**           ptrbr     根节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _Tree_Rb_Insert_Color (PLW_TREE_RB_NODE  ptrbn, PLW_TREE_RB_ROOT  ptrbr)
{
    REGISTER PLW_TREE_RB_NODE  ptrbnParent;                             /*  parent                      */
    REGISTER PLW_TREE_RB_NODE  ptrbnGparent;                            /*  grand parent                */
    
    while (((ptrbnParent = ptrbn->TRBN_ptrbnParent) != LW_NULL) && 
           (ptrbnParent->TRBN_iColor == LW_TREE_RB_RED)) {
    
        ptrbnGparent = ptrbnParent->TRBN_ptrbnParent;
        
        if (ptrbnParent == ptrbnGparent->TRBN_ptrbnLeft) {
            {
                REGISTER PLW_TREE_RB_NODE   ptrbnUncle = ptrbnGparent->TRBN_ptrbnRight;
                
                if (ptrbnUncle && ptrbnUncle->TRBN_iColor == LW_TREE_RB_RED) {
                    ptrbnUncle->TRBN_iColor = LW_TREE_RB_BLACK;
                    ptrbnParent->TRBN_iColor = LW_TREE_RB_BLACK;
                    ptrbnGparent->TRBN_iColor = LW_TREE_RB_RED;
                    ptrbn = ptrbnGparent;
                    continue;
                }
            }
            
            if (ptrbnParent->TRBN_ptrbnRight == ptrbn) {
                REGISTER PLW_TREE_RB_NODE  ptrbTmp;
                __tree_rb_rotate_left(ptrbnParent, ptrbr);
                ptrbTmp = ptrbnParent;
                ptrbnParent = ptrbn;
                ptrbn = ptrbTmp;
            }
            
            ptrbnParent->TRBN_iColor = LW_TREE_RB_BLACK;
            ptrbnGparent->TRBN_iColor = LW_TREE_RB_RED;
            __tree_rb_rotate_right(ptrbnGparent, ptrbr);
        
        } else {
            {
                REGISTER PLW_TREE_RB_NODE   ptrbnUncle = ptrbnGparent->TRBN_ptrbnLeft;
                
                if (ptrbnUncle && ptrbnUncle->TRBN_iColor == LW_TREE_RB_RED) {
                    ptrbnUncle->TRBN_iColor = LW_TREE_RB_BLACK;
                    ptrbnParent->TRBN_iColor = LW_TREE_RB_BLACK;
                    ptrbnGparent->TRBN_iColor = LW_TREE_RB_RED;
                    ptrbn = ptrbnGparent;
                    continue;
                }
            }
            
            if (ptrbnParent->TRBN_ptrbnLeft == ptrbn) {
                REGISTER PLW_TREE_RB_NODE  ptrbTmp;
                __tree_rb_rotate_right(ptrbnParent, ptrbr);
                ptrbTmp = ptrbnParent;
                ptrbnParent = ptrbn;
                ptrbn = ptrbTmp;
            }
            
            ptrbnParent->TRBN_iColor = LW_TREE_RB_BLACK;
            ptrbnGparent->TRBN_iColor = LW_TREE_RB_RED;
            __tree_rb_rotate_left(ptrbnGparent, ptrbr);
        }
    } 
    
    ptrbr->TRBR_ptrbnNode->TRBN_iColor = LW_TREE_RB_BLACK;
}
/*********************************************************************************************************
** 函数名称: _Tree_Rb_Erase
** 功能描述: 删除指定节点
** 输　入  : ptrbn     指定节点
**           ptrbr     根节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _Tree_Rb_Erase (PLW_TREE_RB_NODE  ptrbn, PLW_TREE_RB_ROOT  ptrbr)
{
    REGISTER PLW_TREE_RB_NODE  ptrbnChild;
    REGISTER PLW_TREE_RB_NODE  ptrbnParent;
    
             INT               iColor;
             
    if (!ptrbn->TRBN_ptrbnLeft) {
        ptrbnChild = ptrbn->TRBN_ptrbnRight;
    
    } else if (!ptrbn->TRBN_ptrbnRight) {
        ptrbnChild = ptrbn->TRBN_ptrbnLeft;
    
    } else {
        REGISTER PLW_TREE_RB_NODE  ptrbnOld = ptrbn;
        REGISTER PLW_TREE_RB_NODE  ptrbnLeft;
        
        ptrbn = ptrbn->TRBN_ptrbnRight;
        
        while ((ptrbnLeft = ptrbn->TRBN_ptrbnLeft) != LW_NULL) {
            ptrbn = ptrbnLeft;
        }
        
        ptrbnChild = ptrbn->TRBN_ptrbnRight;
        ptrbnParent = ptrbn->TRBN_ptrbnParent;
        iColor = ptrbn->TRBN_iColor;
        
        if (ptrbnChild) {
            ptrbnChild->TRBN_ptrbnParent = ptrbnParent;
        }
        
        if (ptrbnParent) {
            if (ptrbnParent->TRBN_ptrbnLeft == ptrbn) {
                ptrbnParent->TRBN_ptrbnLeft = ptrbnChild;
            
            } else {
                ptrbnParent->TRBN_ptrbnRight = ptrbnChild;
            }
        } else {
            ptrbr->TRBR_ptrbnNode = ptrbnChild;
        }
        
        if (ptrbn->TRBN_ptrbnParent == ptrbnOld) {
            ptrbnParent = ptrbn;
        }
        
        ptrbn->TRBN_ptrbnParent = ptrbnOld->TRBN_ptrbnParent;
        ptrbn->TRBN_iColor = ptrbnOld->TRBN_iColor;
        ptrbn->TRBN_ptrbnRight = ptrbnOld->TRBN_ptrbnRight;
        ptrbn->TRBN_ptrbnLeft = ptrbnOld->TRBN_ptrbnLeft;
        
        if (ptrbnOld->TRBN_ptrbnParent) {
            if (ptrbnOld->TRBN_ptrbnParent->TRBN_ptrbnLeft == ptrbnOld) {
                ptrbnOld->TRBN_ptrbnParent->TRBN_ptrbnLeft = ptrbn;
            
            } else {
                ptrbnOld->TRBN_ptrbnParent->TRBN_ptrbnRight = ptrbn;
            }
        } else {
            ptrbr->TRBR_ptrbnNode = ptrbn;
        }
        
        ptrbnOld->TRBN_ptrbnLeft->TRBN_ptrbnParent = ptrbn;
        
        if (ptrbnOld->TRBN_ptrbnRight) {
            ptrbnOld->TRBN_ptrbnRight->TRBN_ptrbnParent = ptrbn;
        }
        goto    __color;
    }
    
    ptrbnParent = ptrbn->TRBN_ptrbnParent;
    iColor = ptrbn->TRBN_iColor;

    if (ptrbnChild) {
        ptrbnChild->TRBN_ptrbnParent = ptrbnParent;
    }
    
    if (ptrbnParent) {
        if (ptrbnParent->TRBN_ptrbnLeft == ptrbn) {
            ptrbnParent->TRBN_ptrbnLeft = ptrbnChild;
        
        } else {
            ptrbnParent->TRBN_ptrbnRight = ptrbnChild;
        }
    } else {
        ptrbr->TRBR_ptrbnNode = ptrbnChild;
    }
    
__color:
    if (iColor == LW_TREE_RB_BLACK) {
        __tree_rb_erase_color(ptrbnChild, ptrbnParent, ptrbr);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
