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
LOCAL_TARGET_NAME := libsylixos.a

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
#*********************************************************************************************************
# ARM source
#*********************************************************************************************************
LOCAL_ARM_SRCS = \
SylixOS/arch/arm/backtrace/armBacktrace.c \
SylixOS/arch/arm/common/cp15/armCp15Asm.S \
SylixOS/arch/arm/common/armAssert.c \
SylixOS/arch/arm/common/armContext.c \
SylixOS/arch/arm/common/armContextAsm.S \
SylixOS/arch/arm/common/armExc.c \
SylixOS/arch/arm/common/armExcAsm.S \
SylixOS/arch/arm/common/armIo.c \
SylixOS/arch/arm/common/armLib.c \
SylixOS/arch/arm/common/armLibAsm.S \
SylixOS/arch/arm/common/v7m/armContextV7M.c \
SylixOS/arch/arm/common/v7m/armContextV7MAsm.S \
SylixOS/arch/arm/common/v7m/armExcV7M.c \
SylixOS/arch/arm/common/v7m/armExcV7MAsm.S \
SylixOS/arch/arm/common/v7m/armLibV7M.c \
SylixOS/arch/arm/common/v7m/armLibV7MAsm.S \
SylixOS/arch/arm/dbg/armDbg.c \
SylixOS/arch/arm/dbg/armGdb.c \
SylixOS/arch/arm/dma/pl330/armPl330.c \
SylixOS/arch/arm/elf/armElf.c \
SylixOS/arch/arm/elf/armUnwind.c \
SylixOS/arch/arm/fpu/v7m/armVfpV7M.c \
SylixOS/arch/arm/fpu/v7m/armVfpV7MAsm.S \
SylixOS/arch/arm/fpu/vfp9/armVfp9.c \
SylixOS/arch/arm/fpu/vfp9/armVfp9Asm.S \
SylixOS/arch/arm/fpu/vfp11/armVfp11.c \
SylixOS/arch/arm/fpu/vfp11/armVfp11Asm.S \
SylixOS/arch/arm/fpu/vfpv3/armVfpV3.c \
SylixOS/arch/arm/fpu/vfpv3/armVfpV3Asm.S \
SylixOS/arch/arm/fpu/vfpv4/armVfpV4.c \
SylixOS/arch/arm/fpu/vfpnone/armVfpNone.c \
SylixOS/arch/arm/fpu/armFpu.c \
SylixOS/arch/arm/mm/cache/l2/armL2.c \
SylixOS/arch/arm/mm/cache/l2/armL2A17.c \
SylixOS/arch/arm/mm/cache/l2/armL2A8.c \
SylixOS/arch/arm/mm/cache/l2/armL2x0.c \
SylixOS/arch/arm/mm/cache/v4/armCacheV4.c \
SylixOS/arch/arm/mm/cache/v4/armCacheV4Asm.S \
SylixOS/arch/arm/mm/cache/v5/armCacheV5.c \
SylixOS/arch/arm/mm/cache/v5/armCacheV5Asm.S \
SylixOS/arch/arm/mm/cache/v6/armCacheV6.c \
SylixOS/arch/arm/mm/cache/v6/armCacheV6Asm.S \
SylixOS/arch/arm/mm/cache/v7/armCacheV7.c \
SylixOS/arch/arm/mm/cache/v7/armCacheV7Asm.S \
SylixOS/arch/arm/mm/cache/v7m/armCacheV7M.c \
SylixOS/arch/arm/mm/cache/v8/armCacheV8.c \
SylixOS/arch/arm/mm/cache/armCacheCommonAsm.S \
SylixOS/arch/arm/mm/mmu/v4/armMmuV4.c \
SylixOS/arch/arm/mm/mmu/v7/armMmuV7.c \
SylixOS/arch/arm/mm/mmu/v7/armMmuV7Asm.S \
SylixOS/arch/arm/mm/mmu/armMmuCommon.c \
SylixOS/arch/arm/mm/mmu/armMmuCommonAsm.S \
SylixOS/arch/arm/mm/mpu/v7m/armMpuV7M.c \
SylixOS/arch/arm/mm/mpu/v7r/armMpuV7R.c \
SylixOS/arch/arm/mm/mpu/v7r/armMpuV7RAsm.S \
SylixOS/arch/arm/mm/armCache.c \
SylixOS/arch/arm/mm/armMmu.c \
SylixOS/arch/arm/mm/armMpu.c \
SylixOS/arch/arm/mpcore/scu/armScu.c \
SylixOS/arch/arm/mpcore/armMpCoreAsm.S \
SylixOS/arch/arm/mpcore/armSpinlock.c \
SylixOS/arch/arm/param/armParam.c 

#*********************************************************************************************************
# MIPS source
#*********************************************************************************************************
LOCAL_MIPS_SRCS = \
SylixOS/arch/mips/backtrace/mipsBacktrace.c \
SylixOS/arch/mips/common/cp0/mipsCp0Asm.S \
SylixOS/arch/mips/common/mipsAssert.c \
SylixOS/arch/mips/common/mipsContext.c \
SylixOS/arch/mips/common/mipsContextAsm.S \
SylixOS/arch/mips/common/mipsExc.c \
SylixOS/arch/mips/common/mipsExcAsm.S \
SylixOS/arch/mips/common/mipsLib.c \
SylixOS/arch/mips/common/mipsLibAsm.S \
SylixOS/arch/mips/common/mipsBranch.c \
SylixOS/arch/mips/common/unaligned/mipsUnaligned.c \
SylixOS/arch/mips/common/mipsIdle.c \
SylixOS/arch/mips/common/mipsIdleAsm.S \
SylixOS/arch/mips/common/mipsIo.c \
SylixOS/arch/mips/common/mips64.c \
SylixOS/arch/mips/common/mips64Asm.S \
SylixOS/arch/mips/common/mipsCpuProbe.c \
SylixOS/arch/mips/dbg/mipsDbg.c \
SylixOS/arch/mips/dbg/mipsGdb.c \
SylixOS/arch/mips/elf/mipsElf.c \
SylixOS/arch/mips/fpu/fpu32/mipsVfp32.c \
SylixOS/arch/mips/fpu/vfpnone/mipsVfpNone.c \
SylixOS/arch/mips/fpu/fpu32/mipsVfp32Asm.S \
SylixOS/arch/mips/fpu/emu/cp1emu.c \
SylixOS/arch/mips/fpu/emu/dp_2008class.c \
SylixOS/arch/mips/fpu/emu/dp_add.c \
SylixOS/arch/mips/fpu/emu/dp_cmp.c \
SylixOS/arch/mips/fpu/emu/dp_div.c \
SylixOS/arch/mips/fpu/emu/dp_fint.c \
SylixOS/arch/mips/fpu/emu/dp_flong.c \
SylixOS/arch/mips/fpu/emu/dp_fmax.c \
SylixOS/arch/mips/fpu/emu/dp_fmin.c \
SylixOS/arch/mips/fpu/emu/dp_fsp.c \
SylixOS/arch/mips/fpu/emu/dp_maddf.c \
SylixOS/arch/mips/fpu/emu/dp_mul.c \
SylixOS/arch/mips/fpu/emu/dp_simple.c \
SylixOS/arch/mips/fpu/emu/dp_sqrt.c \
SylixOS/arch/mips/fpu/emu/dp_sub.c \
SylixOS/arch/mips/fpu/emu/dp_tint.c \
SylixOS/arch/mips/fpu/emu/dp_tlong.c \
SylixOS/arch/mips/fpu/emu/dsemul.c \
SylixOS/arch/mips/fpu/emu/ieee754.c \
SylixOS/arch/mips/fpu/emu/ieee754d.c \
SylixOS/arch/mips/fpu/emu/ieee754dp.c \
SylixOS/arch/mips/fpu/emu/ieee754sp.c \
SylixOS/arch/mips/fpu/emu/sp_2008class.c \
SylixOS/arch/mips/fpu/emu/sp_add.c \
SylixOS/arch/mips/fpu/emu/sp_cmp.c \
SylixOS/arch/mips/fpu/emu/sp_div.c \
SylixOS/arch/mips/fpu/emu/sp_fdp.c \
SylixOS/arch/mips/fpu/emu/sp_fint.c \
SylixOS/arch/mips/fpu/emu/sp_flong.c \
SylixOS/arch/mips/fpu/emu/sp_fmax.c \
SylixOS/arch/mips/fpu/emu/sp_fmin.c \
SylixOS/arch/mips/fpu/emu/sp_maddf.c \
SylixOS/arch/mips/fpu/emu/sp_mul.c \
SylixOS/arch/mips/fpu/emu/sp_simple.c \
SylixOS/arch/mips/fpu/emu/sp_sqrt.c \
SylixOS/arch/mips/fpu/emu/sp_sub.c \
SylixOS/arch/mips/fpu/emu/sp_tint.c \
SylixOS/arch/mips/fpu/emu/sp_tlong.c \
SylixOS/arch/mips/fpu/mipsFpu.c \
SylixOS/arch/mips/mm/cache/r4k/mipsCacheR4k.c \
SylixOS/arch/mips/mm/cache/r4k/mipsCacheR4kAsm.S \
SylixOS/arch/mips/mm/cache/loongson3x/mipsCacheLs3x.c \
SylixOS/arch/mips/mm/cache/loongson3x/mipsCacheLs3xAsm.S \
SylixOS/arch/mips/mm/cache/l2/mipsL2R4k.c \
SylixOS/arch/mips/mm/cache/l2/mipsL2R4kAsm.S \
SylixOS/arch/mips/mm/mmu/mips32/mips32Mmu.c \
SylixOS/arch/mips/mm/mmu/mips32/mips32MmuAsm.S \
SylixOS/arch/mips/mm/mipsCache.c \
SylixOS/arch/mips/mm/mipsMmu.c \
SylixOS/arch/mips/mpcore/mipsMpCoreAsm.S \
SylixOS/arch/mips/mpcore/mipsSpinlock.c \
SylixOS/arch/mips/param/mipsParam.c 

#*********************************************************************************************************
# PowerPC source
#*********************************************************************************************************
LOCAL_PPC_SRCS = \
SylixOS/arch/ppc/backtrace/ppcBacktrace.c \
SylixOS/arch/ppc/common/ppcAssert.c \
SylixOS/arch/ppc/common/ppcContext.c \
SylixOS/arch/ppc/common/ppcContextAsm.S \
SylixOS/arch/ppc/common/ppcExc.c \
SylixOS/arch/ppc/common/ppcExcAsm.S \
SylixOS/arch/ppc/common/ppcIo.c \
SylixOS/arch/ppc/common/ppcLib.c \
SylixOS/arch/ppc/common/ppcLibAsm.S \
SylixOS/arch/ppc/common/ppcSprAsm.S \
SylixOS/arch/ppc/common/e500/ppcSprE500Asm.S \
SylixOS/arch/ppc/common/e500/ppcExcE500.c \
SylixOS/arch/ppc/common/e500/ppcExcE500Asm.S \
SylixOS/arch/ppc/dbg/ppcDbg.c \
SylixOS/arch/ppc/dbg/ppcGdb.c \
SylixOS/arch/ppc/elf/ppcElf.c \
SylixOS/arch/ppc/fpu/spe/ppcVfpSpe.c \
SylixOS/arch/ppc/fpu/spe/ppcVfpSpeAsm.S \
SylixOS/arch/ppc/fpu/altivec/ppcVfpAltivec.c \
SylixOS/arch/ppc/fpu/altivec/ppcVfpAltivecAsm.S \
SylixOS/arch/ppc/fpu/vfpnone/ppcVfpNone.c \
SylixOS/arch/ppc/fpu/vfp/ppcVfp.c \
SylixOS/arch/ppc/fpu/vfp/ppcVfpAsm.S \
SylixOS/arch/ppc/fpu/ppcFpu.c \
SylixOS/arch/ppc/mm/ppcCache.c \
SylixOS/arch/ppc/mm/ppcMmu.c \
SylixOS/arch/ppc/mm/cache/common/ppcCache.c \
SylixOS/arch/ppc/mm/cache/e200/ppcCacheE200Asm.S \
SylixOS/arch/ppc/mm/cache/e200/ppcCacheE200.c \
SylixOS/arch/ppc/mm/cache/e500/ppcCacheE500Asm.S \
SylixOS/arch/ppc/mm/cache/e500/ppcCacheE500.c \
SylixOS/arch/ppc/mm/cache/ppc60x/ppcCache603Asm.S \
SylixOS/arch/ppc/mm/cache/ppc60x/ppcCacheEC603Asm.S \
SylixOS/arch/ppc/mm/cache/ppc60x/ppcCache604Asm.S \
SylixOS/arch/ppc/mm/cache/ppc60x/ppcCache745xAsm.S \
SylixOS/arch/ppc/mm/cache/ppc60x/ppcCache83xxAsm.S \
SylixOS/arch/ppc/mm/cache/ppc60x/ppcCache603.c \
SylixOS/arch/ppc/mm/cache/ppc60x/ppcCacheEC603.c \
SylixOS/arch/ppc/mm/cache/ppc60x/ppcCache604.c \
SylixOS/arch/ppc/mm/cache/ppc60x/ppcCache745x.c \
SylixOS/arch/ppc/mm/cache/ppc60x/ppcCache83xx.c \
SylixOS/arch/ppc/mm/cache/l2/ppcL2.c \
SylixOS/arch/ppc/mm/cache/l2/ppc750/ppcL2Cache750.c \
SylixOS/arch/ppc/mm/cache/l2/ppc750/ppcL2Cache750Asm.S \
SylixOS/arch/ppc/mm/cache/l2/ppc750/ppcL2Cache745xAsm.S \
SylixOS/arch/ppc/mm/mmu/common/ppcMmu.c \
SylixOS/arch/ppc/mm/mmu/common/ppcMmuAsm.S \
SylixOS/arch/ppc/mm/mmu/common/ppcMmuHashPageTbl.c \
SylixOS/arch/ppc/mm/mmu/ppc603/ppcMmu603Asm.S \
SylixOS/arch/ppc/mm/mmu/e500/ppcMmuE500.c \
SylixOS/arch/ppc/mm/mmu/e500/ppcMmuE500Asm.S \
SylixOS/arch/ppc/mm/mmu/e500/ppcMmuE500RegAsm.S \
SylixOS/arch/ppc/mm/mmu/e500/ppcMmuE500Tlb1.c \
SylixOS/arch/ppc/mm/mmu/e500/ppcMmuE500Tlb1Asm.S \
SylixOS/arch/ppc/mpcore/ppcMpCoreAsm.S \
SylixOS/arch/ppc/mpcore/ppcSpinlock.c \
SylixOS/arch/ppc/param/ppcParam.c 

