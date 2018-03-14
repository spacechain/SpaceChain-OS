/*
 *  $Id: getprotoby.c,v 1.5 2010/03/28 05:44:42 ralf Exp $
 */
 
#include "netdb.h"
#include "string.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"

#if LW_CFG_NET_EN > 0

static const struct protoent prototab[] = {
					{ "ip",    NULL, IPPROTO_IP },
					{ "icmp",  NULL, IPPROTO_ICMP },
					{ "tcp",   NULL, IPPROTO_TCP },
					{ "udp",   NULL, IPPROTO_UDP },
					};

/*
 * Dummy version of BSD getprotobyname()
 */
struct protoent *
getprotobyname_static (const char *name)
{
	int i;

	for (i = 0 ; i < (sizeof prototab / sizeof prototab[0]) ; i++) {
		if (strcmp (name, prototab[i].p_name) == 0)
			return (struct protoent *) &prototab[i];
	}
	return NULL;
}

/*
 * Dummy version of BSD getprotobynumber()
 */
struct protoent *
getprotobynumber_static (int proto)
{
	int i;

	for (i = 0 ; i < (sizeof prototab / sizeof prototab[0]) ; i++) {
		if (proto == prototab[i].p_proto)
			return (struct protoent *) &prototab[i];
	}
	return NULL;
}

#endif  /*  LW_CFG_NET_EN > 0  */
