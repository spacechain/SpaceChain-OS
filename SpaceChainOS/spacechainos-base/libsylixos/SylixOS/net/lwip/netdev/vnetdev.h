/**
 * @file
 * virtual net device driver.
 * as much as possible compatible with different versions of LwIP
 * Verification using sylixos(tm) real-time operating system
 */

/*
 * Copyright (c) 2006-2017 SylixOS Group.
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
 * Author: Han.hui <hanhui@acoinfo.com>
 *
 */

#ifndef __VNETDEV_H
#define __VNETDEV_H

#include "net/if_vnd.h"
#include "net/if_ether.h"

#if LW_CFG_NET_VNETDEV_EN > 0

#include "lwip/pbuf.h"

#define VNETDEV_MTU_MAX   8192
#define VNETDEV_MTU_MIN   1280
#define VNETDEV_MTU_DEF   ETH_DATA_LEN

struct vnetdev;

/* virtual net device notify */
typedef void (*vndnotify)(struct vnetdev *vnetdev);

/* virtual net device buffer queue node */
struct vnd_q {
  LW_LIST_RING ring;/* ring list */
  struct pbuf_custom p; /* packet */
};

/* virtual net device */
struct vnetdev {
  struct netdev netdev; /* net device */
  int id; /* id number */
  int type; /* network type */
  LW_LIST_RING_HEADER q; /* recv queue */
  size_t cur_size; /* current data size in buffer */
  size_t buf_size; /* buffer size */
  vndnotify notify; /* notify function */
};

/* virtual net device functions */
int vnetdev_add(struct vnetdev *vnetdev, vndnotify notify, size_t bsize, int id, int type, void *priv);
int vnetdev_delete(struct vnetdev *vnetdev);
int vnetdev_put(struct vnetdev *vnetdev, struct pbuf *p);
struct pbuf *vnetdev_get(struct vnetdev *vnetdev);
int vnetdev_linkup(struct vnetdev *vnetdev, int up);
int vnetdev_nread(struct vnetdev *vnetdev);
int vnetdev_nrbytes(struct vnetdev *vnetdev);
int vnetdev_mtu(struct vnetdev *vnetdev);
int vnetdev_maxplen(struct vnetdev *vnetdev);
void vnetdev_flush(struct vnetdev *vnetdev);
int vnetdev_bufsize(struct vnetdev *vnetdev, size_t bsize);

#endif /* LW_CFG_NET_VNETDEV_EN */
#endif /* __VNETDEV_H */
/*
 * end
 */
