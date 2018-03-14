/**
 * @file
 * SylixOS SNMPv3 functions.
 */

/*
 * Copyright (c) 2017 Dirk Ziegelmeier.
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
 * Author: Dirk Ziegelmeier <dziegel@gmx.de>
 *         Xu Guizhou <xuguizhou@acoinfo.com>
 */

#ifndef LWIP_HDR_APPS_SNMP_V3_SYLIXOS_H
#define LWIP_HDR_APPS_SNMP_V3_SYLIXOS_H

#include "lwip/snmp/snmp_opts.h"
#include "lwip/err.h"
#include "lwip/snmp/snmpv3.h"

#if LWIP_SNMP && LWIP_SNMP_V3

#define SNMPV3_FILE_CONFIG    1
#define ENGINEID_DEFAULT      "SylixOS"
#define SNMPV3_BUF_MAX        128
#define SNMPV3_USER_MAX       16
#define ENGINEID_HEADER       5
#define SNMP_MAX_TIME_BOOT    2147483647UL

#define ENGINEID_TYPE_IPV4    1
#define ENGINEID_TYPE_IPV6    2
#define ENGINEID_TYPE_MACADDR 3
#define ENGINEID_TYPE_TEXT    4
#define ENGINEID_TYPE_EXACT   5

#define SNMPV3_BOOTS          "/var/log/snmp/snmpv3_boots"
#define SNMPV3_ENGINEID       "/etc/snmp/snmpv3_engineid"
#define SNMPV3_USER           "/etc/snmp/snmpv3_user"

err_t snmpv3_set_user_auth_algo(const char *username, snmpv3_auth_algo_t algo);
err_t snmpv3_set_user_priv_algo(const char *username, snmpv3_priv_algo_t algo);
err_t snmpv3_set_user_auth_key(const char *username, const char *password);
err_t snmpv3_set_user_priv_key(const char *username, const char *password);

void snmpv3_sylixos_init(void);

#endif /* LWIP_SNMP && LWIP_SNMP_V3 */

#endif /* LWIP_HDR_APPS_SNMP_V3_SYLIXOS_H */
