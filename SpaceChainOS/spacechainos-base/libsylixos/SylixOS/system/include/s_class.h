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
** 文   件   名: s_class.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 13 日
**
** 描        述: 这是系统系统级控件类型定义文件。

** BUG
2007.04.08  删除裁剪宏控制，类型的定义，和裁剪无关
2008.03.23  今天是 2008 台湾地区领导人选举后的第一天, 真希望两岸和平统一.
            改进 I/O 系统, 加入 BSD 和 Linux 系统的相关功能.
2009.02.13  今天开始大刀阔斧改进 I/O 系统, 修改为类似 UNIX/BSD I/O 管理模式, 但是使用特殊算法, 是常用 API
            保持时间复杂度 O(1) 时间复杂度.
2009.08.31  加入对应于设备的功耗管理模块.
2009.10.02  设备头打开次数使用 atomic_t 型.
2009.11.21  加入 io 环境概念.
2009.12.14  在驱动程序接口中加入 symlink 功能. 注意: 符号链接只有部分文件系统支持(部分功能)! 
2010.09.10  file_operations 加入 fstat 操作方法.
2011.07.28  file_operations 加入 mmap 操作方法.
2011.08.07  file_operations 加入 lstat 操作方法.
2012.10.19  LW_FD_NODE 加入引用计数 FDNODE_ulRef, 例如 mmap() 时产生计数. munmap() 时减少计数, 当计数为 0
            时关闭文件.
2012.11.21  文件结构中加入了 unlink 标志, 开始支持文件没有关闭时的 unlink 操作.
2012.12.21  大幅度的更新文件描述符表
            加入一种新的机制: FDENTRY_bRemoveReq 表示如果此时文件结构链表忙, 则请求推迟删除其中一个节点,
            这个新的机制事遍历文件解构表时, 不再锁定 IO 系统.
2013.01.09  LW_IO_ENV 中加入 umask 参数.
2013.05.27  fd entry 加入去除符号连接以外的真实文件名.
2014.07.19  加入新的电源管理机制.
*********************************************************************************************************/

#ifndef __S_CLASS_H
#define __S_CLASS_H

/*********************************************************************************************************
  IO 环境
*********************************************************************************************************/

typedef struct {
    mode_t               IOE_modeUMask;                                 /*  umask                       */
    CHAR                 IOE_cDefPath[MAX_FILENAME_LENGTH];             /*  默认当前路径                */
} LW_IO_ENV;
typedef LW_IO_ENV       *PLW_IO_ENV;

/*********************************************************************************************************
  IO 用户信息
*********************************************************************************************************/

typedef struct {
    uid_t                IOU_uid;
    gid_t                IOU_gid;
} LW_IO_USR;
typedef LW_IO_USR       *PLW_IO_USR;

/*********************************************************************************************************
  电源管理
  
  SylixOS 电源管理分为两大部分:
  
  1: CPU 功耗管理
  2: 外设功耗管理
  
  CPU 功耗分为三个能级: 1: 正常运行   2: 省电模式 (PowerSaving) 3: 休眠模式(Sleep)
  
  正常运行: CPU 正常执行指令
  省电模式: 所有具有电源管理功能的设备进入 PowerSaving 模式, 同时 CPU 降速, 多核 CPU 仅保留一个 CPU 运行.
  休眠模式: 系统休眠, 所有具有电源管理功能的设备进入 Suspend 模式, 系统需要通过指定事件唤醒. 
            休眠模式系统将从复位向量处恢复, 需要 bootloader/BIOS 程序配合.

  
  外设功耗管理分为三个状态: 1: 正常运行 2: 设备关闭 3: 省电模式 4: 打开的设备长时间不使用
  
  正常运行: 设备被打开, 驱动程序调用 pmDevOn() 请求电源管理适配器连通设备电源与时钟, 开始工作.
  设备关闭: 设备被关闭, 驱动程序调用 pmDevOff() 请求电源管理适配器断开设备电源与时钟, 停止工作.
  省电模式: 系统进入省电模式, 请求高能耗设备进入省电模式, PMDF_pfuncPowerSavingEnter 将会被调用.
  
  打开的设备长时间不使用: 设备功耗管理单元具有看门狗功能呢, 一旦空闲时间超过设置, 系统会自动调用 
                          PMDF_pfuncIdleEnter 请求将设备变为 idle 状态.
                          应用程序可以通过 FIOSETWATCHDOG 来设置 idle 时间并激活此功能.
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0

struct lw_pma_funcs;
struct lw_pmd_funcs;

typedef struct {
    LW_LIST_LINE         PMA_lineManage;                                /*  管理链表                    */
    UINT                 PMA_uiMaxChan;                                 /*  电源管理通道总数            */
    struct lw_pma_funcs *PMA_pmafunc;                                   /*  电源管理适配器操作函数      */
    PVOID                PMA_pvReserve[8];
    CHAR                 PMA_cName[1];                                  /*  电源管理适配器名称          */
} LW_PM_ADAPTER;
typedef LW_PM_ADAPTER   *PLW_PM_ADAPTER;

