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
** 文   件   名: diskCacheThread.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 03 月 23 日
**
** 描        述: 磁盘高速缓冲控制器背景回写任务.
**
** BUG:
2016.12.21  简化文件系统回写线程设计.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "diskCacheLib.h"
#include "diskCache.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)
/*********************************************************************************************************
  经验数据
*********************************************************************************************************/
#define __LW_DISKCACHE_BG_SECONDS       2                               /*  回写周期                    */
#define __LW_DISKCACHE_BG_MINSECTOR     128                             /*  每次回写的扇区数            */
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
extern INT  __diskCacheIoctl(PLW_DISKCACHE_CB   pdiskcDiskCache, INT  iCmd, LONG  lArg);
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
LW_OBJECT_HANDLE        _G_ulDiskCacheListLock  = 0ul;                  /*  链表锁                      */
PLW_LIST_LINE           _G_plineDiskCacheHeader = LW_NULL;              /*  链表头                      */
/*********************************************************************************************************
  锁操作
*********************************************************************************************************/
#define __LW_DISKCACHE_LIST_LOCK()      \
        API_SemaphoreMPend(_G_ulDiskCacheListLock, LW_OPTION_WAIT_INFINITE)
#define __LW_DISKCACHE_LIST_UNLOCK()    \
        API_SemaphoreMPost(_G_ulDiskCacheListLock)
/*********************************************************************************************************
** 函数名称: __diskCacheThread
** 功能描述: 磁盘高速缓冲控制器背景回写任务
** 输　入  : pvArg             启动参数
** 输　出  : NULL
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  __diskCacheThread (PVOID  pvArg)
{
    PLW_DISKCACHE_CB   pdiskcDiskCache;
    PLW_LIST_LINE      plineCache;
    ULONG              ulNSector = __LW_DISKCACHE_BG_MINSECTOR;
    
    (VOID)pvArg;
    
    for (;;) {
        API_TimeSSleep(__LW_DISKCACHE_BG_SECONDS);                      /*  近似延时                    */
        
        __LW_DISKCACHE_LIST_LOCK();
        for (plineCache  = _G_plineDiskCacheHeader;
             plineCache != LW_NULL;
             plineCache  = _list_line_get_next(plineCache)) {           /*  遍历所有磁盘缓冲            */
             
            pdiskcDiskCache = _LIST_ENTRY(plineCache, 
                                          LW_DISKCACHE_CB, 
                                          DISKC_lineManage);            /*  回写磁盘                    */
            __diskCacheIoctl(pdiskcDiskCache, LW_BLKD_DISKCACHE_RAMFLUSH, ulNSector);
        }
        __LW_DISKCACHE_LIST_UNLOCK();
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __diskCacheListAdd
** 功能描述: 将 DISK CACHE 块加入回写链
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __diskCacheListAdd (PLW_DISKCACHE_CB   pdiskcDiskCache)
{
    __LW_DISKCACHE_LIST_LOCK();
    _List_Line_Add_Ahead(&pdiskcDiskCache->DISKC_lineManage, &_G_plineDiskCacheHeader);
    __LW_DISKCACHE_LIST_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __diskCacheListDel
** 功能描述: 从回写链中将 DISK CACHE 移除
** 输　入  : pdiskcDiskCache    磁盘 CACHE 控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __diskCacheListDel (PLW_DISKCACHE_CB   pdiskcDiskCache)
{
    __LW_DISKCACHE_LIST_LOCK();
    _List_Line_Del(&pdiskcDiskCache->DISKC_lineManage, &_G_plineDiskCacheHeader);
    __LW_DISKCACHE_LIST_UNLOCK();
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
