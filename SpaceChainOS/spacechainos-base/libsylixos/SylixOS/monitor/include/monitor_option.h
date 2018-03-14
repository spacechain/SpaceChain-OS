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
** 文   件   名: monitor_option.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 08 月 17 日
**
** 描        述: SylixOS 内核事件监控器事件类型.
**
** BUG:
2015.11.12  去掉一些过于频繁的事件.
*********************************************************************************************************/

#ifndef __MONITOR_OPTION_H
#define __MONITOR_OPTION_H

/*********************************************************************************************************
  中断事件
*********************************************************************************************************/

#define MONITOR_EVENT_ID_INT            0

#define MONITOR_EVENT_INT_VECT_EN       0                               /*  中断向量使能                */
#define MONITOR_EVENT_INT_VECT_DIS      1                               /*  中断向量禁能                */

/*********************************************************************************************************
  调度事件
*********************************************************************************************************/

#define MONITOR_EVENT_ID_SCHED          1

#define MONITOR_EVENT_SCHED_TASK        0                               /*  任务态调度                  */
#define MONITOR_EVENT_SCHED_INT         1                               /*  中断态调度                  */

/*********************************************************************************************************
  内核事件
*********************************************************************************************************/

#define MONITOR_EVENT_ID_KERNEL         2

#define MONITOR_EVENT_KERNEL_TICK       0                               /*  内核时钟                    */

/*********************************************************************************************************
  线程事件
*********************************************************************************************************/

#define MONITOR_EVENT_ID_THREAD         3

#define MONITOR_EVENT_THREAD_CREATE     0                               /*  线程创建                    */
#define MONITOR_EVENT_THREAD_DELETE     1                               /*  线程删除                    */

#define MONITOR_EVENT_THREAD_INIT       2                               /*  线程初始化                  */
#define MONITOR_EVENT_THREAD_START      3                               /*  被初始化的线程开始执行      */
#define MONITOR_EVENT_THREAD_RESTART    4                               /*  线程重启                    */

#define MONITOR_EVENT_THREAD_JOIN       5                               /*  线程合并                    */
#define MONITOR_EVENT_THREAD_DETACH     6                               /*  线程解除合并                */

#define MONITOR_EVENT_THREAD_SAFE       7                               /*  线程安全                    */
#define MONITOR_EVENT_THREAD_UNSAFE     8                               /*  线程解除安全                */

#define MONITOR_EVENT_THREAD_SUSPEND    9                               /*  线程挂起                    */
#define MONITOR_EVENT_THREAD_RESUME     10                              /*  线程解除挂起                */

#define MONITOR_EVENT_THREAD_NAME       11                              /*  线程设置名字                */
#define MONITOR_EVENT_THREAD_PRIO       12                              /*  线程设置优先级              */

#define MONITOR_EVENT_THREAD_SLICE      13                              /*  线程设置时间片长度          */
#define MONITOR_EVENT_THREAD_SCHED      14                              /*  线程设置调度参数            */

#define MONITOR_EVENT_THREAD_NOTEPAD    15                              /*  线程设置记事本              */

#define MONITOR_EVENT_THREAD_FEEDWD     16                              /*  线程喂看门狗                */
#define MONITOR_EVENT_THREAD_CANCELWD   17                              /*  线程取消看门狗              */

#define MONITOR_EVENT_THREAD_WAKEUP     18                              /*  线程唤醒                    */
#define MONITOR_EVENT_THREAD_SLEEP      19                              /*  线程睡眠                    */

#define MONITOR_EVENT_THREAD_STOP       20                              /*  线程停止                    */
#define MONITOR_EVENT_THREAD_CONT       21                              /*  线程继续                    */

/*********************************************************************************************************
  协程事件
*********************************************************************************************************/

#define MONITOR_EVENT_ID_COROUTINE      4

#define MONITOR_EVENT_COROUTINE_CREATE  0                               /*  协程创建                    */
#define MONITOR_EVENT_COROUTINE_DELETE  1                               /*  协程删除                    */

#define MONITOR_EVENT_COROUTINE_YIELD   2                               /*  协程主动让出 CPU            */
#define MONITOR_EVENT_COROUTINE_RESUME  3                               /*  协程主动调度                */

/*********************************************************************************************************
  信号量事件
*********************************************************************************************************/

#define MONITOR_EVENT_ID_SEMC           5
#define MONITOR_EVENT_ID_SEMB           6
#define MONITOR_EVENT_ID_SEMM           7
#define MONITOR_EVENT_ID_SEMRW          8

#define MONITOR_EVENT_SEM_CREATE        0                               /*  SEM 创建                    */
#define MONITOR_EVENT_SEM_DELETE        1                               /*  SEM 删除                    */

#define MONITOR_EVENT_SEM_PEND          2                               /*  SEM 等待                    */
#define MONITOR_EVENT_SEM_POST          3                               /*  SEM 发送并有任务被激活      */

