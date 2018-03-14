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
** 文   件   名: i2cDev.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 10 月 20 日
**
** 描        述: i2c 总线挂载的设备结构.
*********************************************************************************************************/

#ifndef __I2CDEV_H
#define __I2CDEV_H

#include "i2cBus.h"                                                     /*  i2c 总线模型                */

/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

/*********************************************************************************************************
  I2C 设备类型
*********************************************************************************************************/

typedef struct lw_i2c_device {
    UINT16                       I2CDEV_usAddr;                         /*  设备地址                    */
    UINT16                       I2CDEV_usFlag;                         /*  标志, 仅支持 10bit 地址选项 */
    
#define LW_I2C_CLIENT_TEN        0x10                                   /*  与 LW_I2C_M_TEN 相同        */
    
    PLW_I2C_ADAPTER              I2CDEV_pi2cadapter;                    /*  挂载的适配器                */
    LW_LIST_LINE                 I2CDEV_lineManage;                     /*  设备挂载链                  */
    atomic_t                     I2CDEV_atomicUsageCnt;                 /*  设备使用计数                */
    CHAR                         I2CDEV_cName[LW_CFG_OBJECT_NAME_SIZE]; /*  设备的名称                  */
} LW_I2C_DEVICE;
typedef LW_I2C_DEVICE           *PLW_I2C_DEVICE;

/*********************************************************************************************************
  驱动程序可以使用的 API
  以下 API 只能在驱动程序中使用, 应用程序仅仅可以看到已经过 io 系统的 i2c 设备.
*********************************************************************************************************/

LW_API INT                   API_I2cLibInit(VOID);

/*********************************************************************************************************
  I2C 适配器基本操作
*********************************************************************************************************/

LW_API INT                   API_I2cAdapterCreate(CPCHAR           pcName, 
                                                  PLW_I2C_FUNCS    pi2cfunc,
                                                  ULONG            ulTimeout,
                                                  INT              iRetry);
LW_API INT                   API_I2cAdapterDelete(CPCHAR  pcName);
LW_API PLW_I2C_ADAPTER       API_I2cAdapterGet(CPCHAR  pcName);

/*********************************************************************************************************
  I2C 设备基本操作
*********************************************************************************************************/

LW_API PLW_I2C_DEVICE        API_I2cDeviceCreate(CPCHAR  pcAdapterName,
                                                 CPCHAR  pcDeviceName,
                                                 UINT16  usAddr,
                                                 UINT16  usFlag);
LW_API INT                   API_I2cDeviceDelete(PLW_I2C_DEVICE   pi2cdevice);

LW_API INT                   API_I2cDeviceUsageInc(PLW_I2C_DEVICE   pi2cdevice);
LW_API INT                   API_I2cDeviceUsageDec(PLW_I2C_DEVICE   pi2cdevice);
LW_API INT                   API_I2cDeviceUsageGet(PLW_I2C_DEVICE   pi2cdevice);

/*********************************************************************************************************
  I2C 设备传输控制操作
*********************************************************************************************************/

LW_API INT                   API_I2cDeviceTransfer(PLW_I2C_DEVICE   pi2cdevice, 
                                                   PLW_I2C_MESSAGE  pi2cmsg,
                                                   INT              iNum);
LW_API INT                   API_I2cDeviceMasterSend(PLW_I2C_DEVICE   pi2cdevice,
                                                     CPCHAR           pcBuffer,
                                                     INT              iCount);
LW_API INT                   API_I2cDeviceMasterRecv(PLW_I2C_DEVICE   pi2cdevice,
                                                     PCHAR            pcBuffer,
                                                     INT              iCount);
LW_API INT                   API_I2cDeviceCtl(PLW_I2C_DEVICE   pi2cdevice, INT  iCmd, LONG  lArg);

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
#endif                                                                  /*  __I2CDEV_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
