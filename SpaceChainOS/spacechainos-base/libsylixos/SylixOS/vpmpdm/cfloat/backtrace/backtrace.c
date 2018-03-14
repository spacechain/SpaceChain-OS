/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 * 
 * Author: Han.hui <sylixos@gmail.com>
 */

#include <unistd.h>

#if LW_CFG_MODULELOADER_EN > 0

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>
#include <execinfo.h>

#define WORD_WIDTH 16

/*
 *  backtrace_symbols
 */
char **backtrace_symbols (void *const *array, int size)
{
    Dl_info info[size];
    int status[size];
    int cnt;
    size_t total = 0;
    char **result;

    /* Fill in the information we can get from `dladdr'.  */
    for (cnt = 0; cnt < size; ++cnt) {
        status[cnt] = dladdr (array[cnt], &info[cnt]);
        if (status[cnt] && info[cnt].dli_fname && info[cnt].dli_fname[0] != '\0') {
            /* We have some info, compute the length of the string which will be
               "<file-name>(<sym-name>+offset) [address].  */
            total += (strlen (info[cnt].dli_fname ?: "")
                  + strlen (info[cnt].dli_sname ?: "")
                  + 3 + WORD_WIDTH + 3 + WORD_WIDTH + 5);
        } else {
            total += 5 + WORD_WIDTH;
        }
    }

    /* Allocate memory for the result.  */
    result = (char **) malloc (size * sizeof (char *) + total);
    if (result != NULL) {
        char *last = (char *) (result + size);

        for (cnt = 0; cnt < size; ++cnt) {
            result[cnt] = last;

            if (status[cnt]
                && info[cnt].dli_fname != NULL && info[cnt].dli_fname[0] != '\0') {
                if (info[cnt].dli_sname == NULL) {
                    /* We found no symbol name to use, so describe it as
                       relative to the file.  */
                    info[cnt].dli_saddr = info[cnt].dli_fbase;
                }

                if (info[cnt].dli_sname == NULL && info[cnt].dli_saddr == 0) {
                    last += 1 + sprintf (last, "%s(%s) [%p]",
                                 info[cnt].dli_fname ?: "",
                                 info[cnt].dli_sname ?: "",
                                 array[cnt]);
                } else {
                    char sign;
                    ptrdiff_t offset;
                    if (array[cnt] >= (void *) info[cnt].dli_saddr) {
                        sign = '+';
                        offset = array[cnt] - info[cnt].dli_saddr;
                    
                    } else {
                        sign = '-';
                        offset = info[cnt].dli_saddr - array[cnt];
                    }

                    last += 1 + sprintf (last, "%s(%s%c%#tx) [%p]",
                                 info[cnt].dli_fname ?: "",
                                 info[cnt].dli_sname ?: "",
                                 sign, offset, array[cnt]);
                }
            } else {
                last += 1 + sprintf (last, "[%p]", array[cnt]);
            }
        }
        assert (last <= (char *) result + size * sizeof (char *) + total);
    }

    return result;
}

/*
 *  backtrace_symbols_fd
 */
void  backtrace_symbols_fd (void *const *array, int size, int fd)
{
    struct iovec iov[9];
    int cnt;

    for (cnt = 0; cnt < size; ++cnt) {
        char buf[WORD_WIDTH];
        char buf2[WORD_WIDTH];
        Dl_info info;
        size_t last = 0;

        if (dladdr (array[cnt], &info)
            && info.dli_fname != NULL && info.dli_fname[0] != '\0') {
            /* Name of the file.  */
            iov[0].iov_base = (void *) info.dli_fname;
            iov[0].iov_len = strlen (info.dli_fname);
            last = 1;

            if (info.dli_sname != NULL) {
                size_t diff;

                iov[last].iov_base = (void *) "(";
                iov[last].iov_len = 1;
                ++last;

                if (info.dli_sname != NULL) {
                    /* We have a symbol name.  */
                    iov[last].iov_base = (void *) info.dli_sname;
                    iov[last].iov_len = strlen (info.dli_sname);
                    ++last;
                }

                if (array[cnt] >= (void *) info.dli_saddr) {
                    iov[last].iov_base = (void *) "+0x";
                    diff = array[cnt] - info.dli_saddr;
                
                } else {
                    iov[last].iov_base = (void *) "-0x";
                    diff = info.dli_saddr - array[cnt];
                }
                iov[last].iov_len = 3;
                ++last;

                iov[last].iov_base = itoa((int)diff, buf2, 16);
                iov[last].iov_len  = (&buf2[WORD_WIDTH]
                                   - (char *) iov[last].iov_base);
                ++last;

                iov[last].iov_base = (void *) ")";
                iov[last].iov_len = 1;
                ++last;
            }
        }

        iov[last].iov_base = (void *) "[0x";
        iov[last].iov_len = 3;
        ++last;

        iov[last].iov_base = itoa((int)array[cnt], buf, 16);
        iov[last].iov_len = &buf[WORD_WIDTH] - (char *) iov[last].iov_base;
        ++last;

        iov[last].iov_base = (void *) "]\n";
        iov[last].iov_len = 2;
        ++last;

        writev(fd, iov, last);
    }
}

#endif /* LW_CFG_MODULELOADER_EN > 0 */
/*
 * end
 */