#*********************************************************************************************************
# x86 common source
#*********************************************************************************************************
LOCAL_X86_COMMON_SRCS = \
SylixOS/arch/x86/acpi/x86AcpiSylixOS.c \
SylixOS/arch/x86/acpi/x86AcpiLib.c \
SylixOS/arch/x86/acpi/x86AcpiTables.c \
SylixOS/arch/x86/acpi/x86AcpiConfig.c \
SylixOS/arch/x86/acpi/x86AcpiShow.c \
SylixOS/arch/x86/acpi/acpica/debugger/dbcmds.c \
SylixOS/arch/x86/acpi/acpica/debugger/dbdisply.c \
SylixOS/arch/x86/acpi/acpica/debugger/dbexec.c \
SylixOS/arch/x86/acpi/acpica/debugger/dbfileio.c \
SylixOS/arch/x86/acpi/acpica/debugger/dbhistry.c \
SylixOS/arch/x86/acpi/acpica/debugger/dbinput.c \
SylixOS/arch/x86/acpi/acpica/debugger/dbmethod.c \
SylixOS/arch/x86/acpi/acpica/debugger/dbnames.c \
SylixOS/arch/x86/acpi/acpica/debugger/dbstats.c \
SylixOS/arch/x86/acpi/acpica/debugger/dbutils.c \
SylixOS/arch/x86/acpi/acpica/debugger/dbxface.c \
SylixOS/arch/x86/acpi/acpica/disassembler/dmbuffer.c \
SylixOS/arch/x86/acpi/acpica/disassembler/dmnames.c \
SylixOS/arch/x86/acpi/acpica/disassembler/dmobject.c \
SylixOS/arch/x86/acpi/acpica/disassembler/dmopcode.c \
SylixOS/arch/x86/acpi/acpica/disassembler/dmresrc.c \
SylixOS/arch/x86/acpi/acpica/disassembler/dmresrcl.c \
SylixOS/arch/x86/acpi/acpica/disassembler/dmresrcs.c \
SylixOS/arch/x86/acpi/acpica/disassembler/dmutils.c \
SylixOS/arch/x86/acpi/acpica/disassembler/dmwalk.c \
SylixOS/arch/x86/acpi/acpica/dispatcher/dsargs.c \
SylixOS/arch/x86/acpi/acpica/dispatcher/dscontrol.c \
SylixOS/arch/x86/acpi/acpica/dispatcher/dsfield.c \
SylixOS/arch/x86/acpi/acpica/dispatcher/dsinit.c \
SylixOS/arch/x86/acpi/acpica/dispatcher/dsmethod.c \
SylixOS/arch/x86/acpi/acpica/dispatcher/dsmthdat.c \
SylixOS/arch/x86/acpi/acpica/dispatcher/dsobject.c \
SylixOS/arch/x86/acpi/acpica/dispatcher/dsopcode.c \
SylixOS/arch/x86/acpi/acpica/dispatcher/dsutils.c \
SylixOS/arch/x86/acpi/acpica/dispatcher/dswexec.c \
SylixOS/arch/x86/acpi/acpica/dispatcher/dswload.c \
SylixOS/arch/x86/acpi/acpica/dispatcher/dswload2.c \
SylixOS/arch/x86/acpi/acpica/dispatcher/dswscope.c \
SylixOS/arch/x86/acpi/acpica/dispatcher/dswstate.c \
SylixOS/arch/x86/acpi/acpica/events/evevent.c \
SylixOS/arch/x86/acpi/acpica/events/evgpe.c \
SylixOS/arch/x86/acpi/acpica/events/evgpeblk.c \
SylixOS/arch/x86/acpi/acpica/events/evgpeinit.c \
SylixOS/arch/x86/acpi/acpica/events/evgpeutil.c \
SylixOS/arch/x86/acpi/acpica/events/evmisc.c \
SylixOS/arch/x86/acpi/acpica/events/evregion.c \
SylixOS/arch/x86/acpi/acpica/events/evrgnini.c \
SylixOS/arch/x86/acpi/acpica/events/evsci.c \
SylixOS/arch/x86/acpi/acpica/events/evxface.c \
SylixOS/arch/x86/acpi/acpica/events/evxfevnt.c \
SylixOS/arch/x86/acpi/acpica/events/evxfgpe.c \
SylixOS/arch/x86/acpi/acpica/events/evxfregn.c \
SylixOS/arch/x86/acpi/acpica/executer/exconfig.c \
SylixOS/arch/x86/acpi/acpica/executer/exconvrt.c \
SylixOS/arch/x86/acpi/acpica/executer/excreate.c \
SylixOS/arch/x86/acpi/acpica/executer/exdebug.c \
SylixOS/arch/x86/acpi/acpica/executer/exdump.c \
SylixOS/arch/x86/acpi/acpica/executer/exfield.c \
SylixOS/arch/x86/acpi/acpica/executer/exfldio.c \
SylixOS/arch/x86/acpi/acpica/executer/exmisc.c \
SylixOS/arch/x86/acpi/acpica/executer/exmutex.c \
SylixOS/arch/x86/acpi/acpica/executer/exnames.c \
SylixOS/arch/x86/acpi/acpica/executer/exoparg1.c \
SylixOS/arch/x86/acpi/acpica/executer/exoparg2.c \
SylixOS/arch/x86/acpi/acpica/executer/exoparg3.c \
SylixOS/arch/x86/acpi/acpica/executer/exoparg6.c \
SylixOS/arch/x86/acpi/acpica/executer/exprep.c \
SylixOS/arch/x86/acpi/acpica/executer/exregion.c \
SylixOS/arch/x86/acpi/acpica/executer/exresnte.c \
SylixOS/arch/x86/acpi/acpica/executer/exresolv.c \
SylixOS/arch/x86/acpi/acpica/executer/exresop.c \
SylixOS/arch/x86/acpi/acpica/executer/exstore.c \
SylixOS/arch/x86/acpi/acpica/executer/exstoren.c \
SylixOS/arch/x86/acpi/acpica/executer/exstorob.c \
SylixOS/arch/x86/acpi/acpica/executer/exsystem.c \
SylixOS/arch/x86/acpi/acpica/executer/exutils.c \
SylixOS/arch/x86/acpi/acpica/hardware/hwacpi.c \
SylixOS/arch/x86/acpi/acpica/hardware/hwgpe.c \
SylixOS/arch/x86/acpi/acpica/hardware/hwpci.c \
SylixOS/arch/x86/acpi/acpica/hardware/hwregs.c \
SylixOS/arch/x86/acpi/acpica/hardware/hwsleep.c \
SylixOS/arch/x86/acpi/acpica/hardware/hwtimer.c \
SylixOS/arch/x86/acpi/acpica/hardware/hwvalid.c \
SylixOS/arch/x86/acpi/acpica/hardware/hwxface.c \
SylixOS/arch/x86/acpi/acpica/namespace/nsaccess.c \
SylixOS/arch/x86/acpi/acpica/namespace/nsalloc.c \
SylixOS/arch/x86/acpi/acpica/namespace/nsdump.c \
SylixOS/arch/x86/acpi/acpica/namespace/nsdumpdv.c \
SylixOS/arch/x86/acpi/acpica/namespace/nseval.c \
SylixOS/arch/x86/acpi/acpica/namespace/nsinit.c \
SylixOS/arch/x86/acpi/acpica/namespace/nsload.c \
SylixOS/arch/x86/acpi/acpica/namespace/nsnames.c \
SylixOS/arch/x86/acpi/acpica/namespace/nsobject.c \
SylixOS/arch/x86/acpi/acpica/namespace/nsparse.c \
SylixOS/arch/x86/acpi/acpica/namespace/nspredef.c \
SylixOS/arch/x86/acpi/acpica/namespace/nsrepair.c \
SylixOS/arch/x86/acpi/acpica/namespace/nsrepair2.c \
SylixOS/arch/x86/acpi/acpica/namespace/nssearch.c \
SylixOS/arch/x86/acpi/acpica/namespace/nsutils.c \
SylixOS/arch/x86/acpi/acpica/namespace/nswalk.c \
SylixOS/arch/x86/acpi/acpica/namespace/nsxfeval.c \
SylixOS/arch/x86/acpi/acpica/namespace/nsxfname.c \
SylixOS/arch/x86/acpi/acpica/namespace/nsxfobj.c \
SylixOS/arch/x86/acpi/acpica/parser/psargs.c \
SylixOS/arch/x86/acpi/acpica/parser/psloop.c \
SylixOS/arch/x86/acpi/acpica/parser/psopcode.c \
SylixOS/arch/x86/acpi/acpica/parser/psparse.c \
SylixOS/arch/x86/acpi/acpica/parser/psscope.c \
SylixOS/arch/x86/acpi/acpica/parser/pstree.c \
SylixOS/arch/x86/acpi/acpica/parser/psutils.c \
SylixOS/arch/x86/acpi/acpica/parser/pswalk.c \
SylixOS/arch/x86/acpi/acpica/parser/psxface.c \
SylixOS/arch/x86/acpi/acpica/resources/rsaddr.c \
SylixOS/arch/x86/acpi/acpica/resources/rscalc.c \
SylixOS/arch/x86/acpi/acpica/resources/rscreate.c \
SylixOS/arch/x86/acpi/acpica/resources/rsdump.c \
SylixOS/arch/x86/acpi/acpica/resources/rsinfo.c \
SylixOS/arch/x86/acpi/acpica/resources/rsio.c \
SylixOS/arch/x86/acpi/acpica/resources/rsirq.c \
SylixOS/arch/x86/acpi/acpica/resources/rslist.c \
SylixOS/arch/x86/acpi/acpica/resources/rsmemory.c \
SylixOS/arch/x86/acpi/acpica/resources/rsmisc.c \
SylixOS/arch/x86/acpi/acpica/resources/rsutils.c \
SylixOS/arch/x86/acpi/acpica/resources/rsxface.c \
SylixOS/arch/x86/acpi/acpica/tables/tbfadt.c \
SylixOS/arch/x86/acpi/acpica/tables/tbfind.c \
SylixOS/arch/x86/acpi/acpica/tables/tbinstal.c \
SylixOS/arch/x86/acpi/acpica/tables/tbutils.c \
SylixOS/arch/x86/acpi/acpica/tables/tbxface.c \
SylixOS/arch/x86/acpi/acpica/tables/tbxfroot.c \
SylixOS/arch/x86/acpi/acpica/utilities/utalloc.c \
SylixOS/arch/x86/acpi/acpica/utilities/utcache.c \
SylixOS/arch/x86/acpi/acpica/utilities/utclib.c \
SylixOS/arch/x86/acpi/acpica/utilities/utcopy.c \
SylixOS/arch/x86/acpi/acpica/utilities/utdebug.c \
SylixOS/arch/x86/acpi/acpica/utilities/utdecode.c \
SylixOS/arch/x86/acpi/acpica/utilities/utdelete.c \
SylixOS/arch/x86/acpi/acpica/utilities/uteval.c \
SylixOS/arch/x86/acpi/acpica/utilities/utglobal.c \
SylixOS/arch/x86/acpi/acpica/utilities/utids.c \
SylixOS/arch/x86/acpi/acpica/utilities/utinit.c \
SylixOS/arch/x86/acpi/acpica/utilities/utlock.c \
SylixOS/arch/x86/acpi/acpica/utilities/utmath.c \
SylixOS/arch/x86/acpi/acpica/utilities/utmisc.c \
SylixOS/arch/x86/acpi/acpica/utilities/utmutex.c \
SylixOS/arch/x86/acpi/acpica/utilities/utobject.c \
SylixOS/arch/x86/acpi/acpica/utilities/utosi.c \
SylixOS/arch/x86/acpi/acpica/utilities/utresrc.c \
SylixOS/arch/x86/acpi/acpica/utilities/utstate.c \
SylixOS/arch/x86/acpi/acpica/utilities/uttrack.c \
SylixOS/arch/x86/acpi/acpica/utilities/utxface.c \
SylixOS/arch/x86/acpi/acpica/utilities/utxferror.c \
SylixOS/arch/x86/apic/x86IoApic.c \
SylixOS/arch/x86/apic/x86LocalApic.c \
SylixOS/arch/x86/apic/x86LocalApicAsm.S \
SylixOS/arch/x86/backtrace/x86Backtrace.c \
SylixOS/arch/x86/bsp/bspLib.c \
SylixOS/arch/x86/bsp/bspInt.c \
SylixOS/arch/x86/bsp/bspSmp.c \
SylixOS/arch/x86/bsp/bspTime8254.c \
SylixOS/arch/x86/bsp/bspTimeHpet.c \
SylixOS/arch/x86/common/x86Assert.c \
SylixOS/arch/x86/common/x86CpuId.c \
SylixOS/arch/x86/common/x86Exc.c \
SylixOS/arch/x86/common/x86Gdt.c \
SylixOS/arch/x86/common/x86Idt.c \
SylixOS/arch/x86/common/x86Lib.c \
SylixOS/arch/x86/common/x86Topology.c \
SylixOS/arch/x86/dbg/x86Dbg.c \
SylixOS/arch/x86/fpu/fpunone/x86FpuNone.c \
SylixOS/arch/x86/fpu/fpusse/x86FpuSse.c \
SylixOS/arch/x86/fpu/x86Fpu.c \
SylixOS/arch/x86/mm/cache/x86Cache.c \
SylixOS/arch/x86/mm/x86Cache.c \
SylixOS/arch/x86/mm/x86Mmu.c \
SylixOS/arch/x86/mpconfig/x86MpApic.c \
SylixOS/arch/x86/mpcore/x86MpCore.c \
SylixOS/arch/x86/mpcore/x86Spinlock.c \
SylixOS/arch/x86/pentium/x86Pentium.c \
SylixOS/arch/x86/param/x86Param.c

