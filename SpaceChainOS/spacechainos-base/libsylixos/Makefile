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
# 文   件   名: Makefile
#
# 创   建   人: RealEvo-IDE
#
# 文件创建日期: 2016 年 10 月 08 日
#
# 描        述: 本文件由 RealEvo-IDE 生成，用于配置 Makefile 功能，请勿手动修改
#*********************************************************************************************************

#*********************************************************************************************************
# Include config.mk
#*********************************************************************************************************
CONFIG_MK_EXIST = $(shell if [ -f ../config.mk ]; then echo exist; else echo notexist; fi;)
ifeq ($(CONFIG_MK_EXIST), exist)
include ../config.mk
else
CONFIG_MK_EXIST = $(shell if [ -f config.mk ]; then echo exist; else echo notexist; fi;)
ifeq ($(CONFIG_MK_EXIST), exist)
include config.mk
else
CONFIG_MK_EXIST =
endif
endif

#*********************************************************************************************************
# Build with lite mode (SylixOS Lite)
# Do you want build SylixOS with Lite Mode
#*********************************************************************************************************
BUILD_LITE_TARGET = 0

#*********************************************************************************************************
# Build options
# Do you want build process support library
#*********************************************************************************************************
ifeq ($(BUILD_LITE_TARGET), 0)
BUILD_PROCESS_SUP_LIB = 1
else
BUILD_PROCESS_SUP_LIB = 0
endif

#*********************************************************************************************************
# Build options
# Do you want build tls support library
#*********************************************************************************************************
ifeq ($(BUILD_LITE_TARGET), 0)
BUILD_TLS_SUP_LIB = 1
else
BUILD_TLS_SUP_LIB = 0
endif

#*********************************************************************************************************
# Do you want build some usefull kernel module
#*********************************************************************************************************
ifeq ($(BUILD_LITE_TARGET), 0)
BUILD_KERNEL_MODULE = 1
else
BUILD_KERNEL_MODULE = 0
endif

#*********************************************************************************************************
# Include header.mk
#*********************************************************************************************************
EMPTY  =
SPACE  = $(EMPTY) $(EMPTY)
MKTEMP = $(subst $(SPACE),\ ,$(SYLIXOS_BASE_PATH))/libsylixos/SylixOS/mktemp

include $(MKTEMP)/header.mk

#*********************************************************************************************************
# Include targets makefiles
#*********************************************************************************************************
include libsylixos.mk

#*********************************************************************************************************
# TI C6X DSP configure
#*********************************************************************************************************
ifeq ($(ARCH), c6x)
BUILD_PROCESS_SUP_LIB = 1
BUILD_KERNEL_MODULE   = 0
endif

ifeq ($(BUILD_PROCESS_SUP_LIB), 1)
include libdsohandle.mk
include libvpmpdm.mk
include environ.mk
endif

ifeq ($(BUILD_TLS_SUP_LIB), 1)
include libmbedcrypto.mk
include libmbedx509.mk
include libmbedtls.mk
include kidvpn.mk
endif

ifeq ($(BUILD_KERNEL_MODULE), 1)
include xinput.mk
include xsiipc.mk
endif

#*********************************************************************************************************
# Include end.mk
#*********************************************************************************************************
include $(END_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
