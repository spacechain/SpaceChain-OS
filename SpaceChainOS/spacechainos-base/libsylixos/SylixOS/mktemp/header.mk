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
# 文   件   名: header.mk
#
# 创   建   人: Jiao.JinXing(焦进星)
#
# 文件创建日期: 2016 年 08 月 24 日
#
# 描        述: makefile 模板首部
#*********************************************************************************************************

#*********************************************************************************************************
# Check configure
#*********************************************************************************************************
check_defined = \
    $(foreach 1,$1,$(__check_defined))
__check_defined = \
    $(if $(value $1),, \
      $(error Undefined $1$(if $(value 2), ($(strip $2)))))

$(call check_defined, CONFIG_MK_EXIST, Please configure this project in RealEvo-IDE or create a config.mk file!)
$(call check_defined, SYLIXOS_BASE_PATH, SylixOS base project path)
$(call check_defined, TOOLCHAIN_PREFIX, the prefix name of toolchain)
$(call check_defined, DEBUG_LEVEL, debug level(debug or release))

#*********************************************************************************************************
# All *.mk files
#*********************************************************************************************************
APPLICATION_MK    = $(MKTEMP)/application.mk
LIBRARY_MK        = $(MKTEMP)/library.mk
STATIC_LIBRARY_MK = $(MKTEMP)/static-library.mk
KERNEL_MODULE_MK  = $(MKTEMP)/kernel-module.mk
KERNEL_LIBRARY_MK = $(MKTEMP)/kernel-library.mk
UNIT_TEST_MK      = $(MKTEMP)/unit-test.mk
LIBSYLIXOS_MK     = $(MKTEMP)/libsylixos.mk
DUMMY_MK          = $(MKTEMP)/dummy.mk
BSP_MK            = $(MKTEMP)/bsp.mk
EXTENSION_MK      = $(MKTEMP)/extension.mk
LITE_BSP_MK       = $(MKTEMP)/lite-bsp.mk
END_MK            = $(MKTEMP)/end.mk
CLEAR_VARS_MK     = $(MKTEMP)/clear-vars.mk

#*********************************************************************************************************
# Build paths
#*********************************************************************************************************
ifeq ($(DEBUG_LEVEL), debug)
OUTDIR = Debug
else
OUTDIR = Release
endif

OUTPATH = ./$(OUTDIR)
OBJPATH = $(OUTPATH)/obj
DEPPATH = $(OUTPATH)/dep

#*********************************************************************************************************
# Define some useful variables
#*********************************************************************************************************
BIAS  = /
EMPTY =
SPACE = $(EMPTY) $(EMPTY)

__TARGET    = $(word 3,$(subst $(BIAS),$(SPACE),$(@)))
__DEP       = $(addprefix $(DEPPATH)/$(__TARGET)/, $(addsuffix .d, $(basename $(<))))
ifneq (,$(findstring cl6x,$(TOOLCHAIN_PREFIX)))
__PP        = $(addprefix $(DEPPATH)/$(__TARGET)/, $(addsuffix .pp, $(basename $(<))))
endif
__LIBRARIES = $($(@F)_DEPEND_LIB_PATH) $($(@F)_DEPEND_LIB)
__OBJS      = $($(@F)_OBJS)
__CPUFLAGS  = $($(@F)_CPUFLAGS)
__DSYMBOL   = $($(@F)_DSYMBOL)

__PRE_LINK_CMD   = $($(@F)_PRE_LINK_CMD)
__POST_LINK_CMD  = $($(@F)_POST_LINK_CMD)

__PRE_STRIP_CMD  = $($(@F)_PRE_STRIP_CMD)
__POST_STRIP_CMD = $($(@F)_POST_STRIP_CMD)

#*********************************************************************************************************
# Include toolchain mk
#*********************************************************************************************************
ifneq (,$(findstring cl6x,$(TOOLCHAIN_PREFIX)))
include $(MKTEMP)/cl6x.mk
else
include $(MKTEMP)/gcc.mk
endif

#*********************************************************************************************************
# Include arch.mk
#*********************************************************************************************************
include $(MKTEMP)/arch.mk

#*********************************************************************************************************
# End
#*********************************************************************************************************
