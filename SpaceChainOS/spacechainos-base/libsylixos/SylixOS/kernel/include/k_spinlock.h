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
** 文   件   名: k_spinlock.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 04 月 07 日
**
** 描        述: 自旋锁类型及基本操作定义。
**
** 注        意: 多数的 spinlock 操作需要由移植 arch 层搞定, 例如 spinlock 重入和 cpu 字段的赋值, 这里考虑
                 到很多 CPU 设计厂家都有自己提高 spinlock 的效率的方法, 各不相同, 所以这里开放给 arch 层
                 处理, 为了适应不同体系结构, 引入 SL_ulCounter 字段作为重入计数. 需要 arch 层支持, 当然
                 arch 层也可以使用 SL_sltData 同时完成重入计数功能. (移植层必须支持 spinlock 可重入)
                 与页表的操作相同, spinlock 涉及 CACHE 的操作完全由操作系统移植程序完成.
                 注意, 运行到 spinlock 程序时, 可能内核并没有初始化 CACHE 库, 所以不能调用操作系统提供的
                 CACHE 操作函数, 需要调用移植包内的 CACHE 操作函数!
                 同时, spinlock 预留了相关的 CPU 排队能力, 可以依照优先级顺序, 也可以使用 FIFO 等待, 这完
                 全取决于 arch 层的移植代码.
** BUG:
2013.07.17  加入必要的内存屏障, 保证 SMP 系统 spinlock 乱序执行安全.
*********************************************************************************************************/

#ifndef __K_SPINLOCK_H
#define __K_SPINLOCK_H

/*********************************************************************************************************
  注意1: 
  
  1: 对 spinlock 的操作要有始有终(资源分配有序性), 否则将出现多核死锁的危险!
  
  2: 对资源的获取要有顺序, 否则就会出现多核死锁!
  
  操作系统对多核大体分为两种互斥资源: 
  
    1: 应用模块 spinlock
    2: 内核数据 spinlock
    
    spinlock 获取的顺序一定是从 1 -> 2, 这样才能有效避免多核资源死锁.
    
  注意2: 
  
    KN_INT_ENABLE();  KN_INT_DISABLE(); 序列只能操作调用者的 CPU, 即当前 CPU, 只能禁止当前 CPU 的中
    断. 而不能禁止其他 CPU 的中断. (禁止当前 CPU 的中断可以保证当前 CPU 不被抢占)
    
    spinlock 如果抢占了一个 spinlock 的竞争者将, 可能抢占该 spinlock 的保持者来运行，但是由于得不到 
    spinlock 抢占者将自旋在那里, 如果竞争者的优先级高于保持者的优先级, 将形成一种死锁的局面, 因为保
    持者无法得到运行而永远不能释放 spinlock. 而竞争者由于不能得到一个不可能释放的 spinlock 而永远自
    旋在那里。
    由于中断处理函数也可以使用 spinlock，如果它使用的 spinlock 已经被一个线程保持, 中断处理函数将无
    法继续进行, 从而形成死锁,
    
    针对上述原则，我们可以提供一种更安全的 spinlock —— 可重入的 spinlock 机制. 它的思想是, 在入参
    中同时提供一个标志核 ID 的 Owner 字段，当某个 CPU 核获取锁后，Owner 字段就等于当前核的 ID。在申
    请锁时, 首先判断 Owner 字段是否等于申请者的核 ID, 如果相等, 则跳过后面的检查流程直接操作共享资源，
    否则再进行 spin lock 的操作。
    
    为了提高系统的效率, 需要在移植层判断 Owner 字段, 并进行重入处理.
    
  注意 3: 
    SylixOS 系统提供了一组 QUICK 接口, 需要说明的是 LW_SPIN_UNLOCK_QUICK() 不会尝试调度, 因为程序判断
    在锁定 spinlock 的过程中, 绝不会发送任务状态迁移的操作, 或者接下来的程序将马上进行真正的调度尝试.
*********************************************************************************************************/

#define LW_SPIN_INITIALIZER     {0, LW_NULL, 0, {LW_NULL}}

/*********************************************************************************************************
  裁剪
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
  SMP 基本 spinlock 操作, 所有的 spinlock 操作内部都包含相应的内存屏障操作.
*********************************************************************************************************/

VOID    _SmpSpinInit(spinlock_t *psl);

VOID    _SmpSpinLock(spinlock_t *psl);
BOOL    _SmpSpinTryLock(spinlock_t *psl);
INT     _SmpSpinUnlock(spinlock_t *psl);

