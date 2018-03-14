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
** 文   件   名: lwip_wlext.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 06 月 26 日
**
** 描        述: lwip 无线网络扩展.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_WIRELESS_EN > 0)
#include "netdev.h"
#include "net/if.h"
#include "net/if_wireless.h"
#include "net/if_whandler.h"
#include "net/if_event.h"
/*********************************************************************************************************
  ioctl 函数类型
*********************************************************************************************************/
typedef int (*wext_ioctl_func)(struct netdev *, struct iwreq *,
                               unsigned int, struct iw_request_info *,
                               iw_handler);
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
extern int iw_handler_get_private(struct netdev          *dev,
                                  struct iw_request_info *info,
                                  union iwreq_data       *wrqu,
                                  char                   *extra);
extern int ioctl_private_call(struct netdev            *dev, 
                              struct iwreq             *iwr,
                              unsigned int              cmd, 
                              struct iw_request_info   *info,
                              iw_handler                handler);
/*********************************************************************************************************
  标准 ioctl 表
*********************************************************************************************************/
static const struct iw_ioctl_description    standard_ioctl[] = {
#ifdef __GNUC__
    [IW_IOCTL_IDX(SIOCSIWCOMMIT)] = {
        .header_type = IW_HEADER_TYPE_NULL,
    },
    [IW_IOCTL_IDX(SIOCGIWNAME)] = {
        .header_type = IW_HEADER_TYPE_CHAR,
        .flags       = IW_DESCR_FLAG_DUMP,
    },
    [IW_IOCTL_IDX(SIOCSIWNWID)] = {
        .header_type = IW_HEADER_TYPE_PARAM,
        .flags       = IW_DESCR_FLAG_EVENT,
    },
    [IW_IOCTL_IDX(SIOCGIWNWID)] = {
        .header_type = IW_HEADER_TYPE_PARAM,
        .flags       = IW_DESCR_FLAG_DUMP,
    },
    [IW_IOCTL_IDX(SIOCSIWFREQ)] = {
        .header_type = IW_HEADER_TYPE_FREQ,
        .flags       = IW_DESCR_FLAG_EVENT,
    },
    [IW_IOCTL_IDX(SIOCGIWFREQ)] = {
        .header_type = IW_HEADER_TYPE_FREQ,
        .flags       = IW_DESCR_FLAG_DUMP,
    },
    [IW_IOCTL_IDX(SIOCSIWMODE)] = {
        .header_type = IW_HEADER_TYPE_UINT,
        .flags       = IW_DESCR_FLAG_EVENT,
    },
    [IW_IOCTL_IDX(SIOCGIWMODE)] = {
        .header_type = IW_HEADER_TYPE_UINT,
        .flags       = IW_DESCR_FLAG_DUMP,
    },
    [IW_IOCTL_IDX(SIOCSIWSENS)] = {
        .header_type = IW_HEADER_TYPE_PARAM,
    },
    [IW_IOCTL_IDX(SIOCGIWSENS)] = {
        .header_type = IW_HEADER_TYPE_PARAM,
    },
    [IW_IOCTL_IDX(SIOCSIWRANGE)] = {
        .header_type = IW_HEADER_TYPE_NULL,
    },
    [IW_IOCTL_IDX(SIOCGIWRANGE)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .max_tokens  = sizeof(struct iw_range),
        .flags       = IW_DESCR_FLAG_DUMP,
    },
    [IW_IOCTL_IDX(SIOCSIWPRIV)] = {
        .header_type = IW_HEADER_TYPE_NULL,
    },
    [IW_IOCTL_IDX(SIOCGIWPRIV)] = {                                     /* (handled directly by us)     */
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = sizeof(struct iw_priv_args),
        .max_tokens  = 16,
        .flags       = IW_DESCR_FLAG_NOMAX,
    },
    [IW_IOCTL_IDX(SIOCSIWSTATS)] = {
        .header_type = IW_HEADER_TYPE_NULL,
    },
    [IW_IOCTL_IDX(SIOCGIWSTATS)] = {                                    /* (handled directly by us)     */
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .max_tokens  = sizeof(struct iw_statistics),
        .flags       = IW_DESCR_FLAG_DUMP,
    },
    [IW_IOCTL_IDX(SIOCSIWSPY)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = sizeof(struct sockaddr),
        .max_tokens  = IW_MAX_SPY,
    },
    [IW_IOCTL_IDX(SIOCGIWSPY)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = sizeof(struct sockaddr) +
                  sizeof(struct iw_quality),
        .max_tokens  = IW_MAX_SPY,
    },
    [IW_IOCTL_IDX(SIOCSIWTHRSPY)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = sizeof(struct iw_thrspy),
        .min_tokens  = 1,
        .max_tokens  = 1,
    },
    [IW_IOCTL_IDX(SIOCGIWTHRSPY)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = sizeof(struct iw_thrspy),
        .min_tokens  = 1,
        .max_tokens  = 1,
    },
    [IW_IOCTL_IDX(SIOCSIWAP)] = {
        .header_type = IW_HEADER_TYPE_ADDR,
    },
    [IW_IOCTL_IDX(SIOCGIWAP)] = {
        .header_type = IW_HEADER_TYPE_ADDR,
        .flags       = IW_DESCR_FLAG_DUMP,
    },
    [IW_IOCTL_IDX(SIOCSIWMLME)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .min_tokens  = sizeof(struct iw_mlme),
        .max_tokens  = sizeof(struct iw_mlme),
    },
    [IW_IOCTL_IDX(SIOCGIWAPLIST)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = sizeof(struct sockaddr) +
                  sizeof(struct iw_quality),
        .max_tokens  = IW_MAX_AP,
        .flags       = IW_DESCR_FLAG_NOMAX,
    },
    [IW_IOCTL_IDX(SIOCSIWSCAN)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .min_tokens  = 0,
        .max_tokens  = sizeof(struct iw_scan_req),
    },
    [IW_IOCTL_IDX(SIOCGIWSCAN)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .max_tokens  = IW_SCAN_MAX_DATA,
        .flags       = IW_DESCR_FLAG_NOMAX,
    },
    [IW_IOCTL_IDX(SIOCSIWESSID)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .max_tokens  = IW_ESSID_MAX_SIZE,
        .flags       = IW_DESCR_FLAG_EVENT,
    },
    [IW_IOCTL_IDX(SIOCGIWESSID)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .max_tokens  = IW_ESSID_MAX_SIZE,
        .flags       = IW_DESCR_FLAG_DUMP,
    },
    [IW_IOCTL_IDX(SIOCSIWNICKN)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .max_tokens  = IW_ESSID_MAX_SIZE,
    },
    [IW_IOCTL_IDX(SIOCGIWNICKN)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .max_tokens  = IW_ESSID_MAX_SIZE,
    },
    [IW_IOCTL_IDX(SIOCSIWRATE)] = {
        .header_type = IW_HEADER_TYPE_PARAM,
    },
    [IW_IOCTL_IDX(SIOCGIWRATE)] = {
        .header_type = IW_HEADER_TYPE_PARAM,
    },
    [IW_IOCTL_IDX(SIOCSIWRTS)] = {
        .header_type = IW_HEADER_TYPE_PARAM,
    },
    [IW_IOCTL_IDX(SIOCGIWRTS)] = {
        .header_type = IW_HEADER_TYPE_PARAM,
    },
    [IW_IOCTL_IDX(SIOCSIWFRAG)] = {
        .header_type = IW_HEADER_TYPE_PARAM,
    },
    [IW_IOCTL_IDX(SIOCGIWFRAG)] = {
        .header_type = IW_HEADER_TYPE_PARAM,
    },
    [IW_IOCTL_IDX(SIOCSIWTXPOW)] = {
        .header_type = IW_HEADER_TYPE_PARAM,
    },
    [IW_IOCTL_IDX(SIOCGIWTXPOW)] = {
        .header_type = IW_HEADER_TYPE_PARAM,
    },
    [IW_IOCTL_IDX(SIOCSIWRETRY)] = {
        .header_type = IW_HEADER_TYPE_PARAM,
    },
    [IW_IOCTL_IDX(SIOCGIWRETRY)] = {
        .header_type = IW_HEADER_TYPE_PARAM,
    },
    [IW_IOCTL_IDX(SIOCSIWENCODE)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .max_tokens  = IW_ENCODING_TOKEN_MAX,
        .flags       = IW_DESCR_FLAG_EVENT | IW_DESCR_FLAG_RESTRICT,
    },
    [IW_IOCTL_IDX(SIOCGIWENCODE)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .max_tokens  = IW_ENCODING_TOKEN_MAX,
        .flags       = IW_DESCR_FLAG_DUMP | IW_DESCR_FLAG_RESTRICT,
    },
    [IW_IOCTL_IDX(SIOCSIWPOWER)] = {
        .header_type = IW_HEADER_TYPE_PARAM,
    },
    [IW_IOCTL_IDX(SIOCGIWPOWER)] = {
        .header_type = IW_HEADER_TYPE_PARAM,
    },
    [IW_IOCTL_IDX(SIOCSIWGENIE)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .max_tokens  = IW_GENERIC_IE_MAX,
    },
    [IW_IOCTL_IDX(SIOCGIWGENIE)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .max_tokens  = IW_GENERIC_IE_MAX,
    },
    [IW_IOCTL_IDX(SIOCSIWAUTH)] = {
        .header_type = IW_HEADER_TYPE_PARAM,
    },
    [IW_IOCTL_IDX(SIOCGIWAUTH)] = {
        .header_type = IW_HEADER_TYPE_PARAM,
    },
    [IW_IOCTL_IDX(SIOCSIWENCODEEXT)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .min_tokens  = sizeof(struct iw_encode_ext),
        .max_tokens  = sizeof(struct iw_encode_ext) +
                       IW_ENCODING_TOKEN_MAX,
    },
    [IW_IOCTL_IDX(SIOCGIWENCODEEXT)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .min_tokens  = sizeof(struct iw_encode_ext),
        .max_tokens  = sizeof(struct iw_encode_ext) +
                       IW_ENCODING_TOKEN_MAX,
    },
    [IW_IOCTL_IDX(SIOCSIWPMKSA)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .min_tokens  = sizeof(struct iw_pmksa),
        .max_tokens  = sizeof(struct iw_pmksa),
    },
#else
    {0},
#endif                                                                  /*  __GNUC__                    */
};
static const unsigned int   standard_ioctl_num = ARRAY_SIZE(standard_ioctl);
/*********************************************************************************************************
 Meta-data about all the additional standard Wireless Extension events
 * we know about.
*********************************************************************************************************/
static const struct iw_ioctl_description    standard_event[] = {
#ifdef __GNUC__
    [IW_EVENT_IDX(IWEVTXDROP)] = {
        .header_type = IW_HEADER_TYPE_ADDR,
    },
    [IW_EVENT_IDX(IWEVQUAL)] = {
        .header_type = IW_HEADER_TYPE_QUAL,
    },
    [IW_EVENT_IDX(IWEVCUSTOM)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .max_tokens  = IW_CUSTOM_MAX,
    },
    [IW_EVENT_IDX(IWEVREGISTERED)] = {
        .header_type = IW_HEADER_TYPE_ADDR,
    },
    [IW_EVENT_IDX(IWEVEXPIRED)] = {
        .header_type = IW_HEADER_TYPE_ADDR,
    },
    [IW_EVENT_IDX(IWEVGENIE)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .max_tokens  = IW_GENERIC_IE_MAX,
    },
    [IW_EVENT_IDX(IWEVMICHAELMICFAILURE)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .max_tokens  = sizeof(struct iw_michaelmicfailure),
    },
    [IW_EVENT_IDX(IWEVASSOCREQIE)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .max_tokens  = IW_GENERIC_IE_MAX,
    },
    [IW_EVENT_IDX(IWEVASSOCRESPIE)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .max_tokens  = IW_GENERIC_IE_MAX,
    },
    [IW_EVENT_IDX(IWEVPMKIDCAND)] = {
        .header_type = IW_HEADER_TYPE_POINT,
        .token_size  = 1,
        .max_tokens  = sizeof(struct iw_pmkid_cand),
    },
#else
    {0},
#endif                                                                  /*  __GNUC__                    */
};
static const unsigned int   standard_event_num = ARRAY_SIZE(standard_event);
/*********************************************************************************************************
 Size (in bytes) of various events.
*********************************************************************************************************/
static const int    event_type_size[] = {
    IW_EV_LCP_LEN,                                                      /* IW_HEADER_TYPE_NULL          */
    0,
    IW_EV_CHAR_LEN,                                                     /* IW_HEADER_TYPE_CHAR          */
    0,
    IW_EV_UINT_LEN,                                                     /* IW_HEADER_TYPE_UINT          */
    IW_EV_FREQ_LEN,                                                     /* IW_HEADER_TYPE_FREQ          */
    IW_EV_ADDR_LEN,                                                     /* IW_HEADER_TYPE_ADDR          */
    0,
    IW_EV_POINT_LEN,                                                    /* Without variable payload     */
    IW_EV_PARAM_LEN,                                                    /* IW_HEADER_TYPE_PARAM         */
    IW_EV_QUAL_LEN,                                                     /* IW_HEADER_TYPE_QUAL          */
};
/*********************************************************************************************************
** 函数名称: wireless_send_event
** 功能描述: 发送一个事件
** 输　入  : dev       网络接口
**           cmd       命令
**           wrqu      事件参数
**           extra     额外信息
** 输　出  : 状态
** 全局变量:
** 调用模块:
*********************************************************************************************************/
void wireless_send_event (struct netdev     *dev,
                          unsigned int       cmd,
                          union iwreq_data  *wrqu,
                          const char        *extra)
{
    int event_len;                                                       /* Its size                    */
    int hdr_len;                                                         /* Size of the event header    */
    int wrqu_off = 0;                                                    /* Offset in wrqu              */
    int tmp_len, extra_len = 0;
    unsigned  cmd_index;                                                 /* *MUST* be unsigned          */
    struct iw_event  *event;                                             /* Mallocated whole event      */
    const struct iw_ioctl_description * descr = NULL;
    struct neattr {
        unsigned short    nea_len;
        unsigned short    nea_type;
    } *nea;

    /*
     * Nothing in the kernel sends scan events with data, be safe.
     * This is necessary because we cannot fix up scan event data
     * for compat, due to being contained in 'extra', but normally
     * applications are required to retrieve the scan data anyway
     * and no data is included in the event, this codifies that
     * practice.
     */
    if ((cmd == SIOCGIWSCAN) && extra) {
        extra = NULL;
    }

    /* Get the description of the Event */
    if (cmd <= SIOCIWLAST) {
        cmd_index = IW_IOCTL_IDX(cmd);
        if (cmd_index < standard_ioctl_num) {
            descr = &(standard_ioctl[cmd_index]);
        }
    } else {
        cmd_index = IW_EVENT_IDX(cmd);
        if (cmd_index < standard_event_num) {
            descr = &(standard_event[cmd_index]);
        }
    }
    /* Don't accept unknown events */
    if (descr == NULL) {
        /* Note : we don't return an error to the driver, because
         * the driver would not know what to do about it. It can't
         * return an error to the user, because the event is not
         * initiated by a user request.
         * The best the driver could do is to log an error message.
         * We will do it ourselves instead...
         */
        return;
    }

    /* Check extra parameters and set extra_len */
    if (descr->header_type == IW_HEADER_TYPE_POINT) {
        /* Check if number of token fits within bounds */
        if (wrqu->data.length > descr->max_tokens) {
            return;
        }
        if (wrqu->data.length < descr->min_tokens) {
            return;
        }
        /* Calculate extra_len - extra is NULL for restricted events */
        if (extra != NULL) {
            extra_len = wrqu->data.length * descr->token_size;
        }
        /* Always at an offset in wrqu */
        wrqu_off = IW_EV_POINT_OFF;
    }

    /* Total length of the event */
    hdr_len = event_type_size[descr->header_type];
    event_len = hdr_len + extra_len;
    tmp_len = (int)((sizeof(struct neattr) + 3) & ~3) + event_len;

    /* malloc SylixOS netevent */
    nea = (struct neattr *)sys_zalloc(tmp_len);
    if (!nea) {
        return;
    }

#define IFLA_WIRELESS  (11)                                             /* Wireless Extension event     */
    nea->nea_type = IFLA_WIRELESS;
    nea->nea_len = tmp_len;

    event = (struct iw_event *)((char *)nea +
            (int)((sizeof(struct neattr) + 3) & ~3));

    event->len = event_len;
    event->cmd = cmd;

    lib_memcpy(&event->u, ((char *) wrqu) + wrqu_off, hdr_len - IW_EV_LCP_LEN);
    if (extra_len)
        lib_memcpy(((char *) event) + hdr_len, extra, extra_len);

    netEventIfWlExt2((struct netif *)dev->sys, (void *)nea, (unsigned int)nea->nea_len);

    sys_free(nea);
}
/*********************************************************************************************************
** 函数名称: get_wireless_stats
** 功能描述: 获得无线网络当前状态
** 输　入  : dev       网络接口
** 输　出  : 状态
** 全局变量:
** 调用模块:
*********************************************************************************************************/
struct iw_statistics *get_wireless_stats (struct netdev *dev)
{
#ifdef CONFIG_WIRELESS_EXT
    struct iw_handler_def *handler;
    
