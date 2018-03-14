/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: x86AcpiLib.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 04 月 14 日
**
** 描        述: x86 体系构架 ACPI 库源文件.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "x86AcpiLib.h"
#include "arch/x86/mpconfig/x86MpApic.h"
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
ACPI_MODULE_NAME("acpi_lib")

static BOOL     _G_bAcpiInited = LW_FALSE;

ULONG           _G_ulAcpiMcfgBaseAddress = 0;
BOOL            _G_bAcpiPciConfigAccess  = LW_TRUE;
BOOL            _G_bAcpiEarlyAccess      = LW_TRUE;
/*********************************************************************************************************
** 函数名称: acpiLibSetModel
** 功能描述: 设置 ACPI 中断模型
** 输　入  : uiModel           中断模型
** 输　出  : ACPI 状态
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ACPI_STATUS  acpiLibSetModel (UINT32  uiModel)
{
    ACPI_OBJECT       arg;
    ACPI_OBJECT_LIST  args;

    arg.Type          = ACPI_TYPE_INTEGER;
    arg.Integer.Value = uiModel;
    args.Count        = 1;
    args.Pointer      = &arg;

    return  (AcpiEvaluateObject(ACPI_ROOT_OBJECT, "_PIC", &args, LW_NULL));
}
/*********************************************************************************************************
** 函数名称: __acpiDummyExceptionHandler
** 功能描述: 假的 ACPI 异常处理程序
** 输　入  : NONE
** 输　出  : AE_OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ACPI_STATUS  __acpiDummyExceptionHandler (ACPI_STATUS  AmlStatus,
                                                 ACPI_NAME    Name,
                                                 UINT16       usOpcode,
                                                 UINT32       uiAmlOffset,
                                                 VOID        *pvContext)
{
    return  (AE_OK);
}
/*********************************************************************************************************
** 函数名称: __acpiNotifyHandler
** 功能描述: ACPI 通知处理程序
** 输　入  : hDevice       设备句柄
**           uiValue       值
**           pvContext     上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __acpiNotifyHandler (ACPI_HANDLE  hDevice, UINT32  uiValue, VOID  *pvContext)
{
    ACPI_INFO((AE_INFO, "Received a notify %X", uiValue));
}
/*********************************************************************************************************
** 函数名称: __acpiShutdownHandler
** 功能描述: ACPI 关机处理
** 输　入  : NONE
** 输　出  : AE_OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT32  __acpiShutdownHandler (VOID  *pvContext)
{
    ACPI_EVENT_STATUS     eStatus;
    ACPI_STATUS __unused  Status;

    /*
     * Get Event Data
     */
    Status = AcpiGetEventStatus(ACPI_EVENT_POWER_BUTTON, &eStatus);

    if (eStatus & ACPI_EVENT_FLAG_ENABLED) {
        AcpiClearEvent(ACPI_EVENT_POWER_BUTTON);
    }

    API_KernelReboot(LW_REBOOT_SHUTDOWN);

    return  (AE_OK);
}
/*********************************************************************************************************
** 函数名称: __acpiSleepHandler
** 功能描述: ACPI 睡眠处理
** 输　入  : NONE
** 输　出  : AE_OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT32  __acpiSleepHandler (VOID  *pvContext)
{
    return  (AE_OK);
}
/*********************************************************************************************************
** 函数名称: __acpiEventHandler
** 功能描述: ACPI 事件处理
** 输　入  : NONE
** 输　出  : AE_OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __acpiEventHandler (UINT32       uiEventType,
                                 ACPI_HANDLE  hDevice,
                                 UINT32       uiEventNumber,
                                 VOID        *pvContext)
{
}
/*********************************************************************************************************
** 函数名称: acpiLibInstallHandlers
** 功能描述: 安装 ACPI 处理程序
** 输　入  : NONE
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  acpiLibInstallHandlers (VOID)
{
    ACPI_STATUS  status;

    status = AcpiInstallNotifyHandler(ACPI_ROOT_OBJECT, ACPI_SYSTEM_NOTIFY,
                                      __acpiNotifyHandler, LW_NULL);
    if (ACPI_FAILURE(status)) {
        ACPI_EXCEPTION((AE_INFO, status, "While installing Notify handler"));
        return  (PX_ERROR);
    }

    status = AcpiInstallGlobalEventHandler(__acpiEventHandler, LW_NULL);
    if (ACPI_FAILURE(status)) {
        ACPI_EXCEPTION((AE_INFO, status, "While installing Global Event handler"));
        return  (PX_ERROR);
    }

    /*
     * 不检查返回值, 大多数 IBM-PC 兼容机都没有休眠按钮
     */
    AcpiInstallFixedEventHandler(ACPI_EVENT_POWER_BUTTON, __acpiShutdownHandler, LW_NULL);
    AcpiInstallFixedEventHandler(ACPI_EVENT_SLEEP_BUTTON, __acpiSleepHandler,    LW_NULL);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: acpiLibInit