VOID    _SmpSpinLockIgnIrq(spinlock_t *psl);
BOOL    _SmpSpinTryLockIgnIrq(spinlock_t *psl);
VOID    _SmpSpinUnlockIgnIrq(spinlock_t *psl);

VOID    _SmpSpinLockIrq(spinlock_t *psl, INTREG  *piregInterLevel);
BOOL    _SmpSpinTryLockIrq(spinlock_t *psl, INTREG  *piregInterLevel);
INT     _SmpSpinUnlockIrq(spinlock_t *psl, INTREG  iregInterLevel);
VOID    _SmpSpinUnlockIrqQuick(spinlock_t *psl, INTREG  iregInterLevel);

VOID    _SmpSpinLockTask(spinlock_t *psl);
BOOL    _SmpSpinTryLockTask(spinlock_t *psl);
INT     _SmpSpinUnlockTask(spinlock_t *psl);

VOID    _SmpSpinLockRaw(spinlock_t *psl, INTREG  *piregInterLevel);
BOOL    _SmpSpinTryLockRaw(spinlock_t *psl, INTREG  *piregInterLevel);
VOID    _SmpSpinUnlockRaw(spinlock_t *psl, INTREG  iregInterLevel);

#define LW_SPIN_INIT(psl)                   _SmpSpinInit(psl)

#define LW_SPIN_LOCK(psl)                   _SmpSpinLock(psl)
#define LW_SPIN_TRYLOCK(psl)                _SmpSpinTryLock(psl)
#define LW_SPIN_UNLOCK(psl)                 _SmpSpinUnlock(psl)

#define LW_SPIN_LOCK_IGNIRQ(psl)            _SmpSpinLockIgnIrq(psl)
#define LW_SPIN_TRYLOCK_IGNIRQ(psl)         _SmpSpinTryLockIgnIrq(psl)
#define LW_SPIN_UNLOCK_IGNIRQ(psl)          _SmpSpinUnlockIgnIrq(psl)

#define LW_SPIN_LOCK_IRQ(psl, pireg)        _SmpSpinLockIrq(psl, pireg)
#define LW_SPIN_TRYLOCK_IRQ(psl, pireg)     _SmpSpinTryLockIrq(psl, pireg)
#define LW_SPIN_UNLOCK_IRQ(psl, ireg)       _SmpSpinUnlockIrq(psl, ireg)

#define LW_SPIN_LOCK_QUICK(psl, pireg)      _SmpSpinLockIrq(psl, pireg)
#define LW_SPIN_UNLOCK_QUICK(psl, ireg)     _SmpSpinUnlockIrqQuick(psl, ireg)

#define LW_SPIN_LOCK_TASK(psl)              _SmpSpinLockTask(psl)
#define LW_SPIN_TRYLOCK_TASK(psl)           _SmpSpinTryLockTask(psl)
#define LW_SPIN_UNLOCK_TASK(psl)            _SmpSpinUnlockTask(psl)

#define LW_SPIN_LOCK_RAW(psl, pireg)        _SmpSpinLockRaw(psl, pireg)
#define LW_SPIN_TRYLOCK_RAW(psl, pireg)     _SmpSpinTryLockRaw(psl, pireg)
#define LW_SPIN_UNLOCK_RAW(psl, ireg)       _SmpSpinUnlockRaw(psl, ireg)

/*********************************************************************************************************
  SMP 内核锁操作.
*********************************************************************************************************/

struct  __lw_tcb;
VOID    _SmpKernelLockIgnIrq(VOID);
VOID    _SmpKernelUnlockIgnIrq(VOID);

VOID    _SmpKernelLockQuick(INTREG  *piregInterLevel);
VOID    _SmpKernelUnlockQuick(INTREG  iregInterLevel);

VOID    _SmpKernelUnlockSched(struct __lw_tcb *ptcbOwner);

#define LW_SPIN_KERN_LOCK_IGNIRQ()          _SmpKernelLockIgnIrq()
#define LW_SPIN_KERN_UNLOCK_IGNIRQ()        _SmpKernelUnlockIgnIrq()

#define LW_SPIN_KERN_LOCK_QUICK(pireg)      _SmpKernelLockQuick(pireg)
#define LW_SPIN_KERN_UNLOCK_QUICK(ireg)     _SmpKernelUnlockQuick(ireg)

#define LW_SPIN_KERN_UNLOCK_SCHED(ptcb)     _SmpKernelUnlockSched(ptcb)

#else
/*********************************************************************************************************
  单处理器伪自旋锁
*********************************************************************************************************/

