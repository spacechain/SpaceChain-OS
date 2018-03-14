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
# 文   件   名: libdb.mk
#
# 创   建   人: RealEvo-IDE
#
# 文件创建日期: 2017 年 12 月 06 日
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
LOCAL_TARGET_NAME := libdb.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS :=  \
src/btree/btree_auto.c \
src/btree/btree_autop.c \
src/btree/bt_compact.c \
src/btree/bt_compare.c \
src/btree/bt_compress.c \
src/btree/bt_conv.c \
src/btree/bt_curadj.c \
src/btree/bt_cursor.c \
src/btree/bt_delete.c \
src/btree/bt_method.c \
src/btree/bt_open.c \
src/btree/bt_put.c \
src/btree/bt_rec.c \
src/btree/bt_reclaim.c \
src/btree/bt_recno.c \
src/btree/bt_rsearch.c \
src/btree/bt_search.c \
src/btree/bt_split.c \
src/btree/bt_stat.c \
src/btree/bt_upgrade.c \
src/btree/bt_verify.c \
src/common/dbt.c \
src/common/db_byteorder.c \
src/common/db_compint.c \
src/common/db_err.c \
src/common/db_getlong.c \
src/common/db_idspace.c \
src/common/db_log2.c \
src/common/db_shash.c \
src/common/mkpath.c \
src/common/openflags.c \
src/common/os_method.c \
src/common/util_arg.c \
src/common/util_cache.c \
src/common/util_log.c \
src/common/util_sig.c \
src/common/zerofill.c \
src/crypto/aes_method.c \
src/crypto/crypto.c \
src/crypto/mersenne/mt19937db.c \
src/crypto/rijndael/rijndael-alg-fst.c \
src/crypto/rijndael/rijndael-api-fst.c \
src/cxx/cxx_db.cpp \
src/cxx/cxx_dbc.cpp \
src/cxx/cxx_dbt.cpp \
src/cxx/cxx_env.cpp \
src/cxx/cxx_except.cpp \
src/cxx/cxx_lock.cpp \
src/cxx/cxx_logc.cpp \
src/cxx/cxx_mpool.cpp \
src/cxx/cxx_multi.cpp \
src/cxx/cxx_seq.cpp \
src/cxx/cxx_txn.cpp \
src/db/crdel_auto.c \
src/db/crdel_autop.c \
src/db/crdel_rec.c \
src/db/db.c \
src/db/db_am.c \
src/db/db_auto.c \
src/db/db_autop.c \
src/db/db_cam.c \
src/db/db_cds.c \
src/db/db_conv.c \
src/db/db_dispatch.c \
src/db/db_dup.c \
src/db/db_iface.c \
src/db/db_join.c \
src/db/db_meta.c \
src/db/db_method.c \
src/db/db_open.c \
src/db/db_overflow.c \
src/db/db_ovfl_vrfy.c \
src/db/db_pr.c \
src/db/db_rec.c \
src/db/db_reclaim.c \
src/db/db_remove.c \
src/db/db_rename.c \
src/db/db_ret.c \
src/db/db_setid.c \
src/db/db_setlsn.c \
src/db/db_sort_multiple.c \
src/db/db_stati.c \
src/db/db_truncate.c \
src/db/db_upg.c \
src/db/db_upg_opd.c \
src/db/db_vrfy.c \
src/db/db_vrfyutil.c \
src/db/partition.c \
src/dbm/dbm.c \
src/dbreg/dbreg.c \
src/dbreg/dbreg_auto.c \
src/dbreg/dbreg_autop.c \
src/dbreg/dbreg_rec.c \
src/dbreg/dbreg_stat.c \
src/dbreg/dbreg_util.c \
src/env/env_alloc.c \
src/env/env_config.c \
src/env/env_failchk.c \
src/env/env_file.c \
src/env/env_globals.c \
src/env/env_method.c \
src/env/env_name.c \
src/env/env_open.c \
src/env/env_recover.c \
src/env/env_region.c \
src/env/env_register.c \
src/env/env_sig.c \
src/env/env_stat.c \
src/fileops/fileops_auto.c \
src/fileops/fileops_autop.c \
src/fileops/fop_basic.c \
src/fileops/fop_rec.c \
src/fileops/fop_util.c \
src/hash/hash.c \
src/hash/hash_auto.c \
src/hash/hash_autop.c \
src/hash/hash_conv.c \
src/hash/hash_dup.c \
src/hash/hash_func.c \
src/hash/hash_meta.c \
src/hash/hash_method.c \
src/hash/hash_open.c \
src/hash/hash_page.c \
src/hash/hash_rec.c \
src/hash/hash_reclaim.c \
src/hash/hash_stat.c \
src/hash/hash_upgrade.c \
src/hash/hash_verify.c \
src/hmac/hmac.c \
src/hmac/sha1.c \
src/hsearch/hsearch.c \
src/lock/lock.c \
src/lock/lock_deadlock.c \
src/lock/lock_failchk.c \
src/lock/lock_id.c \
src/lock/lock_list.c \
src/lock/lock_method.c \
src/lock/lock_region.c \
src/lock/lock_stat.c \
src/lock/lock_timer.c \
src/lock/lock_util.c \
src/log/log.c \
src/log/log_archive.c \
src/log/log_compare.c \
src/log/log_debug.c \
src/log/log_get.c \
src/log/log_method.c \
src/log/log_put.c \
src/log/log_stat.c \
src/mp/mp_alloc.c \
src/mp/mp_bh.c \
src/mp/mp_fget.c \
src/mp/mp_fmethod.c \
src/mp/mp_fopen.c \
src/mp/mp_fput.c \
src/mp/mp_fset.c \
src/mp/mp_method.c \
src/mp/mp_mvcc.c \
src/mp/mp_region.c \
src/mp/mp_register.c \
src/mp/mp_resize.c \
src/mp/mp_stat.c \
src/mp/mp_sync.c \
src/mp/mp_trickle.c \
src/mutex/mut_alloc.c \
src/mutex/mut_failchk.c \
src/mutex/mut_fcntl.c \
src/mutex/mut_method.c \
src/mutex/mut_pthread.c \
src/mutex/mut_region.c \
src/mutex/mut_stat.c \
src/os/os_abort.c \
src/os/os_abs.c \
src/os/os_addrinfo.c \
src/os/os_alloc.c \
src/os/os_clock.c \
src/os/os_config.c \
src/os/os_cpu.c \
src/os/os_ctime.c \
src/os/os_dir.c \
src/os/os_errno.c \
src/os/os_fid.c \
src/os/os_flock.c \
src/os/os_fsync.c \
src/os/os_getenv.c \
src/os/os_handle.c \
src/os/os_map.c \
src/os/os_mkdir.c \
src/os/os_open.c \
src/os/os_pid.c \
src/os/os_rename.c \
src/os/os_root.c \
src/os/os_rpath.c \
src/os/os_rw.c \
src/os/os_seek.c \
src/os/os_stack.c \
src/os/os_stat.c \
src/os/os_tmpdir.c \
src/os/os_truncate.c \
src/os/os_uid.c \
src/os/os_unlink.c \
src/os/os_yield.c \
src/qam/qam.c \
src/qam/qam_auto.c \
src/qam/qam_autop.c \
src/qam/qam_conv.c \
src/qam/qam_files.c \
src/qam/qam_method.c \
src/qam/qam_open.c \
src/qam/qam_rec.c \
src/qam/qam_stat.c \
src/qam/qam_upgrade.c \
src/qam/qam_verify.c \
src/rep/rep_auto.c \
src/rep/rep_backup.c \
src/rep/rep_elect.c \
src/rep/rep_lease.c \
src/rep/rep_log.c \
src/rep/rep_method.c \
src/rep/rep_record.c \
src/rep/rep_region.c \
src/rep/rep_stat.c \
src/rep/rep_util.c \
src/rep/rep_verify.c \
src/repmgr/repmgr_auto.c \
src/repmgr/repmgr_elect.c \
src/repmgr/repmgr_method.c \
src/repmgr/repmgr_msg.c \
src/repmgr/repmgr_net.c \
src/repmgr/repmgr_posix.c \
src/repmgr/repmgr_queue.c \
src/repmgr/repmgr_sel.c \
src/repmgr/repmgr_stat.c \
src/repmgr/repmgr_util.c \
src/sequence/sequence.c \
src/sequence/seq_stat.c \
src/txn/txn.c \
src/txn/txn_auto.c \
src/txn/txn_autop.c \
src/txn/txn_chkpt.c \
src/txn/txn_failchk.c \
src/txn/txn_method.c \
src/txn/txn_rec.c \
src/txn/txn_recover.c \
src/txn/txn_region.c \
src/txn/txn_stat.c \
src/txn/txn_util.c

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH :=  \
-I"./src\config" \
-I"./src"

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := 

#*********************************************************************************************************
# Compiler flags
#*********************************************************************************************************
LOCAL_CFLAGS := 
LOCAL_CXXFLAGS := 

#*********************************************************************************************************
# Depend library (eg. LOCAL_DEPEND_LIB := -la LOCAL_DEPEND_LIB_PATH := -L"Your library search path")
#*********************************************************************************************************
LOCAL_DEPEND_LIB := 
LOCAL_DEPEND_LIB_PATH := 

#*********************************************************************************************************
# C++ config
#*********************************************************************************************************
LOCAL_USE_CXX := yes
LOCAL_USE_CXX_EXCEPT := yes

#*********************************************************************************************************
# Code coverage config
#*********************************************************************************************************
LOCAL_USE_GCOV := no

#*********************************************************************************************************
# OpenMP config
#*********************************************************************************************************
LOCAL_USE_OMP := no

#*********************************************************************************************************
# User link command
#*********************************************************************************************************
LOCAL_PRE_LINK_CMD := 
LOCAL_POST_LINK_CMD := 
LOCAL_PRE_STRIP_CMD := 
LOCAL_POST_STRIP_CMD := 

include $(LIBRARY_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
