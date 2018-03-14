#*********************************************************************************************************
#
#                                    中国软件开源组织
#
#                                   嵌入式实时操作系统
#
#                                SylixOS(TM)  LW : long wing
#
#                               Copyright All Rights Reserved
#
#--------------文件信息--------------------------------------------------------------------------------
#
# 文   件   名: libcextern.mk
#
# 创   建   人: RealEvo-IDE
#
# 文件创建日期: 2016 年 10 月 08 日
#
# 描        述: 本文件由 RealEvo-IDE 生成，用于配置 Makefile 功能，请勿手动修改
#*********************************************************************************************************

#*********************************************************************************************************
# Clear setting
#*********************************************************************************************************
include $(CLEAR_VARS_MK)

#*********************************************************************************************************
# Target
#*********************************************************************************************************
LOCAL_TARGET_NAME := libcextern.so

#*********************************************************************************************************
# BSD fixed src(s) file
#*********************************************************************************************************
BSD_SRCS = \
libcextern/bsdfix/locale_internal.c

#*********************************************************************************************************
# aio src(s) file
#*********************************************************************************************************
AIO_SRCS = \
libcextern/aio/aio.c \
libcextern/aio/aio_lib.c

#*********************************************************************************************************
# ctype src(s) file
#*********************************************************************************************************
CTYPE_SRCS = \
libcextern/ctype/ctype.c 

#*********************************************************************************************************
# citrus src(s) file
#*********************************************************************************************************
CITRUS_SRCS = \
libcextern/citrus/citrus_bcs.c \
libcextern/citrus/citrus_bcs_strtol.c \
libcextern/citrus/citrus_bcs_strtoul.c \
libcextern/citrus/citrus_csmapper.c \
libcextern/citrus/citrus_ctype.c \
libcextern/citrus/citrus_ctype_fallback.c \
libcextern/citrus/citrus_db.c \
libcextern/citrus/citrus_db_factory.c \
libcextern/citrus/citrus_db_hash.c \
libcextern/citrus/citrus_esdb.c \
libcextern/citrus/citrus_hash.c \
libcextern/citrus/citrus_iconv.c \
libcextern/citrus/citrus_lc_ctype.c \
libcextern/citrus/citrus_lc_messages.c \
libcextern/citrus/citrus_lc_monetary.c \
libcextern/citrus/citrus_lc_numeric.c \
libcextern/citrus/citrus_lc_time.c \
libcextern/citrus/citrus_lookup.c \
libcextern/citrus/citrus_lookup_factory.c \
libcextern/citrus/citrus_mapper.c \
libcextern/citrus/citrus_memstream.c \
libcextern/citrus/citrus_mmap.c \
libcextern/citrus/citrus_module.c \
libcextern/citrus/citrus_none.c \
libcextern/citrus/citrus_pivot_factory.c \
libcextern/citrus/citrus_prop.c \
libcextern/citrus/citrus_stdenc.c \
libcextern/citrus/modules/citrus_big5.c \
libcextern/citrus/modules/citrus_dechanyu.c \
libcextern/citrus/modules/citrus_euc.c \
libcextern/citrus/modules/citrus_euctw.c \
libcextern/citrus/modules/citrus_gbk2k.c \
libcextern/citrus/modules/citrus_hz.c \
libcextern/citrus/modules/citrus_iconv_none.c \
libcextern/citrus/modules/citrus_iconv_std.c \
libcextern/citrus/modules/citrus_iso2022.c \
libcextern/citrus/modules/citrus_johab.c \
libcextern/citrus/modules/citrus_mapper_646.c \
libcextern/citrus/modules/citrus_mapper_none.c \
libcextern/citrus/modules/citrus_mapper_serial.c \
libcextern/citrus/modules/citrus_mapper_std.c \
libcextern/citrus/modules/citrus_mapper_zone.c \
libcextern/citrus/modules/citrus_mskanji.c \
libcextern/citrus/modules/citrus_ues.c \
libcextern/citrus/modules/citrus_utf1632.c \
libcextern/citrus/modules/citrus_utf7.c \
libcextern/citrus/modules/citrus_utf8.c \
libcextern/citrus/modules/citrus_viqr.c \
libcextern/citrus/modules/citrus_zw.c

