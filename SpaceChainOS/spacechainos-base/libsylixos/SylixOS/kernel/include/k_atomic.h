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
** 文   件   名: k_atomic.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 09 月 30 日
**
** 描        述: 系统内核原子操作.
*********************************************************************************************************/

#ifndef __K_ATOMIC_H
#define __K_ATOMIC_H

/*********************************************************************************************************
  根据系统配置, 选择锁类型
*********************************************************************************************************/

#define __LW_ATOMIC_LOCK(iregInterLevel)    { LW_SPIN_LOCK_RAW(&_K_slAtomic, &iregInterLevel); }
#define __LW_ATOMIC_UNLOCK(iregInterLevel)  { LW_SPIN_UNLOCK_RAW(&_K_slAtomic, iregInterLevel); }

/*********************************************************************************************************
  基础操作
*********************************************************************************************************/

static LW_INLINE INT  __LW_ATOMIC_ADD (INT  iVal, atomic_t  *patomic)
{
             INTREG  iregInterLevel;
    REGISTER INT     iRet;
    
    __LW_ATOMIC_LOCK(iregInterLevel);
    patomic->counter += iVal;
    iRet = patomic->counter;
    __LW_ATOMIC_UNLOCK(iregInterLevel);
    
    return  (iRet);
}

static LW_INLINE INT  __LW_ATOMIC_SUB (INT  iVal, atomic_t  *patomic)
{
             INTREG  iregInterLevel;
    REGISTER INT     iRet;
    
    __LW_ATOMIC_LOCK(iregInterLevel);
    patomic->counter -= iVal;
    iRet = patomic->counter;
    __LW_ATOMIC_UNLOCK(iregInterLevel);
    
    return  (iRet);
}

static LW_INLINE INT  __LW_ATOMIC_INC (atomic_t  *patomic)
{
    return  (__LW_ATOMIC_ADD(1, patomic));
}

static LW_INLINE INT  __LW_ATOMIC_DEC (atomic_t  *patomic)
{
    return  (__LW_ATOMIC_SUB(1, patomic));
}

static LW_INLINE INT  __LW_ATOMIC_AND (INT  iVal, atomic_t  *patomic)
{
             INTREG  iregInterLevel;
    REGISTER INT     iRet;
    
    __LW_ATOMIC_LOCK(iregInterLevel);
    iRet = patomic->counter & iVal;
    patomic->counter = iRet;
    __LW_ATOMIC_UNLOCK(iregInterLevel);
    
    return  (iRet);
}

static LW_INLINE INT  __LW_ATOMIC_NAND (INT  iVal, atomic_t  *patomic)
{
             INTREG  iregInterLevel;
    REGISTER INT     iRet;
    
    __LW_ATOMIC_LOCK(iregInterLevel);
    iRet = patomic->counter & iVal;
    patomic->counter = ~iRet;
    iRet = patomic->counter;
    __LW_ATOMIC_UNLOCK(iregInterLevel);
    
    return  (iRet);
}

static LW_INLINE INT  __LW_ATOMIC_OR (INT  iVal, atomic_t  *patomic)
{
             INTREG  iregInterLevel;
    REGISTER INT     iRet;
    
    __LW_ATOMIC_LOCK(iregInterLevel);
    iRet = patomic->counter | iVal;
    patomic->counter = iRet;
    __LW_ATOMIC_UNLOCK(iregInterLevel);
    
    return  (iRet);
}

static LW_INLINE INT  __LW_ATOMIC_XOR (INT  iVal, atomic_t  *patomic)
{
             INTREG  iregInterLevel;
    REGISTER INT     iRet;
    
    __LW_ATOMIC_LOCK(iregInterLevel);
    iRet = patomic->counter ^ iVal;
    patomic->counter = iRet;
    __LW_ATOMIC_UNLOCK(iregInterLevel);
    
    return  (iRet);
}

static LW_INLINE VOID  __LW_ATOMIC_SET (INT  iVal, atomic_t  *patomic)
{
    INTREG  iregInterLevel;
    
    __LW_ATOMIC_LOCK(iregInterLevel);
    patomic->counter = iVal;
    __LW_ATOMIC_UNLOCK(iregInterLevel);
}

static LW_INLINE INT  __LW_ATOMIC_GET (atomic_t  *patomic)
{
    return  (patomic->counter);
}

static LW_INLINE INT  __LW_ATOMIC_SWP (INT  iVal, atomic_t  *patomic)
{
             INTREG  iregInterLevel;
    REGISTER INT     iRet;
    
    __LW_ATOMIC_LOCK(iregInterLevel);
    iRet = patomic->counter;
    patomic->counter = iVal;
    __LW_ATOMIC_UNLOCK(iregInterLevel);
    
    return  (iRet);
}

#endif                                                                  /*  __K_ATOMIC_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
