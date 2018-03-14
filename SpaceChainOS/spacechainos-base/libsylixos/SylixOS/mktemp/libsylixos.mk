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
# 文   件   名: libsylixos.mk
#
# 创   建   人: Jiao.JinXing(焦进星)
#
# 文件创建日期: 2016 年 10 月 02 日
#
# 描        述: libsylixos makefile 模板
#*********************************************************************************************************

#*********************************************************************************************************
# Include common.mk
#*********************************************************************************************************
include $(MKTEMP)/common.mk

#*********************************************************************************************************
# Depend and compiler parameter (cplusplus in kernel MUST NOT use exceptions and rtti)
#*********************************************************************************************************
ifeq ($($(target)_USE_CXX_EXCEPT), yes)
$(target)_CXX_EXCEPT  := $(TOOLCHAIN_NO_CXX_EXCEPT_CFLAGS)
else
$(target)_CXX_EXCEPT  := $(TOOLCHAIN_NO_CXX_EXCEPT_CFLAGS)
endif

ifeq ($($(target)_USE_GCOV), yes)
$(target)_GCOV_FLAGS  := $(TOOLCHAIN_NO_GCOV_CFLAGS)
else
$(target)_GCOV_FLAGS  := $(TOOLCHAIN_NO_GCOV_CFLAGS)
endif

ifeq ($($(target)_USE_OMP), yes)
$(target)_OMP_FLAGS   := $(TOOLCHAIN_NO_OMP_CFLAGS)
else
$(target)_OMP_FLAGS   := $(TOOLCHAIN_NO_OMP_CFLAGS)
endif

$(target)_CPUFLAGS                     := $(ARCH_CPUFLAGS_NOFPU) $(ARCH_KERNEL_CFLAGS)
$(target)_CPUFLAGS_WITHOUT_FPUFLAGS    := $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS) $(ARCH_KERNEL_CFLAGS)
$(target)_COMMONFLAGS                  := $($(target)_CPUFLAGS) $(ARCH_COMMONFLAGS) $(TOOLCHAIN_OPTIMIZE) $(TOOLCHAIN_COMMONFLAGS) $($(target)_GCOV_FLAGS) $($(target)_OMP_FLAGS)
$(target)_COMMONFLAGS_WITHOUT_FPUFLAGS := $($(target)_CPUFLAGS_WITHOUT_FPUFLAGS) $(ARCH_COMMONFLAGS) $(TOOLCHAIN_OPTIMIZE) $(TOOLCHAIN_COMMONFLAGS) $($(target)_GCOV_FLAGS) $($(target)_OMP_FLAGS)
$(target)_ASFLAGS                      := $($(target)_COMMONFLAGS) $(TOOLCHAIN_ASFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH)
$(target)_ASFLAGS_WITHOUT_FPUFLAGS     := $($(target)_COMMONFLAGS_WITHOUT_FPUFLAGS) $(TOOLCHAIN_ASFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH)
$(target)_CFLAGS                       := $($(target)_COMMONFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CFLAGS)
$(target)_CXXFLAGS                     := $($(target)_COMMONFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CXX_EXCEPT) $($(target)_CXXFLAGS)

#*********************************************************************************************************
# Targets
#*********************************************************************************************************
$(target)_A := $(OUTPATH)/$(LOCAL_TARGET_NAME)

#*********************************************************************************************************
# Objects
#*********************************************************************************************************
OBJS_ARCH    := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(LOCAL_ARCH_SRCS))))
OBJS_APPL    := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(APPL_SRCS))))
OBJS_DEBUG   := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(DEBUG_SRCS))))
OBJS_DRV     := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(DRV_SRCS))))
OBJS_FS      := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(FS_SRCS))))
OBJS_GUI     := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(GUI_SRCS))))
OBJS_KERN    := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(KERN_SRCS))))
OBJS_LIB     := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(LIB_SRCS))))
OBJS_LOADER  := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(LOADER_SRCS))))
OBJS_MONITOR := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(MONITOR_SRCS))))
OBJS_MPI     := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(MPI_SRCS))))
OBJS_NET     := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(NET_SRCS))))
OBJS_POSIX   := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(POSIX_SRCS))))
OBJS_SHELL   := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(SHELL_SRCS))))
OBJS_SYMBOL  := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(SYMBOL_SRCS))))
OBJS_SYS     := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(SYS_SRCS))))
OBJS_SYSPERF := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(SYSPERF_SRCS))))
OBJS_CPP     := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(CPP_SRCS))))

