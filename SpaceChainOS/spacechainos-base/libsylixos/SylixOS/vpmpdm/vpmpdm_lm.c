/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * LOCAL MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 * 
 * Author: Han.hui <sylixos@gmail.com>
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <netinet/in.h>
#include <netinet6/in6.h>

/*
 * ipaddr_ntoa
 */
char *ip4addr_ntoa (const ip4_addr_t *addr)
{
    static char str[INET_ADDRSTRLEN];
    return  (ip4addr_ntoa_r(addr, str, INET_ADDRSTRLEN));
}

/*
 * inet_ntoa
 */
char *inet_ntoa (struct in_addr addr)
{
    return  (ip4addr_ntoa((const ip4_addr_t*)&(addr)));
}

/*
 * ip6addr_ntoa
 */
char *ip6addr_ntoa (const ip6_addr_t *addr)
{
    static char str[INET6_ADDRSTRLEN];
    return  (ip6addr_ntoa_r(addr, str, INET6_ADDRSTRLEN));
}

/*
 * inet6_ntoa
 */
char *inet6_ntoa (struct in6_addr addr)
{
    return  (ip6addr_ntoa((const ip6_addr_t*)&(addr)));
}

/*
 * strtok
 */
char *strtok (char *s, const char *delim)
{
    static char *last;
    return  (strtok_r(s, delim, &last));
}

/*
 * getlogin
 */
char *getlogin (void)
{
    static char cNameBuffer[LOGIN_NAME_MAX];
    getlogin_r(cNameBuffer, LOGIN_NAME_MAX);
    return  (cNameBuffer);
}

/*
 * end
 */
