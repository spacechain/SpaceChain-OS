/**
 * @file
 * log library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#define __SYLIXOS_KERNEL    /* we need include SylixOS logLib.h */
#include <sys/select.h>
#include "vxWorksCommon.h"

/*
 * fd pool
 */
static int  vxWorksLogFd[FD_SETSIZE];

/*
 * init loglib
 */
void  vxWorksLogInit (void)
{
    int  i;

    for (i = 0; i < FD_SETSIZE; i++) {
        vxWorksLogFd[i] = ERROR;
    }
}

/*
 * deinit loglib
 */
void  vxWorksLogDeinit (void)
{
    fd_set  fdset;
    int     width;
    int     i;

    if (logFdGet(&width, &fdset)) {
        return;
    }

    for (i = 0; i < FD_SETSIZE; i++) {
        if (vxWorksLogFd[i] >= 0) {
            FD_CLR(vxWorksLogFd[i], &fdset);
        }
    }

    logFdSet(width, &fdset);

    for (i = 0; i < FD_SETSIZE; i++) {
        if (vxWorksLogFd[i] >= 0) {
            __KERNEL_SPACE_ENTER();
            close(vxWorksLogFd[i]);
            __KERNEL_SPACE_EXIT();
        }
    }
}

/*
 * add fd to loglib
 */
STATUS  logFdAdd (int fd)
{
    fd_set  fdset;
    int     width;
    int     fdkern;

    if ((fd < 0) || (fd >= FD_SETSIZE)) {
        errno = EBADFD;
        return  (ERROR);
    }

    if (vxWorksLogFd[fd] >= 0) {
        errno = EEXIST;
        return  (ERROR);
    }

    if (logFdGet(&width, &fdset)) {
        return  (ERROR);
    }

    fdkern = dup2kernel(fd);
    if (fdkern < 0) {
        return  (ERROR);
    }

    FD_SET(fdkern, &fdset);

    if (fdkern >= width) {
        width   = fdkern + 1;
    }

    logFdSet(width, &fdset);

    vxWorksLogFd[fd] = fdkern;

    return  (OK);
}

/*
 * add fd to loglib
 */
STATUS  logFdDelete (int fd)
{
    fd_set  fdset;
    int     width;
    int     fdkern;

    if ((fd < 0) || (fd >= FD_SETSIZE)) {
        errno = EBADFD;
        return  (ERROR);
    }

    fdkern = vxWorksLogFd[fd];
    if ((fdkern < 0) || (fdkern >= FD_SETSIZE)) {
        errno = EINVAL;
        return  (ERROR);
    }

    if (logFdGet(&width, &fdset)) {
        return  (ERROR);
    }

    FD_CLR(fdkern, &fdset);

    logFdSet(width, &fdset);

    __KERNEL_SPACE_ENTER();
    close(fdkern);
    __KERNEL_SPACE_EXIT();

    vxWorksLogFd[fd] = ERROR;

    return  (OK);
}

/*
 * end
 */
