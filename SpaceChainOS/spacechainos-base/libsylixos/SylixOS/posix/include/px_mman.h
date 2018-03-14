/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: px_mman.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 08 月 12 日
**
** 描        述: POSIX IEEE Std 1003.1, 2004 Edition sys/mman.h
*********************************************************************************************************/

#ifndef __PX_MMAN_H
#define __PX_MMAN_H

#include "SylixOS.h"                                                    /*  操作系统头文件              */

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

#if LW_CFG_DEVICE_EN > 0

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  page flag
*********************************************************************************************************/

#define PROT_READ                       1                               /*  Page can be read.           */
#define PROT_WRITE                      2                               /*  Page can be written.        */
#define PROT_EXEC                       4                               /*  Page can be executed.       */
#define PROT_NONE                       0                               /*  Page cannot be accessed.    */

/*********************************************************************************************************
  map flag
*********************************************************************************************************/

#define MAP_FILE                        0                               /*  File                        */
#define MAP_SHARED                      0x01                            /*  Share changes.              */
#define MAP_PRIVATE                     0x02                            /*  Changes are private.        */
#define MAP_FIXED                       0x04                            /*  Interpret addr exactly.     */
#define MAP_ANONYMOUS                   0x08                            /*  no fd rela this memory      */
#define MAP_NORESERVE                   0x10                            /*  Do not swap                 */

#ifndef MAP_ANON
#define MAP_ANON                        MAP_ANONYMOUS
#endif

/*********************************************************************************************************
  msync() flag
*********************************************************************************************************/

#define MS_ASYNC                        1                               /*  Perform asynchronous writes.*/
#define MS_SYNC                         2                               /*  Perform synchronous writes. */
#define MS_INVALIDATE                   4                               /*  Invalidate mappings.        */

/*********************************************************************************************************
  memory lock flag
*********************************************************************************************************/

#define MCL_CURRENT                     1                               /*  Lock currently mapped pages.*/
#define MCL_FUTURE                      2                               /*  Lock become mapped pages.   */

/*********************************************************************************************************
  map fail
*********************************************************************************************************/

#define MAP_FAILED                      ((void *)-1)

/*********************************************************************************************************
  anonymous fd
*********************************************************************************************************/

#define MAP_ANON_FD                     (-1)

/*********************************************************************************************************
  mremap() flag
*********************************************************************************************************/

#define MREMAP_MAYMOVE                  1
#define MREMAP_FIXED                    2

/*********************************************************************************************************
  API
*********************************************************************************************************/

LW_API int      mlock(const void  *pvAddr, size_t  stLen);
LW_API int      munlock(const void  *pvAddr, size_t  stLen);
LW_API int      mlockall(int  iFlag);
LW_API int      munlockall(void);
LW_API int      mprotect(void  *pvAddr, size_t  stLen, int  iProt);
LW_API void    *mmap(void  *pvAddr, size_t  stLen, int  iProt, int  iFlag, int  iFd, off_t  off);
LW_API void    *mmap64(void  *pvAddr, size_t  stLen, int  iProt, int  iFlag, int  iFd, off64_t  off);
LW_API void    *mremap(void *pvAddr, size_t stOldSize, size_t stNewSize, int iFlag, ...);
LW_API int      munmap(void  *pvAddr, size_t  stLen);
LW_API int      msync(void  *pvAddr, size_t  stLen, int  iFlag);

/*********************************************************************************************************
  memory advisory information and alignment control
*********************************************************************************************************/

#define POSIX_MADV_NORMAL       0                                       /*  no further special treatment*/
#define POSIX_MADV_RANDOM       1                                       /*  expect random page refs     */
#define POSIX_MADV_SEQUENTIAL   2                                       /*  expect sequential page refs */
#define POSIX_MADV_WILLNEED     3                                       /*  will need these pages       */
#define POSIX_MADV_DONTNEED     4                                       /*  dont need these pages       */

LW_API int      posix_madvise(void *addr, size_t len, int advice);

/*********************************************************************************************************
  share memory
*********************************************************************************************************/

LW_API int      shm_open(const char *name, int oflag, mode_t mode);
LW_API int      shm_unlink(const char * name);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
#endif                                                                  /*  __PX_MMAN_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
