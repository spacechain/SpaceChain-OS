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
 */

#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <acoinfo.h>

#define MAC_SIZE    6   /*  MAC address size                    */
#define HOST_SIZE   2   /*  leading two character net device    */

struct ether_addr {
	unsigned char ether_addr_octet[6];
};

/*
 * Get MAC address according to network device name
 */
int ether_hostton(char *hostname, const struct ether_addr *addr)
{
  struct ifreq ifr;
	int fd = -1;
	int ret = -1;

	/* check hostname valid */
	if ((NULL == hostname) || (strncmp(hostname, "en", HOST_SIZE)))
		return -1;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd <= 0)
		return -2;

	strncpy(ifr.ifr_name, hostname, IFNAMSIZ - 1);

	/* Get MAC from ioctl according to device name */
	ret = ioctl(fd, SIOCGIFHWADDR, &ifr);
	if (ret < 0) {
		close(fd);
		return -3;
	}

	memcpy((char*)addr, ifr.ifr_ifru.ifru_hwaddr.sa_data, MAC_SIZE);

	close(fd);
	return 0;
}
