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
** 文   件   名: if_whandler.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 06 月 25 日
**
** 描        述: Linux 兼容无线网络新型驱动接口.
*********************************************************************************************************/

#ifndef __IF_WHANDLER_H
#define __IF_WHANDLER_H

#include "sys/compiler.h"
#include "if_wireless.h"
#include "if_ether.h"

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

#define CONFIG_WIRELESS_EXT     1                                       /*  use CONFIG_WIRELESS_EXT cfg */

/*********************************************************************************************************
 --- VERSION ---
*********************************************************************************************************/
/*********************************************************************************************************
 This constant is used to know which version of the driver API is
 * available. Hopefully, this will be pretty stable and no changes
 * will be needed...
 * I just plan to increment with each new version.
*********************************************************************************************************/

#define IW_HANDLER_VERSION      8

/*********************************************************************************************************
 --- CONSTANTS ---
*********************************************************************************************************/
/*********************************************************************************************************
 Enhanced spy support available
*********************************************************************************************************/

#define IW_WIRELESS_SPY
#define IW_WIRELESS_THRSPY

/*********************************************************************************************************
 Special error message for the driver to indicate that we
 * should do a commit after return from the iw_handler
*********************************************************************************************************/

#define EIWCOMMIT               EINPROGRESS

/*********************************************************************************************************
 Flags available in struct iw_request_info
*********************************************************************************************************/

#define IW_REQUEST_FLAG_COMPAT  0x0001          /* Compat ioctl call                                    */

/*********************************************************************************************************
 Type of headers we know about (basically union iwreq_data)
*********************************************************************************************************/

#define IW_HEADER_TYPE_NULL     0               /* Not available                                        */
#define IW_HEADER_TYPE_CHAR     2               /* char [IFNAMSIZ]                                      */
#define IW_HEADER_TYPE_UINT     4               /* u32                                                  */
#define IW_HEADER_TYPE_FREQ     5               /* struct iw_freq                                       */
#define IW_HEADER_TYPE_ADDR     6               /* struct sockaddr                                      */
#define IW_HEADER_TYPE_POINT    8               /* struct iw_point                                      */
#define IW_HEADER_TYPE_PARAM    9               /* struct iw_param                                      */
#define IW_HEADER_TYPE_QUAL     10              /* struct iw_quality                                    */

/*********************************************************************************************************
 Handling flags
*********************************************************************************************************/
/*********************************************************************************************************
 Most are not implemented. I just use them as a reminder of some
 * cool features we might need one day ;-)
*********************************************************************************************************/

#define IW_DESCR_FLAG_NONE      0x0000          /* Obvious                                              */

/*********************************************************************************************************
 Wrapper level flags
*********************************************************************************************************/

#define IW_DESCR_FLAG_DUMP      0x0001          /* Not part of the dump command                         */
#define IW_DESCR_FLAG_EVENT     0x0002          /* Generate an event on SET                             */
#define IW_DESCR_FLAG_RESTRICT  0x0004          /* GET : request is ROOT only                           */
                                                /* SET : Omit payload from generated iwevent            */
#define IW_DESCR_FLAG_NOMAX     0x0008          /* GET : no limit on request size                       */

/*********************************************************************************************************
 Driver level flags
*********************************************************************************************************/

#define IW_DESCR_FLAG_WAIT      0x0100          /* Wait for driver event                                */

/*********************************************************************************************************
 --- TYPES ---
*********************************************************************************************************/
/*********************************************************************************************************
 --- WIRELESS HANDLER ---
*********************************************************************************************************/
/*********************************************************************************************************
 A wireless handler is just a standard function, that looks like the
 * ioctl handler.
 * We also define there how a handler list look like... As the Wireless
 * Extension space is quite dense, we use a simple array, which is faster
 * (that's the perfect hash table ;-).
*********************************************************************************************************/
/*********************************************************************************************************
 Meta data about the request passed to the iw_handler.
 * Most handlers can safely ignore what's in there.
 * The 'cmd' field might come handy if you want to use the same handler
 * for multiple command...
 * This struct is also my long term insurance. I can add new fields here
 * without breaking the prototype of iw_handler...
*********************************************************************************************************/

