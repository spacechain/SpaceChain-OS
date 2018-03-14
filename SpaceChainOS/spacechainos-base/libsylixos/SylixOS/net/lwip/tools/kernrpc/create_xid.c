/* Copyright (c) 1998-2016 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Thorsten Kukuk <kukuk@vt.uni-paderborn.de>, 1998.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#ifndef SYLIXOS
#include <libc-lock.h>
#endif
#include <kernrpc/rpc.h>

#if LW_CFG_NET_RPC_EN > 0

/* The RPC code is not threadsafe, but new code should be threadsafe. */

__libc_lock_define_initialized (static, createxid_lock)

static pid_t is_initialized;
#ifndef SYLIXOS
static struct drand48_data __rpc_lrand48_data;
#endif

static int getktid (void)
{
  return ((int)API_ThreadIdSelf() & 0xffff);
}

unsigned long
_create_xid (void)
{
  long int res;

  __libc_lock_lock (createxid_lock);

  pid_t pid = (pid_t)getktid ();
  if (is_initialized != pid)
    {
      struct timeval now;

      __gettimeofday (&now, (struct timezone *) 0);
#ifndef SYLIXOS
      __srand48_r (now.tv_sec ^ now.tv_usec ^ pid,
		   &__rpc_lrand48_data);
#else
      srand48(now.tv_sec ^ now.tv_usec ^ pid);
#endif
      is_initialized = pid;
    }

#ifndef SYLIXOS
  lrand48_r (&__rpc_lrand48_data, &res);
#else
  res = lib_lrand48();
#endif

  __libc_lock_unlock (createxid_lock);

  return res;
}
#endif /* LW_CFG_NET_RPC_EN > 0 */
