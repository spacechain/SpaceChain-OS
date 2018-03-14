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
** 文   件   名: gpioLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 11 月 29 日
**
** 描        述: GPIO (通用输入/输出) 管脚操作模型.
**
** BUG:
2014.05.25  加入 get flags 操作.
2017.12.29  加入 API_GpioGetIrq() 解决多个 GPIO 共享一路中断的竞争风险.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_GPIO_EN > 0
/*********************************************************************************************************
  内部变量
*********************************************************************************************************/
static LW_LIST_LINE_HEADER          _G_plineGpioChips;                  /*  驱动程序链表                */
static LW_GPIO_DESC                 _G_gdesc[LW_CFG_MAX_GPIOS];         /*  每个管脚描述符              */
static LW_SPINLOCK_DEFINE_CACHE_ALIGN(_G_slGpio);
/*********************************************************************************************************
  内部操作宏
*********************************************************************************************************/
#define GPIO_LOCK(pintreg)          LW_SPIN_LOCK_QUICK(&_G_slGpio, (pintreg))
#define GPIO_UNLOCK(intreg)         LW_SPIN_UNLOCK_QUICK(&_G_slGpio, (intreg))

#define GPIO_CHIP_HWGPIO(pgdesc)    ((pgdesc) - &(pgdesc)->GD_pgcChip->GC_gdDesc[0])
#define GPIO_TO_DESC(gpio)          (&_G_gdesc[(gpio)])
#define DESC_TO_GPIO(pgdesc)        ((pgdesc)->GD_pgcChip->GC_uiBase + GPIO_CHIP_HWGPIO(pgdesc))

