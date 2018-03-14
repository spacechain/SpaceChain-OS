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
** 文   件   名: mman.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 08 月 12 日
**
** 描        述: POSIX IEEE Std 1003.1, 2004 Edition sys/mman.h

** BUG:
2011.03.04  如果 vmmMalloc 没有成功则转而使用 __SHEAP_ALLOC().
2011.05.16  mprotect() 使用 vmmSetFlag() 接口操作.
            支持非 REG 文件类型文件 mmap() 操作.
2011.05.17  支持 mmap() 标准, 首先分配虚拟空间, 产生缺页中断时, 分配物理内存.
2011.07.27  当 mmap() 缺页中断填充文件内容没有达到一页时, 需要填充 0 .
            支持设备文件的 mmap() 操作.
2011.08.03  获得文件 stat 使用 fstat api.
2011.12.09  mmap() 填充函数使用 iosRead() 完成.
            加入 mmapShow() 函数, 查看 mmap() 内存情况.
2012.01.10  加入对 MAP_ANONYMOUS 的支持.
2012.08.16  直接使用 pread/pwrite 操作文件.
2012.10.19  mmap() 与 munmap() 加入对文件描述符引用的处理.
2012.12.07  将 mmap 加入资源管理器.
2012.12.22  映射时, 如果不是建立映射的 pid 则判定为回收资源, 这里不减少对文件的引用, 
            因为当前的文件描述符表已不不是创建进程的文件描述符表. 回收器会回收相关的文件描述符.
2012.12.27  mmap 如果是 reg 文件, 也要首先试一下对应文件系统的 mmap 驱动, 如果有则不适用缺页填充.
            加入对 shm_open shm_unlink 的支持.
2012.12.30  mmapShow() 不再显示文件名, 而加入对 pid 的显示.
2013.01.02  结束映射时需要调用驱动的 unmap 操作.
2013.01.12  munmap 如果不是同一进程, 则不操作文件描述符和对应驱动.
2013.03.12  加入 mmap64.
2013.03.16  mmap 如果设备驱动不支持则也可以映射.
2013.03.17  mmap 根据 MAP_SHARED 标志决定是否使能 CACHE.
2013.09.13  支持 MAP_SHARED.
            msync() 存在物理页面且可写时才回写.
2013.12.21  支持 PROT_EXEC.
2014.04.30  加入对 mremap() 的支持.
            存在 VMM 时不在使用 HEAP 进行分配, 保持算法一致性.
