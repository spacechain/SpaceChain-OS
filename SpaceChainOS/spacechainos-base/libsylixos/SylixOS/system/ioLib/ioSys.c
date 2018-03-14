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
** 文   件   名: ioSys.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 13 日
**
** 描        述: 系统 IO 功能函数库，驱动程序部分

** BUG
2007.06.03  API_IosDrvRemove() 函数 pfdentry 没有初始化。
2007.09.12  加入裁剪支持。
2007.09.24  加入 _DebugHandle() 功能。
2007.09.25  加入驱动程序许可证信息管理.
2007.11.13  使用链表库对链表操作进行完全封装.
2007.11.18  整理注释.
2007.11.21  加入 API_IosDevFileClose() 函数, 用于自定义设备删除时使用.
2008.03.23  加入 API_IosDrvInstallEx() 函数, 使用 file_operations 初始化设备函数.
2008.03.23  API_IosIoctl() 加入对 lseek 和 select 针对 file_operations 的特殊处理.
2008.03.23  修改了代码的格式.
2008.05.31  API_IosIoctl() 第三个参数升级为 long 型,支持 64 位指针.
2008.06.27  修改 API_IosDevFind() , 带有名字尾巴的设备, 尾部路径必须以 "/"或者 "\"起始.
2008.09.28  API_IosIoctl() 更加完善.
2008.10.06  API_IosDevFind() 不打印 ERROR 信息.
2008.10.18  API_IosDelete() 如果没有调用驱动函数, 返回是 PX_ERROR,
2008.12.11  加入 API_IosLseek() 函数, lseek 系统调用首先使用此函数.
2009.02.13  修改代码以适应新的 I/O 系统结构组织.
2009.02.19  修正了 iosClose 的一处问题, 造成无法释放文件描述符.
2009.02.19  增加了 close 对 O_TEMP 文件的处理.
2009.03.22  增加 API_IosDevAdd 的安全性.
2009.04.22  增加 FDENTRY_iAbnormity 当设备关闭后, 以前打开的文件进入异常模式, 取代以前的强行关闭方法.
            增加 API_IosDevFileAbnormal() 函数用于设备强制移除.
            API_IosClose() 关闭异常文件时直接归还 desc 即可.
2009.07.09  API_IosLseek() 支持 64 bit 文件.
2009.08.15  API_IosCreate() 加入 flag 参数.
2009.08.27  API_IosDevAdd() 允许安装 "/" 根设备 (root fs).
2009.08.31  API_IosDevAdd() 加入设备功耗管理的相关操作.
2009.09.05  API_IosIoctl() 文件处于异常状态不报错误, 直接退出.
2009.10.02  设备打开次数计数器使用 atomic_t 类型.
2009.10.12  修正了 API_IosClose() 相关的锁保护问题, 当前依然存在 dup 文件 close 时的重入性问题.
2009.10.22  修正 read write 参数及返回值类型.
2009.12.04  修正一些注释.
2009.12.13  可以支持 VxWorks 式的单级设备目录管理和 SylixOS 新的分级目录管理.
2009.12.15  修正了一些函数的 errno.
2009.12.16  加入对 symlink 的初始化.
2010.09.09  加入对 fo_fstat 的初始化.
            加入 API_IosFstat() 函数.
            加入 API_IosDevAddEx() 函数.
2010.10.23  加入 API_IosFdGetName() 函数.
2011.03.06  修正 gcc 4.5.1 相关 warning.
2011.07.28  加入 API_IosMmap() 函数.
2011.08.07  加入 API_IosLstat() 函数.
2012.03.12  API_IosDevAddEx() 支持使用相对路径.
2012.08.16  加入 API_IosPRead 与 API_IosPWrite 操作. 使用驱动 readex 与 writeex 接口实现.
2012.09.01  API_IosDevAddEx() 设备名重复不需要打印信息
            加入 API_IosDevMatchFull() 匹配完整的设备名.
2012.10.19  加入对文件描述符引用计数的功能.
2012.10.24  PLW_FD_ENTRY 内部保存文件的绝对路径名.
            API_IosFdNew() 一次性设置好文件名, API_IosFdSet() 不再设置文件名.
2012.11.21  支持文件没有关闭时的 unlink 操作.
2012.12.21  去掉 API_IosDevFileClose() 必须使用 API_IosDevFileAbnormal().
            将所有 Fd 操作函数移动至 IoFile.c 中
            以前需要遍历文件描述符表的函数, 现在需要使用遍历 fd_entry 链表的方式, 需要说明的是遍历此链表
            会锁定 IO 空间.
2012.12.25  进入设备驱动时, 需要先进入内核空间. 这样驱动程序的内部文件就在内核中所有的进程都可以访问, 例如
            nfs 文件系统的 socket 就在内核中. 其它进程不用继承这个 socket 就可以访问 nfs 卷.
2012.12.31  如果驱动程序中 pread 和 pwrite 不存在则证明驱动不关心文件指针, 则使用 read write 替代.
2013.01.02  加入 API_IosDrvInstallEx2() 用于安装 SylixOS 系统新版本的驱动程序.
2013.01.03  所有对驱动 close 操作必须通过 _IosFileClose 接口.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
  是否需要新的分级目录管理