    if (!dev->wireless_handlers) {
        return  (NULL);
    }
    
    handler = (struct iw_handler_def *)dev->wireless_handlers;
    if (!handler->get_wireless_stats) {
        return  (NULL);
    }
    
    return  (handler->get_wireless_stats(dev));
#else
    return  (NULL);
#endif                                                                  /* CONFIG_WIRELESS_EXT          */
}
/*********************************************************************************************************
** 函数名称: iw_handler_get_iwstats
** 功能描述: 获得无线状态
** 输　入  : dev       网络接口
**           info      request_info
**           wrqu      req_data
**           extra     extra data
** 输　出  : OK or ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
int iw_handler_get_iwstats (struct netdev            *dev,
                            struct iw_request_info   *info,
                            union iwreq_data         *wrqu,
                            char                     *extra)
{
    struct iw_statistics *stats;                                        /* Get stats from the driver */

    stats = get_wireless_stats(dev);
    if (stats) {
        lib_memcpy(extra, stats, sizeof(struct iw_statistics));         /* Copy statistics to extra */
        wrqu->data.length = sizeof(struct iw_statistics);

        /* Check if we need to clear the updated flag */
        if (wrqu->data.flags != 0) {
            stats->qual.updated &= ~IW_QUAL_ALL_UPDATED;
        }
        return  (ERROR_NONE);
    
    } else {
        return  (-EOPNOTSUPP);
    }
}
/*********************************************************************************************************
** 函数名称: get_handler
** 功能描述: 获得无线网络操作函数集合
** 输　入  : dev       网络接口
** 输　出  : 状态
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static iw_handler get_handler (struct netdev *dev, unsigned int cmd)
{
    unsigned int  index;                                                /* *MUST* be unsigned           */
    const struct iw_handler_def *handlers = NULL;
    
    if (dev->wireless_handlers) {
        handlers = (const struct iw_handler_def *)dev->wireless_handlers;
    }
        
    if (!handlers) {
        return  (NULL);
    }
    
    index = IW_IOCTL_IDX(cmd);                                          /* Try as a standard command    */
    if (index < handlers->num_standard)
        return  (handlers->standard[index]);

