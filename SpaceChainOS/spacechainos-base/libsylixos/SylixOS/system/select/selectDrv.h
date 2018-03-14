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
** 文   件   名: selectDrv.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 11 月 07 日
**
** 描        述:  IO 系统 select 子系统驱动程序需要使用的函数声明.

** 注意:请谨慎使用以下函数,这些函数可以使任何设备的驱动程序支持 select() 函数调用.

** BUG
2007.12.11 加入了错误激活的处理.
*********************************************************************************************************/

#ifndef __SELECTDRV_H 
#define __SELECTDRV_H

/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)

LW_API VOID         API_SelWakeupListInit(PLW_SEL_WAKEUPLIST  pselwulList);
LW_API VOID         API_SelWakeupListTerm(PLW_SEL_WAKEUPLIST  pselwulList);
LW_API UINT         API_SelWakeupListLen( PLW_SEL_WAKEUPLIST  pselwulList);

LW_API LW_SEL_TYPE  API_SelWakeupType( PLW_SEL_WAKEUPNODE   pselwunNode);
LW_API VOID         API_SelWakeup(     PLW_SEL_WAKEUPNODE   pselwunNode);
LW_API VOID         API_SelWakeupError(PLW_SEL_WAKEUPNODE   pselwunNode);

LW_API VOID         API_SelWakeupAll( PLW_SEL_WAKEUPLIST   pselwulList, LW_SEL_TYPE  seltyp);
LW_API VOID         API_SelWakeupTerm(PLW_SEL_WAKEUPLIST   pselwulList);

LW_API INT          API_SelNodeAdd(   PLW_SEL_WAKEUPLIST   pselwulList, 
                                      PLW_SEL_WAKEUPNODE   pselwunNode);
LW_API INT          API_SelNodeDelete(PLW_SEL_WAKEUPLIST   pselwulList, 
                                      PLW_SEL_WAKEUPNODE   pselwunNodeDelete);

/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SELECT_EN > 0)      */
/*********************************************************************************************************
  驱动程序使用宏
*********************************************************************************************************/

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)

#define SEL_WAKE_UP_LIST_INIT(pselwulList)      API_SelWakeupListInit(pselwulList)
#define SEL_WAKE_UP_LIST_TERM(pselwulList)      API_SelWakeupListTerm(pselwulList)
#define SEL_WAKE_UP_LIST_LEN(pselwulList)       API_SelWakeupListLen(pselwulList)

#define SEL_WAKE_UP_TYPE(pselwunNode)           API_SelWakeupType(pselwunNode)
#define SEL_WAKE_UP(pselwunNode)                API_SelWakeup(pselwunNode)
#define SEL_WAKE_UP_ERROR(pselwunNode)          API_SelWakeupError(pselwunNode)

#define SEL_WAKE_UP_ALL(pselwulList, seltyp)                    \
        API_SelWakeupAll(pselwulList, seltyp)
        
#define SEL_WAKE_UP_TERM(pselwulList)                           \
        API_SelWakeupTerm(pselwulList)

#define SEL_WAKE_NODE_ADD(pselwulList, pselwunNode)             \
        API_SelNodeAdd(pselwulList, pselwunNode)
        
#define SEL_WAKE_NODE_DELETE(pselwulList, pselwunNodeDelete)    \
        API_SelNodeDelete(pselwulList, pselwunNodeDelete)
        
#else                                                                   /*  裁减了 select() 库          */

static LW_INLINE VOID SEL_WAKE_UP_LIST_INIT(PLW_SEL_WAKEUPLIST  pselwulList)
{
    return;
}
static LW_INLINE VOID SEL_WAKE_UP_LIST_TERM(PLW_SEL_WAKEUPLIST  pselwulList)
{
    return;
}
static LW_INLINE UINT SEL_WAKE_UP_LIST_LEN(PLW_SEL_WAKEUPLIST  pselwulList)
{
    return  (0);
}
static LW_INLINE LW_SEL_TYPE SEL_WAKE_UP_TYPE(PLW_SEL_WAKEUPNODE   pselwunNode)
{
    return  (SELEXCEPT);
}
static LW_INLINE VOID SEL_WAKE_UP(PLW_SEL_WAKEUPNODE   pselwunNode)
{
    return;
}
static LW_INLINE VOID SEL_WAKE_UP_ERROR(PLW_SEL_WAKEUPNODE   pselwunNode)
{
    return;
}
static LW_INLINE VOID SEL_WAKE_UP_ALL( PLW_SEL_WAKEUPLIST   pselwulList, LW_SEL_TYPE  seltyp)
{
    return;
}
static LW_INLINE VOID SEL_WAKE_UP_TERM( PLW_SEL_WAKEUPLIST   pselwulList)
{
    return;
}
static LW_INLINE INT  SEL_WAKE_NODE_ADD(PLW_SEL_WAKEUPLIST   pselwulList, 
                              PLW_SEL_WAKEUPNODE   pselwunNode)
{
    return  (PX_ERROR);
}
static LW_INLINE INT  SEL_WAKE_NODE_DELETE(PLW_SEL_WAKEUPLIST   pselwulList, 
                                 PLW_SEL_WAKEUPNODE   pselwunNodeDelete)
{
    return  (PX_ERROR);
}

/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SELECT_EN > 0)      */
#endif                                                                  /*  __SELECTDRV_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
