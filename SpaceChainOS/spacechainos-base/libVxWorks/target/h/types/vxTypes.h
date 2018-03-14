/**
 * @file
 * VxWorks types header.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_TYPES_H
#define __VXWORKS_TYPES_H

#include "vxWorksCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The following types have been added to support the dual-stack */

#ifdef _TYPE_u_int8_t
_TYPE_u_int8_t;
#undef _TYPE_u_int8_t
#endif

#ifdef _TYPE_u_int16_t
_TYPE_u_int16_t;
#undef _TYPE_u_int16_t
#endif

#ifdef _TYPE_u_int32_t
_TYPE_u_int32_t;
#undef _TYPE_u_int32_t
#endif

#ifdef _TYPE_u_int64_t
_TYPE_u_int64_t;
#undef _TYPE_u_int64_t
#endif

#ifdef _TYPE_u_quad_t
_TYPE_u_quad_t;
#undef _TYPE_u_quad_t
#endif

#ifdef _TYPE_quad_t
_TYPE_quad_t;
#undef _TYPE_quad_t
#endif

/* Type for arbitrary user-supplied argument to a user-supplied routine. */
#ifdef _WRS_CONFIG_LP64
typedef long    _Vx_usr_arg_t;
#else
typedef int     _Vx_usr_arg_t;
#endif /* _WRS_CONFIG_LP64 */

typedef unsigned long      _Vx_ticks_t;
typedef unsigned long long _Vx_ticks64_t;

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_TYPES_H */