#*********************************************************************************************************
# x86 source
#*********************************************************************************************************
LOCAL_X86_SRCS := $(LOCAL_X86_COMMON_SRCS) 
LOCAL_X86_SRCS += \
SylixOS/arch/x86/dbg/x86Gdb.c \
SylixOS/arch/x86/elf/x86Elf.c \
SylixOS/arch/x86/common/x86ContextAsm.S \
SylixOS/arch/x86/common/x86CpuIdAsm.S \
SylixOS/arch/x86/common/x86ExcAsm.S \
SylixOS/arch/x86/common/x86Context.c \
SylixOS/arch/x86/common/x86IoAsm.S \
SylixOS/arch/x86/common/x86LibAsm.S \
SylixOS/arch/x86/common/x86CrAsm.S \
SylixOS/arch/x86/fpu/fpusse/x86FpuSseAsm.S \
SylixOS/arch/x86/mm/cache/x86CacheAsm.S \
SylixOS/arch/x86/mm/mmu/x86Mmu.c \
SylixOS/arch/x86/mm/mmu/x86MmuAsm.S \
SylixOS/arch/x86/mpcore/x86MpCoreAsm.S \
SylixOS/arch/x86/pentium/x86PentiumAsm.S

#*********************************************************************************************************
# x86-64 source
#*********************************************************************************************************
LOCAL_X64_SRCS := $(LOCAL_X86_COMMON_SRCS) 
LOCAL_X64_SRCS += \
SylixOS/arch/x86/dbg/x64Gdb.c \
SylixOS/arch/x86/elf/x64Elf.c \
SylixOS/arch/x86/common/x64/x64ContextAsm.S \
SylixOS/arch/x86/common/x64/x64CpuIdAsm.S \
SylixOS/arch/x86/common/x64/x64ExcAsm.S \
SylixOS/arch/x86/common/x64/x64Context.c \
SylixOS/arch/x86/common/x64/x64IoAsm.S \
SylixOS/arch/x86/common/x64/x64LibAsm.S \
SylixOS/arch/x86/common/x64/x64CrAsm.S \
SylixOS/arch/x86/fpu/fpusse/x64FpuSseAsm.S \
SylixOS/arch/x86/mm/cache/x64CacheAsm.S \
SylixOS/arch/x86/mm/mmu/x64Mmu.c \
SylixOS/arch/x86/mm/mmu/x64MmuAsm.S \
SylixOS/arch/x86/mpcore/x64MpCoreAsm.S \
SylixOS/arch/x86/pentium/x64PentiumAsm.S

#*********************************************************************************************************
# TI C6X DSP source
#*********************************************************************************************************
LOCAL_C6X_SRCS = \
SylixOS/arch/c6x/backtrace/c6xBacktrace.c \
SylixOS/arch/c6x/common/c6xAssert.c \
SylixOS/arch/c6x/common/c6xContext.c \
SylixOS/arch/c6x/common/c6xExc.c \
SylixOS/arch/c6x/common/c6xIo.c \
SylixOS/arch/c6x/common/c6xLib.c \
SylixOS/arch/c6x/common/c6xContextAsm.asm \
SylixOS/arch/c6x/common/c6xExcAsm.asm \
SylixOS/arch/c6x/common/c6xLibAsm.asm \
SylixOS/arch/c6x/dbg/c6xDbg.c \
SylixOS/arch/c6x/dbg/c6xGdb.c \
SylixOS/arch/c6x/elf/c6xElf.c \
SylixOS/arch/c6x/mm/c6xCache.c \
SylixOS/arch/c6x/param/c6xParam.c

#*********************************************************************************************************
# SPARC source
#*********************************************************************************************************
LOCAL_SPARC_SRCS = \
SylixOS/arch/sparc/backtrace/sparcBacktrace.c \
SylixOS/arch/sparc/common/sparcAssert.c \
SylixOS/arch/sparc/common/sparcContext.c \
SylixOS/arch/sparc/common/sparcContextAsm.S \
SylixOS/arch/sparc/common/sparcExc.c \
SylixOS/arch/sparc/common/sparcExcAsm.S \
SylixOS/arch/sparc/common/sparcIo.c \
SylixOS/arch/sparc/common/sparcLib.c \
SylixOS/arch/sparc/common/sparcLibAsm.S \
SylixOS/arch/sparc/common/sparcUnaligned.c \
SylixOS/arch/sparc/common/sparcUnalignedAsm.S \
SylixOS/arch/sparc/common/sparcVectorAsm.S \
SylixOS/arch/sparc/common/sparcWindowAsm.S \
SylixOS/arch/sparc/dbg/sparcDbg.c \
SylixOS/arch/sparc/dbg/sparcGdb.c \
SylixOS/arch/sparc/elf/sparcElf.c \
SylixOS/arch/sparc/fpu/sparcFpu.c \
SylixOS/arch/sparc/fpu/vfp/sparcVfp.c \
SylixOS/arch/sparc/fpu/vfp/sparcVfpAsm.S \
SylixOS/arch/sparc/fpu/vfpnone/sparcVfpNone.c \
SylixOS/arch/sparc/mm/cache/sparcCache.c \
SylixOS/arch/sparc/mm/mmu/sparcMmu.c \
SylixOS/arch/sparc/mm/mmu/sparcMmuAsm.S \
SylixOS/arch/sparc/mm/sparcCache.c \
SylixOS/arch/sparc/mm/sparcMmu.c \
SylixOS/arch/sparc/mpcore/sparcMpCoreAsm.S \
SylixOS/arch/sparc/mpcore/sparcSpinlock.c \
SylixOS/arch/sparc/param/sparcParam.c \

#*********************************************************************************************************
# Buildin internal application source
#*********************************************************************************************************
ifeq ($(BUILD_LITE_TARGET), 0)
APPL_SRCS = \
SylixOS/appl/editors/vi/vi_fix.c \
SylixOS/appl/editors/vi/vi_sylixos.c \
SylixOS/appl/editors/vi/src/vi.c \
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
SylixOS/appl/ssl/mbedtls/library/certs.c \
SylixOS/appl/ssl/mbedtls/library/cipher.c \
SylixOS/appl/ssl/mbedtls/library/cipher_wrap.c \
SylixOS/appl/ssl/mbedtls/library/cmac.c \
SylixOS/appl/ssl/mbedtls/library/ctr_drbg.c \
SylixOS/appl/ssl/mbedtls/library/debug.c \
SylixOS/appl/ssl/mbedtls/library/des.c \
SylixOS/appl/ssl/mbedtls/library/dhm.c \
SylixOS/appl/ssl/mbedtls/library/ecdh.c \
SylixOS/appl/ssl/mbedtls/library/ecdsa.c \
SylixOS/appl/ssl/mbedtls/library/ecjpake.c \
SylixOS/appl/ssl/mbedtls/library/ecp_curves.c \
SylixOS/appl/ssl/mbedtls/library/ecp_mbed.c \
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
SylixOS/appl/ssl/mbedtls/library/net_sockets.c \
SylixOS/appl/ssl/mbedtls/library/oid.c \
SylixOS/appl/ssl/mbedtls/library/padlock.c \
SylixOS/appl/ssl/mbedtls/library/pem.c \
SylixOS/appl/ssl/mbedtls/library/pk.c \
SylixOS/appl/ssl/mbedtls/library/pkcs11.c \
SylixOS/appl/ssl/mbedtls/library/pkcs12.c \
SylixOS/appl/ssl/mbedtls/library/pkcs5.c \
SylixOS/appl/ssl/mbedtls/library/pkparse.c \
SylixOS/appl/ssl/mbedtls/library/pkwrite.c \
SylixOS/appl/ssl/mbedtls/library/pk_wrap.c \
SylixOS/appl/ssl/mbedtls/library/platform.c \
SylixOS/appl/ssl/mbedtls/library/ripemd160.c \
SylixOS/appl/ssl/mbedtls/library/rsa.c \
SylixOS/appl/ssl/mbedtls/library/sha1.c \
SylixOS/appl/ssl/mbedtls/library/sha256.c \
SylixOS/appl/ssl/mbedtls/library/sha512.c \
SylixOS/appl/ssl/mbedtls/library/ssl_cache.c \
SylixOS/appl/ssl/mbedtls/library/ssl_ciphersuites.c \
SylixOS/appl/ssl/mbedtls/library/ssl_cli.c \
SylixOS/appl/ssl/mbedtls/library/ssl_cookie.c \
SylixOS/appl/ssl/mbedtls/library/ssl_srv.c \
SylixOS/appl/ssl/mbedtls/library/ssl_ticket.c \
SylixOS/appl/ssl/mbedtls/library/ssl_tls.c \
SylixOS/appl/ssl/mbedtls/library/threading.c \
SylixOS/appl/ssl/mbedtls/library/timing.c \
SylixOS/appl/ssl/mbedtls/library/version.c \
SylixOS/appl/ssl/mbedtls/library/version_features.c \
SylixOS/appl/ssl/mbedtls/library/x509.c \
SylixOS/appl/ssl/mbedtls/library/x509write_crt.c \
SylixOS/appl/ssl/mbedtls/library/x509write_csr.c \
SylixOS/appl/ssl/mbedtls/library/x509_create.c \
SylixOS/appl/ssl/mbedtls/library/x509_crl.c \
SylixOS/appl/ssl/mbedtls/library/x509_crt.c \
SylixOS/appl/ssl/mbedtls/library/x509_csr.c \
SylixOS/appl/ssl/mbedtls/library/xtea.c \
SylixOS/appl/zip/zlib/zlib_sylixos.c \
SylixOS/appl/zip/zlib/src/adler32.c \
SylixOS/appl/zip/zlib/src/compress.c \
SylixOS/appl/zip/zlib/src/crc32.c \
SylixOS/appl/zip/zlib/src/deflate.c \
SylixOS/appl/zip/zlib/src/example.c \
SylixOS/appl/zip/zlib/src/gzclose.c \
SylixOS/appl/zip/zlib/src/gzlib.c \
SylixOS/appl/zip/zlib/src/gzread.c \
SylixOS/appl/zip/zlib/src/gzwrite.c \
SylixOS/appl/zip/zlib/src/infback.c \
SylixOS/appl/zip/zlib/src/inffast.c \
SylixOS/appl/zip/zlib/src/inflate.c \
SylixOS/appl/zip/zlib/src/inftrees.c \
SylixOS/appl/zip/zlib/src/minigzip.c \
SylixOS/appl/zip/zlib/src/trees.c \
SylixOS/appl/zip/zlib/src/uncompr.c \
SylixOS/appl/zip/zlib/src/zutil.c
else
APPL_SRCS = \
SylixOS/appl/editors/vi/vi_fix.c \
SylixOS/appl/editors/vi/vi_sylixos.c \
SylixOS/appl/editors/vi/src/vi.c 
endif

#*********************************************************************************************************
# Debug source
#*********************************************************************************************************
DEBUG_SRCS = \
SylixOS/debug/dtrace/dtrace.c \
SylixOS/debug/gdb/gdbserver.c \
SylixOS/debug/hwdbg/gdbmodule.c \
SylixOS/debug/hwdbg/openocd.c 

#*********************************************************************************************************
# Drv source
#*********************************************************************************************************
DRV_SRCS = \
SylixOS/driver/can/sja1000.c \
SylixOS/driver/dma/i8237a.c \
SylixOS/driver/int/i8259a.c \
SylixOS/driver/net/dm9000.c \
SylixOS/driver/net/smethnd.c \
SylixOS/driver/pci/sio/pciSioExar.c \
SylixOS/driver/pci/sio/pciSioNetmos.c \
SylixOS/driver/pci/storage/pciStorageAta.c \
SylixOS/driver/pci/storage/pciStorageNvme.c \
SylixOS/driver/pci/storage/pciStorageSata.c \
SylixOS/driver/sio/16c550.c \
SylixOS/driver/timer/i8254.c 