struct iw_request_info {
    u16         cmd;                            /* Wireless Extension command                           */
    u16         flags;                          /* More to come ;-)                                     */
};

struct netdev;                                  /* net device                                           */

/*********************************************************************************************************
 This is how a function handling a Wireless Extension should look
 * like (both get and set, standard and private).
*********************************************************************************************************/

typedef int (*iw_handler)(struct netdev *dev, struct iw_request_info *info,
                          union iwreq_data *wrqu, char *extra);

/*********************************************************************************************************
 This define all the handler that the driver export.
 * As you need only one per driver type, please use a static const
 * shared by all driver instances... Same for the members...
 * This will be linked from net_device in <netdev.h>
*********************************************************************************************************/

#ifndef CONFIG_WEXT_PRIV
#define CONFIG_WEXT_PRIV                        /* SylixOS default have this macro                      */
#endif                                          /* CONFIG_WEXT_PRIV                                     */

struct iw_handler_def {
    /* 
     * Array of handlers for standard ioctls
     * We will call dev->wireless_handlers->standard[ioctl - SIOCIWFIRST]
     */
    const iw_handler            *standard;
    
    /* 
     * Number of handlers defined (more precisely, index of the
     * last defined handler + 1)
     */
    u16                         num_standard;

#ifdef CONFIG_WEXT_PRIV
    u16                         num_private;
    /* 
     * Number of private arg description 
     */
    u16                         num_private_args;
    
    /* 
     * Array of handlers for private ioctls
     * Will call dev->wireless_handlers->private[ioctl - SIOCIWFIRSTPRIV]
     */
    const iw_handler            *private;

    /* 
     * Arguments of private handler. This one is just a list, so you
     * can put it in any order you want and should not leave holes...
     * We will automatically export that to user space... 
     */
    const struct iw_priv_args   *private_args;
#endif

    /* 
     * New location of get_wireless_stats, to de-bloat struct net_device.
     * The old pointer in struct net_device will be gradually phased
     * out, and drivers are encouraged to use this one... 
     */
    struct iw_statistics*       (*get_wireless_stats)(struct netdev *dev);
};

/*********************************************************************************************************
 --- IOCTL DESCRIPTION ---
*********************************************************************************************************/
/*********************************************************************************************************
 One of the main goal of the new interface is to deal entirely with
 * user space/kernel space memory move.
 * For that, we need to know :
 *    o if iwreq is a pointer or contain the full data
 *    o what is the size of the data to copy
 *
 * For private IOCTLs, we use the same rules as used by iwpriv and
 * defined in struct iw_priv_args.
 *
 * For standard IOCTLs, things are quite different and we need to
 * use the stuctures below. Actually, this struct is also more
 * efficient, but that's another story...
*********************************************************************************************************/
/*********************************************************************************************************
 Describe how a standard IOCTL looks like.
*********************************************************************************************************/

struct iw_ioctl_description {
    u8      header_type;                    /* NULL, iw_point or other                                  */
    u8      token_type;                     /* Future                                                   */
    u16     token_size;                     /* Granularity of payload                                   */
    u16     min_tokens;                     /* Min acceptable token number                              */
    u16     max_tokens;                     /* Max acceptable token number                              */
    u32     flags;                          /* Special handling of the request                          */
};

/*********************************************************************************************************
 Need to think of short header translation table. Later.
*********************************************************************************************************/

/*********************************************************************************************************
 --- ENHANCED SPY SUPPORT ---
*********************************************************************************************************/
/*********************************************************************************************************
 In the old days, the driver was handling spy support all by itself.
 * Now, the driver can delegate this task to Wireless Extensions.
 * It needs to include this struct in its private part and use the
 * standard spy iw_handler.
*********************************************************************************************************/
/*********************************************************************************************************
 Instance specific spy data, i.e. addresses spied and quality for them.
*********************************************************************************************************/