*********************************************************************************************************/
#if LW_CFG_PATH_VXWORKS == 0
#include "../SylixOS/fs/rootFs/rootFs.h"
#include "../SylixOS/fs/rootFs/rootFsLib.h"
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
/*********************************************************************************************************
  缩写宏定义
*********************************************************************************************************/
#define __LW_FD_MAINDRV     (pfdentry->FDENTRY_pdevhdrHdr->DEVHDR_usDrvNum)
#define __LW_DEV_MAINDRV    (pdevhdrHdr->DEVHDR_usDrvNum)
/*********************************************************************************************************
** 函数名称: API_IosDrvInstall
** 功能描述: 注册设备驱动程序
** 输　入  : pfuncCreate             驱动程序中的建立函数 (如果非符号链接, 则不可更改 name 参数内容)
**           pfuncDelete             驱动程序中的删除函数
**           pfuncOpen               驱动程序中的打开函数 (如果非符号链接, 则不可更改 name 参数内容)
**           pfuncClose              驱动程序中的关闭函数
**           pfuncRead               驱动程序中的读函数
**           pfuncWrite              驱动程序中的写函数
**           pfuncIoctl              驱动程序中的IO控制函数
** 输　出  : 驱动程序索引号
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosDrvInstall (LONGFUNCPTR    pfuncCreate,
                        FUNCPTR        pfuncDelete,
                        LONGFUNCPTR    pfuncOpen,
                        FUNCPTR        pfuncClose,
                        SSIZETFUNCPTR  pfuncRead,
                        SSIZETFUNCPTR  pfuncWrite,
                        FUNCPTR        pfuncIoctl)
{
    REGISTER PLW_DEV_ENTRY    pdeventry = LW_NULL;
    REGISTER INT              iDrvNum;
    
    _IosLock();                                                         /*  进入 IO 临界区              */
    for (iDrvNum = 0; iDrvNum < LW_CFG_MAX_DRIVERS; iDrvNum++) {        /*  搜索驱动程序表              */
        if (_S_deventryTbl[iDrvNum].DEVENTRY_bInUse == LW_FALSE) {
            pdeventry = &_S_deventryTbl[iDrvNum];                       /*  找到空闲位置                */
            break;
        }
    }
    if (pdeventry == LW_NULL) {                                         /*  没有空闲位置                */
        _IosUnlock();                                                   /*  退出 IO 临界区              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, 
                     "major device is full (driver table full).\r\n");
        _ErrorHandle(ERROR_IOS_DRIVER_GLUT);
        return  (PX_ERROR);
    }
    
    pdeventry->DEVENTRY_bInUse = LW_TRUE;                               /*  填写驱动程序表              */
    pdeventry->DEVENTRY_iType  = LW_DRV_TYPE_ORIG;                      /*  默认为 VxWorks 兼容驱动     */
    
    pdeventry->DEVENTRY_pfuncDevCreate = pfuncCreate;
    pdeventry->DEVENTRY_pfuncDevDelete = pfuncDelete;
    pdeventry->DEVENTRY_pfuncDevOpen   = pfuncOpen;
    pdeventry->DEVENTRY_pfuncDevClose  = pfuncClose;
    pdeventry->DEVENTRY_pfuncDevRead   = pfuncRead;
    pdeventry->DEVENTRY_pfuncDevWrite  = pfuncWrite;
    pdeventry->DEVENTRY_pfuncDevIoctl  = pfuncIoctl;
    
    pdeventry->DEVENTRY_pfuncDevReadEx  = LW_NULL;
    pdeventry->DEVENTRY_pfuncDevWriteEx = LW_NULL;
    pdeventry->DEVENTRY_pfuncDevSelect  = LW_NULL;
    pdeventry->DEVENTRY_pfuncDevLseek   = LW_NULL;
    pdeventry->DEVENTRY_pfuncDevFstat   = LW_NULL;
    pdeventry->DEVENTRY_pfuncDevLstat   = LW_NULL;
    
    pdeventry->DEVENTRY_pfuncDevSymlink  = LW_NULL;
    pdeventry->DEVENTRY_pfuncDevReadlink = LW_NULL;
    
    pdeventry->DEVENTRY_pfuncDevMmap  = LW_NULL;
    pdeventry->DEVENTRY_pfuncDevUnmap = LW_NULL;
    
    pdeventry->DEVENTRY_drvlicLicense.DRVLIC_pcLicense     = LW_NULL;   /*  清空许可证信息              */
    pdeventry->DEVENTRY_drvlicLicense.DRVLIC_pcAuthor      = LW_NULL;
    pdeventry->DEVENTRY_drvlicLicense.DRVLIC_pcDescription = LW_NULL;
    
    _IosUnlock();                                                       /*  退出 IO 临界区              */
    
    return  (iDrvNum);
}
/*********************************************************************************************************
** 函数名称: API_IosDrvInstallEx
** 功能描述: 注册设备驱动程序
** 输　入  : pFileOp                     文件操作块
**           
** 输　出  : 驱动程序索引号
** 全局变量: 
** 调用模块: 
** 注  意  : pfileop 中的 open 与 create 操作 (如果非符号链接, 则不可更改 name 参数内容)
             仅当 LW_CFG_PATH_VXWORKS == 0 才支持部分符号链接功能
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosDrvInstallEx (struct file_operations  *pfileop)
{
    REGISTER PLW_DEV_ENTRY    pdeventry = LW_NULL;
    REGISTER INT              iDrvNum;
    
    if (!pfileop) {                                                     /*  参数错误                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file_operations invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_FILE_OPERATIONS_NULL);
        return  (PX_ERROR);
    }
    
    _IosLock();                                                         /*  进入 IO 临界区              */
    for (iDrvNum = 1; iDrvNum < LW_CFG_MAX_DRIVERS; iDrvNum++) {        /*  搜索驱动程序表              */
        if (_S_deventryTbl[iDrvNum].DEVENTRY_bInUse == LW_FALSE) {
            pdeventry = &_S_deventryTbl[iDrvNum];                       /*  找到空闲位置                */
            break;
        }
    }
    if (pdeventry == LW_NULL) {                                         /*  没有空闲位置                */
        _IosUnlock();                                                   /*  退出 IO 临界区              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "major device is full (driver table full).\r\n");
        _ErrorHandle(ERROR_IOS_DRIVER_GLUT);
        return  (PX_ERROR);
    }
    
    pdeventry->DEVENTRY_bInUse = LW_TRUE;                               /*  填写驱动程序表              */
    pdeventry->DEVENTRY_iType  = LW_DRV_TYPE_ORIG;                      /*  默认为 VxWorks 兼容驱动     */
     
    pdeventry->DEVENTRY_pfuncDevCreate  = pfileop->fo_create;
    pdeventry->DEVENTRY_pfuncDevDelete  = pfileop->fo_release;
    pdeventry->DEVENTRY_pfuncDevOpen    = pfileop->fo_open;
    pdeventry->DEVENTRY_pfuncDevClose   = pfileop->fo_close;
    pdeventry->DEVENTRY_pfuncDevRead    = pfileop->fo_read;
    pdeventry->DEVENTRY_pfuncDevReadEx  = pfileop->fo_read_ex;
    pdeventry->DEVENTRY_pfuncDevWrite   = pfileop->fo_write;
    pdeventry->DEVENTRY_pfuncDevWriteEx = pfileop->fo_write_ex;
    pdeventry->DEVENTRY_pfuncDevIoctl   = pfileop->fo_ioctl;
    pdeventry->DEVENTRY_pfuncDevSelect  = pfileop->fo_select;
    pdeventry->DEVENTRY_pfuncDevLseek   = pfileop->fo_lseek;
    pdeventry->DEVENTRY_pfuncDevFstat   = pfileop->fo_fstat;
    pdeventry->DEVENTRY_pfuncDevLstat   = pfileop->fo_lstat;
    
    pdeventry->DEVENTRY_pfuncDevSymlink  = pfileop->fo_symlink;
    pdeventry->DEVENTRY_pfuncDevReadlink = pfileop->fo_readlink;
    
    pdeventry->DEVENTRY_pfuncDevMmap  = pfileop->fo_mmap;
    pdeventry->DEVENTRY_pfuncDevUnmap = pfileop->fo_unmap;
    
    pdeventry->DEVENTRY_drvlicLicense.DRVLIC_pcLicense     = LW_NULL;   /*  清空许可证信息              */
    pdeventry->DEVENTRY_drvlicLicense.DRVLIC_pcAuthor      = LW_NULL;
    pdeventry->DEVENTRY_drvlicLicense.DRVLIC_pcDescription = LW_NULL;
    
    _IosUnlock();                                                       /*  退出 IO 临界区              */
    
    return  (iDrvNum);
}
/*********************************************************************************************************
** 函数名称: API_IosDrvInstallEx2
** 功能描述: 注册设备驱动程序
** 输　入  : pFileOp                     文件操作块
**           iType                       设备驱动类型 
                                         LW_DRV_TYPE_ORIG or LW_DRV_TYPE_NEW_? or LW_DRV_TYPE_SOCKET
** 输　出  : 驱动程序索引号
** 全局变量: 
** 调用模块: 
** 注  意  : pfileop 中的 open 与 create 操作 (如果非符号链接, 则不可更改 name 参数内容)
             仅当 LW_CFG_PATH_VXWORKS == 0 才支持部分符号链接功能
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosDrvInstallEx2 (struct file_operations  *pfileop, INT  iType)
{
    REGISTER PLW_DEV_ENTRY    pdeventry = LW_NULL;
    REGISTER INT              iDrvNum;
    
    if (!pfileop) {                                                     /*  参数错误                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file_operations invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_FILE_OPERATIONS_NULL);
        return  (PX_ERROR);
    }
    
    if ((iType != LW_DRV_TYPE_ORIG)  &&
        (iType != LW_DRV_TYPE_NEW_1) &&
        (iType != LW_DRV_TYPE_SOCKET)) {                                /*  驱动是否符合标准            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver type invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    _IosLock();                                                         /*  进入 IO 临界区              */
    for (iDrvNum = 1; iDrvNum < LW_CFG_MAX_DRIVERS; iDrvNum++) {        /*  搜索驱动程序表              */
        if (_S_deventryTbl[iDrvNum].DEVENTRY_bInUse == LW_FALSE) {
            pdeventry = &_S_deventryTbl[iDrvNum];                       /*  找到空闲位置                */
            break;
        }
    }
    if (pdeventry == LW_NULL) {                                         /*  没有空闲位置                */
        _IosUnlock();                                                   /*  退出 IO 临界区              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "major device is full (driver table full).\r\n");
        _ErrorHandle(ERROR_IOS_DRIVER_GLUT);
        return  (PX_ERROR);
    }
    
    pdeventry->DEVENTRY_bInUse = LW_TRUE;                               /*  填写驱动程序表              */
    pdeventry->DEVENTRY_iType  = iType;
    
    pdeventry->DEVENTRY_pfuncDevCreate  = pfileop->fo_create;
    pdeventry->DEVENTRY_pfuncDevDelete  = pfileop->fo_release;
    pdeventry->DEVENTRY_pfuncDevOpen    = pfileop->fo_open;
    pdeventry->DEVENTRY_pfuncDevClose   = pfileop->fo_close;
    pdeventry->DEVENTRY_pfuncDevRead    = pfileop->fo_read;
    pdeventry->DEVENTRY_pfuncDevReadEx  = pfileop->fo_read_ex;
    pdeventry->DEVENTRY_pfuncDevWrite   = pfileop->fo_write;
    pdeventry->DEVENTRY_pfuncDevWriteEx = pfileop->fo_write_ex;
    pdeventry->DEVENTRY_pfuncDevIoctl   = pfileop->fo_ioctl;
    pdeventry->DEVENTRY_pfuncDevSelect  = pfileop->fo_select;
    pdeventry->DEVENTRY_pfuncDevLseek   = pfileop->fo_lseek;
    pdeventry->DEVENTRY_pfuncDevFstat   = pfileop->fo_fstat;
    pdeventry->DEVENTRY_pfuncDevLstat   = pfileop->fo_lstat;
    
    pdeventry->DEVENTRY_pfuncDevSymlink  = pfileop->fo_symlink;
    pdeventry->DEVENTRY_pfuncDevReadlink = pfileop->fo_readlink;
    
    pdeventry->DEVENTRY_pfuncDevMmap  = pfileop->fo_mmap;
    pdeventry->DEVENTRY_pfuncDevUnmap = pfileop->fo_unmap;
    
    pdeventry->DEVENTRY_drvlicLicense.DRVLIC_pcLicense     = LW_NULL;   /*  清空许可证信息              */
    pdeventry->DEVENTRY_drvlicLicense.DRVLIC_pcAuthor      = LW_NULL;
    pdeventry->DEVENTRY_drvlicLicense.DRVLIC_pcDescription = LW_NULL;
    
    _IosUnlock();                                                       /*  退出 IO 临界区              */
    
    return  (iDrvNum);
}
/*********************************************************************************************************
** 函数名称: API_IosDrvRemove
** 功能描述: 卸载设备驱动程序
** 输　入  : 
**           iDrvNum                      驱动程序索引号
**           bForceClose                  是否强制关闭打开的文件
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_IosDrvRemove (INT  iDrvNum, BOOL  bForceClose)
{
    REGISTER PLW_LIST_LINE  plineDevHdr;
    REGISTER PLW_LIST_LINE  plineFdEntry;
    REGISTER PLW_DEV_HDR    pdevhdr;
    REGISTER PLW_FD_ENTRY   pfdentry;
    REGISTER PLW_DEV_ENTRY  pdeventry = &_S_deventryTbl[iDrvNum];
    
    if ((iDrvNum < 1) || (iDrvNum >= LW_CFG_MAX_DRIVERS)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (ERROR_IO_NO_DRIVER);
    }
    
    _IosLock();                                                         /*  进入 IO 临界区              */
    
    _IosFileListLock();                                                 /*  开始遍历                    */
    
    plineFdEntry = _S_plineFileEntryHeader;
    while (plineFdEntry) {
        pfdentry = _LIST_ENTRY(plineFdEntry, LW_FD_ENTRY, FDENTRY_lineManage);
        plineFdEntry = _list_line_get_next(plineFdEntry);
        
        if (pfdentry->FDENTRY_pdevhdrHdr->DEVHDR_usDrvNum == (UINT16)iDrvNum) {
            if (bForceClose == LW_FALSE) {
                _IosUnlock();                                           /*  退出 IO 临界区              */
                _IosFileListUnlock();                                   /*  结束遍历, 删除请求删除的节点*/
                _DebugHandle(__ERRORMESSAGE_LEVEL, "device file has exist.\r\n");
                _ErrorHandle(ERROR_IO_FILE_EXIST);
                return  (ERROR_IO_FILE_EXIST);
            
            } else {
                _IosUnlock();                                           /*  退出 IO 临界区              */
                _IosFileClose(pfdentry);                                /*  调用驱动程序关闭            */
                _IosLock();                                             /*  进入 IO 临界区              */
            }
        }
    }
    
    for (plineDevHdr  = _S_plineDevHdrHeader;
         plineDevHdr != LW_NULL;
         plineDevHdr  = _list_line_get_next(plineDevHdr)) {             /*  删除使用该驱动的设备        */
         
         pdevhdr = _LIST_ENTRY(plineDevHdr, LW_DEV_HDR, DEVHDR_lineManage);
         if (pdevhdr->DEVHDR_usDrvNum == (UINT16)iDrvNum) {
             __SHEAP_FREE((PVOID)pdevhdr->DEVHDR_pcName);               /*  释放名字空间                */
             _List_Line_Del(plineDevHdr, &_S_plineDevHdrHeader);
         }
    }
    
    lib_bzero(pdeventry, sizeof(LW_DEV_ENTRY));                         /*  清空对应的驱动程序表        */
    
    _IosUnlock();                                                       /*  退出 IO 临界区              */
    
    _IosFileListUnlock();                                               /*  结束遍历, 删除请求删除的节点*/
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_IosDrvGetType
** 功能描述: 获得设备驱动程序类型
** 输　入  : 
**           iDrvNum                      驱动程序索引号
**           piType                       驱动类型
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_IosDrvGetType (INT  iDrvNum, INT  *piType)
{
    REGISTER PLW_DEV_ENTRY  pdeventry = &_S_deventryTbl[iDrvNum];
    
    if (iDrvNum >= LW_CFG_MAX_DRIVERS) {                                /*  允许获取 null 驱动          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (ERROR_IO_NO_DRIVER);
    }
    
    if (piType && pdeventry->DEVENTRY_bInUse) {
        *piType = pdeventry->DEVENTRY_iType;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_IosDevFileAbnormal
** 功能描述: 将所有设备相关的文件设置为异常状态, (用于设备强制移除)
**           这里已经隐式调用了 close 功能, 为了保证系统健壮性, 文件描述符还是存在的, 应用程序必须显示的
**           调用 close() 函数, 才能将文件完全关闭.
** 输　入  : 
**           pdevhdrHdr                   设备头指针
** 输　出  : 文件数量, 错误返回 PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT     API_IosDevFileAbnormal (PLW_DEV_HDR    pdevhdrHdr)
{
#if LW_CFG_NET_EN > 0
    extern VOID  __socketReset(PLW_FD_ENTRY  pfdentry);
#endif                                                                  /*  LW_CFG_NET_EN > 0           */

    REGISTER INT            iCounter = 0;
    REGISTER PLW_LIST_LINE  plineFdEntry;
    REGISTER PLW_FD_ENTRY   pfdentry;
    
    if (!pdevhdrHdr) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device not found");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    
    _IosLock();                                                         /*  进入 IO 临界区              */
    
    _IosFileListLock();                                                 /*  开始遍历                    */
    
    plineFdEntry = _S_plineFileEntryHeader;
    while (plineFdEntry) {
        pfdentry = _LIST_ENTRY(plineFdEntry, LW_FD_ENTRY, FDENTRY_lineManage);
        plineFdEntry = _list_line_get_next(plineFdEntry);
        
        if ((pfdentry->FDENTRY_pdevhdrHdr == pdevhdrHdr) &&
            (pfdentry->FDENTRY_iAbnormity == 0)) {                      /*  被设备相关正常文件          */
            _IosUnlock();                                               /*  退出 IO 临界区              */
            
#if LW_CFG_NET_EN > 0
            if (pfdentry->FDENTRY_iType == LW_DRV_TYPE_SOCKET) {
                __socketReset(pfdentry);                                /*  设置 SO_LINGER              */
            }
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
            _IosFileClose(pfdentry);                                    /*  调用驱动程序关闭            */
            
            _IosLock();                                                 /*  进入 IO 临界区              */
            iCounter++;                                                 /*  异常文件数量 ++             */
        }
    }
    
    _IosUnlock();                                                       /*  退出 IO 临界区              */
    
    _IosFileListUnlock();                                               /*  结束遍历, 删除请求删除的节点*/
    
    return  (iCounter);
}
/*********************************************************************************************************
** 函数名称: API_IosDevAddEx
** 功能描述: 向系统中添加一个设备 (可以设置设备的 mode)
** 输　入  : 
**           pdevhdrHdr                   设备头指针
**           pcDevName                    设备名
**           iDrvNum                      驱动程序索引
**           ucType                       设备 type (与 dirent 中的 d_type 相同)
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_IosDevAddEx (PLW_DEV_HDR    pdevhdrHdr,
                        CPCHAR         pcDevName,
                        INT            iDrvNum,
                        UCHAR          ucType)
{
    REGISTER PLW_DEV_HDR    pdevhdrMatch;
    REGISTER size_t         stNameLen;
    
             CHAR           cNameBuffer[MAX_FILENAME_LENGTH];
             CPCHAR         pcName;
    
    if (pcDevName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name error.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (EFAULT);
    }
    
    _PathGetFull(cNameBuffer, MAX_FILENAME_LENGTH, pcDevName);
    
    pcName = cNameBuffer;                                               /*  使用绝对路径                */
    
    stNameLen = lib_strlen(pcName);                                     /*  设备名长短                  */
    
    pdevhdrMatch = API_IosDevMatch(pcName);                             /*  匹配设备名                  */
    if (pdevhdrMatch != LW_NULL) {                                      /*  出现重名设备                */
        if (lib_strcmp(pdevhdrMatch->DEVHDR_pcName, pcName) == 0) {
            _ErrorHandle(ERROR_IOS_DUPLICATE_DEVICE_NAME);
            return  (ERROR_IOS_DUPLICATE_DEVICE_NAME);
        }
    }
                                                                        /*  开辟设备名空间              */
    pdevhdrHdr->DEVHDR_pcName = (PCHAR)__SHEAP_ALLOC(stNameLen + 1);
    if (pdevhdrHdr->DEVHDR_pcName == LW_NULL) {                         /*  缺少内存                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (ERROR_SYSTEM_LOW_MEMORY);
    }
    
    pdevhdrHdr->DEVHDR_usDrvNum = (UINT16)iDrvNum;
    pdevhdrHdr->DEVHDR_ucType   = ucType;                               /*  设备 d_type                 */
    pdevhdrHdr->DEVHDR_atomicOpenNum.counter = 0;                       /*  没有被打开过                */
    
#if LW_CFG_PATH_VXWORKS == 0                                            /*  是否分级目录管理            */
    if (rootFsMakeDev(pcName, pdevhdrHdr) < ERROR_NONE) {               /*  创建根目录节点              */
        __SHEAP_FREE(pdevhdrHdr->DEVHDR_pcName);                        /*  释放设备名缓冲              */
        return  (API_GetLastError());
    }
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
    
    lib_strcpy(pdevhdrHdr->DEVHDR_pcName, pcName);                      /*  拷贝名字                    */
    
    _IosLock();                                                         /*  进入 IO 临界区              */
                                                                        /*  添加如设备头链表            */
    _List_Line_Add_Ahead(&pdevhdrHdr->DEVHDR_lineManage, &_S_plineDevHdrHeader);        
    
    _IosUnlock();                                                       /*  退出 IO 临界区              */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_IosDevAdd
** 功能描述: 向系统中添加一个设备
** 输　入  : 
**           pdevhdrHdr                   设备头指针
**           pcName                       设备名
**           iDrvNum                      驱动程序索引
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_IosDevAdd (PLW_DEV_HDR    pdevhdrHdr,
                      CPCHAR         pcName,
                      INT            iDrvNum)
{
    return  (API_IosDevAddEx(pdevhdrHdr, pcName, iDrvNum, DT_UNKNOWN));
}
/*********************************************************************************************************
** 函数名称: API_IosDevDelete
** 功能描述: 删除一个设备
** 输　入  : 
**           pdevhdrHdr                   设备头指针
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_IosDevDelete (PLW_DEV_HDR    pdevhdrHdr)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  是否分级目录管理            */
    rootFsRemoveNode(pdevhdrHdr->DEVHDR_pcName);                        /*  从根文件系统删除            */
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */

    _IosLock();                                                         /*  进入 IO 临界区              */
                                                                        /*  从设备头链表中删除          */
    _List_Line_Del(&pdevhdrHdr->DEVHDR_lineManage, &_S_plineDevHdrHeader);
    
    _IosUnlock();                                                       /*  退出 IO 临界区              */
    
    __SHEAP_FREE(pdevhdrHdr->DEVHDR_pcName);                            /*  释放名字空间                */
}
/*********************************************************************************************************
** 函数名称: API_IosDevFind
** 功能描述: 查找一个设备
** 输　入  : 
**           pcName                        设备名
**           ppcNameTail                   设备名尾指针
** 输　出  : 设备头
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PLW_DEV_HDR  API_IosDevFind (CPCHAR  pcName, PCHAR  *ppcNameTail)
{
    REGISTER PLW_DEV_HDR    pdevhdrMatch = API_IosDevMatch(pcName);
    
    if (pdevhdrMatch != LW_NULL) {                                      /*  设备存在                    */
        REGISTER PCHAR  pcTail = (PCHAR)pcName + lib_strlen(pdevhdrMatch->DEVHDR_pcName);
        
#if LW_CFG_PATH_VXWORKS > 0                                             /*  不进行分级目录管理          */
        if (*pcTail) {                                                  /*  存在尾部                    */
            if ((*pcTail != '\\') &&
                (*pcTail != '/')) {                                     /*  尾部字符串错误              */
                goto    __check_defdev;
            }
        }
#endif                                                                  /*  LW_CFG_PATH_VXWORKS > 0     */
        if (ppcNameTail) {
            *ppcNameTail = pcTail;
        }
        
    } else {                                                            /*  设备不存在                  */
#if LW_CFG_PATH_VXWORKS > 0                                             /*  不进行分级目录管理          */
__check_defdev:
#endif                                                                  /*  LW_CFG_PATH_VXWORKS > 0     */
        
        pdevhdrMatch = API_IosDevMatch(_PathGetDef());                  /*  默认设备                    */
        if (ppcNameTail) {
            *ppcNameTail = (PCHAR)pcName;
        }
    }
    
    if (!pdevhdrMatch) {
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
    }
    
    return  (pdevhdrMatch);
}
/*********************************************************************************************************
** 函数名称: API_IosDevMatch
** 功能描述: 设备匹配
** 输　入  : 
**           pcName                       设备名
** 输　出  : 设备头指针
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PLW_DEV_HDR  API_IosDevMatch (CPCHAR  pcName)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  是否分级目录管理            */
    return  (__rootFsDevMatch(pcName));                                 /*  从根文件系统匹配            */