2014.05.01  修正 mremap() 错误时的 errno.
2014.07.15  编写独立的 mmap 资源释放函数.
2015.06.01  使用新的 vmmMmap 系列函数实现 mmap 接口.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "unistd.h"
#include "../include/px_mman.h"                                         /*  已包含操作系统头文件        */
#include "../include/posixLib.h"                                        /*  posix 内部公共库            */
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if (LW_CFG_POSIX_EN > 0) && (LW_CFG_DEVICE_EN > 0)
/*********************************************************************************************************
** 函数名称: mlock
** 功能描述: 锁定指定内存地址空间不进行换页处理.
** 输　入  : pvAddr        起始地址
**           stLen         内存空间长度
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  mlock (const void  *pvAddr, size_t  stLen)
{
    (VOID)pvAddr;
    (VOID)stLen;
    
    if (geteuid() != 0) {
        errno = EACCES;
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: munlock
** 功能描述: 解锁指定内存地址空间, 允许进行换页处理.
** 输　入  : pvAddr        起始地址
**           stLen         内存空间长度
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  munlock (const void  *pvAddr, size_t  stLen)
{
    (VOID)pvAddr;
    (VOID)stLen;
    
    if (geteuid() != 0) {
        errno = EACCES;
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: mlockall
** 功能描述: 锁定进程空间不进行换页处理.
** 输　入  : iFlag         锁定选项 MCL_CURRENT / MCL_FUTURE
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  mlockall (int  iFlag)
{
    (VOID)iFlag;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: munlockall
** 功能描述: 解锁进程空间, 允许进行换页处理.
** 输　入  : NONE
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  munlockall (void)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: mprotect
** 功能描述: 设置进程内指定的地址段 page flag
** 输　入  : pvAddr        起始地址
**           stLen         长度
**           iProt         新的属性
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  mprotect (void  *pvAddr, size_t  stLen, int  iProt)
{
#if LW_CFG_VMM_EN > 0
    ULONG   ulFlag = LW_VMM_FLAG_READ;
    
    
    if (!ALIGNED(pvAddr, LW_CFG_VMM_PAGE_SIZE) || stLen == 0) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (iProt & (~(PROT_READ | PROT_WRITE | PROT_EXEC))) {
        errno = ENOTSUP;
        return  (PX_ERROR);
    }
    
    if (iProt) {
        if (iProt & PROT_WRITE) {
            ulFlag |= LW_VMM_FLAG_RDWR;                                 /*  可读写                      */
        }
        if (iProt & PROT_EXEC) {
            ulFlag |= LW_VMM_FLAG_EXEC;                                 /*  可执行                      */
        }
    } else {
        ulFlag = LW_VMM_FLAG_FAIL;                                      /*  不允许访问                  */
    }
    
    if (API_VmmSetFlag(pvAddr, ulFlag) == ERROR_NONE) {                 /*  重新设置新的 flag           */
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
#else
    return  (ERROR_NONE);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: mmap
** 功能描述: 内存文件映射函数, 详见: http://www.opengroup.org/onlinepubs/000095399/functions/mmap.html
** 输　入  : pvAddr        起始地址 (这里必须为 NULL, 系统将自动分配内存)
**           stLen         映射长度
**           iProt         页面属性
**           iFlag         映射标志
**           iFd           文件描述符
**           off           文件偏移量
** 输　出  : 文件映射后的内存地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
void  *mmap (void  *pvAddr, size_t  stLen, int  iProt, int  iFlag, int  iFd, off_t  off)
{
#if LW_CFG_VMM_EN > 0
    PVOID   pvRet;
    INT     iFlags;
    ULONG   ulFlag = LW_VMM_FLAG_FAIL;
    
    if (iProt & PROT_READ) {                                            /*  可读                        */
        ulFlag |= LW_VMM_FLAG_READ;
    }
    if (iProt & PROT_WRITE) {
        ulFlag |= LW_VMM_FLAG_RDWR;                                     /*  可写                        */
    }
    if (iProt & PROT_EXEC) {
        ulFlag |= LW_VMM_FLAG_EXEC;                                     /*  可执行                      */
    }
    
    if (iFlag & MAP_FIXED) {                                            /*  不支持 FIX                  */
        errno = ENOTSUP;
        return  (MAP_FAILED);
    }
    if (iFlag & MAP_ANONYMOUS) {
        iFd = PX_ERROR;
    }
    
    if ((iFlag & MAP_SHARED) && ((iFlag & MAP_PRIVATE) == 0)) {
        iFlags = LW_VMM_SHARED_CHANGE;
    
    } else if ((iFlag & MAP_PRIVATE) && ((iFlag & MAP_SHARED) == 0)) {
        iFlags = LW_VMM_PRIVATE_CHANGE;
    
    } else {
        errno = EINVAL;
        return  (MAP_FAILED);
    }
    
    pvRet = API_VmmMmap(pvAddr, stLen, iFlags, ulFlag, iFd, off);
    
    return  ((pvRet != LW_VMM_MAP_FAILED) ? pvRet : MAP_FAILED);
#else
    errno = ENOSYS;
    return  (MAP_FAILED);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: mmap64 (sylixos 内部 off_t 本身就是 64bit)
** 功能描述: 内存文件映射函数, 详见: http://www.opengroup.org/onlinepubs/000095399/functions/mmap.html
** 输　入  : pvAddr        起始地址 (这里必须为 NULL, 系统将自动分配内存)
**           stLen         映射长度
**           iProt         页面属性
**           iFlag         映射标志
**           iFd           文件描述符
**           off           文件偏移量
** 输　出  : 文件映射后的内存地址
** 全局变量: 
** 调用模块: 
** 注  意  : 如果是 REG 文件, 也首先使用驱动程序的 mmap. 共享内存设备的 mmap 直接会完成映射.
                                           API 函数
*********************************************************************************************************/
LW_API  
void  *mmap64 (void  *pvAddr, size_t  stLen, int  iProt, int  iFlag, int  iFd, off64_t  off)
{
    return  (mmap(pvAddr, stLen, iProt, iFlag, iFd, (off_t)off));
}
/*********************************************************************************************************
** 函数名称: mremap
** 功能描述: 重新设置内存区域的大小, 详见: http://man7.org/linux/man-pages/man2/mremap.2.html
** 输　入  : pvAddr        已经分配的虚拟内存地址
**           stOldSize     当前的内存区域大小
**           stNewSize     需要设置的内存区域新大小
**           iFlag         映射标志     MREMAP_MAYMOVE \ MREMAP_FIXED
** 输　出  : 重新设置后的内存区域地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
void  *mremap (void *pvAddr, size_t stOldSize, size_t stNewSize, int iFlag, ...)
{
#if LW_CFG_VMM_EN > 0
    PVOID   pvRet;
    INT     iMoveEn;

    if (iFlag & MREMAP_FIXED) {
        errno = ENOTSUP;
        return  (MAP_FAILED);
    }
    
    iMoveEn = (iFlag & MREMAP_MAYMOVE) ? 1 : 0;
    
    pvRet = API_VmmMremap(pvAddr, stOldSize, stNewSize, iMoveEn);
    
    return  ((pvRet != LW_VMM_MAP_FAILED) ? pvRet : MAP_FAILED);
#else
    errno = ENOSYS;
    return  (MAP_FAILED);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: munmap
** 功能描述: 取消内存文件映射
** 输　入  : pvAddr        起始地址
**           stLen         映射长度
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  munmap (void  *pvAddr, size_t  stLen)
{
#if LW_CFG_VMM_EN > 0
    return  (API_VmmMunmap(pvAddr, stLen));
#else
    errno = ENOSYS;
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: msync
** 功能描述: 将内存中映射的文件数据回写文件
** 输　入  : pvAddr        起始地址
**           stLen         映射长度
**           iFlag         回写属性
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  msync (void  *pvAddr, size_t  stLen, int  iFlag)
{
#if LW_CFG_VMM_EN > 0
    INT     iInval = (iFlag & MS_INVALIDATE) ? 1 : 0;
    
    return  (API_VmmMsync(pvAddr, stLen, iInval));
#else
    errno = ENOSYS;
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: shm_open
** 功能描述: The posix_madvise() function shall advise the implementation on the expected behavior of the 
             application with respect to the data in the memory starting at address addr, and continuing 
             for len bytes. The implementation may use this information to optimize handling of the 
             specified data. The posix_madvise() function shall have no effect on the semantics of 
             access to memory in the specified range, although it may affect the performance of access.
** 输　入  : addr      address
**           len       length
**           advice    advice
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  posix_madvise (void *addr, size_t len, int advice)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: shm_open
** 功能描述: establishes a connection between a shared memory object and a file descriptor.
** 输　入  : name      file name
**           oflag     create flag like open()
**           mode      create mode like open()
** 输　出  : filedesc
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  shm_open (const char *name, int oflag, mode_t mode)
{
    CHAR    cFullName[MAX_FILENAME_LENGTH] = "/dev/shm/";
    
    if (!name || (*name == PX_EOS)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (name[0] == PX_ROOT) {
        lib_strlcat(cFullName, &name[1], MAX_FILENAME_LENGTH);
    } else {
        lib_strlcat(cFullName, name, MAX_FILENAME_LENGTH);
    }
    
    return  (open(cFullName, oflag, mode));
}
/*********************************************************************************************************
** 函数名称: shm_unlink
** 功能描述: removes the name of the shared memory object named by the string pointed to by name.
** 输　入  : name      file name
** 输　出  : filedesc
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  shm_unlink (const char *name)
{
    CHAR    cFullName[MAX_FILENAME_LENGTH] = "/dev/shm/";
    
    if (!name || (*name == PX_EOS)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (name[0] == PX_ROOT) {
        lib_strlcat(cFullName, &name[1], MAX_FILENAME_LENGTH);
    } else {
        lib_strlcat(cFullName, name, MAX_FILENAME_LENGTH);
    }
    
    return  (unlink(cFullName));
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
