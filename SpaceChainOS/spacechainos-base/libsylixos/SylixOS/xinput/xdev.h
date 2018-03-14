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

#ifndef __XDEV_H
#define __XDEV_H

#define MAX_INPUT_DEVICE    5   /* max input physical device */
#define MAX_INPUT_KQUEUE    16  /* max kbd data packets in queue */
#define MAX_INPUT_MQUEUE    4   /* max mse data packets in queue */
#define MAX_INPUT_POINTS    5   /* max input points at once */

#define DEFAULT_KEYBOARD    "/dev/input/keyboard0"
#define DEFAULT_MOUSE       "/dev/input/mouse0"

#define XINPUT_NAME_KBD     "/dev/input/xkbd"
#define XINPUT_NAME_MSE     "/dev/input/xmse"
#define XINPUT_SEL_TO       2
#define XINPUT_BUSY_TO      (2 * LW_OPTION_ONE_TICK)

#define XINPUT_THREAD_SIZE  (12 * 1024)

/*
 * input device
 */
typedef struct {
    int fd;
    char *dev;
} input_dev_t;

/*
 * input device array
 */
extern input_dev_t *xdev_kbd_array; /* keyboard device array */
extern input_dev_t *xdev_mse_array; /* mouse device array */

extern unsigned int   xdev_kbd_num;
extern unsigned int   xdev_mse_num;

void xdev_init(void);
void xdev_try_open(void);
void xdev_close_all(void);
int xdev_set_fdset(fd_set *pfdset);

#endif /* __XDEV_H */
