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
** 文   件   名: monitor_event.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 08 月 29 日
**
** 描        述: SylixOS 内核事件监控器事件产生.
*********************************************************************************************************/

#ifndef __MONITOR_EVENT_H
#define __MONITOR_EVENT_H

/*********************************************************************************************************
  事件信息限制
*********************************************************************************************************/

#if LW_CFG_MONITOR_EN > 0

#define MONITOR_EVENT_MAX_SIZE      600                                 /*  事件最大长度                */

/*********************************************************************************************************
  无参数事件操作
*********************************************************************************************************/

#define MONITOR_EVT(event, sub, addstr)                                     \
        do {                                                                \
            MONITOR_TRACE_EVENT(event, sub, LW_NULL, 0, addstr);            \
        } while (0)

/*********************************************************************************************************
  UINT32 参数类型事件操作
*********************************************************************************************************/

typedef struct {
    UINT32      EVT_uiArg[1];
} LW_EVT_INT1;

typedef struct {
    UINT32      EVT_uiArg[2];
} LW_EVT_INT2;

typedef struct {
    UINT32      EVT_uiArg[3];
} LW_EVT_INT3;

typedef struct {
    UINT32      EVT_uiArg[4];
} LW_EVT_INT4;

typedef struct {
    UINT32      EVT_uiArg[5];
} LW_EVT_INT5;

#define MONITOR_EVT_INT1(event, sub, arg, addstr)                           \
        do {                                                                \
            LW_EVT_INT1     evt;                                            \
            evt.EVT_uiArg[0] = (UINT32)arg;                                 \
            MONITOR_TRACE_EVENT(event, sub,                                 \
                                (CPVOID)&evt, sizeof(evt), addstr);         \
        } while (0)
        
#define MONITOR_EVT_INT2(event, sub, arg1, arg2, addstr)                    \
        do {                                                                \
            LW_EVT_INT2     evt;                                            \
            evt.EVT_uiArg[0] = (UINT32)arg1;                                \
            evt.EVT_uiArg[1] = (UINT32)arg2;                                \
            MONITOR_TRACE_EVENT(event, sub,                                 \
                                (CPVOID)&evt, sizeof(evt), addstr);         \
        } while (0)
        
#define MONITOR_EVT_INT3(event, sub, arg1, arg2, arg3, addstr)              \
        do {                                                                \
            LW_EVT_INT3     evt;                                            \
            evt.EVT_uiArg[0] = (UINT32)arg1;                                \
            evt.EVT_uiArg[1] = (UINT32)arg2;                                \
            evt.EVT_uiArg[2] = (UINT32)arg3;                                \
            MONITOR_TRACE_EVENT(event, sub,                                 \
                                (CPVOID)&evt, sizeof(evt), addstr);         \
        } while (0)
        
#define MONITOR_EVT_INT4(event, sub, arg1, arg2, arg3, arg4, addstr)        \
        do {                                                                \
            LW_EVT_INT4     evt;                                            \
            evt.EVT_uiArg[0] = (UINT32)arg1;                                \
            evt.EVT_uiArg[1] = (UINT32)arg2;                                \
            evt.EVT_uiArg[2] = (UINT32)arg3;                                \
            evt.EVT_uiArg[3] = (UINT32)arg4;                                \
            MONITOR_TRACE_EVENT(event, sub,                                 \
                                (CPVOID)&evt, sizeof(evt), addstr);         \
        } while (0)
        
#define MONITOR_EVT_INT5(event, sub, arg1, arg2, arg3, arg4, arg5, addstr)  \
        do {                                                                \
            LW_EVT_INT5     evt;                                            \
            evt.EVT_uiArg[0] = (UINT32)arg1;                                \
            evt.EVT_uiArg[1] = (UINT32)arg2;                                \
            evt.EVT_uiArg[2] = (UINT32)arg3;                                \
            evt.EVT_uiArg[3] = (UINT32)arg4;                                \
            evt.EVT_uiArg[4] = (UINT32)arg5;                                \
            MONITOR_TRACE_EVENT(event, sub,                                 \
                                (CPVOID)&evt, sizeof(evt), addstr);         \
        } while (0)
        
