/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Enable better support for exporting from AFS */
/* #undef AFS_SUPPORT */

/* Define to 1 if the system has the type `int32'. */
/* #undef HAVE_INT32 */

/* Define to 1 if the system has the type `int64'. */
/* #undef HAVE_INT64 */

/* Define to 1 if you have the `lchown' function. */
/* #undef HAVE_LCHOWN */

/* Define to 1 if you have the <linux/ext2_fs.h> header file. */
/* #undef HAVE_LINUX_EXT2_FS_H */

/* Define to 1 if you have the <mntent.h> header file. */
/* #undef HAVE_MNTENT_H */

/* Define to 1 if you have the <rpc/svc_soc.h> header file. */
/* #undef HAVE_RPC_SVC_SOC_H */

/* Define to 1 if you have the `setegid' function. */
#define HAVE_SETEGID 1

/* Define to 1 if you have the `seteuid' function. */
#define HAVE_SETEUID 1

/* Define to 1 if you have the `setgroups' function. */
#define HAVE_SETGROUPS 1

/* Define to 1 if you have the `setresgid' function. */
#define HAVE_SETRESGID 1

/* Define to 1 if you have the `setresuid' function. */
#define HAVE_SETRESUID 1

/* Define to 1 if you have the `statvfs' function. */
#define HAVE_STATVFS 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if `st_gen' is a member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_GEN */

/* Define to 1 if `xp_fd' is a member of `struct __rpc_svcxprt'. */
/* #undef HAVE_STRUCT___RPC_SVCXPRT_XP_FD */

/* Define to 1 if you have the `svc_getreq_poll' function. */
#define HAVE_SVC_GETREQ_POLL 1

/* Define to 1 if you have the <sys/mnttab.h> header file. */
/* #undef HAVE_SYS_MNTTAB_H */

/* Define to 1 if you have the <sys/mount.h> header file. */
#define HAVE_SYS_MOUNT_H 1

/* Define to 1 if you have the <sys/vmount.h> header file. */
/* #undef HAVE_SYS_VMOUNT_H */

/* Define to 1 if the system has the type `uint32'. */
/* #undef HAVE_UINT32 */

/* Define to 1 if the system has the type `uint64'. */
/* #undef HAVE_UINT64 */

/* Define to 1 if you have the `vsyslog' function. */
/* #undef HAVE_VSYSLOG */

/* Define to 1 if you have the `xdr_int' function. */
#define HAVE_XDR_INT 1

/* Define to 1 if you have the `xdr_int32' function. */
/* #undef HAVE_XDR_INT32 */

/* Define to 1 if you have the `xdr_int32_t' function. */
#define HAVE_XDR_INT32_T 1

/* Define to 1 if you have the `xdr_uint32' function. */
/* #undef HAVE_XDR_UINT32 */

/* Define to 1 if you have the `xdr_uint32_t' function. */
#define HAVE_XDR_UINT32_T 1

/* Define to 1 if you have the `xdr_uint64' function. */
/* #undef HAVE_XDR_UINT64 */

/* Define to 1 if you have the `xdr_uint64_t' function. */
#define HAVE_XDR_UINT64_T 1

/* Define to 1 if you have the `xdr_u_int' function. */
#define HAVE_XDR_U_INT 1

/* Define to 1 if you have the `xdr_u_int32_t' function. */
/* #undef HAVE_XDR_U_INT32_T */

/* Define to 1 if you have the `xdr_u_int64_t' function. */
/* #undef HAVE_XDR_U_INT64_T */

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "unfs3"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "unfs3 0.9.23"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "unfs3"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.9.23"

/* Define to an empty value if you use Solaris. */
#define PORTMAP /**/

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Cluster extensions */
/* #undef WANT_CLUSTER */

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
#define YYTEXT_POINTER 1

/* Number of bits in a file offset, on hosts where this is settable. */
#define _FILE_OFFSET_BITS 64

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/*
 * Default configuration
 */
#define UNFSD_CFG_DEF_IPNET0 "127.0.0.0/255.255.255.0"
#define UNFSD_CFG_DEF_OPT   "rw"
#define UNFSD_CFG_DEF_ITEM  "/media"
