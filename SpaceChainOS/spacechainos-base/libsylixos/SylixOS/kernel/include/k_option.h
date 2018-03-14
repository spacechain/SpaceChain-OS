/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: k_option.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 这是系统选项宏定义。

** BUG
2007.11.04  将 0xFFFFFFFF 改为 __ARCH_ULONG_MAX
2007.11.07  加入 LW_OPTION_THREAD_UNSELECT 表明此线程不需要 select 功能, 可以节省内存.
2008.03.02  加入 reboot 的功能. LW_OPTION_KERNEL_REBOOT 可以设置内核级重启 HOOK.
2011.02.23  加入 LW_OPTION_SIGNAL_INTER 选项, 事件可以选择自己是否可被中断打断.
2012.09.22  加入一些新的 HOOK 已实现相关低功耗管理.
2013.08.29  加入 LW_OPTION_THREAD_NO_MONITOR 线程创建选项, 表示内核事件跟踪器不对此线程起效.
*********************************************************************************************************/

#ifndef  __K_OPTION_H
#define  __K_OPTION_H

/*********************************************************************************************************
  DEFAULT
*********************************************************************************************************/

#define  LW_OPTION_NONE                                 0ul             /*  没有任何选项                */
#define  LW_OPTION_DEFAULT                              0ul             /*  默认选项                    */

/*********************************************************************************************************
  GLOBAL (GLOBAL 对象属于全局对象, 进程退出时不回收此类对象)
  
  注意: 所有驱动程序, 内核模块等内核程序创建内核对象时, 必须使用 LW_OPTION_OBJECT_GLOBAL 选项.
        应用程序或者动态链接库程序则不能使用 LW_OPTION_OBJECT_GLOBAL 选项.
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
#define  LW_OPTION_OBJECT_GLOBAL                        0x80000000      /*  全局对象                    */
#define  LW_OPTION_OBJECT_DEBUG_UNPEND                  0x40000000      /*  调试器关键资源              */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#define  LW_OPTION_OBJECT_LOCAL                         0x00000000      /*  本地对象                    */

/*********************************************************************************************************
  HEAP
*********************************************************************************************************/

#define  LW_OPTION_HEAP_KERNEL                          0x00000000      /*  内核堆                      */
#define  LW_OPTION_HEAP_SYSTEM                          0x00000001      /*  系统堆                      */

/*********************************************************************************************************
  THREAD 
     
  注意: 浮点运算器上下文指针是否有效, 由 BSP FPU 相关实现决定, 
        推荐用户不要自己决定是否使用 LW_OPTION_THREAD_USED_FP
        用户禁止使用 LW_OPTION_THREAD_STK_MAIN 选项.
*********************************************************************************************************/

#define  LW_OPTION_THREAD_STK_CHK                       0x00000003      /*  允许对任务堆栈进行检查      */
#define  LW_OPTION_THREAD_STK_CLR                       0x00000002      /*  在任务建立时堆栈所有数据清零*/
#define  LW_OPTION_THREAD_USED_FP                       0x00000004      /*  使用浮点运算器              */
#define  LW_OPTION_THREAD_SUSPEND                       0x00000010      /*  建立任务后阻塞              */
#define  LW_OPTION_THREAD_NO_AFFINITY                   0x00000020      /*  任务初始化为不继承亲和度    */
#define  LW_OPTION_THREAD_INIT                          0x00000040      /*  初始化任务                  */
#define  LW_OPTION_THREAD_SAFE                          0x00000080      /*  建立的线程为安全模式        */
#define  LW_OPTION_THREAD_DETACHED                      0x00000100      /*  线程不允许被合并            */
#define  LW_OPTION_THREAD_UNSELECT                      0x00000200      /*  此线程不使用 select 功能    */
#define  LW_OPTION_THREAD_NO_MONITOR                    0x00000400      /*  内核事件跟踪器不对此任务起效*/
#define  LW_OPTION_THREAD_SCOPE_PROCESS                 0x00000800      /*  进程区域内竞争 (当前不支持) */

