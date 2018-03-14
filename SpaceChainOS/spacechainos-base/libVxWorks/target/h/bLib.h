/**
 * @file
 * errno library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_BLIB_H
#define __VXWORKS_BLIB_H

#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

int  bcmp(char *buf1, char *buf2, size_t nbytes);
void binvert(char *buf, size_t nbytes);
void bswap(char *buf1, char *buf2, size_t nbytes);
void swab(char *source, char *destination, size_t nbytes);
void uswab(char *source, char *destination, size_t nbytes);
void bcopyBytes(char *source, char *destination, size_t nbytes);
void bcopyWords(char *source, char *destination, size_t nwords);
void bcopyLongs(char *source, char *destination, size_t nlongs);
void bcopyQuads(char *source, char *destination, size_t nquads);
void bfill(char *buf, size_t nbytes, int ch);
void bfillBytes(char *buf, size_t nbytes, int ch);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_BLIB_H */
