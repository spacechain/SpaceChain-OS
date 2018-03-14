/**
 * @file
 * task default defination.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_TASKDEFS_H
#define __VXWORKS_TASKDEFS_H


/* defines */

#define VX_USER_MODE            0x0000   /* INTERNAL: tasks in user mode */
#define VX_SUPERVISOR_MODE      0x0001   /* INTERNAL: tasks in sup mode */
#define VX_DEALLOC_STACK        0x0004   /* INTERNAL: deallocate stack */
#define VX_TASK_OBJ_OWNER       0x0800   /* INTERNAL: task owns objs directly */
#define VX_DEALLOC_EXC_STACK    0x1000   /* INTERNAL: deallocate the exc stack */
#define VX_DEALLOC_TCB          0x8000   /* INTERNAL: deallocate the task TCB */

#endif /* __VXWORKS_TASKDEFS_H */