#ifdef __SYLIXOS_KERNEL
#define  LW_OPTION_THREAD_AFFINITY_ALWAYS               0x20000000      /*  总是锁定一个 CPU 执行       */
#define  LW_OPTION_THREAD_STK_MAIN                      0x40000000      /*  进程主线程 stack            */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  THREAD CANCLE
*********************************************************************************************************/

#define  LW_THREAD_CANCEL_ASYNCHRONOUS                  0               /*  PTHREAD_CANCEL_ASYNCHRONOUS */
#define  LW_THREAD_CANCEL_DEFERRED                      1               /*  PTHREAD_CANCEL_DEFERRED     */

#define  LW_THREAD_CANCEL_ENABLE                        0               /*  允许线程 CANCEL             */
#define  LW_THREAD_CANCEL_DISABLE                       1

#define  LW_THREAD_CANCELED                             ((PVOID)-1)     /*  PTHREAD_CANCELED            */
                                                                        /*  线程被cancel后的返回值      */

/*********************************************************************************************************
  THREAD NOTEPAD
*********************************************************************************************************/

#define  LW_OPTION_THREAD_NOTEPAD_0                     0x00            /*  线程记事本号                */
#define  LW_OPTION_THREAD_NOTEPAD_1                     0x01
#define  LW_OPTION_THREAD_NOTEPAD_2                     0x02
#define  LW_OPTION_THREAD_NOTEPAD_3                     0x03
#define  LW_OPTION_THREAD_NOTEPAD_4                     0x04
#define  LW_OPTION_THREAD_NOTEPAD_5                     0x05
#define  LW_OPTION_THREAD_NOTEPAD_6                     0x06
#define  LW_OPTION_THREAD_NOTEPAD_7                     0x07
#define  LW_OPTION_THREAD_NOTEPAD_8                     0x08
#define  LW_OPTION_THREAD_NOTEPAD_9                     0x09
#define  LW_OPTION_THREAD_NOTEPAD_10                    0x0a
#define  LW_OPTION_THREAD_NOTEPAD_11                    0x0b
#define  LW_OPTION_THREAD_NOTEPAD_12                    0x0c
#define  LW_OPTION_THREAD_NOTEPAD_13                    0x0d
#define  LW_OPTION_THREAD_NOTEPAD_14                    0x0e
#define  LW_OPTION_THREAD_NOTEPAD_15                    0x0f            /*  线程记事本号                */
#define  LW_OPTION_THREAD_NOTEPAD_NO(n)                 (n)

/*********************************************************************************************************
  Semaphore & MsgQueue
*********************************************************************************************************/
/*********************************************************************************************************
  等待时间 (超时选项)
*********************************************************************************************************/

#define  LW_OPTION_NOT_WAIT                             0x00000000      /*  不等待立即退出              */
#define  LW_OPTION_WAIT_INFINITE                        __ARCH_ULONG_MAX
                                                                        /*  永远等待                    */
#define  LW_OPTION_WAIT_A_TICK                          0x00000001      /*  等待一个时钟嘀嗒            */
#define  LW_OPTION_WAIT_A_SECOND                        CLOCKS_PER_SEC
                                                                        /*  等待一秒                    */
/*********************************************************************************************************
  等待排序算法 (创建选项)
*********************************************************************************************************/

#define  LW_OPTION_WAIT_PRIORITY                        0x00010000      /*  按优先级顺序等待            */
#define  LW_OPTION_WAIT_FIFO                            0x00000000      /*  按先进先出顺序等待          */

/*********************************************************************************************************
  删除安全 (创建选项)
*********************************************************************************************************/

#define  LW_OPTION_DELETE_SAFE                          0x00020000      /*  Mutex or RW 安全删除使能    */
#define  LW_OPTION_DELETE_UNSAFE                        0x00000000      /*  Mutex or RW 非安全保护使能  */

