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
# 文   件   名: libcryptopp.mk
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
LOCAL_TARGET_NAME := libcryptopp.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS := \
src/cryptopp/algebra.cpp \
src/cryptopp/algparam.cpp \
src/cryptopp/asn.cpp \
src/cryptopp/basecode.cpp \
src/cryptopp/cpu.cpp \
src/cryptopp/cryptlib.cpp \
src/cryptopp/des.cpp \
src/cryptopp/dessp.cpp \
src/cryptopp/dll.cpp \
src/cryptopp/dsa.cpp \
src/cryptopp/eccrypto.cpp \
src/cryptopp/ecp.cpp \
src/cryptopp/ec2n.cpp \
src/cryptopp/eprecomp.cpp \
src/cryptopp/filters.cpp \
src/cryptopp/fips140.cpp \
src/cryptopp/gf2n.cpp \
src/cryptopp/gfpcrypt.cpp \
src/cryptopp/hex.cpp \
src/cryptopp/hmac.cpp \
src/cryptopp/hrtimer.cpp \
src/cryptopp/integer.cpp \
src/cryptopp/iterhash.cpp \
src/cryptopp/keccak.cpp \
src/cryptopp/misc.cpp \
src/cryptopp/modes.cpp \
src/cryptopp/mqueue.cpp \
src/cryptopp/nbtheory.cpp \
src/cryptopp/oaep.cpp \
src/cryptopp/osrng.cpp \
src/cryptopp/polynomi.cpp \
src/cryptopp/pubkey.cpp \
src/cryptopp/queue.cpp \
src/cryptopp/randpool.cpp \
src/cryptopp/rdtables.cpp \
src/cryptopp/rijndael.cpp \
src/cryptopp/rng.cpp \
src/cryptopp/sha.cpp \
src/cryptopp/sha3.cpp \
src/cryptopp/strciphr.cpp \
src/cryptopp/winpipes.cpp

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH := 

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := -DOS_RNG_AVAILABLE

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
LOCAL_PRE_LINK_CMD   := 
LOCAL_POST_LINK_CMD  := 
LOCAL_PRE_STRIP_CMD  := 
LOCAL_POST_STRIP_CMD := 

include $(LIBRARY_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
