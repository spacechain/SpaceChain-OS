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
** 文   件   名: procFsLib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 03 日
**
** 描        述: proc 文件系统内部库.
*********************************************************************************************************/

#ifndef __PROCFSLIB_H
#define __PROCFSLIB_H

/*********************************************************************************************************
  proc 节点私有操作. 
  注意: PFSNO_pfuncRead, PFSNO_pfuncWrite 比标准 io 多一个参数 offt (第四个参数)
*********************************************************************************************************/

typedef struct lw_procfs_node_op {
    SSIZETFUNCPTR                PFSNO_pfuncRead;                       /*  读操作函数                  */
    SSIZETFUNCPTR                PFSNO_pfuncWrite;                      /*  写操作函数                  */
} LW_PROCFS_NODE_OP;
typedef LW_PROCFS_NODE_OP       *PLW_PROCFS_NODE_OP;

/*********************************************************************************************************
  proc 节点信息
*********************************************************************************************************/

typedef struct lw_procfs_node_message {
    PVOID                        PFSNM_pvValue;                         /*  文件相关私有信息            */
    off_t                        PFSNM_oftPtr;                          /*  文件当前指针                */
                                                                        /*  节点驱动程序不要处理此变量! */
    /*
     *  SylixOS 系统的 proc 文件系统主要用于显示操作系统内核状态, 信息量非常少.
     *  所以这里使用简单的缓冲结构, 而没有使用多页面缓冲.
     */
    PVOID                        PFSNM_pvBuffer;                        /*  文件内存缓冲                */
                                                                        /*  (必须初始化为 NULL)         */
    size_t                       PFSNM_stBufferSize;                    /*  文件当前缓冲大小            */
    size_t                       PFSNM_stNeedSize;                      /*  预估的需要缓冲的大小        */
    size_t                       PFSNM_stRealSize;                      /*  文件真实大小                */
                                                                        /*  (由节点驱动程序确定)        */
} LW_PROCFS_NODE_MSG;
typedef LW_PROCFS_NODE_MSG      *PLW_PROCFS_NODE_MSG;

/*********************************************************************************************************
  proc 节点 (第一个资源必须为 Brother)
*********************************************************************************************************/
#define LW_PROCFS_EMPTY_MESSAGE(pvValue, ulNeedSize)    {pvValue, 0, LW_NULL, 0, ulNeedSize, 0}
                                                                        /*  空消息                      */
#define LW_PROCFS_EMPTY_BROTHER                         {LW_NULL, LW_NULL}  
                                                                        /*  没有兄弟节点                */
/*********************************************************************************************************
  proc 节点静态初始化 (符号文件只能通过 LW_PROCFS_INIT_SYMLINK_IN_CODE 初始化)
*********************************************************************************************************/
#define LW_PROCFS_INIT_NODE(pcName, mode, p_pfsnoFuncs, pvValue, ulNeedSize)    \
        {                                                                       \
            LW_PROCFS_EMPTY_BROTHER,                                            \
            LW_NULL,                                                            \
            LW_NULL,                                                            \
            pcName,                                                             \
            0,                                                                  \
            LW_FALSE,                                                           \
            LW_NULL,                                                            \
            mode,                                                               \
            0,                                                                  \
            0,                                                                  \
            0,                                                                  \
            p_pfsnoFuncs,                                                       \
            LW_PROCFS_EMPTY_MESSAGE(pvValue, ulNeedSize),                       \
        }
        
/*********************************************************************************************************
  proc 节点动态初始化
*********************************************************************************************************/
#define LW_PROCFS_INIT_NODE_IN_CODE(pfsn, pcName, mode, p_pfsnoFuncs, pvValue, stNeedSize)  \
        do {                                                                                \
            lib_bzero((pfsn), sizeof(*(pfsn)));                                             \
            (pfsn)->PFSN_pcName = pcName;                                                   \
            (pfsn)->PFSN_mode   = mode;                                                     \
            (pfsn)->PFSN_p_pfsnoFuncs = p_pfsnoFuncs;                                       \
            (pfsn)->PFSN_pfsnmMessage.PFSNM_pvValue    = pvValue;                           \
            (pfsn)->PFSN_pfsnmMessage.PFSNM_stNeedSize = stNeedSize;                        \
        } while (0)