/*********************************************************************************************************
  等待过程中遇到信号处理方式 (创建选项) (同样适用于 EVENTSET)
*********************************************************************************************************/

#define  LW_OPTION_SIGNAL_INTER                         0x00040000      /*  可被信号打断 (EINTR)        */
#define  LW_OPTION_SIGNAL_UNINTER                       0x00000000      /*  不可被信号打断              */

/*********************************************************************************************************
  互斥信号量优先级算 (创建选项)
*********************************************************************************************************/

#define  LW_OPTION_INHERIT_PRIORITY                     0x00080000      /*  优先级继承算法              */
#define  LW_OPTION_PRIORITY_CEILING                     0x00000000      /*  优先级天花板算法            */

/*********************************************************************************************************
  互斥信号量, 读写锁是否允许重入 (创建选项)
*********************************************************************************************************/

#define  LW_OPTION_NORMAL                               0x00100000      /*  递归时不检查 (不推荐)       */
#define  LW_OPTION_ERRORCHECK                           0x00200000      /*  Mutex or RW 递归时报错      */
#define  LW_OPTION_RECURSIVE                            0x00000000      /*  Mutex or RW 支持写递归调用  */

/*********************************************************************************************************
  读写锁读写优先控制 (创建选项)
*********************************************************************************************************/

#define  LW_OPTION_RW_PREFER_READER                     0x00000000      /*  读访问优先                  */
#define  LW_OPTION_RW_PREFER_WRITER                     0x00400000      /*  写访问优先                  */

/*********************************************************************************************************
  消息队列接收选项 (消息队列接收选项)
*********************************************************************************************************/

#define  LW_OPTION_NOERROR                              0x00400000      /*  大于缓冲区的消息自动截断    */
                                                                        /*  默认接收为此选项            */
/*********************************************************************************************************
  消息队列发送选项
*********************************************************************************************************/

#define  LW_OPTION_URGENT                               0x00000001      /*  消息队列紧急消息发送        */
#define  LW_OPTION_BROADCAST                            0x00000002      /*  消息队列广播发送            */

/*********************************************************************************************************
  EVENT SET
*********************************************************************************************************/
/*********************************************************************************************************
  等待(接收)选项
*********************************************************************************************************/
#define  LW_OPTION_EVENTSET_WAIT_CLR_ALL                0               /*  指定位都为0时激活           */
#define  LW_OPTION_EVENTSET_WAIT_CLR_ANY                1               /*  指定位中任何一位为0时激活   */
#define  LW_OPTION_EVENTSET_WAIT_SET_ALL                2               /*  指定位都为1时激活           */
#define  LW_OPTION_EVENTSET_WAIT_SET_ANY                3               /*  指定位中任何一位为1时激活   */

