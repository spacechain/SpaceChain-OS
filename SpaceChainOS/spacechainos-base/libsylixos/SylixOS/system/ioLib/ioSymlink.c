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
** 文   件   名: ioInterface.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 06 日
**
** 描        述: 系统 IO 功能函数库符号链接(暂时支持部分功能)

** BUG:
2011.08.15  重大决定: 将所有 posix 定义的函数以函数方式(非宏)引出.
2012.10.17  symlink 与 readlink 不再打印错误信息.
2013.01.03  所有对驱动 close 操作必须通过 _IosFileClose 接口.
2014.12.08  修正 symlink 创建重复连接文件问题.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
** 函数名称: symlink
** 功能描述: 创建一个指向 pcLinkDst 的新符号目录项 pcSymPath.
** 输　入  : pcLinkDst             链接的目标
**           pcSymPath             新创建的符号文件
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  symlink (CPCHAR  pcLinkDst, CPCHAR  pcSymPath)
{
    REGISTER LONG  lValue;
    PLW_FD_ENTRY   pfdentry;
    PLW_DEV_HDR    pdevhdrHdr;
    CHAR           cFullFileName[MAX_FILENAME_LENGTH];
    INT            iLinkCount = 0;
    
    PCHAR          pcName = (PCHAR)pcSymPath;
    INT            iRet;
    
    if (pcName == LW_NULL) {                                            /*  检查文件名                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    if (pcName[0] == PX_EOS) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    if (lib_strcmp(pcName, ".") == 0) {                                 /*  滤掉当前目录                */
        pcName++;
    }
    
    if (ioFullFileNameGet(pcName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    pfdentry = _IosFileNew(pdevhdrHdr, cFullFileName);                  /*  创建一个临时的 fd_entry     */
    if (pfdentry == LW_NULL) {
        return  (PX_ERROR);
    }
    
    for (;;) {
        lValue = iosOpen(pdevhdrHdr, cFullFileName, O_RDONLY, 0);
        if (lValue == FOLLOW_LINK_FILE) {                               /*  已经存在连接文件            */
            _IosFileDelete(pfdentry);
            _ErrorHandle(EEXIST);
            return  (PX_ERROR);
        
        } else if (lValue != FOLLOW_LINK_TAIL) {                        /*  非链接文件直接退出          */
            break;
        
        } else {
            if (iLinkCount++ > _S_iIoMaxLinkLevels) {                   /*  链接文件层数太多            */
                _IosFileDelete(pfdentry);
                _ErrorHandle(ELOOP);
                return  (PX_ERROR);
            }
        }
    
        /*
         *  驱动程序如果返回 FOLLOW_LINK_????, cFullFileName内部一定是目标的绝对地址, 即以/起始的文件名.
         */
        if (ioFullFileNameGet(cFullFileName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
            _IosFileDelete(pfdentry);
            _ErrorHandle(EXDEV);
            return  (PX_ERROR);
        }
    }
    
    if (lValue != PX_ERROR) {
        _IosFileSet(pfdentry, pdevhdrHdr, lValue, O_RDONLY);
        _IosFileClose(pfdentry);                                        /*  关闭                        */
    }
    
    _IosFileDelete(pfdentry);                                           /*  删除临时的 fd_entry         */
    
    iRet = iosSymlink(pdevhdrHdr, cFullFileName, pcLinkDst);
    
    if (iRet == ERROR_NONE) {
        MONITOR_EVT(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_SYMLINK,     pcSymPath);
        MONITOR_EVT(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_SYMLINK_DST, pcLinkDst);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: readlink
** 功能描述: 读取符号链接文件的.
** 输　入  : pcSymPath             链接文件
**           pcLinkDst             连接内容缓冲
**           stMaxSize             缓冲大小
** 输　出  : 读取的内容长度
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ssize_t  readlink (CPCHAR  pcSymPath, PCHAR  pcLinkDst, size_t  stMaxSize)
{
    REGISTER LONG  lValue;
    PLW_FD_ENTRY   pfdentry;
    PLW_DEV_HDR    pdevhdrHdr;
    CHAR           cFullFileName[MAX_FILENAME_LENGTH];
    PCHAR          pcLastTimeName;
    INT            iLinkCount = 0;
    
    ssize_t        sstRet;
    
    PCHAR          pcName = (PCHAR)pcSymPath;
    
    if (pcName == LW_NULL) {                                            /*  检查文件名                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    if (pcName[0] == PX_EOS) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    if (lib_strcmp(pcName, ".") == 0) {                                 /*  滤掉当前目录                */
        pcName++;
    }
    
    if (ioFullFileNameGet(pcName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    pcLastTimeName = (PCHAR)__SHEAP_ALLOC(MAX_FILENAME_LENGTH);
    if (pcLastTimeName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_strlcpy(pcLastTimeName, cFullFileName, MAX_FILENAME_LENGTH);
    
    pfdentry = _IosFileNew(pdevhdrHdr, cFullFileName);                  /*  创建一个临时的 fd_entry     */
    if (pfdentry == LW_NULL) {
        __SHEAP_FREE(pcLastTimeName);
        return  (PX_ERROR);
    }
    
    for (;;) {
        lValue = iosOpen(pdevhdrHdr, cFullFileName, O_RDONLY, 0);
        if (lValue != FOLLOW_LINK_TAIL) {                               /*  FOLLOW_LINK_FILE 直接退出   */
            break;
        
        } else {
            if (iLinkCount++ > _S_iIoMaxLinkLevels) {                   /*  链接文件层数太多            */
                _IosFileDelete(pfdentry);
                __SHEAP_FREE(pcLastTimeName);
                _ErrorHandle(ELOOP);
                return  (PX_ERROR);
            }
        }
    
        /*
         *  驱动程序如果返回 FOLLOW_LINK_????, cFullFileName内部一定是目标的绝对地址, 即以/起始的文件名.
         */
        if (ioFullFileNameGet(cFullFileName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
            _IosFileDelete(pfdentry);
            __SHEAP_FREE(pcLastTimeName);
            _ErrorHandle(EXDEV);
            return  (PX_ERROR);
        }
        lib_strlcpy(pcLastTimeName, cFullFileName, MAX_FILENAME_LENGTH);
    }
    
    if ((lValue != PX_ERROR) && (lValue != FOLLOW_LINK_FILE)) {
        _IosFileSet(pfdentry, pdevhdrHdr, lValue, O_RDONLY);
        _IosFileClose(pfdentry);                                        /*  关闭                        */
    }
    
    _IosFileDelete(pfdentry);                                           /*  删除临时的 fd_entry         */
    
    sstRet = iosReadlink(pdevhdrHdr, pcLastTimeName, pcLinkDst, stMaxSize);
    
    __SHEAP_FREE(pcLastTimeName);
    
    return  (sstRet);
}
/*********************************************************************************************************
** 函数名称: link
** 功能描述: 创建硬链接, SylixOS 当前不支持.
** 输　入  : pcLinkDst             链接的目标
**           pcSymPath             新创建的符号文件
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  link (CPCHAR  pcLinkDst, CPCHAR  pcSymPath)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
