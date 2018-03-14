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
** 文   件   名: K_error.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 这是系统错误控制声明

** BUG
2007.11.04 系统错误从 301 开始，与 POSIX 标准错开.
*********************************************************************************************************/

#ifndef  __K_ERROR_H
#define  __K_ERROR_H

/*********************************************************************************************************
  完全正确
*********************************************************************************************************/

#define  ERROR_SUCCESSFUL                  0                            /*  系统没有任何错误            */
#define  ERROR_NONE                        0                            /*  系统没有任何错误            */

/*********************************************************************************************************
  系统错误   300 - 500
*********************************************************************************************************/

#define  ERROR_KERNEL_PNAME_NULL         301                            /*  名字指针为 NULL             */
#define  ERROR_KERNEL_PNAME_TOO_LONG     302                            /*  名字太长                    */
#define  ERROR_KERNEL_HANDLE_NULL        303                            /*  句柄出错                    */
#define  ERROR_KERNEL_IN_ISR             304                            /*  系统处于中断中              */
#define  ERROR_KERNEL_RUNNING            305                            /*  系统正在运行                */
#define  ERROR_KERNEL_NOT_RUNNING        306                            /*  系统没有运行                */
#define  ERROR_KERNEL_OBJECT_NULL        307                            /*  OBJECT 为空                 */
#define  ERROR_KERNEL_LOW_MEMORY         308                            /*  缺少内存                    */
#define  ERROR_KERNEL_BUFFER_NULL        309                            /*  缺少缓冲                    */
#define  ERROR_KERNEL_OPTION             310                            /*  选项出错                    */
#define  ERROR_KERNEL_VECTOR_NULL        311                            /*  中断向量出错                */
#define  ERROR_KERNEL_HOOK_NULL          312                            /*  内核钩子出错                */
#define  ERROR_KERNEL_OPT_NULL           313                            /*  内核钩子选项出错            */
#define  ERROR_KERNEL_MEMORY             314                            /*  内存地址出现错误            */
#define  ERROR_KERNEL_LOCK               315                            /*  内核被锁定了                */
#define  ERROR_KERNEL_CPU_NULL           316                            /*  指定 CPU 错误               */
#define  ERROR_KERNEL_HOOK_FULL          317                            /*  hook 表已满                 */
#define  ERROR_KERNEL_KEY_CONFLICT       318                            /*  key 冲突                    */

/*********************************************************************************************************
  线程错误 500 - 1000
*********************************************************************************************************/

#define  ERROR_THREAD_STACKSIZE_LACK     501                            /*  堆栈太小                    */
#define  ERROR_THREAD_STACK_NULL         502                            /*  缺少堆栈                    */
#define  ERROR_THREAD_FP_STACK_NULL      503                            /*  浮点堆栈                    */
#define  ERROR_THREAD_ATTR_NULL          504                            /*  缺少属性块                  */
#define  ERROR_THREAD_PRIORITY_WRONG     505                            /*  优先级错误                  */
#define  ERROR_THREAD_WAIT_TIMEOUT       506                            /*  等待超时                    */
#define  ERROR_THREAD_NULL               507                            /*  线程句柄无效                */
#define  ERROR_THREAD_FULL               508                            /*  系统线程已满                */
#define  ERROR_THREAD_NOT_INIT           509                            /*  线程没有初始化              */
#define  ERROR_THREAD_NOT_SUSPEND        510                            /*  线程没有被挂起              */
#define  ERROR_THREAD_VAR_FULL           511                            /*  没有变量控制块可用          */
#define  ERROR_THERAD_VAR_NULL           512                            /*  控制块无效                  */
#define  ERROR_THREAD_VAR_NOT_EXIST      513                            /*  没有找到合适的控制块        */
#define  ERROR_THREAD_NOT_READY          514                            /*  线程没有就绪                */
#define  ERROR_THREAD_IN_SAFE            515                            /*  线程处于安全模式            */
#define  ERROR_THREAD_OTHER_DELETE       516                            /*  已经有其他线程在等待删除    */
#define  ERROR_THREAD_JOIN_SELF          517                            /*  线程合并自己                */
#define  ERROR_THREAD_DETACHED           518                            /*  线程已经设定为不可合并      */
#define  ERROR_THREAD_JOIN               519                            /*  线程已经和其他线程合并      */
#define  ERROR_THREAD_NOT_SLEEP          520                            /*  线程并没有睡眠              */
#define  ERROR_THREAD_NOTEPAD_INDEX      521                            /*  记事本索引出错              */
#define  ERROR_THREAD_OPTION             522                            /*  线程选项与执行操作不符      */
#define  ERROR_THREAD_RESTART_SELF       523                            /*  没有启动信号系统, 不能      */
                                                                        /*  重新启动自己                */