#define  LW_OPTION_EVENTSET_RETURN_ALL                  0x00000040      /*  获得事件后返回所有有效的事件*/
#define  LW_OPTION_EVENTSET_RESET                       0x00000080      /*  获得事件后是否自动清除事件  */
#define  LW_OPTION_EVENTSET_RESET_ALL                   0x00000100      /*  获得事件后清除所有事件      */
/*********************************************************************************************************
  设置(发送)选项
*********************************************************************************************************/
#define  LW_OPTION_EVENTSET_SET                         0x1             /*  将指定事件设为 1            */
#define  LW_OPTION_EVENTSET_CLR                         0x0             /*  将指定事件设为 0            */
/*********************************************************************************************************
  事件编号
*********************************************************************************************************/
#define  LW_OPTION_EVENT_0                              0x00000001      /*  事件集的所有事件位          */
#define  LW_OPTION_EVENT_1                              0x00000002
#define  LW_OPTION_EVENT_2                              0x00000004
#define  LW_OPTION_EVENT_3                              0x00000008
#define  LW_OPTION_EVENT_4                              0x00000010
#define  LW_OPTION_EVENT_5                              0x00000020
#define  LW_OPTION_EVENT_6                              0x00000040
#define  LW_OPTION_EVENT_7                              0x00000080
#define  LW_OPTION_EVENT_8                              0x00000100
#define  LW_OPTION_EVENT_9                              0x00000200
#define  LW_OPTION_EVENT_10                             0x00000400
#define  LW_OPTION_EVENT_11                             0x00000800
#define  LW_OPTION_EVENT_12                             0x00001000
#define  LW_OPTION_EVENT_13                             0x00002000
#define  LW_OPTION_EVENT_14                             0x00004000
#define  LW_OPTION_EVENT_15                             0x00008000
#define  LW_OPTION_EVENT_16                             0x00010000
#define  LW_OPTION_EVENT_17                             0x00020000
#define  LW_OPTION_EVENT_18                             0x00040000
#define  LW_OPTION_EVENT_19                             0x00080000
#define  LW_OPTION_EVENT_20                             0x00100000
#define  LW_OPTION_EVENT_21                             0x00200000
#define  LW_OPTION_EVENT_22                             0x00400000
#define  LW_OPTION_EVENT_23                             0x00800000
#define  LW_OPTION_EVENT_24                             0x01000000
#define  LW_OPTION_EVENT_25                             0x02000000
#define  LW_OPTION_EVENT_26                             0x04000000
#define  LW_OPTION_EVENT_27                             0x08000000
#define  LW_OPTION_EVENT_28                             0x10000000
#define  LW_OPTION_EVENT_29                             0x20000000
#define  LW_OPTION_EVENT_30                             0x40000000
#define  LW_OPTION_EVENT_31                             0x80000000
#define  LW_OPTION_EVENT_ALL                           (0xffffffff)     /*  事件集的所有事件位          */

/*********************************************************************************************************
  SCHEDLER
*********************************************************************************************************/

#define  LW_OPTION_SCHED_FIFO                           0x01            /*  调度器 FIFO                 */
#define  LW_OPTION_SCHED_RR                             0x00            /*  调度器 RR                   */

#define  LW_OPTION_RESPOND_IMMIEDIA                     0x00            /*  高速响应线程 (仅供测试使用) */
#define  LW_OPTION_RESPOND_STANDARD                     0x01            /*  普通响应线程                */
#define  LW_OPTION_RESPOND_AUTO                         0x02            /*  自动                        */

/*********************************************************************************************************
  TIME
*********************************************************************************************************/

#define  LW_OPTION_ONE_TICK         0x00000001                          /*  等待一个 TICK               */
#define  LW_OPTION_ONE_SECOND       CLOCKS_PER_SEC                      /*  一秒                        */
#define  LW_OPTION_ONE_MINUTE       (60 * LW_OPTION_ONE_SECOND)         /*  一分钟                      */
#define  LW_OPTION_ONE_HOUR         (60 * LW_OPTION_ONE_MINUTE)         /*  一小时                      */

/*********************************************************************************************************
  TIMER 创建选项
*********************************************************************************************************/

#define  LW_OPTION_ITIMER                               0x00000001      /*  普通                        */
#define  LW_OPTION_HTIMER                               0x00000000      /*  高速                        */

/*********************************************************************************************************
  TIMER 设置选项
*********************************************************************************************************/

#define  LW_OPTION_AUTO_RESTART                         0x00000001      /*  定时器自动重载              */
#define  LW_OPTION_MANUAL_RESTART                       0x00000000      /*  定时器手动重载              */

/*********************************************************************************************************
  IRQ
*********************************************************************************************************/