#ifdef CONFIG_WEXT_PRIV
    index = cmd - SIOCIWFIRSTPRIV;                                      /* Try as a private command     */
    if (index < handlers->num_private) {
        return  (handlers->private[index]);
    }
#endif
    
    return  (NULL);                                                     /*  Not found                   */
}
/*********************************************************************************************************
** 函数名称: ioctl_standard_iw_point
** 功能描述: 无线网卡标准 ioctl
** 输　入  : ...
** 输　出  : 结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int ioctl_standard_iw_point (struct iw_point *iwp, unsigned int cmd,
                                    const struct iw_ioctl_description *descr,
                                    iw_handler handler, struct netdev *dev,
                                    struct iw_request_info *info)
{
    int err, extra_size, user_length = 0, essid_compat = 0;
    char *extra;

    /* Calculate space needed by arguments. Always allocate
     * for max space.
     */
    extra_size = descr->max_tokens * descr->token_size;

    /* Check need for ESSID compatibility for WE < 21 */
    switch (cmd) {
    
    case SIOCSIWESSID:
    case SIOCGIWESSID:
    case SIOCSIWNICKN:
    case SIOCGIWNICKN:
        if (iwp->length == descr->max_tokens + 1) {
            essid_compat = 1;
        
        } else if (IW_IS_SET(cmd) && (iwp->length != 0)) {
            char essid[IW_ESSID_MAX_SIZE + 1];
            unsigned int len;
            
            len = iwp->length * descr->token_size;

            if (len > IW_ESSID_MAX_SIZE) {
                return  (-EFAULT);
            }

            err = copy_from_user(essid, iwp->pointer, len);
            if (err) {
                return  (-EFAULT);
            }

            if (essid[iwp->length - 1] == '\0') {
                essid_compat = 1;
            }
        }
        break;
        
    default:
        break;
    }

    iwp->length -= essid_compat;

    /* Check what user space is giving us */
    if (IW_IS_SET(cmd)) {
        /* Check NULL pointer */
        if (!iwp->pointer && iwp->length != 0) {
            return  (-EFAULT);
        }
        
        /* Check if number of token fits within bounds */
        if (iwp->length > descr->max_tokens) {
            return  (-E2BIG);
        }
        if (iwp->length < descr->min_tokens) {
            return  (-EINVAL);
        }
    } else {
        /* Check NULL pointer */
        if (!iwp->pointer) {
            return  (-EFAULT);
        }
        /* Save user space buffer size for checking */
        user_length = iwp->length;

        /* Don't check if user_length > max to allow forward
         * compatibility. The test user_length < min is
         * implied by the test at the end.
         */

        /* Support for very large requests */
        if ((descr->flags & IW_DESCR_FLAG_NOMAX) &&
            (user_length > descr->max_tokens)) {
            /* Allow userspace to GET more than max so
             * we can support any size GET requests.
             * There is still a limit : -ENOMEM.
             */
            extra_size = user_length * descr->token_size;

            /* Note : user_length is originally a __u16,
             * and token_size is controlled by us,
             * so extra_size won't get negative and
             * won't overflow...
             */
        }
    }

    /* kzalloc() ensures NULL-termination for essid_compat. */
    extra = kzalloc(extra_size, GFP_KERNEL);
    if (!extra) {
        return  (-ENOMEM);
    }

    /* If it is a SET, get all the extra data in here */
    if (IW_IS_SET(cmd) && (iwp->length != 0)) {
        if (copy_from_user(extra, iwp->pointer,
                           iwp->length *
                           descr->token_size)) {
            err = -EFAULT;
            goto    out;
        }
        if (cmd == SIOCSIWENCODEEXT) {
            struct iw_encode_ext *ee = (void *) extra;

            if (iwp->length < sizeof(*ee) + ee->key_len) {
                err = -EFAULT;
                goto    out;
            }
        }
    }

    if (IW_IS_GET(cmd) && !(descr->flags & IW_DESCR_FLAG_NOMAX)) {
        /*
         * If this is a GET, but not NOMAX, it means that the extra
         * data is not bounded by userspace, but by max_tokens. Thus
         * set the length to max_tokens. This matches the extra data
         * allocation.
         * The driver should fill it with the number of tokens it
         * provided, and it may check iwp->length rather than having
         * knowledge of max_tokens. If the driver doesn't change the
         * iwp->length, this ioctl just copies back max_token tokens
         * filled with zeroes. Hopefully the driver isn't claiming
         * them to be valid data.
         */
        iwp->length = descr->max_tokens;
    }

    err = handler(dev, info, (union iwreq_data *)iwp, extra);

    iwp->length += essid_compat;

    /* If we have something to return to the user */
    if (!err && IW_IS_GET(cmd)) {
        /* Check if there is enough buffer up there */
        if (user_length < iwp->length) {
            err = -E2BIG;
            goto    out;
        }

        if (copy_to_user(iwp->pointer, extra,
                         iwp->length *
                         descr->token_size)) {
            err = -EFAULT;
            goto    out;
        }
    }

    /* Generate an event to notify listeners of the change */
    if ((descr->flags & IW_DESCR_FLAG_EVENT) &&
        ((err == 0) || (err == -EIWCOMMIT))) {
        union iwreq_data *data = (union iwreq_data *)iwp;

        if (descr->flags & IW_DESCR_FLAG_RESTRICT) {
            /* If the event is restricted, don't
             * export the payload.
             */
            wireless_send_event(dev, cmd, data, NULL);
        } else {
            wireless_send_event(dev, cmd, data, extra);
        }
    }