typedef struct {
    LW_LIST_LINE         PMD_lineManage;                                /*  管理链表                    */
    PLW_PM_ADAPTER       PMD_pmadapter;                                 /*  电源管理适配器              */
    UINT                 PMD_uiChannel;                                 /*  对应电源管理适配器通道号    */
    PVOID                PMD_pvReserve[8];

    PCHAR                PMD_pcName;                                    /*  管理节点名                  */
    PVOID                PMD_pvBus;                                     /*  总线信息 (驱动程序自行使用) */
    PVOID                PMD_pvDev;                                     /*  设备信息 (驱动程序自行使用) */
    
    UINT                 PMD_uiStatus;                                  /*  初始为 0                    */
#define LW_PMD_STAT_NOR     0
#define LW_PMD_STAT_IDLE    1

    LW_CLASS_WAKEUP_NODE PMD_wunTimer;                                  /*  空闲时间计算                */
#define PMD_bInQ         PMD_wunTimer.WUN_bInQ
#define PMD_ulCounter    PMD_wunTimer.WUN_ulCounter
    
    struct lw_pmd_funcs *PMD_pmdfunc;                                   /*  电源管理适配器操作函数      */
} LW_PM_DEV;
typedef LW_PM_DEV       *PLW_PM_DEV;

/*********************************************************************************************************
  电源管理函数组
*********************************************************************************************************/

typedef struct lw_pma_funcs {
    INT                (*PMAF_pfuncOn)(PLW_PM_ADAPTER  pmadapter, 
                                       PLW_PM_DEV      pmdev);          /*  打开设备电源与时钟          */
    INT                (*PMAF_pfuncOff)(PLW_PM_ADAPTER  pmadapter, 
                                        PLW_PM_DEV      pmdev);         /*  关闭设备电源与时钟          */
    INT                (*PMAF_pfuncIsOn)(PLW_PM_ADAPTER  pmadapter, 
                                         PLW_PM_DEV      pmdev,
                                         BOOL           *pbIsOn);       /*  是否打开                    */
    PVOID                PMAF_pvReserve[16];                            /*  保留                        */
} LW_PMA_FUNCS;
typedef LW_PMA_FUNCS    *PLW_PMA_FUNCS;

typedef struct lw_pmd_funcs {
    INT                (*PMDF_pfuncSuspend)(PLW_PM_DEV  pmdev);         /*  CPU 休眠                    */
    INT                (*PMDF_pfuncResume)(PLW_PM_DEV  pmdev);          /*  CPU 恢复                    */
    
    INT                (*PMDF_pfuncPowerSavingEnter)(PLW_PM_DEV  pmdev);/*  系统进入省电模式            */
    INT                (*PMDF_pfuncPowerSavingExit)(PLW_PM_DEV  pmdev); /*  系统退出省电模式            */
    
    INT                (*PMDF_pfuncIdleEnter)(PLW_PM_DEV  pmdev);       /*  设备长时间不使用进入空闲    */
    INT                (*PMDF_pfuncIdleExit)(PLW_PM_DEV  pmdev);        /*  设备退出空闲                */
    
    INT                (*PMDF_pfuncCpuPower)(PLW_PM_DEV  pmdev);        /*  CPU 改变主频能级            */
    PVOID                PMDF_pvReserve[16];                            /*  保留                        */
} LW_PMD_FUNCS;
typedef LW_PMD_FUNCS    *PLW_PMD_FUNCS;

