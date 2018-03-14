/**
 * @file
 * KidVPN main.
 * as much as possible compatible with different versions of LwIP
 * Verification using sylixos(tm) real-time operating system
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
 * Author: Han.hui <hanhui@acoinfo.com>
 *
 */

#include "kv_lib.h"
#include "kv_serv.h"
#include "kv_client.h"

/* version */
#define KV_VERSION  "0.0.2"

/* key code change function */
static int key_code_change (unsigned char *key, unsigned int *keybits, const char *keyascii)
{
    int  ascii_len = strlen(keyascii);
    int  i, loop;
    unsigned char tmp;

    if ((ascii_len >= 32) && (ascii_len <= 48)) {
        *keybits = 128;
        loop = 16;

    } else if ((ascii_len >= 48) && (ascii_len <= 64)) {
        *keybits = 192;
        loop = 24;

    } else if (ascii_len >= 64) {
        *keybits = 256;
        loop = 32;

    } else {
        fprintf(stderr, "[KidVPN] Key length error.\n");
        return  (-1);
    }

    for (i = 0; i < loop; i++) {
        if ((keyascii[0] >= '0') && (keyascii[0] <= '9')) {
            tmp = ((keyascii[0] - '0') << 4);

        } else if ((keyascii[0] >= 'a') && (keyascii[0] <= 'f')) {
            tmp = ((keyascii[0] - 'a' + 10) << 4);

        } else if ((keyascii[0] >= 'A') && (keyascii[0] <= 'F')) {
            tmp = ((keyascii[0] - 'A' + 10) << 4);

        } else {
            fprintf(stderr, "[KidVPN] Key format error.\n");
            return  (-1);
        }

        if ((keyascii[1] >= '0') && (keyascii[1] <= '9')) {
            tmp |= (keyascii[1] - '0');

        } else if ((keyascii[1] >= 'a') && (keyascii[1] <= 'f')) {
            tmp |= (keyascii[1] - 'a' + 10);

        } else if ((keyascii[1] >= 'A') && (keyascii[1] <= 'F')) {
            tmp |= (keyascii[1] - 'A' + 10);

        } else {
            fprintf(stderr, "[KidVPN] Key format error.\n");
            return  (-1);
        }

        *key = tmp;
        key++;
        keyascii += 2;
    }

    return  (0);
}

/* main function */
int main (int argc, char *argv[])
{
    int  i, vnd_id, rand_fd, mtu = KV_VND_DEF_MTU;
    FILE *fkey;
    char *straddr;
    char keyfile[PATH_MAX];
    char keyascii[65];
    unsigned char keycode[32];
    unsigned int keybits;

    if (argc < 3) {
usage:
        printf("USAGE: kidvpn [-c | -s] [vnd id] [-mtu MTU(1280-1472)] [server | local ip]\n"
               "       kidvpn -c 0 -mtu 1472 123.123.123.123 "
               "(Run as KidVPN client and use virtual net device 0 (MTU:1472) connect to server 123.123.123.123)\n"
               "       kidvpn -s 0 -mtu 1280 192.168.0.1     "
               "(Run as KidVPN server and use virtual net device 0 (MTU:1280) bind local ip 192.168.0.1)\n"
               "       kidvpn -genkey 128          "
               "(Generate a AES-128 key)\n\n"
               "KidVPN Current Version: %s\n", KV_VERSION);
        return  (0);
    }

    if (argc == 3) {
        if (!strcmp(argv[1], "-genkey")) {
            rand_fd = open("/dev/random", O_RDONLY);
            if (rand_fd < 0) {
                fprintf(stderr, "[KidVPN] Can not open /dev/random file, error: %s\n", strerror(errno));
                return  (-1);
            }

            keybits = atoi(argv[2]);
            switch (keybits) {

            case 128:
                read(rand_fd, keycode, 16);
                close(rand_fd);
                break;

            case 192:
                read(rand_fd, keycode, 24);
                close(rand_fd);
                break;

            case 256:
                read(rand_fd, keycode, 32);
                close(rand_fd);
                break;

            default:
                close(rand_fd);
                fprintf(stderr, "[KidVPN] Key bits only support: 128, 192, 256\n");
                return  (-1);
            }

            printf("[KidVPN] Key: ");
            for (i = 0; i < (keybits >> 3); i++) {
                printf("%02x", keycode[i]);
            }
            printf("\n");
            return  (0);

        } else {
            goto    usage;
        }

    } else {
        vnd_id = atoi(argv[2]); /* vnd id */
        snprintf(keyfile, PATH_MAX, "/etc/kidvpn/%d.key", vnd_id); /* build key file name */

        fkey = fopen(keyfile, "r"); /* open key file */
        if (!fkey) {
            fprintf(stderr, "[KidVPN] Open %s error: %s\n", keyfile, strerror(errno));
            return  (-1);
        }

        if (!fgets(keyascii, 65, fkey)) { /* read aes key */
            fprintf(stderr, "[KidVPN] Key file %s error: %s\n", keyfile, strerror(errno));
            fclose(fkey);
            return  (-1);
        }
        fclose(fkey);

        if (key_code_change(keycode, &keybits, keyascii)) { /* get aes key */
            return  (-1);
        }

        if (!strcmp(argv[3], "-mtu")) { /* set MTU */
            if (argc < 6) {
                goto    usage;
            }

            mtu = atoi(argv[4]);
            if ((mtu > KV_VND_MAX_MTU) || (mtu < KV_VND_MIN_MTU)) {
                fprintf(stderr, "[KidVPN] MTU must in %d ~ %d\n", KV_VND_MIN_MTU, KV_VND_MAX_MTU);
                return  (-1);
            }
            straddr = argv[5];

        } else {
            straddr = argv[3];
        }

        if (!strcmp(argv[1], "-c")) {
            daemon(1, 1); /* make client to a daemon mode */
            return  (kv_cli_start(vnd_id, keycode, keybits, straddr, mtu));

        } else if (!strcmp(argv[1], "-s")) {
            daemon(1, 1); /* make server to a daemon mode */
            return  (kv_serv_start(vnd_id, keycode, keybits, straddr, mtu));

        } else {
            goto    usage;
        }
    }
}

/*
 * end
 */
