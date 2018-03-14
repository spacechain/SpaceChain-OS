/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * LOCAL MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 * 
 * Author: Han.hui <sylixos@gmail.com>
 */

#define __SYLIXOS_KERNEL
#include <SylixOS.h>
#include <stdlib.h>
#include <net/if.h>

/*
 * if_nameindex
 */
struct if_nameindex *if_nameindex (void)
{
    size_t  bufsize;
    void *buf;
    struct if_nameindex *ret;
    
    bufsize = if_nameindex_bufsize();
    if (!bufsize) {
        return  (NULL);
    }
    
    buf = malloc(bufsize);
    if (!buf) {
        errno = ENOMEM;
        return  (NULL);
    }
    
    ret = if_nameindex_rnp(buf, bufsize);
    if (!ret) {
        free(buf);
    }
    
    return  (ret);
}

/*
 * if_freenameindex
 */
void  if_freenameindex (struct if_nameindex *buf)
{
    if (buf) {
        free(buf);
    }
}

/*
 * end
 */
