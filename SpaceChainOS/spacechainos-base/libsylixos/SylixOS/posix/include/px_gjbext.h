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
** 文   件   名: px_gjbext.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 04 月 13 日
**
** 描        述: GJB7714 扩展接口.
*********************************************************************************************************/

#ifndef __PX_GJBEXT_H
#define __PX_GJBEXT_H

#include "SylixOS.h"                                                    /*  操作系统头文件              */

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_POSIX_EN > 0) && (LW_CFG_GJB7714_EN > 0)

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  data type
*********************************************************************************************************/

#ifndef __u8_defined
typedef uint8_t                 u8;
typedef SINT8                   s8;
#define __u8_defined            1
#define __s8_defined            1
#endif

#ifndef __u16_defined
typedef uint16_t                u16;
typedef int16_t                 s16;
#define __u16_defined           1
#define __s16_defined           1
#endif

#ifndef __u32_defined
typedef uint32_t                u32;
typedef int32_t                 s32;
#define __u32_defined           1
#define __s32_defined           1
#endif

#ifndef __u64_defined
typedef uint64_t                u64;
typedef int64_t                 s64;
#define __u64_defined           1
#define __s64_defined           1
#endif

typedef int                     boolean;
typedef int                     OS_STATUS;
typedef LW_CLASS_TCB_DESC       pthread_info_t;
typedef ARCH_REG_CTX            REG_SET;

#define OS_OK                   ERROR_NONE
#define OS_ERROR                PX_ERROR

/*********************************************************************************************************
  interrupt (ONLY kernel program can use the following routine)
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
typedef void      (*EXC_HANDLER)(int type);

LW_API int          int_install_handler(const char  *name, 
                                        int          vecnum, 
                                        int          prio, 
                                        void       (*handler)(void *),
                                        void        *param);
LW_API int          int_uninstall_handler(int   vecnum);
LW_API int          int_lock(void);
LW_API void         int_unlock(int level);
LW_API void         int_enable_pic(uint32_t irq);
LW_API void         int_disable_pic(uint32_t irq);
LW_API EXC_HANDLER  exception_handler_set(EXC_HANDLER exc_handler);
LW_API int          shared_int_install(int     vecnum, 
                                       void  (*handler)(void *),
                                       void   *param);
LW_API int          shared_int_uninstall(int     vecnum, 
                                         void  (*handler)(void *));
#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  time related (watchdog timer user function will called by t_itimer thread in kernel task)
*********************************************************************************************************/

LW_API uint64_t     tick_get(void);
LW_API void         tick_set(uint64_t  ticks);
LW_API uint32_t     sys_clk_rate_get(void);
LW_API int          sys_clk_rate_set(int ticks_per_second);

typedef LW_OBJECT_HANDLE    wdg_t;

typedef struct {
    int     run;
    ULONG   cnt;
} wdg_info_t;

LW_API int          wdg_create(wdg_t *wdg_id);
LW_API int          wdg_start(wdg_t wdg_id, int ticks, void (*func)(void *arg), void *arg);
LW_API int          wdg_cancel(wdg_t wdg_id);
LW_API int          wdg_delete(wdg_t wdg_id);
LW_API int          wdg_getinfo(wdg_t wdg_id, wdg_info_t *info);
LW_API int          wdg_show(wdg_t wdg_id);

#if LW_CFG_PTIMER_EN > 0
typedef struct {
    int         run;
    ULONG       cnt;
    ULONG       interval;
    clockid_t   clockid;
    PVOID       reserved[8];
} timer_info_t;

LW_API int          timer_getinfo(timer_t  timer, timer_info_t  *timer_info);
#endif                                                                  /*  LW_CFG_PTIMER_EN > 0        */

/*********************************************************************************************************
  mem related
*********************************************************************************************************/

#define FIRST_FIT_ALLOCATION    0
#define BUDDY_ALLOCATION        1

LW_API int          heap_mem_init(int flag);

struct meminfo {
    ULONG   segment;
    size_t  used;
    size_t  maxused;
    size_t  free;
};

struct partinfo {
    ULONG   segment;
    size_t  used;
    size_t  maxused;
    size_t  free;
};

typedef LW_OBJECT_HANDLE    mpart_id;

LW_API int          mem_findmax(void);
LW_API int          mem_getinfo(struct meminfo *info);
LW_API void         mem_show(void);

LW_API int          mpart_module_init(int  max_parts);
LW_API int          mpart_create(char *addr, size_t  size, mpart_id *mid);
LW_API int          mpart_delete(mpart_id mid);
LW_API int          mpart_addmem(mpart_id mid, char *addr, size_t  size);
LW_API void        *mpart_alloc(mpart_id mid, size_t  size);
LW_API void        *mpart_memalign(mpart_id mid, size_t  alignment, size_t  size);
LW_API void        *mpart_realloc(mpart_id mid, char *addr, size_t  size);
LW_API int          mpart_free(mpart_id mid, char *addr);
LW_API int          mpart_findmaxfree(mpart_id mid);
LW_API int          mpart_getinfo(mpart_id mid, struct partinfo  *pi);

/*********************************************************************************************************
  device related (ONLY kernel program can use the following routine)
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
LW_API void         global_std_set(int  stdfd, int  newfd);
LW_API int          global_std_get(int  stdfd);

LW_API int          register_driver(dev_t                   major, 
                                    struct file_operations *driver_table, 
                                    dev_t                  *registered_major);
LW_API OS_STATUS    unregister_driver(dev_t  major);
LW_API int          register_device(const char *path, dev_t  dev, void *registered);
LW_API int          unregister_device(const char *path);
#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  net related
*********************************************************************************************************/

#if LW_CFG_NET_EN > 0
LW_API int          ifconfig(char *name, char *ip, char *netmask);
LW_API void         if_show(void);
LW_API void         mbuf_show(void);
LW_API void         udp_show(void);
LW_API void         tcp_show(void);
LW_API void         routes_show(void);
#endif                                                                  /*  LW_CFG_NET_EN > 0           */

/*********************************************************************************************************
  fs related
*********************************************************************************************************/

LW_API int          gjb_mount(const char *fs, const char *dev, const char *mpath);
LW_API int          gjb_umount(const char *mpath);
LW_API int          gjb_format(const char *fs, const char *dev);
LW_API int          gjb_cat(char *pathname);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_GJB7714_EN > 0       */
#endif                                                                  /*  __PX_GJBEXT_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