#*********************************************************************************************************
# gen src(s) file
#*********************************************************************************************************
GEN_SRCS = \
libcextern/gen/basename.c \
libcextern/gen/dirname.c \
libcextern/gen/fts.c \
libcextern/gen/ftw.c \
libcextern/gen/nftw.c \
libcextern/gen/getcap.c \
libcextern/gen/getpwent.c \
libcextern/gen/getshadow.c \
libcextern/gen/ulimit.c \
libcextern/gen/utmp.c \
libcextern/gen/utmpx.c \
libcextern/gen/vis.c

#*********************************************************************************************************
# locale src(s) file
#*********************************************************************************************************
LOCALE_SRCS = \
libcextern/locale/aliasname.c \
libcextern/locale/bsdctype.c \
libcextern/locale/ctypeio.c \
libcextern/locale/current_locale.c \
libcextern/locale/dummy_lc_collate.c \
libcextern/locale/fix_grouping.c \
libcextern/locale/generic_lc_all.c \
libcextern/locale/global_locale.c \
libcextern/locale/iswctype_mb.c \
libcextern/locale/localeconv.c \
libcextern/locale/localeio.c \
libcextern/locale/localeio_lc_ctype.c \
libcextern/locale/localeio_lc_messages.c \
libcextern/locale/localeio_lc_monetary.c \
libcextern/locale/localeio_lc_numeric.c \
libcextern/locale/localeio_lc_time.c \
libcextern/locale/multibyte_amd1.c \
libcextern/locale/multibyte_c90.c \
libcextern/locale/nl_langinfo.c \
libcextern/locale/rune.c \
libcextern/locale/runeglue.c \
libcextern/locale/runetable.c \
libcextern/locale/setlocale.c \
libcextern/locale/setlocale1.c \
libcextern/locale/setlocale32.c \
libcextern/locale/wcscoll.c \
libcextern/locale/wcsftime.c \
libcextern/locale/wcstod.c \
libcextern/locale/wcstof.c \
libcextern/locale/wcstoimax.c \
libcextern/locale/wcstol.c \
libcextern/locale/wcstold.c \
libcextern/locale/wcstoll.c \
libcextern/locale/wcstoul.c \
libcextern/locale/wcstoull.c \
libcextern/locale/wcstoumax.c \
libcextern/locale/wcsxfrm.c \
libcextern/locale/_def_messages.c \
libcextern/locale/_def_monetary.c \
libcextern/locale/_def_numeric.c \
libcextern/locale/_def_time.c \
libcextern/locale/_wctrans.c \
libcextern/locale/_wctype.c \
libcextern/locale/__mb_cur_max.c

#*********************************************************************************************************
# iconv src(s) file
#*********************************************************************************************************
ICONV_SRCS = \
libcextern/iconv/iconv.c

#*********************************************************************************************************
# database src(s) file
#*********************************************************************************************************
NDBM_SRCS = \
libcextern/ndbm/btree/bt_close.c \
libcextern/ndbm/btree/bt_conv.c \
libcextern/ndbm/btree/bt_debug.c \
libcextern/ndbm/btree/bt_delete.c \
libcextern/ndbm/btree/bt_get.c \
libcextern/ndbm/btree/bt_open.c \
libcextern/ndbm/btree/bt_overflow.c \
libcextern/ndbm/btree/bt_page.c \
libcextern/ndbm/btree/bt_put.c \
libcextern/ndbm/btree/bt_search.c \
libcextern/ndbm/btree/bt_seq.c \
libcextern/ndbm/btree/bt_split.c \
libcextern/ndbm/btree/bt_utils.c \
libcextern/ndbm/db/db.c \
libcextern/ndbm/hash/hash.c \
libcextern/ndbm/hash/hash_bigkey.c \
libcextern/ndbm/hash/hash_buf.c \
libcextern/ndbm/hash/hash_func.c \
libcextern/ndbm/hash/hash_log2.c \
libcextern/ndbm/hash/hash_page.c \
libcextern/ndbm/hash/ndbm.c \
libcextern/ndbm/hash/ndbmdatum.c \
libcextern/ndbm/mpool/mpool.c \
libcextern/ndbm/recno/rec_close.c \
libcextern/ndbm/recno/rec_delete.c \
libcextern/ndbm/recno/rec_get.c \
libcextern/ndbm/recno/rec_open.c \
libcextern/ndbm/recno/rec_put.c \
libcextern/ndbm/recno/rec_search.c \
libcextern/ndbm/recno/rec_seq.c \
libcextern/ndbm/recno/rec_utils.c

