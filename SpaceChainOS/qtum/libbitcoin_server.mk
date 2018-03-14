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
LOCAL_TARGET_NAME := libbitcoin_server.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS :=  \
src/addrdb.cpp \
src/addrman.cpp \
src/blockencodings.cpp \
src/bloom.cpp \
src/chain.cpp \
src/checkpoints.cpp \
src/consensus/consensus.cpp \
src/dbwrapper.cpp \
src/httprpc.cpp \
src/httpserver.cpp \
src/init.cpp \
src/merkleblock.cpp \
src/miner.cpp \
src/net.cpp \
src/net_processing.cpp \
src/noui.cpp \
src/policy/fees.cpp \
src/policy/policy.cpp \
src/pos.cpp \
src/pow.cpp \
src/qtum/qtumDGP.cpp \
src/qtum/qtumstate.cpp \
src/qtum/qtumtransaction.cpp \
src/qtum/storageresults.cpp \
src/rest.cpp \
src/rpc/blockchain.cpp \
src/rpc/mining.cpp \
src/rpc/misc.cpp \
src/rpc/net.cpp \
src/rpc/rawtransaction.cpp \
src/rpc/server.cpp \
src/script/ismine.cpp \
src/script/sigcache.cpp \
src/timedata.cpp \
src/torcontrol.cpp \
src/txdb.cpp \
src/txmempool.cpp \
src/ui_interface.cpp \
src/validation.cpp \
src/validationinterface.cpp \
src/versionbits.cpp

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH := \
-I"$(WORKSPACE_qtum)/src/compat" \
-I"$(WORKSPACE_qtum)/src/config" \
-I"$(WORKSPACE_qtum)/src/" \
-I"$(WORKSPACE_qtum)/src/qtum/" \
-I"$(WORKSPACE_qtum)/src/cpp-ethereum" \
-I"$(WORKSPACE_qtum)/src/leveldb/include" \
-I"$(WORKSPACE_qtum)/src/univalue/include" \
-I"$(WORKSPACE_qtum)/src/leveldb/helpers/memenv" \
-I"$(WORKSPACE_qtum)/src/include" \
-I"$(WORKSPACE_libdb)/src/config" \
-I"$(WORKSPACE_libevent)/src/include" \
-I"$(WORKSPACE_sylixos-base)/openssl/openssl/include" \
-I"$(WORKSPACE_libboost)/src" 

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := \
-DHAVE_CONFIG_H \
-D_FORTIFY_SOURCE=2 \
-DQTUM_BUILD \
-D__STDC_FORMAT_MACROS \
-D_FILE_OFFSET_BITS=64

#*********************************************************************************************************
# Compiler flags
#*********************************************************************************************************
LOCAL_CFLAGS   := 
LOCAL_CXXFLAGS :=-fexceptions -std=c++11  -frtti

#*********************************************************************************************************
# Depend library (eg. LOCAL_DEPEND_LIB := -la LOCAL_DEPEND_LIB_PATH := -L"Your library search path")
#*********************************************************************************************************
LOCAL_DEPEND_LIB := \
-lbitcoin_consensus \
-lbitcoin_common \
-lcextern 

LOCAL_DEPEND_LIB_PATH := \
-L"./$(Output)"

#*********************************************************************************************************
# Depend target
#*********************************************************************************************************
LOCAL_DEPEND_TARGET := ./$(Output)/libbitcoin_consensus.so
LOCAL_DEPEND_TARGET += ./$(Output)/libbitcoin_common.so

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
