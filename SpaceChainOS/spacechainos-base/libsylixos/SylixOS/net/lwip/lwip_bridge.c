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
** 文   件   名: lwip_bridge.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 05 日
**
** 描        述: 网络桥接管理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_DEV_BRIDGE_EN > 0)
#include "net/if_bridge.h"
#include "bridge/netbridge.h"
/*********************************************************************************************************
  驱动与设备
*********************************************************************************************************/
static LW_DEV_HDR       _G_devhdrNetbr;
static INT              _G_iNetbrDrvNum = PX_ERROR;
/*********************************************************************************************************
** 函数名称: _netbrOpen
** 功能描述: 打开网络桥接控制设备
** 输　入  : pdevhdr          控制设备
**           pcName           名称
**           iFlags           方式
**           iMode            方法
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  _netbrOpen (PLW_DEV_HDR   pdevhdr, 
                         PCHAR         pcName,
                         INT           iFlags, 
                         INT           iMode)
{
    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        LW_DEV_INC_USE_COUNT(pdevhdr);
        return  ((LONG)pdevhdr);
    }
}
/*********************************************************************************************************
** 函数名称: _netbrClose
** 功能描述: 关闭网络桥接控制设备
** 输　入  : pdevhdr          控制设备
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _netbrClose (PLW_DEV_HDR   pdevhdr)
{
    if (pdevhdr) {
        LW_DEV_DEC_USE_COUNT(pdevhdr);
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: _netbrIoctl
** 功能描述: 控制网络桥接控制设备
** 输　入  : pdevhdr          控制设备
**           iCmd             功能
**           lArg             参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _netbrIoctl (PLW_DEV_HDR   pdevhdr, 
                         INT           iCmd, 
                         LONG          lArg)
{
    struct net_bridge_ctl   *pnetbrcrl;
    struct stat             *pstat;
    INT                      iRet = PX_ERROR;
    
    switch (iCmd) {
    
    case NETBR_CTL_ADD:
        pnetbrcrl = (struct net_bridge_ctl *)lArg;
        if (pnetbrcrl) {
            iRet = netbr_add(pnetbrcrl->br_dev, LW_NULL, LW_NULL, LW_NULL, &pnetbrcrl->br_index);
        }
        return  (iRet);
    
    case NETBR_CTL_DELETE:
        pnetbrcrl = (struct net_bridge_ctl *)lArg;
        if (pnetbrcrl) {
            iRet = netbr_delete(pnetbrcrl->br_dev);
        }
        return  (iRet);
    
    case NETBR_CTL_ADD_DEV:
        pnetbrcrl = (struct net_bridge_ctl *)lArg;
        if (pnetbrcrl) {
            iRet = netbr_add_dev(pnetbrcrl->br_dev, pnetbrcrl->eth_dev, 0);
        }
        return  (iRet);
    
    case NETBR_CTL_DELETE_DEV:
        pnetbrcrl = (struct net_bridge_ctl *)lArg;
        if (pnetbrcrl) {
            iRet = netbr_delete_dev(pnetbrcrl->br_dev, pnetbrcrl->eth_dev, 0);
        }
        return  (iRet);
        
    case NETBR_CTL_ADD_IF:
        pnetbrcrl = (struct net_bridge_ctl *)lArg;
        if (pnetbrcrl) {
            iRet = netbr_add_dev(pnetbrcrl->br_dev, pnetbrcrl->eth_dev, 1);
        }
        return  (iRet);
    
    case NETBR_CTL_DELETE_IF:
        pnetbrcrl = (struct net_bridge_ctl *)lArg;
        if (pnetbrcrl) {
            iRet = netbr_delete_dev(pnetbrcrl->br_dev, pnetbrcrl->eth_dev, 1);
        }
        return  (iRet);
    
    case NETBR_CTL_CACHE_FLUSH:
        pnetbrcrl = (struct net_bridge_ctl *)lArg;
        if (pnetbrcrl) {
            iRet = netbr_flush_cache(pnetbrcrl->br_dev);
        }
        return  (iRet);
    
    case FIOFSTATGET:
        pstat = (struct stat *)lArg;
        if (pstat) {
            pstat->st_dev     = (dev_t)pdevhdr;
            pstat->st_ino     = (ino_t)0;                               /*  相当于唯一节点              */
            pstat->st_mode    = 0600 | S_IFCHR;
            pstat->st_nlink   = 1;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 1;
            pstat->st_size    = 0;
            pstat->st_blksize = 0;
            pstat->st_blocks  = 0;
            pstat->st_atime   = API_RootFsTime(LW_NULL);
            pstat->st_mtime   = API_RootFsTime(LW_NULL);
            pstat->st_ctime   = API_RootFsTime(LW_NULL);
            iRet              = ERROR_NONE;
        }
        return  (iRet);
    
    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: _netBridgeDrvInit
