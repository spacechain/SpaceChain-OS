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
** 文   件   名: RmsCreate.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 11 日
**
** 描        述: 建立一个精度单调调度器

** BUG
2007.11.04  加入 _DebugHandle() 功能.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2008.05.31  使用 __KERNEL_MODE_...().
2011.07.29  加入对象创建/销毁回调.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  注意：
        使用精度单调调度器，不可以设置系统时间，但是可是设置 RTC 时间。
        
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: API_RmsCreate
** 功能描述: 建立一个精度单调调度器
** 输　入  : 
**           pcName                        名字
**           ulOption                      选项
**           pulId                         Id 号
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)

LW_API  
LW_OBJECT_HANDLE  API_RmsCreate (CPCHAR             pcName,
                                 ULONG              ulOption,
                                 LW_OBJECT_ID      *pulId)
{
    REGISTER PLW_CLASS_RMS  prms;
    REGISTER ULONG          ulIdTemp;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (_Object_Name_Invalid(pcName)) {                                 /*  检查名字有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    __KERNEL_MODE_PROC(
        prms = _Allocate_Rms_Object();
    );                                                                  /*  退出内核                    */
    
    if (!prms) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is no ID to buide a RMS.\r\n");
        _ErrorHandle(ERROR_RMS_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    prms->RMS_ucType     = LW_RMS_USED;
    prms->RMS_ucStatus   = LW_RMS_INACTIVE;
    prms->RMS_ulTickNext = 0ul;
    prms->RMS_ulTickSave = 0ul;
    
    if (pcName) {                                                       /*  拷贝名字                    */
        lib_strcpy(prms->RMS_cRmsName, pcName);
    } else {
        prms->RMS_cRmsName[0] = PX_EOS;                                 /*  清空名字                    */
    }
    
    ulIdTemp = _MakeObjectId(_OBJECT_RMS, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             prms->RMS_usIndex);                        /*  构建对象 id                 */
    
    if (pulId) {
        *pulId = ulIdTemp;
    }
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, ulOption);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "RMS \"%s\" has been create.\r\n", (pcName ? pcName : ""));
    
    return  (ulIdTemp);
}

#endif                                                                  /*  (LW_CFG_RMS_EN > 0)         */
                                                                        /*  (LW_CFG_MAX_RMSS > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
