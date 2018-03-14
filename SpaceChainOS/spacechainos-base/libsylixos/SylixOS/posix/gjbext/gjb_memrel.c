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
** 文   件   名: gjb_memrel.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 04 月 13 日
**
** 描        述: GJB7714 扩展接口内存管理相关部分.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_POSIX_EN > 0) && (LW_CFG_GJB7714_EN > 0)
#include "../include/px_gjbext.h"
/*********************************************************************************************************
** 函数名称: heap_mem_init
** 功能描述: 初始化系统内存管理.
** 输　入  : flag      FIRST_FIT_ALLOCATION or BUDDY_ALLOCATION
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  heap_mem_init (int flag)
{
    (VOID)flag;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: mem_findmax
** 功能描述: 获得系统内存最长连续空闲区域大小.
** 输　入  : flag      FIRST_FIT_ALLOCATION or BUDDY_ALLOCATION
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  mem_findmax (void)
{
    size_t  stMax;
    
    if (getpid() > 0) {
        return  (LW_CFG_MB_SIZE);
    
    } else {
        stMax = _HeapGetMax(_K_pheapSystem);
    }
    
    return  ((int)stMax);
}
/*********************************************************************************************************
** 函数名称: mem_getinfo
** 功能描述: 获得系统内存信息.
** 输　入  : info      内存信息
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  mem_getinfo (struct meminfo *info)
{
    if (!info) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    if (getpid() > 0) {
        errno = EACCES;
        return  (PX_ERROR);
    
    } else {
        __KERNEL_ENTER();
        info->segment = _K_pheapSystem->HEAP_ulSegmentCounter;
        info->used    = _K_pheapSystem->HEAP_stUsedByteSize;
        info->maxused = _K_pheapSystem->HEAP_stMaxUsedByteSize;
        info->free    = _K_pheapSystem->HEAP_stFreeByteSize;
        __KERNEL_EXIT();
        
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: mem_show
** 功能描述: 显示系统内存信息.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
void  mem_show (void)
{
    lib_system("free");
}
/*********************************************************************************************************
** 函数名称: mpart_module_init
** 功能描述: 初始化内存管理模块.
** 输　入  : max_parts     最大分块数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  mpart_module_init (int  max_parts)
{
    if (max_parts > LW_CFG_MAX_REGIONS) {
        errno = ENOSPC;
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: mpart_create
** 功能描述: 创建一个内存分区.
** 输　入  : addr      内存基地址
**           size      内存区域大小
**           mid       内存 ID
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  mpart_create (char *addr, size_t  size, mpart_id *mid)
{
    if (!addr || !size || !mid) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (API_RegionCreate("GJB mpart", addr, size, 
                         LW_OPTION_DEFAULT, mid)) {
        return  (ERROR_NONE);
    }
    
    errno = ENOSPC;
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: mpart_delete
** 功能描述: 删除一个内存分区.
** 输　入  : mid       内存 ID
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  mpart_delete (mpart_id mid)
{
    ULONG   ulError;
    
    ulError = API_RegionDelete(&mid);
    if (ulError) {
        if (ulError == ERROR_REGION_USED) {
            errno = EBUSY;
            return  (PX_ERROR);
        }
        
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: mpart_addmem
** 功能描述: 向内存分区添加内存.
** 输　入  : mid       内存 ID
**           addr      内存基址
**           size      内存大小
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  mpart_addmem (mpart_id mid, char *addr, size_t  size)
{
    if (!addr || !size) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (API_RegionAddMem(mid, addr, size)) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: mpart_alloc
** 功能描述: 分配内存.
** 输　入  : mid       内存 ID
**           size      大小
** 输　出  : 分配出来的内存地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
void  *mpart_alloc (mpart_id mid, size_t  size)
{
    void  *ret;

    if (!size) {
        errno = EINVAL;
        return  (LW_NULL);
    }
    
    ret = API_RegionGet(mid, size);
    if (!ret) {
        if (errno == ERROR_REGION_NOMEM) {
            errno =  ENOMEM;
        }
    }
    
    return  (ret);
}
/*********************************************************************************************************
** 函数名称: mpart_memalign
** 功能描述: 对齐分配内存.
** 输　入  : mid       内存 ID
**           alignment 对齐大小
**           size      大小
** 输　出  : 分配出来的内存地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
void  *mpart_memalign (mpart_id mid, size_t  alignment, size_t  size)
{
    void  *ret;

    if (!size) {
        errno = EINVAL;
        return  (LW_NULL);
    }
    
    ret = API_RegionGetAlign(mid, size, alignment);
    if (!ret) {
        if (errno == ERROR_REGION_NOMEM) {
            errno =  ENOMEM;
        }
    }
    
    return  (ret);
}
/*********************************************************************************************************
** 函数名称: mpart_realloc
** 功能描述: 重新分配内存.
** 输　入  : mid       内存 ID
**           addr      内存
**           size      大小
** 输　出  : 分配出来的内存地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
void  *mpart_realloc (mpart_id mid, char *addr, size_t  size)
{
    void  *ret;

    if (!size) {
        errno = EINVAL;
        return  (LW_NULL);
    }
    
    ret = API_RegionReget(mid, addr, size);
    if (!ret) {
        if (errno == ERROR_REGION_NOMEM) {
            errno =  ENOMEM;
        }
    }
    
    return  (ret);
}
/*********************************************************************************************************
** 函数名称: mpart_free
** 功能描述: 释放内存.
** 输　入  : mid       内存 ID
**           addr      内存
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  mpart_free (mpart_id mid, char *addr)
{
    void  *ret;

    if (!addr) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    ret = API_RegionPut(mid, addr);
    if (ret) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: mpart_findmaxfree
** 功能描述: 获得内存最大空闲分断.
** 输　入  : mid       内存 ID
** 输　出  : 最大空闲分段
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  mpart_findmaxfree (mpart_id mid)
{
    size_t  max;

    if (API_RegionGetMax(mid, &max)) {
        return  (PX_ERROR);
    }
    
    return  ((int)max);
}
/*********************************************************************************************************
** 函数名称: mpart_getinfo
** 功能描述: 获得内存信息.
** 输　入  : mid       内存 ID
**           pi        内存分段信息
** 输　出  : 最大空闲分段
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  mpart_getinfo (mpart_id mid, struct partinfo  *pi)
{
    if (!pi) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (API_RegionStatus(mid, LW_NULL, &pi->segment, 
                         &pi->used, &pi->free, &pi->maxused)) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_GJB7714_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
