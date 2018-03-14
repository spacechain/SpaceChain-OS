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
# 文   件   名: libvpmpdm.mk
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
LOCAL_TARGET_NAME := libmbedcrypto.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS := \
SylixOS/appl/ssl/mbedtls/library/aes.c \
SylixOS/appl/ssl/mbedtls/library/aesni.c \
SylixOS/appl/ssl/mbedtls/library/arc4.c \
SylixOS/appl/ssl/mbedtls/library/asn1parse.c \
SylixOS/appl/ssl/mbedtls/library/asn1write.c \
SylixOS/appl/ssl/mbedtls/library/base64.c \
SylixOS/appl/ssl/mbedtls/library/bignum.c \
SylixOS/appl/ssl/mbedtls/library/blowfish.c \
SylixOS/appl/ssl/mbedtls/library/camellia.c \
SylixOS/appl/ssl/mbedtls/library/ccm.c \
SylixOS/appl/ssl/mbedtls/library/cipher.c \
SylixOS/appl/ssl/mbedtls/library/cipher_wrap.c \
SylixOS/appl/ssl/mbedtls/library/cmac.c \
SylixOS/appl/ssl/mbedtls/library/ctr_drbg.c \
SylixOS/appl/ssl/mbedtls/library/des.c \
SylixOS/appl/ssl/mbedtls/library/dhm.c \
SylixOS/appl/ssl/mbedtls/library/ecdh.c \
SylixOS/appl/ssl/mbedtls/library/ecdsa.c \
SylixOS/appl/ssl/mbedtls/library/ecjpake.c \
SylixOS/appl/ssl/mbedtls/library/ecp_mbed.c \
SylixOS/appl/ssl/mbedtls/library/ecp_curves.c \
SylixOS/appl/ssl/mbedtls/library/entropy.c \
SylixOS/appl/ssl/mbedtls/library/entropy_poll.c \
SylixOS/appl/ssl/mbedtls/library/error.c \
SylixOS/appl/ssl/mbedtls/library/gcm.c \
SylixOS/appl/ssl/mbedtls/library/havege.c \
SylixOS/appl/ssl/mbedtls/library/hmac_drbg.c \
SylixOS/appl/ssl/mbedtls/library/md.c \
SylixOS/appl/ssl/mbedtls/library/md2.c \
SylixOS/appl/ssl/mbedtls/library/md4.c \
SylixOS/appl/ssl/mbedtls/library/md5.c \
SylixOS/appl/ssl/mbedtls/library/md_wrap.c \
SylixOS/appl/ssl/mbedtls/library/memory_buffer_alloc.c \
SylixOS/appl/ssl/mbedtls/library/oid.c \
SylixOS/appl/ssl/mbedtls/library/padlock.c \
SylixOS/appl/ssl/mbedtls/library/pem.c \
SylixOS/appl/ssl/mbedtls/library/pk.c \
SylixOS/appl/ssl/mbedtls/library/pk_wrap.c \
SylixOS/appl/ssl/mbedtls/library/pkcs12.c \
SylixOS/appl/ssl/mbedtls/library/pkcs5.c \
SylixOS/appl/ssl/mbedtls/library/pkparse.c \
SylixOS/appl/ssl/mbedtls/library/pkwrite.c \
SylixOS/appl/ssl/mbedtls/library/platform.c \
SylixOS/appl/ssl/mbedtls/library/ripemd160.c \
SylixOS/appl/ssl/mbedtls/library/rsa.c \
SylixOS/appl/ssl/mbedtls/library/sha1.c \
SylixOS/appl/ssl/mbedtls/library/sha256.c \
SylixOS/appl/ssl/mbedtls/library/sha512.c \
SylixOS/appl/ssl/mbedtls/library/threading.c \
SylixOS/appl/ssl/mbedtls/library/timing.c \
SylixOS/appl/ssl/mbedtls/library/version.c \
SylixOS/appl/ssl/mbedtls/library/version_features.c \
SylixOS/appl/ssl/mbedtls/library/xtea.c 

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your hearder files search path")
#*********************************************************************************************************
LOCAL_INC_PATH := 

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := 

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