#define  ERROR_THREAD_DELETE_SELF        524                            /*  没有启动信号系统  不能      */
                                                                        /*  删除自己                    */
#define  ERROR_THREAD_NEED_SIGNAL_SPT    525                            /*  需要信号系统支持            */
#define  ERROR_THREAD_DISCANCEL          526                            /*  线程设置了 DISCANCEL 标志   */

/*********************************************************************************************************
  时间错误 1000 - 1500
*********************************************************************************************************/

#define  ERROR_TIME_NULL                1000                            /*  时间为空                    */

/*********************************************************************************************************
  事件错误 1500 - 2000
*********************************************************************************************************/

#define  ERROR_EVENT_MAX_COUNTER_NULL   1500                            /*  最大值错误                  */
#define  ERROR_EVENT_INIT_COUNTER       1501                            /*  初始值错误                  */
#define  ERROR_EVENT_NULL               1502                            /*  事件控制块错误              */
#define  ERROR_EVENT_FULL               1503                            /*  事件控制块已用完            */
#define  ERROR_EVENT_TYPE               1504                            /*  事件类型出错                */
#define  ERROR_EVENT_WAS_DELETED        1505                            /*  事件已经被删除              */
#define  ERROR_EVENT_NOT_OWN            1506                            /*  没有事件所有权              */

/*********************************************************************************************************
  中断错误 1550 - 2000
*********************************************************************************************************/

#define  ERROR_INTER_LEVEL_NULL         1550                            /*  LOCK 入口参数为空           */

/*********************************************************************************************************
  事件错误 2000 - 2500
*********************************************************************************************************/

#define  ERROR_EVENTSET_NULL            2000                            /*  事件集没有                  */
#define  ERROR_EVENTSET_FULL            2001                            /*  事件集已满                  */
#define  ERROR_EVENTSET_TYPE            2002                            /*  事件集类型错误              */
#define  ERROR_EVENTSET_WAIT_TYPE       2003                            /*  事件等待方法错              */
#define  ERROR_EVENTSET_WAS_DELETED     2004                            /*  事件集已经被删除            */
#define  ERROR_EVENTSET_OPTION          2005                            /*  事件选项出错                */

/*********************************************************************************************************
  消息队列错误 2500 - 3000
*********************************************************************************************************/

#define  ERROR_MSGQUEUE_MAX_COUNTER_NULL    2500                        /*  消息数量错                  */
#define  ERROR_MSGQUEUE_MAX_LEN_NULL        2501                        /*  最长长度空                  */
#define  ERROR_MSGQUEUE_FULL                2502                        /*  消息队列满                  */
#define  ERROR_MSGQUEUE_NULL                2503                        /*  消息队列空                  */
#define  ERROR_MSGQUEUE_TYPE                2504                        /*  消息类型错                  */
#define  ERROR_MSGQUEUE_WAS_DELETED         2505                        /*  队列被删除了                */
#define  ERROR_MSGQUEUE_MSG_NULL            2506                        /*  消息错                      */
#define  ERROR_MSGQUEUE_MSG_LEN             2507                        /*  消息长度错                  */
#define  ERROR_MSGQUEUE_OPTION              2508                        /*  OPTION 选项错               */