#*********************************************************************************************************
# Make archive object files
#*********************************************************************************************************
$($(target)_A): $($(target)_OBJS)
		@rm -f $@
		$(__PRE_LINK_CMD)
		$(AR) $(TOOLCHAIN_AR_FLAGS) $@ $(OBJS_APPL)
		$(AR) $(TOOLCHAIN_AR_FLAGS) $@ $(OBJS_ARCH)
		$(AR) $(TOOLCHAIN_AR_FLAGS) $@ $(OBJS_DEBUG)
		$(AR) $(TOOLCHAIN_AR_FLAGS) $@ $(OBJS_DRV)
		$(AR) $(TOOLCHAIN_AR_FLAGS) $@ $(OBJS_FS)
		$(AR) $(TOOLCHAIN_AR_FLAGS) $@ $(OBJS_GUI)
		$(AR) $(TOOLCHAIN_AR_FLAGS) $@ $(OBJS_KERN)
		$(AR) $(TOOLCHAIN_AR_FLAGS) $@ $(OBJS_LIB)
		$(AR) $(TOOLCHAIN_AR_FLAGS) $@ $(OBJS_MONITOR)
		$(AR) $(TOOLCHAIN_AR_FLAGS) $@ $(OBJS_LOADER)
		$(AR) $(TOOLCHAIN_AR_FLAGS) $@ $(OBJS_MPI)
		$(AR) $(TOOLCHAIN_AR_FLAGS) $@ $(OBJS_NET)
		$(AR) $(TOOLCHAIN_AR_FLAGS) $@ $(OBJS_POSIX)
		$(AR) $(TOOLCHAIN_AR_FLAGS) $@ $(OBJS_SHELL)
		$(AR) $(TOOLCHAIN_AR_FLAGS) $@ $(OBJS_SYMBOL)
		$(AR) $(TOOLCHAIN_AR_FLAGS) $@ $(OBJS_SYS)
		$(AR) $(TOOLCHAIN_AR_FLAGS) $@ $(OBJS_SYSPERF)
		$(AR) $(TOOLCHAIN_AR_FLAGS) $@ $(OBJS_CPP)
		$(__POST_LINK_CMD)

#*********************************************************************************************************
# Create symbol files
#*********************************************************************************************************
ifeq ($(ARCH), c6x)
$(OUTPATH)/symbol.c: $($(target)_A)
		@rm -f $@
		cp SylixOS/hosttools/c6x/makesymbol/Makefile $(OUTDIR)
		cp SylixOS/hosttools/c6x/makesymbol/makesymbol.bat $(OUTDIR)
		cp SylixOS/hosttools/c6x/makesymbol/makesymbol.sh $(OUTDIR)
		cp SylixOS/hosttools/c6x/makesymbol/nm.exe $(OUTDIR)
		make -C $(OUTDIR)
else
$(OUTPATH)/symbol.c: $($(target)_A)
		@rm -f $@
		cp SylixOS/hosttools/makesymbol/Makefile $(OUTDIR)
		cp SylixOS/hosttools/makesymbol/makesymbol.bat $(OUTDIR)
		cp SylixOS/hosttools/makesymbol/makesymbol.sh $(OUTDIR)
		cp SylixOS/hosttools/makesymbol/nm.exe $(OUTDIR)
		make -C $(OUTDIR)
endif

#*********************************************************************************************************
# Add targets
#*********************************************************************************************************
TARGETS := $(TARGETS) $($(target)_A) $(OUTPATH)/symbol.c

#*********************************************************************************************************
# End
#*********************************************************************************************************
