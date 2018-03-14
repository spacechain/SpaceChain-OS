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
** 文   件   名: inlObjectCheck.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 18 日
**
** 描        述: 这是系统对象检查函数。

** BUG
2007.04.10  _Heap_Index_Invalid() 中的判断数改为：(LW_CFG_MAX_REGIONS + 2)，因为系统包括两个系统堆内存
2013.12.14  加入 object 名字长度检查
*********************************************************************************************************/

#ifndef  __INLOBJECTCHECK_H
#define  __INLOBJECTCHECK_H

/*********************************************************************************************************
  _Object_Name_Invalid
*********************************************************************************************************/

static LW_INLINE BOOL  _Object_Name_Invalid (CPCHAR  pcName)
{
    return  ((pcName == LW_NULL) ? LW_FALSE : 
             (lib_strnlen(pcName, LW_CFG_OBJECT_NAME_SIZE) >= LW_CFG_OBJECT_NAME_SIZE));
}

/*********************************************************************************************************
  _Inter_Vector_Invalid
*********************************************************************************************************/

static LW_INLINE INT  _Inter_Vector_Invalid (ULONG    ulVector)
{
    return  (ulVector >= LW_CFG_MAX_INTER_SRC);
}

/*********************************************************************************************************
  _Thread_Invalid
*********************************************************************************************************/

static LW_INLINE INT  _Thread_Invalid (UINT16    usIndex)
{
    return  (!_K_ptcbTCBIdTable[usIndex]);
}

/*********************************************************************************************************
  _Thread_Index_Invalid (except idle thread)
*********************************************************************************************************/

static LW_INLINE INT  _Thread_Index_Invalid (UINT16    usIndex)
{
    return  ((usIndex >= LW_CFG_MAX_THREADS) || (usIndex < LW_NCPUS));
}

/*********************************************************************************************************
  _Event_Index_Invalid
*********************************************************************************************************/

static LW_INLINE INT  _Event_Index_Invalid (UINT16    usIndex)
{
    return  (usIndex >= LW_CFG_MAX_EVENTS);
}

/*********************************************************************************************************
  _Event_Type_Invalid
*********************************************************************************************************/
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

static LW_INLINE INT  _Event_Type_Invalid (UINT16    usIndex, UINT8    ucType)
{
    return  (_K_eventBuffer[usIndex].EVENT_ucType != ucType);
}

#endif                                                                  /*  (LW_CFG_EVENT_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  _EventSet_Index_Invalid
*********************************************************************************************************/

static LW_INLINE INT  _EventSet_Index_Invalid (UINT16    usIndex)
{
    return  (usIndex >= LW_CFG_MAX_EVENTSETS);
}

/*********************************************************************************************************
  _EventSet_Type_Invalid
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

static LW_INLINE INT  _EventSet_Type_Invalid (UINT16    usIndex, UINT8    ucType)
{
    return  (_K_esBuffer[usIndex].EVENTSET_ucType != ucType);
}

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  _Timer_Index_Invalid
*********************************************************************************************************/

static LW_INLINE INT  _Timer_Index_Invalid (UINT16    usIndex)
{
    return  (usIndex >= LW_CFG_MAX_TIMERS);
}

/*********************************************************************************************************
  _Timer_Type_Invalid
*********************************************************************************************************/
#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS)

static LW_INLINE INT  _Timer_Type_Invalid (UINT16    usIndex)
{
    return  (_K_tmrBuffer[usIndex].TIMER_ucType == LW_TYPE_TIMER_UNUSED);
}

#endif                                                                  /*  ((LW_CFG_HTIMER_EN > 0)     */
                                                                        /*  (LW_CFG_ITIMER_EN > 0))     */
                                                                        /*  (LW_CFG_MAX_TIMERS)         */
/*********************************************************************************************************
  _Partition_Index_Invalid
*********************************************************************************************************/

static LW_INLINE INT  _Partition_Index_Invalid (UINT16    usIndex)
{
    return  (usIndex >= LW_CFG_MAX_PARTITIONS);
}

/*********************************************************************************************************
  _Partition_Type_Invalid
*********************************************************************************************************/
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)

static LW_INLINE INT  _Partition_Type_Invalid (UINT16    usIndex)
{
    return  (_K__partBuffer[usIndex].PARTITION_ucType == LW_PARTITION_UNUSED);
}

#endif                                                                  /*  (LW_CFG_PARTITION_EN > 0)   */
                                                                        /*  (LW_CFG_MAX_PARTITIONS > 0) */
/*********************************************************************************************************
  _Heap_Index_Invalid
*********************************************************************************************************/

static LW_INLINE INT  _Heap_Index_Invalid (UINT16    usIndex)
{
    return  (usIndex >= (LW_CFG_MAX_REGIONS + 2));
}

/*********************************************************************************************************
  _Heap_ByteSize_Invalid
*********************************************************************************************************/

static LW_INLINE INT  _Heap_ByteSize_Invalid (ULONG    ulByteSize)
{
    return  (ulByteSize < (__SEGMENT_BLOCK_SIZE_ALIGN + LW_CFG_HEAP_SEG_MIN_SIZE));
}

/*********************************************************************************************************
  _Rms_Index_Invalid
*********************************************************************************************************/

static LW_INLINE INT  _Rms_Index_Invalid (UINT16    usIndex)
{
    return  (usIndex >= LW_CFG_MAX_RMSS);
}

/*********************************************************************************************************
  _Rms_Type_Invalid
*********************************************************************************************************/
#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)

static LW_INLINE INT  _Rms_Type_Invalid (UINT16    usIndex)
{
    return  (_K_rmsBuffer[usIndex].RMS_ucType == LW_RMS_UNUSED);
}

#endif                                                                  /*  (LW_CFG_RMS_EN > 0)         */
                                                                        /*  (LW_CFG_MAX_RMSS > 0)       */
#endif                                                                  /*  __INLOBJECTCHECK_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
