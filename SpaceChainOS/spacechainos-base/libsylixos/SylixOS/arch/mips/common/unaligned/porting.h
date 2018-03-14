/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: porting.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 02 月 15 日
**
** 描        述: MIPS 非对齐处理移植头文件.
*********************************************************************************************************/

#ifndef __ARCH_MIPSUNALIGNEDPORT_H
#define __ARCH_MIPSUNALIGNEDPORT_H

#ifdef __GNUC__
#define PTR         .word
#else
#define PTR         TODO
#endif

enum {
    VERIFY_READ,
    VERIFY_WRITE,
};

/*
 * 注意: 始终认为可以访问，如果地址不合法，可能会产生异常嵌套，可以通过异常嵌套时打印的调用栈分析应用
 * 中有问题的代码
 */
#define access_ok(type, va, len)     1

#define IS_ENABLED(config)      MIPS_##config

#define get_fs()                0
#define get_ds()                1
#define segment_eq(s1, s2)      0

extern ARCH_FPU_CTX   _G_mipsFpuCtx[LW_CFG_MAX_PROCESSORS];
extern int fpu_emulator_cop1Handler(ARCH_REG_CTX *xcp, ARCH_FPU_CTX *ctx,
                                    int has_fpu, void *__user *fault_addr);

#endif                                                                  /*  __ARCH_MIPSUNALIGNEDPORT_H  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
