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
** 文   件   名: Lw_Class_Kernel.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 这是系统提供给 C / C++ 用户的内核对象类型层。用于 Lw_Object_GetClass 返回判断
*********************************************************************************************************/

#ifndef __LW_CLASS_KERNEL_H
#define __LW_CLASS_KERNEL_H

#define LW_OBJECT_CLASS_THREAD                _OBJECT_THREAD            /*  线程                        */
#define LW_OBJECT_CLASS_SEM_C                 _OBJECT_SEM_C             /*  计数型信号量                */
#define LW_OBJECT_CLASS_SEM_B                 _OBJECT_SEM_B             /*  二值型信号量                */
#define LW_OBJECT_CLASS_SEM_M                 _OBJECT_SEM_M             /*  互斥型信号量                */
#define LW_OBJECT_CLASS_SEM_RW                _OBJECT_SEM_RW            /*  读写信号量                  */
#define LW_OBJECT_CLASS_MSGQUEUE              _OBJECT_MSGQUEUE          /*  消息队列                    */
#define LW_OBJECT_CLASS_EVENT_SET             _OBJECT_EVENT_SET         /*  事件集                      */
#define LW_OBJECT_CLASS_TIMER                 _OBJECT_TIMER             /*  定时器                      */
#define LW_OBJECT_CLASS_PARTITION             _OBJECT_PARTITION         /*  内存块分区                  */
#define LW_OBJECT_CLASS_REGION                _OBJECT_REGION            /*  内存可变分区                */
#define LW_OBJECT_CLASS_RMS                   _OBJECT_RMS               /*  精度单调器                  */

#endif                                                                  /*  __LW_CLASS_KERNEL_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
