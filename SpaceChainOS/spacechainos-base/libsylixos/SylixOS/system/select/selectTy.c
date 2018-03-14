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
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SIO_DEVICE_EN > 0)
/*********************************************************************************************************
** 函数名称: __selTyAdd 
** 功能描述: 向指定的 Ty wake up list 添加一个等待节点. (ioctl FIOSELECT 使用)
** 输　入  : ptyDev             Ty 设备.
             lArg               select wake up node 控制结构
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID    __selTyAdd (TY_DEV_ID   ptyDev, LONG  lArg)
{
    REGISTER PLW_SEL_WAKEUPNODE   pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
    
    SEL_WAKE_NODE_ADD(&ptyDev->TYDEV_selwulList, pselwunNode);          /*  添加节点                    */
    
    switch (pselwunNode->SELWUN_seltypType) {
    
    case SELREAD:                                                       /*  等待数据可读                */
        if (rngNBytes(ptyDev->TYDEV_vxringidRdBuf) > 0) {               /*  有数据可读                  */
            SEL_WAKE_UP(pselwunNode);                                   /*  唤醒节点                    */
        }
        break;
        
    case SELWRITE:                                                      /*  等待数据可写                */
        if (rngFreeBytes(ptyDev->TYDEV_vxringidWrBuf) > 0) {            /*  有空间可写                  */
            SEL_WAKE_UP(pselwunNode);                                   /*  唤醒节点                    */
        }
        break;
        
    case SELEXCEPT:                                                     /*  等待设备异常                */
        if (LW_DEV_GET_USE_COUNT(&ptyDev->TYDEV_devhdrHdr) == 0) {      /*  设备关闭了                  */
            SEL_WAKE_UP(pselwunNode);                                   /*  唤醒节点                    */
        }
        break;
    }
}
/*********************************************************************************************************
** 函数名称: __selTyDelete
** 功能描述: 从指定的 Ty wake up list 删除等待节点. (ioctl FIOUNSELECT 使用)
** 输　入  : ptyDev             Ty 设备.
             lArg               select wake up node 控制结构
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID    __selTyDelete (TY_DEV_ID   ptyDev, LONG  lArg)
{
    SEL_WAKE_NODE_DELETE(&ptyDev->TYDEV_selwulList, (PLW_SEL_WAKEUPNODE)lArg);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SIO_DEVICE_EN > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
