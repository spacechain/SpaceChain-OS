/**
 * @file
 * SylixOS multi-input device support kernel module.
 */

/*
 * Copyright (c) 2006-2013 SylixOS Group.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 4. This code has been or is applying for intellectual property protection 
 *    and can only be used with acoinfo software products.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Author: Han.hui <sylixos@gmail.com>
 *
 */

#define  __SYLIXOS_KERNEL
#include <module.h>
#include <input_device.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <keyboard.h>
#include <mouse.h>
#include <sys/select.h>

#include "xdev.h"

/*
 * procfs add file
 */
extern void xinput_proc_init(void);
extern void xinput_proc_deinit(void);

/*
 * device hdr
 */
typedef struct {
    LW_DEV_HDR devhdr;
    LW_LIST_LINE_HEADER fdnode_header;
    LW_HANDLE queue;
    LW_SEL_WAKEUPLIST sel_list;
    time_t time;
} xinput_dev_t;

static xinput_dev_t kdb_xinput;
static xinput_dev_t mse_xinput;

/*
 * xinput device driver number
 */
static int xinput_drv_num = -1;

/*
 * xinput device hotplug event
 */
static LW_HANDLE xinput_thread;
static BOOL xinput_hotplug_fd = -1;
static BOOL xinput_hotplug = FALSE;
static spinlock_t xinput_sl;

/*
 * xinput device msgq size
 */
static int xinput_kmqsize = MAX_INPUT_KQUEUE;
static int xinput_mmqsize = MAX_INPUT_MQUEUE;

/*
 * xinput open
 */
static long xinput_open (xinput_dev_t *xinput, char *name, int flags, int mode)
{
    PLW_FD_NODE     pfdnode;
    BOOL            is_new;

    if (name == NULL) {
        errno = ERROR_IO_NO_DEVICE_NAME_IN_PATH;
        return  (PX_ERROR);

    } else {
        pfdnode = API_IosFdNodeAdd(&xinput->fdnode_header,
                                   (dev_t)xinput, 0,
                                   flags, mode, 0, 0, 0, NULL, &is_new);
        if (pfdnode == NULL) {
            return  (PX_ERROR);
        }

        LW_DEV_INC_USE_COUNT(&xinput->devhdr);

        return  ((long)pfdnode);
    }
}

/*
 * xinput close
 */
static int xinput_close (PLW_FD_ENTRY pfdentry)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    xinput_dev_t *xinput  = (xinput_dev_t *)pfdentry->FDENTRY_pdevhdrHdr;

    if (pfdentry && pfdnode) {
        API_IosFdNodeDec(&xinput->fdnode_header, pfdnode, NULL);

        LW_DEV_DEC_USE_COUNT(&xinput->devhdr);
        if (LW_DEV_GET_USE_COUNT(&xinput->devhdr) == 0) {
            API_MsgQueueClear(xinput->queue);
        }

        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}

/*
 * xinput read
 */
static ssize_t xinput_read (PLW_FD_ENTRY pfdentry, char *buffer, size_t maxbytes)
{
    u_long  timeout;
    size_t  msglen = 0;
    xinput_dev_t *xinput = (xinput_dev_t *)pfdentry->FDENTRY_pdevhdrHdr;

    if (!pfdentry || !buffer) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    if (!maxbytes) {
        return  (0);
    }

    if (pfdentry->FDENTRY_iFlag & O_NONBLOCK) {
        timeout = 0;
    } else {
        timeout = LW_OPTION_WAIT_INFINITE;
    }

    if (API_MsgQueueReceive(xinput->queue, buffer, maxbytes, &msglen, timeout)) {
        return  (PX_ERROR);
    }

    return  ((ssize_t)msglen);
}

/*
 * xinput lstat
 */
