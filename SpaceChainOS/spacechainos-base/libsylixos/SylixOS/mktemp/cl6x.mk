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
# 文   件   名: cl6x.mk
#
# 创   建   人: Jiao.JinXing(焦进星)
#
# 文件创建日期: 2017 年 05 月 19 日
#
# 描        述: TI C6000 Compiler 相关变量定义
#*********************************************************************************************************

#*********************************************************************************************************
# Toolchain select
#*********************************************************************************************************
CC      = c6x-cc
CXX     = c6x-cc
AS      = c6x-cc
AR      = ar6x
LD      = c6x-ld
OC      = hex6x
STRIP   = c6x-strip
DIS     = dis6x
SZ      = c6x-size
LZOCOM  = c6x-lzocom
LINK    = c6x-link
DEPFIX  = c6x-dep

#*********************************************************************************************************
# Commercial toolchain check
#*********************************************************************************************************
TOOLCHAIN_COMMERCIAL = 1

#*********************************************************************************************************
# Toolchain version
#*********************************************************************************************************
TOOLCHAIN_VERSION_STR   = 4.5.1
TOOLCHAIN_VERSION_MAJOR = 4
TOOLCHAIN_VERSION_MINOR = 5
TOOLCHAIN_VERSION_PATCH = 1
TOOLCHAIN_VERSION       = $(TOOLCHAIN_VERSION_MAJOR)$(TOOLCHAIN_VERSION_MINOR)$(TOOLCHAIN_VERSION_PATCH)

#*********************************************************************************************************
# Compiler optimize flag
# Do NOT use -O3 and -Os, -Os is not align for function loop and jump.
#                         -O3 default use inline function.
#*********************************************************************************************************
ifeq ($(DEBUG_LEVEL), debug)
TOOLCHAIN_OPTIMIZE = -O0 -g
else
TOOLCHAIN_OPTIMIZE = -O1 -g
endif

#*********************************************************************************************************
# Toolchain flag
#*********************************************************************************************************
TOOLCHAIN_CXX_EXCEPT_CFLAGS    = --exceptions --rtti
TOOLCHAIN_NO_CXX_EXCEPT_CFLAGS =
TOOLCHAIN_GCOV_CFLAGS          =
TOOLCHAIN_OMP_CFLAGS           =
TOOLCHAIN_COMMONFLAGS          =
TOOLCHAIN_ASFLAGS              =

TOOLCHAIN_AR_FLAGS             = rq
TOOLCHAIN_STRIP_FLAGS          =
TOOLCHAIN_STRIP_KO_FLAGS       =
TOOLCHAIN_DIS_FLAGS            = --all

#*********************************************************************************************************
# Toolchain link library
#*********************************************************************************************************
TOOLCHAIN_LINK_VPMPDM    = -llibvpmpdm.so
TOOLCHAIN_LINK_CEXTERN   = -llibcextern.so
TOOLCHAIN_LINK_DSOHANDLE = -llibdsohandle.a
TOOLCHAIN_LINK_SYLIXOS   = -llibsylixos.a
TOOLCHAIN_LINK_GCOV      =
TOOLCHAIN_LINK_GCOV_KO   =
TOOLCHAIN_LINK_OMP       =
TOOLCHAIN_LINK_CXX       =
TOOLCHAIN_LINK_M         =
TOOLCHAIN_LINK_GCC       = -llibrts$(CPU_TYPE)_elf.a

TOOLCHAIN_LINK_PIC_GCOV  =
TOOLCHAIN_LINK_PIC_OMP   =
TOOLCHAIN_LINK_PIC_CXX   =
TOOLCHAIN_LINK_PIC_M     =
TOOLCHAIN_LINK_PIC_GCC   = -llibrts$(CPU_TYPE)_elf_pic.a

#*********************************************************************************************************
# Toolchain include & define
#*********************************************************************************************************
TOOLCHAIN_HEADER_INC     = -I
TOOLCHAIN_LIB_INC        = -i
TOOLCHAIN_DEF_SYMBOL     = -D

#*********************************************************************************************************
# End
#*********************************************************************************************************
