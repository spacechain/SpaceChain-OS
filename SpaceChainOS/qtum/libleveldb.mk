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
# 文   件   名: libleveldb.mk
#
# 创   建   人: RealEvo-IDE
#
# 文件创建日期: 2017 年 12 月 11 日
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
LOCAL_TARGET_NAME := libleveldb.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS := \
src/leveldb/db/builder.cc \
src/leveldb/db/c.cc \
src/leveldb/db/dbformat.cc \
src/leveldb/db/db_impl.cc \
src/leveldb/db/db_iter.cc \
src/leveldb/db/dumpfile.cc \
src/leveldb/db/filename.cc \
src/leveldb/db/log_reader.cc \
src/leveldb/db/log_writer.cc \
src/leveldb/db/memtable.cc \
src/leveldb/db/repair.cc \
src/leveldb/db/table_cache.cc \
src/leveldb/db/version_edit.cc \
src/leveldb/db/version_set.cc \
src/leveldb/db/write_batch.cc \
src/leveldb/port/port_posix.cc \
src/leveldb/port/port_posix_sse.cc \
src/leveldb/table/block.cc \
src/leveldb/table/block_builder.cc \
src/leveldb/table/filter_block.cc \
src/leveldb/table/format.cc \
src/leveldb/table/iterator.cc \
src/leveldb/table/merger.cc \
src/leveldb/table/table.cc \
src/leveldb/table/table_builder.cc \
src/leveldb/table/two_level_iterator.cc \
src/leveldb/util/arena.cc \
src/leveldb/util/bloom.cc \
src/leveldb/util/cache.cc \
src/leveldb/util/coding.cc \
src/leveldb/util/comparator.cc \
src/leveldb/util/crc32c.cc \
src/leveldb/util/env.cc \
src/leveldb/util/env_posix.cc \
src/leveldb/util/filter_policy.cc \
src/leveldb/util/hash.cc \
src/leveldb/util/histogram.cc \
src/leveldb/util/logging.cc \
src/leveldb/util/options.cc \
src/leveldb/util/status.cc

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH := \
-I"./src/" \
-I"./src/leveldb" \
-I"./src/leveldb/include" \
-I"./src/config"

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := \
-DOS_LINUX \
-DLEVELDB_ATOMIC_PRESENT \
-D__STDC_LIMIT_MACROS  \
-DLEVELDB_PLATFORM_POSIX \
-DHAVE_CONFIG_H \
-D_FORTIFY_SOURCE=2 \
-D__STDC_FORMAT_MACROS \
-D_FILE_OFFSET_BITS=64 \

#*********************************************************************************************************
# Compiler flags
#*********************************************************************************************************
LOCAL_CFLAGS   := 
LOCAL_CXXFLAGS := -std=c++11 -frtti

#*********************************************************************************************************
# Depend library (eg. LOCAL_DEPEND_LIB := -la LOCAL_DEPEND_LIB_PATH := -L"Your library search path")
#*********************************************************************************************************
LOCAL_DEPEND_LIB      := 
LOCAL_DEPEND_LIB_PATH := 

#*********************************************************************************************************
# C++ config
#*********************************************************************************************************
LOCAL_USE_CXX := yes
LOCAL_USE_CXX_EXCEPT := no

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
LOCAL_PRE_LINK_CMD   := 
LOCAL_POST_LINK_CMD  := 
LOCAL_PRE_STRIP_CMD  := 
LOCAL_POST_STRIP_CMD := 

include $(LIBRARY_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