out:
    kfree(extra);
    return  (err);
}
/*********************************************************************************************************
** 函数名称: call_commit_handler
** 功能描述: 无线网卡提交句柄
** 输　入  : dev       网络接口
** 输　出  : 结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
int call_commit_handler (struct netdev *dev)
{
    struct iw_handler_def *handler;
    
    handler = (struct iw_handler_def *)dev->wireless_handlers;
    if (!handler) {
        return  (0);
    }

#ifdef CONFIG_WIRELESS_EXT
    if ((dev->if_flags & IFF_UP) && handler &&
        (handler->standard[0] != NULL)) {
        return  (handler->standard[0](dev, NULL, NULL, NULL));
    } else {
        return  (0);
    }
#else
    return  (0);                                                        /* cfg80211 has no commit       */
#endif
}
/*********************************************************************************************************
** 函数名称: wireless_process_ioctl
** 功能描述: 无线网卡标准 ioctl 过程
** 输　入  : 
** 输　出  : 结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int wireless_process_ioctl (struct ifreq *ifr,
                                   unsigned int cmd,
                                   struct iw_request_info *info,
                                   wext_ioctl_func standard,
                                   wext_ioctl_func private)
{
    struct iwreq  *iwr = (struct iwreq *)ifr;
    struct netdev *dev;
    iw_handler     handler;

    /* Permissions are already checked in dev_ioctl() before calling us.
     * The copy_to/from_user() of ifr is also dealt with in there */

    /* Make sure the device exist */
    if ((dev = netdev_find_by_ifname(ifr->ifr_name)) == NULL) {
        return  (-ENODEV);
    }

    /* A bunch of special cases, then the generic case...
     * Note that 'cmd' is already filtered in dev_ioctl() with
     * (cmd >= SIOCIWFIRST && cmd <= SIOCIWLAST) */
    if (cmd == SIOCGIWSTATS) {
        return  (standard(dev, iwr, cmd, info,
                          &iw_handler_get_iwstats));
    }

