/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: pnameLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 30 日
**
** 描        述: posix 内部命名管理.

** BUG:
2010.01.07  加入支持 proc 文件的接口.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/posixLib.h"                                        /*  已包含操作系统头文件        */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
  hash 定义
*********************************************************************************************************/
INT  __hashHorner(CPCHAR  pcKeyword, INT  iTableSize);                  /*  霍纳多项式                  */

LW_LIST_LINE_HEADER          _G_plinePxNameNodeHash[__PX_NAME_NODE_HASH_SIZE];
UINT                         _G_uiNamedNodeCounter = 0;
/*********************************************************************************************************
** 函数名称: __pxnameSeach
** 功能描述: 从 posix 名字系统查询指定的节点.
** 输　入  : pcName        名字
**           iHash         哈希表入口
** 输　出  : 找到的节点指针
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
__PX_NAME_NODE  *__pxnameSeach (CPCHAR  pcName, INT  iHash)
{
    PLW_LIST_LINE      plineTemp;
    __PX_NAME_NODE    *pxnode;
    
    if (iHash < 0) {
        iHash = __hashHorner(pcName, __PX_NAME_NODE_HASH_SIZE);
    }
    
    for (plineTemp  = _G_plinePxNameNodeHash[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pxnode = _LIST_ENTRY(plineTemp, __PX_NAME_NODE, PXNODE_lineManage);
        if (lib_strcmp(pxnode->PXNODE_pcName, pcName) == 0) {
            break;
        }
    }
    
    if (plineTemp) {
        return  (pxnode);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: __pxnameAdd
** 功能描述: 创建一个名字节点.
** 输　入  : pxnode        名字节点
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __pxnameAdd (__PX_NAME_NODE  *pxnode)
{
    INT                iHash;
    
    if (pxnode == LW_NULL) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    iHash = __hashHorner(pxnode->PXNODE_pcName, __PX_NAME_NODE_HASH_SIZE);
                                                                        /*  确定 hash 表入口            */
    if (__pxnameSeach(pxnode->PXNODE_pcName, iHash)) {
        errno = EEXIST;                                                 /*  已经存在此节点              */
        return  (PX_ERROR);
    }
    API_AtomicSet(0, &pxnode->PXNODE_atomic);                           /*  初始化计数器                */
    
    _List_Line_Add_Ahead(&pxnode->PXNODE_lineManage,
                         &_G_plinePxNameNodeHash[iHash]);               /*  插入 hash 表                */
    _G_uiNamedNodeCounter++;
                         
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pxnameDel
** 功能描述: 删除一个名字节点.
** 输　入  : pcName            名字
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __pxnameDel (CPCHAR  pcName)
{
    __PX_NAME_NODE    *pxnode;
    INT                iHash;

    if (pcName == LW_NULL) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    iHash = __hashHorner(pcName, __PX_NAME_NODE_HASH_SIZE);             /*  确定 hash 表入口            */
    
    pxnode = __pxnameSeach(pcName, iHash);
    if (pxnode == LW_NULL) {
        errno = ENOENT;                                                 /*  没有对应节点                */
        return  (PX_ERROR);
    }
    
    _List_Line_Del(&pxnode->PXNODE_lineManage,
                   &_G_plinePxNameNodeHash[iHash]);                     /*  从 hash 表中删除            */
    _G_uiNamedNodeCounter--;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pxnameDelByNode
** 功能描述: 删除一个名字节点.
** 输　入  : pxnode            名字节点
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __pxnameDelByNode (__PX_NAME_NODE  *pxnode)
{
    INT  iHash;

    if (pxnode == LW_NULL) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    iHash = __hashHorner(pxnode->PXNODE_pcName, 
                         __PX_NAME_NODE_HASH_SIZE);                     /*  确定 hash 表入口            */
    
    _List_Line_Del(&pxnode->PXNODE_lineManage,
                   &_G_plinePxNameNodeHash[iHash]);                     /*  从 hash 表中删除            */
    _G_uiNamedNodeCounter--;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pxnameGet
** 功能描述: 获取一个名字节点.
** 输　入  : pcName            名字
**           ppvData           节点数据
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __pxnameGet (CPCHAR  pcName, PVOID  *ppvData)
{
    __PX_NAME_NODE    *pxnode;

    if ((pcName == LW_NULL) || (ppvData == LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pxnode = __pxnameSeach(pcName, -1);
    if (pxnode == LW_NULL) {
        errno = ENOENT;                                                 /*  没有对应节点                */
        return  (PX_ERROR);
    }
    
    *ppvData = pxnode->PXNODE_pvData;
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
