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
# 文   件   名: libbitcoin_util.mk
#
# 创   建   人: RealEvo-IDE
#
# 文件创建日期: 2017 年 12 月 07 日
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
LOCAL_TARGET_NAME := libbitcoin_util.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS :=  \
src/chainparamsbase.cpp \
src/clientversion.cpp \
src/compat/glibcxx_sanity.cpp \
src/compat/glibc_compat.cpp \
src/compat/glibc_sanity.cpp \
src/compat/strnlen.cpp \
src/random.cpp \
src/rpc/protocol.cpp \
src/support/cleanse.cpp \
src/support/lockedpool.cpp \
src/sync.cpp \
src/threadinterrupt.cpp \
src/util.cpp \
src/utilmoneystr.cpp \
src/utilstrencodings.cpp \
src/utiltime.cpp

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH := \
-I"./src/include" \
-I"./src/" \
-I"./src/univalue/include" \
-I"./src/compat" \
-I"./src/config" \
-I"./src/cpp-ethereum" \
-I"$(WORKSPACE_sylixos-base)/openssl/openssl/include" \
-I"$(WORKSPACE_libboost)/src" 

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := \
-DHAVE_CONFIG_H \
-D_FORTIFY_SOURCE=2 \
-DQTUM_BUILD \
-D_FILE_OFFSET_BITS=64

#*********************************************************************************************************
# Compiler flags
#*********************************************************************************************************
LOCAL_CFLAGS   := 
LOCAL_CXXFLAGS := -fexceptions -std=c++11 -frtti

#*********************************************************************************************************
# Depend library (eg. LOCAL_DEPEND_LIB := -la LOCAL_DEPEND_LIB_PATH := -L"Your library search path")
#*********************************************************************************************************
LOCAL_DEPEND_LIB := \
-lboost_filesystem \
-lboost_system     \
-lboost_thread-mt-sd  \
-lboost_program_options-d \
-lunivalue \
-lcextern \
-lssl

LOCAL_DEPEND_LIB_PATH := \
-L"./$(Output)" \
-L"$(WORKSPACE_sylixos-base)/openssl/$(Output)" \
-L"$(WORKSPACE_libboost)/$(Output)"

#*********************************************************************************************************
# Depend target
#*********************************************************************************************************
LOCAL_DEPEND_TARGET := $(WORKSPACE_libboost)/$(Output)/libboost_filesystem.so
LOCAL_DEPEND_TARGET += $(WORKSPACE_libboost)/$(Output)/libboost_system.so
LOCAL_DEPEND_TARGET += $(WORKSPACE_libboost)/$(Output)/libboost_thread-mt-sd.so
LOCAL_DEPEND_TARGET += $(WORKSPACE_libboost)/$(Output)/libboost_program_options-d.so
LOCAL_DEPEND_TARGET += $(WORKSPACE_sylixos-base)/openssl/$(Output)/libssl.so
LOCAL_DEPEND_TARGET += ./$(Output)/libunivalue.so

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
