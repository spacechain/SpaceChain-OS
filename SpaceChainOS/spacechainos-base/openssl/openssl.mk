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
# 文   件   名: openssl.mk
#
# 创   建   人: RealEvo-IDE
#
# 文件创建日期: 2018 年 02 月 06 日
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
LOCAL_TARGET_NAME := openssl

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS := \
openssl/apps/app_rand.c \
openssl/apps/dhparam.c \
openssl/apps/rsautl.c \
openssl/apps/verify.c \
openssl/apps/genpkey.c \
openssl/apps/crl.c \
openssl/apps/s_server.c \
openssl/apps/rsa.c \
openssl/apps/nseq.c \
openssl/apps/pkcs8.c \
openssl/apps/engine.c \
openssl/apps/apps.c \
openssl/apps/ts.c \
openssl/apps/srp.c \
openssl/apps/dsa.c \
openssl/apps/spkac.c \
openssl/apps/dsaparam.c \
openssl/apps/req.c \
openssl/apps/s_client.c \
openssl/apps/smime.c \
openssl/apps/pkcs7.c \
openssl/apps/version.c \
openssl/apps/s_socket.c \
openssl/apps/gendsa.c \
openssl/apps/openssl.c \
openssl/apps/crl2p7.c \
openssl/apps/dh.c \
openssl/apps/enc.c \
openssl/apps/s_time.c \
openssl/apps/ocsp.c \
openssl/apps/errstr.c \
openssl/apps/pkeyparam.c \
openssl/apps/ca.c \
openssl/apps/genrsa.c \
openssl/apps/rand.c \
openssl/apps/pkeyutl.c \
openssl/apps/gendh.c \
openssl/apps/passwd.c \
openssl/apps/x509.c \
openssl/apps/pkey.c \
openssl/apps/s_cb.c \
openssl/apps/cms.c \
openssl/apps/pkcs12.c \
openssl/apps/ecparam.c \
openssl/apps/prime.c \
openssl/apps/ec.c \
openssl/apps/dgst.c \
openssl/apps/speed.c \
openssl/apps/ciphers.c \
openssl/apps/asn1pars.c \
openssl/apps/sess_id.c \

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH := -I"./openssl" -I"./openssl/include"

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := -DMONOLITH -DOPENSSL_PIC -DOPENSSL_THREADS -D_REENTRANT -DDSO_DLFCN -DHAVE_DLFCN_H

ifeq ($(ARCH), sparc)
LOCAL_DSYMBOL += -DB_ENDIAN 
else ifeq ($(ARCH), ppc)
LOCAL_DSYMBOL += -DB_ENDIAN 
else
LOCAL_DSYMBOL += -DL_ENDIAN 
endif

#*********************************************************************************************************
# Compiler flags
#*********************************************************************************************************
LOCAL_CFLAGS   := 
LOCAL_CXXFLAGS := 

#*********************************************************************************************************
# Depend library (eg. LOCAL_DEPEND_LIB := -la LOCAL_DEPEND_LIB_PATH := -L"Your library search path")
#*********************************************************************************************************
LOCAL_DEPEND_LIB      := -lssl -lcrypto
LOCAL_DEPEND_LIB_PATH := -L"$(OUTDIR)"

#*********************************************************************************************************
# C++ config
#*********************************************************************************************************
LOCAL_USE_CXX        := no
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

#*********************************************************************************************************
# Depend target
#*********************************************************************************************************
LOCAL_DEPEND_TARGET := ./$(OUTDIR)/libssl.so ./$(OUTDIR)/libcrypto.so

include $(APPLICATION_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
