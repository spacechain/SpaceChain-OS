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
# Include header.mk
#*********************************************************************************************************
EMPTY  =
SPACE  = $(EMPTY) $(EMPTY)
MKTEMP = $(subst $(SPACE),\ ,$(SYLIXOS_BASE_PATH))/libsylixos/SylixOS/mktemp

include $(MKTEMP)/header.mk

#*********************************************************************************************************
# LUA Version select (MUST same as liblua)
#*********************************************************************************************************
LUA_VER = 5.2

ifeq ($(LUA_VER), 5.1)
LUA_INC_PATH = -I"$(SYLIXOS_BASE_PATH)/liblua/lua5.1/src"
endif
ifeq ($(LUA_VER), 5.2)
LUA_INC_PATH = -I"$(SYLIXOS_BASE_PATH)/liblua/lua5.2/src"
endif
ifeq ($(LUA_VER), 5.3)
LUA_INC_PATH = -I"$(SYLIXOS_BASE_PATH)/liblua/lua5.3/src"
endif

#*********************************************************************************************************
# Include targets makefiles
#*********************************************************************************************************
include lfs.mk
include llthreads2.mk
include lpack.mk
include lsqlite3.mk
include lxp.mk
include socket.mk
include task.mk
include mime.mk
include unix.mk
include cjson.mk
include xml.mk
include copas.mk
include debugger.mk

#*********************************************************************************************************
# Include end.mk
#*********************************************************************************************************
include $(END_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
