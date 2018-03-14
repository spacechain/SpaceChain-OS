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
** 文   件   名: input_device.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 10 月 24 日
**
** 描        述: GUI 输入设备管理工具. (支持固定设备与热插拔设备)
                 向热插拔注册了 LW_HOTPLUG_MSG_USB_KEYBOARD, LW_HOTPLUG_MSG_USB_MOUSE 事件回调.
                 input_device 主要是提供多鼠标与触摸屏, 多键盘的支持. 当使用单键盘, 单鼠标时可以直接操作
                 相关的设备.
                 
** BUG:
2011.06.07  加入 proc 文件系统信息.
2011.07.15  修正 __procFsInputDevPrint() 的错误.
2011.07.19  使用 __SHEAP... 替换 C 库内存分配.
2011.08.11  当 gid 线程退出时, 将关闭所有打开的文件并且将线程句柄设为无效.
2012.04.12  鼠标(触摸屏)支持多点触摸.
2012.12.13  输入线程为进程内线程, 进程退出前需要调用 API_GuiInputDevProcStop() 否则进程无法退出.
2013.10.03  不再使用老的热插拔检测方法, 而是用 /dev/hotplug 热插拔消息设备检测热插拔.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "keyboard.h"
#include "mouse.h"
#include "stdlib.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#include "../SylixOS/config/gui/gui_cfg.h"
#if LW_CFG_GUI_INPUT_DEV_EN > 0
#include "input_device.h"
/*********************************************************************************************************
  同时支持几点触摸
*********************************************************************************************************/
#define __GID_MAX_POINT      6
/*********************************************************************************************************
  输入文件集合结构
*********************************************************************************************************/
typedef struct {
    INT                      GID_iFd;                                   /*  打开的文件句柄              */
    PCHAR                    GID_pcName;                                /*  输入设备文件名              */
    INT                      GID_iType;                                 /*  输入设备的类型              */
#define __GID_KEYBOARD       0
#define __GID_MOUSE          1
} __GUI_INPUT_DEV;
typedef __GUI_INPUT_DEV     *__PGUI_INPUT_DEV;
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static __PGUI_INPUT_DEV      _G_pgidArray[2];
static INT                   _G_iGidDevNum[2];
static LW_SPINLOCK_DEFINE   (_G_slGidDev);
static INT                   _G_iHotplugFd   = PX_ERROR;
static BOOL                  _G_bIsHotplugIn = LW_FALSE;                /*  是否有新设备需要打开        */
static BOOL                  _G_bIsNeedStop  = LW_FALSE;
/*********************************************************************************************************
  输入回调函数
*********************************************************************************************************/
static VOIDFUNCPTR           _G_pfuncGidDevNotify[2];
/*********************************************************************************************************
  记录全局量
*********************************************************************************************************/
static size_t                _G_stInputDevNameTotalLen;
/*********************************************************************************************************
  线程句柄
*********************************************************************************************************/
static LW_OBJECT_HANDLE      _G_ulGidThread;
/*********************************************************************************************************
  proc 文件系统相关内容
  
  input_device proc 文件函数声明
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0

static ssize_t  __procFsInputDevRead(PLW_PROCFS_NODE  p_pfsn, 
                                     PCHAR            pcBuffer, 
                                     size_t           stMaxBytes,
                                     off_t            oft);
/*********************************************************************************************************
  input_device proc 文件操作函数组
*********************************************************************************************************/
static LW_PROCFS_NODE_OP        _G_pfsnoInputDevFuncs = {
    __procFsInputDevRead, LW_NULL
};
/*********************************************************************************************************
  input_device proc 文件 (input_device 预置大小为零, 表示需要手动分配内存)
*********************************************************************************************************/
static LW_PROCFS_NODE           _G_pfsnInputDev[] = 
{          
    LW_PROCFS_INIT_NODE("input_device", 
                        (S_IRUSR | S_IRGRP | S_IROTH | S_IFREG), 
                        &_G_pfsnoInputDevFuncs, 
                        "I",
                        0),
};
/*********************************************************************************************************
  input_device 打印头
*********************************************************************************************************/
static const CHAR               _G_cInputDevInfoHdr[] = "\n\
        INPUT DEV NAME         TYPE OPENED\n\
------------------------------ ---- ------\n";
/*********************************************************************************************************
** 函数名称: __procFsInputDevPrint
** 功能描述: 打印所有 input_device 信息
** 输　入  : pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
** 输　出  : 实际打印的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static size_t  __procFsInputDevPrint (PCHAR  pcBuffer, size_t  stMaxBytes)
{
             size_t             stRealSize;
             INT                i;
    REGISTER __PGUI_INPUT_DEV   pgidArray;
    
    stRealSize = bnprintf(pcBuffer, stMaxBytes, 0, "%s", _G_cInputDevInfoHdr);
    
    pgidArray = _G_pgidArray[__GID_KEYBOARD];
    for (i = 0; i < _G_iGidDevNum[__GID_KEYBOARD]; i++) {
        PCHAR   pcOpened;
        if (pgidArray->GID_pcName) {
            if (pgidArray->GID_iFd >= 0) {
                pcOpened = "true";
            } else {
                pcOpened = "false";
            }
            stRealSize = bnprintf(pcBuffer, stMaxBytes, stRealSize, 
                                  "%-30s %-4s %-6s\n",
                                  pgidArray->GID_pcName,
                                  "KBD", pcOpened);
        }
        pgidArray++;
    }
    
    pgidArray = _G_pgidArray[__GID_MOUSE];
    for (i = 0; i < _G_iGidDevNum[__GID_MOUSE]; i++) {
        PCHAR   pcOpened;
        if (pgidArray->GID_pcName) {
            if (pgidArray->GID_iFd >= 0) {
                pcOpened = "true";
            } else {
                pcOpened = "false";
            }
            stRealSize = bnprintf(pcBuffer, stMaxBytes, stRealSize, 
                                  "%-30s %-4s %-6s\n",
                                  pgidArray->GID_pcName,
                                  "PID", pcOpened);
        }
        pgidArray++;
    }
    
    stRealSize = bnprintf(pcBuffer, stMaxBytes, stRealSize, 
                          "\ntotal input device in spy: %d\n", 
                          (_G_iGidDevNum[__GID_KEYBOARD] + _G_iGidDevNum[__GID_MOUSE]));
                           
    return  (stRealSize);
}
/*********************************************************************************************************
** 函数名称: __procFsInputDevRead
** 功能描述: procfs 读 input_device 命名节点 proc 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsInputDevRead (PLW_PROCFS_NODE  p_pfsn, 
                                      PCHAR            pcBuffer, 
                                      size_t           stMaxBytes,
                                      off_t            oft)
{
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  实际的文件内容大小          */
             size_t     stCopeBytes;

    /*
     *  由于预置内存大小为 0 , 所以打开后第一次读取需要手动开辟内存.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  还没有分配内存              */
        INT         iCnt = _G_iGidDevNum[__GID_KEYBOARD]
                         + _G_iGidDevNum[__GID_MOUSE];
        size_t      stNeedBufferSize;
        
        /*
         *  stNeedBufferSize 已按照最大大小计算了.
         */
        stNeedBufferSize  = ((size_t)iCnt * 48) + _G_stInputDevNameTotalLen;
        stNeedBufferSize += lib_strlen(_G_cInputDevInfoHdr) + 32;
        stNeedBufferSize += 64;                                         /*  最后一行的 total 字符串     */
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            errno = ENOMEM;
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  重新获得文件缓冲区地址      */
        
        stRealSize = __procFsInputDevPrint(pcFileBuffer, 
                                           stNeedBufferSize);           /*  打印信息                    */
        
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    } else {
        stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  计算实际拷贝的字节数        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}

