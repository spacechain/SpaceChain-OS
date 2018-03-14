/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * /bin/env /usr/bin/env
 * this file is support current module malloc/free use his own heap in a vprocess.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>

extern char **environ;

static void usage (void)
{
    fprintf(stderr, "usage: env [-i] [name=value ...] [utility [argument ...]]\n");
    exit(1);
}

int main (int argc, char **argv)
{
    char **ep, *p;
    char *cleanenv[1];
    int   ch;

    while ((ch = getopt(argc, argv, "i-")) != -1) {
        switch(ch) {

        case '-':
        case 'i':
            environ = cleanenv;
            cleanenv[0] = NULL;
            break;

        case '?':
        default:
            usage();
        }
    }

    for (argv += optind; *argv && (p = strchr(*argv, '=')); ++argv) {
        putenv(*argv);
    }

    if (*argv) {
        execvp(*argv, argv);
        perror(*argv);
    }

    for (ep = environ; *ep; ep++) {
        printf("%s\n", *ep);
    }

    exit(0);
}

/*
 * end
 */
