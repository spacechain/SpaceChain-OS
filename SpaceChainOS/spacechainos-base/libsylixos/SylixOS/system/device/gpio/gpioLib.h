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
** 文   件   名: gpioLib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 11 月 29 日
**
** 描        述: GPIO (通用输入/输出) 管脚操作模型.
*********************************************************************************************************/

#ifndef __GPIOLIB_H
#define __GPIOLIB_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_GPIO_EN > 0

/*********************************************************************************************************
  GPIO 控制器
  
  GC_pfuncRequest  
  表示请求一个 GPIO 如果驱动没有特殊操作, 例如电源管理等等, 则可以为 LW_NULL
  
  GC_pfuncFree
  释放一个正在被使用的 GPIO, 如果当前是中断模式则, 放弃中断输入功能.
  
  GC_pfuncGetDirection
  获得当前 GPIO 方向, 1 表示输出, 0 表示输入
  
  GC_pfuncDirectionInput
  设置 GPIO 为输入模式 (如果当前是中断模式则, 放弃中断输入功能)
  
  GC_pfuncGet
  获得 GPIO 输入值
  
  GC_pfuncDirectionOutput
  设置 GPIO 为输出模式 (如果当前是中断模式则, 放弃中断输入功能)
  
  GC_pfuncSetDebounce
  设置 GPIO 去抖动参数
  
  GC_pfuncSetPull
  设置 GPIO 控制器上下拉功能, 0: 开路 1: 上拉 pull up 2: 下拉 pull down
  
  GC_pfuncSet
  设置 GPIO 输出值
  
  GC_pfuncGetIrq
  获取 GPIO IRQ 向量号, 不设置 GPIO 中断, 仅仅获取 IRQ 号
  bIsLevel 1: 电平触发 0:边沿触发, uiType 1:上升沿触发 0:下降沿触发 2:双边沿触发
  
  GC_pfuncSetupIrq
  设置 GPIO 为外部中断输入口, 同时返回对应的 IRQ 向量号
  bIsLevel 1: 电平触发 0:边沿触发, uiType 1:上升沿触发 0:下降沿触发 2:双边沿触发
  
  GC_pfuncClearIrq
  GPIO 为外部中断输入模式时, 发生中断后, 在中断上下文中清除中断请求操作.
  
  GC_pfuncSvrIrq
  GPIO 产生外部中断时会调用此函数, 如果是本 GPIO 产生的中断, 则返回 LW_IRQ_HANDLED
  如果不是, 则返回 LW_IRQ_NONE.
  
  以上函数 uiOffset 参数为针对 GC_uiBase 的偏移量, GPIO 驱动程序需要通过此数值确定对应的硬件寄存器.
  注意: 相同条件下 GC_pfuncGetIrq, GC_pfuncSetupIrq 返回值必须相同.
*********************************************************************************************************/

struct lw_gpio_desc;
typedef struct lw_gpio_chip {
    CPCHAR                  GC_pcLabel;
    LW_LIST_LINE            GC_lineManage;
    ULONG                   GC_ulVerMagic;
#define LW_GPIO_VER_MAGIC   0xfffffff1
    
    INT                   (*GC_pfuncRequest)(struct lw_gpio_chip *pgchip, UINT uiOffset);
    VOID                  (*GC_pfuncFree)(struct lw_gpio_chip *pgchip, UINT uiOffset);
    INT                   (*GC_pfuncGetDirection)(struct lw_gpio_chip *pgchip, UINT uiOffset);
    INT                   (*GC_pfuncDirectionInput)(struct lw_gpio_chip *pgchip, UINT uiOffset);
    INT                   (*GC_pfuncGet)(struct lw_gpio_chip *pgchip, UINT uiOffset);
    INT                   (*GC_pfuncDirectionOutput)(struct lw_gpio_chip *pgchip, UINT uiOffset, 
                                                     INT iValue);
    INT                   (*GC_pfuncSetDebounce)(struct lw_gpio_chip *pgchip, UINT uiOffset, 
                                                 UINT uiDebounce);
    INT                   (*GC_pfuncSetPull)(struct lw_gpio_chip *pgchip, UINT uiOffset, UINT uiType);
    VOID                  (*GC_pfuncSet)(struct lw_gpio_chip *pgchip, UINT uiOffset, INT iValue);
    ULONG                 (*GC_pfuncGetIrq)(struct lw_gpio_chip *pgchip, UINT uiOffset,
                                            BOOL bIsLevel, UINT uiType);
    ULONG                 (*GC_pfuncSetupIrq)(struct lw_gpio_chip *pgchip, UINT uiOffset,
                                              BOOL bIsLevel, UINT uiType);
    VOID                  (*GC_pfuncClearIrq)(struct lw_gpio_chip *pgchip, UINT uiOffset);
    irqreturn_t           (*GC_pfuncSvrIrq)(struct lw_gpio_chip *pgchip, UINT uiOffset);
    
    UINT                    GC_uiBase;
    UINT                    GC_uiNGpios;
    struct lw_gpio_desc    *GC_gdDesc;
    
    ULONG                   GC_ulPad[16];                               /*  保留未来扩展                */
} LW_GPIO_CHIP;
typedef LW_GPIO_CHIP       *PLW_GPIO_CHIP;

/*********************************************************************************************************
  GPIO 管脚描述符 (内部使用)
*********************************************************************************************************/

