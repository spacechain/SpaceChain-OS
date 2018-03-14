/**
 * @file
 * vxworks version.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_VERSION_H
#define __VXWORKS_VERSION_H

#include "vxWorksCommon.h"

/*
 * Release identification, major, minor, and maintenance numbers.
 *
 * Current VxWorks release number is 6.9.0
 */

#define _WRS_VXWORKS_MAJOR 6
#define _WRS_VXWORKS_MINOR 9
#define _WRS_VXWORKS_MAINT 0

/* For printLogo(), the max combined length is 44 characters */

#define RUNTIME_NAME     "VxWorks"
#define RUNTIME_SMP_NAME "SMP"
#define RUNTIME_VERSION  "6.9"

/* libVxWorks version */

#define _LIB_VXWORKS_MAJOR 0
#define _LIB_VXWORKS_MINOR 2
#define _LIB_VXWORKS_MAINT 6

/* there is now a space between the runtime name, and the version */

#define VXWORKS_VERSION     RUNTIME_NAME " " RUNTIME_VERSION
#define VXWORKS_SMP_VERSION RUNTIME_NAME " " RUNTIME_VERSION " " RUNTIME_SMP_NAME
#define RUNTIME_SMP_VERSION RUNTIME_VERSION " " RUNTIME_SMP_NAME

/* VxWorks compatibility layer */
/* 0.1.1  add cpu affinity */
#define LIB_VXWORKS_VERSION "SylixOS Compatibility Pack 0.2.6"

#ifdef __cplusplus
extern "C" {
#endif

extern char *creationDate;
extern char *runtimeVersion;
extern char *runtimeName;
extern char *vxWorksVersion;

extern const unsigned int vxWorksVersionMajor;
extern const unsigned int vxWorksVersionMinor;
extern const unsigned int vxWorksVersionMaint;

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_VERSION_H */
