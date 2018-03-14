/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

/*
 * Copyright (C) 2006 Aleksey Cheusov
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted 
 * without fee. Permission to modify the code and to distribute modified
 * code is also granted without any restrictions.
 */

#include "wcfix.h"

#include <stdlib.h>
#include <assert.h>
#include <wchar.h>

/*
 * wcsdup
 */
__weak_reference(wcsdup,_wcsdup);

wchar_t *wcsdup (const wchar_t *str)
{
	wchar_t *copy;
	size_t len;

	_DIAGASSERT(str != NULL);

	len = wcslen(str) + 1;
	copy = lib_malloc(len * sizeof (wchar_t));

	if (!copy)
		return NULL;

	return wmemcpy(copy, str, len);
}

/*
 * end
 */