#*********************************************************************************************************
# File system source
#*********************************************************************************************************
FS_SRCS = \
SylixOS/fs/diskCache/diskCache.c \
SylixOS/fs/diskCache/diskCacheLib.c \
SylixOS/fs/diskCache/diskCachePipeline.c \
SylixOS/fs/diskCache/diskCacheProc.c \
SylixOS/fs/diskCache/diskCacheThread.c \
SylixOS/fs/diskPartition/diskPartition.c \
SylixOS/fs/diskRaid/diskRaid0.c \
SylixOS/fs/diskRaid/diskRaid1.c \
SylixOS/fs/diskRaid/diskRaidLib.c \
SylixOS/fs/fatFs/diskio.c \
SylixOS/fs/fatFs/fatFs.c \
SylixOS/fs/fatFs/fatstat.c \
SylixOS/fs/fatFs/fattime.c \
SylixOS/fs/fatFs/ff.c \
SylixOS/fs/fatFs/option/unicode.c \
SylixOS/fs/fatFs/option/syscall.c \
SylixOS/fs/fsCommon/fsCommon.c \
SylixOS/fs/mount/mount.c \
SylixOS/fs/mtd/mtdcore.c \
SylixOS/fs/mtd/linux/bch.c \
SylixOS/fs/mtd/linux/strim.c \
SylixOS/fs/mtd/nand/nand_base.c \
SylixOS/fs/mtd/nand/nand_bbt.c \
SylixOS/fs/mtd/nand/nand_bch.c \
SylixOS/fs/mtd/nand/nand_ecc.c \
SylixOS/fs/mtd/nand/nand_ids.c \
SylixOS/fs/mtd/onenand/onenand_base.c \
SylixOS/fs/mtd/onenand/onenand_bbt.c \
SylixOS/fs/nandRCache/nandRCache.c \
SylixOS/fs/nfs/mount_clnt.c \
SylixOS/fs/nfs/nfs_clnt.c \
SylixOS/fs/nfs/nfs_sylixos.c \
SylixOS/fs/nfs/nfs_xdr.c \
SylixOS/fs/oemDisk/oemBlkIo.c \
SylixOS/fs/oemDisk/oemDisk.c \
SylixOS/fs/oemDisk/oemFdisk.c \
SylixOS/fs/oemDisk/oemGrub.c \
SylixOS/fs/procFs/procFs.c \
SylixOS/fs/procFs/procFsLib.c \
SylixOS/fs/procFs/procBsp/procBsp.c \
SylixOS/fs/procFs/procFssup/procFssup.c \
SylixOS/fs/procFs/procHook/procHook.c \
SylixOS/fs/procFs/procKernel/procKernel.c \
SylixOS/fs/procFs/procPower/procPower.c \
SylixOS/fs/ramFs/ramFs.c \
SylixOS/fs/ramFs/ramFsLib.c \
SylixOS/fs/romFs/romFs.c \
SylixOS/fs/romFs/romFsLib.c \
SylixOS/fs/rootFs/rootFs.c \
SylixOS/fs/rootFs/rootFsLib.c \
SylixOS/fs/rootFs/rootFsMap.c \
SylixOS/fs/tpsFs/tpsfs_btree.c \
SylixOS/fs/tpsFs/tpsfs_dev_buf.c \
SylixOS/fs/tpsFs/tpsfs_dir.c \
SylixOS/fs/tpsFs/tpsfs_inode.c \
SylixOS/fs/tpsFs/tpsfs_super.c \
SylixOS/fs/tpsFs/tpsfs_sylixos.c \
SylixOS/fs/tpsFs/tpsfs_trans.c \
SylixOS/fs/tpsFs/tpsfs.c \
SylixOS/fs/unique/unique.c \
SylixOS/fs/yaffs2/yaffs_allocator.c \
SylixOS/fs/yaffs2/yaffs_attribs.c \
SylixOS/fs/yaffs2/yaffs_bitmap.c \
SylixOS/fs/yaffs2/yaffs_checkptrw.c \
SylixOS/fs/yaffs2/yaffs_ecc.c \
SylixOS/fs/yaffs2/yaffs_guts.c \
SylixOS/fs/yaffs2/yaffs_mtdif_multi.c \
SylixOS/fs/yaffs2/yaffs_nameval.c \
SylixOS/fs/yaffs2/yaffs_nand.c \
SylixOS/fs/yaffs2/yaffs_nor.c \
SylixOS/fs/yaffs2/yaffs_packedtags1.c \
SylixOS/fs/yaffs2/yaffs_packedtags2.c \
SylixOS/fs/yaffs2/yaffs_summary.c \
SylixOS/fs/yaffs2/yaffs_sylixos.c \
SylixOS/fs/yaffs2/yaffs_sylixosproc.c \
SylixOS/fs/yaffs2/yaffs_tagscompat.c \
SylixOS/fs/yaffs2/yaffs_tagsmarshall.c \
SylixOS/fs/yaffs2/yaffs_verify.c \
SylixOS/fs/yaffs2/yaffs_yaffs1.c \
SylixOS/fs/yaffs2/yaffs_yaffs2.c \
SylixOS/fs/yaffs2/direct/yaffscfg.c \
SylixOS/fs/yaffs2/direct/yaffsfs.c \
SylixOS/fs/yaffs2/direct/yaffs_hweight.c \
SylixOS/fs/yaffs2/direct/yaffs_qsort.c

#*********************************************************************************************************
# GUI tools source
#*********************************************************************************************************
GUI_SRCS = \
SylixOS/gui/input_device/input_device.c

#*********************************************************************************************************
# Kernel source
#*********************************************************************************************************
KERN_SRCS = \
SylixOS/kernel/cache/cache.c \
SylixOS/kernel/cdump/cdump.c \
SylixOS/kernel/cdump/cdumpLib.c \
SylixOS/kernel/core/_BitmapLib.c \
SylixOS/kernel/core/_CandTable.c \
SylixOS/kernel/core/_CoroutineLib.c \
SylixOS/kernel/core/_CoroutineShell.c \
SylixOS/kernel/core/_CpuLib.c \
SylixOS/kernel/core/_ErrorLib.c \
SylixOS/kernel/core/_EventHighLevel.c \
SylixOS/kernel/core/_EventInit.c \
SylixOS/kernel/core/_EventQueue.c \
SylixOS/kernel/core/_EventSetBlock.c \
SylixOS/kernel/core/_EventSetInit.c \
SylixOS/kernel/core/_EventSetReady.c \
SylixOS/kernel/core/_EventSetUnQueue.c \
SylixOS/kernel/core/_GlobalInit.c \
SylixOS/kernel/core/_HeapInit.c \
SylixOS/kernel/core/_HeapLib.c \
SylixOS/kernel/core/_IdleThread.c \
SylixOS/kernel/core/_InterVectInit.c \
SylixOS/kernel/core/_ITimerThread.c \
SylixOS/kernel/core/_JobQueue.c \
SylixOS/kernel/core/_KernelHighLevelInit.c \
SylixOS/kernel/core/_KernelLowLevelInit.c \
SylixOS/kernel/core/_KernelStatus.c \
SylixOS/kernel/core/_MsgQueueInit.c \
SylixOS/kernel/core/_MsgQueueLib.c \
SylixOS/kernel/core/_Object.c \
SylixOS/kernel/core/_PartitionInit.c \
SylixOS/kernel/core/_PartitionLib.c \
SylixOS/kernel/core/_PriorityInit.c \
SylixOS/kernel/core/_ReadyRing.c \
SylixOS/kernel/core/_ReadyTableInit.c \
SylixOS/kernel/core/_ReadyTableLib.c \
SylixOS/kernel/core/_RmsInit.c \
SylixOS/kernel/core/_RmsLib.c \
SylixOS/kernel/core/_RtcInit.c \
SylixOS/kernel/core/_Sched.c \
SylixOS/kernel/core/_SchedCand.c \
SylixOS/kernel/core/_SmpIpi.c \
SylixOS/kernel/core/_SmpSpinlock.c \
SylixOS/kernel/core/_SmpSpinlockKernel.c \
SylixOS/kernel/core/_StackCheckInit.c \
SylixOS/kernel/core/_StackMem.c \
SylixOS/kernel/core/_ThreadAffinity.c \
SylixOS/kernel/core/_ThreadFpu.c \
SylixOS/kernel/core/_ThreadIdInit.c \
SylixOS/kernel/core/_ThreadInit.c \
SylixOS/kernel/core/_ThreadJoinLib.c \
SylixOS/kernel/core/_ThreadMiscLib.c \
SylixOS/kernel/core/_ThreadSafeLib.c \
SylixOS/kernel/core/_ThreadShell.c \
SylixOS/kernel/core/_ThreadStatus.c \
SylixOS/kernel/core/_ThreadVarInit.c \
SylixOS/kernel/core/_ThreadVarLib.c \
SylixOS/kernel/core/_TimeCvt.c \
SylixOS/kernel/core/_TimerInit.c \
SylixOS/kernel/core/_TimeTick.c \
SylixOS/kernel/core/_UpSpinlock.c \
SylixOS/kernel/core/_UpSpinlockKernel.c \
SylixOS/kernel/core/_WakeupLine.c \
SylixOS/kernel/interface/CoroutineCreate.c \
SylixOS/kernel/interface/CoroutineDelete.c \
SylixOS/kernel/interface/CoroutineResume.c \
SylixOS/kernel/interface/CoroutineStackCheck.c \
SylixOS/kernel/interface/CoroutineYield.c \
SylixOS/kernel/interface/CpuActive.c \
SylixOS/kernel/interface/CpuPerf.c \
SylixOS/kernel/interface/CpuPower.c \
SylixOS/kernel/interface/EventSetCreate.c \
SylixOS/kernel/interface/EventSetDelete.c \
SylixOS/kernel/interface/EventSetGet.c \
SylixOS/kernel/interface/EventSetGetName.c \
SylixOS/kernel/interface/EventSetSet.c \
SylixOS/kernel/interface/EventSetStatus.c \
SylixOS/kernel/interface/EventSetTryGet.c \
SylixOS/kernel/interface/GetLastError.c \
SylixOS/kernel/interface/InterContext.c \
SylixOS/kernel/interface/InterDefer.c \
SylixOS/kernel/interface/InterEnterExit.c \
SylixOS/kernel/interface/InterLock.c \
SylixOS/kernel/interface/InterPriority.c \
SylixOS/kernel/interface/InterStack.c \
SylixOS/kernel/interface/InterTarget.c \
SylixOS/kernel/interface/InterVectorBase.c \
SylixOS/kernel/interface/InterVectorConnect.c \
SylixOS/kernel/interface/InterVectorEnable.c \
SylixOS/kernel/interface/InterVectorFlag.c \
SylixOS/kernel/interface/InterVectorIsr.c \
SylixOS/kernel/interface/InterVectorMeasure.c \
SylixOS/kernel/interface/KernelAtomic.c \
SylixOS/kernel/interface/KernelFpu.c \
SylixOS/kernel/interface/KernelGetKid.c \
SylixOS/kernel/interface/KernelGetPriority.c \
SylixOS/kernel/interface/KernelGetThreadNum.c \
SylixOS/kernel/interface/KernelHeapInfo.c \
SylixOS/kernel/interface/KernelHook.c \
SylixOS/kernel/interface/KernelHookDelete.c \
SylixOS/kernel/interface/KernelIpi.c \
SylixOS/kernel/interface/KernelIsRunning.c \
SylixOS/kernel/interface/KernelMisc.c \
SylixOS/kernel/interface/KernelObject.c \
SylixOS/kernel/interface/KernelParam.c \
SylixOS/kernel/interface/KernelReboot.c \
SylixOS/kernel/interface/KernelResume.c \
SylixOS/kernel/interface/KernelSpinlock.c \
SylixOS/kernel/interface/KernelStart.c \
SylixOS/kernel/interface/KernelSuspend.c \
SylixOS/kernel/interface/KernelTicks.c \
SylixOS/kernel/interface/KernelUuid.c \
SylixOS/kernel/interface/KernelVersion.c \
SylixOS/kernel/interface/MsgQueueClear.c \
SylixOS/kernel/interface/MsgQueueCreate.c \
SylixOS/kernel/interface/MsgQueueDelete.c \
SylixOS/kernel/interface/MsgQueueFlush.c \
SylixOS/kernel/interface/MsgQueueGetName.c \
SylixOS/kernel/interface/MsgQueueReceive.c \
SylixOS/kernel/interface/MsgQueueSend.c \
SylixOS/kernel/interface/MsgQueueSendEx.c \
SylixOS/kernel/interface/MsgQueueStatus.c \
SylixOS/kernel/interface/MsgQueueStatusEx.c \
SylixOS/kernel/interface/MsgQueueTryReceive.c \
SylixOS/kernel/interface/PartitionCreate.c \
SylixOS/kernel/interface/PartitionDelete.c \
SylixOS/kernel/interface/PartitionGet.c \
SylixOS/kernel/interface/PartitionGetName.c \
SylixOS/kernel/interface/PartitionPut.c \
SylixOS/kernel/interface/PartitionStatus.c \
SylixOS/kernel/interface/RegionAddMem.c \
SylixOS/kernel/interface/RegionCreate.c \
SylixOS/kernel/interface/RegionDelete.c \
SylixOS/kernel/interface/RegionGet.c \
SylixOS/kernel/interface/RegionGetAlign.c \
SylixOS/kernel/interface/RegionGetName.c \
SylixOS/kernel/interface/RegionPut.c \
SylixOS/kernel/interface/RegionReget.c \
SylixOS/kernel/interface/RegionStatus.c \
SylixOS/kernel/interface/RegionStatusEx.c \
SylixOS/kernel/interface/RmsCancel.c \
SylixOS/kernel/interface/RmsCreate.c \
SylixOS/kernel/interface/RmsDelete.c \
SylixOS/kernel/interface/RmsExecTimeGet.c \
SylixOS/kernel/interface/RmsGetName.c \
SylixOS/kernel/interface/RmsPeriod.c \
SylixOS/kernel/interface/RmsStatus.c \
SylixOS/kernel/interface/SemaphoreBClear.c \
SylixOS/kernel/interface/SemaphoreBCreate.c \
SylixOS/kernel/interface/SemaphoreBDelete.c \
SylixOS/kernel/interface/SemaphoreBFlush.c \
SylixOS/kernel/interface/SemaphoreBPend.c \
SylixOS/kernel/interface/SemaphoreBPendEx.c \
SylixOS/kernel/interface/SemaphoreBPost.c \
SylixOS/kernel/interface/SemaphoreBPostEx.c \
SylixOS/kernel/interface/SemaphoreBRelease.c \
SylixOS/kernel/interface/SemaphoreBStatus.c \
SylixOS/kernel/interface/SemaphoreBTryPend.c \
SylixOS/kernel/interface/SemaphoreCClear.c \
SylixOS/kernel/interface/SemaphoreCCreate.c \
SylixOS/kernel/interface/SemaphoreCDelete.c \
SylixOS/kernel/interface/SemaphoreCFlush.c \
SylixOS/kernel/interface/SemaphoreCPend.c \
SylixOS/kernel/interface/SemaphoreCPost.c \
SylixOS/kernel/interface/SemaphoreCRelease.c \
SylixOS/kernel/interface/SemaphoreCStatus.c \
SylixOS/kernel/interface/SemaphoreCStatusEx.c \
SylixOS/kernel/interface/SemaphoreCTryPend.c \
SylixOS/kernel/interface/SemaphoreDelete.c \
SylixOS/kernel/interface/SemaphoreFlush.c \
SylixOS/kernel/interface/SemaphoreGetName.c \
SylixOS/kernel/interface/SemaphoreMCreate.c \
SylixOS/kernel/interface/SemaphoreMDelete.c \
SylixOS/kernel/interface/SemaphoreMPend.c \
SylixOS/kernel/interface/SemaphoreMPost.c \
SylixOS/kernel/interface/SemaphoreMStatus.c \
SylixOS/kernel/interface/SemaphoreMStatusEx.c \
SylixOS/kernel/interface/SemaphorePend.c \
SylixOS/kernel/interface/SemaphorePost.c \
SylixOS/kernel/interface/SemaphorePostPend.c \
SylixOS/kernel/interface/SemaphoreRWCreate.c \
SylixOS/kernel/interface/SemaphoreRWDelete.c \
SylixOS/kernel/interface/SemaphoreRWPend.c \
SylixOS/kernel/interface/SemaphoreRWPost.c \
SylixOS/kernel/interface/SemaphoreRWStatus.c \
SylixOS/kernel/interface/ThreadAffinity.c \
SylixOS/kernel/interface/ThreadAttrBuild.c \
SylixOS/kernel/interface/ThreadCancel.c \
SylixOS/kernel/interface/ThreadCancelWatchDog.c \
SylixOS/kernel/interface/ThreadCPUUsageRefresh.c \
SylixOS/kernel/interface/ThreadCreate.c \
SylixOS/kernel/interface/ThreadDelete.c \
SylixOS/kernel/interface/ThreadDesc.c \
SylixOS/kernel/interface/ThreadDetach.c \
SylixOS/kernel/interface/ThreadFeedWatchDog.c \
SylixOS/kernel/interface/ThreadForceDelete.c \
SylixOS/kernel/interface/ThreadForceResume.c \
SylixOS/kernel/interface/ThreadGetCPUUsage.c \
SylixOS/kernel/interface/ThreadGetName.c \
SylixOS/kernel/interface/ThreadGetNotePad.c \
SylixOS/kernel/interface/ThreadGetPriority.c \
SylixOS/kernel/interface/ThreadGetSchedParam.c \
SylixOS/kernel/interface/ThreadGetSlice.c \
SylixOS/kernel/interface/ThreadGetStackMini.c \
SylixOS/kernel/interface/ThreadIdSelf.c \
SylixOS/kernel/interface/ThreadInit.c \
SylixOS/kernel/interface/ThreadIsLock.c \
SylixOS/kernel/interface/ThreadIsReady.c \
SylixOS/kernel/interface/ThreadIsSafe.c \
SylixOS/kernel/interface/ThreadIsSuspend.c \
SylixOS/kernel/interface/ThreadJoin.c \
SylixOS/kernel/interface/ThreadLock.c \
SylixOS/kernel/interface/ThreadRestart.c \
SylixOS/kernel/interface/ThreadResume.c \
SylixOS/kernel/interface/ThreadSafe.c \
SylixOS/kernel/interface/ThreadSetCancelState.c \
SylixOS/kernel/interface/ThreadSetCancelType.c \
SylixOS/kernel/interface/ThreadSetName.c \
SylixOS/kernel/interface/ThreadSetNotePad.c \
SylixOS/kernel/interface/ThreadSetPriority.c \
SylixOS/kernel/interface/ThreadSetSchedParam.c \
SylixOS/kernel/interface/ThreadSetSlice.c \
SylixOS/kernel/interface/ThreadStackCheck.c \
SylixOS/kernel/interface/ThreadStart.c \
SylixOS/kernel/interface/ThreadStop.c \
SylixOS/kernel/interface/ThreadSuspend.c \
SylixOS/kernel/interface/ThreadTestCancel.c \
SylixOS/kernel/interface/ThreadUnlock.c \
SylixOS/kernel/interface/ThreadUnsafe.c \
SylixOS/kernel/interface/ThreadVarAdd.c \
SylixOS/kernel/interface/ThreadVarDelete.c \
SylixOS/kernel/interface/ThreadVarGet.c \
SylixOS/kernel/interface/ThreadVarInfo.c \
SylixOS/kernel/interface/ThreadVarSet.c \
SylixOS/kernel/interface/ThreadVerify.c \
SylixOS/kernel/interface/ThreadWakeup.c \
SylixOS/kernel/interface/ThreadYield.c \
SylixOS/kernel/interface/TimeGet.c \
SylixOS/kernel/interface/TimerCancel.c \
SylixOS/kernel/interface/TimerCreate.c \
SylixOS/kernel/interface/TimerDelete.c \
SylixOS/kernel/interface/TimerGetName.c \
SylixOS/kernel/interface/TimerHGetFrequency.c \
SylixOS/kernel/interface/TimerHTicks.c \
SylixOS/kernel/interface/TimerReset.c \
SylixOS/kernel/interface/TimerStart.c \
SylixOS/kernel/interface/TimerStatus.c \
SylixOS/kernel/interface/TimeSleep.c \
SylixOS/kernel/interface/TimeTod.c \
SylixOS/kernel/interface/ugid.c \
SylixOS/kernel/interface/WorkQueue.c \
SylixOS/kernel/list/listEvent.c \
SylixOS/kernel/list/listEventSet.c \
SylixOS/kernel/list/listHeap.c \
SylixOS/kernel/list/listLink.c \
SylixOS/kernel/list/listMsgQueue.c \
SylixOS/kernel/list/listPartition.c \
SylixOS/kernel/list/listRms.c \
SylixOS/kernel/list/listThread.c \
SylixOS/kernel/list/listThreadVar.c \
SylixOS/kernel/list/listTimer.c \
SylixOS/kernel/resource/resourceLib.c \
SylixOS/kernel/resource/resourceReclaim.c \
SylixOS/kernel/show/BacktraceShow.c \
SylixOS/kernel/show/CPUUsageShow.c \
SylixOS/kernel/show/EventSetShow.c \
SylixOS/kernel/show/InterShow.c \
SylixOS/kernel/show/MsgQueueShow.c \
SylixOS/kernel/show/PartitionShow.c \
SylixOS/kernel/show/RegionShow.c \
SylixOS/kernel/show/RmsShow.c \
SylixOS/kernel/show/SemaphoreShow.c \
SylixOS/kernel/show/StackShow.c \
SylixOS/kernel/show/ThreadShow.c \
SylixOS/kernel/show/TimerShow.c \
SylixOS/kernel/show/TimeShow.c \
SylixOS/kernel/show/VmmShow.c \
SylixOS/kernel/threadext/ThreadCleanup.c \
SylixOS/kernel/threadext/ThreadCond.c \
SylixOS/kernel/threadext/ThreadOnce.c \
SylixOS/kernel/tree/treeRb.c \
SylixOS/kernel/vmm/pageLib.c \
SylixOS/kernel/vmm/pageTable.c \
SylixOS/kernel/vmm/phyPage.c \
SylixOS/kernel/vmm/virPage.c \
SylixOS/kernel/vmm/vmm.c \
SylixOS/kernel/vmm/vmmAbort.c \
SylixOS/kernel/vmm/vmmArea.c \
SylixOS/kernel/vmm/vmmIo.c \
SylixOS/kernel/vmm/vmmMalloc.c \
SylixOS/kernel/vmm/vmmMmap.c \
SylixOS/kernel/vmm/vmmStack.c \
SylixOS/kernel/vmm/vmmSwap.c