typedef struct lw_gpio_desc {
    PLW_GPIO_CHIP           GD_pgcChip;
    ULONG                   GD_ulFlags;
#define LW_GPIODF_REQUESTED             0x0001
#define LW_GPIODF_IS_OUT                0x0002
#define LW_GPIODF_TRIG_FALL             0x0004
#define LW_GPIODF_TRIG_RISE             0x0008
#define LW_GPIODF_TRIG_LEVEL            0x0010
#define LW_GPIODF_OPEN_DRAIN            0x0020
#define LW_GPIODF_OPEN_SOURCE           0x0040

#define LW_GPIODF_ID_SHIFT              16
#define LW_GPIODF_MASK                  ((1 << ID_SHIFT) - 1)
#define LW_GPIODF_TRIGGER_MASK          (LW_GPIODF_TRIG_FALL | LW_GPIODF_TRIG_RISE)

    CPCHAR                  GD_pcLabel;
} LW_GPIO_DESC;
typedef LW_GPIO_DESC       *PLW_GPIO_DESC;

/*********************************************************************************************************
  GPIO 用户管脚描述
*********************************************************************************************************/

typedef struct lw_gpio {
    UINT                    G_ulGpio;
    ULONG                   G_ulFlags;
#define LW_GPIOF_DIR_OUT                (0 << 0)
#define LW_GPIOF_DIR_IN                 (1 << 0)

#define LW_GPIOF_INIT_LOW               (0 << 1)
#define LW_GPIOF_INIT_HIGH              (1 << 1)

#define LW_GPIOF_IN                     (LW_GPIOF_DIR_IN)
#define LW_GPIOF_OUT_INIT_LOW           (LW_GPIOF_DIR_OUT | LW_GPIOF_INIT_LOW)
#define LW_GPIOF_OUT_INIT_HIGH          (LW_GPIOF_DIR_OUT | LW_GPIOF_INIT_HIGH)

#define LW_GPIOF_OPEN_DRAIN             (1 << 2)
#define LW_GPIOF_OPEN_SOURCE            (1 << 3)
    
    CPCHAR                  G_pcLabel;
} LW_GPIO;
typedef LW_GPIO            *PLW_GPIO;

/*********************************************************************************************************
  GPIO API (以下 API 仅供驱动程序内部使用, 应用程序不可使用)
*********************************************************************************************************/

LW_API VOID             API_GpioInit(VOID);
LW_API INT              API_GpioChipAdd(PLW_GPIO_CHIP pgchip);
LW_API INT              API_GpioChipDelete(PLW_GPIO_CHIP pgchip);
LW_API PLW_GPIO_CHIP    API_GpioChipFind(PVOID pvData, 
                                         BOOL (*pfuncMatch)(PLW_GPIO_CHIP pgchip, 
                                                            PVOID  pvData));
LW_API INT              API_GpioIsValid(UINT uiGpio);
LW_API INT              API_GpioRequest(UINT uiGpio, CPCHAR pcLabel);
LW_API INT              API_GpioRequestOne(UINT uiGpio, ULONG ulFlags, CPCHAR pcLabel);
LW_API VOID             API_GpioFree(UINT uiGpio);
LW_API INT              API_GpioRequestArray(PLW_GPIO pgArray, size_t stNum);
LW_API VOID             API_GpioFreeArray(PLW_GPIO pgArray, size_t stNum);
LW_API INT              API_GpioGetFlags(UINT uiGpio, ULONG *pulFlags);
LW_API VOID             API_GpioOpenDrain(UINT uiGpio, BOOL bOpenDrain);
LW_API VOID             API_GpioOpenSource(UINT uiGpio, BOOL bOpenSource);
LW_API INT              API_GpioSetDebounce(UINT uiGpio, UINT uiDebounce);
LW_API INT              API_GpioSetPull(UINT uiGpio, UINT uiType);
LW_API INT              API_GpioDirectionInput(UINT uiGpio);
LW_API INT              API_GpioDirectionOutput(UINT uiGpio, INT iValue);
LW_API INT              API_GpioGetValue(UINT uiGpio);
LW_API VOID             API_GpioSetValue(UINT uiGpio, INT iValue);
LW_API ULONG            API_GpioGetIrq(UINT uiGpio, BOOL bIsLevel, UINT uiType);
LW_API ULONG            API_GpioSetupIrq(UINT uiGpio, BOOL bIsLevel, UINT uiType);
LW_API VOID             API_GpioClearIrq(UINT uiGpio);
LW_API irqreturn_t      API_GpioSvrIrq(UINT uiGpio);

#define gpioInit                API_GpioInit
#define gpioChipAdd             API_GpioChipAdd
#define gpioChipDelete          API_GpioChipDelete
#define gpioChipFind            API_GpioChipFind
#define gpioIsValid             API_GpioIsValid
#define gpioRequest             API_GpioRequest
#define gpioRequestOne          API_GpioRequestOne
#define gpioFree                API_GpioFree
#define gpioRequestArray        API_GpioRequestArray
#define gpioFreeArray           API_GpioFreeArray
#define gpioGetFlags            API_GpioGetFlags
#define gpioOpenDrain           API_GpioOpenDrain
#define gpioOpenSource          API_GpioOpenSource
#define gpioSetDebounce         API_GpioSetDebounce
#define gpioSetPull             API_GpioSetPull
#define gpioDirectionInput      API_GpioDirectionInput
#define gpioDirectionOutput     API_GpioDirectionOutput
#define gpioGetValue            API_GpioGetValue
#define gpioSetValue            API_GpioSetValue
#define gpioGetIrq              API_GpioGetIrq
#define gpioSetupIrq            API_GpioSetupIrq
#define gpioClearIrq            API_GpioClearIrq
#define gpioSvrIrq              API_GpioSvrIrq

#endif                                                                  /*  LW_CFG_GPIO_EN > 0          */
#endif                                                                  /*  __GPIOLIB_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