/*********************************************************************************************************
  LONG 参数类型事件操作
*********************************************************************************************************/

typedef struct {
    LONG       EVT_lArg[1];
} LW_EVT_LONG1;

typedef struct {
    LONG       EVT_lArg[2];
} LW_EVT_LONG2;

typedef struct {
    LONG       EVT_lArg[3];
} LW_EVT_LONG3;

typedef struct {
    LONG       EVT_lArg[4];
} LW_EVT_LONG4;

typedef struct {
    LONG       EVT_lArg[5];
} LW_EVT_LONG5;

#define MONITOR_EVT_LONG1(event, sub, arg, addstr)                          \
        do {                                                                \
            LW_EVT_LONG1   evt;                                             \
            evt.EVT_lArg[0] = (LONG)arg;                                    \
            MONITOR_TRACE_EVENT(event, sub,                                 \
                                (CPVOID)&evt, sizeof(evt), addstr);         \
        } while (0)
        
#define MONITOR_EVT_LONG2(event, sub, arg1, arg2, addstr)                   \
        do {                                                                \
            LW_EVT_LONG2   evt;                                             \
            evt.EVT_lArg[0] = (LONG)arg1;                                   \
            evt.EVT_lArg[1] = (LONG)arg2;                                   \
            MONITOR_TRACE_EVENT(event, sub,                                 \
                                (CPVOID)&evt, sizeof(evt), addstr);         \
        } while (0)
        
#define MONITOR_EVT_LONG3(event, sub, arg1, arg2, arg3, addstr)             \
        do {                                                                \
            LW_EVT_LONG3   evt;                                             \
            evt.EVT_lArg[0] = (LONG)arg1;                                   \
            evt.EVT_lArg[1] = (LONG)arg2;                                   \
            evt.EVT_lArg[2] = (LONG)arg3;                                   \
            MONITOR_TRACE_EVENT(event, sub,                                 \
                                (CPVOID)&evt, sizeof(evt), addstr);         \
        } while (0)
        
#define MONITOR_EVT_LONG4(event, sub, arg1, arg2, arg3, arg4, addstr)       \
        do {                                                                \
            LW_EVT_LONG4   evt;                                             \
            evt.EVT_lArg[0] = (LONG)arg1;                                   \
            evt.EVT_lArg[1] = (LONG)arg2;                                   \
            evt.EVT_lArg[2] = (LONG)arg3;                                   \
            evt.EVT_lArg[3] = (LONG)arg4;                                   \
            MONITOR_TRACE_EVENT(event, sub,                                 \
                                (CPVOID)&evt, sizeof(evt), addstr);         \
        } while (0)
        
#define MONITOR_EVT_LONG5(event, sub, arg1, arg2, arg3, arg4, arg5, addstr) \
        do {                                                                \
            LW_EVT_LONG5   evt;                                             \
            evt.EVT_lArg[0] = (LONG)arg1;                                   \
            evt.EVT_lArg[1] = (LONG)arg2;                                   \
            evt.EVT_lArg[2] = (LONG)arg3;                                   \
            evt.EVT_lArg[3] = (LONG)arg4;                                   \
            evt.EVT_lArg[4] = (LONG)arg5;                                   \
            MONITOR_TRACE_EVENT(event, sub,                                 \
                                (CPVOID)&evt, sizeof(evt), addstr);         \
        } while (0)
        
/*********************************************************************************************************
  UINT64 参数类型事件操作
*********************************************************************************************************/

typedef struct {
    UINT64      EVT_u64Arg[1];
} LW_EVT_D1;

typedef struct {
    UINT64      EVT_u64Arg[2];
} LW_EVT_D2;

typedef struct {
    UINT64      EVT_u64Arg[3];
} LW_EVT_D3;

typedef struct {
    UINT64      EVT_u64Arg[4];
} LW_EVT_D4;

typedef struct {
    UINT64      EVT_u64Arg[5];
} LW_EVT_D5;

#define MONITOR_EVT_D1(event, sub, arg, addstr)                             \
        do {                                                                \
            LW_EVT_D1     evt;                                              \
            evt.EVT_u64Arg[0] = (UINT64)arg;                                \
            MONITOR_TRACE_EVENT(event, sub,                                 \
                                (CPVOID)&evt, sizeof(evt), addstr);         \
        } while (0)
        
