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
LOCAL_TARGET_NAME := libvpmpdm.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS := \
SylixOS/vpmpdm/cfloat/backtrace/backtrace.c \
SylixOS/vpmpdm/cfloat/float/float.c \
SylixOS/vpmpdm/cfloat/iniparser/dictionary.c \
SylixOS/vpmpdm/cfloat/iniparser/iniparser.c \
SylixOS/vpmpdm/cfloat/stdio/asprintf.c \
SylixOS/vpmpdm/cfloat/stdio/cvtfloat.c \
SylixOS/vpmpdm/cfloat/stdio/fdprintf.c \
SylixOS/vpmpdm/cfloat/stdio/fdscanf.c \
SylixOS/vpmpdm/cfloat/stdio/fprintf.c \
SylixOS/vpmpdm/cfloat/stdio/fscanf.c \
SylixOS/vpmpdm/cfloat/stdio/gets.c \
SylixOS/vpmpdm/cfloat/stdio/printf.c \
SylixOS/vpmpdm/cfloat/stdio/puts.c \
SylixOS/vpmpdm/cfloat/stdio/scanf.c \
SylixOS/vpmpdm/cfloat/stdio/snprintf.c \
SylixOS/vpmpdm/cfloat/stdio/sprintf.c \
SylixOS/vpmpdm/cfloat/stdio/sscanf.c \
SylixOS/vpmpdm/cfloat/stdio/vfprintf.c \
SylixOS/vpmpdm/cfloat/stdio/vfscanf.c \
SylixOS/vpmpdm/cfloat/stdio/vprintf.c \
SylixOS/vpmpdm/cfloat/stdio/vscanf.c \
SylixOS/vpmpdm/cfloat/stdio/vsnprintf.c \
SylixOS/vpmpdm/cfloat/stdio/vsprintf.c \
SylixOS/vpmpdm/cfloat/stdio/vsscanf.c \
SylixOS/vpmpdm/cfloat/stdlib/lib_rand.c \
SylixOS/vpmpdm/cfloat/stdlib/lib_search.c \
SylixOS/vpmpdm/cfloat/stdlib/lib_sort.c \
SylixOS/vpmpdm/cfloat/stdlib/lib_strto.c \
SylixOS/vpmpdm/cfloat/stdlib/lib_strtod.c \
SylixOS/vpmpdm/cfloat/time/lib_difftime.c \
SylixOS/vpmpdm/cfloat/wchar/wchar.c \
SylixOS/vpmpdm/cfloat/wchar/wcsdup.c \
SylixOS/vpmpdm/dlmalloc/dl_malloc.c \
SylixOS/vpmpdm/dlmalloc/dlmalloc.c \
SylixOS/vpmpdm/net/getifaddrs.c \
SylixOS/vpmpdm/net/if.c \
SylixOS/vpmpdm/tlsf/tlsf.c \
SylixOS/vpmpdm/vpmpdm_cpp.cpp \
SylixOS/vpmpdm/vpmpdm_lm.c \
SylixOS/vpmpdm/vpmpdm_start.c \
SylixOS/vpmpdm/vpmpdm.c

#*********************************************************************************************************
# TI C6X DSP source
#*********************************************************************************************************
LOCAL_C6X_SRCS = \
SylixOS/vpmpdm/c6x/alloca.c \
SylixOS/vpmpdm/c6x/libc.c

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
