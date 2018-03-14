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
** 文   件   名: KernelUuid.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 04 月 24 日
**
** 描        述: uuid 发生器.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
#include "sys/uuid.h"
#include "sys/time.h"
/*********************************************************************************************************
** 函数名称: __uuidGen
** 功能描述: 获得 uuid
** 输　入  : ptimeNow      当前时间
**           uuid          产生一个结果
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID __uuidGen (time_t timeNow, clock_t clock, uuid_t *uuid)
{
    UINT32  uiTemp;

    uuid->time_low = (UINT32)timeNow;
    uuid->time_mid = (UINT16)(timeNow >> 32);
    uuid->time_hi_and_version = (UINT16)(((timeNow >> 48) & 0xfff) | (1 << 12));
    uuid->clock_seq_hi_and_reserved = (UINT8)(clock >> 8);
    uuid->clock_seq_low = (UINT8)clock;
    
    uiTemp = (UINT32)lib_random();
    uuid->node[0] = (UINT8)(uiTemp >> 24);
    uuid->node[1] = (UINT8)(uiTemp >> 16);
    uuid->node[2] = (UINT8)(uiTemp >> 8);
    uuid->node[3] = (UINT8)(uiTemp);
    
    uiTemp = (UINT32)lib_random();
    uuid->node[4] = (UINT8)(uiTemp >> 24);
    uuid->node[5] = (UINT8)(uiTemp >> 16);
}
/*********************************************************************************************************
** 函数名称: uuidgen
** 功能描述: 获得 uuid
** 输　入  : uuidArray     保存数组
**           iCnt          产生的数量
** 输　出  : 是否成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  uuidgen (uuid_t *uuidArray, INT  iCnt)
{
    time_t  timeNow;
    clock_t clock;

    if (!uuidArray || (iCnt < 1) || (iCnt > 2048)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    timeNow = lib_time(LW_NULL);
    clock   = lib_clock();
    
    for (; iCnt > 0; iCnt--) {
        __uuidGen(timeNow, clock, uuidArray);
        uuidArray++;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