struct iw_spy_data {
    /*
     * Standard spy support
     */
    int                 spy_number;
    u_char              spy_address[IW_MAX_SPY][ETH_ALEN];
    struct iw_quality   spy_stat[IW_MAX_SPY];
    
    /* 
     * Enhanced spy support (event) 
     */
    struct iw_quality   spy_thr_low;        /* Low threshold                                            */
    struct iw_quality   spy_thr_high;       /* High threshold                                           */
    u_char              spy_thr_under[IW_MAX_SPY];
};

/*********************************************************************************************************
 --- DEVICE WIRELESS DATA ---
*********************************************************************************************************/
/*********************************************************************************************************
 This is all the wireless data specific to a device instance that
 * is managed by the core of Wireless Extensions or the 802.11 layer.
 * We only keep pointer to those structures, so that a driver is free
 * to share them between instances.
 * This structure should be initialised before registering the device.
 * Access to this data follow the same rules as any other struct net_device
 * data (i.e. valid as long as struct net_device exist, same locking rules).
*********************************************************************************************************/

struct iw_public_data {
    /* 
     * Driver enhanced spy support 
     */
    struct iw_spy_data      *spy_data;
    
    /* 
     * Legacy structure managed by the ipw2x00-specific IEEE 802.11 layer 
     */
    void                    *dev_priv;
};

/*********************************************************************************************************
 --- PROTOTYPES ---
*********************************************************************************************************/
/*********************************************************************************************************
 Functions part of the Wireless Extensions (defined in net/core/wireless.c).
 * Those may be called only within the kernel.
*********************************************************************************************************/
/*********************************************************************************************************
 First : function strictly used inside the kernel
*********************************************************************************************************/
/* 
 * Handle /proc/net/wireless, called in net/code/dev.c 
 */
int dev_get_wireless_info(char *buffer, char **start, off_t offset, int length);

/*********************************************************************************************************
 Second : functions that may be called by driver modules
*********************************************************************************************************/
/* 
 * Send a single event to user space 
 */
void wireless_send_event(struct netdev *dev, unsigned int cmd,
                         union iwreq_data *wrqu, const char *extra);

/*********************************************************************************************************
 We may need a function to send a stream of events to user space.
 * More on that later... 
*********************************************************************************************************/
/* 
 * Standard handler for SIOCSIWSPY
 */
int iw_handler_set_spy(struct netdev *dev, struct iw_request_info *info,
                       union iwreq_data *wrqu, char *extra);
                       
/* 
 * Standard handler for SIOCGIWSPY
 */
int iw_handler_get_spy(struct netdev *dev, struct iw_request_info *info,
                       union iwreq_data *wrqu, char *extra);
                       
/* 
 * Standard handler for SIOCSIWTHRSPY
 */
int iw_handler_set_thrspy(struct netdev *dev, struct iw_request_info *info,
                          union iwreq_data *wrqu, char *extra);
                          
/* 
 * Standard handler for SIOCGIWTHRSPY
 */
int iw_handler_get_thrspy(struct netdev *dev, struct iw_request_info *info,
                          union iwreq_data *wrqu, char *extra);
                          
/* 
 * Driver call to update spy records
 */
void wireless_spy_update(struct netdev *dev, unsigned char *address,
                         struct iw_quality *wstats);
                         
/*********************************************************************************************************
 wext 
*********************************************************************************************************/

struct iw_statistics *get_wireless_stats(struct netdev *dev);

int iw_handler_get_iwstats(struct netdev            *dev,
                           struct iw_request_info   *info,
                           union iwreq_data         *wrqu,
                           char                     *extra);

/*********************************************************************************************************
 --- INLINE FUNTIONS ---
*********************************************************************************************************/
/*********************************************************************************************************
 Function that are so simple that it's more efficient inlining them
 Notice: SylixOS do not use CONFIG_COMPAT
*********************************************************************************************************/

