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
** 文   件   名: busSystem.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 10 月 27 日
**
** 描        述: 总线系统管理 (目前仅限于打印信息).

** BUG:
2011.03.06  修正 gcc 4.5.1 相关 warning.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_LIST_LINE_HEADER          _G_plineBusAdapterHeader = LW_NULL;
static LW_OBJECT_HANDLE             _G_hBusListLock = LW_OBJECT_HANDLE_INVALID;
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define __BUS_LIST_LOCK()           API_SemaphoreBPend(_G_hBusListLock, LW_OPTION_WAIT_INFINITE)
#define __BUS_LIST_UNLOCK()         API_SemaphoreBPost(_G_hBusListLock)
/*********************************************************************************************************
** 函数名称: __busSystemInit
** 功能描述: 初始化 BUS 组件库
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __busSystemInit (VOID)
{
    if (_G_hBusListLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hBusListLock = API_SemaphoreBCreate("bus_listlock", 
                                               LW_TRUE, LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                               LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: __busAdapterCreate
** 功能描述: 创建一个总线适配器
** 输　入  : pbusadapter       总线适配器控制块
**           pcName            总线适配器名
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __busAdapterCreate (PLW_BUS_ADAPTER  pbusadapter, CPCHAR  pcName)
{
    API_AtomicSet(0, &pbusadapter->BUSADAPTER_atomicCounter);
    lib_strlcpy(pbusadapter->BUSADAPTER_cName, pcName, LW_CFG_OBJECT_NAME_SIZE);

    __BUS_LIST_LOCK();
    _List_Line_Add_Ahead(&pbusadapter->BUSADAPTER_lineManage, &_G_plineBusAdapterHeader);
    __BUS_LIST_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __busAdapterDelete
** 功能描述: 删除一个总线适配器
** 输　入  : pcName            总线适配器名
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __busAdapterDelete (CPCHAR  pcName)
{
    REGISTER PLW_BUS_ADAPTER        pbusadapter = __busAdapterGet(pcName);
    
    if (pbusadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);
        return  (PX_ERROR);
    }
    
    __BUS_LIST_LOCK();
    _List_Line_Del(&pbusadapter->BUSADAPTER_lineManage, &_G_plineBusAdapterHeader);
    __BUS_LIST_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __busAdapterGet
** 功能描述: 查找一个总线适配器
** 输　入  : pcName            总线适配器名
** 输　出  : 总线适配器控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_BUS_ADAPTER  __busAdapterGet (CPCHAR  pcName)
{
    REGISTER PLW_LIST_LINE          plineTemp;
    REGISTER PLW_BUS_ADAPTER        pbusadapter = LW_NULL;

    if (pcName == LW_NULL) {
        return  (LW_NULL);
    }
    
    __BUS_LIST_LOCK();                                                  /*  锁定 BUS 链表               */
    for (plineTemp  = _G_plineBusAdapterHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  遍历适配器表                */
        
        pbusadapter = _LIST_ENTRY(plineTemp, LW_BUS_ADAPTER, BUSADAPTER_lineManage);
        if (lib_strcmp(pcName, pbusadapter->BUSADAPTER_cName) == 0) {
            break;                                                      /*  已经找到了                  */
        }
    }
    __BUS_LIST_UNLOCK();                                                /*  解锁 BUS 链表               */
    
    if (plineTemp) {
        return  (pbusadapter);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: API_BusShow
** 功能描述: 打印系统总线信息
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_BusShow (VOID)
{
    static const CHAR   cBusInfoHdr[] = "\n"
    "    BUS NAME   DEV NUM\n"
    "-------------- -------\n";

    REGISTER PLW_LIST_LINE          plineTemp;
    REGISTER PLW_BUS_ADAPTER        pbusadapter;
             INT                    iCounter;
    
    printf(cBusInfoHdr);

    __BUS_LIST_LOCK();                                                  /*  锁定 BUS 链表               */
    for (plineTemp  = _G_plineBusAdapterHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  遍历适配器表                */
    
        pbusadapter = _LIST_ENTRY(plineTemp, LW_BUS_ADAPTER, BUSADAPTER_lineManage);
        iCounter    = API_AtomicGet(&pbusadapter->BUSADAPTER_atomicCounter);
        
        printf("%-14s %7d\n", pbusadapter->BUSADAPTER_cName, iCounter);
    }
    __BUS_LIST_UNLOCK();                                                /*  解锁 BUS 链表               */
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
