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
** 文   件   名: c6x_io.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 02 月 28 日
**
** 描        述: TI C6X DSP 体系构架 I/O 访问接口.
*********************************************************************************************************/

#ifndef __ARCH_C6X_IO_H
#define __ARCH_C6X_IO_H

#include "endian.h"
#include "common/c6xIo.h"

/*********************************************************************************************************
  C6X 处理器 IOMEM 端口操作
*********************************************************************************************************/
/*********************************************************************************************************
  read IOMEM
*********************************************************************************************************/

#define read8(a)            read8_raw(a)
#define read16(a)           read16_raw(a)
#define read32(a)           read32_raw(a)
#define read64(a)           read64_raw(a)

#define read8_le(a)         read8_raw(a)
#define read16_le(a)        le16toh(read16_raw(a))
#define read32_le(a)        le32toh(read32_raw(a))
#define read64_le(a)        le64toh(read64_raw(a))

#define read8_be(a)         read8_raw(a)
#define read16_be(a)        be16toh(read16_raw(a))
#define read32_be(a)        be32toh(read32_raw(a))
#define read64_be(a)        be64toh(read64_raw(a))

#define reads8(a, b, c)     reads8_raw(a, b, c)
#define reads16(a, b, c)    reads16_raw(a, b, c)
#define reads32(a, b, c)    reads32_raw(a, b, c)
#define reads64(a, b, c)    reads64_raw(a, b, c)

/*********************************************************************************************************
  write IOMEM
*********************************************************************************************************/

#define write8(d, a)        write8_raw(d, a)
#define write16(d, a)       write16_raw(d, a)
#define write32(d, a)       write32_raw(d, a)
#define write64(d, a)       write64_raw(d, a)

#define write8_le(d, a)     write8_raw(d, a)
#define write16_le(d, a)    write16_raw(htole16(d), a)
#define write32_le(d, a)    write32_raw(htole32(d), a)
#define write64_le(d, a)    write64_raw(htole64(d), a)

#define write8_be(d, a)     write8_raw(d, a)
#define write16_be(d, a)    write16_raw(htobe16(d), a)
#define write32_be(d, a)    write32_raw(htobe32(d), a)
#define write64_be(d, a)    write64_raw(htobe64(d), a)

#define writes8(a, b, c)    writes8_raw(a, b, c)
#define writes16(a, b, c)   writes16_raw(a, b, c)
#define writes32(a, b, c)   writes32_raw(a, b, c)
#define writes64(a, b, c)   writes64_raw(a, b, c)

/*********************************************************************************************************
  C6X 处理器 I/O 端口操作 (C6X 处理器没有独立 I/O 端口, 处理方法与 I/O 内存相同)
*********************************************************************************************************/

extern ioaddr_t  __IO_BASE__;

/*********************************************************************************************************
  in IOPORT
*********************************************************************************************************/

#define in8(a)              read8(__IO_BASE__ + a)
#define in16(a)             read16(__IO_BASE__ + a)
#define in32(a)             read32(__IO_BASE__ + a)
#define in64(a)             read64(__IO_BASE__ + a)

#define in8_le(a)           read8_le(__IO_BASE__ + a)
#define in16_le(a)          read16_le(__IO_BASE__ + a)
#define in32_le(a)          read32_le(__IO_BASE__ + a)
#define in64_le(a)          read64_le(__IO_BASE__ + a)

#define in8_be(a)           read8_be(__IO_BASE__ + a)
#define in16_be(a)          read16_be(__IO_BASE__ + a)
#define in32_be(a)          read32_be(__IO_BASE__ + a)
#define in64_be(a)          read64_be(__IO_BASE__ + a)

#define ins8(a, b, c)       reads8(__IO_BASE__ + a, b, c)
#define ins16(a, b, c)      reads16(__IO_BASE__ + a, b, c)
#define ins32(a, b, c)      reads32(__IO_BASE__ + a, b, c)
#define ins64(a, b, c)      reads64(__IO_BASE__ + a, b, c)

/*********************************************************************************************************
  out IOPORT
*********************************************************************************************************/

#define out8(d, a)          write8(d, __IO_BASE__ + a)
#define out16(d, a)         write16(d, __IO_BASE__ + a)
#define out32(d, a)         write32(d, __IO_BASE__ + a)
#define out64(d, a)         write64(d, __IO_BASE__ + a)

#define out8_le(d, a)       write8_le(d, __IO_BASE__ + a)
#define out16_le(d, a)      write16_le(d, __IO_BASE__ + a)
#define out32_le(d, a)      write32_le(d, __IO_BASE__ + a)
#define out64_le(d, a)      write64_le(d, __IO_BASE__ + a)

#define out8_be(d, a)       write8_be(d, __IO_BASE__ + a)
#define out16_be(d, a)      write16_be(d, __IO_BASE__ + a)
#define out32_be(d, a)      write32_be(d, __IO_BASE__ + a)
#define out64_be(d, a)      write64_be(d, __IO_BASE__ + a)

#define outs8(a, b, c)      writes8(__IO_BASE__ + a, b, c)
#define outs16(a, b, c)     writes16(__IO_BASE__ + a, b, c)
#define outs32(a, b, c)     writes32(__IO_BASE__ + a, b, c)
#define outs64(a, b, c)     writes64(__IO_BASE__ + a, b, c)

#endif                                                                  /*  __ARCH_C6X_IO_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