#endif                                                                  /*  LW_CFG_POWERM_EN            */
/*********************************************************************************************************
  设备头
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE               DEVHDR_lineManage;                       /*  设备头管理链表              */
    UINT16                     DEVHDR_usDrvNum;                         /*  设备驱动程序索引号          */
    PCHAR                      DEVHDR_pcName;                           /*  设备名称                    */
    UCHAR                      DEVHDR_ucType;                           /*  设备 dirent d_type          */
    atomic_t                   DEVHDR_atomicOpenNum;                    /*  打开的次数                  */
    PVOID                      DEVHDR_pvReserve;                        /*  保留                        */
} LW_DEV_HDR;
typedef LW_DEV_HDR            *PLW_DEV_HDR;

static LW_INLINE INT  LW_DEV_INC_USE_COUNT(PLW_DEV_HDR  pdevhdrHdr)
{
    return  ((pdevhdrHdr) ? (API_AtomicInc(&pdevhdrHdr->DEVHDR_atomicOpenNum)) : (PX_ERROR));
}

static LW_INLINE INT  LW_DEV_DEC_USE_COUNT(PLW_DEV_HDR  pdevhdrHdr)
{
    return  ((pdevhdrHdr) ? (API_AtomicDec(&pdevhdrHdr->DEVHDR_atomicOpenNum)) : (PX_ERROR));
}

static LW_INLINE INT  LW_DEV_GET_USE_COUNT(PLW_DEV_HDR  pdevhdrHdr)
{
    return  ((pdevhdrHdr) ? (API_AtomicGet(&pdevhdrHdr->DEVHDR_atomicOpenNum)) : (PX_ERROR));
}

typedef LW_DEV_HDR             DEV_HDR;

/*********************************************************************************************************
  驱动程序许可证
*********************************************************************************************************/

typedef struct {
    PCHAR                      DRVLIC_pcLicense;                        /*  许可证                      */
    PCHAR                      DRVLIC_pcAuthor;                         /*  作者                        */
    PCHAR                      DRVLIC_pcDescription;                    /*  描述                        */
} LW_DRV_LICENSE;

/*********************************************************************************************************
  驱动程序类型
*********************************************************************************************************/

#define LW_DRV_TYPE_ORIG      0                                         /*  原始设备驱动, 兼容 VxWorks  */
#define LW_DRV_TYPE_SOCKET    1                                         /*  SOCKET 型设备驱动程序       */
#define LW_DRV_TYPE_NEW_1     2                                         /*  NEW_1 型设备驱动程序        */

/*********************************************************************************************************
  驱动程序控制块 (主设备) (只有使能 SylixOS 新的目录管理功能才能使用符号链接)
*********************************************************************************************************/

typedef struct {
    LONGFUNCPTR                DEVENTRY_pfuncDevCreate;                 /*  建立函数                    */
    FUNCPTR                    DEVENTRY_pfuncDevDelete;                 /*  删除函数                    */
    
    LONGFUNCPTR                DEVENTRY_pfuncDevOpen;                   /*  打开函数                    */
    FUNCPTR                    DEVENTRY_pfuncDevClose;                  /*  关闭函数                    */
    
    SSIZETFUNCPTR              DEVENTRY_pfuncDevRead;                   /*  读设备函数                  */
    SSIZETFUNCPTR              DEVENTRY_pfuncDevWrite;                  /*  写设备函数                  */
    
    SSIZETFUNCPTR              DEVENTRY_pfuncDevReadEx;                 /*  读设备函数扩展函数          */
    SSIZETFUNCPTR              DEVENTRY_pfuncDevWriteEx;                /*  读设备函数扩展函数          */
    
    FUNCPTR                    DEVENTRY_pfuncDevIoctl;                  /*  设备控制函数                */
    FUNCPTR                    DEVENTRY_pfuncDevSelect;                 /*  select 功能                 */
    OFFTFUNCPTR                DEVENTRY_pfuncDevLseek;                  /*  lseek 功能                  */
    
    FUNCPTR                    DEVENTRY_pfuncDevFstat;                  /*  fstat 功能                  */
    FUNCPTR                    DEVENTRY_pfuncDevLstat;                  /*  lstat 功能                  */
    
    FUNCPTR                    DEVENTRY_pfuncDevSymlink;                /*  建立链接文件                */
    SSIZETFUNCPTR              DEVENTRY_pfuncDevReadlink;               /*  读取链接文件                */
    
    FUNCPTR                    DEVENTRY_pfuncDevMmap;                   /*  文件映射                    */
    FUNCPTR                    DEVENTRY_pfuncDevUnmap;                  /*  映射结束                    */
    
    BOOL                       DEVENTRY_bInUse;                         /*  是否被使用                  */
    INT                        DEVENTRY_iType;                          /*  设备驱动类型                */
    
    LW_DRV_LICENSE             DEVENTRY_drvlicLicense;                  /*  驱动程序许可证              */
} LW_DEV_ENTRY;
typedef LW_DEV_ENTRY          *PLW_DEV_ENTRY;

