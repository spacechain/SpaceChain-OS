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
** 文   件   名: ioFile.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 12 月 21 日
**
** 描        述: 系统加入进程功能后, 每个进程有独立的文件描述符表, 这里实现一些基本的文件映射

** BUG:
2012.12.25  加入是否在内核空间的判断, 这样就可以实现内核文件虚拟的设备可以被进程访问
            例如, mount 的 nfs 文件系统内部的 socket.
2013.01.03  所有关于驱动 close 的操作全部放在 _IosFileClose 中完成.
2013.01.05  将文件锁统一在 _IosFileIoctl 中操作.
2013.01.06  _IosFileSet 如果是 NEW_1 型驱动, 正常打开时, 如果有 O_APPEND 参数则需要将读写指针放在文件结尾
2013.01.11  简化 IO 操作, 去掉不再需要的函数.
2013.05.31  加入对 fd_node 类型文件的加锁操作, 不允许写, 不允许删除.
2013.11.18  _IosFileDup() 加入一个参数, 可以控制最小文件描述符数值.
2015.03.02  进程回收 I/O 资源时 socket 需要先复位连接.
2013.01.06  _IosFileSet 不再处理 O_APPEND 选项.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
  内核文件描述符基本操作
*********************************************************************************************************/
#define STD_FIX(fd)         ((fd) + 3)
#define STD_UNFIX(fd)       ((fd) - 3)
#define STD_MAP(fd)         (STD_UNFIX(((fd) >= 0 && (fd) < 3) ?    \
                             API_IoTaskStdGet(API_ThreadIdSelf(), fd) : (fd)))