#ifdef CONFIG_WEXT_PRIV
    if (cmd == SIOCGIWPRIV && dev->wireless_handlers) {
        return  (standard(dev, iwr, cmd, info,
                          &iw_handler_get_private));
    }
#endif

    /* New driver API : try to find the handler */
    handler = get_handler(dev, cmd);
    if (handler) {
        /* Standard and private are not the same */
        if (cmd < SIOCIWFIRSTPRIV) {
            return  (standard(dev, iwr, cmd, info, handler));
            
        } else if (private) {
            return  (private(dev, iwr, cmd, info, handler));
        }
    }
    
    /* Old driver API : call driver ioctl handler */
    if (dev->drv->ioctl) {
        return  (dev->drv->ioctl(dev, cmd, ifr));
    }
    
    return  (-EOPNOTSUPP);
}
/*********************************************************************************************************
** 函数名称: wext_permission_check
** 功能描述: 无线网卡命令检查
** 输　入  : 
** 输　出  : 结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int wext_permission_check (unsigned int cmd)
{
    return  (0);                                                        /*  SylixOS 允许操作无线网卡    */
}
/*********************************************************************************************************
** 函数名称: wext_ioctl_dispatch
** 功能描述: 无线网卡命令分配
** 输　入  : 
** 输　出  : 结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int wext_ioctl_dispatch (struct ifreq *ifr,
                                unsigned int cmd, struct iw_request_info *info,
                                wext_ioctl_func standard,
                                wext_ioctl_func private)
{
    int ret = wext_permission_check(cmd);

    if (ret) {
        return  (ret);
    }

    return  (wireless_process_ioctl(ifr, cmd, info, standard, private));
}
/*********************************************************************************************************
** 函数名称: ioctl_standard_call
** 功能描述: Wrapper to call a standard Wireless Extension handler.
** 输　入  : 
** 输　出  : 结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int ioctl_standard_call (struct netdev          *dev,
                                struct iwreq           *iwr,
                                unsigned int            cmd,
                                struct iw_request_info *info,
                                iw_handler              handler)
{
    const struct iw_ioctl_description *descr;
    int ret = -EINVAL;

    /* Get the description of the IOCTL */
    if (IW_IOCTL_IDX(cmd) >= standard_ioctl_num) {
        return  (-EOPNOTSUPP);
    }
    
    descr = &(standard_ioctl[IW_IOCTL_IDX(cmd)]);

    /* Check if we have a pointer to user space data or not */
    if (descr->header_type != IW_HEADER_TYPE_POINT) {
        /* No extra arguments. Trivial to handle */
        ret = handler(dev, info, &(iwr->u), NULL);

        /* Generate an event to notify listeners of the change */
        if ((descr->flags & IW_DESCR_FLAG_EVENT) &&
           ((ret == 0) || (ret == -EIWCOMMIT))) {
            wireless_send_event(dev, cmd, &(iwr->u), NULL);
        }
    } else {
        ret = ioctl_standard_iw_point(&iwr->u.data, cmd, descr,
                                      handler, dev, info);
    }

    /* Call commit handler if needed and defined */
    if (ret == -EIWCOMMIT) {
        ret = call_commit_handler(dev);
    }

    /* Here, we will generate the appropriate event if needed */
    return  (ret);
}
/*********************************************************************************************************
** 函数名称: wext_handle_ioctl
** 功能描述: 无线网络 ioctl 入口.
** 输　入  : 
** 输　出  : 结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
int wext_handle_ioctl (unsigned int cmd, struct ifreq *ifr)
{
    struct iw_request_info info;
    int ret;
    
    info.cmd   = cmd;
    info.flags = 0;

    ret = wext_ioctl_dispatch(ifr, cmd, &info,
                              ioctl_standard_call,
                              ioctl_private_call);
    return  (ret);
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_WIRELESS_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