#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
** 函数名称: __guiInputDevReg
** 功能描述: 注册 gui 输入设备
** 输　入  : iType             设备类型
**           pcDevName         设备名
**           iNum              设备数量
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT   __guiInputDevReg (INT  iType, CPCHAR  pcDevName[], INT  iNum)
{
             INT                 i;
             size_t              stNameLen;
    REGISTER __PGUI_INPUT_DEV    pgidArray;

    _G_pgidArray[iType] = (__PGUI_INPUT_DEV)__SHEAP_ALLOC((size_t)iNum * sizeof(__GUI_INPUT_DEV));
    if (_G_pgidArray[iType] == LW_NULL) {
        return  (PX_ERROR);
    }
    _G_iGidDevNum[iType] = iNum;
    pgidArray = _G_pgidArray[iType];

    for (i = 0; i < iNum; i++) {
        pgidArray->GID_iFd   = PX_ERROR;
        pgidArray->GID_iType = iType;

        if (pcDevName[i]) {
            stNameLen = lib_strnlen(pcDevName[i], PATH_MAX);
            pgidArray->GID_pcName = (PCHAR)__SHEAP_ALLOC(stNameLen + 1);
            if (pgidArray->GID_pcName == LW_NULL) {
                return  (PX_ERROR);
            }
            lib_strlcpy(pgidArray->GID_pcName, pcDevName[i], PATH_MAX);
            _G_stInputDevNameTotalLen += stNameLen;
        } else {
            pgidArray->GID_pcName = LW_NULL;
        }
        pgidArray++;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __guiInputDevUnreg
** 功能描述: 卸载 gui 输入设备
** 输　入  : iType             设备类型
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __guiInputDevUnreg (INT  iType)
{
             INT                 i;
             INT                 iNum;
    REGISTER __PGUI_INPUT_DEV    pgidArray;

    pgidArray = _G_pgidArray[iType];
    iNum      = _G_iGidDevNum[iType];

    if (!pgidArray) {
        return;
    }

    for (i = 0; i < iNum; i++) {
        if (pgidArray->GID_pcName) {
            __SHEAP_FREE(pgidArray->GID_pcName);
        }
        pgidArray++;
    }

    __SHEAP_FREE(_G_pgidArray[iType]);

    _G_pgidArray[iType]  = LW_NULL;
    _G_iGidDevNum[iType] = 0;
}
/*********************************************************************************************************
** 函数名称: __guiInputDevMakeFdset
** 功能描述: 生成 gui 输入设备文件集
** 输　入  : iType             设备类型
** 输　出  : 最大的文件描述符
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __guiInputDevMakeFdset (INT  iType, fd_set  *pfdset)
{
             INT                 i;
             INT                 iMaxFd = 0;
             INT                 iNum;
    REGISTER __PGUI_INPUT_DEV    pgidArray;

    pgidArray = _G_pgidArray[iType];
    iNum      = _G_iGidDevNum[iType];

    if (!pgidArray) {
        return  (iMaxFd);
    }

    for (i = 0; i < iNum; i++) {
        if (pgidArray->GID_iFd >= 0) {
            FD_SET(pgidArray->GID_iFd, pfdset);
            if (iMaxFd < pgidArray->GID_iFd) {
                iMaxFd = pgidArray->GID_iFd;
            }
        }
        pgidArray++;
    }

    return  (iMaxFd);
}
/*********************************************************************************************************
** 函数名称: __guiInputDevTryOpen
** 功能描述: 试图打开 gui 输入设备文件
** 输　入  : iType             设备类型
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __guiInputDevTryOpen (INT  iType)
{
             INT                 i;
             INT                 iNum;
    REGISTER __PGUI_INPUT_DEV    pgidArray;

    pgidArray = _G_pgidArray[iType];
    iNum      = _G_iGidDevNum[iType];

    if (!pgidArray) {
        return;
    }

    for (i = 0; i < iNum; i++) {
        if ((pgidArray->GID_iFd < 0) && (pgidArray->GID_pcName)) {
            pgidArray->GID_iFd = open(pgidArray->GID_pcName, O_RDONLY);
        }
        pgidArray++;
    }
}
/*********************************************************************************************************
** 函数名称: __guiInputDevCloseAll
** 功能描述: 关闭所有 gui 输入设备文件
** 输　入  : iType             设备类型
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __guiInputDevCloseAll (INT  iType)
{
             INT                 i;
             INT                 iNum;
    REGISTER __PGUI_INPUT_DEV    pgidArray;

    pgidArray = _G_pgidArray[iType];
    iNum      = _G_iGidDevNum[iType];

    if (!pgidArray) {
        return;
    }

    for (i = 0; i < iNum; i++) {
        if ((pgidArray->GID_iFd >= 0) && (pgidArray->GID_pcName)) {
            close(pgidArray->GID_iFd);
            pgidArray->GID_iFd = PX_ERROR;
        }
        pgidArray++;
    }
}
/*********************************************************************************************************
** 函数名称: __guiInputDevHotplugMsg
** 功能描述: gui 输入设备热插拔事件消息处理
** 输　入  : pucMsg       热插拔消息
**           stSize       消息长度
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __guiInputDevHotplugMsg (PUCHAR  pucMsg, size_t stSize)
{
    INTREG  iregInterLevel;
    INT     iMsg;
    
    (VOID)stSize;
    
    iMsg = (INT)((pucMsg[0] << 24) + (pucMsg[1] << 16) + (pucMsg[2] << 8) + pucMsg[3]);
    if ((iMsg == LW_HOTPLUG_MSG_USB_KEYBOARD) ||
        (iMsg == LW_HOTPLUG_MSG_USB_MOUSE)    ||
        (iMsg == LW_HOTPLUG_MSG_PCI_INPUT)) {
        if (pucMsg[4]) {
            LW_SPIN_LOCK_QUICK(&_G_slGidDev, &iregInterLevel);
            _G_bIsHotplugIn = LW_TRUE;
            LW_SPIN_UNLOCK_QUICK(&_G_slGidDev, iregInterLevel);
        }
    }
}
/*********************************************************************************************************
** 函数名称: __guiInputDevCleanup
** 功能描述: gui 输入线程中止时的清除处理
** 输　入  : lIsIn         是否为插入事件
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __guiInputDevCleanup (VOID)
{
    __guiInputDevCloseAll(__GID_KEYBOARD);
    __guiInputDevCloseAll(__GID_MOUSE);
    
    _G_ulGidThread  = LW_HANDLE_INVALID;
    _G_bIsHotplugIn = LW_TRUE;                                          /*  需要重新打开文件            */
    
    if (_G_iHotplugFd >= 0) {
        close(_G_iHotplugFd);
        _G_iHotplugFd = PX_ERROR;
    }
}
/*********************************************************************************************************
** 函数名称: __guiInputExitCheck
** 功能描述: gui 输入线程检查是否需要退出
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __guiInputExitCheck (VOID)
{
    if (_G_bIsNeedStop) {
        API_GuiInputDevKeyboardHookSet((VOIDFUNCPTR)LW_NULL);
        API_GuiInputDevMouseHookSet((VOIDFUNCPTR)LW_NULL);
        API_ThreadExit(LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: __guiInputDevProc
** 功能描述: gui 输入设备处理线程
** 输　入  : pvArg     目前未使用
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PVOID  __guiInputDevProc (PVOID  pvArg)
{
             INTREG                 iregInterLevel;
             INT                    i;
             INT                    iWidth;
             INT                    iRet;
             fd_set                 fdsetInput;
             struct timeval         timevalTO = {LW_CFG_GUI_INPUT_DEV_TIMEOUT, 0};

             INT                    iNum;
    REGISTER __PGUI_INPUT_DEV       pgidArray;
             VOIDFUNCPTR            pfuncTemp;

             keyboard_event_notify  knotify;
             mouse_event_notify     mnotify[__GID_MAX_POINT];

             BOOL                   bIsNeedCheck;

    (VOID)pvArg;
    
    API_ThreadCleanupPush(__guiInputDevCleanup, LW_NULL);               /*  清除函数入栈                */

    _G_iHotplugFd = open(LW_HOTPLUG_DEV_PATH, O_RDONLY);                /*  打开热插拔消息文件          */

    for (;;) {
        FD_ZERO(&fdsetInput);                                           /*  清空文件集                  */

        LW_SPIN_LOCK_QUICK(&_G_slGidDev, &iregInterLevel);
        bIsNeedCheck    = _G_bIsHotplugIn;
        _G_bIsHotplugIn = LW_FALSE;
        LW_SPIN_UNLOCK_QUICK(&_G_slGidDev, iregInterLevel);

        if (bIsNeedCheck) {
            /*
             *  试图打开设备
             */
            __guiInputDevTryOpen(__GID_KEYBOARD);
            __guiInputDevTryOpen(__GID_MOUSE);
        }

        /*
         *  将关心的所有打开的文件全部添加到输入文件集
         */
        iRet   = __guiInputDevMakeFdset(__GID_KEYBOARD, &fdsetInput);
        iWidth = __guiInputDevMakeFdset(__GID_MOUSE,    &fdsetInput);
        iWidth = (iWidth > iRet) ? iWidth : iRet;
        
        if (_G_iHotplugFd >= 0) {
            FD_SET(_G_iHotplugFd, &fdsetInput);
            iWidth = (iWidth > _G_iHotplugFd) ? iWidth : _G_iHotplugFd;
        }
        
        iWidth += 1;                                                    /*  最大文件号 + 1              */

        iRet = select(iWidth, &fdsetInput, LW_NULL, LW_NULL, &timevalTO);
        if (iRet < 0) {
            /*
             *  select() 出现错误, 可能是设备驱动不支持或者设备文件产生了变动.
             *  这里等待一段时间, 不允许此线程由于异常造成 CPU 利用率过高.
             */
            __guiInputDevCloseAll(__GID_KEYBOARD);
            __guiInputDevCloseAll(__GID_MOUSE);

            LW_SPIN_LOCK_QUICK(&_G_slGidDev, &iregInterLevel);
            _G_bIsHotplugIn = LW_TRUE;                                  /*  需要重新打开文件            */
            LW_SPIN_UNLOCK_QUICK(&_G_slGidDev, iregInterLevel);

            __guiInputExitCheck();                                      /*  检查是否请求退出            */
            sleep(LW_CFG_GUI_INPUT_DEV_TIMEOUT);                        /*  等待一段时间重试            */
            continue;

        } else if (iRet == 0) {
            /*
             *  select() 超时, 没有任何设备有可读取事件发生, 重新等待.
             */
            __guiInputExitCheck();                                      /*  检查是否请求退出            */
            continue;

        } else {
            /*
             *  有文件产生了可读取事件, 首先判断键盘事件
             */
            ssize_t     sstTemp;
            
            if (_G_iHotplugFd >= 0 && 
                FD_ISSET(_G_iHotplugFd, &fdsetInput)) {                 /*  热插拔消息                  */
                UCHAR   ucMsg[LW_HOTPLUG_DEV_MAX_MSGSIZE];
                sstTemp = read(_G_iHotplugFd, ucMsg, LW_HOTPLUG_DEV_MAX_MSGSIZE);
                if (sstTemp > 0) {
                    __guiInputDevHotplugMsg(ucMsg, (size_t)sstTemp);
                }
            }

            pgidArray = _G_pgidArray[__GID_KEYBOARD];
            iNum      = _G_iGidDevNum[__GID_KEYBOARD];

            LW_SPIN_LOCK_QUICK(&_G_slGidDev, &iregInterLevel);
            pfuncTemp = _G_pfuncGidDevNotify[__GID_KEYBOARD];
            LW_SPIN_UNLOCK_QUICK(&_G_slGidDev, iregInterLevel);

            for (i = 0; i < iNum; i++, pgidArray++) {
                if(pgidArray->GID_iFd >= 0) {                           /*  忽略未打开文件              */
                    if (FD_ISSET(pgidArray->GID_iFd, &fdsetInput)) {
                        sstTemp = read(pgidArray->GID_iFd, (PVOID)&knotify, sizeof(knotify));
                        if (sstTemp <= 0) {
                            close(pgidArray->GID_iFd);                  /*  关闭异常的文件              */
                            pgidArray->GID_iFd = PX_ERROR;

                        } else {
                            if (pfuncTemp) {
                                LW_SOFUNC_PREPARE(pfuncTemp);
                                pfuncTemp(&knotify, 1);                 /*  通知用户回调函数            */
                            }
                        }
                    }
                }
            }

            /*
             *  判断鼠标事件
             */
            pgidArray = _G_pgidArray[__GID_MOUSE];
            iNum      = _G_iGidDevNum[__GID_MOUSE];

            LW_SPIN_LOCK_QUICK(&_G_slGidDev, &iregInterLevel);
            pfuncTemp = _G_pfuncGidDevNotify[__GID_MOUSE];
            LW_SPIN_UNLOCK_QUICK(&_G_slGidDev, iregInterLevel);

            for (i = 0; i < iNum; i++, pgidArray++) {
                if(pgidArray->GID_iFd >= 0) {                           /*  忽略未打开文件              */
                    if (FD_ISSET(pgidArray->GID_iFd, &fdsetInput)) {
                        sstTemp = read(pgidArray->GID_iFd, (PVOID)mnotify, 
                                       (sizeof(mouse_event_notify) * __GID_MAX_POINT));
                        if (sstTemp <= 0) {
                            close(pgidArray->GID_iFd);                  /*  关闭异常的文件              */
                            pgidArray->GID_iFd = PX_ERROR;

                        } else {
                            if (pfuncTemp) {                            /*  通知用户回调函数            */
                                LW_SOFUNC_PREPARE(pfuncTemp);
                                pfuncTemp(mnotify, (sstTemp / sizeof(mouse_event_notify)));
                            }
                        }
                    }
                }
            }
        }
        
        __guiInputExitCheck();                                          /*  检查是否请求退出            */
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_GuiInputDevReg
** 功能描述: 注册 gui 输入设备 (在调用任何 API 之前, 必须首先调用此 API 注册输入设备集)
** 输　入  : pcKeyboardName        键盘文件名数组
**           iKeyboardNum          键盘数量
**           pcMouseName           鼠标文件名数组
**           iMouseNum             鼠标数量
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_GuiInputDevReg (CPCHAR  pcKeyboardName[],
                         INT     iKeyboardNum,
                         CPCHAR  pcMouseName[],
                         INT     iMouseNum)
{
    static  BOOL    bIsInit = LW_FALSE;
            INT     iError;
    
    if (bIsInit) {
        return  (ERROR_NONE);
    }

    if (pcKeyboardName && (iKeyboardNum > 0)) {
        iError = __guiInputDevReg(__GID_KEYBOARD, pcKeyboardName, iKeyboardNum);
        if (iError < 0) {
            __guiInputDevUnreg(__GID_KEYBOARD);
            errno = ENOMEM;
            return  (PX_ERROR);
        }
    }

    if (pcMouseName && (iMouseNum > 0)) {
        iError = __guiInputDevReg(__GID_MOUSE, pcMouseName, iMouseNum);
        if (iError < 0) {
            __guiInputDevUnreg(__GID_KEYBOARD);
            __guiInputDevUnreg(__GID_MOUSE);
            errno = ENOMEM;
            return  (PX_ERROR);
        }
    }

    LW_SPIN_INIT(&_G_slGidDev);
    _G_bIsHotplugIn = LW_TRUE;                                          /*  需要打开一次设备            */

    bIsInit = LW_TRUE;

#if LW_CFG_PROCFS_EN > 0
    API_ProcFsMakeNode(&_G_pfsnInputDev[0], "/");                       /*  创建 procfs 节点            */
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_GuiInputDevProcStart
** 功能描述: 启动 GUI 输入设备线程
** 输　入  : pthreadattr       线程属性
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_GuiInputDevProcStart (PLW_CLASS_THREADATTR  pthreadattr)
{
    LW_CLASS_THREADATTR threadattr;

    if (_G_ulGidThread) {
        return  (ERROR_NONE);
    }

    if (pthreadattr) {
        threadattr = *pthreadattr;
    
    } else {
        threadattr = API_ThreadAttrGetDefault();
    }
    
    threadattr.THREADATTR_ulOption &= ~LW_OPTION_OBJECT_GLOBAL;         /*  进程内线程                  */
    
    _G_bIsNeedStop = LW_FALSE;                                          /*  没有请求退出                */

    _G_ulGidThread = API_ThreadCreate("t_gidproc", __guiInputDevProc, &threadattr, LW_NULL);
    if (_G_ulGidThread == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: API_GuiInputDevProcStop
** 功能描述: 停止 GUI 输入设备线程 (进程结束前必须调用此函数)
** 输　入  : pthreadattr       线程属性
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_GuiInputDevProcStop (VOID)
{
    _G_bIsNeedStop = LW_TRUE;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_GuiInputDevKeyboardHookSet
** 功能描述: 设置 GUI 输入设备线程钩子函数
** 输　入  : pfuncNew          新的键盘数据回调函数
** 输　出  : 旧的键盘回调函数
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
VOIDFUNCPTR  API_GuiInputDevKeyboardHookSet (VOIDFUNCPTR  pfuncNew)
{
    INTREG          iregInterLevel;
    VOIDFUNCPTR     pfuncOld;

    LW_SPIN_LOCK_QUICK(&_G_slGidDev, &iregInterLevel);
    pfuncOld = _G_pfuncGidDevNotify[__GID_KEYBOARD];
    _G_pfuncGidDevNotify[__GID_KEYBOARD] = pfuncNew;
    LW_SPIN_UNLOCK_QUICK(&_G_slGidDev, iregInterLevel);

    return  (pfuncOld);
}
/*********************************************************************************************************
** 函数名称: API_GuiInputDevMouseHookSet
** 功能描述: 设置 GUI 输入设备线程钩子函数
** 输　入  : pfuncNew          新的鼠标数据回调函数
** 输　出  : 旧的鼠标回调函数
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
VOIDFUNCPTR  API_GuiInputDevMouseHookSet (VOIDFUNCPTR  pfuncNew)
{
    INTREG          iregInterLevel;
    VOIDFUNCPTR     pfuncOld;

    LW_SPIN_LOCK_QUICK(&_G_slGidDev, &iregInterLevel);
    pfuncOld = _G_pfuncGidDevNotify[__GID_MOUSE];
    _G_pfuncGidDevNotify[__GID_MOUSE] = pfuncNew;
    LW_SPIN_UNLOCK_QUICK(&_G_slGidDev, iregInterLevel);

    return  (pfuncOld);
}

#endif                                                                  /*  LW_CFG_GUI_INPUT_DEV_EN     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