static inline int iwe_stream_lcp_len (struct iw_request_info *info)
{
#ifdef CONFIG_COMPAT
    if (info->flags & IW_REQUEST_FLAG_COMPAT)
        return IW_EV_COMPAT_LCP_LEN;
#endif
    return IW_EV_LCP_LEN;
}

static inline int iwe_stream_point_len (struct iw_request_info *info)
{
#ifdef CONFIG_COMPAT
    if (info->flags & IW_REQUEST_FLAG_COMPAT)
        return IW_EV_COMPAT_POINT_LEN;
#endif
    return IW_EV_POINT_LEN;
}

static inline int iwe_stream_event_len_adjust (struct iw_request_info *info,
                          int event_len)
{
#ifdef CONFIG_COMPAT
    if (info->flags & IW_REQUEST_FLAG_COMPAT) {
        event_len -= IW_EV_LCP_LEN;
        event_len += IW_EV_COMPAT_LCP_LEN;
    }
#endif
    return event_len;
}

/*********************************************************************************************************
 Wrapper to add an Wireless Event to a stream of events.
*********************************************************************************************************/

static inline char *
iwe_stream_add_event (struct iw_request_info *info, char *stream, char *ends,
                      struct iw_event *iwe, int event_len)
{
    int lcp_len = iwe_stream_lcp_len(info);

    event_len = iwe_stream_event_len_adjust(info, event_len);

    /* 
     * Check if it's possible 
     */
    if (likely((stream + event_len) < ends)) {
        iwe->len = event_len;
        /* 
         * Beware of alignement issues on 64 bits 
         */
        lib_memcpy(stream, (char *) iwe, IW_EV_LCP_PK_LEN);
        lib_memcpy(stream + lcp_len, &iwe->u,
                   event_len - lcp_len);
        stream += event_len;
    }
    
    return stream;
}

/*********************************************************************************************************
 Wrapper to add an short Wireless Event containing a pointer to a
 * stream of events.
*********************************************************************************************************/

static inline char *
iwe_stream_add_point (struct iw_request_info *info, char *stream, char *ends,
                      struct iw_event *iwe, char *extra)
{
    int event_len = iwe_stream_point_len(info) + iwe->u.data.length;
    int point_len = iwe_stream_point_len(info);
    int lcp_len   = iwe_stream_lcp_len(info);

    /* 
     * Check if it's possible 
     */
    if (likely((stream + event_len) < ends)) {
        iwe->len = event_len;
        lib_memcpy(stream, (char *) iwe, IW_EV_LCP_PK_LEN);
        lib_memcpy(stream + lcp_len,
                   ((char *) &iwe->u) + IW_EV_POINT_OFF,
                   IW_EV_POINT_PK_LEN - IW_EV_LCP_PK_LEN);
        lib_memcpy(stream + point_len, extra, iwe->u.data.length);
        stream += event_len;
    }
    
    return stream;
}

/*********************************************************************************************************
 Wrapper to add a value to a Wireless Event in a stream of events.
 * Be careful, this one is tricky to use properly :
 * At the first run, you need to have (value = event + IW_EV_LCP_LEN).
*********************************************************************************************************/

static inline char *
iwe_stream_add_value (struct iw_request_info *info, char *event, char *value,
                      char *ends, struct iw_event *iwe, int event_len)
{
    int lcp_len = iwe_stream_lcp_len(info);

    event_len -= IW_EV_LCP_LEN;                                         /* Don't duplicate LCP          */

    /* 
     * Check if it's possible 
     */
    if (likely((value + event_len) < ends)) {
        /* Add new value */
        lib_memcpy(value, &iwe->u, event_len);
        value += event_len;
        /* Patch LCP */
        iwe->len = value - event;
        lib_memcpy(event, (char *) iwe, lcp_len);
    }
    
    return value;
}

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_NET_EN               */
#endif                                                                  /*  __IF_WHANDLER_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
