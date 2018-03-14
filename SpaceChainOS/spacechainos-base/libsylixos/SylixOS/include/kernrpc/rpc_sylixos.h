/**
 * @file
 * rpc SylixOS porting file.
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
 * Author: Jiao.JinXing
 *
 */
 
#ifndef _RPC_SYLIXOS_H
#define _RPC_SYLIXOS_H 1

#include <pthread.h>
#include <stdio.h>

typedef gid_t __gid_t;
typedef uid_t __uid_t;

#define internal_function

#define libc_hidden_nolink_sunrpc(...)
#define libc_sunrpc_symbol(...)
#define libc_hidden_def(a)

#define __libc_once_define(filed, var)              filed pthread_once_t var = PTHREAD_ONCE_INIT
#define __libc_once(var, func)                      pthread_once(&var, func)

#define __libc_lock_define_initialized(filed, var)  filed pthread_mutex_t var = PTHREAD_MUTEX_INITIALIZER;
#define __libc_lock_lock(var)                       pthread_mutex_lock(&var)
#define __libc_lock_unlock(var)                     pthread_mutex_unlock(&var)

#define __glibc_unlikely(x) (x)

#define __FLOAT_WORD_ORDER  __BYTE_ORDER

#ifndef IPPORT_TIMESERVER
#define IPPORT_TIMESERVER   37
#endif

#define _(str)              str
#define N_(str)             str

void __svc_accept_failed (void);

struct sockaddr_in;
int bindresvport (int sd, struct sockaddr_in *sin);
int internal_function __get_socket (struct sockaddr_in *saddr);

struct protoent;
extern int __getprotobyname_r (const char *__name,
                               struct protoent *__result_buf,
                               char *__buf, size_t __buflen,
                               struct protoent **__result);

#define ffsl                        ffs
#define __getdtablesize()           sysconf(_SC_OPEN_MAX)
#define __fxprintf(fp, fmt, ...)    fprintf(((fp) ? (fp) : stderr), fmt, __VA_ARGS__)
#define __asprintf                  asprintf
#define __fcntl                     fcntl
#define __lseek                     lseek
#define __gettimeofday              gettimeofday
#define __nanosleep                 nanosleep
#define __socket                    socket
#define __bind(sock, addr, len)     bind(sock, (struct sockaddr *)addr, len)
#define __getsockname               getsockname
#define __listen                    listen
#define __close                     close
#define __setsockopt                setsockopt
#define __recvmsg                   recvmsg
#define __sendmsg                   sendmsg
#define __sendto                    sendto
#define __recvfrom                  recvfrom
#define __connect                   connect
#define __poll                      poll
#define __read                      read
#define __write                     write
#define close_not_cancel            close
#define __getpid                    getpid
#define __geteuid                   geteuid
#define __getegid                   getegid
#define __getgroups                 getgroups
#define __bzero                     bzero
#define __set_errno(num)            errno = (num)
#define __getpeername               getpeername
#ifndef LW_CFG_CPU_ARCH_C6X
#define __alloca                    alloca
#endif
#define __gethostbyname_r           gethostbyname_r
#define __gethostname               gethostname
#define __strerror_r(num, buf, len) (strerror_r(num, buf, len) == 0 ? (buf) : NULL)

#endif /* _RPC_SYLIXOS_H */
