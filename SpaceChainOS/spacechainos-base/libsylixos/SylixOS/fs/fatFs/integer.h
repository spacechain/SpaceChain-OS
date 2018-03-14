/*-------------------------------------------*/
/* Integer type definitions for FatFs module */
/*-------------------------------------------*/

#ifndef _INTEGER
#define _INTEGER

#include "../SylixOS/kernel/include/k_kernel.h"

/* These types MUST be 16 bit */
typedef INT16			SHORT;
typedef UINT16	        USHORT;
typedef UINT16	        WCHAR;
typedef UINT16          WORD;

/* These types MUST be 32 bit */
typedef UINT32          DWORD;

/* These types MUST be 64 bit */
typedef UINT64          QWORD;

#endif
