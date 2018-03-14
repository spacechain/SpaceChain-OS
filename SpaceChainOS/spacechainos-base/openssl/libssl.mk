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
# 文   件   名: libssl.mk
#
# 创   建   人: RealEvo-IDE
#
# 文件创建日期: 2018 年 02 月 01 日
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
LOCAL_TARGET_NAME := libssl.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS :=  \
openssl/ssl/s2_meth.c \
openssl/ssl/s3_enc.c \
openssl/ssl/t1_reneg.c \
openssl/ssl/ssl_conf.c \
openssl/ssl/s3_clnt.c \
openssl/ssl/t1_ext.c \
openssl/ssl/ssl_txt.c \
openssl/ssl/ssl_err.c \
openssl/ssl/d1_srtp.c \
openssl/ssl/ssl_cert.c \
openssl/ssl/t1_trce.c \
openssl/ssl/d1_meth.c \
openssl/ssl/d1_lib.c \
openssl/ssl/s2_srvr.c \
openssl/ssl/ssl_rsa.c \
openssl/ssl/s2_pkt.c \
openssl/ssl/s2_lib.c \
openssl/ssl/t1_srvr.c \
openssl/ssl/ssl_utst.c \
openssl/ssl/s23_pkt.c \
openssl/ssl/kssl.c \
openssl/ssl/s23_lib.c \
openssl/ssl/ssl_ciph.c \
openssl/ssl/s3_lib.c \
openssl/ssl/s3_pkt.c \
openssl/ssl/bio_ssl.c \
openssl/ssl/ssl_stat.c \
openssl/ssl/ssl_asn1.c \
openssl/ssl/s3_both.c \
openssl/ssl/t1_meth.c \
openssl/ssl/tls_srp.c \
openssl/ssl/t1_clnt.c \
openssl/ssl/d1_pkt.c \
openssl/ssl/ssl_algs.c \
openssl/ssl/t1_enc.c \
openssl/ssl/s3_srvr.c \
openssl/ssl/s2_clnt.c \
openssl/ssl/d1_clnt.c \
openssl/ssl/s23_clnt.c \
openssl/ssl/ssl_sess.c \
openssl/ssl/s3_cbc.c \
openssl/ssl/ssl_lib.c \
openssl/ssl/d1_srvr.c \
openssl/ssl/d1_both.c \
openssl/ssl/ssl_err2.c \
openssl/ssl/t1_lib.c \
openssl/ssl/s23_meth.c \
openssl/ssl/s23_srvr.c \
openssl/ssl/s3_meth.c \
openssl/ssl/s2_enc.c \

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH := -I"./openssl" -I"./openssl/crypto" -I"./openssl/crypto/evp" -I"./openssl/include"

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := -DOPENSSL_PIC -DOPENSSL_THREADS -D_REENTRANT -DDSO_DLFCN -DHAVE_DLFCN_H

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
