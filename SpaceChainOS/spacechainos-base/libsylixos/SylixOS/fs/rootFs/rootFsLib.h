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
** 文   件   名: rootFsLib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 03 日
**
** 描        述: root 文件系统内部库 (在安装所有设备之前, 必须安装 rootFs 驱动和 root 设备, 
                                      它定义所有设备的挂接点).
*********************************************************************************************************/

#ifndef __ROOTFSLIB_H
#define __ROOTFSLIB_H

/*********************************************************************************************************
  rootfs 节点内容
*********************************************************************************************************/

typedef union lw_rootfs_node_value {
    PCHAR                        RFSNV_pcName;                          /*  节点名字                    */
    PLW_DEV_HDR                  RFSNV_pdevhdr;                         /*  设备指针                    */
} LW_ROOTFS_NODE_VALUE;
typedef LW_ROOTFS_NODE_VALUE    *PLW_ROOTFS_NODE_VALUE;

/*********************************************************************************************************
  rootfs 节点
*********************************************************************************************************/

typedef struct lw_rootfs_node {
    LW_LIST_LINE                 RFSN_lineBrother;                      /*  兄弟节点                    */
    struct lw_rootfs_node       *RFSN_prfsnFather;                      /*  父系节点                    */
    PLW_LIST_LINE                RFSN_plineSon;                         /*  儿子节点                    */
    
    INT                          RFSN_iOpenNum;                         /*  打开次数                    */
    size_t                       RFSN_stAllocSize;                      /*  此节点占用内存大小          */
    mode_t                       RFSN_mode;                             /*  模式                        */
    time_t                       RFSN_time;                             /*  创建时间                    */
    INT                          RFSN_iNodeType;                        /*  节点类型                    */
    
    uid_t                        RFSN_uid;
    gid_t                        RFSN_gid;
    
    LW_ROOTFS_NODE_VALUE         RFSN_rfsnv;                            /*  节点的内容                  */
    PCHAR                        RFSN_pcLink;                           /*  链接目标 (不是链接文件为 0) */
} LW_ROOTFS_NODE;
typedef LW_ROOTFS_NODE          *PLW_ROOTFS_NODE;

/*********************************************************************************************************
  rootfs 根
*********************************************************************************************************/

typedef struct lw_rootfs_root {
    PLW_LIST_LINE                RFSR_plineSon;                         /*  指向第一个儿子              */
    size_t                       RFSR_stMemUsed;                        /*  内存消耗量                  */
    ULONG                        RFSR_ulFiles;                          /*  文件总数                    */
    time_t                       RFSR_time;                             /*  创建时间                    */
} LW_ROOTFS_ROOT;
typedef LW_ROOTFS_ROOT          *PLW_ROOTFS_ROOT;

/*********************************************************************************************************
  rootfs 全局变量
*********************************************************************************************************/
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PATH_VXWORKS == 0)

extern LW_ROOTFS_ROOT            _G_rfsrRoot;                           /*  rootFs 根                   */
extern LW_DEV_HDR                _G_devhdrRoot;                         /*  rootfs 设备                 */

#define __LW_IS_ROOTDEV(pdevhdr) ((pdevhdr) == &_G_devhdrRoot)

/*********************************************************************************************************
  rootfs 锁 (使用 IO 系统锁)
*********************************************************************************************************/

#define __LW_ROOTFS_LOCK()       _IosLock()
#define __LW_ROOTFS_UNLOCK()     _IosUnlock()

/*********************************************************************************************************
  rootfs 匹配设备
*********************************************************************************************************/

PLW_ROOTFS_NODE  __rootFsFindNode(CPCHAR            pcName, 
                                  PLW_ROOTFS_NODE  *pprfsnFather, 
                                  BOOL             *pbRoot,
                                  BOOL             *pbLast,
                                  PCHAR            *ppcTail);           /*  查找节点                    */
PLW_DEV_HDR  __rootFsDevMatch(CPCHAR  pcName);                          /*  匹配设备                    */

ssize_t      __rootFsReadNode(CPCHAR  pcName, 
                              PCHAR   pcBuffer, 
                              size_t  stSize);                          /*  读取链接节点内容            */

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_PATH_VXWORKS == 0    */
#endif                                                                  /*  __ROOTFSLIB_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
