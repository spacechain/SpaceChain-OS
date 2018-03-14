/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * portmap.c, Implements the program,version to port number mapping for rpc.
 */

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

static SVCXPRT  *pmapXprt;
struct pmaplist *pmapList;
static int debugging = 0;

static void reg_service(struct svc_req *, SVCXPRT *);
static void reg_service_internal(struct svc_req *, SVCXPRT *);
static void callit(struct svc_req *, SVCXPRT *);

int main (int argc, char **argv)
{
    SVCXPRT *xprt = NULL;
    int udp_sockfd;
    int tcp_sockfd;
    struct sockaddr_in addr;
    socklen_t len = (socklen_t)sizeof(struct sockaddr_in);

    memset(&addr, 0, sizeof(addr));

    addr.sin_addr.s_addr = 0;
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((u_short)PMAPPORT);

    if ((udp_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("portmap cannot create socket");
        abort();

    } else {
        if (bind(udp_sockfd, (struct sockaddr *)&addr, len) != 0) {
            perror("portmap cannot bind");
            abort();
        }

        xprt = svcudp_create(udp_sockfd);
        if (xprt == NULL) {
            fprintf(stderr, "couldn't do svcudp_create\n");
            abort();
        }
    }

    if ((tcp_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("portmap cannot create socket");
        abort();

    } else {
        if (bind(tcp_sockfd, (struct sockaddr *)&addr, len) != 0) {
            perror("portmap cannot bind");
            abort();
        }

        if ((xprt = svctcp_create(tcp_sockfd, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE)) == (SVCXPRT *)NULL) {
            fprintf(stderr, "couldn't do svctcp_create\n");
            abort();
        }
    }

    (void)svc_register(xprt, PMAPPROG, PMAPVERS, reg_service, (rpcproc_t)0);
    pmapXprt = xprt;
    svc_run();
    fprintf(stderr, "svc_run returned unexpectedly\n");
    abort();
}

static struct pmaplist *
find_service(prog, vers, prot)
    u_long prog;
    u_long vers;
    u_long prot;
{
    register struct pmaplist *hit = NULL;
    register struct pmaplist *pml;

    for (pml = pmapList; pml != NULL; pml = pml->pml_next) {
        if ((pml->pml_map.pm_prog != prog) ||
            (pml->pml_map.pm_prot != prot))
            continue;
        hit = pml;
        if (pml->pml_map.pm_vers == vers)
            break;
    }
    return (hit);
}

/*
 * Wrapper function to register the portmapper service
 * with the portmapper. Actual function "reg_service_internal
 * will be called at the end of this function.
 */
static void reg_service (rqstp, xprt)
    struct svc_req *rqstp;
    SVCXPRT *xprt;
{
    struct pmaplist *pml, *fnd;
    static BOOL bPmapFirstTime = TRUE;

    if (bPmapFirstTime) {
        bPmapFirstTime = FALSE;

        /*
         * While registering the PORT MAPPER DAEMON
         * it cannot set it's own entry. This can be a problem
         * when somebody tries to query a PMAP service info.
         * The service is registered but the entries are missing in
         * the pmap list. Hence this workaround when the PMAP request
         * comes for the first time.
         * Here we will register a service on UDP port
         */
        pml = (struct pmaplist *)malloc(sizeof(struct pmaplist));
        if (pml == NULL)
            return;

        pml->pml_map.pm_prog = PMAPPROG;
        pml->pml_map.pm_vers = PMAPVERS;
        pml->pml_map.pm_prot = IPPROTO_UDP;
        pml->pml_map.pm_port = pmapXprt->xp_port;
        pml->pml_next        = 0;

        if (pmapList == 0) {
            pmapList = pml;
        } else {
            for (fnd = pmapList; fnd->pml_next != 0; fnd = fnd->pml_next)
                ;
            fnd->pml_next = pml;
        }
        /*
         * Now register the service for TCP port
         */
        pml = (struct pmaplist *)malloc(sizeof(struct pmaplist));
        pml->pml_map.pm_prog = PMAPPROG;
        pml->pml_map.pm_vers = PMAPVERS;
        pml->pml_map.pm_prot = IPPROTO_TCP;
        pml->pml_map.pm_port = pmapXprt->xp_port;
        pml->pml_next        = 0;

        if (pmapList == 0) {
            pmapList = pml;
        } else {
            for (fnd = pmapList; fnd->pml_next != 0; fnd = fnd->pml_next)
                ;
            fnd->pml_next = pml;
        }
    }
    reg_service_internal(rqstp, xprt);
}

static void reg_service_internal(rqstp, xprt)
    struct svc_req *rqstp;
    SVCXPRT *xprt;
{
    struct pmap reg;
    struct pmaplist *pml, *prevpml, *fnd;
    bool_t ans;
    u_short port;
    caddr_t t;

    switch ((int)rqstp->rq_proc) {

    case PMAPPROC_NULL:
        /*
         * Null proc call
         */
        if ((!svc_sendreply(xprt, (xdrproc_t)xdr_void, (char *)NULL)) && debugging) {
            abort();
        }
        break;

    case PMAPPROC_SET:
        /*
         * Set a program,version to port mapping
         */
        if (!svc_getargs(xprt, (xdrproc_t)xdr_pmap, (caddr_t )&reg))
            svcerr_decode(xprt);
        else {
            /*
             * check to see if already used
             * find_service returns a hit even if
             * the versions don't match, so check for it
             */
            fnd = find_service(reg.pm_prog, reg.pm_vers, reg.pm_prot);
            if (fnd && fnd->pml_map.pm_vers == reg.pm_vers) {
                if (fnd->pml_map.pm_port == reg.pm_port) {
                    ans = 1;
                    goto done;
                } else {
                    ans = 0;
                    goto done;
                }
            } else {
                /*
                 * add to END of list
                 */
                pml = (struct pmaplist *)malloc(sizeof(struct pmaplist));
                pml->pml_map = reg;
                pml->pml_next = 0;
                if (pmapList == 0) {
                    pmapList = pml;
                } else {
                    for (fnd = pmapList; fnd->pml_next != 0; fnd = fnd->pml_next)
                        ;
                    fnd->pml_next = pml;
                }
                ans = 1;
            }
done:
            if ((!svc_sendreply(xprt, (xdrproc_t)xdr_bool, (caddr_t)&ans)) && debugging) {
                fprintf(stderr, "svc_sendreply\n");
                abort();
            }
        }
        break;

    case PMAPPROC_UNSET:
        /*
         * Remove a program,version to port mapping.
         */
        if (!svc_getargs(xprt, (xdrproc_t)xdr_pmap, (caddr_t )&reg))
            svcerr_decode(xprt);
        else {
            ans = 0;
            for (prevpml = NULL, pml = pmapList; pml != NULL;) {
                if ((pml->pml_map.pm_prog != reg.pm_prog) ||
                    (pml->pml_map.pm_vers != reg.pm_vers)) {
                    /*
                     * both pml & prevpml move forwards
                     */
                    prevpml = pml;
                    pml = pml->pml_next;
                    continue;
                }
                /*
                 * found it; pml moves forward, prevpml stays
                 */
                ans = 1;
                t = (caddr_t)pml;
                pml = pml->pml_next;
                if (prevpml == NULL)
                    pmapList = pml;
                else
                    prevpml->pml_next = pml;
                free(t);
            }
            if ((!svc_sendreply(xprt, (xdrproc_t)xdr_bool, (caddr_t)&ans)) && debugging) {
                fprintf(stderr, "svc_sendreply\n");
                abort();
            }
        }
        break;

    case PMAPPROC_GETPORT:
        /*
         * Lookup the mapping for a program,version and return its port
         */
        if (!svc_getargs(xprt, (xdrproc_t)xdr_pmap, (caddr_t )&reg))
            svcerr_decode(xprt);
        else {
            fnd = find_service(reg.pm_prog, reg.pm_vers, reg.pm_prot);
            if (fnd)
                port = fnd->pml_map.pm_port;
            else
                port = 0;
            if ((!svc_sendreply(xprt, (xdrproc_t)xdr_u_short, (caddr_t)&port)) && debugging) {
                fprintf(stderr, "svc_sendreply\n");
                abort();
            }
        }
        break;

    case PMAPPROC_DUMP:
        /*
         * Return the current set of mapped program,version
         */
        if (!svc_getargs(xprt, (xdrproc_t)xdr_void, NULL))
            svcerr_decode(xprt);
        else {
            if ((!svc_sendreply(xprt, (xdrproc_t)xdr_pmaplist, (caddr_t)&pmapList)) && debugging) {
                fprintf(stderr, "svc_sendreply\n");
                abort();
            }
        }
        break;

    case PMAPPROC_CALLIT:
        /*
         * Calls a procedure on the local machine.  If the requested
         * procedure is not registered this procedure does not return
         * error information!!
         * This procedure is only supported on rpc/udp and calls via
         * rpc/udp.  It passes null authentication parameters.
         */
        callit(rqstp, xprt);
        break;

    default:
        svcerr_noproc(xprt);
        break;
    }
}

/*
 * Stuff for the rmtcall service
 */
#define ARGSIZE 9000

typedef struct encap_parms {
    u_long  arglen;
    char   *args;
} ENCAP_PARMS;

static bool_t
xdr_encap_parms(xdrs, epp)
    XDR *xdrs;
    struct encap_parms *epp;
{
    return (xdr_bytes(xdrs, &(epp->args), (u_int *)&(epp->arglen), (u_int)ARGSIZE));
}

typedef struct rmtcallargs {
    u_long   rmt_prog;
    u_long   rmt_vers;
    u_long   rmt_port;
    u_long   rmt_proc;
    struct encap_parms rmt_args;
} RMTCALLARGS;

static bool_t
xdr_rmtcall_args(xdrs, cap)
    register XDR *xdrs;
    register struct rmtcallargs *cap;
{
    /* does not get a port number */
    if (xdr_u_long(xdrs, &(cap->rmt_prog)) &&
        xdr_u_long(xdrs, &(cap->rmt_vers)) &&
        xdr_u_long(xdrs, &(cap->rmt_proc))) {
        return (xdr_encap_parms(xdrs, &(cap->rmt_args)));
    }
    return (FALSE);
}

static bool_t
xdr_rmtcall_result(xdrs, cap)
    register XDR *xdrs;
    register struct rmtcallargs *cap;
{
    if (xdr_u_long(xdrs, &(cap->rmt_port)))
        return (xdr_encap_parms(xdrs, &(cap->rmt_args)));
    return (FALSE);
}

/*
 * only worries about the struct encap_parms part of struct rmtcallargs.
 * The arglen must already be set!!
 */
static bool_t
xdr_opaque_parms(xdrs, cap)
    XDR *xdrs;
    struct rmtcallargs *cap;
{
    return (xdr_opaque(xdrs, cap->rmt_args.args, (u_int)cap->rmt_args.arglen));
}

/*
 * This routine finds and sets the length of incoming opaque paraters
 * and then calls xdr_opaque_parms.
 */
static bool_t
xdr_len_opaque_parms(xdrs, cap)
    register XDR *xdrs;
    struct rmtcallargs *cap;
{
    register u_int beginpos, lowpos, highpos, currpos, pos;

    beginpos = lowpos = pos = xdr_getpos(xdrs);
    highpos = lowpos + ARGSIZE;
    while ((int)(highpos - lowpos) >= 0) {
        currpos = (lowpos + highpos) / 2;
        if (xdr_setpos(xdrs, currpos)) {
            pos = currpos;
            lowpos = currpos + 1;
        } else {
            highpos = currpos - 1;
        }
    }
    xdr_setpos(xdrs, beginpos);
    cap->rmt_args.arglen = pos - beginpos;
    return (xdr_opaque_parms(xdrs, cap));
}

/*
 * Call a remote procedure service
 * This procedure is very quiet when things go wrong.
 * The proc is written to support broadcast rpc.  In the broadcast case,
 * a machine should shut-up instead of complain, less the requestor be
 * overrun with complaints at the expense of not hearing a valid reply ...
 */
static void
callit(rqstp, xprt)
    struct svc_req *rqstp;
    SVCXPRT *xprt;
{
    char  *buf;
    struct rmtcallargs a;
    struct pmaplist *pml;
    u_short port;
    struct sockaddr_in me;
    int socket = -1;
    CLIENT *client;
    struct authunix_parms *au = (struct authunix_parms *)rqstp->rq_clntcred;
    struct timeval timeout;

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    buf = malloc(ARGSIZE);
    if (buf == NULL)
        return;

    a.rmt_args.args = buf;
    if (!svc_getargs(xprt, xdr_rmtcall_args, (caddr_t )&a)) {
        free(buf);
        return;
    }

    if ((pml = find_service(a.rmt_prog, a.rmt_vers, IPPROTO_UDP)) == NULL) {
        free(buf);
        return;
    }

    port = (u_short)pml->pml_map.pm_port;
    get_myaddress(&me);
    me.sin_port = (u_short)htons(port);
    client = clntudp_create(&me, a.rmt_prog, a.rmt_vers, timeout, &socket);
    if (client != (CLIENT *)NULL) {
        if (rqstp->rq_cred.oa_flavor == AUTH_UNIX) {
            client->cl_auth = authunix_create(au->aup_machname,
                                              au->aup_uid,
                                              au->aup_gid,
                                              (int)au->aup_len,
                                              au->aup_gids);
            if (client->cl_auth == NULL) {
                free(buf);
                return;
            }
        }
        a.rmt_port = (u_long)port;
        if (clnt_call(client, a.rmt_proc,
                      xdr_opaque_parms, (caddr_t)&a,
                      xdr_len_opaque_parms, (caddr_t)&a,
                      timeout) == RPC_SUCCESS) {
            (void)svc_sendreply(xprt, xdr_rmtcall_result, (char *)&a);
        }
        AUTH_DESTROY(client->cl_auth);
        clnt_destroy(client);
    }
    free(buf);
    return;

    /*
     * the following close ALWAYS fails
     * because clnt_destroy was called, which closed the socket.
     */
    /*
     * (void)close(socket);
     */
}