/*********************************************************************************************************
  设备文件操作控制块
*********************************************************************************************************/

typedef struct file_operations {
    struct module              *owner;
    
    long                      (*fo_create)();                           /*  DEVENTRY_pfuncDevCreate     */
    int                       (*fo_release)();                          /*  DEVENTRY_pfuncDevDelete     */
    
    long                      (*fo_open)();                             /*  DEVENTRY_pfuncDevOpen       */
    int                       (*fo_close)();                            /*  DEVENTRY_pfuncDevClose      */
    
    ssize_t                   (*fo_read)();                             /*  DEVENTRY_pfuncDevRead       */
    ssize_t                   (*fo_read_ex)();                          /*  DEVENTRY_pfuncDevReadEx     */
    
    ssize_t                   (*fo_write)();                            /*  DEVENTRY_pfuncDevWrite      */
    ssize_t                   (*fo_write_ex)();                         /*  DEVENTRY_pfuncDevWriteEx    */
    
    int                       (*fo_ioctl)();                            /*  DEVENTRY_pfuncDevIoctl      */
    int                       (*fo_select)();                           /*  DEVENTRY_pfuncDevSelect     */
    
    int                       (*fo_lock)();                             /*  not support now             */
    off_t                     (*fo_lseek)();                            /*  DEVENTRY_pfuncDevLseek      */
    
    int                       (*fo_fstat)();                            /*  DEVENTRY_pfuncDevFstat      */
    int                       (*fo_lstat)();                            /*  DEVENTRY_pfuncDevLstat      */
    
    int                       (*fo_symlink)();                          /*  DEVENTRY_pfuncDevSymlink    */
    ssize_t                   (*fo_readlink)();                         /*  DEVENTRY_pfuncDevReadlink   */
    
    int                       (*fo_mmap)();                             /*  DEVENTRY_pfuncDevMmap       */
    int                       (*fo_unmap)();                            /*  DEVENTRY_pfuncDevUnmmap     */
    
    ULONG                       fo_pad[16];                             /*  reserve                     */
} FILE_OPERATIONS;
typedef FILE_OPERATIONS        *PFILE_OPERATIONS;

/*********************************************************************************************************
  fo_mmap 和 fo_unmap 地址信息参数
*********************************************************************************************************/

typedef struct {
    PVOID                       DMAP_pvAddr;                            /*  起始地址                    */
    size_t                      DMAP_stLen;                             /*  内存长度 (单位:字节)        */
    off_t                       DMAP_offPages;                          /*  文件映射偏移量 (单位:页面)  */
    ULONG                       DMAP_ulFlag;                            /*  当前虚拟地址 VMM 属性       */
} LW_DEV_MMAP_AREA;
typedef LW_DEV_MMAP_AREA       *PLW_DEV_MMAP_AREA;

