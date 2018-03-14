/*
 * linux string support
 * drivers and users.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define  __SYLIXOS_KERNEL
#include "SylixOS.h"

/**
 * skip_spaces - Removes leading whitespace from @str.
 * @str: The string to be stripped.
 *
 * Returns a pointer to the first non-whitespace character in @str.
 */
char *skip_spaces(const char *str)
{
    while (lib_isspace(*str))
        ++str;
    return (char *)str;
}

/**
 * strim - Removes leading and trailing whitespace from @s.
 * @s: The string to be stripped.
 *
 * Note that the first trailing whitespace is replaced with a %NUL-terminator
 * in the given string @s. Returns a pointer to the first non-whitespace
 * character in @s.
 */
char *strim(char *s)
{
    size_t size;
    char *end;

    s = skip_spaces(s);
    size = lib_strlen(s);
    if (!size)
        return s;

    end = s + size - 1;
    while (end >= s && lib_isspace(*end))
        end--;
    *(end + 1) = '\0';

    return s;
}

/*
 * end
 */
