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
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>

#include "xdev.h"

/*
 * input device array
 */
input_dev_t *xdev_kbd_array; /* keyboard device array */
input_dev_t *xdev_mse_array; /* mouse device array */

unsigned int   xdev_kbd_num = 0;
unsigned int   xdev_mse_num = 0;

/*
 * input device array init
 */
static int xdev_getdev (char *dev, char *dev_list[])
{
    int   i = 0;
    char *divptr = dev;
    char *start;

    do {
        start  = divptr;
        divptr = strchr(start, ':');
        if (divptr) {
            *divptr = '\0';
            divptr++;
        }

        if (i < MAX_INPUT_DEVICE) {
            dev_list[i] = start;
        } else {
            break;
        }

        i++;
    } while (divptr);

    return (i);
}

/*
 * input device make array
 */
static int xdev_makearray (int kbd_num, char *kbd_name_array[],
                           int mse_num, char *mse_name_array[])
{
    int i;

    xdev_kbd_array = (input_dev_t *)sys_malloc(sizeof(input_dev_t) * kbd_num);
    if (xdev_kbd_array == NULL) {
        printk(KERN_ERR "xdev can not alloc memory.\n");
        return  (PX_ERROR);
    }

    for (i = 0; i < kbd_num; i++) {
        xdev_kbd_array[i].fd = PX_ERROR;
        xdev_kbd_array[i].dev = (char *)sys_malloc(strlen(kbd_name_array[i]) + 1);
        if (xdev_kbd_array[i].dev) {
            strcpy(xdev_kbd_array[i].dev, kbd_name_array[i]);
        }
        xdev_kbd_num++;
    }

    xdev_mse_array = (input_dev_t *)sys_malloc(sizeof(input_dev_t) * mse_num);
    if (xdev_mse_array == NULL) {
        printk(KERN_ERR "xdev can not alloc memory.\n");
        return  (PX_ERROR);
    }

    for (i = 0; i < mse_num; i++) {
        xdev_mse_array[i].fd = PX_ERROR;
        xdev_mse_array[i].dev = (char *)sys_malloc(strlen(mse_name_array[i]) + 1);
        if (xdev_mse_array[i].dev) {
            strcpy(xdev_mse_array[i].dev, mse_name_array[i]);
        }
        xdev_mse_num++;
    }

    return  (ERROR_NONE);
}

/*
 * input device array init
 */
void xdev_init (void)
{
    char    kbd_devs[PATH_MAX + 1];
    char    mse_devs[PATH_MAX + 1];
    char    *kbd_name_array[MAX_INPUT_DEVICE];
    char    *mse_name_array[MAX_INPUT_DEVICE];
    int     kbd_num = 0, mse_num = 0;

    if (getenv_r("KEYBOARD", kbd_devs, PATH_MAX + 1) < 0) {
        printk(KERN_ERR "xdev can not get environ 'KEYBOARD'.\n");
    } else {
        kbd_num = xdev_getdev(kbd_devs, kbd_name_array);
    }

    if (getenv_r("MOUSE", mse_devs, PATH_MAX + 1) < 0) {
        printk(KERN_ERR "xdev can not get environ 'MOUSE'.\n");
    } else {
        mse_num = xdev_getdev(mse_devs, mse_name_array);
    }

    if (kbd_num <= 0) {
        kbd_num = 1;
        kbd_name_array[0] = DEFAULT_KEYBOARD;
    }

    if (mse_num <= 0) {
        mse_num = 1;
        mse_name_array[0] = DEFAULT_MOUSE;
    }

    xdev_makearray(kbd_num, kbd_name_array, mse_num, mse_name_array);
}

/*
 * input device try open array
 */
void xdev_try_open (void)
{
    int i;
    input_dev_t *input;

    input = xdev_kbd_array;
    for (i = 0; i < xdev_kbd_num; i++) {
        if ((input->fd < 0) && (input->dev)) {
            input->fd = open(input->dev, O_RDONLY);
        }
        input++;
    }

    input = xdev_mse_array;
    for (i = 0; i < xdev_mse_num; i++) {
        if ((input->fd < 0) && (input->dev)) {
            input->fd = open(input->dev, O_RDONLY);
        }
        input++;
    }
}

/*
 * input device close all
 */
void xdev_close_all (void)
{
    int i;
    input_dev_t *input;

    input = xdev_kbd_array;
    for (i = 0; i < xdev_kbd_num; i++) {
        if ((input->fd >= 0) && (input->dev)) {
            close(input->fd);
            input->fd = -1;
        }
        input++;
    }

    input = xdev_mse_array;
    for (i = 0; i < xdev_mse_num; i++) {
        if ((input->fd >= 0) && (input->dev)) {
            close(input->fd);
            input->fd = -1;
        }
        input++;
    }
}

/*
 * input device close all
 */
int xdev_set_fdset (fd_set *pfdset)
{
    int i;
    int max = 0;
    input_dev_t *input;

    input = xdev_kbd_array;
    for (i = 0; i < xdev_kbd_num; i++) {
        if ((input->fd >= 0) && (input->dev)) {
            FD_SET(input->fd, pfdset);
            if (max < input->fd) {
                max = input->fd;
            }
        }
        input++;
    }

    input = xdev_mse_array;
    for (i = 0; i < xdev_mse_num; i++) {
        if ((input->fd >= 0) && (input->dev)) {
            FD_SET(input->fd, pfdset);
            if (max < input->fd) {
                max = input->fd;
            }
        }
        input++;
    }

    return  (max);
}
/*
 * end
 */