#else
    REGISTER PLW_LIST_LINE  plineDevHdr;
    
    REGISTER PLW_DEV_HDR    pdevhdr;
    REGISTER INT            iLen;
    REGISTER PLW_DEV_HDR    pdevhdrBest = LW_NULL;
    REGISTER INT            iMaxLen = 0;
    
    REGISTER INT            iSameNameSubString;
    
    _IosLock();                                                         /*  进入 IO 临界区              */
    for (plineDevHdr  = _S_plineDevHdrHeader;
         plineDevHdr != LW_NULL;
         plineDevHdr  = _list_line_get_next(plineDevHdr)) {
         
        pdevhdr = _LIST_ENTRY(plineDevHdr, LW_DEV_HDR, DEVHDR_lineManage);
         
        iLen = (INT)lib_strlen(pdevhdr->DEVHDR_pcName);                 /*  获得名字长度                */
        iSameNameSubString = lib_strncmp(pdevhdr->DEVHDR_pcName, pcName, iLen);
         
        if (iSameNameSubString == 0) {                                  /*  记录最为合适的              */
            if (iLen > iMaxLen) {
                pdevhdrBest = pdevhdr;
                iMaxLen     = iLen;
            }
        }
    }
    _IosUnlock();                                                       /*  退出 IO 临界区              */
    
    if (pdevhdrBest && (iMaxLen <= 1)) {                                /*  仅与 root 匹配              */
        if (lib_strcmp(pcName, PX_STR_ROOT)) {                          /*  需要寻找的设备不是根设备    */
            pdevhdrBest = LW_NULL;                                      /*  无法匹配设备                */
        }
    }
    
    return  (pdevhdrBest);
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
}
/*********************************************************************************************************
** 函数名称: API_IosDevMatchFull
** 功能描述: 设备匹配, 设备名必须完全匹配
** 输　入  : 
**           pcName                       设备名
** 输　出  : 设备头指针
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PLW_DEV_HDR  API_IosDevMatchFull (CPCHAR  pcName)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  是否分级目录管理            */
    PLW_DEV_HDR pdevhdr;
    
    _IosLock();                                                         /*  进入 IO 临界区              */
    pdevhdr = __rootFsDevMatch(pcName);                                 /*  从根文件系统匹配            */
    if (pdevhdr) {
        if (lib_strcmp(pdevhdr->DEVHDR_pcName, pcName) == 0) {
            _IosUnlock();
            return  (pdevhdr);
        }
    }
    _IosUnlock();                                                       /*  退出 IO 临界区              */
    
    return  (LW_NULL);

