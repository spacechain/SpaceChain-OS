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
** 文   件   名: selectTy.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 03 月 01 日
**
** 描        述:  IO 系统 select 子系统 TY 设备操作库函数.

** BUG
2008.05.31  将参数改为 long 型, 支持 64 位系统.
2008.07.19  解决了 PTY 发送时, 发送状态对 select() 的 WAIT READ 的影响.
2009.12.16  SELREAD 时检测主机是否有数据需要发送, 仅检测缓冲区是否有数据即可.
            不用判断 TYDEVWRSTAT_bCR, 这个一定是 \n 产生的. 产生 \n 时一定会激活 select() 然后连续读时可以
            保证可读出, (但是目前还保留判断!)
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SIO_DEVICE_EN > 0) && (LW_CFG_PTY_DEVICE_EN > 0)
#include "../SylixOS/system/device/pty/ptyLib.h"
/*********************************************************************************************************
** 函数名称: __selPtyAdd 
** 功能描述: 向指定的 pty wake up list 添加一个等待节点. (ioctl FIOSELECT 使用)
** 输　入  : p_ptyddev          Ty 虚拟设备控制块
             lArg               select wake up node 控制结构
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID    __selPtyAdd (P_PTY_D_DEV   p_ptyddev, LONG  lArg)
{
    REGISTER TY_DEV_ID            ptyDev;
    REGISTER PLW_SEL_WAKEUPNODE   pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
    REGISTER P_PTY_H_DEV          p_ptyhdev   = (P_PTY_H_DEV)_LIST_ENTRY(p_ptyddev, 
                                                PTY_DEV, 
                                                PTYDEV_ptyddev);        /*  获得 HOST 端口              */
                                                     
    ptyDev = &p_ptyhdev->PTYHDEV_tydevTyDev;
    
    SEL_WAKE_NODE_ADD(&p_ptyddev->PTYDDEV_selwulList, pselwunNode);     /*  添加节点                    */
    
    switch (pselwunNode->SELWUN_seltypType) {
    
    case SELREAD:                                                       /*  等待数据可读                */
        if ((rngNBytes(ptyDev->TYDEV_vxringidWrBuf) > 0) ||
            (ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bCR)) {
            SEL_WAKE_UP(pselwunNode);                                   /*  检测主机是否有数据需要发送  */
        } else {
            ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy = LW_FALSE;     /*  确保可以被 Startup 激活     */
        }
        break;
    
    case SELWRITE:                                                      /*  等待数据可写                */
        if (rngFreeBytes(ptyDev->TYDEV_vxringidRdBuf) > 0) {            /*  检测主机接收缓存是否有空间  */
            SEL_WAKE_UP(pselwunNode);                                   /*  唤醒节点                    */
        }
        break;
        
    case SELEXCEPT:                                                     /*  等待设备异常                */
        if ((LW_DEV_GET_USE_COUNT(&p_ptyddev->PTYDDEV_devhdrDevice) == 0) ||
            (LW_DEV_GET_USE_COUNT(&p_ptyhdev->PTYHDEV_tydevTyDev.TYDEV_devhdrHdr)
             == 0)) {                                                   /*  设备关闭了                  */
            SEL_WAKE_UP(pselwunNode);                                   /*  唤醒节点                    */
        }
        break;
    }
}
/*********************************************************************************************************
** 函数名称: __selPtyDelete
** 功能描述: 从指定的 Pty wake up list 删除等待节点. (ioctl FIOUNSELECT 使用)
** 输　入  : ptyDev             Ty 虚拟设备控制块
             lArg               select wake up node 控制结构
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID    __selPtyDelete (P_PTY_D_DEV   p_ptyddev, LONG  lArg)
{
    SEL_WAKE_NODE_DELETE(&p_ptyddev->PTYDDEV_selwulList, (PLW_SEL_WAKEUPNODE)lArg);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SIO_DEVICE_EN > 0)  */
                                                                        /*  (LW_CFG_PTY_DEVICE_EN > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
