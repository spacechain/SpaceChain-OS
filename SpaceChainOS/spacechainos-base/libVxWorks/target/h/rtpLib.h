/**
 * @file
 * Real-Time process library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_RTPLIB_H
#define __VXWORKS_RTPLIB_H

#include "rtpLibCommon.h"

/* defines */

#define RTP_DEL_VIA_TASK_DELETE 0x1     /* rtpDelete() via taskDestroy() */
#define RTP_DEL_FORCE           0x2     /* Forceful rtpDelete() DANGER!!! */

#ifdef __cplusplus
extern "C" {
#endif

/* function declarations */

STATUS  rtpDelete(RTP_ID id, int options, int status);
BOOL    rtpShow(char *rtpNameOrId, int level);

/*
 * RTP signals are always present when RTPs are included.  The public RTP
 * signal APIs are declared here.
 */

int     rtpKill(RTP_ID rtpId,  int signo);
int     rtpSigqueue(RTP_ID rtpId,  int signo ,
                    const union sigval value);
int     rtpTaskKill(TASK_ID tid, int signo);
int     rtpTaskSigqueue(TASK_ID tid, int signo,
                        const union sigval value);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_RTPLIB_H */
