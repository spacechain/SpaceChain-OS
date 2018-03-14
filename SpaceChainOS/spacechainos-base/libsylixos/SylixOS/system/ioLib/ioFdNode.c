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
** 文   件   名: ioFdNode.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 01 月 06 日
**
** 描        述: 针对 NEW_1 型设备驱动的 fd_node 操作
*********************************************************************************************************/
#define  __SYLIXOS_STDARG
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
** 函数名称: API_IosFdNodeAdd
** 功能描述: 添加一个 fd_node (如果同一个设备已经有一个重复的 inode 被打开则只增加引用)
** 输　入  : pplineHeader  同一设备 fd_node 链表
**           dev           设备描述符
**           inode64       文件 inode (同一设备 inode 必须唯一)
**           iFlags        打开选项
**           mode          文件 mode
**           uid           所属用户 id
**           gid           所属组 id
**           oftSize       文件当前大小
**           pvFile        驱动私有信息
**           pbIsNew       返回给驱动程序告知是否为新建节点还是
** 输　出  : fd_node
** 全局变量: 
** 调用模块: 
** 注  意  : 此函数线程不安全, 需要在设备锁内执行
                                           API 函数
*********************************************************************************************************/
LW_API 
PLW_FD_NODE  API_IosFdNodeAdd (LW_LIST_LINE_HEADER  *pplineHeader,
                               dev_t                 dev,
                               ino64_t               inode64,
                               INT                   iFlags,
                               mode_t                mode,
                               uid_t                 uid,
                               gid_t                 gid,
                               off_t                 oftSize,
                               PVOID                 pvFile,
                               BOOL                 *pbIsNew)
{
    PLW_LIST_LINE   plineTemp;
    PLW_FD_NODE     pfdnode;
    
    for (plineTemp  = *pplineHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  遍历已经打开的文件          */
         
        pfdnode = _LIST_ENTRY(plineTemp, LW_FD_NODE, FDNODE_lineManage);
        if ((pfdnode->FDNODE_dev     == dev) &&
            (pfdnode->FDNODE_inode64 == inode64)) {                     /*  重复打开                    */
            
            if (pfdnode->FDNODE_bRemove ||
                (pfdnode->FDNODE_ulLock && 
                (iFlags & (O_WRONLY | O_RDWR | O_TRUNC)))) {
                _ErrorHandle(EBUSY);                                    /*  文件被锁定或者请求删除      */
                return  (LW_NULL);
            }
            
            pfdnode->FDNODE_ulRef++;
            if (pbIsNew) {
                *pbIsNew = LW_FALSE;                                    /*  只增加引用非创建            */
            }
            return  (pfdnode);
        }
    }
    
    pfdnode = (PLW_FD_NODE)__SHEAP_ALLOC(sizeof(LW_FD_NODE));
    if (pfdnode == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    lib_bzero(pfdnode, sizeof(LW_FD_NODE));
    
    pfdnode->FDNODE_ulSem = API_SemaphoreBCreate("fd_node_lock", LW_TRUE, 
                                                 LW_OPTION_OBJECT_GLOBAL |
                                                 LW_OPTION_WAIT_PRIORITY, LW_NULL);
    if (pfdnode->FDNODE_ulSem == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pfdnode);
        return  (LW_NULL);
    }
    
    pfdnode->FDNODE_dev     = dev;
    pfdnode->FDNODE_inode64 = inode64;
    pfdnode->FDNODE_mode    = mode;
    pfdnode->FDNODE_uid     = uid;
    pfdnode->FDNODE_gid     = gid;
    pfdnode->FDNODE_oftSize = oftSize;
    pfdnode->FDNODE_pvFile  = pvFile;
    pfdnode->FDNODE_bRemove = LW_FALSE;
    pfdnode->FDNODE_ulLock  = 0;                                        /*  没有锁定                    */
    pfdnode->FDNODE_ulRef   = 1;                                        /*  初始化引用为 1              */
    
    if (pbIsNew) {
        *pbIsNew = LW_TRUE;
    }
    
    _List_Line_Add_Ahead(&pfdnode->FDNODE_lineManage,
                         pplineHeader);
    
    return  (pfdnode);
}
/*********************************************************************************************************
** 函数名称: API_IosFdNodeDec
** 功能描述: 删除一个 fd_node (如果引用不为 1 则只是 -- )
** 输　入  : pplineHeader  同一设备 fd_node 链表
**           pfdnode       fd_node
**           bRemove       是否需要删除文件.
** 输　出  : -1 : 错误
**            0 : 正常删除
**           >0 : 减少引用后的引用数量
** 全局变量: 
** 调用模块: 
** 注  意  : 此函数线程不安全, 需要在设备锁内执行
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_IosFdNodeDec (LW_LIST_LINE_HEADER  *pplineHeader,
                       PLW_FD_NODE           pfdnode,
                       BOOL                 *bRemove)
{
    if (!pfdnode) {
        return  (PX_ERROR);
    }

    pfdnode->FDNODE_ulRef--;
    if (pfdnode->FDNODE_ulRef) {
        if (bRemove) {
            *bRemove = LW_FALSE;
        }
        return  ((INT)pfdnode->FDNODE_ulRef);                           /*  仅仅减少文件引用计数即可    */
    }
    
    _FdLockfClearFdNode(pfdnode);                                       /*  删除所有记录锁              */
    
    _List_Line_Del(&pfdnode->FDNODE_lineManage,
                   pplineHeader);
    
    API_SemaphoreBDelete(&pfdnode->FDNODE_ulSem);
    
    if (bRemove) {
        *bRemove = pfdnode->FDNODE_bRemove;
    }
    
    __SHEAP_FREE(pfdnode);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_IosFdNodeFind
** 功能描述: 查找一个 fd_node 
** 输　入  : plineHeader   同一设备 fd_node 链表
**           dev           设备描述符
**           inode64       文件 inode (同一设备 inode 必须唯一)
** 输　出  : fd_node
** 全局变量: 
** 调用模块: 
** 注  意  : 此函数线程不安全, 需要在设备锁内执行
                                           API 函数
*********************************************************************************************************/
LW_API 
PLW_FD_NODE  API_IosFdNodeFind (LW_LIST_LINE_HEADER  plineHeader, dev_t  dev, ino64_t  inode64)
{
    PLW_LIST_LINE   plineTemp;
    PLW_FD_NODE     pfdnode;
    
    for (plineTemp  = plineHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  遍历已经打开的文件          */
         
        pfdnode = _LIST_ENTRY(plineTemp, LW_FD_NODE, FDNODE_lineManage);
        if ((pfdnode->FDNODE_dev     == dev) &&
            (pfdnode->FDNODE_inode64 == inode64)) {
            return  (pfdnode);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_IosFdNodeLock
** 功能描述: 锁定一个 fd_node 在文件关闭前不允许写, 不允许被删除, 关闭文件后自动解除.
** 输　入  : pfdnode       fd_node
** 输　出  : NONE
** 全局变量: -1 : 错误
**            0 : 正常删除
** 调用模块: 
** 注  意  : 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_IosFdNodeLock (PLW_FD_NODE  pfdnode)
{
    if (!pfdnode) {
        return  (PX_ERROR);
    }

    pfdnode->FDNODE_ulLock = 1;
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