#define GPIO_IS_VALID(gpio)         ((gpio) < LW_CFG_MAX_GPIOS)
/*********************************************************************************************************
** 函数名称: __gpioGetDirection
** 功能描述: 获得指定 GPIO 方向
** 输　入  : pgdesc    GPIO 管脚描述符
** 输　出  : 0: 输入 1: 输出
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __gpioGetDirection (PLW_GPIO_DESC pgdesc)
{
    INT            iError;
    PLW_GPIO_CHIP  pgchip = pgdesc->GD_pgcChip;
    
    if (!pgchip->GC_pfuncGetDirection) {
        return  (PX_ERROR);
    }
    
    iError = pgchip->GC_pfuncGetDirection(pgchip, GPIO_CHIP_HWGPIO(pgdesc));
    if (iError > 0) {
        pgdesc->GD_ulFlags |= LW_GPIODF_IS_OUT;
        return  (1);
    
    } else if (iError == 0) {
        pgdesc->GD_ulFlags &= ~LW_GPIODF_IS_OUT;
        return  (0);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __gpioGetDesc
** 功能描述: 通过 GPIO 号获得指定 GPIO 描述符
** 输　入  : uiGpio        GPIO 号
**           bRequested    是否检查请求
** 输　出  : GPIO 管脚描述符
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_GPIO_DESC  __gpioGetDesc (UINT  uiGpio, BOOL  bRequested)
{
    PLW_GPIO_DESC   pgdesc;
    
    if (!GPIO_IS_VALID(uiGpio)) {
        return  (LW_NULL);
    }
    
    pgdesc = GPIO_TO_DESC(uiGpio);
    if (bRequested && !(pgdesc->GD_ulFlags & LW_GPIODF_REQUESTED)) {
        return  (LW_NULL);
    }
    
    return  (pgdesc);
}
/*********************************************************************************************************
** 函数名称: __gpioSetValueOpenDrain
** 功能描述: 设置一个 OPEN DRAIN 属性的 GPIO
** 输　入  : pgdesc    GPIO 管脚描述符
**           iValue    设置的值
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __gpioSetValueOpenDrain (PLW_GPIO_DESC pgdesc, INT iValue)
{
    INT             iError;
    PLW_GPIO_CHIP   pgchip = pgdesc->GD_pgcChip;
    
    if (iValue) {
        iError = pgchip->GC_pfuncDirectionInput(pgchip, GPIO_CHIP_HWGPIO(pgdesc));
        if (iError == ERROR_NONE) {
            pgdesc->GD_ulFlags &= ~LW_GPIODF_IS_OUT;
        }
    
    } else {
        iError = pgchip->GC_pfuncDirectionOutput(pgchip, GPIO_CHIP_HWGPIO(pgdesc), 0);
        if (iError == ERROR_NONE) {
            pgdesc->GD_ulFlags |= LW_GPIODF_IS_OUT;
        }
    }
}
/*********************************************************************************************************
** 函数名称: __gpioSetValueOpenSource
** 功能描述: 设置一个 OPEN SOURCE 属性的 GPIO
** 输　入  : pgdesc    GPIO 管脚描述符
**           iValue    设置的值
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __gpioSetValueOpenSource (PLW_GPIO_DESC pgdesc, INT iValue)
{
    INT             iError;
    PLW_GPIO_CHIP   pgchip = pgdesc->GD_pgcChip;
    
    if (iValue) {
        iError = pgchip->GC_pfuncDirectionOutput(pgchip, GPIO_CHIP_HWGPIO(pgdesc), 1);
        if (iError == ERROR_NONE) {
            pgdesc->GD_ulFlags |= LW_GPIODF_IS_OUT;
        }
    
    } else {
        iError = pgchip->GC_pfuncDirectionInput(pgchip, GPIO_CHIP_HWGPIO(pgdesc));
        if (iError == ERROR_NONE) {
            pgdesc->GD_ulFlags &= ~LW_GPIODF_IS_OUT;
        }
    }
}
/*********************************************************************************************************
** 函数名称: _GpioInit
** 功能描述: 初始化 GPIO 库
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_GpioInit (VOID)
{
    LW_SPIN_INIT(&_G_slGpio);
}
/*********************************************************************************************************
** 函数名称: API_GpioChipAdd
** 功能描述: 加入一个 GPIO 芯片驱动
** 输　入  : pgchip    GPIO 驱动
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GpioChipAdd (PLW_GPIO_CHIP pgchip)
{
    INTREG         iregInterLevel;
    UINT           i;
    PLW_LIST_LINE  plineTemp;
    PLW_LIST_LINE  plinePrev;
    PLW_GPIO_CHIP  pgchipTemp;
    
    if (!pgchip) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pgchip->GC_ulVerMagic != LW_GPIO_VER_MAGIC) {                   /*  驱动不匹配                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, 
                     "GPIO driver version not matching to current system.\r\n");
        _ErrorHandle(EFTYPE);
        return  (PX_ERROR);
    }
    
    if ((!GPIO_IS_VALID(pgchip->GC_uiBase)) || 
        (!GPIO_IS_VALID(pgchip->GC_uiBase + pgchip->GC_uiNGpios - 1))) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    GPIO_LOCK(&iregInterLevel);                                         /*  锁定 GPIO                   */
    for (plineTemp  = _G_plineGpioChips;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pgchipTemp = _LIST_ENTRY(plineTemp, LW_GPIO_CHIP, GC_lineManage);
        if (pgchipTemp->GC_uiBase >= (pgchip->GC_uiBase + pgchip->GC_uiNGpios)) {
            break;
        }
    }
    if (plineTemp) {
        plinePrev = _list_line_get_prev(plineTemp);
        if (plinePrev) {
            pgchipTemp = _LIST_ENTRY(plinePrev, LW_GPIO_CHIP, GC_lineManage);
            if ((pgchipTemp->GC_uiBase + pgchipTemp->GC_uiNGpios) > pgchip->GC_uiBase) {
                GPIO_UNLOCK(iregInterLevel);                            /*  解锁 GPIO                   */
                _DebugHandle(__ERRORMESSAGE_LEVEL, 
                             "GPIO integer space overlap, cannot add chip.\r\n");
                _ErrorHandle(EBUSY);
                return  (PX_ERROR);
            }
        }
        _List_Line_Add_Left(&pgchip->GC_lineManage, plineTemp);
    
    } else if (_G_plineGpioChips) {                                     /*  新加入点 base 最大          */
        _List_Line_Add_Right(&pgchip->GC_lineManage, &pgchipTemp->GC_lineManage);
    
    } else {
        _List_Line_Add_Ahead(&pgchip->GC_lineManage, &_G_plineGpioChips);
    }
    
    pgchip->GC_gdDesc = &_G_gdesc[pgchip->GC_uiBase];
    for (i = 0; i < pgchip->GC_uiNGpios; i++) {
        PLW_GPIO_DESC pgdesc = &pgchip->GC_gdDesc[i];
        pgdesc->GD_pgcChip = pgchip;
        
        if (pgchip->GC_pfuncDirectionInput == LW_NULL) {                /*  没有输入功能                */
            pgdesc->GD_ulFlags = LW_GPIODF_IS_OUT;
        
        } else {
            pgdesc->GD_ulFlags = 0ul;                                   /*  默认为输入态(芯片都这样设计)*/
        }
    }
    GPIO_UNLOCK(iregInterLevel);                                        /*  解锁 GPIO                   */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_GpioChipDelete
