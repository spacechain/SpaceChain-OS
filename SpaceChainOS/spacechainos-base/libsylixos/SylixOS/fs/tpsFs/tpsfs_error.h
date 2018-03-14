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
** 文   件   名: tpsfs_error.h
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: tpsfs 错误定义

** BUG:
*********************************************************************************************************/

#ifndef __TPSFS_ERROR_H
#define __TPSFS_ERROR_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0

typedef enum {
    TPS_ERR_NONE = 0,

    TPS_ERR_PARAM_NULL,                                                 /* 参数为空                     */
    TPS_ERR_PARAM,                                                      /* 参数错误                     */
    TPS_ERR_ALLOC,                                                      /* 内存分配错误                 */
    TPS_ERR_FORMAT,                                                     /* 文件系统格式错误             */

    TPS_ERR_SECTOR_SIZE,                                                /* 扇区大小错误                 */
    TPS_ERR_BLOCK_SIZE,                                                 /* 扇区大小错误                 */
    TPS_ERR_READSECTOR,                                                 /* 读扇区错误                   */
    TPS_ERR_WRITESECTOR,                                                /* 写扇区错误                   */
    TPS_ERR_DEV_SYNC,                                                   /* 扇区同步错误                 */

    TPS_ERR_WALKPATH,                                                   /* 解析路径失败                 */
    TPS_ERR_OP_NOT_SUPPORT,                                             /* 不支持该操作                 */

    TPS_ERR_CHECK_LOG,                                                  /* 日志检查错误                 */
    TPS_ERR_COMLETE_LOG,                                                /* 标记文件系统一致性错误       */

    TPS_ERR_INODE_GET,                                                  /* 获取inode错误                */
    TPS_ERR_INODE_OPEN ,                                                /* 打开inode错误                */
    TPS_ERR_FORMAT_SETTING,                                             /* 格式化设置错误               */
    TPS_ERR_INODE_CREAT,                                                /* 创建inode错误                */
    TPS_ERR_UNMOUNT_INUSE,                                              /* 卸载正在使用的卷             */
    TPS_ERR_INODE_ADD_BLK,                                              /* 添加块到inode错误            */
    TPS_ERR_INODE_WRITE,                                                /* 写文件错误                   */
    TPS_ERR_INODE_READ,                                                 /* 读文件错误                   */
    TPS_ERR_INODE_DELETED,                                              /* 删除inode错误                */
    TPS_ERR_INODE_TRUNC,                                                /* 截断inode错误                */
    TPS_ERR_INODE_SIZE,                                                 /* inode大小错误                */
    TPS_ERR_INODE_SYNC,                                                 /* 同步inode错误                */
    TPS_ERR_INODE_SERIAL,                                               /* 序列号inode错误              */
    TPS_ERR_INODE_HASHNOEMPTY,                                          /* 目录非空                     */
    TPS_ERR_INODE_BUFF,                                                 /* inode缓冲区错误              */

    TPS_ERR_BTREE_INIT,                                                 /* 初始化b+tree                 */
    TPS_ERR_BTREE_INSERT,                                               /* 插入块到b+tree错误           */
    TPS_ERR_BTREE_TRUNC,                                                /* 截断b+tree错误               */
    TPS_ERR_BTREE_ALLOC,                                                /* 分配b+tree节点错误           */
    TPS_ERR_BTREE_FREE,                                                 /* 释放b+tree节点错误           */
    TPS_ERR_BTREE_FREE_ND,                                              /* 释放b+tree节点到块缓冲错误   */
    TPS_ERR_BTREE_KEY_AREADY_EXIT,                                      /* 键值冲突                     */
    TPS_ERR_BTREE_PUT_NODE,                                             /* 写b+tree节点到磁盘错误       */
    TPS_ERR_BTREE_GET_NODE,                                             /* 从磁盘获取b+tree节点错误     */
    TPS_ERR_BTREE_INSERT_NODE,                                          /* 插入节点到b+tree错误         */
    TPS_ERR_BTREE_NODE_TYPE,                                            /* 节点类型错误                 */
    TPS_ERR_BTREE_NODE_NOT_EXIST,                                       /* 指定键值节点不存在           */
    TPS_ERR_BTREE_BLOCK_COUNT,                                          /* 块数量参数错误               */
    TPS_ERR_BTREE_NODE_OVERLAP,                                         /* 块区间重叠                   */
    TPS_ERR_BTREE_NODE_REMOVE,                                          /* 删除b+tree节点错误           */
    TPS_ERR_BTREE_KEY_NOTFOUND,                                         /* 查找指定键值错误             */
    TPS_ERR_BTREE_UPDATE_KEY,                                           /* 更新键值错                   */
    TPS_ERR_BTREE_DISK_SPACE,                                           /* 磁盘空间不足                 */
    TPS_ERR_BTREE_NODE_MAGIC,                                           /* BTREE 节点 magic错误         */
    TPS_ERR_BTREE_OVERFLOW,                                             /* BTREE节点访问溢出            */

    TPS_ERR_CHECK_NAME,                                                 /* 文件路径检查错误             */

    TPS_ERR_TRANS_ALLOC,                                                /* 分配事物错误                 */
    TPS_ERR_TRANS_WRITE,                                                /* 事务写操作错误               */
    TPS_ERR_TRANS_COMMIT,                                               /* 提交事务错误                 */
    TPS_ERR_TRANS_CHECK,                                                /* 分配事物错误                 */
    TPS_ERR_TRANS_OVERFLOW,                                             /* 事务内存溢出                 */
    TPS_ERR_TRANS_NEED_COMMIT,                                          /* 事务需要提交                 */
    TPS_TRAN_INIT_SIZE,                                                 /* 初始化事务区间大小错误       */
    TPS_ERR_TRANS_COMMIT_FAULT,                                         /* 事务提交过程出错             */
    TPS_ERR_TRANS_ROLLBK_FAULT,                                         /* 回滚事务过程出错             */
    TPS_ERR_TRANS_READ,                                                 /* 事务读取失败                 */

    TPS_ERR_ENTRY_NOT_EXIST,                                            /* 文件不存在                   */
    TPS_ERR_ENTRY_AREADY_EXIST,                                         /* 文件已存在                   */
    TPS_ERR_ENTRY_CREATE,                                               /* 创建目录项错误               */
    TPS_ERR_ENTRY_FIND,                                                 /* 查找目录项错误               */
    TPS_ERR_ENTRY_REMOVE,                                               /* 删除目录项错误               */
    TPS_ERR_ENTRY_UNEQUAL,                                              /* 目录名不相等                 */

    TPS_ERR_BUF_READ,                                                   /* 读磁盘错误                   */
    TPS_ERR_BUF_WRITE,                                                  /* 写磁盘错误                   */
    TPS_ERR_BUF_SYNC,                                                   /* 同步磁盘错误                 */
    TPS_ERR_BUF_TRIM,                                                   /* 回收磁盘块错误               */
    TPS_ERR_SEEK_DEV,                                                   /* seek磁盘设备错误             */
    TPS_ERR_READ_DEV,                                                   /* 读磁盘设备错误               */
    TPS_ERR_WRITE_DEV,                                                  /* 写磁盘设备错误               */

    TPS_ERR_BP_INIT,                                                    /* 初始化块缓冲队列错误         */
    TPS_ERR_BP_READ,                                                    /* 读块缓冲队列错误             */
    TPS_ERR_BP_WRITE,                                                   /* 写块缓冲队列错误             */
    TPS_ERR_BP_ALLOC,                                                   /* 从块缓冲队列分配块错误       */
    TPS_ERR_BP_FREE,                                                    /* 释放块到块缓冲队列错误       */
    TPS_ERR_BP_ADJUST,                                                  /* 调整块缓冲队列大小错误       */

    TPS_ERR_DIR_MK,                                                     /* 创建目录错误                 */

    TPS_ERR_HASH_EXIST,                                                 /* hash节点已存在               */
    TPS_ERR_HASH_TOOLONG_NAME,                                          /* 存放在hash节点中的名字过长   */
    TPS_ERR_HASH_INSERT,                                                /* 插入hash节点失败             */
    TPS_ERR_HASH_NOT_EXIST,                                             /* hash节点不存在               */
    TPS_ERR_HASH_REMOVE,                                                /* 删除hash节点失败             */

    TPS_ERR_UNEXPECT                                                    /* 位置错误                     */
} TPS_RESULT;

#endif                                                                  /* LW_CFG_TPSFS_EN > 0          */
#endif                                                                  /* __TPSFS_ERROR_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