#define MONITOR_EVT_D2(event, sub, arg1, arg2, addstr)                      \
        do {                                                                \
            LW_EVT_D2     evt;                                              \
            evt.EVT_u64Arg[0] = (UINT64)arg1;                               \
            evt.EVT_u64Arg[1] = (UINT64)arg2;                               \
            MONITOR_TRACE_EVENT(event, sub,                                 \
                                (CPVOID)&evt, sizeof(evt), addstr);         \
        } while (0)
        
#define MONITOR_EVT_D3(event, sub, arg1, arg2, arg3, addstr)                \
        do {                                                                \
            LW_EVT_D3     evt;                                              \
            evt.EVT_u64Arg[0] = (UINT64)arg1;                               \
            evt.EVT_u64Arg[1] = (UINT64)arg2;                               \
            evt.EVT_u64Arg[2] = (UINT64)arg3;                               \
            MONITOR_TRACE_EVENT(event, sub,                                 \
                                (CPVOID)&evt, sizeof(evt), addstr);         \
        } while (0)
        
#define MONITOR_EVT_D4(event, sub, arg1, arg2, arg3, arg4, addstr)          \
        do {                                                                \
            LW_EVT_D4     evt;                                              \
            evt.EVT_u64Arg[0] = (UINT64)arg1;                               \
            evt.EVT_u64Arg[1] = (UINT64)arg2;                               \
            evt.EVT_u64Arg[2] = (UINT64)arg3;                               \
            evt.EVT_u64Arg[3] = (UINT64)arg4;                               \
            MONITOR_TRACE_EVENT(event, sub,                                 \
                                (CPVOID)&evt, sizeof(evt), addstr);         \
        } while (0)
        
#define MONITOR_EVT_D5(event, sub, arg1, arg2, arg3, arg4, arg5, addstr)    \
        do {                                                                \
            LW_EVT_D5     evt;                                              \
            evt.EVT_u64Arg[0] = (UINT64)arg1;                               \
            evt.EVT_u64Arg[1] = (UINT64)arg2;                               \
            evt.EVT_u64Arg[2] = (UINT64)arg3;                               \
            evt.EVT_u64Arg[3] = (UINT64)arg4;                               \
            evt.EVT_u64Arg[4] = (UINT64)arg5;                               \
            MONITOR_TRACE_EVENT(event, sub,                                 \
                                (CPVOID)&evt, sizeof(evt), addstr);         \
        } while (0)
        
#else

#define MONITOR_EVT(event, sub, addstr)

#define MONITOR_EVT_INT1(event, sub, arg, addstr)
#define MONITOR_EVT_INT2(event, sub, arg1, arg2, addstr)
#define MONITOR_EVT_INT3(event, sub, arg1, arg2, arg3, addstr)
#define MONITOR_EVT_INT4(event, sub, arg1, arg2, arg3, arg4, addstr)
#define MONITOR_EVT_INT5(event, sub, arg1, arg2, arg3, arg4, arg5, addstr)

#define MONITOR_EVT_LONG1(event, sub, arg, addstr)
#define MONITOR_EVT_LONG2(event, sub, arg1, arg2, addstr)
#define MONITOR_EVT_LONG3(event, sub, arg1, arg2, arg3, addstr)
#define MONITOR_EVT_LONG4(event, sub, arg1, arg2, arg3, arg4, addstr)
#define MONITOR_EVT_LONG5(event, sub, arg1, arg2, arg3, arg4, arg5, addstr)

#define MONITOR_EVT_D1(event, sub, arg, addstr)
#define MONITOR_EVT_D2(event, sub, arg1, arg2, addstr)
#define MONITOR_EVT_D3(event, sub, arg1, arg2, arg3, addstr)
#define MONITOR_EVT_D4(event, sub, arg1, arg2, arg3, arg4, addstr)
#define MONITOR_EVT_D5(event, sub, arg1, arg2, arg3, arg4, arg5, addstr)

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */

#endif                                                                  /*  __MONITOR_EVENT_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