** 功能描述: 删除一个 GPIO 芯片驱动
** 输　入  : pgchip    GPIO 驱动
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GpioChipDelete (PLW_GPIO_CHIP pgchip)
{
    INTREG  iregInterLevel;
    UINT    i;

    if (!pgchip) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    GPIO_LOCK(&iregInterLevel);                                         /*  锁定 GPIO                   */
    for (i = 0; i < pgchip->GC_uiNGpios; i++) {
        if (pgchip->GC_gdDesc[i].GD_ulFlags & LW_GPIODF_REQUESTED) {
            GPIO_UNLOCK(iregInterLevel);                                /*  解锁 GPIO                   */
            _ErrorHandle(EBUSY);
            return  (PX_ERROR);
        }
    }
    
    for (i = 0; i < pgchip->GC_uiNGpios; i++) {
        pgchip->GC_gdDesc[i].GD_pgcChip = LW_NULL;
    }
    
    _List_Line_Del(&pgchip->GC_lineManage, &_G_plineGpioChips);
    GPIO_UNLOCK(iregInterLevel);                                        /*  解锁 GPIO                   */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_GpioChipFind
** 功能描述: 查询一个 GPIO 芯片驱动
** 输　入  : pvData        匹配函数参数
**           pfuncMatch    查询匹配函数 (此函数返回 LW_TRUE 表示找到)
** 输　出  : 查询到的驱动结构
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PLW_GPIO_CHIP  API_GpioChipFind (PVOID pvData, BOOL (*pfuncMatch)(PLW_GPIO_CHIP pgchip, PVOID  pvData))
{
    INTREG         iregInterLevel;
    PLW_GPIO_CHIP  pgchip;
    PLW_LIST_LINE  plineTemp;
    
    if (!pfuncMatch) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    GPIO_LOCK(&iregInterLevel);                                         /*  锁定 GPIO                   */
    for (plineTemp  = _G_plineGpioChips;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pgchip = _LIST_ENTRY(plineTemp, LW_GPIO_CHIP, GC_lineManage);
        if (pfuncMatch(pgchip, pvData)) {
            GPIO_UNLOCK(iregInterLevel);                                /*  解锁 GPIO                   */
            return  (pgchip);
        }
    }
    GPIO_UNLOCK(iregInterLevel);                                        /*  解锁 GPIO                   */
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_GpioIsValid
** 功能描述: GPIO 号是否有效
** 输　入  : uiGpio        GPIO 号
** 输　出  : 1: 有效 0:无效
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GpioIsValid (UINT uiGpio)
{
    return  (GPIO_IS_VALID(uiGpio));
}
/*********************************************************************************************************
** 函数名称: API_GpioRequest
** 功能描述: 请求使用一个 GPIO
** 输　入  : uiGpio        请求 GPIO 号
**           pcLabel       标签
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GpioRequest (UINT uiGpio, CPCHAR pcLabel)
{
    INTREG          iregInterLevel;
    INT             iError;
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;
    
    if (!GPIO_IS_VALID(uiGpio)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pgdesc = GPIO_TO_DESC(uiGpio);
    
    GPIO_LOCK(&iregInterLevel);                                         /*  锁定 GPIO                   */
    pgchip = pgdesc->GD_pgcChip;
    if (!pgchip) {
        GPIO_UNLOCK(iregInterLevel);                                    /*  解锁 GPIO                   */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pgdesc->GD_ulFlags & LW_GPIODF_REQUESTED) {                     /*  已经在使用中                */
        GPIO_UNLOCK(iregInterLevel);                                    /*  解锁 GPIO                   */
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }
    
    pgdesc->GD_ulFlags |= LW_GPIODF_REQUESTED;
    pgdesc->GD_pcLabel  = pcLabel ? pcLabel : "?";
    
    if (pgchip->GC_pfuncRequest) {
        GPIO_UNLOCK(iregInterLevel);                                    /*  解锁 GPIO                   */
        iError = pgchip->GC_pfuncRequest(pgchip, GPIO_CHIP_HWGPIO(pgdesc));
        GPIO_LOCK(&iregInterLevel);                                     /*  锁定 GPIO                   */
        
        if (iError < ERROR_NONE) {
            pgdesc->GD_ulFlags &= ~LW_GPIODF_REQUESTED;
            pgdesc->GD_pcLabel  = LW_NULL;
            GPIO_UNLOCK(iregInterLevel);                                /*  解锁 GPIO                   */
            return  (iError);
        }
    }
    
    if (pgchip->GC_pfuncGetDirection) {
        GPIO_UNLOCK(iregInterLevel);                                    /*  解锁 GPIO                   */
        __gpioGetDirection(pgdesc);
    
    } else {
        GPIO_UNLOCK(iregInterLevel);                                    /*  解锁 GPIO                   */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_GpioRequestOne
** 功能描述: 请求使用一个 GPIO
** 输　入  : uiGpio        请求 GPIO 号
**           ulFlags       需要设置的属性
**           pcLabel       标签
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GpioRequestOne (UINT uiGpio, ULONG ulFlags, CPCHAR pcLabel)
{
    INT             iError;
    PLW_GPIO_DESC   pgdesc;
    
    iError = API_GpioRequest(uiGpio, pcLabel);
    if (iError < ERROR_NONE) {
        return  (iError);
    }
    
    pgdesc = GPIO_TO_DESC(uiGpio);
    
    if (ulFlags & LW_GPIOF_OPEN_DRAIN) {
        pgdesc->GD_ulFlags |= LW_GPIODF_OPEN_DRAIN;
    }
    
    if (ulFlags & LW_GPIOF_OPEN_SOURCE) {
        pgdesc->GD_ulFlags |= LW_GPIODF_OPEN_SOURCE;
    }
    
    if (ulFlags & LW_GPIOF_DIR_IN) {
        iError = API_GpioDirectionInput(uiGpio);
    
    } else {
        iError = API_GpioDirectionOutput(uiGpio, (ulFlags & LW_GPIOF_INIT_HIGH) ? 1 : 0);
    }
    
    if (iError < ERROR_NONE) {
        API_GpioFree(uiGpio);
        return  (iError);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_GpioFree
** 功能描述: 释放使用一个 GPIO
** 输　入  : uiGpio        GPIO 号
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_GpioFree (UINT uiGpio)
{
    INTREG          iregInterLevel;
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;
    
    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        return;
    }
    
    GPIO_LOCK(&iregInterLevel);                                         /*  锁定 GPIO                   */
    pgchip = pgdesc->GD_pgcChip;
    if (pgchip && (pgdesc->GD_ulFlags & LW_GPIODF_REQUESTED)) {
        if (pgchip->GC_pfuncFree) {
            GPIO_UNLOCK(iregInterLevel);                                /*  解锁 GPIO                   */
            pgchip->GC_pfuncFree(pgchip, GPIO_CHIP_HWGPIO(pgdesc));
            GPIO_LOCK(&iregInterLevel);                                 /*  锁定 GPIO                   */
        }
        
        pgdesc->GD_ulFlags &= ~(LW_GPIODF_REQUESTED | LW_GPIODF_TRIGGER_MASK);
        pgdesc->GD_pcLabel  = LW_NULL;
    }
    GPIO_UNLOCK(iregInterLevel);                                        /*  解锁 GPIO                   */
}
/*********************************************************************************************************
** 函数名称: API_GpioRequestArray
** 功能描述: 请求使用一组 GPIO
** 输　入  : pgArray       GPIO 数组
**           stNum         数组元素个数
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GpioRequestArray (PLW_GPIO pgArray, size_t stNum)
{
    INT     i;
    INT     iError;
    
    if (!pgArray || !stNum) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    for (i = 0; i < stNum; i++, pgArray++) {
        iError = API_GpioRequestOne(pgArray->G_ulGpio, pgArray->G_ulFlags, pgArray->G_pcLabel);
        if (iError < ERROR_NONE) {
            goto    __error_handle;
        }
    }
    
    return  (ERROR_NONE);
    
__error_handle:
    while (i) {
        pgArray--;
        i--;
        API_GpioFree(pgArray->G_ulGpio);
    }
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_GpioFreeArray
** 功能描述: 释放一组 GPIO
** 输　入  : pgArray       GPIO 数组
**           stNum         数组元素个数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_GpioFreeArray (PLW_GPIO pgArray, size_t stNum)
{
    if (!pgArray) {
        return;
    }

    while (stNum) {
        API_GpioFree(pgArray->G_ulGpio);
        pgArray++;
        stNum--;
    }
}
/*********************************************************************************************************
** 函数名称: API_GpioGetFlags
** 功能描述: 获取一个 GPIO flags
** 输　入  : uiGpio        请求 GPIO 号
**           pulFlags      获取的 flags
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GpioGetFlags (UINT uiGpio, ULONG *pulFlags)
{
    PLW_GPIO_DESC   pgdesc;

    if (!pulFlags) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    *pulFlags = pgdesc->GD_ulFlags;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_GpioOpenDrain
** 功能描述: 设置一个 GPIO OPEN DRAIN 参数
** 输　入  : uiGpio        请求 GPIO 号
**           bOpenDrain    参数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_GpioOpenDrain (UINT uiGpio, BOOL bOpenDrain)
{
    PLW_GPIO_DESC   pgdesc;

    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        return;
    }
    
    if (bOpenDrain) {
        pgdesc->GD_ulFlags |= LW_GPIODF_OPEN_DRAIN;
    
    } else {
        pgdesc->GD_ulFlags &= ~LW_GPIODF_OPEN_DRAIN;
    }
}
/*********************************************************************************************************
** 函数名称: API_GpioOpenSource
** 功能描述: 设置一个 GPIO OPEN SOURCE 参数
** 输　入  : uiGpio        请求 GPIO 号
**           bOpenSource   参数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_GpioOpenSource (UINT uiGpio, BOOL bOpenSource)
{
    PLW_GPIO_DESC   pgdesc;

    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        return;
    }
    
    if (bOpenSource) {
        pgdesc->GD_ulFlags |= LW_GPIODF_OPEN_SOURCE;
    
    } else {
        pgdesc->GD_ulFlags &= ~LW_GPIODF_OPEN_SOURCE;
    }
}
/*********************************************************************************************************
** 函数名称: API_GpioSetDebounce
** 功能描述: 设置指定 GPIO 去抖动时间参数
** 输　入  : uiGpio        GPIO 号
**           uiDebounce    去抖动时间参数
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GpioSetDebounce (UINT uiGpio, UINT uiDebounce)
{
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;

    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pgchip = pgdesc->GD_pgcChip;
    if (!pgchip || !pgchip->GC_pfuncSetDebounce) {
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    return  (pgchip->GC_pfuncSetDebounce(pgchip, GPIO_CHIP_HWGPIO(pgdesc), uiDebounce));
}
/*********************************************************************************************************
** 函数名称: API_GpioSetPull
** 功能描述: 设置指定 GPIO 上下拉参数
** 输　入  : uiGpio        GPIO 号
**           uiType        上下拉参数 0: 开路 1: 上拉 pull up 2: 下拉 pull down
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GpioSetPull (UINT uiGpio, UINT uiType)
{
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;

    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pgchip = pgdesc->GD_pgcChip;
    if (!pgchip || !pgchip->GC_pfuncSetPull) {
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    return  (pgchip->GC_pfuncSetPull(pgchip, GPIO_CHIP_HWGPIO(pgdesc), uiType));
}
/*********************************************************************************************************
** 函数名称: API_GpioDirectionInput
** 功能描述: 设置指定 GPIO 为输入模式
** 输　入  : uiGpio        GPIO 号
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GpioDirectionInput (UINT uiGpio)
{
    INT             iError;
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;

    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pgchip = pgdesc->GD_pgcChip;
    if (!pgchip || !pgchip->GC_pfuncGet || !pgchip->GC_pfuncDirectionInput) {
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    iError = pgchip->GC_pfuncDirectionInput(pgchip, GPIO_CHIP_HWGPIO(pgdesc));
    if (iError == ERROR_NONE) {
        pgdesc->GD_ulFlags &= ~LW_GPIODF_IS_OUT;
    }
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_GpioDirectionOutput
** 功能描述: 设置指定 GPIO 为输出模式
** 输　入  : uiGpio        GPIO 号
**           iValue        1: 高电平 0: 低电平
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GpioDirectionOutput (UINT uiGpio, INT iValue)
{
    INT             iError;
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;

    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pgchip = pgdesc->GD_pgcChip;
    if (!pgchip || !pgchip->GC_pfuncSet || !pgchip->GC_pfuncDirectionOutput) {
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    iError = pgchip->GC_pfuncDirectionOutput(pgchip, GPIO_CHIP_HWGPIO(pgdesc), iValue);
    if (iError == ERROR_NONE) {
        pgdesc->GD_ulFlags |= LW_GPIODF_IS_OUT;
    }
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_GpioGetValue
** 功能描述: 获取指定 GPIO 的值
** 输　入  : uiGpio        GPIO 号
** 输　出  : GPIO 当前电平状态 1: 高电平 0: 低电平
** 全局变量: 
** 调用模块: 
** 注  意  : 此函数不做任何参数有效性判断, 所以用户必须保证 uiGpio 已经请求成功, 方向与其他参数设置正确.

                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GpioGetValue (UINT uiGpio)
{
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;
    
    pgdesc = GPIO_TO_DESC(uiGpio);                                      /*  不做任何检查, 加快速度      */
    pgchip = pgdesc->GD_pgcChip;
    
    return  (pgchip->GC_pfuncGet(pgchip, GPIO_CHIP_HWGPIO(pgdesc)));
}
/*********************************************************************************************************
** 函数名称: API_GpioSetValue
** 功能描述: 设置指定 GPIO 的值
** 输　入  : uiGpio        GPIO 号
**           iValue        1: 高电平 0: 低电平
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 此函数不做任何参数有效性判断, 所以用户必须保证 uiGpio 已经请求成功, 方向与其他参数设置正确.

                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_GpioSetValue (UINT uiGpio, INT iValue)
{
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;
    
    pgdesc = GPIO_TO_DESC(uiGpio);                                      /*  不做任何检查, 加快速度      */
    pgchip = pgdesc->GD_pgcChip;
    
    if (pgdesc->GD_ulFlags & LW_GPIODF_OPEN_DRAIN) {
        __gpioSetValueOpenDrain(pgdesc, iValue);
    
    } else if (pgdesc->GD_ulFlags & LW_GPIODF_OPEN_SOURCE) {
        __gpioSetValueOpenSource(pgdesc, iValue);
    
    } else {
        pgchip->GC_pfuncSet(pgchip, GPIO_CHIP_HWGPIO(pgdesc), iValue);
    }
}
/*********************************************************************************************************
** 函数名称: API_GpioGetIrq
** 功能描述: 根据指定 GPIO 号返回对应的 IRQ 号
** 输　入  : uiGpio        GPIO 号
**           bIsLevel      是否为电平触发
**           uiType        如果为电平触发, 1 表示高电平触发, 0 表示低电平触发
**                         如果为边沿触发, 1 表示上升沿触发, 0 表示下降沿触发, 2 表示双边沿触发
** 输　出  : IRQ 号, 错误返回 LW_VECTOR_INVALID
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_GpioGetIrq (UINT uiGpio, BOOL bIsLevel, UINT uiType)
{
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;

    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        _ErrorHandle(EINVAL);
        return  (LW_VECTOR_INVALID);
    }
    
    pgchip = pgdesc->GD_pgcChip;
    
    if (pgchip->GC_pfuncGetIrq) {
        return  (pgchip->GC_pfuncGetIrq(pgchip, GPIO_CHIP_HWGPIO(pgdesc), bIsLevel, uiType));
    
    } else {
        _ErrorHandle(ENXIO);
        return  (LW_VECTOR_INVALID);
    }
}
/*********************************************************************************************************
** 函数名称: API_GpioSetupIrq
** 功能描述: 根据指定 GPIO 号设置相应的外部中断, 并返回对应的 IRQ 号
** 输　入  : uiGpio        GPIO 号
**           bIsLevel      是否为电平触发
**           uiType        如果为电平触发, 1 表示高电平触发, 0 表示低电平触发
**                         如果为边沿触发, 1 表示上升沿触发, 0 表示下降沿触发, 2 表示双边沿触发
** 输　出  : IRQ 号, 错误返回 LW_VECTOR_INVALID
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_GpioSetupIrq (UINT uiGpio, BOOL bIsLevel, UINT uiType)
{
    ULONG           ulVector;
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;

    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        _ErrorHandle(EINVAL);
        return  (LW_VECTOR_INVALID);
    }
    
    pgchip = pgdesc->GD_pgcChip;
    
    if (pgchip->GC_pfuncSetupIrq) {
        ulVector = pgchip->GC_pfuncSetupIrq(pgchip, GPIO_CHIP_HWGPIO(pgdesc), bIsLevel, uiType);
        if (ulVector != LW_VECTOR_INVALID) {                            /*  外部中断设置成功            */
            if (bIsLevel) {
                pgdesc->GD_ulFlags |= LW_GPIODF_TRIG_LEVEL;
            }
            if (uiType == 0) {
                pgdesc->GD_ulFlags |= LW_GPIODF_TRIG_FALL;
            
            } else if (uiType == 1) {
                pgdesc->GD_ulFlags |= LW_GPIODF_TRIG_RISE;
                
            } else if (uiType == 2) {
                pgdesc->GD_ulFlags |= (LW_GPIODF_TRIG_FALL | LW_GPIODF_TRIG_RISE);
            }
        }
        return  (ulVector);
    
    } else {
        _ErrorHandle(ENXIO);
        return  (LW_VECTOR_INVALID);
    }
}
/*********************************************************************************************************
** 函数名称: API_GpioClearIrq
** 功能描述: GPIO 为外部中断输入模式时, 在中断上下文中清除中断请求操作
** 输　入  : uiGpio        GPIO 号
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 此函数不做任何参数有效性判断, 所以用户必须保证 uiGpio 已经请求成功, 方向与其他参数设置正确.

                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_GpioClearIrq (UINT uiGpio)
{
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;
    
    pgdesc = GPIO_TO_DESC(uiGpio);                                      /*  不做任何检查, 加快速度      */
    pgchip = pgdesc->GD_pgcChip;
    
    if (pgchip->GC_pfuncClearIrq) {
        pgchip->GC_pfuncClearIrq(pgchip, GPIO_CHIP_HWGPIO(pgdesc));
    }
}
/*********************************************************************************************************
** 函数名称: API_GpioSvrIrq
** 功能描述: GPIO 为外部中断输入模式时, 判断当前是否为指定的 GPIO 中断
** 输　入  : uiGpio        GPIO 号
** 输　出  : LW_IRQ_HANDLED 表示当前中断是指定 GPIO 产生的中断
**           LW_IRQ_NONE    表示当前中断不是指定 GPIO 产生的中断
** 全局变量: 
** 调用模块: 
** 注  意  : 此函数不做任何参数有效性判断, 所以用户必须保证 uiGpio 已经请求成功, 方向与其他参数设置正确.

                                           API 函数
*********************************************************************************************************/
LW_API  
irqreturn_t  API_GpioSvrIrq (UINT uiGpio)
{
    irqreturn_t     irqret = LW_IRQ_NONE;
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;
    
    pgdesc = GPIO_TO_DESC(uiGpio);                                      /*  不做任何检查, 加快速度      */
    pgchip = pgdesc->GD_pgcChip;
    
    if (pgchip->GC_pfuncSvrIrq) {
        irqret = pgchip->GC_pfuncSvrIrq(pgchip, GPIO_CHIP_HWGPIO(pgdesc));
    }
    
    return  (irqret);
}

#endif                                                                  /*  LW_CFG_GPIO_EN > 0          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
