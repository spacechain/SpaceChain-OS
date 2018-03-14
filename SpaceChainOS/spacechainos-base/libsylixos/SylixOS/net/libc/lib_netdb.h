/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: lib_netdb.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 04 月 07 日
**
** 描        述: net 相关库 netdb 部分声明. (这里面一些函数需要加入 cextern 库)
*********************************************************************************************************/

#ifndef __LIB_NETDB_H
#define __LIB_NETDB_H

#include "stdio.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _PATH_HEQUIV
#define _PATH_HEQUIV        "/etc/hosts.equiv"
#endif

#define _PATH_HOSTS         "/etc/hosts"
#define _PATH_NETWORKS      "/etc/networks"
#define _PATH_PROTOCOLS     "/etc/protocols"
#define _PATH_SERVICES      "/etc/services"

#ifndef _PATH_SERVICES_DB
#define _PATH_SERVICES_DB   "/var/db/services.db"
#endif

struct netent {
    char         *n_name;                                               /* official name of net         */
    char        **n_aliases;                                            /* alias list                   */
    int           n_addrtype;                                           /* net address type             */
    uint32_t      n_net;                                                /* network #                    */
};

struct servent {
    char     *s_name;                                                   /* official service name        */
    char    **s_aliases;                                                /* alias list                   */
    int       s_port;                                                   /* port #                       */
    char     *s_proto;                                                  /* protocol to use              */
};

struct protoent {
    char     *p_name;                                                   /* official protocol name       */
    char    **p_aliases;                                                /* alias list                   */
    int       p_proto;                                                  /* protocol #                   */
};

/*********************************************************************************************************
  libc net db service (getproto... getserv... application can use libcextern library )
*********************************************************************************************************/

void                setprotoent(int);
void                endprotoent(void);

void                setservent(int);
void                endservent(void);

void                sethostent(int stayopen);
void                endhostent(void);

struct protoent    *getprotobyname(const char *);
struct protoent    *getprotobynumber(int);
struct protoent    *getprotoent(void);

struct servent     *getservbyname(const char *, const char *);
struct servent     *getservbyport(int, const char *);
struct servent     *getservent(void);

struct hostent     *gethostent(void);
struct hostent     *gethostent_r(char* buf, int len);
struct hostent     *gethostent2_r(FILE *fp, char* buf, int buflen);

int                 inet_pton(int af, const char *src, void *dst);
const char         *inet_ntop(int af, const void *src, char *dst, socklen_t size);

/*********************************************************************************************************
  Scope delimit character
*********************************************************************************************************/

#define SCOPE_DELIMITER '%'

/*********************************************************************************************************
  Error return codes from getaddrinfo()
*********************************************************************************************************/

#define EAI_ADDRFAMILY  1                               /*%< address family for hostname not supported  */
#define EAI_AGAIN       2                               /*%< temporary failure in name resolution       */
#define EAI_BADFLAGS    3                               /*%< invalid value for ai_flags                 */
#define EAI_FAIL        4                               /*%< non-recoverable failure in name resolution */
#define EAI_FAMILY      5                               /*%< ai_family not supported                    */
#define EAI_MEMORY      6                               /*%< memory allocation failure                  */
#define EAI_NODATA      7                               /*%< no address associated with hostname        */
#define EAI_NONAME      8                               /*%< hostname nor servname provided or not known*/
#define EAI_SERVICE     9                               /*%< servname not supported for ai_socktype     */
#define EAI_SOCKTYPE    10                              /*%< ai_socktype not supported                  */
#define EAI_SYSTEM      11                              /*%< system error returned in errno             */
#define EAI_BADHINTS    12                              /* invalid value for hints                      */
#define EAI_PROTOCOL    13                              /* resolved protocol is unknown                 */
#define EAI_OVERFLOW    14                              /* argument buffer overflow                     */
#define EAI_MAX         15

/*********************************************************************************************************
 Error return codes from gethostbyname() and gethostbyaddr()
  (left in extern int h_errno).
*********************************************************************************************************/

#define NO_ADDRESS      NO_DATA                         /* no address, look for MX record               */
#define HOST_NOT_FOUND  1                               /*%< Authoritative Answer Host not found        */
#define TRY_AGAIN       2                               /*%< Non-Authoritive Host not found | SERVERFAIL*/
#define NO_RECOVERY     3                               /*%< Non-recoverable, FORMERR|REFUSED|NOTIMP    */
#define NO_DATA         4                               /*%< Valid name no data record of requested type*/

/*********************************************************************************************************
  Flag values for getnameinfo()
*********************************************************************************************************/

#define NI_NOFQDN       0x00000001
#define NI_NUMERICHOST  0x00000002
#define NI_NAMEREQD     0x00000004
#define NI_NUMERICSERV  0x00000008
#define NI_DGRAM        0x00000010
#define NI_WITHSCOPEID  0x00000020
#define NI_NUMERICSCOPE 0x00000040

/*********************************************************************************************************
  sylixos socket defined
*********************************************************************************************************/

LW_API struct hostent  *gethostbyname(const char *name);
LW_API int              gethostbyname_r(const char *name, struct hostent *ret, char *buf,
                                        size_t buflen, struct hostent **result, int *h_errnop);
LW_API struct hostent  *gethostbyaddr(const void *addr, socklen_t length, int type);
LW_API struct hostent  *gethostbyaddr_r(const void *addr, socklen_t length, int type,
                                        struct hostent *ret, char  *buffer, int buflen, int *h_errnop);
LW_API void             freeaddrinfo(struct addrinfo *ai);
LW_API int              getaddrinfo(const char *nodename, const char *servname,
                                    const struct addrinfo *hints, struct addrinfo **res);
LW_API int              getnameinfo(const struct sockaddr *addr, socklen_t len,
                                    char *host, socklen_t hostlen,
                                    char *serv, socklen_t servlen, int flag);

LW_API const char      *gai_strerror(int);

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __LIB_NETDB_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