/*********************************************************************************************************
  缩写宏定义
*********************************************************************************************************/
#define __LW_FD_MAINDRV     (pfdentry->FDENTRY_pdevhdrHdr->DEVHDR_usDrvNum)
/*********************************************************************************************************
  SylixOS I/O 系统结构
  
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
                  ...                                 ...
                  
 (这一层之下不同的驱动版本有区别)
*********************************************************************************************************/
/*********************************************************************************************************
  全局文件描述符表
*********************************************************************************************************/
static LW_FD_DESC       _G_fddescTbl[LW_CFG_MAX_FILES];                 /*  启动时自动清零              */
/*********************************************************************************************************
** 函数名称: _IosFileListLock
** 功能描述: 锁定文件结构链表
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _IosFileListLock (VOID)
{
    API_AtomicInc(&_S_atomicFileLineOp);
}
/*********************************************************************************************************
** 函数名称: _IosFileListRemoveReq
** 功能描述: 由于 fd_entry 链表被锁定, 这里请求在解锁时删除请求删除的点 (IO 系统锁定情况下被调用)
** 输　入  : pfdentry      fd_entry 节点
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _IosFileListRemoveReq (PLW_FD_ENTRY   pfdentry)
{
    pfdentry->FDENTRY_bRemoveReq = LW_TRUE;
    _S_bFileEntryRemoveReq = LW_TRUE;
}
/*********************************************************************************************************
** 函数名称: _IosFileListUnlock
** 功能描述: 解锁文件结构链表 (如果是最后一次解锁, 则需要删除请求删除的节点)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _IosFileListUnlock (VOID)
{
    REGISTER PLW_LIST_LINE  plineFdEntry;
    REGISTER PLW_FD_ENTRY   pfdentry;
    
    _IosLock();                                                         /*  进入 IO 临界区              */
    if (API_AtomicDec(&_S_atomicFileLineOp) == 0) {
        if (_S_bFileEntryRemoveReq) {                                   /*  存在请求删除                */
            plineFdEntry = _S_plineFileEntryHeader;
            while (plineFdEntry) {
                pfdentry = _LIST_ENTRY(plineFdEntry, LW_FD_ENTRY, FDENTRY_lineManage);
                plineFdEntry = _list_line_get_next(plineFdEntry);
                
                if (pfdentry->FDENTRY_bRemoveReq) {
                    _List_Line_Del(&pfdentry->FDENTRY_lineManage,
                                   &_S_plineFileEntryHeader);           /*  从文件结构表中删除          */
                    __SHEAP_FREE(pfdentry);
                }
            }
            _S_bFileEntryRemoveReq = LW_FALSE;
        }
    }
    _IosUnlock();                                                       /*  退出 IO 临界区              */
}
/*********************************************************************************************************
** 函数名称: _IosFileListIslock
** 功能描述: 文件结构链表是否在占用
** 输　入  : NONE
** 输　出  : 占用层数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  _IosFileListIslock (VOID)
{
    return  (API_AtomicGet(&_S_atomicFileLineOp));
}
/*********************************************************************************************************
** 函数名称: _IosFileGetKernel
** 功能描述: 通过 fd 获得系统 fd_entry
** 输　入  : iFd           文件描述符
**           bIsIgnAbn     如果是异常文件也获取
** 输　出  : fd_entry
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PLW_FD_ENTRY  _IosFileGetKernel (INT  iFd, BOOL  bIsIgnAbn)
{
    PLW_FD_DESC     pfddesc;
    
    iFd = STD_MAP(iFd);
    
    if (iFd < 0 || iFd >= LW_CFG_MAX_FILES) {
        return  (LW_NULL);
    }
    
    pfddesc = &_G_fddescTbl[iFd];
    
    if (pfddesc->FDDESC_pfdentry && pfddesc->FDDESC_ulRef) {
        if (bIsIgnAbn) {                                                /*  忽略异常文件                */
            return  (pfddesc->FDDESC_pfdentry);
        
        } else if (!pfddesc->FDDESC_pfdentry->FDENTRY_iAbnormity) {
            return  (pfddesc->FDDESC_pfdentry);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: _IosFileGet
** 功能描述: 通过 fd 获得 fd_entry
** 输　入  : iFd           文件描述符
**           bIsIgnAbn     如果是异常文件也获取
** 输　出  : fd_entry
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PLW_FD_ENTRY  _IosFileGet (INT  iFd, BOOL  bIsIgnAbn)
{
    PLW_FD_DESC     pfddesc;
    
    if (iFd < 0) {
        return  (LW_NULL);
    }

    if ((__PROC_GET_PID_CUR() != 0) && (!__KERNEL_SPACE_ISENTER())) {   /*  需要获取进程文件信息        */
        if (_S_pfuncFileGet) {
            return  (_S_pfuncFileGet(iFd, bIsIgnAbn));
        }
    
    } else {
        iFd = STD_MAP(iFd);
        
        if (iFd < 0 || iFd >= LW_CFG_MAX_FILES) {
            return  (LW_NULL);
        }
        
        pfddesc = &_G_fddescTbl[iFd];
        
        if (pfddesc->FDDESC_pfdentry && pfddesc->FDDESC_ulRef) {
            if (bIsIgnAbn) {                                            /*  忽略异常文件                */
                return  (pfddesc->FDDESC_pfdentry);
            
            } else if (!pfddesc->FDDESC_pfdentry->FDENTRY_iAbnormity) {
                return  (pfddesc->FDDESC_pfdentry);
            }
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: _IosFileDescGet
** 功能描述: 通过 fd 获得 filedesc
** 输　入  : iFd           文件描述符
**           bIsIgnAbn     如果是异常文件也获取
** 输　出  : filedesc
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PLW_FD_DESC  _IosFileDescGet (INT  iFd, BOOL  bIsIgnAbn)
{
    PLW_FD_DESC     pfddesc;
    
    if (iFd < 0) {
        return  (LW_NULL);
    }
    
    if ((__PROC_GET_PID_CUR() != 0) && (!__KERNEL_SPACE_ISENTER())) {   /*  需要获取进程文件信息        */
        if (_S_pfuncFileDescGet) {
            return  (_S_pfuncFileDescGet(iFd, bIsIgnAbn));
        }
    
    } else {
        
        iFd = STD_MAP(iFd);
        
        if (iFd < 0 || iFd >= LW_CFG_MAX_FILES) {
            return  (LW_NULL);
        }
        
        pfddesc = &_G_fddescTbl[iFd];
        
        if (pfddesc->FDDESC_pfdentry && pfddesc->FDDESC_ulRef) {
            if (bIsIgnAbn) {                                            /*  忽略异常文件                */
                return  (pfddesc);
            
            } else if (!pfddesc->FDDESC_pfdentry->FDENTRY_iAbnormity) {
                return  (pfddesc);
            }
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: _IosFileDup
** 功能描述: 查找一个空的文件描述符, 并把文件结构 pfdentry 放入, 并且初始化文件描述符计数器为 1 
** 输　入  : pfdentry      文件结构
             (如成功则 pfdentry 的引用数量++)
**           iMinFd        最小描述符数值
** 输　出  : 新的文件描述符
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  _IosFileDup (PLW_FD_ENTRY pfdentry, INT iMinFd)
{
    INT             i;
    PLW_FD_DESC     pfddesc;

    if ((__PROC_GET_PID_CUR() != 0) && (!__KERNEL_SPACE_ISENTER())) {   /*  需要获取进程文件信息        */
        if (_S_pfuncFileDup) {
            return  (_S_pfuncFileDup(pfdentry, iMinFd));
        }
    
    } else {
        if (iMinFd >= 3) {                                              /*  内核 IO 环境 0 1 2 为映射符 */
            iMinFd = STD_UNFIX(iMinFd);
        }
    
        for (i = iMinFd; i < LW_CFG_MAX_FILES; i++) {
            pfddesc = &_G_fddescTbl[i];
            if (!pfddesc->FDDESC_pfdentry) {
                pfddesc->FDDESC_pfdentry = pfdentry;
                pfddesc->FDDESC_bCloExec = LW_FALSE;
                pfddesc->FDDESC_ulRef    = 1ul;                         /*  新创建的 fd 引用数为 1      */
                pfdentry->FDENTRY_ulCounter++;                          /*  总引用数++                  */
                return  (STD_FIX(i));
            }
        }
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: _IosFileDup2
** 功能描述: 查找一个空的文件描述符, 并把文件结构 pfdentry 放入, 并且初始化文件描述符计数器为 1 
**           (如成功则 pfdentry 的引用数量++)
** 输　入  : pfdentry      文件结构
**           iNewFd        新的文件描述符 (内核文件不支持 dup 的目标为 0 ~ 2)
** 输　出  : 新的文件描述符
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  _IosFileDup2 (PLW_FD_ENTRY pfdentry, INT  iNewFd)
{
    INT             i;
    PLW_FD_DESC     pfddesc;
    
    if (iNewFd < 0) {
        return  (PX_ERROR);
    }

    if ((__PROC_GET_PID_CUR() != 0) && (!__KERNEL_SPACE_ISENTER())) {   /*  需要获取进程文件信息        */
        if (_S_pfuncFileDup2) {
            return  (_S_pfuncFileDup2(pfdentry, iNewFd));
        }
        
    } else {
        if ((iNewFd >= 0) && (iNewFd < 3)) {                            /*  内核不支持标准文件重定向    */
            return  (PX_ERROR);
        }
        
        i = STD_UNFIX(iNewFd);
        
        if (i < 0 || i >= LW_CFG_MAX_FILES) {
            return  (PX_ERROR);
        }

        pfddesc = &_G_fddescTbl[i];
        
        if (!pfddesc->FDDESC_pfdentry) {                                /*  需要提前关闭这个对应的文件  */
            pfddesc->FDDESC_pfdentry = pfdentry;
            pfddesc->FDDESC_bCloExec = LW_FALSE;
            pfddesc->FDDESC_ulRef    = 1ul;                             /*  新创建的 fd 引用数为 1      */
            pfdentry->FDENTRY_ulCounter++;                              /*  总引用数++                  */
            return  (iNewFd);
        }
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: _IosFileRefInc
** 功能描述: 通过 fd 获得设置 fd_entry 引用计数++
** 输　入  : iFd           文件描述符
** 输　出  : ++ 后的引用数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  _IosFileRefInc (INT  iFd)
{
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;
    
    if (iFd < 0) {
        return  (PX_ERROR);
    }

    if ((__PROC_GET_PID_CUR() != 0) && (!__KERNEL_SPACE_ISENTER())) {   /*  需要获取进程文件信息        */
        if (_S_pfuncFileRefInc) {
            return  (_S_pfuncFileRefInc(iFd));
        }
        
    } else {
        iFd = STD_MAP(iFd);
        
        if (iFd < 0 || iFd >= LW_CFG_MAX_FILES) {
            return  (PX_ERROR);
        }
        
        pfddesc = &_G_fddescTbl[iFd];
        
        pfdentry = pfddesc->FDDESC_pfdentry;
        if (pfdentry) {
            pfddesc->FDDESC_ulRef++;
            pfdentry->FDENTRY_ulCounter++;                              /*  总引用数++                  */
            return  ((INT)pfddesc->FDDESC_ulRef);
        }
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: _IosFileRefDec
** 功能描述: 通过 fd 获得设置 fd_entry 引用计数--
** 输　入  : iFd           文件描述符
** 输　出  : -- 后的引用数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  _IosFileRefDec (INT  iFd)
{
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;
    
    if (iFd < 0) {
        return  (PX_ERROR);
    }

    if ((__PROC_GET_PID_CUR() != 0) && (!__KERNEL_SPACE_ISENTER())) {   /*  需要获取进程文件信息        */
        if (_S_pfuncFileRefDec) {
            return  (_S_pfuncFileRefDec(iFd));
        }
        
    } else {
        iFd = STD_MAP(iFd);
        
        if (iFd < 0 || iFd >= LW_CFG_MAX_FILES) {
            return  (PX_ERROR);
        }
        
        pfddesc = &_G_fddescTbl[iFd];
        
        pfdentry = pfddesc->FDDESC_pfdentry;
        if (pfdentry) {
            pfddesc->FDDESC_ulRef--;
            pfdentry->FDENTRY_ulCounter--;                              /*  总引用数--                  */
            if (pfddesc->FDDESC_ulRef == 0) {
                pfddesc->FDDESC_pfdentry = LW_NULL;
                pfddesc->FDDESC_bCloExec = LW_FALSE;
            }
            return  ((INT)pfddesc->FDDESC_ulRef);
        }
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: _IosFileRefGet
** 功能描述: 通过 fd 获得获得 fd_entry 引用计数
** 输　入  : iFd           文件描述符
** 输　出  : 引用数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  _IosFileRefGet (INT  iFd)
{
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;
    
    if (iFd < 0) {
        return  (PX_ERROR);
    }

    if ((__PROC_GET_PID_CUR() != 0) && (!__KERNEL_SPACE_ISENTER())) {   /*  需要获取进程文件信息        */
        if (_S_pfuncFileRefGet) {
            return  (_S_pfuncFileRefGet(iFd));
        }
        
    } else {
        iFd = STD_MAP(iFd);
        
        if (iFd < 0 || iFd >= LW_CFG_MAX_FILES) {
            return  (PX_ERROR);
        }
        
        pfddesc = &_G_fddescTbl[iFd];
        
        pfdentry = pfddesc->FDDESC_pfdentry;
        if (pfdentry) {
            return  ((INT)pfddesc->FDDESC_ulRef);
        }
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: _IosFileNew
** 功能描述: 创建一个 fd_entry 结构
** 输　入  : pdevhdrHdr        设备头
**           pcName            名字
** 输　出  : fd_entry
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PLW_FD_ENTRY  _IosFileNew (PLW_DEV_HDR  pdevhdrHdr, CPCHAR  pcName)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
             size_t          stNameLen;
             size_t          stDevNameLen;
             size_t          stAllocLen;
             
    if (pcName) {
        stNameLen    = lib_strlen(pcName);
        stDevNameLen = lib_strlen(pdevhdrHdr->DEVHDR_pcName);
        if (stNameLen && (stDevNameLen == 1)) {                         /*  根目录设备 "/" 且存在后缀   */
            stDevNameLen = 0;                                           /*  只拷贝文件名                */
        }
        stAllocLen = stNameLen + stDevNameLen + 1 + sizeof(LW_FD_ENTRY);
    
    } else {
        stAllocLen = sizeof(LW_FD_ENTRY);
    }

    pfdentry = (PLW_FD_ENTRY)__SHEAP_ALLOC(stAllocLen);
    if (pfdentry == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    lib_bzero(pfdentry, sizeof(LW_FD_ENTRY));                           /*  结构清零                    */
    
    pfdentry->FDENTRY_lValue      = PX_ERROR;                           /*  不存在驱动信息              */
    pfdentry->FDENTRY_ulCounter   = 0;                                  /*  如果 dup 成功则引用为 1     */
    pfdentry->FDENTRY_iAbnormity  = 1;                                  /*  异常文件 (还不允许操作)     */
    pfdentry->FDENTRY_bRemoveReq  = LW_FALSE;
    
    if (pcName == LW_NULL) {
        pfdentry->FDENTRY_pcName = LW_NULL;
        
    } else {
        pfdentry->FDENTRY_pcName = (PCHAR)pfdentry + sizeof(LW_FD_ENTRY);
        if (stDevNameLen) {
            lib_strcpy(pfdentry->FDENTRY_pcName, pdevhdrHdr->DEVHDR_pcName);
        }
        if (stNameLen) {
            lib_strcpy(&pfdentry->FDENTRY_pcName[stDevNameLen], pcName);/*  保存绝对路径文件名          */
        }
    }
    
    pfdentry->FDENTRY_pcRealName = pfdentry->FDENTRY_pcName;            /*  默认等统于打开时的文件名    */
    
    _IosLock();                                                         /*  进入 IO 临界区              */
	_List_Line_Add_Ahead(&pfdentry->FDENTRY_lineManage,                 /*  ADD 操作不影响遍历, 不加锁  */
	                     &_S_plineFileEntryHeader);                     /*  加入文件结构表              */
	_IosUnlock();                                                       /*  退出 IO 临界区              */
	
    return  (pfdentry);
}
/*********************************************************************************************************
** 函数名称: _IosFileDelete
** 功能描述: 删除一个 fd_entry 结构
** 输　入  : pfdentry          fd_entry
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _IosFileDelete (PLW_FD_ENTRY    pfdentry)
{
    _IosLock();                                                         /*  进入 IO 临界区              */
    if (_IosFileListIslock()) {                                         /*  链表忙, 这里只要请求就行了  */
        _IosFileListRemoveReq(pfdentry);
    
    } else {
        _List_Line_Del(&pfdentry->FDENTRY_lineManage,
                       &_S_plineFileEntryHeader);                       /*  从文件结构表中删除          */
        if (pfdentry->FDENTRY_pcRealName &&
            (pfdentry->FDENTRY_pcRealName != pfdentry->FDENTRY_pcName)) {
            __SHEAP_FREE(pfdentry->FDENTRY_pcRealName);
        }
        __SHEAP_FREE(pfdentry);                                         /*  释放文件结构内存            */
    }
    _IosUnlock();                                                       /*  退出 IO 临界区              */
}
/*********************************************************************************************************
** 函数名称: _IosFileSet
** 功能描述: 设置一个 fd_entry 结构
** 输　入  : pfdentry          fd_entry
**           pdevhdrHdr        设备头
**           lValue            文件底层信息
**           iFlag             打开标志
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _IosFileSet (PLW_FD_ENTRY   pfdentry,
                   PLW_DEV_HDR    pdevhdrHdr,
                   LONG           lValue,
                   INT            iFlag)
{
    pfdentry->FDENTRY_pdevhdrHdr = pdevhdrHdr;
    pfdentry->FDENTRY_lValue     = lValue;
    pfdentry->FDENTRY_iFlag      = iFlag;
    
    iosDrvGetType(pdevhdrHdr->DEVHDR_usDrvNum, 
                  &pfdentry->FDENTRY_iType);                            /*  获得驱动程序的类型          */
    
    if (lValue != PX_ERROR) {                                           /*  正常打开                    */
        pfdentry->FDENTRY_iAbnormity = 0;                               /*  回归正常文件, 允许用户操作  */
    }
}
/*********************************************************************************************************
** 函数名称: _IosFileRealName
** 功能描述: 设置一个 fd_entry 结构的真实文件名 (必须在 _IosFileSet 之后被调用)
** 输　入  : pfdentry          fd_entry
**           pcRealName        真实文件名
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  _IosFileRealName (PLW_FD_ENTRY   pfdentry, CPCHAR  pcRealName)
{
    size_t  stNameLen;
    size_t  stDevNameLen;
    
    stNameLen    = lib_strlen(pcRealName);
    stDevNameLen = lib_strlen(pfdentry->FDENTRY_pdevhdrHdr->DEVHDR_pcName);
    if (stNameLen && (stDevNameLen == 1)) {                             /*  根目录设备 "/" 且存在后缀   */
        stDevNameLen = 0;                                               /*  只拷贝文件名                */
    }
    
    pfdentry->FDENTRY_pcRealName = (PCHAR)__SHEAP_ALLOC(stNameLen + stDevNameLen + 1);
    if (pfdentry->FDENTRY_pcRealName == LW_NULL) {
        pfdentry->FDENTRY_pcRealName = pfdentry->FDENTRY_pcName;
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    if (stDevNameLen) {
        lib_strcpy(pfdentry->FDENTRY_pcRealName, pfdentry->FDENTRY_pdevhdrHdr->DEVHDR_pcName);
    }
    if (stNameLen) {
        lib_strcpy(&pfdentry->FDENTRY_pcRealName[stDevNameLen], pcRealName);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _IosFileLock
** 功能描述: 设置一个 fd_entry 结构对应的 fd node 锁定, 只对 NEW_1 型驱动有效.
** 输　入  : pfdentry          fd_entry
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  _IosFileLock (PLW_FD_ENTRY   pfdentry)
{
    PLW_FD_NODE  pfdnode;
    
    if (pfdentry->FDENTRY_iType == LW_DRV_TYPE_NEW_1) {
        pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
        pfdnode->FDNODE_ulLock = 1;
        return  (ERROR_NONE);
    }
    
    _ErrorHandle(ENOTSUP);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: _IosFileClose
** 功能描述: 使一个 fd_entry 调用驱动 close 操作
** 输　入  : pfdentry          fd_entry
** 输　出  : 驱动返回值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  _IosFileClose (PLW_FD_ENTRY   pfdentry)
{
    REGISTER INT        iErrCode  = ERROR_NONE;
    REGISTER FUNCPTR    pfuncDrvClose;

    if (pfdentry->FDENTRY_iAbnormity == 0) {
        if ((pfdentry->FDENTRY_lValue != PX_ERROR)         &&
            (pfdentry->FDENTRY_lValue != FOLLOW_LINK_FILE) &&
            (pfdentry->FDENTRY_lValue != FOLLOW_LINK_TAIL)) {           /*  必须含有驱动信息            */

            pfuncDrvClose = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevClose;
            if (pfuncDrvClose) {
                PVOID       pvArg = __GET_DRV_ARG(pfdentry);            /*  根据驱动程序版本类型选择参数*/
                
                __KERNEL_SPACE_ENTER();
                iErrCode = pfuncDrvClose(pvArg);
                __KERNEL_SPACE_EXIT();
            
            } else {
                iErrCode = ERROR_NONE;
            }
        }
        /*
         *  这里必须设置为异常状态, 否则 unlink() 可能造成设备驱动调用 API_IosDevFileAbnormal()
         *  可能造成错误.
         */
        pfdentry->FDENTRY_iAbnormity = 1;                               /*  设置为异常状态              */
    }
    
    return  (iErrCode);
}
/*********************************************************************************************************
** 函数名称: _IosFileIoctl
** 功能描述: 使一个 fd_entry 调用驱动 close 操作
** 输　入  : pfdentry          fd_entry
**           iCmd              命令
**           lArg              命令参数
** 输　出  : 驱动返回值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  _IosFileIoctl (PLW_FD_ENTRY   pfdentry, INT  iCmd, LONG  lArg)
{
    REGISTER FUNCPTR       pfuncDrvIoctl;
    REGISTER INT           iErrCode;
    
    pfuncDrvIoctl = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevIoctl;
    if (pfuncDrvIoctl == LW_NULL) {
        if (iCmd == FIONREAD) {
            *(INT *)lArg = 0;
            return  (ERROR_NONE);
        
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown request.\r\n");
            _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
            return  (PX_ERROR);
        }
        
    } else if ((iCmd == FIOGETLK) ||
               (iCmd == FIOSETLK) ||
               (iCmd == FIOSETLKW)) {                                   /*  文件锁                      */
        return  (_FdLockfIoctl(pfdentry, iCmd, (struct flock *)lArg));
    
    } else {
        PVOID       pvArg = __GET_DRV_ARG(pfdentry);                    /*  根据驱动程序版本类型选择参数*/
        
        __KERNEL_SPACE_ENTER();
        iErrCode = pfuncDrvIoctl(pvArg, iCmd, lArg);
        __KERNEL_SPACE_EXIT();
        
        if ((iErrCode != ERROR_NONE) && 
            ((iErrCode == ENOSYS) || (errno == ENOSYS))) {
            REGISTER FUNCPTR   pfuncDrvRoutine;
            
            if (iCmd == FIOSELECT ||
                iCmd == FIOUNSELECT) {
                pfuncDrvRoutine = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevSelect;
                if (pfuncDrvRoutine) {
                    __KERNEL_SPACE_ENTER();
                    iErrCode = pfuncDrvRoutine(pvArg, iCmd, lArg);
                    __KERNEL_SPACE_EXIT();
                }
            }
        }
        return  (iErrCode);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosFdValue
** 功能描述: 确认一个打开的文件描述符有效性，并返回一个设备专有值
** 输　入  : iFd                           文件描述符
** 输　出  : 设备专有值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
LONG  API_IosFdValue (INT  iFd)
{
    REGISTER PLW_FD_ENTRY   pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    if (pfdentry) {                                                     /*  文件有效                    */
        return  (pfdentry->FDENTRY_lValue);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);                /*  文件描述符出错              */
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosFdFree
** 功能描述: 释放一个设备文件描述符
** 输　入  : 
**           iFd                           文件描述符
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_IosFdFree (INT  iFd)
{
             INT            i, iRef;
    REGISTER PLW_FD_ENTRY   pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_TRUE);                               /*  可以获取异常文件            */
    
    if (pfdentry != LW_NULL) {
        _IosLock();                                                     /*  进入 IO 临界区              */
        iRef = _IosFileRefGet(iFd);
        for (i = 0; i < iRef; i++) {
            _IosFileRefDec(iFd);                                        /*  将文件标号设为无效          */
        }
        __LW_FD_DELETE_HOOK(iFd, __PROC_GET_PID_CUR());
        _IosUnlock();                                                   /*  退出 IO 临界区              */
        _IosFileDelete(pfdentry);
        
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosFdSet
** 功能描述: 记录一个设备文件描述符
** 输　入  : iFd                           文件描述符
**           pdevhdrHdr                    设备头
**           lValue                        设备相关的值
**           iFlag                         文件打开标志
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosFdSet (INT            iFd,
                   PLW_DEV_HDR    pdevhdrHdr,
                   LONG           lValue,
                   INT            iFlag)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_TRUE);                               /*  可以获取异常文件            */
    if (pfdentry == LW_NULL) {
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    _IosFileSet(pfdentry, pdevhdrHdr, lValue, iFlag);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_IosFdRealName
** 功能描述: 设置一个 fd_entry 结构的真实文件名 (必须在 _IosFileSet 之后被调用)
** 输　入  : pfdentry          fd_entry
**           pcRealName        真实文件名
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosFdRealName (INT  iFd, CPCHAR  pcRealName)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_TRUE);                               /*  可以获取异常文件            */
    if (pfdentry == LW_NULL) {
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    return  (_IosFileRealName(pfdentry, pcRealName));
}
/*********************************************************************************************************
** 函数名称: API_IosFdLock
** 功能描述: 锁定文件, 不允许写. 不允许被删除 (文件关闭后节点自动解锁)
** 输　入  : iFd                           文件描述符
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosFdLock (INT  iFd)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry == LW_NULL) {
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    return  (_IosFileLock(pfdentry));
}
/*********************************************************************************************************
** 函数名称: API_IosFdNew
** 功能描述: 申请并初始化一个新的文件描述符 (为异常文件, 直到 FdSet lValue != PX_ERROR 为止)
** 输　入  : 
**           pdevhdrHdr                    设备头
**           pcName                        设备名 (这是忽略设备名后的文件名)
**           lValue                        设备相关的值
**           iFlag                         文件打开标志
** 输　出  : 文件描述符
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosFdNew (PLW_DEV_HDR    pdevhdrHdr,
                   CPCHAR         pcName,
                   LONG           lValue,
                   INT            iFlag)
{
    REGISTER INT             iFd;
    REGISTER PLW_FD_ENTRY    pfdentry;
    REGISTER INT             iError;
             
    pfdentry = _IosFileNew(pdevhdrHdr, pcName);                         /*  创建一个 fd_entry 结构      */
    if (pfdentry == LW_NULL) {
        return  (PX_ERROR);
    }
    
    _IosLock();                                                         /*  进入 IO 临界区              */
    iFd = _IosFileDup(pfdentry, 0);                                     /*  分配一个文件描述符          */
    if (iFd < 0) {
        _IosUnlock();                                                   /*  退出 IO 临界区              */
        _IosFileDelete(pfdentry);                                       /*  删除 fd_entry               */
        return  (PX_ERROR);
    }
    __LW_FD_CREATE_HOOK(iFd, __PROC_GET_PID_CUR());
    _IosUnlock();                                                       /*  退出 IO 临界区              */

    iError = API_IosFdSet(iFd, pdevhdrHdr, lValue, iFlag);              /*  设置文件描述块              */
    if (iError != ERROR_NONE) {
        _IosLock();                                                     /*  进入 IO 临界区              */
        _IosFileRefDec(iFd);                                            /*  释放文件描述符              */
        __LW_FD_DELETE_HOOK(iFd, __PROC_GET_PID_CUR());
        _IosUnlock();                                                   /*  退出 IO 临界区              */
        _IosFileDelete(pfdentry);                                       /*  删除 fd_entry               */
        return  (PX_ERROR);
    }
    
	return  (iFd);
}
/*********************************************************************************************************
** 函数名称: API_IosFdUnlink
** 功能描述: 对没有关闭的文件描述符执行 unlink 操作.
** 输　入  : pdevhdrHdr                    设备头
**           pcName                        文件名
** 输　出  : < 0 表示直接可以删除
**           0 表示已经请求, 最后一次关闭后将执行删除操作
**           1 文件被锁定, 不允许删除.
** 全局变量: 
** 调用模块: 
** 注  意  : 这里的遍历使用 IO 锁, 中段没有打开此锁, 所以不用加 file list 锁.
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosFdUnlink (PLW_DEV_HDR  pdevhdrHdr, CPCHAR  pcName)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
    REGISTER PLW_LIST_LINE   plineFdEntry;
             
             size_t          stDevNameLen;
             CHAR            cUnlinkName[MAX_FILENAME_LENGTH];
             
    stDevNameLen = lib_strlen(pdevhdrHdr->DEVHDR_pcName);
    if (stDevNameLen > 1) {
        lib_strcpy(cUnlinkName, pdevhdrHdr->DEVHDR_pcName);
        lib_strcpy(&cUnlinkName[stDevNameLen], pcName);
    
    } else {                                                            /*  如果是根设备 "/" 则不拷贝   */
        lib_strcpy(cUnlinkName, pcName);
    }

    _IosLock();                                                         /*  进入 IO 临界区              */
    for (plineFdEntry  = _S_plineFileEntryHeader;
         plineFdEntry != LW_NULL;
         plineFdEntry  = _list_line_get_next(plineFdEntry)) {           /*  删除使用该驱动文件          */
        
        pfdentry = _LIST_ENTRY(plineFdEntry, LW_FD_ENTRY, FDENTRY_lineManage);
        if ((pfdentry->FDENTRY_iAbnormity == 0) && 
            (pfdentry->FDENTRY_pdevhdrHdr == pdevhdrHdr)) {             /*  文件正常, 且为同一设备      */
            
            if (pfdentry->FDENTRY_pcRealName &&
                (lib_strcmp(pfdentry->FDENTRY_pcRealName, cUnlinkName) == 0)) {
                
                if (pfdentry->FDENTRY_iType == LW_DRV_TYPE_NEW_1) {
                    PLW_FD_NODE  pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
                    if (pfdnode->FDNODE_ulLock) {
                        _IosUnlock();                                   /*  退出 IO 临界区              */
                        return  (1);                                    /*  文件被锁定, 不能删除        */
                    
                    } else {
                        pfdnode->FDNODE_bRemove = LW_TRUE;              /*  最后一次关闭文件需要删除    */
                        _IosUnlock();                                   /*  退出 IO 临界区              */
                        return  (ERROR_NONE);                           /*  fdnode 只一个, 可以直接退出 */
                    }
                
                } else {
                    _IosUnlock();                                       /*  退出 IO 临界区              */
                    return  (1);                                        /*  文件被打开, 不能删除        */
                }
            }
        }
    }
    _IosUnlock();                                                       /*  退出 IO 临界区              */
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_IosFdDevFind
** 功能描述: 校验一个打开的文件描述符是否有效，并返回指向相关设备的设备头
** 输　入  : iFd                           文件描述符
** 输　出  : 设备头指针
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PLW_DEV_HDR  API_IosFdDevFind (INT  iFd)
{
    REGISTER PLW_FD_ENTRY   pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    if (pfdentry) {                                                     /*  文件有效                    */
        return  (pfdentry->FDENTRY_pdevhdrHdr);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);                /*  文件描述符出错              */
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosFdSetCloExec
** 功能描述: 设置文件描述符的 FD_CLOEXEC
** 输　入  : 
**           iFd                           文件描述符
**           iCloExec                      FD_CLOEXEC 状态
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosFdSetCloExec (INT  iFd, INT  iCloExec)
{
    REGISTER PLW_FD_DESC    pfddesc;

    pfddesc = _IosFileDescGet(iFd, LW_TRUE);
    if (pfddesc) {
        if (iCloExec & FD_CLOEXEC) {
            pfddesc->FDDESC_bCloExec = LW_TRUE;
        } else {
            pfddesc->FDDESC_bCloExec = LW_FALSE;
        }
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosFdGetCloExec
** 功能描述: 设置文件描述符的 FD_CLOEXEC
** 输　入  : 
**           iFd                           文件描述符
**           piCloExec                     是否含有 FD_CLOEXEC 标志
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosFdGetCloExec (INT  iFd, INT  *piCloExec)
{
    REGISTER PLW_FD_DESC    pfddesc;
    
    if (!piCloExec) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pfddesc = _IosFileDescGet(iFd, LW_TRUE);
    if (pfddesc) {
        *piCloExec = (INT)pfddesc->FDDESC_bCloExec;
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosFdGetType
** 功能描述: 获得文件描述符的类型
** 输　入  : 
**           iFd                           文件描述符
**           piType                        类型返回
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosFdGetType (INT  iFd, INT  *piType)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry) {
        if (piType) {
            *piType = pfdentry->FDENTRY_iType;
        }
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosFdGetFlag
** 功能描述: 获得文件描述符的 flag
** 输　入  : 
**           iFd                           文件描述符
**           piFlag                        文件 flag  O_RDONLY  O_WRONLY  O_RDWR  O_CREAT ...
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosFdGetFlag (INT  iFd, INT  *piFlag)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry) {
        if (piFlag) {
            *piFlag = pfdentry->FDENTRY_iFlag;
        }
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosFdGetName
** 功能描述: 获得文件描述符内保存的文件名
** 输　入  : 
**           iFd                           文件描述符
**           pcName                        文件名缓冲
**           stSize                        缓冲区大小
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosFdGetName (INT  iFd, PCHAR  pcName, size_t  stSize)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry && pfdentry->FDENTRY_pcName) {
        if (pcName && (stSize > 0)) {
            lib_strlcpy(pcName, pfdentry->FDENTRY_pcName, (INT)stSize); /*  拷贝文件名                  */
        }
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);                /*  无名时, 也为此 errno        */
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosFdGetRealName
** 功能描述: 获得文件描述符内保存的完整文件名
** 输　入  : 
**           iFd                           文件描述符
**           pcName                        文件名缓冲
**           stSize                        缓冲区大小
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosFdGetRealName (INT  iFd, PCHAR  pcName, size_t  stSize)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry && pfdentry->FDENTRY_pcRealName) {
        if (pcName && (stSize > 0)) {
            lib_strlcpy(pcName, pfdentry->FDENTRY_pcRealName, (INT)stSize);
        }
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);                /*  无名时, 也为此 errno        */
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosFdRefInc
** 功能描述: 文件描述符引用次数 ++
** 输　入  : iFd                           文件描述符
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosFdRefInc (INT  iFd)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    _IosLock();                                                         /*  进入 IO 临界区              */
    if (pfdentry->FDENTRY_ulCounter == 0) {                             /*  文件正在被关闭的过程中      */
        _IosUnlock();                                                   /*  退出 IO 临界区              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    if (_IosFileRefInc(iFd) < 0) {
        _IosUnlock();                                                   /*  退出 IO 临界区              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    _IosUnlock();                                                       /*  退出 IO 临界区              */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_IosFdRefDec
** 功能描述: 文件描述符引用次数 -- (引用次数为 0 时将关闭文件)
** 输　入  : iFd                           文件描述符
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : 当前依然存在 dup 文件 close 时的重入性问题, 由应用程序保证.
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosFdRefDec (INT  iFd)
{
    REGISTER BOOL            bCallFunc = LW_TRUE;
    REGISTER PLW_FD_ENTRY    pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry == LW_NULL) {
        pfdentry =  _IosFileGet(iFd, LW_TRUE);                          /*  忽略异常文件                */
        if (pfdentry == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
            _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
            return  (PX_ERROR);
        }
        bCallFunc = LW_FALSE;                                           /*  异常文件, 不需要调用驱动    */
    }
    
    _IosLock();                                                         /*  进入 IO 临界区              */
    if (_IosFileRefDec(iFd) == 0) {
        __LW_FD_DELETE_HOOK(iFd, __PROC_GET_PID_CUR());
        
        MONITOR_EVT_INT1(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_CLOSE, iFd, LW_NULL);
        
        if (pfdentry->FDENTRY_ulCounter == 0) {                         /*  没有描述符引用此 fd_entry   */
            _IosUnlock();                                               /*  退出 IO 临界区              */
            
            _FdLockfClearFdEntry(pfdentry, __PROC_GET_PID_CUR());       /*  回收记录锁                  */
            if (bCallFunc) {                                            /*  正常文件需要调用驱动        */
                _IosFileClose(pfdentry);
            }
            _IosFileDelete(pfdentry);
        
        } else {
            _IosUnlock();                                               /*  退出 IO 临界区              */
            
            _FdLockfClearFdEntry(pfdentry, __PROC_GET_PID_CUR());       /*  回收记录锁                  */
        }
    
    } else {
        _IosUnlock();                                                   /*  退出 IO 临界区              */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_IosEntryReclaim
** 功能描述: 文件描述符引用次数 -- (引用次数为 0 时将关闭文件, 此函数供进程回收器使用, 回收进程打开的文件)
** 输　入  : pfdentry                      fd_entry
**           ulRefDec                      需要减少的引用数量
**           pid                           执行回收的进程号
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : 由于此函数在进程消亡时才被调用, 这里不可能产生继续使用文件的情况, 所以 pfdentry 引用最终会被
             减到 0, 所以只在 pfdentry 减到 0 时再回记录锁即可.
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosFdEntryReclaim (PLW_FD_ENTRY  pfdentry, ULONG  ulRefDec, pid_t  pid)
{
#if LW_CFG_NET_EN > 0
    extern VOID  __socketReset(PLW_FD_ENTRY  pfdentry);
#endif                                                                  /*  LW_CFG_NET_EN > 0           */

    REGISTER INT        iErrCode  = ERROR_NONE;
    REGISTER BOOL       bCallFunc = LW_TRUE;

    _IosLock();                                                         /*  进入 IO 临界区              */
    if (pfdentry->FDENTRY_iAbnormity) {
        bCallFunc = LW_FALSE;
    }
    if (pfdentry->FDENTRY_ulCounter >  ulRefDec) {
        pfdentry->FDENTRY_ulCounter -= ulRefDec;                        /*  只需要减去相应的引用个数即可*/
        _IosUnlock();                                                   /*  退出 IO 临界区              */
        
    } else {                                                            /*  需要回收 pfdentry           */
        pfdentry->FDENTRY_ulCounter = 0;                                /*  不允许再次进行操作          */
        _IosUnlock();                                                   /*  退出 IO 临界区              */
        
        _FdLockfClearFdEntry(pfdentry, pid);                            /*  回收指定进程创建的记录锁    */
        if (bCallFunc) {                                                /*  正常文件需要调用驱动        */
#if LW_CFG_NET_EN > 0
            if (pfdentry->FDENTRY_iType == LW_DRV_TYPE_SOCKET) {
                __socketReset(pfdentry);                                /*  设置 SO_LINGER              */
            }
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
            _IosFileClose(pfdentry);
        }
        _IosFileDelete(pfdentry);
    }
    
    return  (iErrCode);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