/*********************************************************************************************************
  SylixOS I/O 系统结构
  
  (ORIG 型驱动)
  
       0                       1                       N
  +---------+             +---------+             +---------+
  | FD_DESC |             | FD_DESC |     ...     | FD_DESC |
  +---------+             +---------+             +---------+
       |                       |                       |
       |                       |                       |
       \-----------------------/                       |
                   |                                   |
                   |                                   |
             +------------+                      +------------+
  HEADER ->  |  FD_ENTERY |   ->    ...   ->     |  FD_ENTERY |  ->  NULL
             +------------+                      +------------+
                   |                                   |
                   |                                   |
                   |                                   |
             +------------+                      +------------+
             |  DEV_HDR   |                      |  DEV_HDR   |
             +------------+                      +------------+
                   |                                   |
                   |                                   |
                   \-----------------------------------/
                                    |
                                    |
                              +------------+
                              |   DRIVER   |
                              +------------+
  
  (NEW_1 型驱动)
  
       0                       1                       N
  +---------+             +---------+             +---------+
  | FD_DESC |             | FD_DESC |     ...     | FD_DESC |
  +---------+             +---------+             +---------+
       |                       |                       |
       |                       |                       |
       \-----------------------/                       |
                   |                                   |
                   |                                   |
             +------------+                      +------------+
  HEADER ->  |  FD_ENTERY |   ->    ...   ->     |  FD_ENTERY |  ->  NULL
             +------------+                      +------------+
                   |                 |                 |
                   |                 |                 |
                   \-----------------/                 |
                             |                         |
                             |                         |
                      +------------+             +------------+
                      |   FD_NODE  |    ...      |   FD_NODE  |
                      | lockf->... |             | lockf->... |
                      +------------+             +------------+
                             |           |             |
                             |           |             |
                             \-----------/             |
                                   |                   |
                                   |                   |
                            +------------+       +------------+
                            |  DEV_HDR   |       |  DEV_HDR   |
                            +------------+       +------------+
                                   |                   |
                                   |                   |
                                   \-------------------/
                                             |
                                             |
                                       +------------+
                                       |   DRIVER   |
                                       +------------+
*********************************************************************************************************/
/*********************************************************************************************************
  文件记录锁
  只有 NEW_1 或更高级的设备驱动类型可以支持文件记录锁.
*********************************************************************************************************/

typedef struct __fd_lockf {
    LW_OBJECT_HANDLE           FDLOCK_ulBlock;                          /*  阻塞锁                      */
    INT16                      FDLOCK_usFlags;                          /*  F_WAIT F_FLOCK F_POSIX      */
    INT16                      FDLOCK_usType;                           /*  F_RDLCK F_WRLCK             */
    off_t                      FDLOCK_oftStart;                         /*  start of the lock           */
    off_t                      FDLOCK_oftEnd;                           /*  end of the lock (-1=EOF)    */
    pid_t                      FDLOCK_pid;                              /*  resource holding process    */
    
    struct  __fd_lockf       **FDLOCK_ppfdlockHead;                     /*  back to the head of list    */
    struct  __fd_lockf        *FDLOCK_pfdlockNext;                      /*  Next lock on this fd_node,  */
                                                                        /*  or blocking lock            */
    
    LW_LIST_LINE_HEADER        FDLOCK_plineBlockHd;                     /*  阻塞在当前锁的记录锁队列    */
    LW_LIST_LINE               FDLOCK_lineBlock;                        /*  请求等待链表 header is lockf*/
    LW_LIST_LINE               FDLOCK_lineBlockQ;                       /*  阻塞链表, header is fd_node */
} LW_FD_LOCKF;
typedef LW_FD_LOCKF           *PLW_FD_LOCKF;

#define F_WAIT                 0x10                                     /* wait until lock is granted   */
#define F_FLOCK                0x20                                     /* BSD semantics for lock       */
#define F_POSIX                0x40                                     /* POSIX semantics for lock     */
#define F_ABORT                0x80                                     /* lock wait abort!             */

/*********************************************************************************************************
  文件节点 
  
  只有 NEW_1 或更高级的设备驱动类型会用到此结构
  一个 dev_t 和 一个 ino_t 对应唯一一个实体文件, 操作系统统多次打开同一个实体文件时, 只有一个文件节点
  多个 fd_entry 同时指向这个节点.
  需要说明的是 FDNODE_oftSize 字段需要 NEW_1 驱动程序自己来维护.
  
  fd node 被锁定时, 将不允许写, 也不允许删除. 当关闭文件后此文件的 lock 将自动被释放.
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE               FDNODE_lineManage;                       /*  同一设备 fd_node 链表       */
    
    LW_OBJECT_HANDLE           FDNODE_ulSem;                            /*  内部操作锁                  */
    dev_t                      FDNODE_dev;                              /*  设备                        */
    ino64_t                    FDNODE_inode64;                          /*  inode (64bit 为了兼容性)    */
    mode_t                     FDNODE_mode;                             /*  文件 mode                   */
    uid_t                      FDNODE_uid;                              /*  文件所属用户信息            */
    gid_t                      FDNODE_gid;
    
    off_t                      FDNODE_oftSize;                          /*  当前文件大小                */
    
    struct  __fd_lockf        *FDNODE_pfdlockHead;                      /*  第一个锁                    */
    LW_LIST_LINE_HEADER        FDNODE_plineBlockQ;                      /*  当前有阻塞的记录锁队列      */
    
    BOOL                       FDNODE_bRemove;                          /*  是否在文件未关闭时有 unlink */
    ULONG                      FDNODE_ulLock;                           /*  锁定, 不允许写, 不允许删除  */
    ULONG                      FDNODE_ulRef;                            /*  fd_entry 引用此 fd_node 数量*/
    PVOID                      FDNODE_pvFile;                           /*  驱动使用此变量标示文件      */
    PVOID                      FDNODE_pvFsExtern;                       /*  文件系统扩展使用            */
} LW_FD_NODE;
typedef LW_FD_NODE            *PLW_FD_NODE;