#define  LW_IRQ_0                                       0               /*  中断向量索引                */
#define  LW_IRQ_1                                       1
#define  LW_IRQ_2                                       2
#define  LW_IRQ_3                                       3
#define  LW_IRQ_4                                       4
#define  LW_IRQ_5                                       5
#define  LW_IRQ_6                                       6
#define  LW_IRQ_7                                       7
#define  LW_IRQ_8                                       8
#define  LW_IRQ_9                                       9
#define  LW_IRQ_10                                      10
#define  LW_IRQ_11                                      11
#define  LW_IRQ_12                                      12
#define  LW_IRQ_13                                      13
#define  LW_IRQ_14                                      14
#define  LW_IRQ_15                                      15
#define  LW_IRQ_16                                      16
#define  LW_IRQ_17                                      17
#define  LW_IRQ_18                                      18
#define  LW_IRQ_19                                      19
#define  LW_IRQ_20                                      20
#define  LW_IRQ_21                                      21
#define  LW_IRQ_22                                      22
#define  LW_IRQ_23                                      23
#define  LW_IRQ_24                                      24
#define  LW_IRQ_25                                      25
#define  LW_IRQ_26                                      26
#define  LW_IRQ_27                                      27
#define  LW_IRQ_28                                      28
#define  LW_IRQ_29                                      29
#define  LW_IRQ_30                                      30
#define  LW_IRQ_31                                      31
#define  LW_IRQ_NO(n)                                   (n)

/*********************************************************************************************************
  IRQ FLAG (设置为 QUEUE 模式向量后, 系统无法再返回非 QUEUE 模式向量)
*********************************************************************************************************/

#define  LW_IRQ_FLAG_QUEUE                              0x0001          /*  单向量, 多服务              */
#define  LW_IRQ_FLAG_PREEMPTIVE                         0x0002          /*  是否允许抢占 (arch负责实现) */
#define  LW_IRQ_FLAG_SAMPLE_RAND                        0x0004          /*  是否可用作系统随机数种子    */
#define  LW_IRQ_FLAG_GJB7714                            0x0008          /*  GBJ7714 无返回值服务        */

/*********************************************************************************************************
  API_InterVectorDisconnectEx() 选项
*********************************************************************************************************/

#define  LW_IRQ_DISCONN_DEFAULT                         0x0000          /*  与 API_...Disconnect() 相同 */
#define  LW_IRQ_DISCONN_ALL                             0x0001          /*  解除所有中断服务链接        */
#define  LW_IRQ_DISCONN_IGNORE_ARG                      0x0002          /*  忽略参数, 仅匹配函数        */

/*********************************************************************************************************
  时间选项 API_TimeNanoSleepMethod()
*********************************************************************************************************/

#define  LW_TIME_NANOSLEEP_METHOD_TICK                  0               /*  nanosleep 使用 tick 精度    */
#define  LW_TIME_NANOSLEEP_METHOD_THRS                  1               /*  TIME_HIGH_RESOLUTION 修正   */

/*********************************************************************************************************
  RTC
*********************************************************************************************************/

#define  LW_OPTION_RTC_SOFT                             0               /*  RTC 使用系统 TICK 软件时钟源*/
#define  LW_OPTION_RTC_HARD                             1               /*  RTC 使用硬件 RTC 时钟源     */

/*********************************************************************************************************
  REBOOT
*********************************************************************************************************/

#define  LW_REBOOT_FORCE                                (-1)            /*  立即重新启动计算机(致命错误)*/
#define  LW_REBOOT_WARM                                 0               /*  热启动 (BSP决定具体行为)    */
#define  LW_REBOOT_COLD                                 1               /*  冷启动 (BSP决定具体行为)    */
#define  LW_REBOOT_SHUTDOWN                             2               /*  关闭 (BSP决定具体行为)      */

/*********************************************************************************************************
  CPU 能耗
*********************************************************************************************************/

#define  LW_CPU_POWERLEVEL_TOP                          0               /*  CPU 最快速度运行            */
#define  LW_CPU_POWERLEVEL_FAST                         1               /*  CPU 快速运行                */
#define  LW_CPU_POWERLEVEL_NORMAL                       2               /*  CPU 正常运行                */
#define  LW_CPU_POWERLEVEL_SLOW                         3               /*  CPU 慢速运行                */

#endif                                                                  /*  __K_OPTION_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