#*********************************************************************************************************
# Buildin library source
#*********************************************************************************************************
LIB_SRCS = \
SylixOS/lib/extern/libc.c \
SylixOS/lib/libc/crypt/bcrypt.c \
SylixOS/lib/libc/crypt/crypt-sha1.c \
SylixOS/lib/libc/crypt/crypt.c \
SylixOS/lib/libc/crypt/hmac_sha1.c \
SylixOS/lib/libc/crypt/md5crypt.c \
SylixOS/lib/libc/crypt/pw_gensalt.c \
SylixOS/lib/libc/crypt/util.c \
SylixOS/lib/libc/ctype/lib_ctype.c \
SylixOS/lib/libc/error/lib_panic.c \
SylixOS/lib/libc/float/lib_isinf.c \
SylixOS/lib/libc/inttypes/lib_inttypes.c \
SylixOS/lib/libc/locale/lib_locale.c \
SylixOS/lib/libc/setjmp/setjmp.c \
SylixOS/lib/libc/stdio/asprintf.c \
SylixOS/lib/libc/stdio/clrerr.c \
SylixOS/lib/libc/stdio/ctermid.c \
SylixOS/lib/libc/stdio/cvtfloat.c \
SylixOS/lib/libc/stdio/fclose.c \
SylixOS/lib/libc/stdio/fdopen.c \
SylixOS/lib/libc/stdio/fdprintf.c \
SylixOS/lib/libc/stdio/fdscanf.c \
SylixOS/lib/libc/stdio/feof.c \
SylixOS/lib/libc/stdio/ferror.c \
SylixOS/lib/libc/stdio/fflush.c \
SylixOS/lib/libc/stdio/fgetc.c \
SylixOS/lib/libc/stdio/fgetln.c \
SylixOS/lib/libc/stdio/fgetpos.c \
SylixOS/lib/libc/stdio/fgets.c \
SylixOS/lib/libc/stdio/fileno.c \
SylixOS/lib/libc/stdio/findfp.c \
SylixOS/lib/libc/stdio/flags.c \
SylixOS/lib/libc/stdio/fopen.c \
SylixOS/lib/libc/stdio/fprintf.c \
SylixOS/lib/libc/stdio/fpurge.c \
SylixOS/lib/libc/stdio/fputc.c \
SylixOS/lib/libc/stdio/fputs.c \
SylixOS/lib/libc/stdio/fread.c \
SylixOS/lib/libc/stdio/freopen.c \
SylixOS/lib/libc/stdio/fscanf.c \
SylixOS/lib/libc/stdio/fseek.c \
SylixOS/lib/libc/stdio/fsetpos.c \
SylixOS/lib/libc/stdio/ftell.c \
SylixOS/lib/libc/stdio/funopen.c \
SylixOS/lib/libc/stdio/fvwrite.c \
SylixOS/lib/libc/stdio/fwalk.c \
SylixOS/lib/libc/stdio/fwrite.c \
SylixOS/lib/libc/stdio/getc.c \
SylixOS/lib/libc/stdio/getchar.c \
SylixOS/lib/libc/stdio/gets.c \
SylixOS/lib/libc/stdio/getw.c \
SylixOS/lib/libc/stdio/lib_file.c \
SylixOS/lib/libc/stdio/makebuf.c \
SylixOS/lib/libc/stdio/mktemp.c \
SylixOS/lib/libc/stdio/perror.c \
SylixOS/lib/libc/stdio/popen.c \
SylixOS/lib/libc/stdio/printf.c \
SylixOS/lib/libc/stdio/putc.c \
SylixOS/lib/libc/stdio/putchar.c \
SylixOS/lib/libc/stdio/puts.c \
SylixOS/lib/libc/stdio/putw.c \
SylixOS/lib/libc/stdio/refill.c \
SylixOS/lib/libc/stdio/remove.c \
SylixOS/lib/libc/stdio/rewind.c \
SylixOS/lib/libc/stdio/rget.c \
SylixOS/lib/libc/stdio/scanf.c \
SylixOS/lib/libc/stdio/setbuf.c \
SylixOS/lib/libc/stdio/setbuffer.c \
SylixOS/lib/libc/stdio/setvbuf.c \
SylixOS/lib/libc/stdio/snprintf.c \
SylixOS/lib/libc/stdio/sprintf.c \
SylixOS/lib/libc/stdio/sscanf.c \
SylixOS/lib/libc/stdio/stdio.c \
SylixOS/lib/libc/stdio/tempnam.c \
SylixOS/lib/libc/stdio/tmpfile.c \
SylixOS/lib/libc/stdio/tmpnam.c \
SylixOS/lib/libc/stdio/ungetc.c \
SylixOS/lib/libc/stdio/vfprintf.c \
SylixOS/lib/libc/stdio/vfscanf.c \
SylixOS/lib/libc/stdio/vprintf.c \
SylixOS/lib/libc/stdio/vscanf.c \
SylixOS/lib/libc/stdio/vsnprintf.c \
SylixOS/lib/libc/stdio/vsprintf.c \
SylixOS/lib/libc/stdio/vsscanf.c \
SylixOS/lib/libc/stdio/wbuf.c \
SylixOS/lib/libc/stdio/wsetup.c \
SylixOS/lib/libc/stdlib/lib_abs.c \
SylixOS/lib/libc/stdlib/lib_env.c \
SylixOS/lib/libc/stdlib/lib_lldiv.c \
SylixOS/lib/libc/stdlib/lib_memlib.c \
SylixOS/lib/libc/stdlib/lib_rand.c \
SylixOS/lib/libc/stdlib/lib_search.c \
SylixOS/lib/libc/stdlib/lib_sort.c \
SylixOS/lib/libc/stdlib/lib_strto.c \
SylixOS/lib/libc/stdlib/lib_strtod.c \
SylixOS/lib/libc/stdlib/lib_system.c \
SylixOS/lib/libc/string/lib_ffs.c \
SylixOS/lib/libc/string/lib_index.c \
SylixOS/lib/libc/string/lib_memchr.c \
SylixOS/lib/libc/string/lib_memcmp.c \
SylixOS/lib/libc/string/lib_memcpy.c \
SylixOS/lib/libc/string/lib_mempcpy.c \
SylixOS/lib/libc/string/lib_memrchr.c \
SylixOS/lib/libc/string/lib_memset.c \
SylixOS/lib/libc/string/lib_rindex.c \
SylixOS/lib/libc/string/lib_stpcpy.c \
SylixOS/lib/libc/string/lib_strcasecmp.c \
SylixOS/lib/libc/string/lib_strcat.c \
SylixOS/lib/libc/string/lib_strchrnul.c \
SylixOS/lib/libc/string/lib_strcmp.c \
SylixOS/lib/libc/string/lib_strcpy.c \
SylixOS/lib/libc/string/lib_strcspn.c \
SylixOS/lib/libc/string/lib_strdup.c \
SylixOS/lib/libc/string/lib_strerror.c \
SylixOS/lib/libc/string/lib_strftime.c \
SylixOS/lib/libc/string/lib_stricmp.c \
SylixOS/lib/libc/string/lib_strlen.c \
SylixOS/lib/libc/string/lib_strncasecmp.c \
SylixOS/lib/libc/string/lib_strncat.c \
SylixOS/lib/libc/string/lib_strncmp.c \
SylixOS/lib/libc/string/lib_strncpy.c \
SylixOS/lib/libc/string/lib_strndup.c \
SylixOS/lib/libc/string/lib_strnlen.c \
SylixOS/lib/libc/string/lib_strnset.c \
SylixOS/lib/libc/string/lib_strpbrk.c \
SylixOS/lib/libc/string/lib_strptime.c \
SylixOS/lib/libc/string/lib_strsep.c \
SylixOS/lib/libc/string/lib_strsignal.c \
SylixOS/lib/libc/string/lib_strspn.c \
SylixOS/lib/libc/string/lib_strstr.c \
SylixOS/lib/libc/string/lib_strtok.c \
SylixOS/lib/libc/string/lib_strxfrm.c \
SylixOS/lib/libc/string/lib_swab.c \
SylixOS/lib/libc/string/lib_tolower.c \
SylixOS/lib/libc/string/lib_toupper.c \
SylixOS/lib/libc/string/lib_xstrdup.c \
SylixOS/lib/libc/string/lib_xstrndup.c \
SylixOS/lib/libc/sys/lib_statvfs.c \
SylixOS/lib/libc/time/lib_asctime.c \
SylixOS/lib/libc/time/lib_clock.c \
SylixOS/lib/libc/time/lib_ctime.c \
SylixOS/lib/libc/time/lib_daytime.c \
SylixOS/lib/libc/time/lib_difftime.c \
SylixOS/lib/libc/time/lib_gmtime.c \
SylixOS/lib/libc/time/lib_hrtime.c \
SylixOS/lib/libc/time/lib_localtime.c \
SylixOS/lib/libc/time/lib_mktime.c \
SylixOS/lib/libc/time/lib_time.c \
SylixOS/lib/libc/time/lib_tzset.c \
SylixOS/lib/libc/user/getpwent.c \
SylixOS/lib/libc/user/getshadow.c \
SylixOS/lib/libc/user/userdb.c \
SylixOS/lib/libc/wchar/wchar.c \
SylixOS/lib/libc/wchar/wcscasecmp.c \
SylixOS/lib/libc/wchar/wcscat.c \
SylixOS/lib/libc/wchar/wcschr.c \
SylixOS/lib/libc/wchar/wcscmp.c \
SylixOS/lib/libc/wchar/wcscpy.c \
SylixOS/lib/libc/wchar/wcscspn.c \
SylixOS/lib/libc/wchar/wcsdup.c \
SylixOS/lib/libc/wchar/wcslcat.c \
SylixOS/lib/libc/wchar/wcslcpy.c \
SylixOS/lib/libc/wchar/wcslen.c \
SylixOS/lib/libc/wchar/wcsncasecmp.c \
SylixOS/lib/libc/wchar/wcsncat.c \
SylixOS/lib/libc/wchar/wcsncmp.c \
SylixOS/lib/libc/wchar/wcsncpy.c \
SylixOS/lib/libc/wchar/wcspbrk.c \
SylixOS/lib/libc/wchar/wcsrchr.c \
SylixOS/lib/libc/wchar/wcsspn.c \
SylixOS/lib/libc/wchar/wcsstr.c \
SylixOS/lib/libc/wchar/wcstok.c \
SylixOS/lib/libc/wchar/wcswcs.c \
SylixOS/lib/libc/wchar/wmemchr.c \
SylixOS/lib/libc/wchar/wmemcmp.c \
SylixOS/lib/libc/wchar/wmemcpy.c \
SylixOS/lib/libc/wchar/wmemmove.c \
SylixOS/lib/libc/wchar/wmemset.c \
SylixOS/lib/libcompiler/synchronize/lib_synchronize.c \
SylixOS/lib/nl_compatible/nl_reent.c