** 功能描述: 初始化网络桥接管理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _netBridgeDrvInit (VOID)
{
    if (_G_iNetbrDrvNum <= 0) {
        _G_iNetbrDrvNum  = iosDrvInstall(_netbrOpen,
                                         LW_NULL,
                                         _netbrOpen,
                                         _netbrClose,
                                         LW_NULL,
                                         LW_NULL,
                                         _netbrIoctl);
        DRIVER_LICENSE(_G_iNetbrDrvNum,     "Dual BSD/GPL->Ver 1.0");
        DRIVER_AUTHOR(_G_iNetbrDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iNetbrDrvNum, "net bridge management driver.");
    }
    
    return  ((_G_iNetbrDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** 函数名称: _netBridgeDevCreate
** 功能描述: 安装 netbr 设备
** 输　入  : NONE
** 输　出  : 设备是否创建成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _netBridgeDevCreate (VOID)
{
    if (_G_iNetbrDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&_G_devhdrNetbr, NETBR_CTL_PATH, _G_iNetbrDrvNum, DT_CHR)) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellNetbr
** 功能描述: 系统命令 "netbr"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellNetbr (INT  iArgC, PCHAR  *ppcArgV)
{
    INT     iRet;
    INT     iIndex;
    
    if (iArgC == 3) {
        if (lib_strcmp(ppcArgV[1], "addbr") == 0) {
            iRet = netbr_add(ppcArgV[2], LW_NULL, LW_NULL, LW_NULL, &iIndex);
            if (iRet) {
                fprintf(stderr, "can not add net bridge device!\n");
            } else {
                printf("net bridge device add ok, if index: %d.\n", iIndex);
            }
        
        } else if (lib_strcmp(ppcArgV[1], "delbr") == 0) {
            iRet = netbr_delete(ppcArgV[2]);
            if (iRet) {
                fprintf(stderr, "can not delete net bridge device!\n");
            } else {
                printf("net bridge device delete.\n");
            }
        
        } else if (lib_strcmp(ppcArgV[1], "flush") == 0) {
            iRet = netbr_flush_cache(ppcArgV[2]);
            if (iRet) {
                fprintf(stderr, "can not flush net bridge device!\n");
            }
        
        } else {
            goto    __arg_error;
        }
    
    } else if (iArgC == 4) {
        if (lib_strcmp(ppcArgV[1], "adddev") == 0) {
            iRet = netbr_add_dev(ppcArgV[2], ppcArgV[3], 0);
            if (iRet) {
                fprintf(stderr, "can not add device '%s' to bridge '%s'!\n", ppcArgV[3], ppcArgV[2]);
            }
        
        } else if (lib_strcmp(ppcArgV[1], "deldev") == 0) {
            iRet = netbr_delete_dev(ppcArgV[2], ppcArgV[3], 0);
            if (iRet) {
                fprintf(stderr, "can not delete device '%s' from bridge '%s'!\n", ppcArgV[3], ppcArgV[2]);
            }
        
        } else if (lib_strcmp(ppcArgV[1], "addif") == 0) {
            iRet = netbr_add_dev(ppcArgV[2], ppcArgV[3], 1);
            if (iRet) {
                fprintf(stderr, "can not add if '%s' to bridge '%s'!\n", ppcArgV[3], ppcArgV[2]);
            }
        
        } else if (lib_strcmp(ppcArgV[1], "delif") == 0) {
            iRet = netbr_delete_dev(ppcArgV[2], ppcArgV[3], 1);
            if (iRet) {
                fprintf(stderr, "can not delete if '%s' from bridge '%s'!\n", ppcArgV[3], ppcArgV[2]);
            }
        
        } else {
            goto    __arg_error;
        }
    
    } else {
        goto    __arg_error;
    }
    
    return  (iRet);
    
__arg_error:
    fprintf(stderr, "arguments error!\n");
    return  (-ERROR_TSHELL_EPARAM);
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** 函数名称: _netBridgeInit
** 功能描述: 安装 netbr 设备
** 输　入  : NONE
** 输　出  : 设备是否创建成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _netBridgeInit (VOID)
{
    if (_netBridgeDrvInit()) {
        return  (PX_ERROR);
    }
    
    if (_netBridgeDevCreate()) {
        return  (PX_ERROR);
    }
    
#if LW_CFG_SHELL_EN > 0
    API_TShellKeywordAdd("netbr", __tshellNetbr);
    API_TShellFormatAdd("netbr", " [...]");
    API_TShellHelpAdd("netbr",   "add / delete / control net bridge.\n"
                                 "eg. netbr addbr bridge0\n"
                                 "    netbr delbr bridge0\n"
                                 "    netbr flush bridge0\n"
                                 "    netbr adddev bridge0 ethdev0\n"
                                 "    netbr deldev bridge0 ethdev0\n"
                                 "    netbr addif bridge0 en1\n"
                                 "    netbr delif bridge0 en1\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_NET_DEV_BRIDGE_EN    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