static int xinput_lstat (xinput_dev_t *xinput, char *name, struct stat *pstat)
{
    if (pstat) {
        pstat->st_dev     = (dev_t)xinput;
        pstat->st_ino     = (ino_t)0;
        pstat->st_mode    = (S_IFCHR | S_IRUSR | S_IRGRP | S_IROTH);
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;
        pstat->st_gid     = 0;
        pstat->st_rdev    = 0;
        pstat->st_size    = 0;
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        pstat->st_atime   = xinput->time;
        pstat->st_mtime   = xinput->time;
        pstat->st_ctime   = xinput->time;
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}

/*
 * xinput ioctl
 */
static int xinput_ioctl (PLW_FD_ENTRY pfdentry, int cmd, long arg)
{
    struct stat *pstat;
    PLW_SEL_WAKEUPNODE  pselnode;
    u_long  count = 0;
    size_t  len = 0;
    xinput_dev_t *xinput = (xinput_dev_t *)pfdentry->FDENTRY_pdevhdrHdr;

    switch (cmd) {

    case FIONBIO:
        if (*(int *)arg) {
            pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);

    case FIONREAD:
        API_MsgQueueStatus(xinput->queue, NULL, &count, &len, NULL, NULL);
        if (count && arg) {
            *(int *)arg = (int)len;
        }
        return  (ERROR_NONE);

    case FIONREAD64:
        API_MsgQueueStatus(xinput->queue, NULL, &count, &len, NULL, NULL);
        if (count && arg) {
            *(off_t *)arg = (off_t)len;
        }
        return  (ERROR_NONE);

    case FIOFLUSH:
        API_MsgQueueClear(xinput->queue);
        return  (ERROR_NONE);

    case FIOFSTATGET:
        pstat = (struct stat *)arg;
        if (pstat) {
            pstat->st_dev     = (dev_t)xinput;
            pstat->st_ino     = (ino_t)0;
            pstat->st_mode    = (S_IRUSR | S_IRGRP | S_IROTH);
            pstat->st_nlink   = 1;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 0;
            pstat->st_size    = 0;
            pstat->st_blksize = 0;
            pstat->st_blocks  = 0;
            pstat->st_atime   = xinput->time;
            pstat->st_mtime   = xinput->time;
            pstat->st_ctime   = xinput->time;
            return  (ERROR_NONE);
        }
        return  (PX_ERROR);

    case FIOSETFL:
        if ((int)arg & O_NONBLOCK) {
            pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);

    case FIOSELECT:
        pselnode = (PLW_SEL_WAKEUPNODE)arg;
        SEL_WAKE_NODE_ADD(&xinput->sel_list, pselnode);
        switch (pselnode->SELWUN_seltypType) {

        case SELREAD:
            if (API_MsgQueueStatus(xinput->queue, NULL, &count, NULL, NULL, NULL)) {
                SEL_WAKE_UP(pselnode);
            } else if (count) {
                SEL_WAKE_UP(pselnode);
            }
            break;

        case SELWRITE:
        case SELEXCEPT:
            break;
        }
        return  (ERROR_NONE);

    case FIOUNSELECT:
        SEL_WAKE_NODE_DELETE(&xinput->sel_list, (PLW_SEL_WAKEUPNODE)arg);
        return  (ERROR_NONE);

    default:
        errno = ENOSYS;
        return  (PX_ERROR);
    }
}

/*
 * xinput device install driver
 */
static int xinput_drv (void)
{
    struct file_operations     fileop;

    if (xinput_drv_num >= 0) {
        return  (ERROR_NONE);
    }

    memset(&fileop, 0, sizeof(struct file_operations));

    fileop.owner     = THIS_MODULE;
    fileop.fo_create = xinput_open;
    fileop.fo_open   = xinput_open;
    fileop.fo_close  = xinput_close;
    fileop.fo_read   = xinput_read;
    fileop.fo_lstat  = xinput_lstat;
    fileop.fo_ioctl  = xinput_ioctl;

    xinput_drv_num = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);

    DRIVER_LICENSE(xinput_drv_num,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(xinput_drv_num,      "Han.hui");
    DRIVER_DESCRIPTION(xinput_drv_num, "xinput driver.");

    return  ((xinput_drv_num > 0) ? (ERROR_NONE) : (PX_ERROR));
}

/*
 * xinput device install
 */
static int xinput_devadd (void)
{
    kdb_xinput.queue = API_MsgQueueCreate("xkbd_q", (ULONG)xinput_kmqsize,
                                          sizeof(struct keyboard_event_notify),
                                          LW_OPTION_OBJECT_GLOBAL, NULL);
    SEL_WAKE_UP_LIST_INIT(&kdb_xinput.sel_list);
    kdb_xinput.time = time(NULL);

    if (iosDevAddEx(&kdb_xinput.devhdr, XINPUT_NAME_KBD, xinput_drv_num, DT_CHR) != ERROR_NONE) {
        printk(KERN_ERR "xinput can not add device: %s.\n", strerror(errno));
        return  (PX_ERROR);
    }

    mse_xinput.queue = API_MsgQueueCreate("xmse_q", (ULONG)xinput_mmqsize,
                                          (sizeof(struct mouse_event_notify) * MAX_INPUT_POINTS),
                                          LW_OPTION_OBJECT_GLOBAL, NULL);
    SEL_WAKE_UP_LIST_INIT(&mse_xinput.sel_list);
    mse_xinput.time = time(NULL);

    if (iosDevAddEx(&mse_xinput.devhdr, XINPUT_NAME_MSE, xinput_drv_num, DT_CHR) != ERROR_NONE) {
        printk(KERN_ERR "xinput can not add device: %s.\n", strerror(errno));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

/*
 * xinput hotplug callback
 */
static void  xinput_hotplug_cb (unsigned char *phpmsg, size_t size)
{
    INTREG  int_lock;
    int     msg_type;

    (void)size;

    msg_type = (int)((phpmsg[0] << 24) + (phpmsg[1] << 16) + (phpmsg[2] << 8) + phpmsg[3]);
    if ((msg_type == LW_HOTPLUG_MSG_USB_KEYBOARD) ||
        (msg_type == LW_HOTPLUG_MSG_USB_MOUSE)    ||
        (msg_type == LW_HOTPLUG_MSG_PCI_INPUT)) {
        if (phpmsg[4]) {
            LW_SPIN_LOCK_QUICK(&xinput_sl, &int_lock);
            xinput_hotplug = TRUE; /* need open files */
            LW_SPIN_UNLOCK_QUICK(&xinput_sl, int_lock);
        }
    }
}

/*
 * xinput_scan thread
 */
static void  *xinput_scan (void *arg)
{
    INTREG                 int_lock;
    int                    i;
    fd_set                 fdset;
    int                    width;
    int                    ret;

    keyboard_event_notify  knotify;
    mouse_event_notify     mnotify[MAX_INPUT_POINTS];

    BOOL                   need_check;

    (void)arg;

    xinput_hotplug_fd = open("/dev/hotplug", O_RDONLY);

    for (;;) {
        FD_ZERO(&fdset);

        LW_SPIN_LOCK_QUICK(&xinput_sl, &int_lock);
        need_check     = xinput_hotplug;
        xinput_hotplug = FALSE;
        LW_SPIN_UNLOCK_QUICK(&xinput_sl, int_lock);

        if (need_check) { /* need check if a device has been plug on */
            xdev_try_open();
        }

        width = xdev_set_fdset(&fdset);
        if (xinput_hotplug_fd >= 0) {
            FD_SET(xinput_hotplug_fd, &fdset);
            width = (width > xinput_hotplug_fd) ? width : xinput_hotplug_fd;
        }
        width += 1;

        ret = select(width, &fdset, LW_NULL, LW_NULL, LW_NULL); /* no timeout needed */
        if (ret < 0) {
            xdev_close_all();

            LW_SPIN_LOCK_QUICK(&xinput_sl, &int_lock);
            xinput_hotplug = TRUE; /* need reopen files */
            LW_SPIN_UNLOCK_QUICK(&xinput_sl, int_lock);

            sleep(XINPUT_SEL_TO);
            continue;

        } else if (ret == 0) {
            continue; /* EINTR */

        } else {
            ssize_t temp;
            input_dev_t *input;

            if (xinput_hotplug_fd >= 0 && FD_ISSET(xinput_hotplug_fd, &fdset)) {
                unsigned char hpmsg[LW_HOTPLUG_DEV_MAX_MSGSIZE];
                temp = read(xinput_hotplug_fd, hpmsg, LW_HOTPLUG_DEV_MAX_MSGSIZE);
                if (temp > 0) {
                    xinput_hotplug_cb(hpmsg, (size_t)temp);
                }
            }

            input = xdev_kbd_array;
            for (i = 0; i < xdev_kbd_num; i++, input++) {
                if (input->fd >= 0) {
                    if (FD_ISSET(input->fd, &fdset)) {
                        temp = read(input->fd, (PVOID)&knotify, sizeof(knotify));
                        if (temp <= 0) {
                            close(input->fd);
                            input->fd = -1;
                        } else {    /* must send kbd message ok     */
                            if (LW_DEV_GET_USE_COUNT(&kdb_xinput.devhdr)) {
                                API_MsgQueueSend2(kdb_xinput.queue, (void *)&knotify,
                                                 (u_long)temp, LW_OPTION_WAIT_INFINITE);
                                SEL_WAKE_UP_ALL(&kdb_xinput.sel_list, SELREAD);
                            }
                        }
                    }
                }
            }

            input = xdev_mse_array;
            for (i = 0; i < xdev_mse_num; i++, input++) {
                if (input->fd >= 0) {
                    if (FD_ISSET(input->fd, &fdset)) {
                        temp = read(input->fd, (PVOID)mnotify,
                                sizeof(mouse_event_notify) * MAX_INPUT_POINTS);
                        if (temp <= 0) {
                            close(input->fd);
                            input->fd = -1;
                        } else {
                            if (LW_DEV_GET_USE_COUNT(&mse_xinput.devhdr)) {
                                if (!(mnotify[0].kstat & MOUSE_LEFT)) {/* release operate must send suc */
                                    API_MsgQueueSend2(mse_xinput.queue, (void *)mnotify,
                                                      (u_long)temp, LW_OPTION_WAIT_INFINITE);
                                } else {
                                    API_MsgQueueSend(mse_xinput.queue, (void *)mnotify, (u_long)temp);
                                }
                                SEL_WAKE_UP_ALL(&mse_xinput.sel_list, SELREAD);
                            }
                        }
                    }
                }
            }
        }
    }

    return  (NULL);
}

/*
 * module_init
 */
int module_init (void)
{
    LW_CLASS_THREADATTR threadattr;
    char    temp_str[128];
    int     prio;

    xdev_init();

    xinput_proc_init();

    if (xinput_drv()) {
        return  (PX_ERROR);
    }

    if (xinput_devadd()) {
        return  (PX_ERROR);
    }

    LW_SPIN_INIT(&xinput_sl);
    xinput_hotplug = TRUE; /* need open once first */

    if (getenv_r("XINPUT_KQSIZE", temp_str, sizeof(temp_str)) == 0) {
        xinput_kmqsize = atoi(temp_str);
    }

    if (getenv_r("XINPUT_MQSIZE", temp_str, sizeof(temp_str)) == 0) {
        xinput_mmqsize = atoi(temp_str);
    }

    threadattr = API_ThreadAttrGetDefault();

    if (getenv_r("XINPUT_PRIO", temp_str, sizeof(temp_str)) == 0) {
        prio = atoi(temp_str);
        threadattr.THREADATTR_ucPriority = (uint8_t)prio;
    }

    threadattr.THREADATTR_ulOption |= LW_OPTION_OBJECT_GLOBAL;
    threadattr.THREADATTR_stStackByteSize = XINPUT_THREAD_SIZE;

    xinput_thread = API_ThreadCreate("t_xinput", xinput_scan, &threadattr, NULL);

    return  (ERROR_NONE);
}

void module_exit (void)
{
    xinput_proc_deinit();

#if LW_CFG_SIGNAL_EN > 0
    kill(xinput_thread, SIGTERM);
#else
    API_ThreadDelete(&xinput_thread, LW_NULL);
#endif

    if (xinput_hotplug_fd >= 0) {
        close(xinput_hotplug_fd);
    }

    iosDevFileAbnormal(&kdb_xinput.devhdr);
    iosDevFileAbnormal(&mse_xinput.devhdr);

    iosDevDelete(&kdb_xinput.devhdr);
    iosDevDelete(&mse_xinput.devhdr);

    API_MsgQueueDelete(&kdb_xinput.queue);
    API_MsgQueueDelete(&mse_xinput.queue);

    SEL_WAKE_UP_LIST_TERM(&kdb_xinput.sel_list);
    SEL_WAKE_UP_LIST_TERM(&mse_xinput.sel_list);

    iosDrvRemove(xinput_drv_num, TRUE);
}

/*
 * end
 */
