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
# 文   件   名: unit-test.mk
#
# 创   建   人: Jiao.JinXing(焦进星)
#
# 文件创建日期: 2016 年 10 月 01 日
#
# 描        述: 单元测试应用程序类目标 makefile 模板
#*********************************************************************************************************

#*********************************************************************************************************
# Include common.mk
#*********************************************************************************************************
include $(MKTEMP)/common.mk

#*********************************************************************************************************
# Depend and compiler parameter (cplusplus in kernel MUST NOT use exceptions and rtti)
#*********************************************************************************************************
ifeq ($($(target)_USE_CXX_EXCEPT), yes)
$(target)_CXX_EXCEPT  := $(TOOLCHAIN_CXX_EXCEPT_CFLAGS)
else
$(target)_CXX_EXCEPT  := $(TOOLCHAIN_NO_CXX_EXCEPT_CFLAGS)
endif

ifeq ($($(target)_USE_GCOV), yes)
$(target)_GCOV_FLAGS  := $(TOOLCHAIN_GCOV_CFLAGS)
else
$(target)_GCOV_FLAGS  := $(TOOLCHAIN_NO_GCOV_CFLAGS)
endif

ifeq ($($(target)_USE_OMP), yes)
$(target)_OMP_FLAGS   := $(TOOLCHAIN_OMP_CFLAGS)
else
$(target)_OMP_FLAGS   := $(TOOLCHAIN_NO_OMP_CFLAGS)
endif

$(target)_CPUFLAGS    := $(ARCH_CPUFLAGS)
$(target)_COMMONFLAGS := $($(target)_CPUFLAGS) $(ARCH_COMMONFLAGS) $(TOOLCHAIN_OPTIMIZE) $(TOOLCHAIN_COMMONFLAGS) $($(target)_GCOV_FLAGS) $($(target)_OMP_FLAGS)
$(target)_ASFLAGS     := $($(target)_COMMONFLAGS) $(ARCH_PIC_ASFLAGS) $(TOOLCHAIN_ASFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH)
$(target)_CFLAGS      := $($(target)_COMMONFLAGS) $(ARCH_PIC_CFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CFLAGS)
$(target)_CXXFLAGS    := $($(target)_COMMONFLAGS) $(ARCH_PIC_CFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CXX_EXCEPT) $($(target)_CXXFLAGS)

#*********************************************************************************************************
# Targets
#*********************************************************************************************************
$(target)_EXE       := $(addprefix $(OUTPATH)/$(target)/, $(basename $(LOCAL_SRCS)))
$(target)_STRIP_EXE := $(addprefix $(OUTPATH)/strip/$(target)/, $(basename $(LOCAL_SRCS)))

#*********************************************************************************************************
# Depend base library search paths (Select the user specified optimization level library first)
#*********************************************************************************************************
$(target)_DEPEND_LIB_PATH := $(TOOLCHAIN_LIB_INC)"$(SYLIXOS_BASE_PATH)/libsylixos/$(OUTDIR)"

ifeq ($(DEBUG_LEVEL), debug)
$(target)_DEPEND_LIB_PATH += $(TOOLCHAIN_LIB_INC)"$(SYLIXOS_BASE_PATH)/libsylixos/Release"
else
$(target)_DEPEND_LIB_PATH += $(TOOLCHAIN_LIB_INC)"$(SYLIXOS_BASE_PATH)/libsylixos/Debug"
endif

ifeq ($($(target)_USE_CXX), yes)
$(target)_DEPEND_LIB_PATH += $(TOOLCHAIN_LIB_INC)"$(SYLIXOS_BASE_PATH)/libcextern/$(OUTDIR)"

ifeq ($(DEBUG_LEVEL), debug)
$(target)_DEPEND_LIB_PATH += $(TOOLCHAIN_LIB_INC)"$(SYLIXOS_BASE_PATH)/libcextern/Release"
else
$(target)_DEPEND_LIB_PATH += $(TOOLCHAIN_LIB_INC)"$(SYLIXOS_BASE_PATH)/libcextern/Debug"
endif
endif

#*********************************************************************************************************
# Depend user library search paths
#*********************************************************************************************************
$(target)_DEPEND_LIB_PATH += $(LOCAL_DEPEND_LIB_PATH)

#*********************************************************************************************************
# Depend libraries
#*********************************************************************************************************
$(target)_DEPEND_LIB := $(LOCAL_DEPEND_LIB)
$(target)_DEPEND_LIB += $(TOOLCHAIN_LINK_VPMPDM)

ifeq ($($(target)_USE_GCOV), yes)
$(target)_DEPEND_LIB += $(TOOLCHAIN_LINK_PIC_GCOV)
endif