/*********************************************************************************************************
  文件结构 (文件表)
  FDENTRY_ulCounter 计数器是所有对应的 LW_FD_DESC 计数器总和. 
  如果没有 dup 过, 则 FDENTRY_ulCounter 应该与 FDDESC_ulRef 相同.
  
  如果是 ORIG 类型驱动:
  则 open 后传入驱动的首参数为 FDENTRY_lValue.
  
  如果是 NEW_1 类型驱动:
  则 open 后传入驱动的首参数为 fd_entry 本身, FDENTRY_lValue 为 open 返回值其首参数必须为 LW_FD_NODE.
  需要说明的是 FDENTRY_oftPtr 字段需要驱动程序自己来维护.
*********************************************************************************************************/

typedef struct {
    PLW_DEV_HDR                FDENTRY_pdevhdrHdr;                      /*  设备头                      */
    PCHAR                      FDENTRY_pcName;                          /*  文件名                      */
    PCHAR                      FDENTRY_pcRealName;                      /*  去除符号链接的真实文件名    */
    LW_LIST_LINE               FDENTRY_lineManage;                      /*  文件控制信息遍历表          */
    
#define FDENTRY_pfdnode        FDENTRY_lValue
    LONG                       FDENTRY_lValue;                          /*  驱动程序内部数据            */
                                                                        /*  如果为 NEW_1 驱动则为fd_node*/
                                                                        
    INT                        FDENTRY_iType;                           /*  文件类型 (根据驱动判断)     */
    INT                        FDENTRY_iFlag;                           /*  文件属性                    */
    INT                        FDENTRY_iAbnormity;                      /*  文件异常                    */
    ULONG                      FDENTRY_ulCounter;                       /*  总引用计数器                */
    off_t                      FDENTRY_oftPtr;                          /*  文件当前指针                */
                                                                        /*  只有 NEW_1 或更高级驱动使用 */
    BOOL                       FDENTRY_bRemoveReq;                      /*  删除请求                    */
} LW_FD_ENTRY;
typedef LW_FD_ENTRY           *PLW_FD_ENTRY;

/*********************************************************************************************************
  文件描述符表
*********************************************************************************************************/

typedef struct {
    PLW_FD_ENTRY               FDDESC_pfdentry;                         /*  文件结构                    */
    BOOL                       FDDESC_bCloExec;                         /*  FD_CLOEXEC                  */
    ULONG                      FDDESC_ulRef;                            /*  对应文件描述符的引用计数    */
} LW_FD_DESC;
typedef LW_FD_DESC            *PLW_FD_DESC;

/*********************************************************************************************************
    线程池
*********************************************************************************************************/

typedef struct {
    LW_LIST_MONO               TPCB_monoResrcList;                      /*  资源链表                    */
    PTHREAD_START_ROUTINE      TPCB_pthreadStartAddress;                /*  线程代码入口点              */
    PLW_LIST_RING              TPCB_pringFirstThread;                   /*  第一个线程                  */
    LW_CLASS_THREADATTR        TPCB_threakattrAttr;                     /*  线程建立属性块              */
    
    UINT16                     TPCB_usMaxThreadCounter;                 /*  最多线程数量                */
    UINT16                     TPCB_usThreadCounter;                    /*  当前线程数量                */
    
    LW_OBJECT_HANDLE           TPCB_hMutexLock;                         /*  操作锁                      */
    UINT16                     TPCB_usIndex;                            /*  索引下标                    */
                                                                        /*  名字                        */
    CHAR                       TPCB_cThreadPoolName[LW_CFG_OBJECT_NAME_SIZE];
} LW_CLASS_THREADPOOL;
typedef LW_CLASS_THREADPOOL   *PLW_CLASS_THREADPOOL;

#endif                                                                  /*  __S_CLASS_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
