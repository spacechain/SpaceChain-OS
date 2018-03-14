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
** 文   件   名: rootFs.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 08 月 26 日
**
** 描        述: 根目录文件系统.
*********************************************************************************************************/

#ifndef __ROOTFS_H
#define __ROOTFS_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

/*********************************************************************************************************
  API
*********************************************************************************************************/
LW_API INT          API_RootFsDrvInstall(VOID);
LW_API INT          API_RootFsDevCreate(VOID);
LW_API time_t       API_RootFsTime(time_t  *time);

#define rootFsDrv                API_RootFsDrvInstall
#define rootFsDevCreate          API_RootFsDevCreate
#define rootFsTime               API_RootFsTime

/*********************************************************************************************************
  是否分级目录管理模式 API (以下操作谨慎使用! 推荐使用通用 IO 接口函数)
*********************************************************************************************************/
#if LW_CFG_PATH_VXWORKS == 0

#define LW_ROOTFS_NODE_TYPE_DIR             0                           /*  目录节点                    */
#define LW_ROOTFS_NODE_TYPE_DEV             1                           /*  设备节点                    */
#define LW_ROOTFS_NODE_TYPE_LNK             2                           /*  链接文件                    */
#define LW_ROOTFS_NODE_TYPE_SOCK            3                           /*  socket 文件 (AF_UNIX)       */
#define LW_ROOTFS_NODE_TYPE_REG             4                           /*  普通文件, 仅作为 IPC key    */

#define LW_ROOTFS_NODE_OPT_NONE             0
#define LW_ROOTFS_NODE_OPT_ROOTFS_TIME      0x01                        /*  使用根文件系统时间          */

LW_API INT          API_RootFsMakeNode(CPCHAR  pcName, INT  iNodeType, INT  iNodeOpt, 
                                       INT  iMode, PVOID  pvValue);
LW_API INT          API_RootFsRemoveNode(CPCHAR  pcName);               /*  unlink() 会自动调用此函数   */
                                                                        /*  用户慎用!                   */

#else                                                                   /*  使用 VxWorks 兼容设备目录   */
                                                                        /*  这里提供空函数              */
#define API_RootFsMakeNode(a, b, c, d, e)   (PX_ERROR)
#define API_RootFsRemoveNode(a)             (PX_ERROR)

#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */

#define rootFsMakeDir(pcPath, mode)   \
        API_RootFsMakeNode(pcPath, LW_ROOTFS_NODE_TYPE_DIR, LW_ROOTFS_NODE_OPT_NONE, mode, LW_NULL)
        
#define rootFsMakeDev(pcPath, pdevhdr)  \
        API_RootFsMakeNode(pcPath, LW_ROOTFS_NODE_TYPE_DEV, LW_ROOTFS_NODE_OPT_NONE, 0, (PVOID)(pdevhdr))
        
#define rootFsMakeSock(pcPath, mode)  \
        API_RootFsMakeNode(pcPath, LW_ROOTFS_NODE_TYPE_SOCK, LW_ROOTFS_NODE_OPT_NONE, mode, LW_NULL)
        
#define rootFsMakeReg(pcPath, mode)  \
        API_RootFsMakeNode(pcPath, LW_ROOTFS_NODE_TYPE_REG,  LW_ROOTFS_NODE_OPT_NONE, mode, LW_NULL)
        
#define rootFsRemoveNode    \
        API_RootFsRemoveNode

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
#endif                                                                  /*  __ROOTFS_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
