/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: k_perror.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 11 月 04 日
**
** 描        述: 这是系统 POSIX 错误类型定义。

** BUG:
2010.02.23  EWOULDBLOCK 应该与 EAGAIN 相同.
*********************************************************************************************************/

#ifndef  __K_PERROR_H
#define  __K_PERROR_H

/*********************************************************************************************************
   POSIX ERROR HANDLE
*********************************************************************************************************/

extern errno_t *__errno(VOID);                                          /* get error address            */

#ifndef __errno
#define __errno __errno
#endif                                                                  /*  __errno                     */

#ifndef errno
#define errno   (*__errno())
#endif                                                                  /*  errno                       */

/*********************************************************************************************************
   POSIX ERROR CODE
*********************************************************************************************************/

#define	EPERM			1		                                        /* Not owner                    */
#define	ENOENT			2		                                        /* No such file or directory    */
#define	ESRCH			3		                                        /* No such process              */
#define	EINTR			4		                                        /* Interrupted system call      */
#define	EIO				5		                                        /* I/O error                    */
#define	ENXIO			6		                                        /* No such device or address    */
#define	E2BIG			7		                                        /* Arg list too long            */
#define EOVERFLOW       E2BIG
#define	ENOEXEC			8		                                        /* Exec format error            */
#define	EBADF			9		                                        /* Bad file number              */
#define	ECHILD			10		                                        /* No children                  */
#define	EAGAIN			11		                                        /* No more processes            */
#define	ENOMEM			12		                                        /* No enough memory             */
#define	EACCES			13		                                        /* Permission denied            */
#define	EFAULT			14		                                        /* Bad address                  */
#define	ENOTEMPTY		15		                                        /* Directory not empty          */
#define	EBUSY			16		                                        /* Mount device busy            */
#define	EEXIST			17		                                        /* File exists                  */
#define	EXDEV			18		                                        /* Cross-device link            */
#define	ENODEV			19		                                        /* No such device               */
#define	ENOTDIR			20		                                        /* Not a directory              */
#define	EISDIR			21		                                        /* Is a directory               */
#define	EINVAL			22		                                        /* Invalid argument             */
#define	ENFILE			23		                                        /* File table overflow          */
#define	EMFILE			24		                                        /* Too many open files          */
#define	ENOTTY			25		                                        /* Not a typewriter             */
#define	ENAMETOOLONG	26		                                        /* File name too long           */
#define	EFBIG			27		                                        /* File too large               */
#define	ENOSPC			28		                                        /* No space left on device      */
#define	ESPIPE			29		                                        /* Illegal seek                 */
#define	EROFS			30		                                        /* Read-only file system        */
#define	EMLINK			31		                                        /* Too many links               */
#define	EPIPE			32		                                        /* Broken pipe                  */
#define	EDEADLK			33		                                        /* Resource deadlock avoided    */
#define	ENOLCK			34		                                        /* No locks available           */
#define	ENOTSUP			35		                                        /* Unsupported value            */
#define	EMSGSIZE		36		                                        /* Message size                 */
#define EFTYPE          EINVAL                                          /* File format error            */

/*********************************************************************************************************
   ANSI math software
*********************************************************************************************************/

#define	EDOM			37		                                        /* Argument too large           */
#define	ERANGE			38		                                        /* Result too large             */

/*********************************************************************************************************
   ipc/network software
*********************************************************************************************************/

/*********************************************************************************************************
   argument errors
*********************************************************************************************************/

#define	EDESTADDRREQ	40		                                        /* Destination address required     */
#define	EPROTOTYPE		41		                                        /* Protocol wrong type for socket   */
#define	ENOPROTOOPT		42		                                        /* Protocol not available           */
#define	EPROTONOSUPPORT	43		                                        /* Protocol not supported           */
#define	ESOCKTNOSUPPORT	44		                                        /* Socket type not supported        */
#define	EOPNOTSUPP		45		                                        /* Operation not supported on socket*/
#define	EPFNOSUPPORT	46		                                        /* Protocol family not supported    */
#define	EAFNOSUPPORT	47		                                        /* Addr family not supported        */
#define	EADDRINUSE		48		                                        /* Address already in use           */
#define	EADDRNOTAVAIL	49		                                        /* Can't assign requested address   */
#define	ENOTSOCK		50		                                        /* Socket operation on non-socket   */

/*********************************************************************************************************
   operational errors
*********************************************************************************************************/

#define	ENETUNREACH		51		                                        /* Network is unreachable           */
#define	ENETRESET		52		                                        /* Network dropped connection on reset*/
#define	ECONNABORTED	53		                                        /* Software caused connection abort */
#define	ECONNRESET		54		                                        /* Connection reset by peer         */
#define	ENOBUFS			55		                                        /* No buffer space available        */
#define	EISCONN			56		                                        /* Socket is already connected      */
#define	ENOTCONN		57		                                        /* Socket is not connected          */
#define	ESHUTDOWN		58		                                        /* Can't send after socket shutdown */
#define	ETOOMANYREFS	59		                                        /* Too many references: can't splice*/
#define	ETIMEDOUT		60		                                        /* Connection timed out             */
#define	ECONNREFUSED	61		                                        /* Connection refused               */
#define	ENETDOWN		62		                                        /* Network is down                  */
#define	ETXTBSY			63		                                        /* Text file busy                   */
#define	ELOOP			64		                                        /* Too many levels of symbolic links*/
#define	EHOSTUNREACH	65		                                        /* No route to host                 */
#define	ENOTBLK			66		                                        /* Block device required            */
#define	EHOSTDOWN		67		                                        /* Host is down                     */

