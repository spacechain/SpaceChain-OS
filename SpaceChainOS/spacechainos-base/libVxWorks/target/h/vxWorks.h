/**
 * @file
 * VxWorks header.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_VXWORKS_H
#define __VXWORKS_VXWORKS_H

#include "vxWorksCommon.h"

/* BUS types */

#define BUS_TYPE_NONE       NONE
#define BUS_TYPE_VME        1
#define BUS_TYPE_MULTIBUS   2
#define BUS_TYPE_PCI        3

#define VME_BUS             BUS_TYPE_VME        /* for backward compat. */
#define MULTI_BUS           BUS_TYPE_MULTIBUS

#if !defined _WRS_CHANGE_INSN_SET_ENABLE
/* these macros can be used to switch target calls into an
 * alternate instruction set and back, e.g., ARM/Thumb, or MIPS16.
 * They are null functions by default.
*/
#define _WRS_CHANGE_INSN_SET(p) {}
#define _WRS_CHANGE_INSN_SET_PTR(p) (p)
#define _WRS_REV_INSN_SET_CHANGE(p) (p)
#endif /* _WRS_CHANGE_INSN_SET */

/* The _WRS_KERNEL macro must be defined for kernel-side builds only */

#ifndef _WRS_KERNEL
#define _WRS_KERNEL
#endif /* !_WRS_KERNEL */

#endif /* __VXWORKS_VXWORKS_H */
