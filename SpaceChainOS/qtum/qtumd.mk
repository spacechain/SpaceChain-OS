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
# 文   件   名: qtumd.mk
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
LOCAL_TARGET_NAME := qtumd

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS :=  \
src/bitcoind.cpp

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH :=  \
-I"./src\include" \
-I"./src" \
-I"./src/config" \
-I"./src/compat" \
-I"./src/cpp-ethereum" \
-I"./src/univalue/include" \
-I"$(WORKSPACE_libboost)/src" 

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := -DHAVE_CONFIG_H

#*********************************************************************************************************
# Compiler flags
#*********************************************************************************************************
LOCAL_CFLAGS := 
LOCAL_CXXFLAGS := -std=c++11 -fexceptions -frtti

#*********************************************************************************************************
# Depend library (eg. LOCAL_DEPEND_LIB := -la LOCAL_DEPEND_LIB_PATH := -L"Your library search path")
#*********************************************************************************************************
LOCAL_DEPEND_LIB := \
-Wl,-Bstatic \
-lbitcoin_server \
-lbitcoin_common \
-lunivalue \
-lbitcoin_util \
-lbitcoin_consensus \
-lbitcoin_wallet \
-lbitcoin_crypto \
-lleveldb \
-lmemenv \
-lcryptopp \
-lsecp256k1 \
-lboost_system \
-lboost_filesystem \
-lboost_program_options-d \
-lboost_thread-mt-sd \
-lboost_chrono-mt-s \
-lboost_random-mt-s \
-Wl,-Bdynamic \
-ldb \
-lssl \
-lcrypto \
-levent \
-levent_pthreads 

LOCAL_DEPEND_LIB_PATH := \
-L"$(WORKSPACE_qtum)/$(Output)" \
-L"$(WORKSPACE_libevent)/$(Output)" \
-L"$(WORKSPACE_libboost)/$(Output)" \
-L"$(WORKSPACE_libdb)/$(Output)" \
-L"$(WORKSPACE_sylixos-base)/openssl/$(Output)" 

#*********************************************************************************************************
# Depend target
#*********************************************************************************************************
LOCAL_DEPEND_TARGET := ./$(Output)/libsecp256k1.a
LOCAL_DEPEND_TARGET += ./$(Output)/libcryptopp.a
LOCAL_DEPEND_TARGET += ./$(Output)/libmemenv.a
LOCAL_DEPEND_TARGET += ./$(Output)/libleveldb.a
LOCAL_DEPEND_TARGET += ./$(Output)/libbitcoin_server.a
LOCAL_DEPEND_TARGET += ./$(Output)/libbitcoin_util.a
LOCAL_DEPEND_TARGET += ./$(Output)/libbitcoin_consensus.a
LOCAL_DEPEND_TARGET += ./$(Output)/libbitcoin_wallet.a
LOCAL_DEPEND_TARGET += ./$(Output)/libbitcoin_crypto.a
LOCAL_DEPEND_TARGET += ./$(Output)/libunivalue.a
LOCAL_DEPEND_TARGET += $(WORKSPACE_libboost)/$(Output)/libboost_system.a
LOCAL_DEPEND_TARGET += $(WORKSPACE_libboost)/$(Output)/libboost_filesystem.a
LOCAL_DEPEND_TARGET += $(WORKSPACE_libboost)/$(Output)/libboost_program_options-d.a
LOCAL_DEPEND_TARGET += $(WORKSPACE_libboost)/$(Output)/libboost_thread-mt-sd.a
LOCAL_DEPEND_TARGET += $(WORKSPACE_libboost)/$(Output)/libboost_chrono-mt-s.a
LOCAL_DEPEND_TARGET += $(WORKSPACE_libboost)/$(Output)/libboost_random-mt-s.a
LOCAL_DEPEND_TARGET += $(WORKSPACE_libdb)/$(Output)/libdb.so
LOCAL_DEPEND_TARGET += $(WORKSPACE_libevent)/$(Output)/libevent.so
LOCAL_DEPEND_TARGET += $(WORKSPACE_libevent)/$(Output)/libevent_pthreads.so
LOCAL_DEPEND_TARGET += $(WORKSPACE_sylixos-base)/openssl/$(Output)/libssl.so
LOCAL_DEPEND_TARGET += $(WORKSPACE_sylixos-base)/openssl/$(Output)/libcrypto.so

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
LOCAL_PRE_LINK_CMD := 
LOCAL_POST_LINK_CMD := 
LOCAL_PRE_STRIP_CMD := 
LOCAL_POST_STRIP_CMD := 

include $(APPLICATION_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
