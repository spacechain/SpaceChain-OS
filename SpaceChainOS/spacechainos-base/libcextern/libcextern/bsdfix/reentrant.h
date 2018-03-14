/*  $NetBSD: reentrant,v 1.0 sylixos fixed $ */

/*-
 * Copyright (c) 1991, 1993, 1994
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  @(#)namespace.h 8.2 (Berkeley) 7/14/94
 */

#ifndef __REENTRANT_H
#define __REENTRANT_H

#include <pthread.h>
#include <sys/queue.h>
#include <sys/posix_initializer.h>

/*
 * mutex
 */
typedef pthread_mutex_t         mutex_t;

#define MUTEX_INITIALIZER       PTHREAD_MUTEX_INITIALIZER

#define mutex_lock(pmutex)      pthread_mutex_lock(pmutex)
#define mutex_unlock(pmutex)    pthread_mutex_unlock(pmutex)

/*
 * read/write lock
 */
typedef pthread_rwlock_t        rwlock_t;

#define RWLOCK_INITIALIZER      PTHREAD_RWLOCK_INITIALIZER

#define rwlock_rdlock(rwlock)   pthread_rwlock_rdlock(rwlock)
#define rwlock_wrlock(rwlock)   pthread_rwlock_wrlock(rwlock)
#define rwlock_unlock(rwlock)   pthread_rwlock_unlock(rwlock)

/*
 * libc thread
 */
#define mutex_t                 pthread_mutex_t
#define MUTEX_INITIALIZER       PTHREAD_MUTEX_INITIALIZER
#define mutexattr_t             pthread_mutexattr_t

#define MUTEX_TYPE_NORMAL       PTHREAD_MUTEX_NORMAL
#define MUTEX_TYPE_ERRORCHECK   PTHREAD_MUTEX_ERRORCHECK
#define MUTEX_TYPE_RECURSIVE    PTHREAD_MUTEX_RECURSIVE

#define cond_t                  pthread_cond_t
#define COND_INITIALIZER        PTHREAD_COND_INITIALIZER

#define condattr_t              pthread_condattr_t
#define rwlock_t                pthread_rwlock_t
#define RWLOCK_INITIALIZER      PTHREAD_RWLOCK_INITIALIZER

#define rwlockattr_t            pthread_rwlockattr_t
#define thread_key_t            pthread_key_t
#define thr_t                   pthread_t
#define thrattr_t               pthread_attr_t

#define once_t                  pthread_once_t
#define ONCE_INITIALIZER        PTHREAD_ONCE_INIT

#define __isthreaded            1 /* multi-thread */
#define thr_self()              pthread_self()

#endif /* __REENTRANT_H */

