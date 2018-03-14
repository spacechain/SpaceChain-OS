/**
 * @file
 * message queue library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_MSGQLIB_H
#define __VXWORKS_MSGQLIB_H

#include "msgQLibCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

/* function declarations */

STATUS      msgQLibInit(void);
void        msgQCreateLibInit(void);
void        msgQShowInit(void);
STATUS      msgQShow(MSG_Q_ID msgQId, int level);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_MSGQLIB_H */
