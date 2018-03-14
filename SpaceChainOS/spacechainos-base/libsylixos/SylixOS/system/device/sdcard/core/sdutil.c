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
** 文   件   名: sdutil.c
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2014 年 10 月 27 日
**
** 描        述: sd 工具库.

** BUG:
2014.11.07  更改__listObjInsert(), 元素插入到链头
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0)
#include "sdutil.h"
/*********************************************************************************************************
  数据结构定义
*********************************************************************************************************/
typedef struct __unit_pool {
    UINT32              UNITPOOL_uiSpace;
    LW_SPINLOCK_DEFINE (UNITPOOL_slLock);
} __UNIT_POOL;
/*********************************************************************************************************
** 函数名称: __sdUnitPoolCreate
** 功能描述: 创建一个单元号池
** 输    入: NONE
** 输    出: 单元号池对象
** 返    回:
*********************************************************************************************************/
VOID *__sdUnitPoolCreate (VOID)
{
    __UNIT_POOL *punitpool =  (__UNIT_POOL *)__SHEAP_ALLOC(sizeof(__UNIT_POOL));
    if (!punitpool) {
        return  (LW_NULL);
    }

    punitpool->UNITPOOL_uiSpace = 0;
    LW_SPIN_INIT(&punitpool->UNITPOOL_slLock);

    return  (punitpool);
}
/*********************************************************************************************************
** 函数名称: __sdUnitPoolDelete
** 功能描述: 删除一个单元号池
** 输    入: pvUnitPool   单元号池对象
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __sdUnitPoolDelete (VOID *pvUnitPool)
{
    if (pvUnitPool) {
        __SHEAP_FREE(pvUnitPool);
    }
}
/*********************************************************************************************************
** 函数名称: __sdUnitGet
** 功能描述: 从单元号池获得一个单元号
** 输    入: pvUnitPool   单元号池对象
** 输    出: 分配的单元号(成功: > 0, 失败: < 0)
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT   __sdUnitGet (VOID *pvUnitPool)
{
    __UNIT_POOL *punitpool = (__UNIT_POOL *)pvUnitPool;
    INT          iUnit;
    INTREG       iregInterLevel;

    if (!pvUnitPool) {
        return  (-1);
    }

    LW_SPIN_LOCK_QUICK(&punitpool->UNITPOOL_slLock, &iregInterLevel);
    for (iUnit = 0; iUnit < 32; iUnit++) {
        if ((punitpool->UNITPOOL_uiSpace & (1 << iUnit)) == 0) {
            punitpool->UNITPOOL_uiSpace |= (1 << iUnit);
            LW_SPIN_UNLOCK_QUICK(&punitpool->UNITPOOL_slLock, iregInterLevel);
            return  (iUnit);
        }
    }
    LW_SPIN_UNLOCK_QUICK(&punitpool->UNITPOOL_slLock, iregInterLevel);

    return  (-1);
}
/*********************************************************************************************************
** 函数名称: __sdUnitPut
** 功能描述: 回收单元号
** 输    入: pvUnitPool   单元号池对象
**           iUnit        要回收的单元号
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __sdUnitPut (VOID *pvUnitPool, INT iUnit)
{
    __UNIT_POOL *punitpool = (__UNIT_POOL *)pvUnitPool;
    INTREG       iregInterLevel;

    if (iUnit < 0 || iUnit > 31 || !pvUnitPool) {
        return;
    }

    LW_SPIN_LOCK_QUICK(&punitpool->UNITPOOL_slLock, &iregInterLevel);
    punitpool->UNITPOOL_uiSpace &= ~(1 << iUnit);
    LW_SPIN_UNLOCK_QUICK(&punitpool->UNITPOOL_slLock, iregInterLevel);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