ifeq ($($(target)_USE_OMP), yes)
$(target)_DEPEND_LIB += $(TOOLCHAIN_LINK_PIC_OMP)
endif

ifeq ($($(target)_USE_CXX), yes)
$(target)_DEPEND_LIB += $(TOOLCHAIN_LINK_CEXTERN) $(TOOLCHAIN_LINK_PIC_CXX) $(TOOLCHAIN_LINK_DSOHANDLE)
endif

$(target)_DEPEND_LIB += $(TOOLCHAIN_LINK_PIC_M) $(TOOLCHAIN_LINK_PIC_GCC)

#*********************************************************************************************************
# Define some useful variables
#*********************************************************************************************************
__UNIT_TEST_TARGET         = $(word 3,$(subst $(BIAS),$(SPACE),$(1)))
__UNIT_TEST_STRIP_TARGET   = $(word 4,$(subst $(BIAS),$(SPACE),$(1)))

__UNIT_TEST_LIBRARIES      = $($(__UNIT_TEST_TARGET)_DEPEND_LIB_PATH) $($(__UNIT_TEST_TARGET)_DEPEND_LIB)

__UNIT_TEST_PRE_LINK_CMD   = $($(__UNIT_TEST_TARGET)_PRE_LINK_CMD)
__UNIT_TEST_POST_LINK_CMD  = $($(__UNIT_TEST_TARGET)_POST_LINK_CMD)

__UNIT_TEST_PRE_STRIP_CMD  = $($(__UNIT_TEST_STRIP_TARGET)_PRE_STRIP_CMD)
__UNIT_TEST_POST_STRIP_CMD = $($(__UNIT_TEST_STRIP_TARGET)_POST_STRIP_CMD)

__UNIT_TEST_CPUFLAGS       = $($(__UNIT_TEST_TARGET)_CPUFLAGS)

#*********************************************************************************************************
# Link object files
#*********************************************************************************************************
ifeq ($(ARCH), c6x)

define CREATE_TARGET_EXE
$1: $2 $3
		@if [ ! -d "$(dir $1)" ]; then mkdir -p "$(dir $1)"; fi
		@rm -f $1
		$(__UNIT_TEST_PRE_LINK_CMD)
		$(LD) $(__UNIT_TEST_CPUFLAGS) $(ARCH_PIC_LDFLAGS) $2 $(__UNIT_TEST_LIBRARIES) -o $1 
		@mv $1 $1.c6x
		@nm $1.c6x > $1_nm.txt
		@$(DIS) $(TOOLCHAIN_DIS_FLAGS) $1.c6x > $1_dis.txt
		@$(LINK) $1_nm.txt $1_dis.txt $1.c6x
		@mv $1.c6x $1
		@rm -f $1_nm.txt $1_dis.txt
		$(__UNIT_TEST_POST_LINK_CMD)
endef

else

define CREATE_TARGET_EXE
$1: $2 $3
		@if [ ! -d "$(dir $1)" ]; then mkdir -p "$(dir $1)"; fi
		@rm -f $1
		$(__UNIT_TEST_PRE_LINK_CMD)
		$(LD) $(__UNIT_TEST_CPUFLAGS) $(ARCH_PIC_LDFLAGS) $2 $(__UNIT_TEST_LIBRARIES) -o $1 
		$(__UNIT_TEST_POST_LINK_CMD)
endef

endif

$(foreach src,$(LOCAL_SRCS),$(eval $(call CREATE_TARGET_EXE,\
$(addprefix $(OUTPATH)/$(target)/, $(basename $(src))),\
$(addprefix $(OBJPATH)/$(target)/, $(addsuffix .o, $(basename $(src)))),\
$($(target)_DEPEND_TARGET),\
)))

#*********************************************************************************************************
# Strip target
#*********************************************************************************************************
define CREATE_TARGET_STRIP_EXE
$1: $2
		@if [ ! -d "$(dir $1)" ]; then mkdir -p "$(dir $1)"; fi
		@rm -f $1
		$(__UNIT_TEST_PRE_STRIP_CMD)
		$(STRIP) $(TOOLCHAIN_STRIP_FLAGS) $2 -o $1
		$(__UNIT_TEST_POST_STRIP_CMD)
endef

$(foreach src,$(LOCAL_SRCS),$(eval $(call CREATE_TARGET_STRIP_EXE,\
$(addprefix $(OUTPATH)/strip/$(target)/, $(basename $(src))),\
$(addprefix $(OUTPATH)/$(target)/, $(basename $(src))),\
)))

#*********************************************************************************************************
# Add targets
#*********************************************************************************************************
TARGETS := $(TARGETS) $($(target)_EXE) $($(target)_STRIP_EXE)

#*********************************************************************************************************
# End
#*********************************************************************************************************
