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
** 文   件   名: gpioDev.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 11 月 29 日
**
** 描        述: GPIO (通用输入/输出) 管脚用户态操作设备模型.

** BUG:
2014.04.21  修正多 GPIO 复用一个中断向量的问题.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_GPIO_EN > 0)
#include "sys/gpiofd.h"
/*********************************************************************************************************
  gpiofd flags 整合定义
*********************************************************************************************************/
#define GPIO_FLAG_IRQ       (GPIO_FLAG_TRIG_FALL | GPIO_FLAG_TRIG_RISE | GPIO_FLAG_TRIG_LEVEL)
#define GPIO_FLAG_IRQ_T0    (GPIO_FLAG_TRIG_FALL)
#define GPIO_FLAG_IRQ_T1    (GPIO_FLAG_TRIG_RISE)
#define GPIO_FLAG_IRQ_T2    (GPIO_FLAG_TRIG_FALL | GPIO_FLAG_TRIG_RISE)
/*********************************************************************************************************
  驱动程序全局变量
*********************************************************************************************************/
static INT              _G_iGpiofdDrvNum = PX_ERROR;
static LW_GPIOFD_DEV    _G_gpiofddev;
static LW_OBJECT_HANDLE _G_hGpiofdSelMutex;
/*********************************************************************************************************
  驱动程序
*********************************************************************************************************/
static LONG     _gpiofdOpen(PLW_GPIOFD_DEV    pgpiofddev, PCHAR  pcName, INT  iFlags, INT  iMode);
static INT      _gpiofdClose(PLW_GPIOFD_FILE  pgpiofdfil);
static ssize_t  _gpiofdRead(PLW_GPIOFD_FILE   pgpiofdfil, PCHAR  pcBuffer, size_t  stMaxBytes);
static ssize_t  _gpiofdWrite(PLW_GPIOFD_FILE  pgpiofdfil, PCHAR  pcBuffer, size_t  stNBytes);
static INT      _gpiofdIoctl(PLW_GPIOFD_FILE  pgpiofdfil, INT    iRequest, LONG  lArg);
/*********************************************************************************************************
  中断服务
*********************************************************************************************************/
static irqreturn_t  _gpiofdIsr(PLW_GPIOFD_FILE    pgpiofdfil);
/*********************************************************************************************************
** 函数名称: API_GpiofdDrvInstall
** 功能描述: 安装 gpiofd 设备驱动程序
** 输　入  : NONE
** 输　出  : 驱动是否安装成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GpiofdDrvInstall (VOID)
{
    if (_G_iGpiofdDrvNum <= 0) {
        _G_iGpiofdDrvNum  = iosDrvInstall(LW_NULL,
                                          LW_NULL,
                                          _gpiofdOpen,
                                          _gpiofdClose,
                                          _gpiofdRead,
                                          _gpiofdWrite,
                                          _gpiofdIoctl);
        DRIVER_LICENSE(_G_iGpiofdDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iGpiofdDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iGpiofdDrvNum, "gpiofd driver.");
    }
    
    if (_G_hGpiofdSelMutex == LW_OBJECT_HANDLE_INVALID) {
        _G_hGpiofdSelMutex =  API_SemaphoreMCreate("gpiofdsel_lock", LW_PRIO_DEF_CEILING, 
                                                   LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                                   LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                   LW_NULL);
    }
    
    return  ((_G_iGpiofdDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** 函数名称: API_GpiofdDevCreate
** 功能描述: 安装 gpiofd 设备
** 输　入  : NONE
** 输　出  : 设备是否创建成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GpiofdDevCreate (VOID)
{
    if (_G_iGpiofdDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&_G_gpiofddev.GD_devhdrHdr, LW_GPIOFD_DEV_PATH, 
                    _G_iGpiofdDrvNum, DT_DIR) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gpiofd
** 功能描述: 打开 gpiofd 文件
** 输　入  : gpio           gpio 号
**           flags          打开标志 GFD_CLOEXEC / GFD_NONBLOCK
**           gpio_flags     gpio 属性标志
** 输　出  : gpiofd 文件描述符
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  gpiofd (unsigned int gpio, int flags, int gpio_flags)
{
    INT  iFd;
    INT  iError;
    CHAR cGpioName[MAX_FILENAME_LENGTH];

    flags &= (GFD_CLOEXEC | GFD_NONBLOCK);
    
    snprintf(cGpioName, MAX_FILENAME_LENGTH, "%s/%d", LW_GPIOFD_DEV_PATH, gpio);
    
    iFd = open(cGpioName, O_RDWR | flags);
    if (iFd >= 0) {
        iError = ioctl(iFd, GPIO_CMD_SET_FLAGS, gpio_flags);
        if (iError < 0) {
            close(iFd);
            return  (iError);
        }
    }
    
    return  (iFd);
}
/*********************************************************************************************************
** 函数名称: gpiofd_read
** 功能描述: 读取 gpiofd 文件
** 输　入  : fd        文件描述符
**           value     读取缓冲
** 输　出  : 0 : 成功  -1 : 失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  gpiofd_read (int fd, uint8_t *value)
{
    return  (read(fd, value, sizeof(uint8_t)) != sizeof(uint8_t) ? PX_ERROR : ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gpiofd_write
** 功能描述: 写 gpiofd 文件
** 输　入  : fd        文件描述符
**           value     写入数据
** 输　出  : 0 : 成功  -1 : 失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  gpiofd_write (int fd, uint8_t  value)
{
    return  (write(fd, &value, sizeof(uint8_t)) != sizeof(uint8_t) ? PX_ERROR : ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _gpiofdOpen
** 功能描述: 打开 gpiofd 设备
** 输　入  : pgpiofddev       gpiofd 设备
**           pcName           名称
**           iFlags           方式
**           iMode            方法
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  _gpiofdOpen (PLW_GPIOFD_DEV pgpiofddev, 
                          PCHAR          pcName,
                          INT            iFlags, 
                          INT            iMode)
{
#define GPIO_IS_ROOT(gpio)  ((gpio) == __ARCH_UINT_MAX)

    PLW_GPIOFD_FILE  pgpiofdfil;
    UINT             uiGpio;
    PCHAR            pcTemp;
    ULONG            ulGpioLibFlags;

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);
            return  (PX_ERROR);
        }
        
        if (*pcName == PX_DIVIDER) {
            pcName++;
        }
        
        for (pcTemp = pcName; *pcTemp != PX_EOS; pcTemp++) {
            if (!lib_isdigit(*pcTemp)) {
                _ErrorHandle(ENOENT);
                return  (PX_ERROR);
            }
        }
        
        if (pcName[0] == PX_EOS) {
            uiGpio = __ARCH_UINT_MAX;
        
        } else {
            uiGpio = lib_atoi(pcName);
            if (API_GpioRequest(uiGpio, "gpiofd")) {
                return  (PX_ERROR);
            }
        }
        
        pgpiofdfil = (PLW_GPIOFD_FILE)__SHEAP_ALLOC(sizeof(LW_GPIOFD_FILE));
        if (!pgpiofdfil) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        
        pgpiofdfil->GF_iFlag  = iFlags;
        pgpiofdfil->GF_uiGpio = uiGpio;
        pgpiofdfil->GF_ulIrq  = LW_VECTOR_INVALID;
        
        API_GpioGetFlags(pgpiofdfil->GF_uiGpio, &ulGpioLibFlags);
        
        if (ulGpioLibFlags & LW_GPIODF_IS_OUT) {
            pgpiofdfil->GF_iGpioFlags = GPIO_FLAG_DIR_OUT;
        } else {
            pgpiofdfil->GF_iGpioFlags = GPIO_FLAG_DIR_IN;
        }
        
        if (ulGpioLibFlags & LW_GPIODF_TRIG_FALL) {
            pgpiofdfil->GF_iGpioFlags |= GPIO_FLAG_TRIG_FALL;
        }
        
        if (ulGpioLibFlags & LW_GPIODF_TRIG_RISE) {
            pgpiofdfil->GF_iGpioFlags |= GPIO_FLAG_TRIG_RISE;
        }
        
        if (ulGpioLibFlags & LW_GPIODF_TRIG_LEVEL) {
            pgpiofdfil->GF_iGpioFlags |= GPIO_FLAG_TRIG_LEVEL;
        }
        
        if (ulGpioLibFlags & LW_GPIODF_OPEN_DRAIN) {
            pgpiofdfil->GF_iGpioFlags |= GPIO_FLAG_OPEN_DRAIN;
        }
        
        if (ulGpioLibFlags & LW_GPIODF_OPEN_SOURCE) {
            pgpiofdfil->GF_iGpioFlags |= GPIO_FLAG_OPEN_SOURCE;
        }
        
        lib_bzero(&pgpiofdfil->GF_selwulist, sizeof(LW_SEL_WAKEUPLIST));
        pgpiofdfil->GF_selwulist.SELWUL_hListLock = _G_hGpiofdSelMutex;
        
        LW_DEV_INC_USE_COUNT(&_G_gpiofddev.GD_devhdrHdr);
        
        return  ((LONG)pgpiofdfil);
    }
}
/*********************************************************************************************************
** 函数名称: _gpiofdClose
** 功能描述: 关闭 gpiofd 文件
** 输　入  : pgpiofdfil        gpiofd 文件
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _gpiofdClose (PLW_GPIOFD_FILE  pgpiofdfil)
{
    if (pgpiofdfil) {
        SEL_WAKE_UP_TERM(&pgpiofdfil->GF_selwulist);
        
        LW_DEV_DEC_USE_COUNT(&_G_gpiofddev.GD_devhdrHdr);
        
        if (!GPIO_IS_ROOT(pgpiofdfil->GF_uiGpio)) {
            API_GpioFree(pgpiofdfil->GF_uiGpio);
            
            if (pgpiofdfil->GF_ulIrq != LW_VECTOR_INVALID) {
                API_InterVectorDisable(pgpiofdfil->GF_ulIrq);
                API_InterVectorDisconnect(pgpiofdfil->GF_ulIrq, (PINT_SVR_ROUTINE)_gpiofdIsr,
                                          (PVOID)pgpiofdfil);
            }
        }
        
        __SHEAP_FREE(pgpiofdfil);
        
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: _gpiofdRead
** 功能描述: 读 gpiofd 设备
** 输　入  : pgpiofdfil       gpiofd 文件
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : 读取字节数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  _gpiofdRead (PLW_GPIOFD_FILE pgpiofdfil, 
                             PCHAR           pcBuffer, 
                             size_t          stMaxBytes)
{
    INT  iValue;

    if (!pcBuffer || !stMaxBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (GPIO_IS_ROOT(pgpiofdfil->GF_uiGpio)) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    iValue = API_GpioGetValue(pgpiofdfil->GF_uiGpio);
    if (iValue < 0) {
        return  (iValue);
    }
    
    *pcBuffer = (CHAR)iValue;
    return  (1);
}
/*********************************************************************************************************
** 函数名称: _gpiofdWrite
** 功能描述: 写 gpiofd 设备
** 输　入  : pgpiofdfil       gpiofd 文件
**           pcBuffer         将要写入的数据指针
**           stNBytes         写入数据大小
** 输　出  : 写入字节数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  _gpiofdWrite (PLW_GPIOFD_FILE pgpiofdfil, 
                              PCHAR           pcBuffer, 
                              size_t          stNBytes)
{
    if (!pcBuffer || !stNBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (GPIO_IS_ROOT(pgpiofdfil->GF_uiGpio)) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    API_GpioSetValue(pgpiofdfil->GF_uiGpio, *pcBuffer);
    return  (1);
}
/*********************************************************************************************************
** 函数名称: _gpiofdReadDir
** 功能描述: 读 gpiofd 目录
** 输　入  : pgpiofdfil       gpiofd 文件
**           pdir             当前偏移量上的文件信息
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _gpiofdReadDir (PLW_GPIOFD_FILE pgpiofdfil, DIR  *pdir)
{
    if (!pdir) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!GPIO_IS_ROOT(pgpiofdfil->GF_uiGpio)) {
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }
    
    if (pdir->dir_pos >= LW_CFG_MAX_GPIOS) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    snprintf(pdir->dir_dirent.d_name, NAME_MAX + 1, "%ld", pdir->dir_pos);
    lib_strlcpy(pdir->dir_dirent.d_shortname, 
                pdir->dir_dirent.d_name, 
                sizeof(pdir->dir_dirent.d_shortname));
    pdir->dir_dirent.d_type = DT_CHR;
    pdir->dir_pos++;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _gpiofdIsr
** 功能描述: gpiofd 文件中断服务程序
** 输　入  : pgpiofdfil       gpiofd 文件
**           iFlags           gpiofd 属性标志
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static irqreturn_t  _gpiofdIsr (PLW_GPIOFD_FILE pgpiofdfil)
{
    irqreturn_t irqret;
    
    irqret = API_GpioSvrIrq(pgpiofdfil->GF_uiGpio);
    if (LW_IRQ_RETVAL(irqret)) {
        SEL_WAKE_UP_ALL(&pgpiofdfil->GF_selwulist, SELREAD);
        API_GpioClearIrq(pgpiofdfil->GF_uiGpio);
    }

    return  (irqret);
}
/*********************************************************************************************************
** 函数名称: _gpiofdSetFlagsIrq
** 功能描述: 设置 gpiofd 文件中断属性标志
** 输　入  : pgpiofdfil       gpiofd 文件
**           iFlags           gpiofd 属性标志
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _gpiofdSetFlagsIrq (PLW_GPIOFD_FILE pgpiofdfil, INT  iFlags)
{
    BOOL   bIsLevel;
    UINT   uiType;
    ULONG  ulIrq;
    
    if (iFlags & GPIO_FLAG_TRIG_LEVEL) {
        bIsLevel = LW_TRUE;
    
    } else {
        bIsLevel = LW_FALSE;
    }
    
    if ((iFlags & GPIO_FLAG_IRQ_T2) == GPIO_FLAG_IRQ_T2) {
        uiType = 2;
    
    } else if ((iFlags & GPIO_FLAG_IRQ_T0) == GPIO_FLAG_IRQ_T0) {
        uiType = 0;
    
    } else if ((iFlags & GPIO_FLAG_IRQ_T1) == GPIO_FLAG_IRQ_T1) {
        uiType = 1;
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    ulIrq = API_GpioGetIrq(pgpiofdfil->GF_uiGpio, bIsLevel, uiType);
    if (ulIrq == LW_VECTOR_INVALID) {
        return  (PX_ERROR);
    }
    
    API_InterVectorConnect(ulIrq, (PINT_SVR_ROUTINE)_gpiofdIsr,
                           (PVOID)pgpiofdfil, "gpiofd_isr");
    
    pgpiofdfil->GF_ulIrq = API_GpioSetupIrq(pgpiofdfil->GF_uiGpio, bIsLevel, uiType);
    if (pgpiofdfil->GF_ulIrq == LW_VECTOR_INVALID) {
        API_InterVectorDisconnect(pgpiofdfil->GF_ulIrq, 
                                  (PINT_SVR_ROUTINE)_gpiofdIsr, (PVOID)pgpiofdfil);
        return  (PX_ERROR);
    }
    
    API_InterVectorEnable(pgpiofdfil->GF_ulIrq);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _gpiofdSetFlagsOrg
** 功能描述: 设置 gpiofd 文件普通属性标志
** 输　入  : pgpiofdfil       gpiofd 文件
**           iFlags           gpiofd 属性标志
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _gpiofdSetFlagsOrg (PLW_GPIOFD_FILE pgpiofdfil, INT  iFlags)
{
    if (iFlags & GPIO_FLAG_DIR_IN) {
        if (API_GpioDirectionInput(pgpiofdfil->GF_uiGpio) < 0) {
            return  (PX_ERROR);
        }
    
    } else {
        if (API_GpioDirectionOutput(pgpiofdfil->GF_uiGpio, 
                                    ((iFlags & GPIO_FLAG_INIT_HIGH) ? 1 : 0)) < 0) {
            return  (PX_ERROR);
        }
    }
    
    if (iFlags & GPIO_FLAG_OPEN_DRAIN) {
        API_GpioOpenDrain(pgpiofdfil->GF_uiGpio, LW_TRUE);
    
    } else {
        API_GpioOpenDrain(pgpiofdfil->GF_uiGpio, LW_FALSE);
    }
    
    if (iFlags & GPIO_FLAG_OPEN_SOURCE) {
        API_GpioOpenSource(pgpiofdfil->GF_uiGpio, LW_TRUE);
    
    } else {
        API_GpioOpenSource(pgpiofdfil->GF_uiGpio, LW_FALSE);
    }
    
    if (pgpiofdfil->GF_ulIrq != LW_VECTOR_INVALID) {
        API_InterVectorDisable(pgpiofdfil->GF_ulIrq);
        API_InterVectorDisconnect(pgpiofdfil->GF_ulIrq, (PINT_SVR_ROUTINE)_gpiofdIsr, (PVOID)pgpiofdfil);
        pgpiofdfil->GF_ulIrq = LW_VECTOR_INVALID;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _gpiofdSetFlags
** 功能描述: 设置 gpiofd 文件属性标志
** 输　入  : pgpiofdfil       gpiofd 文件
**           iFlags           gpiofd 属性标志
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _gpiofdSetFlags (PLW_GPIOFD_FILE pgpiofdfil, INT  iFlags)
{
    INT  iError = ERROR_NONE;

    if (GPIO_IS_ROOT(pgpiofdfil->GF_uiGpio)) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (iFlags & GPIO_FLAG_PULL_UP) {
        iError = API_GpioSetPull(pgpiofdfil->GF_uiGpio, 1);
    
    } else if (iFlags & GPIO_FLAG_PULL_DOWN) {
        iError = API_GpioSetPull(pgpiofdfil->GF_uiGpio, 2);
    
    } else if (iFlags & GPIO_FLAG_PULL_DISABLE) {
        iError = API_GpioSetPull(pgpiofdfil->GF_uiGpio, 0);
    }
    
    if (iError < ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (iFlags & GPIO_FLAG_IRQ) {
        iError = _gpiofdSetFlagsIrq(pgpiofdfil, iFlags);
    
    } else {
        iError = _gpiofdSetFlagsOrg(pgpiofdfil, iFlags);
    }
    
    if (iError == ERROR_NONE) {
        pgpiofdfil->GF_iGpioFlags = iFlags;
    }
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: _gpiofdGetFlags
** 功能描述: 获取 gpiofd 文件属性标志
** 输　入  : pgpiofdfil       gpiofd 文件
**           piFlags          gpiofd 属性标志
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _gpiofdGetFlags (PLW_GPIOFD_FILE pgpiofdfil, INT  *piFlags)
{
    if (GPIO_IS_ROOT(pgpiofdfil->GF_uiGpio)) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (piFlags) {
        *piFlags = pgpiofdfil->GF_iGpioFlags;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _gpiofdSelect
** 功能描述: gpiofd FIOSELECT
** 输　入  : pgpiofdfil       gpiofd 文件
**           pselwunNode      select 节点
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _gpiofdSelect (PLW_GPIOFD_FILE  pgpiofdfil, PLW_SEL_WAKEUPNODE   pselwunNode)
{
    if (GPIO_IS_ROOT(pgpiofdfil->GF_uiGpio)) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    SEL_WAKE_NODE_ADD(&pgpiofdfil->GF_selwulist, pselwunNode);
    
    switch (pselwunNode->SELWUN_seltypType) {
    
    case SELREAD:
        if (!(pgpiofdfil->GF_iGpioFlags & GPIO_FLAG_IRQ)) {
            SEL_WAKE_UP(pselwunNode);
        }
        break;
        
    case SELWRITE:
        SEL_WAKE_UP(pselwunNode);
        break;
        
    case SELEXCEPT:
        break;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _gpiofdUnselect
** 功能描述: gpiofd FIOUNSELECT
** 输　入  : pgpiofdfil       gpiofd 文件
**           pselwunNode      select 节点
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _gpiofdUnselect (PLW_GPIOFD_FILE  pgpiofdfil, PLW_SEL_WAKEUPNODE   pselwunNode)
{
    if (GPIO_IS_ROOT(pgpiofdfil->GF_uiGpio)) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    SEL_WAKE_NODE_DELETE(&pgpiofdfil->GF_selwulist, pselwunNode);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _gpiofdIoctl
** 功能描述: 控制 gpiofd 文件
** 输　入  : pgpiofdfil       gpiofd 文件
**           iRequest         功能
**           lArg             参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _gpiofdIoctl (PLW_GPIOFD_FILE pgpiofdfil, 
                          INT             iRequest, 
                          LONG            lArg)
{
    struct stat         *pstatGet;
    struct statfs       *pstatfsGet;
    PLW_SEL_WAKEUPNODE   pselwunNode;
    
    switch (iRequest) {
    
    case FIONBIO:
        if (*(INT *)lArg) {
            pgpiofdfil->GF_iFlag |= O_NONBLOCK;
        } else {
            pgpiofdfil->GF_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    case FIOFSTATGET:
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev = (dev_t)&_G_gpiofddev;
            if (GPIO_IS_ROOT(pgpiofdfil->GF_uiGpio)) {
                pstatGet->st_ino  = (ino_t)0;
                pstatGet->st_mode = 0666 | S_IFDIR;
                pstatGet->st_size = 0;
                pstatGet->st_blksize = 1;
                pstatGet->st_blocks  = 0;
            
            } else {
                pstatGet->st_ino  = (ino_t)pgpiofdfil->GF_uiGpio;
                pstatGet->st_mode = 0666 | S_IFCHR;
                pstatGet->st_size = 1;
                pstatGet->st_blksize = 1;
                pstatGet->st_blocks  = 1;
            }
            pstatGet->st_nlink    = 1;
            pstatGet->st_uid      = 0;
            pstatGet->st_gid      = 0;
            pstatGet->st_rdev     = 1;
            pstatGet->st_atime    = API_RootFsTime(LW_NULL);
            pstatGet->st_mtime    = API_RootFsTime(LW_NULL);
            pstatGet->st_ctime    = API_RootFsTime(LW_NULL);
        } else {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        break;
        
    case FIOFSTATFSGET:
        pstatfsGet = (struct statfs *)lArg;
        if (pstatfsGet) {
            pstatfsGet->f_type   = 0;
            pstatfsGet->f_bsize  = 0;
            pstatfsGet->f_blocks = 1;
            pstatfsGet->f_bfree  = 0;
            pstatfsGet->f_bavail = 1;
            
            pstatfsGet->f_files  = LW_CFG_MAX_GPIOS;
            pstatfsGet->f_ffree  = 0;
            
#if LW_CFG_CPU_WORD_LENGHT == 64
            pstatfsGet->f_fsid.val[0] = (int32_t)((addr_t)&_G_gpiofddev >> 32);
            pstatfsGet->f_fsid.val[1] = (int32_t)((addr_t)&_G_gpiofddev & 0xffffffff);
#else
            pstatfsGet->f_fsid.val[0] = (int32_t)&_G_gpiofddev;
            pstatfsGet->f_fsid.val[1] = 0;
#endif
            
            pstatfsGet->f_flag    = 0;
            pstatfsGet->f_namelen = PATH_MAX;
        } else {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        break;
        
    case FIOREADDIR:
        return  (_gpiofdReadDir(pgpiofdfil, (DIR *)lArg));
        
    case GPIO_CMD_SET_FLAGS:
        return  (_gpiofdSetFlags(pgpiofdfil, (INT)lArg));
        
    case GPIO_CMD_GET_FLAGS:
        return  (_gpiofdGetFlags(pgpiofdfil, (INT *)lArg));
        
    case FIOSELECT:
        pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
        return  (_gpiofdSelect(pgpiofdfil, pselwunNode));
        
    case FIOUNSELECT:
        pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
        return  (_gpiofdUnselect(pgpiofdfil, pselwunNode));
        
    case FIOFSTYPE:
        *(PCHAR *)lArg = "GPIO FileSystem";
        return  (ERROR_NONE);
    
    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_GPIO_EN > 0          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