#else
    REGISTER PLW_LIST_LINE  plineDevHdr;
    
    REGISTER PLW_DEV_HDR    pdevhdr;
    REGISTER INT            iLen;
    REGISTER PLW_DEV_HDR    pdevhdrBest = LW_NULL;
    REGISTER INT            iMaxLen = 0;
    
    REGISTER INT            iSameNameSubString;
    
    _IosLock();                                                         /*  进入 IO 临界区              */
    for (plineDevHdr  = _S_plineDevHdrHeader;
         plineDevHdr != LW_NULL;
         plineDevHdr  = _list_line_get_next(plineDevHdr)) {
         
        pdevhdr = _LIST_ENTRY(plineDevHdr, LW_DEV_HDR, DEVHDR_lineManage);
         
        if (lib_strcmp(pdevhdr->DEVHDR_pcName, pcName) == 0) {
            pdevhdrBest = pdevhdr;
            iMaxLen = (INT)lib_strlen(pdevhdr->DEVHDR_pcName);          /*  获得名字长度                */
        }
    }
    _IosUnlock();                                                       /*  退出 IO 临界区              */
    
    if (pdevhdrBest && (iMaxLen <= 1)) {                                /*  仅与 root 匹配              */
        if (lib_strcmp(pcName, PX_STR_ROOT)) {                          /*  需要寻找的设备不是根设备    */
            pdevhdrBest = LW_NULL;                                      /*  无法匹配设备                */
        }
    }
    
    return  (pdevhdrBest);
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
}
/*********************************************************************************************************
** 函数名称: API_IosNextDevGet
** 功能描述: 在设备链表中获得当前设备的下一个设备
** 输　入  : 
**           pdevhdrHdr                    设备头指针
** 输　出  : 设备头指针
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PLW_DEV_HDR  API_IosNextDevGet (PLW_DEV_HDR    pdevhdrHdr)
{
    REGISTER PLW_LIST_LINE  plineDevHdrNext;
    REGISTER PLW_DEV_HDR    pdevhdrNext;
        
    if (pdevhdrHdr == LW_NULL) {                                        /*  链表中的第一个设备          */
        pdevhdrNext = _LIST_ENTRY(_S_plineDevHdrHeader, LW_DEV_HDR, DEVHDR_lineManage); 
        return  (pdevhdrNext);
        
    } else {                                                            /*  下一个链表节点              */
        plineDevHdrNext = _list_line_get_next(&pdevhdrHdr->DEVHDR_lineManage);          
                                                                        /*  链表中的下一个设备          */
        pdevhdrNext = _LIST_ENTRY(plineDevHdrNext, LW_DEV_HDR, DEVHDR_lineManage);      
        return  (pdevhdrNext);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosCreate
** 功能描述: 调用驱动程序建立设备
** 输　入  : 
**           pdevhdrHdr                    设备头
**           pcName                        设备名
**           iFlag                         创建方式
**           iMode                         UNIX MODE
** 输　出  : 驱动程序返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
LONG  API_IosCreate (PLW_DEV_HDR    pdevhdrHdr,
                     PCHAR          pcName,
                     INT            iFlag,
                     INT            iMode)
{
    REGISTER LONGFUNCPTR  pfuncDrvCreate = _S_deventryTbl[__LW_DEV_MAINDRV].DEVENTRY_pfuncDevCreate;
    
    if (pfuncDrvCreate) {
        LONG    lFValue;
        __KERNEL_SPACE_ENTER();
        lFValue = pfuncDrvCreate(pdevhdrHdr, pcName, iFlag, iMode);
        __KERNEL_SPACE_EXIT();
        return  (lFValue);
    
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosDelete
** 功能描述: 调用驱动程序删除设备
** 输　入  : 
**           pdevhdrHdr                    设备头
**           pcName                        设备名
** 输　出  : 驱动程序返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosDelete (PLW_DEV_HDR    pdevhdrHdr,
                    PCHAR          pcName)
{
    REGISTER FUNCPTR  pfuncDrvDelete = _S_deventryTbl[__LW_DEV_MAINDRV].DEVENTRY_pfuncDevDelete;
    
    if (pfuncDrvDelete) {
        INT     iRet;
        __KERNEL_SPACE_ENTER();
        iRet = pfuncDrvDelete(pdevhdrHdr, pcName);
        __KERNEL_SPACE_EXIT();
        return  (iRet);
        
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosOpen
** 功能描述: 调用驱动程序打开设备
** 输　入  : 
**           pdevhdrHdr                    设备头
**           pcName                        设备名
**           iFlag                         标志
**           iMode                         方式
** 输　出  : 驱动程序返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
LONG  API_IosOpen (PLW_DEV_HDR    pdevhdrHdr,
                   PCHAR          pcName,
                   INT            iFlag,
                   INT            iMode)
{
    REGISTER LONGFUNCPTR  pfuncDrvOpen = _S_deventryTbl[__LW_DEV_MAINDRV].DEVENTRY_pfuncDevOpen;
    
    if (pfuncDrvOpen) {
        LONG    lFValue;
        __KERNEL_SPACE_ENTER();
        lFValue = pfuncDrvOpen(pdevhdrHdr, pcName, iFlag, iMode);
        __KERNEL_SPACE_EXIT();
        return  (lFValue);
        
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosClose
** 功能描述: 调用驱动程序关闭设备
** 输　入  : 
**           iFd                           文件描述符
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosClose (INT  iFd)
{
    return  (API_IosFdRefDec(iFd));                                     /*  只要减少文件引用即可        */
}
/*********************************************************************************************************
** 函数名称: API_IosRead
** 功能描述: 调用驱动程序读取设备
** 输　入  : 
**           iFd                           文件描述符
**           pcBuffer                      缓冲区
**           stMaxByte                     缓冲区大小
** 输　出  : 驱动程序返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ssize_t  API_IosRead (INT      iFd,
                      PCHAR    pcBuffer,
                      size_t   stMaxByte)
{
    REGISTER SSIZETFUNCPTR pfuncDrvRead;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    if (pfdentry->FDENTRY_iFlag & O_WRONLY) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file can not read.\r\n");
        _ErrorHandle(ERROR_IOS_FILE_READ_PROTECTED);
        return  (PX_ERROR);
    }
    
    pfuncDrvRead = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevRead;
    
    if (pfuncDrvRead) {
        ssize_t     sstNum;
        PVOID       pvArg = __GET_DRV_ARG(pfdentry);                    /*  根据驱动程序版本类型选择参数*/
        __KERNEL_SPACE_ENTER();
        sstNum = pfuncDrvRead(pvArg, pcBuffer, stMaxByte);
        __KERNEL_SPACE_EXIT();
        
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_READ, 
                          iFd, stMaxByte, sstNum, LW_NULL);
        
        return  (sstNum);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver not support.\r\n");
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosPRead
** 功能描述: 调用驱动程序读取设备 (在指定偏移量位置, 不改变文件当前偏移量)
** 输　入  : 
**           iFd                           文件描述符
**           pcBuffer                      缓冲区
**           stMaxByte                     缓冲区大小
**           oftPos                        位置
** 输　出  : 驱动程序返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ssize_t  API_IosPRead (INT      iFd,
                       PCHAR    pcBuffer,
                       size_t   stMaxByte,
                       off_t    oftPos)
{
    REGISTER SSIZETFUNCPTR pfuncDrvReadEx;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    if (pfdentry->FDENTRY_iFlag & O_WRONLY) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file can not read.\r\n");
        _ErrorHandle(ERROR_IOS_FILE_READ_PROTECTED);
        return  (PX_ERROR);
    }
    
    pfuncDrvReadEx = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevReadEx;
    if (pfuncDrvReadEx == LW_NULL) {                                    /*  使用 read 替代              */
        pfuncDrvReadEx = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevRead;
    }
    
    if (pfuncDrvReadEx) {
        ssize_t     sstNum;
        PVOID       pvArg = __GET_DRV_ARG(pfdentry);                    /*  根据驱动程序版本类型选择参数*/
        __KERNEL_SPACE_ENTER();
        sstNum = pfuncDrvReadEx(pvArg, pcBuffer, stMaxByte, oftPos);
        __KERNEL_SPACE_EXIT();
        
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_READ, 
                          iFd, stMaxByte, sstNum, LW_NULL);
                         
        return  (sstNum);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver not support.\r\n");
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosWrite
** 功能描述: 调用驱动程序写设备
** 输　入  : 
**           iFd                           文件描述符
**           pcBuffer                      缓冲区
**           stNBytes                      写入数据字节数
** 输　出  : 驱动程序返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ssize_t  API_IosWrite (INT      iFd,
                       CPCHAR   pcBuffer,
                       size_t   stNBytes)
{
    REGISTER SSIZETFUNCPTR pfuncDrvWrite;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    if ((pfdentry->FDENTRY_iFlag & (O_WRONLY | O_RDWR)) == 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file can not write.\r\n");
        _ErrorHandle(ERROR_IOS_FILE_WRITE_PROTECTED);
        return  (PX_ERROR);
    }
    
    pfuncDrvWrite = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevWrite;
    
    if (pfuncDrvWrite) {
        ssize_t     sstNum;
        PVOID       pvArg = __GET_DRV_ARG(pfdentry);                    /*  根据驱动程序版本类型选择参数*/
        __KERNEL_SPACE_ENTER();
        sstNum = pfuncDrvWrite(pvArg, pcBuffer, stNBytes);
        __KERNEL_SPACE_EXIT();
        
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_WRITE, 
                          iFd, stNBytes, sstNum, LW_NULL);
                         
        return  (sstNum);
        
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver not support.\r\n");
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosPWrite
** 功能描述: 调用驱动程序写设备 (在指定偏移量位置, 不改变文件当前偏移量)
** 输　入  : 
**           iFd                           文件描述符
**           pcBuffer                      缓冲区
**           stNBytes                      写入数据字节数
**           oftPos                        位置
** 输　出  : 驱动程序返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ssize_t  API_IosPWrite (INT      iFd,
                        CPCHAR   pcBuffer,
                        size_t   stNBytes,
                        off_t    oftPos)
{
    REGISTER SSIZETFUNCPTR pfuncDrvWriteEx;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    if ((pfdentry->FDENTRY_iFlag & (O_WRONLY | O_RDWR)) == 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file can not write.\r\n");
        _ErrorHandle(ERROR_IOS_FILE_WRITE_PROTECTED);
        return  (PX_ERROR);
    }
    
    pfuncDrvWriteEx = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevWriteEx;
    if (pfuncDrvWriteEx == LW_NULL) {                                   /*  使用 write 替代             */
        pfuncDrvWriteEx = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevWrite;
    }
    
    if (pfuncDrvWriteEx) {
        ssize_t     sstNum;
        PVOID       pvArg = __GET_DRV_ARG(pfdentry);                    /*  根据驱动程序版本类型选择参数*/
        __KERNEL_SPACE_ENTER();
        sstNum = pfuncDrvWriteEx(pvArg, pcBuffer, stNBytes, oftPos);
        __KERNEL_SPACE_EXIT();
        
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_WRITE, 
                          iFd, stNBytes, sstNum, LW_NULL);
                         
        return  (sstNum);
        
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver not support.\r\n");
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosLseek
** 功能描述: 调用驱动程序设置设备的当前指针 (当驱动程序不支持 DEVENTRY_pfuncDevLseek 时, 不作处理)
** 输　入  : 
**           iFd                           文件描述符
**           oftOffset                     偏移量
**           iWhence                       基地址
** 输　出  : 驱动程序返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
off_t  API_IosLseek (INT      iFd,
                     off_t    oftOffset,
                     INT      iWhence)
{
    REGISTER OFFTFUNCPTR   pfuncDrvLseek;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    pfuncDrvLseek = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevLseek;
    
    if (pfuncDrvLseek) {
        off_t       oftOps;
        PVOID       pvArg = __GET_DRV_ARG(pfdentry);                    /*  根据驱动程序版本类型选择参数*/
        __KERNEL_SPACE_ENTER();
        oftOps = pfuncDrvLseek(pvArg, oftOffset, iWhence);
        __KERNEL_SPACE_EXIT();
        return  (oftOps);
    
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosFstat
** 功能描述: 调用驱动程序获取 stat (当驱动程序不支持 DEVENTRY_pfuncDevFstat 时, 不作处理)
** 输　入  : 
**           iFd                           文件描述符
**           pstat                         stat 缓冲区
** 输　出  : 驱动程序返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosFstat (INT   iFd, struct stat *pstat)
{
    REGISTER FUNCPTR       pfuncDrvFstat;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    pfuncDrvFstat = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevFstat;
    
    if (pfuncDrvFstat) {
        INT         iRet;
        PVOID       pvArg = __GET_DRV_ARG(pfdentry);                    /*  根据驱动程序版本类型选择参数*/
        __KERNEL_SPACE_ENTER();
        iRet = pfuncDrvFstat(pvArg, pstat);
        __KERNEL_SPACE_EXIT();
        return  (iRet);
        
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosLstat
** 功能描述: 调用驱动程序获取 lstat (当驱动程序不支持 DEVENTRY_pfuncDevLstat 时, 不作处理)
** 输　入  : 
**           pdevhdrHdr                    设备头
**           pcName                        文件名
**           pstat                         stat 缓冲区
** 输　出  : 驱动程序返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosLstat (PLW_DEV_HDR    pdevhdrHdr,
                   PCHAR          pcName, 
                   struct stat   *pstat)
{
    REGISTER FUNCPTR  pfuncDrvLstat = _S_deventryTbl[__LW_DEV_MAINDRV].DEVENTRY_pfuncDevLstat;
    
    if (pfuncDrvLstat) {
        INT     iRet;
        __KERNEL_SPACE_ENTER();
        iRet = pfuncDrvLstat(pdevhdrHdr, pcName, pstat);
        __KERNEL_SPACE_EXIT();
        return  (iRet);
        
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosIoctl
** 功能描述: 调用驱动程序控制设备 (如果文件处于异常状态不报错误.)
** 输　入  : 
**           iFd                           文件描述符
**           iCmd                          命令
**           lArg                          命令参数
** 输　出  : 驱动程序返回值
** 全局变量: 
** 调用模块: 
** 注  意  : 有些程序正在 select() 等待时, 被删除了, 这是任务清除函数需要 FIOUNSELECT 刚才等待的文件
             如果这些描述文件在这之前被删除了的话, 这里 pfdentry 就等于 NULL 这种情况下不需要打印错误
             因为 FIOUNSELECT 有完善的匹配机制, 就算突然又有新文件打开, 恰好文件描述符又等于 select()
             的文件描述符, 造成 UNSELECT 和 SELECT 的文件虽然描述符相同但文件本质不同的情况, 此情况不会
             造成内核错误, 因为 FIOUNSELECT 有完善的匹配机制.
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosIoctl (INT  iFd, INT  iCmd, LONG  lArg)
{
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    if (pfdentry == LW_NULL) {
        pfdentry =  _IosFileGet(iFd, LW_TRUE);                          /*  忽略异常文件                */
        if (pfdentry == LW_NULL) {                                      /*  文件不存在                  */
            if (iCmd == FIOUNSELECT) {                                  /*  这里可能是 unselect 之前关闭*/
                return  (PX_ERROR);                                     /*  了文件, 不用打印错误        */
            }
            _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
            _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        }
        return  (PX_ERROR);
    }
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_IOCTL, iFd, iCmd, lArg, LW_NULL);
    
    return  (_IosFileIoctl(pfdentry, iCmd, lArg));
}
/*********************************************************************************************************
** 函数名称: API_IosSymlink
** 功能描述: 创建一个连接文件链接到指定的地址
** 输　入  : pdevhdrHdr                    设备头
**           pcName                        创建的文件名
**           pcLinkDst                     链接目标
** 输　出  : 驱动程序返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosSymlink (PLW_DEV_HDR    pdevhdrHdr,
                     PCHAR          pcName,
                     CPCHAR         pcLinkDst)
{
    REGISTER FUNCPTR  pfuncDrvSymlink = _S_deventryTbl[__LW_DEV_MAINDRV].DEVENTRY_pfuncDevSymlink;
    
    if (pfuncDrvSymlink) {
        INT     iRet;
        __KERNEL_SPACE_ENTER();
        iRet = pfuncDrvSymlink(pdevhdrHdr, pcName, pcLinkDst);
        __KERNEL_SPACE_EXIT();
        return  (iRet);
        
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosReadlink
** 功能描述: 读取一个连接文件的连接内容
** 输　入  : pdevhdrHdr                    设备头
**           pcName                        链接原始文件名
**           pcLinkDst                     链接目标文件名
**           stMaxSize                     缓冲大小
** 输　出  : 驱动程序返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ssize_t  API_IosReadlink (PLW_DEV_HDR    pdevhdrHdr,
                          PCHAR          pcName,
                          PCHAR          pcLinkDst,
                          size_t         stMaxSize)
{
    REGISTER SSIZETFUNCPTR  pfuncDrvReadlink = _S_deventryTbl[__LW_DEV_MAINDRV].DEVENTRY_pfuncDevReadlink;
    
    if (pfuncDrvReadlink) {
        ssize_t     sstNum;
        __KERNEL_SPACE_ENTER();
        sstNum = pfuncDrvReadlink(pdevhdrHdr, pcName, pcLinkDst, stMaxSize);
        __KERNEL_SPACE_EXIT();
        return  (sstNum);
    
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosMmap
** 功能描述: 调用驱动程序 mmap 操作.
** 输　入  : 
**           iFd                           文件描述符
**           pdmap                         映射虚拟地址信息
** 输　出  : 驱动程序返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosMmap (INT   iFd, PLW_DEV_MMAP_AREA  pdmap)
{
    REGISTER FUNCPTR       pfuncDrvMmap;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    pfuncDrvMmap = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevMmap;
    
    if (pfuncDrvMmap) {
        INT         iRet;
        PVOID       pvArg = __GET_DRV_ARG(pfdentry);                    /*  根据驱动程序版本类型选择参数*/
        __KERNEL_SPACE_ENTER();
        iRet = pfuncDrvMmap(pvArg, pdmap);
        __KERNEL_SPACE_EXIT();
        return  (iRet);
    
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_IosUnmap
** 功能描述: 调用驱动程序 unmmap 操作.
** 输　入  : 
**           iFd                           文件描述符
**           pdmap                         映射虚拟地址信息
** 输　出  : 驱动程序返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_IosUnmap (INT   iFd, PLW_DEV_MMAP_AREA  pdmap)
{
    REGISTER FUNCPTR       pfuncDrvUnmap;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    pfuncDrvUnmap = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevUnmap;
    
    if (pfuncDrvUnmap) {
        INT         iRet;
        PVOID       pvArg = __GET_DRV_ARG(pfdentry);                    /*  根据驱动程序版本类型选择参数*/
        __KERNEL_SPACE_ENTER();
        iRet = pfuncDrvUnmap(pvArg, pdmap);
        __KERNEL_SPACE_EXIT();
        return  (iRet);
    
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