#*********************************************************************************************************
# regex src(s) file
#*********************************************************************************************************
REGEX_SRCS = \
libcextern/regex/regcomp.c \
libcextern/regex/regerror.c \
libcextern/regex/regexec.c \
libcextern/regex/regfree.c

#*********************************************************************************************************
# inet(libbind) src(s) file
#*********************************************************************************************************
INET_SRCS = \
libcextern/inet/nsap_addr.c

#*********************************************************************************************************
# isc(libbind) src(s) file
#*********************************************************************************************************
ISC_SRCS = \
libcextern/isc/assertions.c \
libcextern/isc/eventlib.c \
libcextern/isc/ev_connects.c \
libcextern/isc/ev_files.c \
libcextern/isc/ev_streams.c \
libcextern/isc/ev_timers.c \
libcextern/isc/heap.c \
libcextern/isc/memcluster.c

#*********************************************************************************************************
# dst(libbind) src(s) file
#*********************************************************************************************************
DST_SRCS = \
libcextern/dst/dst_api.c \
libcextern/dst/hmac_link.c \
libcextern/dst/md5_dgst.c \
libcextern/dst/support.c

#*********************************************************************************************************
# nameser src(s) file
#*********************************************************************************************************
NAMESER_SRCS = \
libcextern/nameser/ns_name.c \
libcextern/nameser/ns_netint.c \
libcextern/nameser/ns_parse.c \
libcextern/nameser/ns_print.c \
libcextern/nameser/ns_samedomain.c \
libcextern/nameser/ns_ttl.c

#*********************************************************************************************************
# resolv src(s) file
#*********************************************************************************************************
RESOLV_SRCS = \
libcextern/resolv/herror.c \
libcextern/resolv/h_errno.c \
libcextern/resolv/mtctxres.c \
libcextern/resolv/res_comp.c \
libcextern/resolv/res_compat.c \
libcextern/resolv/res_data.c \
libcextern/resolv/res_debug.c \
libcextern/resolv/res_init.c \
libcextern/resolv/res_mkquery.c \
libcextern/resolv/res_query.c \
libcextern/resolv/res_send.c \
libcextern/resolv/res_state.c \
libcextern/resolv/__dn_comp.c \
libcextern/resolv/__res_close.c \
libcextern/resolv/__res_send.c

#*********************************************************************************************************
# stdlib src(s) file
#*********************************************************************************************************
STDLIB_SRCS = \
libcextern/stdlib/insque.c \
libcextern/stdlib/remque.c \
libcextern/stdlib/tdelete.c \
libcextern/stdlib/tfind.c \
libcextern/stdlib/twalk.c \
libcextern/stdlib/tsearch.c \
libcextern/stdlib/lsearch.c \
libcextern/stdlib/search.c \
libcextern/stdlib/strfmon.c

#*********************************************************************************************************
# net src(s) file
#*********************************************************************************************************
NET_SRCS = \
libcextern/net/base64.c \
libcextern/net/fparseln.c \
libcextern/net/getaddrinfo.c \
libcextern/net/gethnamaddr.c \
libcextern/net/getifaddrs.c \
libcextern/net/getnameinfo.c \
libcextern/net/getprotobyname_r.c \
libcextern/net/getprotobyname.c \
libcextern/net/getprotobynumber_r.c \
libcextern/net/getprotobynumber.c \
libcextern/net/getprotoent_r.c \
libcextern/net/getprotoent.c \
libcextern/net/getservbyname_r.c \
libcextern/net/getservbyname.c \
libcextern/net/getservbyport_r.c \
libcextern/net/getservbyport.c \
libcextern/net/getservent_r.c \
libcextern/net/getservent.c \
libcextern/net/hesiod.c \
libcextern/net/linkaddr.c \
libcextern/net/nsdispatch.c \
libcextern/net/nslexer.c \
libcextern/net/nsparser.c \
libcextern/net/netpatch.c

#*********************************************************************************************************
# stdlib src(s) file
#*********************************************************************************************************
NLS_SRCS = \
libcextern/nls/catclose.c \
libcextern/nls/catgets.c \
libcextern/nls/catopen.c \
libcextern/nls/_catclose.c \
libcextern/nls/_catgets.c \
libcextern/nls/_catopen.c