/*********************************************************************************************************
   non-blocking and interrupt i/o
*********************************************************************************************************/

#define	EINPROGRESS		68		                                        /* Operation now in progress    */
#define	EALREADY		69		                                        /* Operation already in progress*/
#define	EWOULDBLOCK		EAGAIN		                                    /* Operation would block        */
#define	ENOSYS			71		                                        /* Function not implemented     */
#define ENOIOCTLCMD     ENOSYS

/*********************************************************************************************************
   aio errors (should be under posix)
*********************************************************************************************************/

#define	ECANCELED		72		                                        /* Operation canceled           */

/*********************************************************************************************************
   specific STREAMS errno values
*********************************************************************************************************/

#define ENOSR           74                                              /* Insufficient memory          */
#define ENOSTR          75                                              /* STREAMS device required      */
#define EPROTO          76                                              /* Generic STREAMS error        */
#define EBADMSG         77                                              /* Invalid STREAMS message      */
#define ENODATA         78                                              /* Missing expected message data*/
#define ETIME           79                                              /* STREAMS timeout occurred     */
#define ENOMSG          80                                              /* Unexpected message type      */ 

/*********************************************************************************************************
   io errors (should be under posix)
*********************************************************************************************************/

#define EWRPROTECT      90
#define EFORMAT         91

/*********************************************************************************************************
   io errors (should be under posix)
*********************************************************************************************************/

#define EIDRM           92                                              /* Identifier removed           */
#define ECHRNG          93                                              /* Channel number out of range  */
#define EL2NSYNC        94                                              /* Level 2 not synchronized     */
#define EL3HLT          95                                              /* Level 3 halted               */
#define EL3RST          96                                              /* Level 3 reset                */
#define ELNRNG          97                                              /* Link number out of range     */
#define EUNATCH         98                                              /* Protocol driver not attached */
#define ENOCSI          99                                              /* No CSI structure available   */
#define EL2HLT          100                                             /* Level 2 halted               */
#define EBADE           101                                             /* Invalid exchange             */
#define EBADR           102                                             /* Invalid request descriptor   */
#define EXFULL          103                                             /* Exchange full                */
#define ENOANO          104                                             /* No anode                     */
#define EBADRQC         105                                             /* Invalid request code         */
#define EBADSLT         106                                             /* Invalid slot                 */

#define EDEADLOCK       EDEADLK

#define EBFONT          107                                             /* Bad font file format         */
#define ENONET          108                                             /* Machine is not on the network*/
#define ENOPKG          109                                             /* Package not installed        */
#define EREMOTE         110                                             /* Object is remote             */
#define ENOLINK         111                                             /* Link has been severed        */
#define EADV            112                                             /* Advertise error              */
#define ESRMNT          113                                             /* Srmount error                */
#define ECOMM           114                                             /* Communication error on send  */
#define EMULTIHOP       115                                             /* Multihop attempted           */
#define EDOTDOT         116                                             /* RFS specific error           */
#define EUCLEAN         117                                             /* Structure needs cleaning     */
#define ENOTUNIQ        118                                             /* Name not unique on network   */
#define EBADFD          119                                             /* File descriptor in bad state */
#define EREMCHG         120                                             /* Remote address changed       */
#define ELIBACC         121                                             /* Can not access a needed      */
                                                                        /* shared library               */
#define ELIBBAD         122                                             /* Accessing a corrupted shared */
                                                                        /* library                      */
#define ELIBSCN         123                                             /* .lib section in a.out        */
                                                                        /* corrupted                    */
#define ELIBMAX         124                                             /* Attempting to link in too    */
                                                                        /* many shared libraries        */
#define ELIBEXEC        125                                             /* Cannot exec a shared library */
                                                                        /* directly                     */
#define ERESTART        126                                             /* Interrupted system call      */
                                                                        /* should be restarted          */
#define ESTRPIPE        127                                             /* Streams pipe error           */
#define EUSERS          128                                             /* Too many users               */
#define ESTALE          129                                             /* Stale NFS file handle        */
#define ENOTNAM         130                                             /* Not a XENIX named type file  */
#define ENAVAIL         131                                             /* No XENIX semaphores available*/
#define EISNAM          132                                             /* Is a named type file         */
#define EREMOTEIO       133                                             /* Remote I/O error             */
#define EDQUOT          134                                             /* Quota exceeded               */
#define ENOMEDIUM       135                                             /* No medium found              */
#define EMEDIUMTYPE     136                                             /* Wrong medium type            */
#define EILSEQ          138

/*********************************************************************************************************
   For robust mutexes
*********************************************************************************************************/

#define EOWNERDEAD      140                                             /* Owner died                   */
#define ENOTRECOVERABLE 141                                             /* State not recoverable        */

/*********************************************************************************************************
   GJB7714 extern
*********************************************************************************************************/
#if LW_CFG_GJB7714_EN > 0

#define ECALLEDINISR    ERROR_KERNEL_IN_ISR                             /*  called in isr routine       */
#define EMNOTINITED     EINVAL                                          /*  module not inited           */

#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */
/*********************************************************************************************************
   system errno max
*********************************************************************************************************/

#define	ERRMAX			200000

#endif                                                                  /*  __K_PERROR_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
