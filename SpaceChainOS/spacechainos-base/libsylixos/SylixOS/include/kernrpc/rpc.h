/*
 * rpc.h, Just includes the billions of rpc header files necessary to
 * do remote procedure calling.
 *
 * Copyright (c) 2010, Oracle America, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *     * Neither the name of the "Oracle America, Inc." nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 *   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _RPC_RPC_H
#define _RPC_RPC_H 1

#include <kernrpc/types.h>		/* some typedefs */
#include <netinet/in.h>

/* external data representation interfaces */
#include <kernrpc/xdr.h>		/* generic (de)serializer */

/* Client side only authentication */
#include <kernrpc/auth.h>		/* generic authenticator (client side) */

/* Client side (mostly) remote procedure call */
#include <kernrpc/clnt.h>		/* generic rpc stuff */

/* semi-private protocol headers */
#include <kernrpc/rpc_msg.h>	/* protocol for rpc messages */
#include <kernrpc/auth_unix.h>	/* protocol for unix style cred */
#include <kernrpc/auth_des.h>	/* protocol for des style cred */

/* Server side only remote procedure callee */
#include <kernrpc/svc.h>		/* service manager and multiplexer */
#include <kernrpc/svc_auth.h>	/* service side authenticator */

/*
 * COMMENT OUT THE NEXT INCLUDE IF RUNNING ON SUN OS OR ON A VERSION
 * OF UNIX BASED ON NFSSRC.  These systems will already have the structures
 * defined by <kernrpc/netdb.h> included in <netdb.h>.
 */
/* routines for parsing /etc/rpc */
#include <kernrpc/netdb.h>		/* structures and routines to parse /etc/rpc */

__BEGIN_DECLS

#ifdef _RPC_THREAD_SAFE_
/* Global variables, protected for multi-threaded applications.  */
extern fd_set *__rpc_thread_svc_fdset (void) __attribute__ ((__const__));
#define svc_fdset (*__rpc_thread_svc_fdset ())

extern struct rpc_createerr *__rpc_thread_createerr (void)
     __attribute__ ((__const__));
#define get_rpc_createerr() (*__rpc_thread_createerr ())
/* The people who "engineered" RPC should bee punished for naming the
   data structure and the variable the same.  We cannot always define the
   macro 'rpc_createerr' because this would prevent people from defining
   object of type 'struct rpc_createerr'.  So we leave it up to the user
   to select transparent replacement also of this variable.  */
#ifdef _RPC_MT_VARS
# define rpc_createerr (*__rpc_thread_createerr ())
#endif

extern struct pollfd **__rpc_thread_svc_pollfd (void)
     __attribute__ ((__const__));
#define svc_pollfd (*__rpc_thread_svc_pollfd ())

extern int *__rpc_thread_svc_max_pollfd (void) __attribute__ ((__const__));
#define svc_max_pollfd (*__rpc_thread_svc_max_pollfd ())
#else
extern fd_set svc_fdset;
extern struct rpc_createerr rpc_createerr;
#define get_rpc_createerr() rpc_createerr
extern struct pollfd *svc_pollfd;
extern int svc_max_pollfd;
#endif

__END_DECLS

#endif /* rpc/rpc.h */