** 功能描述: ACPI 库初始化
** 输　入  : bEarlyInit        是否早期初始化
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  acpiLibInit (BOOL  bEarlyInit)
{
    ACPI_STATUS         status;
    ACPI_SYSTEM_INFO    systemInfoBuffer;
    ACPI_BUFFER         acpiBuffer;
    UINT32              uiLocalAcpiDbgLevel;
    UINT32              uiMode = 0;

    _G_bAcpiEarlyAccess = bEarlyInit;

    __ACPI_DEBUG_LOG("\n**** acpiLibInit ****\n");

    if (!ACPI_DEBUG_ENABLED) {
        uiLocalAcpiDbgLevel = AcpiDbgLevel;
        AcpiDbgLevel = 0;                                               /*  No debug text output        */
    }

    /*
     * Init PCIE ECAM base addess
     */
    _G_ulAcpiMcfgBaseAddress = acpiGetMcfg();

    /*
     * Init ACPI and start debugger thread
     */

    /*
     * Init the ACPI system information
     */
    __ACPI_DEBUG_LOG("\n  AcpiInitializeSubsystem \n");
    status = AcpiInitializeSubsystem();                                 /*  ACPI CAPR 3.2.1.1 (266)     */
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> Could not initialize subsystem, %s <---\n",
                         AcpiFormatException(status));
        return  (status);
    }

    if (!ACPI_DEBUG_ENABLED) {
        AcpiInstallExceptionHandler(__acpiDummyExceptionHandler);       /*  Report no errors            */
    }

    __ACPI_DEBUG_LOG("\n  AcpiInitializeTables\n");
    status = AcpiInitializeTables(LW_NULL, 24, 1);                      /*  Not there                   */
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> Could not initialize ACPI tables, %s <---\n",
                         AcpiFormatException(status));
        return  (status);
    }

    __ACPI_DEBUG_LOG("\n  AcpiLoadTables\n");                           /*  ACPI CAPR 3.2.2.2           */
    status = AcpiLoadTables();                                          /*  372                         */
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> Could not load ACPI tables, %s <---\n",
                         AcpiFormatException(status));
        return  (status);
    }

    __ACPI_DEBUG_LOG("\n  AcpiEvInstallRegionHandlers\n");
    status = AcpiEvInstallRegionHandlers();
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> Unable to install the ACPI region handler, %s <---\n",
                         AcpiFormatException(status));
        goto    __error_handle;
    }

    /*
     * Complete namespace initialization by initializing device
     * objects and executing AML code for Regions, buffers, etc.
     * ACPI CAPR 3.2.2.3
     */
    __ACPI_DEBUG_LOG("\n  AcpiEnableSubsystem\n");                      /*  ACPI CAPR 3.2.4.1, 389      */
    status = AcpiEnableSubsystem(ACPI_NO_ACPI_ENABLE);
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> Unable to start the ACPI Interpreter, %s <---\n",
                         AcpiFormatException(status));
        goto    __error_handle;
    }

    __ACPI_DEBUG_LOG("\n  AcpiInitializeObjects\n");
    status = AcpiInitializeObjects(ACPI_NO_DEVICE_INIT);                /*  376                         */
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> Unable to initialize ACPI objects, %s <---\n",
                         AcpiFormatException(status));
        goto    __error_handle;
    }

    __ACPI_DEBUG_LOG("\n  Interpreter enabled\n");

    /*
     * TODO: 虚拟化 GUEST 系统不需要调用 acpiLibSetModel
     */
    status = acpiLibSetModel(1);
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> Unable to set ACPI Model: %s <---\n",
                         AcpiFormatException(status));
    }

    acpiBuffer.Length  = sizeof(ACPI_SYSTEM_INFO);
    acpiBuffer.Pointer = &systemInfoBuffer;

    status = AcpiGetSystemInfo(&acpiBuffer);
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> Unable to get ACPI system information, %s <---\n",
                         AcpiFormatException(status));
        goto    __error_handle;
    }

    __ACPI_DEBUG_LOG("\n  ACPI system info:\n");
    __ACPI_DEBUG_LOG("\n    AcpiCaVersion - 0x%x\n", systemInfoBuffer.AcpiCaVersion);
    __ACPI_DEBUG_LOG("    Flags - 0x%x\n",           systemInfoBuffer.Flags);
    __ACPI_DEBUG_LOG("    TimerResolution - 0x%x\n", systemInfoBuffer.TimerResolution);
    __ACPI_DEBUG_LOG("    DebugLevel - 0x%x\n",      systemInfoBuffer.DebugLevel);
    __ACPI_DEBUG_LOG("    DebugLayer - 0x%x\n",      systemInfoBuffer.DebugLayer);

    /*
     * Full ACPI subsystem init is needed by SylixOS for power management.
     */
    if (ACPI_DEBUG_ENABLED) {
        uiMode = AcpiHwGetMode();
        if (uiMode == ACPI_SYS_MODE_ACPI) {
            __ACPI_DEBUG_LOG("\n  Current operating state of system : ACPI_SYS_MODE_ACPI %d \n",
                             uiMode);
        } else {
            if (uiMode == ACPI_SYS_MODE_LEGACY) {
                __ACPI_DEBUG_LOG("\n  Current operating state of system : ACPI_SYS_MODE_LEGACY %d \n",
                                 uiMode);
            } else {
                __ACPI_DEBUG_LOG("\n  Current operating state of system : ACPI_SYS_MODE_UNKNOWN %d \n",
                                 uiMode);
            }
        }
    }

    if (!_G_bAcpiEarlyAccess) {
        __ACPI_DEBUG_LOG("\n  AcpiEnable\n");
        status = AcpiEnable();                                          /*  ACPI CAPR 6.3.1             */
        if (ACPI_FAILURE(status)) {
            __ACPI_DEBUG_LOG("\n---> Unable to enable ACPI system %s <--\n",
                             AcpiFormatException(status));
        }
    }

    AcpiOsInfoShow();                                                   /*  打印操作系统相关的信息      */

    __ACPI_DEBUG_LOG("\n**** acpiLibInit: completed ERROR_NONE ****\n");

    if (!ACPI_DEBUG_ENABLED) {
        AcpiDbgLevel = uiLocalAcpiDbgLevel;
    }

    return  (ERROR_NONE);

