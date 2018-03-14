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
** 文   件   名: inlErrorHandle.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 这是系统当前错误保存函数

** BUG
2007.03.22  加入系统没有启动时的错误处理机制
2007.11.04  加入 posix error 处理.
2008.03.28  修改了 error handle 的通道选择
2009.03.10  修正的 errno 的相关机制, 可以选择 API 成功立即清除 errno, 或者期待用户清除.
            用户清除指定使用 errno = 0;
2009.06.30  _DebugHandle() 对错误的处理加入 __FILE__ 和 __LINE__ 进行定位, 但是有些编译器不支持 __func__
            __FUNCTION__ 所以, 源码中继续保留函数名.
2009.07.09  加入对线程名的显示.
2012.07.21  不再使用宏. 所有操作全部放在 _ErrorLib.c 中实现
*********************************************************************************************************/

#ifndef  __INLERRORHANDLE_H
#define  __INLERRORHANDLE_H

/*********************************************************************************************************
  错误保存句柄
*********************************************************************************************************/

VOID  _ErrorHandle(ULONG  ulErrorCode);

/*********************************************************************************************************
  错误打印
*********************************************************************************************************/

VOID  _DebugMessage(INT  iLevel, CPCHAR  pcPosition, CPCHAR  pcString);

#if LW_CFG_ERRORMESSAGE_EN > 0 || LW_CFG_LOGMESSAGE_EN > 0
#define _DebugHandle(level, msg) \
        (((level) & __ERRORMESSAGE_LEVEL) ? \
         _DebugMessage((level), __func__, (msg)) : \
         _DebugMessage((level), LW_NULL, (msg)))
#else
#define _DebugHandle(level, msg)
#endif                                                                  /*  LW_CFG_ERRORMESSAGE_EN > 0  */
                                                                        /*  LW_CFG_LOGMESSAGE_EN > 0    */
/*********************************************************************************************************
  错误格式化打印
*********************************************************************************************************/

VOID  _DebugFmtMsg(INT  iLevel, CPCHAR  pcPosition, CPCHAR  pcFmt, ...);

#if LW_CFG_ERRORMESSAGE_EN > 0 || LW_CFG_LOGMESSAGE_EN > 0
#define _DebugFormat(level, fmt, ...)   \
        (((level) & __ERRORMESSAGE_LEVEL) ? \
         _DebugFmtMsg((level), __func__, (fmt), ##__VA_ARGS__) : \
         _DebugFmtMsg((level), LW_NULL, (fmt), ##__VA_ARGS__))
#else
#define _DebugFormat(level, fmt, ...)
#endif                                                                  /*  LW_CFG_ERRORMESSAGE_EN > 0  */
                                                                        /*  LW_CFG_LOGMESSAGE_EN > 0    */
/*********************************************************************************************************
  BUG 打印
*********************************************************************************************************/

#if LW_CFG_BUGMESSAGE_EN > 0
#define _BugHandle(cond, stop, msg) \
        if (cond) {     \
            _DebugMessage(__BUGMESSAGE_LEVEL, __func__, (msg)); \
            if (LW_KERN_BUG_REBOOT_EN_GET()) {  \
                archReboot(LW_REBOOT_FORCE, 0ul);   \
            }   \
            if (stop) { \
                for (;;);   \
            }   \
        }
#else
#define _BugHandle(cond, stop, msg) \
        if (cond) {     \
            if (LW_KERN_BUG_REBOOT_EN_GET()) {  \
                archReboot(LW_REBOOT_FORCE, 0ul);   \
            }   \
            if (stop) { \
                for (;;);   \
            }   \
        }
#endif                                                                  /*  LW_CFG_BUGMESSAGE_EN > 0    */

/*********************************************************************************************************
  BUG 格式化打印
*********************************************************************************************************/

#if LW_CFG_BUGMESSAGE_EN > 0
#define _BugFormat(cond, stop, fmt, ...)    \
        if (cond) {     \
            _DebugFmtMsg(__BUGMESSAGE_LEVEL, __func__, (fmt), ##__VA_ARGS__); \
            if (LW_KERN_BUG_REBOOT_EN_GET()) {  \
                archReboot(LW_REBOOT_FORCE, 0ul);   \
            }   \
            if (stop) { \
                for (;;);   \
            }   \
        }
#else
#define _BugFormat(cond, stop, fmt, ...)    \
        if (cond) {     \
            if (LW_KERN_BUG_REBOOT_EN_GET()) {  \
                archReboot(LW_REBOOT_FORCE, 0ul);   \
            }   \
            if (stop) { \
                for (;;);   \
            }   \
        }
#endif                                                                  /*  LW_CFG_BUGMESSAGE_EN > 0    */

/*********************************************************************************************************
  PRINT 打印
*********************************************************************************************************/

#define _PrintHandle(msg)   \
        _DebugMessage(__PRINTMESSAGE_LEVEL, LW_NULL, msg)
#define _PrintFormat(fmt, ...)  \
        _DebugFmtMsg(__PRINTMESSAGE_LEVEL, LW_NULL, fmt, ##__VA_ARGS__)

#endif                                                                  /*  __INLERRORHANDLE_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
