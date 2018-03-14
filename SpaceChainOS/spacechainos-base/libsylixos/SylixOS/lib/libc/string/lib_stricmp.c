/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */

#include "../SylixOS/kernel/include/k_kernel.h"

int
lib_stricmp(const char *s1, const char *s2)
{
  while (lib_tolower((unsigned char)*s1) == lib_tolower((unsigned char)*s2))
  {
    if (*s1 == 0)
      return 0;
    s1++;
    s2++;
  }
  return (int)lib_tolower((unsigned char)*s1) - (int)lib_tolower((unsigned char)*s2);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
