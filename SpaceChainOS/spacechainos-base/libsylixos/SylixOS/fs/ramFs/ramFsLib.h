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
** 文   件   名: ramFsLib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 05 月 24 日
**
** 描        述: 内存文件系统内部函数.
*********************************************************************************************************/

#ifndef __RAMFSLIB_H
#define __RAMFSLIB_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0 && LW_CFG_RAMFS_EN > 0

/*********************************************************************************************************
  检测路径字串是否为根目录或者直接指向设备
*********************************************************************************************************/

#define __STR_IS_ROOT(pcName)           ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))

/*********************************************************************************************************
  内存配置
*********************************************************************************************************/

#if (LW_CFG_RAMFS_VMM_EN > 0) && (LW_CFG_VMM_EN > 0)
#define __RAM_BSIZE_SHIFT               LW_CFG_VMM_PAGE_SHIFT           /*  page size                   */
#else
#define __RAM_BSIZE_SHIFT               11                              /*  (1 << 11) = 2048 (blk size) */
#endif                                                                  /*  LW_CFG_RAMFS_VMM_EN         */

/*********************************************************************************************************
  一个内存分片大小
*********************************************************************************************************/

#define __RAM_BSIZE                     (1 << __RAM_BSIZE_SHIFT)
#define __RAM_BSIZE_MASK                ((1 << __RAM_BSIZE_SHIFT) - 1)
#define __RAM_BDATASIZE                 (__RAM_BSIZE - (sizeof(PVOID) * 2))

/*********************************************************************************************************
  文件缓存内存管理
*********************************************************************************************************/

#if (LW_CFG_RAMFS_VMM_EN > 0) && (LW_CFG_VMM_EN > 0)
#define __RAM_BALLOC(size)              API_VmmMalloc(size);
#define __RAM_BFREE(ptr)                API_VmmFree(ptr);
#else
#define __RAM_BALLOC(size)              __SHEAP_ALLOC(size)
#define __RAM_BFREE(ptr)                __SHEAP_FREE(ptr)
#endif                                                                  /*  LW_CFG_RAMFS_VMM_EN         */

/*********************************************************************************************************
  类型
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR          RAMFS_devhdrHdr;                                /*  ramfs 文件系统设备头        */
    LW_OBJECT_HANDLE    RAMFS_hVolLock;                                 /*  卷操作锁                    */
    LW_LIST_LINE_HEADER RAMFS_plineFdNodeHeader;                        /*  fd_node 链表                */
    LW_LIST_LINE_HEADER RAMFS_plineSon;                                 /*  儿子链表                    */
    
    BOOL                RAMFS_bForceDelete;                             /*  是否允许强制卸载卷          */
    BOOL                RAMFS_bValid;
    
    uid_t               RAMFS_uid;                                      /*  用户 id                     */
    gid_t               RAMFS_gid;                                      /*  组   id                     */
    mode_t              RAMFS_mode;                                     /*  文件 mode                   */
    time_t              RAMFS_time;                                     /*  创建时间                    */
    ULONG               RAMFS_ulCurBlk;                                 /*  当前消耗内存大小            */
    ULONG               RAMFS_ulMaxBlk;                                 /*  最大内存消耗量              */
} RAM_VOLUME;
typedef RAM_VOLUME     *PRAM_VOLUME;

typedef struct {
    LW_LIST_LINE        RAMB_lineManage;                                /*  管理链表                    */
    BYTE                RAMB_ucData[__RAM_BDATASIZE];                   /*  内存块                      */
} RAM_BUFFER;
typedef RAM_BUFFER     *PRAM_BUFFER;

typedef struct ramfs_node {
    LW_LIST_LINE        RAMN_lineBrother;                               /*  兄弟节点链表                */
    struct ramfs_node  *RAMN_pramnFather;                               /*  父亲指针                    */
    PLW_LIST_LINE       RAMN_plineSon;                                  /*  子节点链表                  */
    PRAM_VOLUME         RAMN_pramfs;                                    /*  文件系统                    */
    
    BOOL                RAMN_bChanged;                                  /*  文件内容是否更改            */
    mode_t              RAMN_mode;                                      /*  文件 mode                   */
    time_t              RAMN_timeCreate;                                /*  创建时间                    */
    time_t              RAMN_timeAccess;                                /*  最后访问时间                */
    time_t              RAMN_timeChange;                                /*  最后修改时间                */
    
    size_t              RAMN_stSize;                                    /*  当前文件大小 (可能大于缓冲) */
    size_t              RAMN_stVSize;                                   /*  lseek 出的虚拟大小          */
    
    uid_t               RAMN_uid;                                       /*  用户 id                     */
    gid_t               RAMN_gid;                                       /*  组   id                     */
    PCHAR               RAMN_pcName;                                    /*  文件名称                    */
    PCHAR               RAMN_pcLink;                                    /*  链接目标                    */
    
    PLW_LIST_LINE       RAMN_plineBStart;                               /*  文件头                      */
    PLW_LIST_LINE       RAMN_plineBEnd;                                 /*  文件尾                      */
    ULONG               RAMN_ulCnt;                                     /*  文件数据块数量              */
    
    PRAM_BUFFER         RAMN_prambCookie;                               /*  文件 cookie                 */
    ULONG               RAMN_ulCookie;                                  /*  文件 cookie 下标            */
} RAM_NODE;
typedef RAM_NODE       *PRAM_NODE;

/*********************************************************************************************************
  内部函数
*********************************************************************************************************/
PRAM_NODE   __ram_open(PRAM_VOLUME  pramfs,
                       CPCHAR       pcName,
                       PRAM_NODE   *ppramnFather,
                       BOOL        *pbRoot,
                       BOOL        *pbLast,
                       PCHAR       *ppcTail);
PRAM_NODE   __ram_maken(PRAM_VOLUME  pramfs,
                        CPCHAR       pcName,
                        PRAM_NODE    pramnFather,
                        mode_t       mode,
                        CPCHAR       pcLink);
INT         __ram_unlink(PRAM_NODE  pramn);
VOID        __ram_truncate(PRAM_NODE  pramn, size_t  stOft);
VOID        __ram_increase(PRAM_NODE  pramn, size_t  stNewSize);
ssize_t     __ram_read(PRAM_NODE  pramn, PVOID  pvBuffer, size_t  stSize, size_t  stOft);
ssize_t     __ram_write(PRAM_NODE  pramn, CPVOID  pvBuffer, size_t  stNBytes, size_t  stOft);
VOID        __ram_mount(PRAM_VOLUME  pramfs);
VOID        __ram_unmount(PRAM_VOLUME  pramfs);
VOID        __ram_close(PRAM_NODE  pramn, INT iFlag);
INT         __ram_move(PRAM_NODE  pramn, PCHAR  pcNewName);
VOID        __ram_stat(PRAM_NODE  pramn, PRAM_VOLUME  pramfs, struct stat  *pstat);
VOID        __ram_statfs(PRAM_VOLUME  pramfs, struct statfs  *pstatfs);
                       
#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_RAMFS_EN > 0         */
#endif                                                                  /*  __RAMFSLIB_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
