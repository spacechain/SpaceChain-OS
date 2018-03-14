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
** 文   件   名: lwip_vlanctl.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 29 日
**
** 描        述: ioctl vlan 支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_VLAN_EN > 0
#include "net/if_vlan.h"
#include "net/if_lock.h"
#include "vlan/eth_vlan.h"
/*********************************************************************************************************
** 函数名称: __ifVlanSet
** 功能描述: 设置 VLAN 参数
** 输　入  : pvlanreq    参数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ifVlanSet (const struct vlanreq  *pvlanreq)
{
    INT  iRet = ethernet_vlan_set(pvlanreq);
    
    if (iRet) {
        if (iRet == ETH_VLAN_ENODEV) {
            _ErrorHandle(ENODEV);
        } else {
            _ErrorHandle(ENOTSUP);
        }
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __ifVlanGet
** 功能描述: 设置 VLAN 参数
** 输　入  : pvlanreq    参数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ifVlanGet (struct vlanreq  *pvlanreq)
{
    INT  iRet = ethernet_vlan_get(pvlanreq);
    
    if (iRet) {
        if (iRet == ETH_VLAN_ENODEV) {
            _ErrorHandle(ENODEV);
        } else {
            _ErrorHandle(ENOTSUP);
        }
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __ifVlanWalk
** 功能描述: 遍历 IPv4 路由信息
** 输　入  : pvlanreq   vlan 信息
**           pvlrlist   用户缓存
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __ifVlanWalk (struct vlanreq *pvlanreq, 
                           struct vlanreq_list *pvlrlist)
{
    if (pvlrlist->vlrl_num < pvlrlist->vlrl_bcnt) {
        struct vlanreq *pvlanreqSave = &pvlrlist->vlrl_buf[pvlrlist->vlrl_num];
        *pvlanreqSave = *pvlanreq;
        pvlrlist->vlrl_num++;
    }
}
/*********************************************************************************************************
** 函数名称: __ifVlanLst
** 功能描述: 获取整个 VLAN 设置
** 输　入  : prtelist  路由信息缓存
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ifVlanLst (struct vlanreq_list *pvlrlist)
{
    UINT    uiTotal;
    
    pvlrlist->vlrl_num = 0;
    
    ethernet_vlan_total(&uiTotal);
    pvlrlist->vlrl_total = uiTotal;
    if (!pvlrlist->vlrl_bcnt || !pvlrlist->vlrl_buf) {
        return  (ERROR_NONE);
    }
    ethernet_vlan_traversal(__ifVlanWalk, pvlrlist, LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ifIoctlVlan
** 功能描述: SIOCGETVLAN / SIOCSETVLAN / SIOCLSTVLAN 命令处理接口
** 输　入  : iCmd       SIOCGETVLAN / SIOCSETVLAN / SIOCLSTVLAN
**           pvArg      struct vlanreq / struct vlanreq_list
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __ifIoctlVlan (INT  iCmd, PVOID  pvArg)
{
    struct vlanreq  *pvlanreq = (struct vlanreq *)pvArg;
    INT              iRet     = PX_ERROR;
    
    if (!pvlanreq) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    switch (iCmd) {
    
    case SIOCSETVLAN:
        LWIP_IF_LIST_LOCK(LW_FALSE);
        iRet = __ifVlanSet(pvlanreq);
        LWIP_IF_LIST_UNLOCK();
        break;
        
    case SIOCGETVLAN:
        LWIP_IF_LIST_LOCK(LW_FALSE);
        iRet = __ifVlanGet(pvlanreq);
        LWIP_IF_LIST_UNLOCK();
        break;
    
    case SIOCLSTVLAN:
        LWIP_IF_LIST_LOCK(LW_FALSE);
        iRet = __ifVlanLst((struct vlanreq_list *)pvArg);
        LWIP_IF_LIST_UNLOCK();
        break;
        
    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_NET_VLAN_EN          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