#define MONITOR_EVENT_SEM_FLUSH         4                               /*  SEM 激活所有等待者          */
#define MONITOR_EVENT_SEM_CLEAR         5                               /*  SEM 清除有效的信号量        */

/*********************************************************************************************************
  消息队列事件
*********************************************************************************************************/

#define MONITOR_EVENT_ID_MSGQ           9

#define MONITOR_EVENT_MSGQ_CREATE       0                               /*  MSGQ 创建                   */
#define MONITOR_EVENT_MSGQ_DELETE       1                               /*  MSGQ 删除                   */

#define MONITOR_EVENT_MSGQ_PEND         2                               /*  MSGQ 等待                   */
#define MONITOR_EVENT_MSGQ_POST         3                               /*  MSGQ 发送并有任务被激活     */

#define MONITOR_EVENT_MSGQ_FLUSH        4                               /*  MSGQ 激活所有等待者         */
#define MONITOR_EVENT_MSGQ_CLEAR        5                               /*  MSGQ 清除有效的信号量       */

/*********************************************************************************************************
  事件集事件
*********************************************************************************************************/

#define MONITOR_EVENT_ID_ESET           10

#define MONITOR_EVENT_ESET_CREATE       0                               /*  ESET 创建                   */
#define MONITOR_EVENT_ESET_DELETE       1                               /*  ESET 删除                   */

#define MONITOR_EVENT_ESET_PEND         2                               /*  ESET 等待                   */
#define MONITOR_EVENT_ESET_POST         3                               /*  ESET 发送并有任务被激活     */

/*********************************************************************************************************
  定时器事件
*********************************************************************************************************/

#define MONITOR_EVENT_ID_TIMER          11

#define MONITOR_EVENT_TIMER_CREATE      0                               /*  定时器创建                  */
#define MONITOR_EVENT_TIMER_DELETE      1                               /*  定时器删除                  */

/*********************************************************************************************************
  定长内存管理
*********************************************************************************************************/

#define MONITOR_EVENT_ID_PART           12

#define MONITOR_EVENT_PART_CREATE       0                               /*  定长内存创建                */
#define MONITOR_EVENT_PART_DELETE       1                               /*  定长内存删除                */

#define MONITOR_EVENT_PART_GET          2                               /*  定长内存分配                */
#define MONITOR_EVENT_PART_PUT          3                               /*  定长内存回收                */

/*********************************************************************************************************
  变长内存管理
*********************************************************************************************************/

#define MONITOR_EVENT_ID_REGION         13

#define MONITOR_EVENT_REGION_CREATE     0                               /*  变长内存创建                */
#define MONITOR_EVENT_REGION_DELETE     1                               /*  变长内存删除                */

#define MONITOR_EVENT_REGION_ALLOC      2                               /*  变长内存分配                */
#define MONITOR_EVENT_REGION_FREE       3                               /*  变长内存回收                */
#define MONITOR_EVENT_REGION_REALLOC    4                               /*  变长内存 realloc            */

/*********************************************************************************************************
  I/O 基本操作
*********************************************************************************************************/

#define MONITOR_EVENT_ID_IO             14

#define MONITOR_EVENT_IO_OPEN           0                               /*  open                        */
#define MONITOR_EVENT_IO_CREAT          1                               /*  creat                       */
#define MONITOR_EVENT_IO_CLOSE          2                               /*  close                       */
#define MONITOR_EVENT_IO_UNLINK         3                               /*  unlink                      */

#define MONITOR_EVENT_IO_READ           4                               /*  read                        */
#define MONITOR_EVENT_IO_WRITE          5                               /*  write                       */
#define MONITOR_EVENT_IO_IOCTL          6                               /*  ioctl                       */

#define MONITOR_EVENT_IO_MOVE_FROM      7                               /*  rename from                 */
#define MONITOR_EVENT_IO_MOVE_TO        8                               /*  rename to                   */

#define MONITOR_EVENT_IO_CHDIR          9                               /*  chdir                       */
#define MONITOR_EVENT_IO_DUP            10                              /*  dup                         */
#define MONITOR_EVENT_IO_SYMLINK        11                              /*  symlink                     */
#define MONITOR_EVENT_IO_SYMLINK_DST    12                              /*  symlink                     */

/*********************************************************************************************************
  信号
*********************************************************************************************************/

#define MONITOR_EVENT_ID_SIGNAL         15

#define MONITOR_EVENT_SIGNAL_KILL       0                               /*  kill                        */
#define MONITOR_EVENT_SIGNAL_SIGQUEUE   1                               /*  sigqueue                    */
#define MONITOR_EVENT_SIGNAL_SIGEVT     2                               /*  内核信号                    */