#*********************************************************************************************************
# Loader source
#*********************************************************************************************************
LOADER_SRCS = \
SylixOS/loader/elf/elf_loader.c \
SylixOS/loader/src/loader.c \
SylixOS/loader/src/loader_affinity.c \
SylixOS/loader/src/loader_exec.c \
SylixOS/loader/src/loader_file.c \
SylixOS/loader/src/loader_malloc.c \
SylixOS/loader/src/loader_proc.c \
SylixOS/loader/src/loader_shell.c \
SylixOS/loader/src/loader_symbol.c \
SylixOS/loader/src/loader_vppatch.c \
SylixOS/loader/src/loader_vpstack.c \
SylixOS/loader/src/loader_vpthread.c \
SylixOS/loader/src/loader_vptimer.c \
SylixOS/loader/src/loader_wait.c

#*********************************************************************************************************
# Monitor source
#*********************************************************************************************************
MONITOR_SRCS = \
SylixOS/monitor/src/monitorBuffer.c \
SylixOS/monitor/src/monitorFileUpload.c \
SylixOS/monitor/src/monitorNetUpload.c \
SylixOS/monitor/src/monitorTrace.c \
SylixOS/monitor/src/monitorUpload.c

#*********************************************************************************************************
# MPI source
#*********************************************************************************************************
MPI_SRCS = \
SylixOS/mpi/dualportmem/dualportmem.c \
SylixOS/mpi/dualportmem/dualportmemLib.c \
SylixOS/mpi/src/mpiInit.c

