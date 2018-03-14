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
# 文   件   名: liblua.mk
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
LOCAL_TARGET_NAME := liblua.so

#*********************************************************************************************************
# LUA 5.1 Source list
#*********************************************************************************************************
LOCAL_SRCS_51 := \
lua5.1/src/lapi.c \
lua5.1/src/lauxlib.c \
lua5.1/src/lbaselib.c \
lua5.1/src/lcode.c \
lua5.1/src/ldblib.c \
lua5.1/src/ldebug.c \
lua5.1/src/ldo.c \
lua5.1/src/ldump.c \
lua5.1/src/lfunc.c \
lua5.1/src/lgc.c \
lua5.1/src/linit.c \
lua5.1/src/liolib.c \
lua5.1/src/llex.c \
lua5.1/src/lmathlib.c \
lua5.1/src/lmem.c \
lua5.1/src/loadlib.c \
lua5.1/src/lobject.c \
lua5.1/src/lopcodes.c \
lua5.1/src/loslib.c \
lua5.1/src/lparser.c \
lua5.1/src/lstate.c \
lua5.1/src/lstring.c \
lua5.1/src/lstrlib.c \
lua5.1/src/ltable.c \
lua5.1/src/ltablib.c \
lua5.1/src/ltm.c \
lua5.1/src/lundump.c \
lua5.1/src/lvm.c \
lua5.1/src/lzio.c \
lua5.1/src/print.c 

#*********************************************************************************************************
# LUA 5.2 Source list
#*********************************************************************************************************
LOCAL_SRCS_52 := \
lua5.2/src/lapi.c \
lua5.2/src/lauxlib.c \
lua5.2/src/lbaselib.c \
lua5.2/src/lbitlib.c \
lua5.2/src/lcode.c \
lua5.2/src/lcorolib.c \
lua5.2/src/lctype.c \
lua5.2/src/ldblib.c \
lua5.2/src/ldebug.c \
lua5.2/src/ldo.c \
lua5.2/src/ldump.c \
lua5.2/src/lfunc.c \
lua5.2/src/lgc.c \
lua5.2/src/linit.c \
lua5.2/src/liolib.c \
lua5.2/src/llex.c \
lua5.2/src/lmathlib.c \
lua5.2/src/lmem.c \
lua5.2/src/loadlib.c \
lua5.2/src/lobject.c \
lua5.2/src/lopcodes.c \
lua5.2/src/loslib.c \
lua5.2/src/lparser.c \
lua5.2/src/lstate.c \
lua5.2/src/lstring.c \
lua5.2/src/lstrlib.c \
lua5.2/src/ltable.c \
lua5.2/src/ltablib.c \
lua5.2/src/ltm.c \
lua5.2/src/lundump.c \
lua5.2/src/lvm.c \
lua5.2/src/lzio.c 

#*********************************************************************************************************
# LUA 5.3 Source list
#*********************************************************************************************************
LOCAL_SRCS_53 := \
lua5.3/src/lapi.c \
lua5.3/src/lauxlib.c \
lua5.3/src/lbaselib.c \
lua5.3/src/lbitlib.c \
lua5.3/src/lcode.c \
lua5.3/src/lcorolib.c \
lua5.3/src/lctype.c \
lua5.3/src/ldblib.c \
lua5.3/src/ldebug.c \
lua5.3/src/ldo.c \
lua5.3/src/ldump.c \
lua5.3/src/lfunc.c \
lua5.3/src/lgc.c \
lua5.3/src/linit.c \
lua5.3/src/liolib.c \
lua5.3/src/llex.c \
lua5.3/src/lmathlib.c \
lua5.3/src/lmem.c \
lua5.3/src/loadlib.c \
lua5.3/src/lobject.c \
lua5.3/src/lopcodes.c \
lua5.3/src/loslib.c \
lua5.3/src/lparser.c \
lua5.3/src/lstate.c \
lua5.3/src/lstring.c \
lua5.3/src/lstrlib.c \
lua5.3/src/ltable.c \
lua5.3/src/ltablib.c \
lua5.3/src/ltm.c \
lua5.3/src/lundump.c \
lua5.3/src/lutf8lib.c \
lua5.3/src/lvm.c \
lua5.3/src/lzio.c 

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
ifeq ($(LUA_VER), 5.1)
LOCAL_SRCS := $(LOCAL_SRCS_51)
endif
ifeq ($(LUA_VER), 5.2)
LOCAL_SRCS := $(LOCAL_SRCS_52)
endif
ifeq ($(LUA_VER), 5.3)
LOCAL_SRCS := $(LOCAL_SRCS_53)
endif

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your hearder files search path")
#*********************************************************************************************************
LOCAL_INC_PATH := 
LOCAL_INC_PATH += -I"$(SYLIXOS_BASE_PATH)/libreadline"

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := -DLUA_USE_LINUX -DLUA_COMPAT_ALL -DLUA_COMPAT_5_1 -DLUA_COMPAT_5_2

#*********************************************************************************************************
# Depend library (eg. LOCAL_DEPEND_LIB := -la LOCAL_DEPEND_LIB_PATH := -L"Your library search path")
#*********************************************************************************************************
LOCAL_DEPEND_LIB      := -lreadline
LOCAL_DEPEND_LIB_PATH := -L"$(SYLIXOS_BASE_PATH)/libreadline/$(OUTDIR)"

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
