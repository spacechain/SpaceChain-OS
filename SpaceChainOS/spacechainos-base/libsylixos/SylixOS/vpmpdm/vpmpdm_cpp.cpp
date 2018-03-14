/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 * 
 * Author: Han.hui <sylixos@gmail.com>
 */

#include <SylixOS.h>

extern "C" {
void    *lib_malloc_new(size_t nbytes);
void     lib_free(void *p);
}

/*
 *  c++ new, delete, new[], delete[] operator
 */
void  *operator  new (size_t nbytes)
{
    return  (lib_malloc_new(nbytes));
}

void  *operator  new[] (size_t nbytes)
{
    return  (lib_malloc_new(nbytes));
}

void  operator  delete (void  *p)
{
    if (p) {
        lib_free(p);
    }
}

void  operator  delete[] (void  *p)
{
    if (p) {
        lib_free(p);
    }
}

/*
 * end
 */