#*********************************************************************************************************
# Net source
#*********************************************************************************************************
NET_SRCS = \
SylixOS/net/libc/gethostbyht.c \
SylixOS/net/libc/getifaddrs.c \
SylixOS/net/libc/getproto.c \
SylixOS/net/libc/getprotoby.c \
SylixOS/net/libc/getprotoent.c \
SylixOS/net/libc/getprotoname.c \
SylixOS/net/libc/getservbyname.c \
SylixOS/net/libc/getservbyport.c \
SylixOS/net/libc/getservent.c \
SylixOS/net/libc/inet_ntop.c \
SylixOS/net/libc/inet_pton.c \
SylixOS/net/lwip/lwip_arpctl.c \
SylixOS/net/lwip/lwip_bridge.c \
SylixOS/net/lwip/lwip_fix.c \
SylixOS/net/lwip/lwip_flowctl.c \
SylixOS/net/lwip/lwip_flowsh.c \
SylixOS/net/lwip/lwip_iphook.c \
SylixOS/net/lwip/lwip_if.c \
SylixOS/net/lwip/lwip_ifctl.c \
SylixOS/net/lwip/lwip_ifparam.c \
SylixOS/net/lwip/lwip_jobqueue.c \
SylixOS/net/lwip/lwip_loginbl.c \
SylixOS/net/lwip/lwip_mrtctl.c \
SylixOS/net/lwip/lwip_netif.c \
SylixOS/net/lwip/lwip_netstat.c \
SylixOS/net/lwip/lwip_route.c \
SylixOS/net/lwip/lwip_rtctl.c \
SylixOS/net/lwip/lwip_shell.c \
SylixOS/net/lwip/lwip_shell6.c \
SylixOS/net/lwip/lwip_socket.c \
SylixOS/net/lwip/lwip_srtctl.c \
SylixOS/net/lwip/lwip_sylixos.c \
SylixOS/net/lwip/lwip_vlanctl.c \
SylixOS/net/lwip/lwip_vnd.c \
SylixOS/net/lwip/balancing/ip4_sroute_x.c \
SylixOS/net/lwip/balancing/ip4_sroute.c \
SylixOS/net/lwip/balancing/ip6_sroute_x.c \
SylixOS/net/lwip/balancing/ip6_sroute.c \
SylixOS/net/lwip/bridge/netbridge.c \
SylixOS/net/lwip/event/lwip_netevent.c \
SylixOS/net/lwip/flowctl/cor_flowctl.c \
SylixOS/net/lwip/flowctl/ip4_flowctl.c \
SylixOS/net/lwip/flowctl/ip6_flowctl.c \
SylixOS/net/lwip/flowctl/net_flowctl.c \
SylixOS/net/lwip/mem/tlsf_mem.c \
SylixOS/net/lwip/mem/tlsf.c \
SylixOS/net/lwip/mroute/ip4_mrt.c \
SylixOS/net/lwip/mroute/ip6_mrt.c \
SylixOS/net/lwip/netcrc/ncrc_bitrev.c \
SylixOS/net/lwip/netcrc/ncrc_crc16.c \
SylixOS/net/lwip/netcrc/ncrc_crc32.c \
SylixOS/net/lwip/netdev/netdev.c \
SylixOS/net/lwip/netdev/netzcbuf.c \
SylixOS/net/lwip/netdev/vnetdev.c \
SylixOS/net/lwip/packet/af_packet.c \
SylixOS/net/lwip/packet/af_packet_eth.c \
SylixOS/net/lwip/proc/lwip_proc.c \
SylixOS/net/lwip/radix/cor_radix.c \
SylixOS/net/lwip/radix/ip4_radix.c \
SylixOS/net/lwip/radix/ip6_radix.c \
SylixOS/net/lwip/route/af_route.c \
SylixOS/net/lwip/route/ip4_route.c \
SylixOS/net/lwip/route/ip4_route_x.c \
SylixOS/net/lwip/route/ip6_route.c \
SylixOS/net/lwip/route/ip6_route_x.c \
SylixOS/net/lwip/route/tcp_mss_adj.c \
SylixOS/net/lwip/src/api/api_lib.c \
SylixOS/net/lwip/src/api/api_msg.c \
SylixOS/net/lwip/src/api/err.c \
SylixOS/net/lwip/src/api/netbuf.c \
SylixOS/net/lwip/src/api/netdb.c \
SylixOS/net/lwip/src/api/netifapi.c \
SylixOS/net/lwip/src/api/sockets.c \
SylixOS/net/lwip/src/api/tcpip.c \
SylixOS/net/lwip/src/core/altcp.c \
SylixOS/net/lwip/src/core/altcp_alloc.c \
SylixOS/net/lwip/src/core/altcp_tcp.c \
SylixOS/net/lwip/src/core/def.c \
SylixOS/net/lwip/src/core/dns.c \
SylixOS/net/lwip/src/core/inet_chksum.c \
SylixOS/net/lwip/src/core/init.c \
SylixOS/net/lwip/src/core/ip.c \
SylixOS/net/lwip/src/core/mcast.c \
SylixOS/net/lwip/src/core/mem.c \
SylixOS/net/lwip/src/core/memp.c \
SylixOS/net/lwip/src/core/netif.c \
SylixOS/net/lwip/src/core/pbuf.c \
SylixOS/net/lwip/src/core/raw.c \
SylixOS/net/lwip/src/core/stats.c \
SylixOS/net/lwip/src/core/sys.c \
SylixOS/net/lwip/src/core/tcp.c \
SylixOS/net/lwip/src/core/tcp_in.c \
SylixOS/net/lwip/src/core/tcp_out.c \
SylixOS/net/lwip/src/core/timeouts.c \
SylixOS/net/lwip/src/core/udp.c \
SylixOS/net/lwip/src/core/ipv4/autoip.c \
SylixOS/net/lwip/src/core/ipv4/dhcp.c \
SylixOS/net/lwip/src/core/ipv4/etharp.c \
SylixOS/net/lwip/src/core/ipv4/icmp.c \
SylixOS/net/lwip/src/core/ipv4/igmp.c \
SylixOS/net/lwip/src/core/ipv4/ip4.c \
SylixOS/net/lwip/src/core/ipv4/ip4_addr.c \
SylixOS/net/lwip/src/core/ipv4/ip4_frag.c \
SylixOS/net/lwip/src/core/ipv6/dhcp6.c \
SylixOS/net/lwip/src/core/ipv6/ethip6.c \
SylixOS/net/lwip/src/core/ipv6/icmp6.c \
SylixOS/net/lwip/src/core/ipv6/inet6.c \
SylixOS/net/lwip/src/core/ipv6/ip6.c \
SylixOS/net/lwip/src/core/ipv6/ip6_addr.c \
SylixOS/net/lwip/src/core/ipv6/ip6_frag.c \
SylixOS/net/lwip/src/core/ipv6/mld6.c \
SylixOS/net/lwip/src/core/ipv6/nd6.c \
SylixOS/net/lwip/src/netif/ethernet.c \
SylixOS/net/lwip/src/netif/ethernetif.c \
SylixOS/net/lwip/src/netif/ifqueue.c \
SylixOS/net/lwip/src/netif/slipif.c \
SylixOS/net/lwip/src/netif/aodv/aodv_hello.c \
SylixOS/net/lwip/src/netif/aodv/aodv_if.c \
SylixOS/net/lwip/src/netif/aodv/aodv_mcast.c \
SylixOS/net/lwip/src/netif/aodv/aodv_mtunnel.c \
SylixOS/net/lwip/src/netif/aodv/aodv_neighbor.c \
SylixOS/net/lwip/src/netif/aodv/aodv_proto.c \
SylixOS/net/lwip/src/netif/aodv/aodv_rerr.c \
SylixOS/net/lwip/src/netif/aodv/aodv_route.c \
SylixOS/net/lwip/src/netif/aodv/aodv_rrep.c \
SylixOS/net/lwip/src/netif/aodv/aodv_rreq.c \
SylixOS/net/lwip/src/netif/aodv/aodv_seeklist.c \
SylixOS/net/lwip/src/netif/aodv/aodv_timer.c \
SylixOS/net/lwip/src/netif/aodv/aodv_timercb.c \
SylixOS/net/lwip/src/netif/ppp/auth.c \
SylixOS/net/lwip/src/netif/ppp/ccp.c \
SylixOS/net/lwip/src/netif/ppp/chap-md5.c \
SylixOS/net/lwip/src/netif/ppp/chap-new.c \
SylixOS/net/lwip/src/netif/ppp/chap_ms.c \
SylixOS/net/lwip/src/netif/ppp/demand.c \
SylixOS/net/lwip/src/netif/ppp/eap.c \
SylixOS/net/lwip/src/netif/ppp/ecp.c \
SylixOS/net/lwip/src/netif/ppp/eui64.c \
SylixOS/net/lwip/src/netif/ppp/fsm.c \
SylixOS/net/lwip/src/netif/ppp/ipcp.c \
SylixOS/net/lwip/src/netif/ppp/ipv6cp.c \
SylixOS/net/lwip/src/netif/ppp/lcp.c \
SylixOS/net/lwip/src/netif/ppp/magic.c \
SylixOS/net/lwip/src/netif/ppp/mppe.c \
SylixOS/net/lwip/src/netif/ppp/multilink.c \
SylixOS/net/lwip/src/netif/ppp/ppp.c \
SylixOS/net/lwip/src/netif/ppp/pppapi.c \
SylixOS/net/lwip/src/netif/ppp/pppcrypt.c \
SylixOS/net/lwip/src/netif/ppp/pppoe.c \
SylixOS/net/lwip/src/netif/ppp/pppol2tp.c \
SylixOS/net/lwip/src/netif/ppp/pppos.c \
SylixOS/net/lwip/src/netif/ppp/upap.c \
SylixOS/net/lwip/src/netif/ppp/utils.c \
SylixOS/net/lwip/src/netif/ppp/vj.c \
SylixOS/net/lwip/src/netif/radio/aes_crypt.c \
SylixOS/net/lwip/src/netif/radio/crypt_driver.c \
SylixOS/net/lwip/src/netif/radio/csma_mac.c \
SylixOS/net/lwip/src/netif/radio/ieee802154_aes.c \
SylixOS/net/lwip/src/netif/radio/ieee802154_aes_ccm.c \
SylixOS/net/lwip/src/netif/radio/ieee802154_eth.c \
SylixOS/net/lwip/src/netif/radio/ieee802154_frame.c \
SylixOS/net/lwip/src/netif/radio/lowpan_compress.c \
SylixOS/net/lwip/src/netif/radio/lowpan_frag.c \
SylixOS/net/lwip/src/netif/radio/lowpan_if.c \
SylixOS/net/lwip/src/netif/radio/mac_driver.c \
SylixOS/net/lwip/src/netif/radio/null_mac.c \
SylixOS/net/lwip/src/netif/radio/null_rdc.c \
SylixOS/net/lwip/src/netif/radio/simple_crypt.c \
SylixOS/net/lwip/src/netif/radio/tdma_mac.c \
SylixOS/net/lwip/src/netif/radio/xmac_rdc.c \
SylixOS/net/lwip/tcpsig/tcp_md5.c \
SylixOS/net/lwip/tools/ftp/lwip_ftp.c \
SylixOS/net/lwip/tools/ftp/lwip_ftpd.c \
SylixOS/net/lwip/tools/hosttable/lwip_hosttable.c \
SylixOS/net/lwip/tools/iac/lwip_iac.c \
SylixOS/net/lwip/tools/nat/lwip_nat.c \
SylixOS/net/lwip/tools/nat/lwip_natlib.c \
SylixOS/net/lwip/tools/netbios/lwip_netbios.c \
SylixOS/net/lwip/tools/npf/lwip_npf.c \
SylixOS/net/lwip/tools/ping/lwip_ping.c \
SylixOS/net/lwip/tools/ping6/lwip_ping6.c \
SylixOS/net/lwip/tools/ppp/lwip_ppp.c \
SylixOS/net/lwip/tools/kernrpc/authdes_prot.c \
SylixOS/net/lwip/tools/kernrpc/authuxprot.c \
SylixOS/net/lwip/tools/kernrpc/auth_none.c \
SylixOS/net/lwip/tools/kernrpc/auth_unix.c \
SylixOS/net/lwip/tools/kernrpc/bindrsvprt.c \
SylixOS/net/lwip/tools/kernrpc/clnt_gen.c \
SylixOS/net/lwip/tools/kernrpc/clnt_perr.c \
SylixOS/net/lwip/tools/kernrpc/clnt_raw.c \
SylixOS/net/lwip/tools/kernrpc/clnt_simp.c \
SylixOS/net/lwip/tools/kernrpc/clnt_tcp.c \
SylixOS/net/lwip/tools/kernrpc/clnt_udp.c \
SylixOS/net/lwip/tools/kernrpc/clnt_unix.c \
SylixOS/net/lwip/tools/kernrpc/create_xid.c \
SylixOS/net/lwip/tools/kernrpc/des_crypt.c \
SylixOS/net/lwip/tools/kernrpc/des_impl.c \
SylixOS/net/lwip/tools/kernrpc/des_soft.c \
SylixOS/net/lwip/tools/kernrpc/getprotobyname.c \
SylixOS/net/lwip/tools/kernrpc/getrpcport.c \
SylixOS/net/lwip/tools/kernrpc/get_myaddr.c \
SylixOS/net/lwip/tools/kernrpc/key_call.c \
SylixOS/net/lwip/tools/kernrpc/key_prot.c \
SylixOS/net/lwip/tools/kernrpc/pmap_clnt.c \
SylixOS/net/lwip/tools/kernrpc/pmap_prot.c \
SylixOS/net/lwip/tools/kernrpc/pmap_prot2.c \
SylixOS/net/lwip/tools/kernrpc/pmap_rmt.c \
SylixOS/net/lwip/tools/kernrpc/pm_getmaps.c \
SylixOS/net/lwip/tools/kernrpc/pm_getport.c \
SylixOS/net/lwip/tools/kernrpc/rpc_cmsg.c \
SylixOS/net/lwip/tools/kernrpc/rpc_common.c \
SylixOS/net/lwip/tools/kernrpc/rpc_dtable.c \
SylixOS/net/lwip/tools/kernrpc/rpc_prot.c \
SylixOS/net/lwip/tools/kernrpc/rpc_thread.c \
SylixOS/net/lwip/tools/kernrpc/rtime.c \
SylixOS/net/lwip/tools/kernrpc/svc.c \
SylixOS/net/lwip/tools/kernrpc/svc_auth.c \
SylixOS/net/lwip/tools/kernrpc/svc_authux.c \
SylixOS/net/lwip/tools/kernrpc/svc_raw.c \
SylixOS/net/lwip/tools/kernrpc/svc_run.c \
SylixOS/net/lwip/tools/kernrpc/svc_simple.c \
SylixOS/net/lwip/tools/kernrpc/svc_tcp.c \
SylixOS/net/lwip/tools/kernrpc/svc_udp.c \
SylixOS/net/lwip/tools/kernrpc/svc_unix.c \
SylixOS/net/lwip/tools/kernrpc/xcrypt.c \
SylixOS/net/lwip/tools/kernrpc/xdr.c \
SylixOS/net/lwip/tools/kernrpc/xdr_array.c \
SylixOS/net/lwip/tools/kernrpc/xdr_float.c \
SylixOS/net/lwip/tools/kernrpc/xdr_intXX_t.c \
SylixOS/net/lwip/tools/kernrpc/xdr_mem.c \
SylixOS/net/lwip/tools/kernrpc/xdr_rec.c \
SylixOS/net/lwip/tools/kernrpc/xdr_ref.c \
SylixOS/net/lwip/tools/kernrpc/xdr_sizeof.c \
SylixOS/net/lwip/tools/kernrpc/xdr_stdio.c \
SylixOS/net/lwip/tools/snmp/snmpv3.c \
SylixOS/net/lwip/tools/snmp/snmpv3_mbedtls.c \
SylixOS/net/lwip/tools/snmp/snmpv3_sylixos.c \
SylixOS/net/lwip/tools/snmp/snmp_asn1.c \
SylixOS/net/lwip/tools/snmp/snmp_core.c \
SylixOS/net/lwip/tools/snmp/snmp_mib2.c \
SylixOS/net/lwip/tools/snmp/snmp_mib2_icmp.c \
SylixOS/net/lwip/tools/snmp/snmp_mib2_interfaces.c \
SylixOS/net/lwip/tools/snmp/snmp_mib2_ip.c \
SylixOS/net/lwip/tools/snmp/snmp_mib2_snmp.c \
SylixOS/net/lwip/tools/snmp/snmp_mib2_system.c \
SylixOS/net/lwip/tools/snmp/snmp_mib2_tcp.c \
SylixOS/net/lwip/tools/snmp/snmp_mib2_udp.c \
SylixOS/net/lwip/tools/snmp/snmp_msg.c \
SylixOS/net/lwip/tools/snmp/snmp_netconn.c \
SylixOS/net/lwip/tools/snmp/snmp_pbuf_stream.c \
SylixOS/net/lwip/tools/snmp/snmp_raw.c \
SylixOS/net/lwip/tools/snmp/snmp_scalar.c \
SylixOS/net/lwip/tools/snmp/snmp_snmpv2_framework.c \
SylixOS/net/lwip/tools/snmp/snmp_snmpv2_usm.c \
SylixOS/net/lwip/tools/snmp/snmp_table.c \
SylixOS/net/lwip/tools/snmp/snmp_threadsync.c \
SylixOS/net/lwip/tools/snmp/snmp_traps.c \
SylixOS/net/lwip/tools/telnet/lwip_telnet.c \
SylixOS/net/lwip/tools/tftp/lwip_tftp.c \
SylixOS/net/lwip/tools/vlan/lwip_vlan.c \
SylixOS/net/lwip/tools/vpn/lwip_vpn.c \
SylixOS/net/lwip/unix/af_unix.c \
SylixOS/net/lwip/unix/af_unix_msg.c \
SylixOS/net/lwip/vlan/eth_vlan.c \
SylixOS/net/lwip/wireless/lwip_wlext.c \
SylixOS/net/lwip/wireless/lwip_wlpriv.c \
SylixOS/net/lwip/wireless/lwip_wlspy.c 

#*********************************************************************************************************
# POSIX source
#*********************************************************************************************************
POSIX_SRCS = \
SylixOS/posix/aio/aio.c \
SylixOS/posix/aio/aio_lib.c \
SylixOS/posix/dlfcn/dlfcn.c \
SylixOS/posix/execinfo/execinfo.c \
SylixOS/posix/fmtmsg/fmtmsg.c \
SylixOS/posix/fnmatch/fnmatch.c \
SylixOS/posix/gjbext/gjb_devrel.c \
SylixOS/posix/gjbext/gjb_fsrel.c \
SylixOS/posix/gjbext/gjb_interrupt.c \
SylixOS/posix/gjbext/gjb_memrel.c \
SylixOS/posix/gjbext/gjb_netrel.c \
SylixOS/posix/gjbext/gjb_timerel.c \
SylixOS/posix/mman/mman.c \
SylixOS/posix/mqueue/mqueue.c \
SylixOS/posix/poll/poll.c \
SylixOS/posix/posixLib/pnameLib.c \
SylixOS/posix/posixLib/posixLib.c \
SylixOS/posix/posixLib/posixShell.c \
SylixOS/posix/posixLib/procPosix.c \
SylixOS/posix/pthread/pthread.c \
SylixOS/posix/pthread/pthread_attr.c \
SylixOS/posix/pthread/pthread_barrier.c \
SylixOS/posix/pthread/pthread_cond.c \
SylixOS/posix/pthread/pthread_key.c \
SylixOS/posix/pthread/pthread_mutex.c \
SylixOS/posix/pthread/pthread_rwlock.c \
SylixOS/posix/pthread/pthread_spinlock.c \
SylixOS/posix/resource/resource.c \
SylixOS/posix/sched/sched_rms.c \
SylixOS/posix/sched/sched.c \
SylixOS/posix/semaphore/semaphore.c \
SylixOS/posix/spawn/spawn.c \
SylixOS/posix/sysconf/sysconf.c \
SylixOS/posix/syslog/syslog.c \
SylixOS/posix/timeb/adjtime.c \
SylixOS/posix/timeb/timeb.c \
SylixOS/posix/timeb/times.c \
SylixOS/posix/utsname/utsname.c

#*********************************************************************************************************
# Shell source
#*********************************************************************************************************
SHELL_SRCS = \
SylixOS/shell/extLib/ttinyShellExtCmd.c \
SylixOS/shell/fsLib/ttinyShellFsCmd.c \
SylixOS/shell/getopt/getopt_bsd.c \
SylixOS/shell/getopt/getopt_var.c \
SylixOS/shell/hashLib/hashHorner.c \
SylixOS/shell/heapLib/ttinyShellHeapCmd.c \
SylixOS/shell/modemLib/ttinyShellModemCmd.c \
SylixOS/shell/perfLib/ttinyShellPerfCmd.c \
SylixOS/shell/tarLib/ttinyShellTarCmd.c \
SylixOS/shell/ttinyShell/ttinyShell.c \
SylixOS/shell/ttinyShell/ttinyShellColor.c \
SylixOS/shell/ttinyShell/ttinyShellLib.c \
SylixOS/shell/ttinyShell/ttinyShellReadline.c \
SylixOS/shell/ttinyShell/ttinyShellSysCmd.c \
SylixOS/shell/ttinyShell/ttinyShellSysVar.c \
SylixOS/shell/ttinyShell/ttinyString.c \
SylixOS/shell/ttinyUser/ttinyUserAdmin.c \
SylixOS/shell/ttinyUser/ttinyUserAuthen.c \
SylixOS/shell/ttinyUser/ttinyUserCache.c \
SylixOS/shell/ttinyVar/ttinyVar.c \
SylixOS/shell/ttinyVar/ttinyVarLib.c

#*********************************************************************************************************
# Symbol source
#*********************************************************************************************************
SYMBOL_SRCS = \
SylixOS/symbol/symBsp/symBsp.c \
SylixOS/symbol/symLibc/symLibc.c \
SylixOS/symbol/symTable/symProc.c \
SylixOS/symbol/symTable/symTable.c

