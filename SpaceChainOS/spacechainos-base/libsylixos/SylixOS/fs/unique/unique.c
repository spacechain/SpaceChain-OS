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
** 文   件   名: unique.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 01 月 08 日
**
** 描        述: 为没有 inode 序列号的文件系统提供序列号支持, 例如 FAT SMB 等.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_FATFS_EN > 0)
#include "unique.h"
/*********************************************************************************************************
  inode unique pool
*********************************************************************************************************/
#define UNIQ_INO_IS_BUSY(index, arr)    \
        (((arr)[((index) >> 3)] >> ((index) & (8 - 1))) & 0x01)
#define SET_UNIQ_INO_BUSY(index, arr)   \
        ((arr)[((index) >> 3)] |= (0x01 << ((index) & (8 - 1))))
#define SET_UNIQ_INO_FREE(index, arr)   \
        ((arr)[((index) >> 3)] &= (~(0x01 << ((index) & (8 - 1)))))
/*********************************************************************************************************
** 函数名称: API_FsUniqueCreate
** 功能描述: 创建一个 unique 的发生器
** 输　入  : stSize        unique 池大小
**           ulResvNo      保留数字的最大值
** 输　出  : LW_UNIQUE_POOL 结构指针
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
PLW_UNIQUE_POOL  API_FsUniqueCreate (size_t  stSize, ULONG  ulResvNo)
{
    PLW_UNIQUE_POOL     punip = (PLW_UNIQUE_POOL)__SHEAP_ALLOC(sizeof(LW_UNIQUE_POOL));
    
    if (punip == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    
    punip->UNIP_pcArray = (PCHAR)__SHEAP_ALLOC(stSize);
    if (punip->UNIP_pcArray == LW_NULL) {
        __SHEAP_FREE(punip);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    lib_bzero(punip->UNIP_pcArray, stSize);
    
    if (ulResvNo == 0) {
        ulResvNo =  1;
    }
    
    punip->UNIP_ulIndex  = 0;
    punip->UNIP_stSize   = stSize;
    punip->UNIP_ulResvNo = ulResvNo;
    
    return  (punip);
}
/*********************************************************************************************************
** 函数名称: API_FsUniqueDelete
** 功能描述: 删除一个 unique 的发生器
** 输　入  : punip         UNIQUE_POOL 结构
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID  API_FsUniqueDelete (PLW_UNIQUE_POOL  punip)
{
    if (punip) {
        if (punip->UNIP_pcArray) {
            __SHEAP_FREE(punip->UNIP_pcArray);
        }
        __SHEAP_FREE(punip);
    }
}
/*********************************************************************************************************
** 函数名称: API_FsUniqueAlloc
** 功能描述: 分配一个 unique 号
** 输　入  : punip         UNIQUE_POOL 结构
** 输　出  : 分配的 unique 号
** 全局变量: 
** 调用模块: 
** 注  意  : 这里不能使用 realloc 操作, 如果 realloc 失败会丢失以前管理的 inode
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_FsUniqueAlloc (PLW_UNIQUE_POOL  punip)
{
    INT     i;
    BOOL    bGet = LW_FALSE;
    PCHAR   pcNewArray;
    size_t  stNewSize;
    
    while (!bGet) {
        for (i = 0; i < punip->UNIP_stSize; i++) {
            if (!UNIQ_INO_IS_BUSY(punip->UNIP_ulIndex, punip->UNIP_pcArray)) {
                SET_UNIQ_INO_BUSY(punip->UNIP_ulIndex, punip->UNIP_pcArray);
                return (punip->UNIP_ulResvNo + punip->UNIP_ulIndex);
            }
            punip->UNIP_ulIndex++;
            if (punip->UNIP_ulIndex >= punip->UNIP_stSize) {
                punip->UNIP_ulIndex = 0;
            }
        }
        
        if ((punip->UNIP_stSize << 1) < (0x0FFFFFFF - punip->UNIP_ulResvNo)) {
            stNewSize  = punip->UNIP_stSize << 1;
            pcNewArray = (PCHAR)__SHEAP_ALLOC(stNewSize);               /*  分配更大的 unique 缓冲区    */
            if (pcNewArray == LW_NULL) {
                bGet = LW_TRUE;
            
            } else {
                lib_memcpy(pcNewArray, punip->UNIP_pcArray, punip->UNIP_stSize);
                __SHEAP_FREE(punip->UNIP_pcArray);
                punip->UNIP_pcArray = pcNewArray;
                punip->UNIP_stSize  = stNewSize;
            }
        } else {
            bGet = LW_TRUE;
        }
    }
    
    return  (0);                                                        /*  分配失败                    */
}
/*********************************************************************************************************
** 函数名称: API_FsUniqueFree
** 功能描述: 回收一个 unique 号
** 输　入  : punip         UNIQUE_POOL 结构
**           ulNo          inode 号
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID  API_FsUniqueFree (PLW_UNIQUE_POOL  punip, ULONG  ulNo)
{
    SET_UNIQ_INO_FREE((ulNo - punip->UNIP_ulResvNo), punip->UNIP_pcArray);
}
/*********************************************************************************************************
** 函数名称: API_FsUniqueIsVal
** 功能描述: 判断一个 unique 号范围是否合法
** 输　入  : punip         UNIQUE_POOL 结构
**           ulNo          inode 号
** 输　出  : 是否合法
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
BOOL  API_FsUniqueIsVal (PLW_UNIQUE_POOL  punip, ULONG  ulNo)
{
    return  (ulNo >= punip->UNIP_ulResvNo);
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_FATFS_EN > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