#define MONITOR_EVENT_SIGNAL_SIGSUSPEND 3                               /*  sigsuspend                  */
#define MONITOR_EVENT_SIGNAL_PAUSE      4                               /*  pause                       */
#define MONITOR_EVENT_SIGNAL_SIGWAIT    5                               /*  sigwait                     */
#define MONITOR_EVENT_SIGNAL_SIGRUN     6                               /*  运行信号句柄                */

/*********************************************************************************************************
  装载器
*********************************************************************************************************/

#define MONITOR_EVENT_ID_LOADER         16

#define MONITOR_EVENT_LOADER_LOAD       0                               /*  装载                        */
#define MONITOR_EVENT_LOADER_UNLOAD     1                               /*  卸载                        */
#define MONITOR_EVENT_LOADER_REFRESH    2                               /*  刷新共享区                  */

/*********************************************************************************************************
  虚拟进程
*********************************************************************************************************/

#define MONITOR_EVENT_ID_VPROC          17

#define MONITOR_EVENT_VPROC_CREATE      0                               /*  创建进程                    */
#define MONITOR_EVENT_VPROC_DELETE      1                               /*  删除进程                    */
#define MONITOR_EVENT_VPROC_RUN         2                               /*  进程运行可执行文件          */

/*********************************************************************************************************
  虚拟内存管理
*********************************************************************************************************/

#define MONITOR_EVENT_ID_VMM            18

#define MONITOR_EVENT_VMM_ALLOC         0                               /*  API_VmmMalloc               */
#define MONITOR_EVENT_VMM_ALLOC_A       1                               /*  API_VmmMallocArea           */
#define MONITOR_EVENT_VMM_FREE          2                               /*  API_VmmFree                 */

#define MONITOR_EVENT_VMM_REMAP_A       3                               /*  API_VmmRemapArea            */
#define MONITOR_EVENT_VMM_INVAL_A       4                               /*  API_VmmInvalidateArea       */

#define MONITOR_EVENT_VMM_PREALLOC_A    5                               /*  API_VmmPreallocArea         */
#define MONITOR_EVENT_VMM_SHARE_A       6                               /*  API_VmmShareArea            */

#define MONITOR_EVENT_VMM_PHY_ALLOC     7                               /*  API_VmmPhyAlloc             */
#define MONITOR_EVENT_VMM_PHY_FREE      8                               /*  API_VmmPhyFree              */

#define MONITOR_EVENT_VMM_DMA_ALLOC     9                               /*  API_VmmDmaAlloc             */
#define MONITOR_EVENT_VMM_DMA_FREE      10                              /*  API_VmmDmaFree              */

#define MONITOR_EVENT_VMM_IOREMAP       11                              /*  API_VmmIoRemap              */
#define MONITOR_EVENT_VMM_IOUNMAP       12                              /*  API_VmmIoUnmap              */

#define MONITOR_EVENT_VMM_SETFLAG       13                              /*  API_VmmSetFlag              */

#define MONITOR_EVENT_VMM_MMAP          14                              /*  mmap                        */
#define MONITOR_EVENT_VMM_MSYNC         15                              /*  msync                       */
#define MONITOR_EVENT_VMM_MUNMAP        16                              /*  munmap                      */

/*********************************************************************************************************
  异常管理
*********************************************************************************************************/

#define MONITOR_EVENT_ID_EXCEPTION      19

#define MONITOR_EVENT_EXCEPTION_SOFT    0                               /*  _excJob message             */

/*********************************************************************************************************
  网络
*********************************************************************************************************/

#define MONITOR_EVENT_ID_NETWORK        20

#define MONITOR_EVENT_NETWORK_SOCKPAIR  0                               /*  socketpair                  */
#define MONITOR_EVENT_NETWORK_SOCKET    1                               /*  socket                      */
#define MONITOR_EVENT_NETWORK_ACCEPT    2                               /*  accept, accept4             */
#define MONITOR_EVENT_NETWORK_BIND      3                               /*  bind                        */
#define MONITOR_EVENT_NETWORK_SHUTDOWN  4                               /*  shutdown                    */
#define MONITOR_EVENT_NETWORK_CONNECT   5                               /*  connect                     */
#define MONITOR_EVENT_NETWORK_SOCKOPT   6                               /*  setsocketopt                */
#define MONITOR_EVENT_NETWORK_LISTEN    7                               /*  listen                      */
#define MONITOR_EVENT_NETWORK_RECV      8                               /*  recv, recvfrom, recvmsg     */
#define MONITOR_EVENT_NETWORK_SEND      9                               /*  send, sendto, sendmsg       */

/*********************************************************************************************************
  限制数值
*********************************************************************************************************/

#define MONITOR_EVENT_ID_USER           48                              /*  用户自定义时间最小值        */
#define MONITOR_EVENT_ID_MAX            63                              /*  最大事件编号                */

#endif                                                                  /*  __MONITOR_OPTION_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