/*********************************************************************************************************
  定时器错误 3000 - 3500
*********************************************************************************************************/

#define  ERROR_TIMER_FULL                   3000                        /*  定时器已满                  */
#define  ERROR_TIMER_NULL                   3001                        /*  定时器为空                  */
#define  ERROR_TIMER_CALLBACK_NULL          3002                        /*  回调为空                    */
#define  ERROR_TIMER_ISR                    3003                        /*  定时器不能完成该操作        */ 
                                                                        /*  不能在高于 _ItimerThread    */
                                                                        /*  线程中操作 ITIMER 链表      */
#define  ERROR_TIMER_TIME                   3004                        /*  时间出错                    */
#define  ERROR_TIMER_OPTION                 3005                        /*  选项出错                    */

/*********************************************************************************************************
  PARTITION 3500 - 4000
*********************************************************************************************************/

#define  ERROR_PARTITION_FULL               3500                        /*  缺乏PARTITION控制块         */
#define  ERROR_PARTITION_NULL               3501                        /*  PARTITION相关参数为空       */
#define  ERROR_PARTITION_BLOCK_COUNTER      3502                        /*  分块数量错误                */
#define  ERROR_PARTITION_BLOCK_SIZE         3503                        /*  分块大小错误                */
#define  ERROR_PARTITION_BLOCK_USED         3504                        /*  有分块被使用                */

/*********************************************************************************************************
  REGION 4000 - 4500
*********************************************************************************************************/

#define  ERROR_REGION_FULL                  4000                        /*  缺乏控制块                  */
#define  ERROR_REGION_NULL                  4001                        /*  控制块错误                  */
#define  ERROR_REGION_SIZE                  4002                        /*  分区大小太小                */
#define  ERROR_REGION_USED                  4003                        /*  分区正在使用                */
#define  ERROR_REGION_ALIGN                 4004                        /*  对齐关系错误                */
#define  ERROR_REGION_NOMEM                 4005                        /*  没有内存可供分配            */

/*********************************************************************************************************
  RMS 4500 - 5000
*********************************************************************************************************/

#define  ERROR_RMS_FULL                     4500                        /*  缺乏控制块                  */
#define  ERROR_RMS_NULL                     4501                        /*  控制块错误                  */
#define  ERROR_RMS_TICK                     4502                        /*  TICK 错误                   */
#define  ERROR_RMS_WAS_CHANGED              4503                        /*  状态被改变                  */
#define  ERROR_RMS_STATUS                   4504                        /*  状态错误                    */

/*********************************************************************************************************
  RTC 5000 - 5500
*********************************************************************************************************/

#define  ERROR_RTC_NULL                     5000                        /*  没有 RTC                    */
#define  ERROR_RTC_TIMEZONE                 5001                        /*  时区错误                    */

/*********************************************************************************************************
  VMM 5500 - 6000
*********************************************************************************************************/

#define  ERROR_VMM_LOW_PHYSICAL_PAGE        5500                        /*  缺少物理页面                */
#define  ERROR_VMM_LOW_LEVEL                5501                        /*  底层软件致命错误            */
#define  ERROR_VMM_PHYSICAL_PAGE            5502                        /*  物理页面错误                */
#define  ERROR_VMM_VIRTUAL_PAGE             5503                        /*  虚拟页面错误                */
#define  ERROR_VMM_PHYSICAL_ADDR            5504                        /*  物理地址错误                */
#define  ERROR_VMM_VIRTUAL_ADDR             5505                        /*  虚拟地址错误                */
#define  ERROR_VMM_ALIGN                    5506                        /*  对齐关系错误                */
#define  ERROR_VMM_PAGE_INVAL               5507                        /*  页面无效                    */
#define  ERROR_VMM_LOW_PAGE                 5508                        /*  缺少页面                    */

#endif                                                                  /*  __K_ERROR_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
