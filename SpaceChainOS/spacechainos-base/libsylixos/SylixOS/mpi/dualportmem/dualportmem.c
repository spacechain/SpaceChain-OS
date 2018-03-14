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
** 文   件   名: dualportmem.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 04 月 10 日
**
** 描        述: 系统支持共享内存式多处理器双口内存接口函数

** BUG
2007.10.21  修改注释.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock(); 
2008.05.31  使用 __KERNEL_MODE_...().
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/mpi/include/mpi_mpi.h"
/*********************************************************************************************************
** 函数名称: API_PortCreate
** 功能描述: 建立一个 DPMA 通道
** 输　入  : 
**           pcName                        DPMA 通道名字
**           pvInternalStart               内部入口地址
**           pvExternalStart               内部外部地址
**           stByteLenght                  大小(字节)
**           pulId                         Id指针
** 输　出  : 新建立的 DPMA 通道句柄
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_MPI_EN > 0

LW_API  
LW_OBJECT_HANDLE  API_PortCreate (PCHAR          pcName,
                                  PVOID          pvInternalStart,
                                  PVOID          pvExternalStart,
                                  size_t         stByteLenght,
                                  LW_OBJECT_ID  *pulId)
{
    REGISTER PLW_CLASS_DPMA pdpma;
    REGISTER ULONG          ulIdTemp;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pvInternalStart) {                                             /*  内部地址出错                */
        _ErrorHandle(ERROR_DPMA_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (!pvExternalStart) {                                             /*  外部地址出错                */
        _ErrorHandle(ERROR_DPMA_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (!_Addresses_Is_Aligned(pvInternalStart) ||
        !_Addresses_Is_Aligned(pvExternalStart)) {
        _ErrorHandle(ERROR_DPMA_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (stByteLenght < sizeof(INT)) {                                   
        _ErrorHandle(ERROR_DPMA_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
#endif
    
    if (_Object_Name_Invalid(pcName)) {                                 /*  检查名字有效性              */
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    __KERNEL_MODE_PROC(
        pdpma = _Allocate_Dpma_Object();                                /*  获得一个分区控制块          */
    );
    
    if (!pdpma) {
        _ErrorHandle(ERROR_DPMA_FULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    pdpma->DPMA_pvInternalBase = pvInternalStart;
    pdpma->DPMA_pvExternalBase = pvExternalStart;
    pdpma->DPMA_stByteLength   = stByteLenght - 1;
    
    if (pcName) {                                                       /*  拷贝名字                    */
        lib_strcpy(pdpma->DPMA_cDpmaName, pcName);
    } else {
        pdpma->DPMA_cDpmaName[0] = PX_EOS;                              /*  清空名字                    */
    }
    
    ulIdTemp = _MakeObjectId(_OBJECT_DPMA, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             pdpma->DPMA_usIndex);                      /*  构建对象 id                 */
                              
    if (pulId) {
        *pulId = ulIdTemp;
    }
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, LW_OPTION_OBJECT_GLOBAL);         /*  默认为全局                  */
    
    return  (ulIdTemp);
}
/*********************************************************************************************************
** 函数名称: API_PortDelete
** 功能描述: 删除一个 DPMA 通道
** 输　入  : 
**           pulId                         Id指针
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_PortDelete (LW_OBJECT_HANDLE   *pulId)
{
    REGISTER PLW_CLASS_DPMA         pdpma;
    REGISTER UINT16                 usIndex;
    REGISTER LW_OBJECT_HANDLE       ulId;
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_DPMA)) {                          /*  对象类型检查                */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Dpma_Index_Invalid(usIndex)) {                                 /*  缓冲区索引检查              */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    
    pdpma = &_G_dpmaBuffer[usIndex];

    _ObjectCloseId(pulId);                                              /*  关闭句柄                    */
    
    __KERNEL_MODE_PROC(
        _Free_Dpma_Object(pdpma);                                       /*  释放控制块                  */
    );
    
    __LW_OBJECT_DELETE_HOOK(ulId);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PortExToIn
** 功能描述: 将外部地址转换成内部地址
** 输　入  : 
**           ulId                          句柄
**           pvExternal                    外部地址
**           ppvInternal                   转换完成的内部地址
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_PortExToIn (LW_OBJECT_HANDLE   ulId,
                       PVOID              pvExternal,
                       PVOID             *ppvInternal)
{
    REGISTER PLW_CLASS_DPMA         pdpma;
    REGISTER UINT16                 usIndex;
    REGISTER size_t                 ulEnding;

    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_DPMA)) {                          /*  对象类型检查                */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Dpma_Index_Invalid(usIndex)) {                                 /*  缓冲区索引检查              */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (!pvExternal) {                                                  /*  地址是否有效                */
        _ErrorHandle(ERROR_DPMA_NULL);
        return  (ERROR_DPMA_NULL);
    }
    if (!ppvInternal) {                                                 /*  保存点是否有效              */
        _ErrorHandle(ERROR_DPMA_NULL);
        return  (ERROR_DPMA_NULL);
    }
#endif
    
    pdpma = &_G_dpmaBuffer[usIndex];
    
    ulEnding = _Addresses_Subtract(pvExternal, pdpma->DPMA_pvExternalBase);
    
    if (ulEnding > pdpma->DPMA_stByteLength) {                          /*  出界了                      */
        *ppvInternal = pvExternal;
        _ErrorHandle(ERROR_DPMA_OVERFLOW);
        return  (ERROR_DPMA_OVERFLOW);
        
    } else {                                                            /*  正常                        */
        *ppvInternal = _Addresses_Add_Offset(pdpma->DPMA_pvInternalBase,
                                             ulEnding);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: API_PortInToEx
** 功能描述: 将内部地址转换成外部地址
** 输　入  : 
**           ulId                          句柄
**           pvInternal                    内部地址
**           ppvExternal                   转换完成的外部地址
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_PortInToEx (LW_OBJECT_HANDLE   ulId,
                       PVOID              pvInternal,
                       PVOID             *ppvExternal)
{
    REGISTER PLW_CLASS_DPMA         pdpma;
    REGISTER UINT16                 usIndex;
    REGISTER size_t                 stEnding;

    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_DPMA)) {                          /*  对象类型检查                */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Dpma_Index_Invalid(usIndex)) {                                 /*  缓冲区索引检查              */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (!pvInternal) {                                                  /*  地址是否有效                */
        _ErrorHandle(ERROR_DPMA_NULL);
        return  (ERROR_DPMA_NULL);
    }
    if (!ppvExternal) {                                                 /*  保存点是否有效              */
        _ErrorHandle(ERROR_DPMA_NULL);
        return  (ERROR_DPMA_NULL);
    }
#endif
    
    pdpma = &_G_dpmaBuffer[usIndex];
    
    stEnding = _Addresses_Subtract(pvInternal, pdpma->DPMA_pvInternalBase);
    
    if (stEnding > pdpma->DPMA_stByteLength) {                          /*  出界了                      */
        *ppvExternal = pvInternal;
        _ErrorHandle(ERROR_DPMA_OVERFLOW);
        return  (ERROR_DPMA_OVERFLOW);
        
    } else {
        *ppvExternal = _Addresses_Add_Offset(pdpma->DPMA_pvExternalBase,
                                             stEnding);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: API_PortGetName
** 功能描述: 获得端口名字
** 输　入  : 
**           ulId                          句柄
**           pcName                        名字缓冲区
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_PortGetName (LW_OBJECT_HANDLE  ulId, PCHAR  pcName)
{
    REGISTER PLW_CLASS_DPMA         pdpma;
    REGISTER UINT16                 usIndex;

    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_DPMA)) {                          /*  对象类型检查                */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Dpma_Index_Invalid(usIndex)) {                                 /*  缓冲区索引检查              */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    pdpma = &_G_dpmaBuffer[usIndex];
    
    lib_strcpy(pcName, pdpma->DPMA_cDpmaName);                          /*  拷贝名字                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MPI_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