#*********************************************************************************************************
# libterm src(s) file
#*********************************************************************************************************
TERM_SRCS = \
libcextern/term/termcap.c \
libcextern/term/tgoto.c \
libcextern/term/tputs.c \
libcextern/term/tputws.c

#*********************************************************************************************************
# uuid src(s) file
#*********************************************************************************************************
UUID_SRCS = \
libcextern/uuid/uuid_compare.c \
libcextern/uuid/uuid_create_nil.c \
libcextern/uuid/uuid_create.c \
libcextern/uuid/uuid_equal.c \
libcextern/uuid/uuid_from_string.c \
libcextern/uuid/uuid_hash.c \
libcextern/uuid/uuid_is_nil.c \
libcextern/uuid/uuid_stream.c \
libcextern/uuid/uuid_to_string.c

#*********************************************************************************************************
# version src(s) file
#*********************************************************************************************************
VERSION_SRCS = \
libcextern/likely.c \
libcextern/version.c

#*********************************************************************************************************
# All libcextern source
#*********************************************************************************************************
LOCAL_SRCS :=
LOCAL_SRCS += $(BSD_SRCS)
LOCAL_SRCS += $(AIO_SRCS)
LOCAL_SRCS += $(CTYPE_SRCS)
LOCAL_SRCS += $(CITRUS_SRCS)
LOCAL_SRCS += $(GEN_SRCS)
LOCAL_SRCS += $(LOCALE_SRCS)
LOCAL_SRCS += $(ICONV_SRCS)
LOCAL_SRCS += $(NDBM_SRCS)
LOCAL_SRCS += $(REGEX_SRCS)
LOCAL_SRCS += $(INET_SRCS)
LOCAL_SRCS += $(ISC_SRCS)
LOCAL_SRCS += $(DST_SRCS)
LOCAL_SRCS += $(NAMESER_SRCS)
LOCAL_SRCS += $(RESOLV_SRCS)
LOCAL_SRCS += $(STDLIB_SRCS)
LOCAL_SRCS += $(NET_SRCS)
LOCAL_SRCS += $(NLS_SRCS)
LOCAL_SRCS += $(TERM_SRCS)
LOCAL_SRCS += $(UUID_SRCS)
LOCAL_SRCS += $(VERSION_SRCS)

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your hearder files search path")
#*********************************************************************************************************
LOCAL_INC_PATH := 
LOCAL_INC_PATH += -I"./libcextern/include"
LOCAL_INC_PATH += -I"./libcextern/bsdfix"
LOCAL_INC_PATH += -I"./libcextern/interinc"
LOCAL_INC_PATH += -I"./libcextern/locale"
LOCAL_INC_PATH += -I"./libcextern/citrus"
LOCAL_INC_PATH += -I"./libcextern/bsdsrc/iconv"

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := 
LOCAL_DSYMBOL += -DBSD=200804# Compatibility of BSD version
LOCAL_DSYMBOL += -DWITH_RUNE# With full local function
LOCAL_DSYMBOL += -D_REENTRANT# Thread safe
LOCAL_DSYMBOL += -D_DIAGASSERT=assert# Assert function
LOCAL_DSYMBOL += -DINET6# We support IPv6 protocol
LOCAL_DSYMBOL += -D_NETBSD_SOURCE# We compatibility with NetBSD
LOCAL_DSYMBOL += -D__DBINTERFACE_PRIVATE# Compile internal DB source
LOCAL_DSYMBOL += -D__RENAME=__FBSDID
LOCAL_DSYMBOL += -D__RCSID=__FBSDID

#*********************************************************************************************************
# Depend library (eg. LOCAL_DEPEND_LIB := -la LOCAL_DEPEND_LIB_PATH := -L"Your library search path")
#*********************************************************************************************************
LOCAL_DEPEND_LIB      := 
LOCAL_DEPEND_LIB_PATH := 

#*********************************************************************************************************
# C++ config
#*********************************************************************************************************
LOCAL_USE_CXX        := no
LOCAL_USE_CXX_EXCEPT := no

#*********************************************************************************************************
# Code coverage config
#*********************************************************************************************************
LOCAL_USE_GCOV := no

include $(LIBRARY_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