__error_handle:
    AcpiOsTerminate();
    __ACPI_DEBUG_LOG("\n---> acpiLibInit FAILED <---\n");
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: acpiLibDisable
** 功能描述: ACPI 库关闭
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  acpiLibDisable (VOID)
{
    __ACPI_DEBUG_LOG("\n**** acpiLibDisable: AcpiTerminate ****\n");
    AcpiTerminate();
}
/*********************************************************************************************************
** 函数名称: acpiGetMcfg
** 功能描述: 从 ACPI 表获得 MCFG 地址
** 输　入  : NONE
** 输　出  : MCFG 地址或 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG  acpiGetMcfg (VOID)
{
    ACPI_TABLE_RSDP   *pRsdp;
    ACPI_TABLE_HEADER *pHeader = LW_NULL;
    ACPI_TABLE_RSDT   *pRsdt;
    ACPI_TABLE_XSDT   *pXsdt;
    INT                iTableEntriesNr, i;

    pRsdp = acpiFindRsdp();
    if (pRsdp == LW_NULL) {
        return  (0);
    }

    /*
     * There are two possible table pointers
     */

    /*
     * If we have a 32 bit long address table
     */
    if (_G_pAcpiRsdt != LW_NULL) {
        pRsdt = AcpiOsMapMemory((ACPI_PHYSICAL_ADDRESS)_G_pAcpiRsdt, sizeof(ACPI_TABLE_RSDT));
        if (pRsdt == LW_NULL) {
            return  (0);
        }

        /*
         * Compute number of tables
         */
        iTableEntriesNr = (INT)((pRsdt->Header.Length - sizeof(ACPI_TABLE_HEADER)) / sizeof(UINT32));

        /*
         * We need to verify the length
         */
        if (iTableEntriesNr >= 0) {
            for (i = 0; i < iTableEntriesNr; i++) {
                pHeader = AcpiOsMapMemory((ACPI_PHYSICAL_ADDRESS)pRsdt->TableOffsetEntry[i],
                                          sizeof(ACPI_TABLE_HEADER));
                if (pHeader == LW_NULL) {
                    continue;
                }

                if (ACPI_NAME_COMPARE(pHeader->Signature, "MCFG")) {
                    AcpiOsUnmapMemory(pRsdt, sizeof(ACPI_TABLE_RSDT));
                    return  ((ULONG)pHeader);
                }
            }

            /*
             * Unmap header for both 32-bit and 64-bit if not found
             */
            AcpiOsUnmapMemory(pHeader, sizeof(ACPI_TABLE_HEADER));
        }
        AcpiOsUnmapMemory(pRsdt, sizeof(ACPI_TABLE_RSDT));
    }

    /*
     * If we have a 64 bit long address table
     */
    if (_G_pAcpiXsdt != LW_NULL) {
        pXsdt = AcpiOsMapMemory((ACPI_PHYSICAL_ADDRESS)_G_pAcpiXsdt, sizeof(ACPI_TABLE_XSDT));
        if (pXsdt == LW_NULL) {
            return  (0);
        }

        /*
         * Compute number of tables
         */
        iTableEntriesNr = (INT)((pXsdt->Header.Length - sizeof(ACPI_TABLE_HEADER)) / sizeof(UINT64));
        if (iTableEntriesNr >= 0) {
            for (i = 0; i < iTableEntriesNr; i++) {
                pHeader = AcpiOsMapMemory((ACPI_PHYSICAL_ADDRESS)pXsdt->TableOffsetEntry[i],
                                          sizeof(ACPI_TABLE_HEADER));
                if (pHeader == LW_NULL) {
                    continue;
                }

                if (ACPI_NAME_COMPARE(pHeader->Signature, "MCFG")) {
                    AcpiOsUnmapMemory(pXsdt, sizeof(ACPI_TABLE_XSDT));
                    return  ((ULONG)pHeader);
                }
            }
            AcpiOsUnmapMemory(pHeader, sizeof(ACPI_TABLE_HEADER));
        }
        AcpiOsUnmapMemory(pXsdt, sizeof(ACPI_TABLE_XSDT));
    }

    return  (0);
}
/*********************************************************************************************************
** 函数名称: x86AcpiAvailable
** 功能描述: 判断 ACPI 是否可用
** 输　入  : NONE
** 输　出  : LW_TRUE OR LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  x86AcpiAvailable (VOID)
{
    return  (_G_bAcpiInited);
}
/*********************************************************************************************************
** 函数名称: x86AcpiInit
** 功能描述: ACPI 组件初始化
** 输　入  : NONE
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86AcpiInit (VOID)
{
    INT  iError;

    /*
     * 查找 ACPI 的表并映射它们到内存
     */
    iError = acpiTableInit();
    if (iError != ERROR_NONE) {
        return  (PX_ERROR);
    }

    /*
     * 后期初始化 ACPI 库
     */
    iError = acpiLibInit(LW_FALSE);
    if (iError != ERROR_NONE) {
        return  (PX_ERROR);
    }

    /*
     * 安装 ACPI 处理程序
     */
    iError = acpiLibInstallHandlers();
    if (iError != ERROR_NONE) {
        return  (PX_ERROR);
    }

    /*
     * 因为早期已经扫描过 ACPI 设备, 获得了完整的信息(如中断路由表和总线拓扑)， 并填充到 MPAPIC_DATA
     * 所以这里不再需要调用缓慢的 acpiLibDevScan 函数
     */

    _G_bAcpiInited = LW_TRUE;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
