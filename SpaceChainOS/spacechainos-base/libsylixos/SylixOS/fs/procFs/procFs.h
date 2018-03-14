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
** 文   件   名: procFs.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 03 日
**
** 描        述: proc 文件系统.
*********************************************************************************************************/

#ifndef __PROCFS_H
#define __PROCFS_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_PROCFS_EN > 0)

#ifdef __SYLIXOS_KERNEL

#include "procFsLib.h"

/*********************************************************************************************************
  内部 API 函数 (内部是有的路径起始为 procfs 根, 而非操作系统的 根目录)
  
  注意: 如果 API_ProcFsRemoveNode() 返回 -1 errno == EBUSY, 系统已提交删除请求, 
        他将在最后一次关闭时执行删除操作.
*********************************************************************************************************/
LW_API INT      API_ProcFsMakeNode(PLW_PROCFS_NODE  p_pfsnNew, CPCHAR  pcFatherName);
LW_API INT      API_ProcFsRemoveNode(PLW_PROCFS_NODE  p_pfsn, VOIDFUNCPTR  pfuncFree);
LW_API INT      API_ProcFsAllocNodeBuffer(PLW_PROCFS_NODE  p_pfsn, size_t  stSize);
LW_API INT      API_ProcFsFreeNodeBuffer(PLW_PROCFS_NODE  p_pfsn);
LW_API size_t   API_ProcFsNodeBufferSize(PLW_PROCFS_NODE  p_pfsn);
LW_API PVOID    API_ProcFsNodeBuffer(PLW_PROCFS_NODE  p_pfsn);
LW_API PVOID    API_ProcFsNodeMessageValue(PLW_PROCFS_NODE  p_pfsn);
LW_API VOID     API_ProcFsNodeSetRealFileSize(PLW_PROCFS_NODE  p_pfsn, size_t  stRealSize);
LW_API size_t   API_ProcFsNodeGetRealFileSize(PLW_PROCFS_NODE  p_pfsn);

#define procFsMakeNode                  API_ProcFsMakeNode
#define procFsRemoveNode                API_ProcFsRemoveNode
#define procFsAllocNodeBuffer           API_ProcFsAllocNodeBuffer
#define procFsFreeNodeBuffer            API_ProcFsFreeNodeBuffer
#define procFsNodeBufferSize            API_ProcFsNodeBufferSize
#define procFsNodeBuffer                API_ProcFsNodeBuffer
#define procFsNodeMessageValue          API_ProcFsNodeMessageValue
#define procFsNodeSetRealFileSize       API_ProcFsNodeSetRealFileSize
#define procFsNodeGetRealFileSize       API_ProcFsNodeGetRealFileSize


#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  API 函数
*********************************************************************************************************/

LW_API INT      API_ProcFsDrvInstall(VOID);
LW_API INT      API_ProcFsDevCreate(VOID);

#define procFsDrv               API_ProcFsDrvInstall
#define procFsDevCreate         API_ProcFsDevCreate

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_PROCFS_EN > 0)      */
#endif                                                                  /*  __PROCFS_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
