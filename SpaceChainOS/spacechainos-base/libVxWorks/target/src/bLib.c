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

#include <stddef.h>
#include <SylixOS.h>

/*
 * bcmp - compare one buffer to another
 */
int bcmp (char *buf1, char *buf2, size_t nbytes)
{
    unsigned char *p1;
    unsigned char *p2;

    if (nbytes == 0) {
        return  (0);
    }

    p1 = (unsigned char *)buf1;
    p2 = (unsigned char *)buf2;

    while (*p1++ == *p2++) {
        if (--nbytes == 0) {
            return  (0);
        }
    }

    return  ((*--p1) - (*--p2));
}

/*
 * binvert - invert the order of bytes in a buffer
 */
void binvert (char *buf, size_t nbytes)
{
    char *buf_end = buf + nbytes - 1;
    char  temp;

    while (buf < buf_end) {
        temp     = *buf;
        *buf     = *buf_end;
        *buf_end = temp;

        buf_end--;
        buf++;
    }
}

/*
 * bswap - swap buffers
 */
void bswap (char *buf1, char *buf2, size_t nbytes)
{
    char temp;

    while (nbytes >= 1) {
        temp    = *buf1;
        *buf1++ = *buf2;
        *buf2++ = temp;
        nbytes--;
    }
}

/*
 * swab - swap bytes
 */
void swab (char *source, char *destination, size_t nbytes)
{
    unsigned short *src     = (unsigned short *)source;
    unsigned short *dst     = (unsigned short *)destination;
    unsigned short *dst_end = (unsigned short *)(destination + nbytes);

    for (; dst < dst_end; dst++, src++) {
        *dst = (short)(((*src & 0x00ff) << 8) | ((*src & 0xff00) >> 8));
    }
}

/*
 * uswab - swap bytes with buffers that are not necessarily aligned
 */
void uswab (char *source, char *destination, size_t nbytes)
{
    char *dst     = (char *)destination;
    char *dst_end = dst + nbytes;
    char byte1;
    char byte2;

    while (dst < dst_end) {
        byte1 = *source++;
        byte2 = *source++;
        *dst++ = byte2;
        *dst++ = byte1;
    }
}

/*
 * bcopyBytes - copy one buffer to another one byte (8 bits) at a time
 */
void bcopyBytes (char *source, char *destination, size_t nbytes)
{
    char *dstend;
    size_t offset = (size_t)(destination - source);

    if (offset == 0) {
        return;
    }

    if (offset >= nbytes) { /* true also when destination < source if no wrap */
        /*
         * forward copy
         */
        dstend = destination + nbytes;
        while (destination != dstend) {
            *destination++ = *source++;
        }
    } else {
        /*
         * backward copy
         */
        dstend       = destination;
        destination += nbytes;
        source      += nbytes;

        while (destination != dstend) {
            *--destination = *--source;
        }
    }
}

/*
 * bcopyWords - copy one buffer to another one word (16 bits) at a time
 */
void bcopyWords (char *source, char *destination, size_t nwords)
{
    short *dstend;
    short *src    = (short *)source;
    short *dst    = (short *)destination;
    size_t nbytes = nwords << 1;           /* convert to bytes */
    size_t offset = (size_t)(destination - source);

    if (offset == 0) {
        return;
    }

    if (offset >= nbytes) { /* true also when destination < source if no wrap */
        /*
         * forward copy
         */
        dstend = dst + nwords;
        while (dst != dstend) {
            *dst++ = *src++;
        }
    } else {
        /*
         * backward copy
         */
        dstend = dst;
        dst   += nwords;
        src   += nwords;
        while (dst != dstend) {
            *--dst = *--src;
        }
    }
}

/*
 * bcopyLongs - copy one buffer to another one long word (32 bits) at a time
 */
void bcopyLongs (char *source, char *destination, size_t nlongs)
{
    UINT32 *dstend;
    UINT32 *src   = (UINT32 *)source;
    UINT32 *dst   = (UINT32 *)destination;
    size_t nbytes = nlongs * sizeof(UINT32);    /* convert to bytes */
    size_t offset = (size_t)(destination - source);

    if (offset == 0) {
        return;
    }

    if (offset >= nbytes) { /* true also when destination < source if no wrap */
        /*
         * forward copy
         */
        dstend = dst + nlongs;
        while (dst != dstend) {
            *dst++ = *src++;
        }
    } else {
        /*
         * backward copy
         */
        dstend = dst;
        dst   += nlongs;
        src   += nlongs;
        while (dst != dstend) {
            *--dst = *--src;
        }
    }
}

/*
 * bcopyQuads - copy one buffer to another quad word (64 bits) at a time (64-bit)
 */
void bcopyQuads (char *source, char *destination, size_t nquads)
{
    UINT64 *dstend;
    UINT64 *src   = (UINT64 *)source;
    UINT64 *dst   = (UINT64 *)destination;
    size_t nbytes = nquads * sizeof(UINT64);    /* convert to bytes */
    size_t offset = (size_t)(destination - source);

    if (offset == 0) {
        return;
    }

    if (offset >= nbytes) { /* true also when destination < source if no wrap */
        /*
         * forward copy
         */
        dstend = dst + nquads;
        while (dst != dstend) {
            *dst++ = *src++;
        }
    } else {
        /*
         * backward copy
         */
        dstend = dst;
        dst   += nquads;
        src   += nquads;
        while (dst != dstend) {
            *--dst = *--src;
        }
    }
}

/*
 * bfill - fill a buffer with a specified character
 */
void bfill (char *buf, size_t nbytes, int ch)
{
    lib_memset(buf, ch, nbytes);
}

/*
 * bfillBytes - fill buffer with a specified character one byte at a time
 */
void bfillBytes (char *buf, size_t nbytes, int ch)
{
    char *bufend = buf + nbytes;

    while (buf != bufend) {
        *buf++ = (char)ch;
    }
}

/*
 * end
 */