#*********************************************************************************************************
# System source
#*********************************************************************************************************
SYS_SRCS = \
SylixOS/system/bus/busSystem.c \
SylixOS/system/device/ahci/ahci.c \
SylixOS/system/device/ahci/ahciCtrl.c \
SylixOS/system/device/ahci/ahciDev.c \
SylixOS/system/device/ahci/ahciDrv.c \
SylixOS/system/device/ahci/ahciLib.c \
SylixOS/system/device/ahci/ahciPm.c \
SylixOS/system/device/ahci/ahciPort.c \
SylixOS/system/device/ahci/ahciShow.c \
SylixOS/system/device/ahci/ahciSmart.c \
SylixOS/system/device/ata/ata.c \
SylixOS/system/device/ata/ataLib.c \
SylixOS/system/device/block/blockIo.c \
SylixOS/system/device/block/blockRaw.c \
SylixOS/system/device/block/ramDisk.c \
SylixOS/system/device/buzzer/buzzer.c \
SylixOS/system/device/can/can.c \
SylixOS/system/device/dma/dma.c \
SylixOS/system/device/dma/dmaLib.c \
SylixOS/system/device/eventfd/eventfdDev.c \
SylixOS/system/device/gpio/gpioDev.c \
SylixOS/system/device/gpio/gpioLib.c \
SylixOS/system/device/graph/gmemDev.c \
SylixOS/system/device/hstimerfd/hstimerfdDev.c \
SylixOS/system/device/hwrtc/hwrtc.c \
SylixOS/system/device/i2c/i2cLib.c \
SylixOS/system/device/mem/memDev.c \
SylixOS/system/device/mii/miiDev.c \
SylixOS/system/device/nvme/nvme.c \
SylixOS/system/device/nvme/nvmeCtrl.c \
SylixOS/system/device/nvme/nvmeDev.c \
SylixOS/system/device/nvme/nvmeDrv.c \
SylixOS/system/device/nvme/nvmeLib.c \
SylixOS/system/device/nvme/nvmePm.c \
SylixOS/system/device/pci/pciAuto.c \
SylixOS/system/device/pci/pciCap.c \
SylixOS/system/device/pci/pciCapExt.c \
SylixOS/system/device/pci/pciDb.c \
SylixOS/system/device/pci/pciDev.c \
SylixOS/system/device/pci/pciDrv.c \
SylixOS/system/device/pci/pciError.c \
SylixOS/system/device/pci/pciLib.c \
SylixOS/system/device/pci/pciMsi.c \
SylixOS/system/device/pci/pciMsix.c \
SylixOS/system/device/pci/pciPm.c \
SylixOS/system/device/pci/pciProc.c \
SylixOS/system/device/pci/pciScan.c \
SylixOS/system/device/pci/pciShow.c \
SylixOS/system/device/pci/pciVpd.c \
SylixOS/system/device/pipe/pipe.c \
SylixOS/system/device/pipe/pipeLib.c \
SylixOS/system/device/pty/pty.c \
SylixOS/system/device/pty/ptyDevice.c \
SylixOS/system/device/pty/ptyHost.c \
SylixOS/system/device/rand/randDev.c \
SylixOS/system/device/rand/randDevLib.c \
SylixOS/system/device/sd/sdLib.c \
SylixOS/system/device/sdcard/client/sdiobaseDrv.c \
SylixOS/system/device/sdcard/client/sdmemory.c \
SylixOS/system/device/sdcard/client/sdmemoryDrv.c \
SylixOS/system/device/sdcard/core/sdcore.c \
SylixOS/system/device/sdcard/core/sdcoreLib.c \
SylixOS/system/device/sdcard/core/sdcrc.c \
SylixOS/system/device/sdcard/core/sddrvm.c \
SylixOS/system/device/sdcard/core/sdiocoreLib.c \
SylixOS/system/device/sdcard/core/sdiodrvm.c \
SylixOS/system/device/sdcard/core/sdutil.c \
SylixOS/system/device/sdcard/host/sdhci.c \
SylixOS/system/device/shm/shm.c \
SylixOS/system/device/spi/spiLib.c \
SylixOS/system/device/spipe/spipe.c \
SylixOS/system/device/spipe/spipeLib.c \
SylixOS/system/device/ty/termios.c \
SylixOS/system/device/ty/tty.c \
SylixOS/system/device/ty/tyLib.c \
SylixOS/system/distribute/distribute.c \
SylixOS/system/epoll/epollDev.c \
SylixOS/system/epoll/epollLib.c \
SylixOS/system/excLib/excLib.c \
SylixOS/system/hotplugLib/hotplugDev.c \
SylixOS/system/hotplugLib/hotplugLib.c \
SylixOS/system/ioLib/ioDir.c \
SylixOS/system/ioLib/ioFcntl.c \
SylixOS/system/ioLib/ioFdNode.c \
SylixOS/system/ioLib/ioFifo.c \
SylixOS/system/ioLib/ioFile.c \
SylixOS/system/ioLib/ioFormat.c \
SylixOS/system/ioLib/ioInterface.c \
SylixOS/system/ioLib/ioLib.c \
SylixOS/system/ioLib/ioLicense.c \
SylixOS/system/ioLib/ioLockF.c \
SylixOS/system/ioLib/ioPath.c \
SylixOS/system/ioLib/ioShow.c \
SylixOS/system/ioLib/ioStat.c \
SylixOS/system/ioLib/ioSymlink.c \
SylixOS/system/ioLib/ioSys.c \
SylixOS/system/logLib/logLib.c \
SylixOS/system/pm/pmAdapter.c \
SylixOS/system/pm/pmDev.c \
SylixOS/system/pm/pmIdle.c \
SylixOS/system/pm/pmSystem.c \
SylixOS/system/ptimer/ptimer.c \
SylixOS/system/ptimer/ptimerDev.c \
SylixOS/system/select/selectCtl.c \
SylixOS/system/select/selectInit.c \
SylixOS/system/select/selectLib.c \
SylixOS/system/select/selectList.c \
SylixOS/system/select/selectNode.c \
SylixOS/system/select/selectPty.c \
SylixOS/system/select/selectTy.c \
SylixOS/system/select/waitfile.c \
SylixOS/system/signal/signal.c \
SylixOS/system/signal/signalDev.c \
SylixOS/system/signal/signalEvent.c \
SylixOS/system/signal/signalJmp.c \
SylixOS/system/signal/signalLib.c \
SylixOS/system/sysHookList/sysHookList.c \
SylixOS/system/sysHookList/sysHookListLib.c \
SylixOS/system/sysInit/sysInit.c \
SylixOS/system/threadpool/threadpool.c \
SylixOS/system/threadpool/threadpoolLib.c \
SylixOS/system/util/bmsgLib.c \
SylixOS/system/util/rngLib.c

#*********************************************************************************************************
# Sysperf source
#*********************************************************************************************************
SYSPERF_SRCS = \
SylixOS/sysperf/sysperf.c \
SylixOS/sysperf/sysperfLib.c 

#*********************************************************************************************************
# C++ source
#*********************************************************************************************************
CPP_SRCS = \
SylixOS/cplusplus/cppRtLib/cppEabiLib.cpp \
SylixOS/cplusplus/cppRtLib/cppMemLib.cpp \
SylixOS/cplusplus/cppRtLib/cppRtBegin.cpp \
SylixOS/cplusplus/cppRtLib/cppRtEnd.cpp \
SylixOS/cplusplus/cppRtLib/cppSupLib.cpp

LOCAL_SRCS := $(APPL_SRCS)
LOCAL_SRCS += $(DEBUG_SRCS)
LOCAL_SRCS += $(DRV_SRCS)
LOCAL_SRCS += $(FS_SRCS)
LOCAL_SRCS += $(GUI_SRCS)
LOCAL_SRCS += $(KERN_SRCS)
LOCAL_SRCS += $(LIB_SRCS)
LOCAL_SRCS += $(MONITOR_SRCS)
LOCAL_SRCS += $(LOADER_SRCS)
LOCAL_SRCS += $(MPI_SRCS)
LOCAL_SRCS += $(NET_SRCS)
LOCAL_SRCS += $(POSIX_SRCS)
LOCAL_SRCS += $(SHELL_SRCS)
LOCAL_SRCS += $(SYMBOL_SRCS)
LOCAL_SRCS += $(SYS_SRCS)
LOCAL_SRCS += $(SYSPERF_SRCS)
LOCAL_SRCS += $(CPP_SRCS)

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your hearder files search path")
#*********************************************************************************************************
LOCAL_INC_PATH := 
ifeq ($(ARCH), $(filter $(ARCH),x86 x64))
LOCAL_INC_PATH += -I"./SylixOS/arch/x86/acpi/acpica/include"
endif

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := 
ifeq ($(BUILD_LITE_TARGET), 1)
LOCAL_DSYMBOL += -D__SYLIXOS_LITE
endif
ifeq ($(ARCH), $(filter $(ARCH),x86 x64))
LOCAL_DSYMBOL += -DACPI_DEBUG_OUTPUT -DACPI_DISASSEMBLER -DACPI_APPLICATION
endif

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
# compile ARM FPU source files
#*********************************************************************************************************
ARM_FPU_ASFLAGS = -mfloat-abi=softfp -mfpu=vfpv3

$(OBJPATH)/libsylixos.a/SylixOS/arch/arm/fpu/v7m/armVfpV7MAsm.o: ./SylixOS/arch/arm/fpu/v7m/armVfpV7MAsm.S
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__DEP))" ]; then \
			mkdir -p "$(dir $(__DEP))"; fi
		$(AS) $(ARM_FPU_ASFLAGS) $($(__TARGET)_ASFLAGS_WITHOUT_FPUFLAGS) -MMD -MP -MF $(__DEP) -c $< -o $@

$(OBJPATH)/libsylixos.a/SylixOS/arch/arm/fpu/vfp9/armVfp9Asm.o: ./SylixOS/arch/arm/fpu/vfp9/armVfp9Asm.S
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__DEP))" ]; then \
			mkdir -p "$(dir $(__DEP))"; fi
		$(AS) $(ARM_FPU_ASFLAGS) $($(__TARGET)_ASFLAGS_WITHOUT_FPUFLAGS) -MMD -MP -MF $(__DEP) -c $< -o $@

$(OBJPATH)/libsylixos.a/SylixOS/arch/arm/fpu/vfp11/armVfp11Asm.o: ./SylixOS/arch/arm/fpu/vfp11/armVfp11Asm.S
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__DEP))" ]; then \
			mkdir -p "$(dir $(__DEP))"; fi
		$(AS) $(ARM_FPU_ASFLAGS) $($(__TARGET)_ASFLAGS_WITHOUT_FPUFLAGS) -MMD -MP -MF $(__DEP) -c $< -o $@

$(OBJPATH)/libsylixos.a/SylixOS/arch/arm/fpu/vfpv3/armVfpV3Asm.o: ./SylixOS/arch/arm/fpu/vfpv3/armVfpV3Asm.S
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__DEP))" ]; then \
			mkdir -p "$(dir $(__DEP))"; fi
		$(AS) $(ARM_FPU_ASFLAGS) $($(__TARGET)_ASFLAGS_WITHOUT_FPUFLAGS) -MMD -MP -MF $(__DEP) -c $< -o $@

#*********************************************************************************************************
# compile MIPS FPU source files
#*********************************************************************************************************
MIPS_FPU_ASFLAGS = -mhard-float

$(OBJPATH)/libsylixos.a/SylixOS/arch/mips/fpu/fpu32/mipsVfp32Asm.o: ./SylixOS/arch/mips/fpu/fpu32/mipsVfp32Asm.S
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__DEP))" ]; then \
			mkdir -p "$(dir $(__DEP))"; fi
		$(AS) $(MIPS_FPU_ASFLAGS) $($(__TARGET)_ASFLAGS_WITHOUT_FPUFLAGS) -MMD -MP -MF $(__DEP) -c $< -o $@

#*********************************************************************************************************
# compile PowerPC FPU source files
#*********************************************************************************************************
PPC_FPU_ASFLAGS = -mhard-float

$(OBJPATH)/libsylixos.a/SylixOS/arch/ppc/fpu/vfp/ppcVfpAsm.o: ./SylixOS/arch/ppc/fpu/vfp/ppcVfpAsm.S
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__DEP))" ]; then \
			mkdir -p "$(dir $(__DEP))"; fi
		$(AS) $(PPC_FPU_ASFLAGS) $($(__TARGET)_ASFLAGS_WITHOUT_FPUFLAGS) -MMD -MP -MF $(__DEP) -c $< -o $@

#*********************************************************************************************************
# compile x86 FPU source files
#*********************************************************************************************************
X86_FPU_ASFLAGS = -mhard-float

$(OBJPATH)/libsylixos.a/SylixOS/arch/x86/fpu/fpusse/x86FpuSseAsm.o: ./SylixOS/arch/x86/fpu/fpusse/x86FpuSseAsm.S
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__DEP))" ]; then \
			mkdir -p "$(dir $(__DEP))"; fi
		$(AS) $(X86_FPU_ASFLAGS) $($(__TARGET)_ASFLAGS_WITHOUT_FPUFLAGS) -MMD -MP -MF $(__DEP) -c $< -o $@
		
#*********************************************************************************************************
# compile x86-64 FPU source files
#*********************************************************************************************************
X64_FPU_ASFLAGS = -mhard-float

$(OBJPATH)/libsylixos.a/SylixOS/arch/x86/fpu/fpusse/x64FpuSseAsm.o: ./SylixOS/arch/x86/fpu/fpusse/x64FpuSseAsm.S
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__DEP))" ]; then \
			mkdir -p "$(dir $(__DEP))"; fi
		$(AS) $(X64_FPU_ASFLAGS) $($(__TARGET)_ASFLAGS_WITHOUT_FPUFLAGS) -MMD -MP -MF $(__DEP) -c $< -o $@
		
#*********************************************************************************************************
# compile SPARC FPU source files
#*********************************************************************************************************
SPARC_FPU_ASFLAGS = -mhard-float

$(OBJPATH)/libsylixos.a/SylixOS/arch/sparc/fpu/vfp/sparcVfpAsm.o: ./SylixOS/arch/sparc/fpu/vfp/sparcVfpAsm.S
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__DEP))" ]; then \
			mkdir -p "$(dir $(__DEP))"; fi
		$(AS) $(SPARC_FPU_ASFLAGS) $($(__TARGET)_ASFLAGS_WITHOUT_FPUFLAGS) -MMD -MP -MF $(__DEP) -c $< -o $@
		
include $(LIBSYLIXOS_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
