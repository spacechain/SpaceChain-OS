/**
 * @file
 * sylixos add popen pclose.
 */

/*
 * Copyright (c) 2006-2012 SylixOS Group.
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

#define __SYLIXOS_KERNEL
#include "stdio.h"
#include "unistd.h"

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0) && (LW_CFG_SHELL_EN > 0)

/*
 *  get standard file
 */
static int get_std_file (int std)
{
    if (getpid() != 0) {
        return  (std);
    } else {
        return  (API_IoTaskStdGet(API_ThreadIdSelf(), std));
    }
}
 
/*
 * popen
 */
FILE *popen (const char *cmd, const char *type)
{
    int pipefd[2];
    
    int child_std[3];
    BOOL child_closed[3];
    
    LW_OBJECT_HANDLE childsh = 0;
    
    FILE *fp;
    int err;
    
    /* only allow 'r' or 'w' */
    if ((type[0] != 'r' && type[0] != 'w') || type[1] != 0) {
        errno = EINVAL;
        return  (NULL);
    }
    
    if (pipe(pipefd) < 0) {
        return  (NULL);
    }
    
    if (*type == 'r') {
        fp = fdopen(pipefd[0], type);
        if (fp == NULL) {
            return  (NULL);
        }
        
        child_std[0] = get_std_file(STD_IN);
        child_std[1] = pipefd[1];
        child_std[2] = pipefd[1];
        
        child_closed[0] = LW_FALSE;
        child_closed[1] = LW_TRUE; /* sh exit close std_out */
        child_closed[2] = LW_FALSE;
    
    } else {
        fp = fdopen(pipefd[1], type);
        if (fp == NULL) {
            return  (NULL);
        }
        
        child_std[0] = pipefd[0];
        child_std[1] = get_std_file(STD_OUT);
        child_std[2] = get_std_file(STD_ERR);
        
        child_closed[0] = LW_TRUE; /* sh exit close std_in */
        child_closed[1] = LW_FALSE;
        child_closed[2] = LW_FALSE;
    }
    
    err = API_TShellExecBg(cmd, child_std, child_closed, LW_FALSE, &childsh); /* create shell to run command */
    if (err) {
        /* here an error occur, child_closed'fd has been closed, so we only need to fclose(fp) */
        fclose(fp);
        return  (NULL);
    }
    
    _stdfile_pvsh(fp) = (PVOID)childsh;
    
    return  (fp);
}

/*
 * pclose
 */
int pclose (FILE *fp)
{
    LW_OBJECT_HANDLE childsh;
    int fd = fileno(fp);
    int status = -1;
    
    if (!fp || (fd < 0)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    childsh = (LW_OBJECT_HANDLE)_stdfile_pvsh(fp);
    if (!childsh) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    fclose(fp);
    
    /*
     *  注意, 这里没有使用 waitpid 因为无法确定 popen 是否执行的是进程, 
     *  如果执行的是进程, 如果是进程, 则必须等到当前进程结束时, 子进程资源才能被回收器回收.
     *  这是一个折中方案, 目前没有好的解决方法.
     */
    API_ThreadJoin(childsh, (void **)&status);
    
    return  (status);
}

#endif  /* (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0) && (LW_CFG_SHELL_EN > 0) */