#define LW_PROCFS_INIT_SYMLINK_IN_CODE(pfsn, pcName, mode, p_pfsnoFuncs, pvValue, pcDst)    \
        do {                                                                                \
            size_t  stFileSize = lib_strlen(pcDst) + 1;                                     \
            lib_bzero((pfsn), sizeof(*(pfsn)));                                             \
            (pfsn)->PFSN_pcName = pcName;                                                   \
            (pfsn)->PFSN_mode   = mode | S_IFLNK;                                           \
            (pfsn)->PFSN_p_pfsnoFuncs = p_pfsnoFuncs;                                       \
            (pfsn)->PFSN_pfsnmMessage.PFSNM_pvValue    = pvValue;                           \
            (pfsn)->PFSN_pfsnmMessage.PFSNM_pvBuffer   = __SHEAP_ALLOC(stFileSize);         \
            if ((pfsn)->PFSN_pfsnmMessage.PFSNM_pvBuffer == LW_NULL) {                      \
                _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);                                      \
                break;                                                                      \
            }                                                                               \
            lib_strcpy((PCHAR)(pfsn)->PFSN_pfsnmMessage.PFSNM_pvBuffer, pcDst);             \
            (pfsn)->PFSN_pfsnmMessage.PFSNM_stRealSize = stFileSize;                        \
            (pfsn)->PFSN_pfsnmMessage.PFSNM_stNeedSize = stFileSize;                        \
        } while (0)
/*********************************************************************************************************
  proc 节点类型
*********************************************************************************************************/
typedef struct lw_procfs_node {
    LW_LIST_LINE                 PFSN_lineBrother;                      /*  兄弟节点                    */
    struct lw_procfs_node       *PFSN_p_pfsnFather;                     /*  父系节点                    */
    PLW_LIST_LINE                PFSN_plineSon;                         /*  儿子节点                    */
    PCHAR                        PFSN_pcName;                           /*  节点名                      */
    INT                          PFSN_iOpenNum;                         /*  打开的次数                  */
    BOOL                         PFSN_bReqRemove;                       /*  是否请求删除                */
    VOIDFUNCPTR                  PFSN_pfuncFree;                        /*  请求删除释放函数            */
    mode_t                       PFSN_mode;                             /*  节点类型                    */
    time_t                       PFSN_time;                             /*  节点时间, 一般为当前时间    */
    uid_t                        PFSN_uid;
    gid_t                        PFSN_gid;
    PLW_PROCFS_NODE_OP           PFSN_p_pfsnoFuncs;                     /*  文件操作函数                */
    LW_PROCFS_NODE_MSG           PFSN_pfsnmMessage;                     /*  节点私有数据                */
#define PFSN_pvValue             PFSN_pfsnmMessage.PFSNM_pvValue
} LW_PROCFS_NODE;
typedef LW_PROCFS_NODE          *PLW_PROCFS_NODE;

/*********************************************************************************************************
  proc 根
*********************************************************************************************************/

typedef struct lw_procfs_root {
    PLW_LIST_LINE                PFSR_plineSon;                         /*  指向第一个儿子              */
    ULONG                        PFSR_ulFiles;                          /*  文件总数                    */
} LW_PROCFS_ROOT;
typedef LW_PROCFS_ROOT          *PLW_PROCFS_ROOT;

/*********************************************************************************************************
  procfs 全局变量
*********************************************************************************************************/
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0

extern LW_PROCFS_ROOT            _G_pfsrRoot;                           /*  procFs 根                   */
extern LW_OBJECT_HANDLE          _G_ulProcFsLock;                       /*  procFs 操作锁               */

/*********************************************************************************************************
  procfs 锁
*********************************************************************************************************/

#define __LW_PROCFS_LOCK()       API_SemaphoreMPend(_G_ulProcFsLock, LW_OPTION_WAIT_INFINITE)
#define __LW_PROCFS_UNLOCK()     API_SemaphoreMPost(_G_ulProcFsLock)

#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
#endif                                                                  /*  __PROCFSLIB_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