VOID    _UpSpinInit(spinlock_t *psl);

VOID    _UpSpinLock(spinlock_t *psl);
BOOL    _UpSpinTryLock(spinlock_t *psl);
INT     _UpSpinUnlock(spinlock_t *psl);

VOID    _UpSpinLockIgnIrq(spinlock_t *psl);
BOOL    _UpSpinTryLockIgnIrq(spinlock_t *psl);
VOID    _UpSpinUnlockIgnIrq(spinlock_t *psl);

VOID    _UpSpinLockIrq(spinlock_t *psl, INTREG  *piregInterLevel);
BOOL    _UpSpinTryLockIrq(spinlock_t *psl, INTREG  *piregInterLevel);
INT     _UpSpinUnlockIrq(spinlock_t *psl, INTREG  iregInterLevel);
VOID    _UpSpinUnlockIrqQuick(spinlock_t *psl, INTREG  iregInterLevel);

VOID    _UpSpinLockRaw(spinlock_t *psl, INTREG  *piregInterLevel);
BOOL    _UpSpinTryLockRaw(spinlock_t *psl, INTREG  *piregInterLevel);
VOID    _UpSpinUnlockRaw(spinlock_t *psl, INTREG  iregInterLevel);

#define LW_SPIN_INIT(psl)                   _UpSpinInit(psl)

#define LW_SPIN_LOCK(psl)                   _UpSpinLock(psl)
#define LW_SPIN_TRYLOCK(psl)                _UpSpinTryLock(psl)
#define LW_SPIN_UNLOCK(psl)                 _UpSpinUnlock(psl)

#define LW_SPIN_LOCK_IGNIRQ(psl)            _UpSpinLockIgnIrq(psl)
#define LW_SPIN_TRYLOCK_IGNIRQ(psl)         _UpSpinTryLockIgnIrq(psl)
#define LW_SPIN_UNLOCK_IGNIRQ(psl)          _UpSpinUnlockIgnIrq(psl)

#define LW_SPIN_LOCK_IRQ(psl, pireg)        _UpSpinLockIrq(psl, pireg)
#define LW_SPIN_TRYLOCK_IRQ(psl, pireg)     _UpSpinTryLockIrq(psl, pireg)
#define LW_SPIN_UNLOCK_IRQ(psl, ireg)       _UpSpinUnlockIrq(psl, ireg)

#define LW_SPIN_LOCK_QUICK(psl, pireg)      _UpSpinLockIrq(psl, pireg)
#define LW_SPIN_UNLOCK_QUICK(psl, ireg)     _UpSpinUnlockIrqQuick(psl, ireg)

#define LW_SPIN_LOCK_TASK(psl)              _UpSpinLock(psl)
#define LW_SPIN_TRYLOCK_TASK(psl)           _UpSpinTryLock(psl)
#define LW_SPIN_UNLOCK_TASK(psl)            _UpSpinUnlock(psl)

#define LW_SPIN_LOCK_RAW(psl, pireg)        _UpSpinLockRaw(psl, pireg)
#define LW_SPIN_TRYLOCK_RAW(psl, pireg)     _UpSpinTryLockRaw(psl, pireg)
#define LW_SPIN_UNLOCK_RAW(psl, ireg)       _UpSpinUnlockRaw(psl, ireg)

/*********************************************************************************************************
  单处理器内核锁操作
*********************************************************************************************************/

struct  __lw_tcb;
VOID    _UpKernelLockIgnIrq(VOID);
VOID    _UpKernelUnlockIgnIrq(VOID);

VOID    _UpKernelLockQuick(INTREG  *piregInterLevel);
VOID    _UpKernelUnlockQuick(INTREG  iregInterLevel);

VOID    _UpKernelUnlockSched(struct __lw_tcb *ptcbOwner);

#define LW_SPIN_KERN_LOCK_IGNIRQ()          _UpKernelLockIgnIrq()
#define LW_SPIN_KERN_UNLOCK_IGNIRQ()        _UpKernelUnlockIgnIrq()

#define LW_SPIN_KERN_LOCK_QUICK(pireg)      _UpKernelLockQuick(pireg)
#define LW_SPIN_KERN_UNLOCK_QUICK(ireg)     _UpKernelUnlockQuick(ireg)

#define LW_SPIN_KERN_UNLOCK_SCHED(ptcb)     _UpKernelUnlockSched(ptcb)

#endif                                                                  /*  LW_CFG_SMP_EN               */
#endif                                                                  /*  __K_SPINLOCK_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
