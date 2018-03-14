#ifndef __LINUX_COMPAT_H
#define __LINUX_COMPAT_H

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "limits.h"
#include "endian.h"
#include "bitops.h"
#include "io.h"

#ifndef __user
#define __user
#endif

#ifndef __iomem
#define __iomem
#endif

#ifndef __init
#define __init
#endif

#ifndef __exit
#define __exit
#endif

#ifndef __initdata
#define __initdata
#endif

/*
 * I/O memory
 *
 * SylixOS RW I/O memory is with CPU endian
 *
 * Linux RW I/O memory is always little endian.
 */
#define readb(addr)                 read8((addr_t)(addr))
#define readw(addr)                 le16toh(read16((addr_t)(addr)))
#define readl(addr)                 le32toh(read32((addr_t)(addr)))
#define readq(addr)                 le64toh(read64((addr_t)(addr)))

#define writeb(val, addr)           write8(val, (addr_t)(addr))
#define writew(val, addr)           write16(htole16(val), (addr_t)(addr))
#define writel(val, addr)           write32(htole32(val), (addr_t)(addr))
#define writeq(val, addr)           write64(htole64(val), (addr_t)(addr))

#define readb_relaxed(addr)         readb((addr_t)(addr))
#define readw_relaxed(addr)         readw((addr_t)(addr))
#define readl_relaxed(addr)         readl((addr_t)(addr))
#define readq_relaxed(addr)         readq((addr_t)(addr))

#define writeb_relaxed(val, addr)   writeb(val, (addr_t)(addr))
#define writew_relaxed(val, addr)   writew(val, (addr_t)(addr))
#define writel_relaxed(val, addr)   writel(val, (addr_t)(addr))
#define writeq_relaxed(val, addr)   writeq(val, (addr_t)(addr))

/*
 * I/O port
 *
 * SylixOS RW I/O port is with CPU endian
 *
 * Linux RW I/O port is always little endian.
 */
#define inb(addr)                   in8((ioaddr_t)(addr))
#define inw(addr)                   le16toh(in16((ioaddr_t)(addr)))
#define inl(addr)                   le32toh(in32((ioaddr_t)(addr)))

#define outb(val, addr)             out8(val, (addr_t)(addr))
#define outw(val, addr)             out16(htole16(val), (addr_t)(addr))
#define outl(val, addr)             out32(htole32(val), (addr_t)(addr))

/*
 * data type
 */
#ifndef __u8_defined
typedef uint8_t                 u8;
typedef SINT8                   s8;
#define __u8_defined            1
#define __s8_defined            1
#endif

#ifndef __u16_defined
typedef uint16_t                u16;
typedef int16_t                 s16;
#define __u16_defined           1
#define __s16_defined           1
#endif

#ifndef __u32_defined
typedef uint32_t                u32;
typedef int32_t                 s32;
#define __u32_defined           1
#define __s32_defined           1
#endif

#ifndef __u64_defined
typedef uint64_t                u64;
typedef int64_t                 s64;
#define __u64_defined           1
#define __s64_defined           1
#endif

#define ARRAY_SIZE(x)           (sizeof(x) / sizeof((x)[0]))

#ifndef SYLIXOS_LIB
#define ndelay(x)	            bspDelayNs(x)
#define udelay(x)	            bspDelayUs(x)
#else
extern void ndelay(unsigned long ns);
extern void udelay(unsigned long us);
#endif /* !SYLIXOS_LIB */

#ifndef __LINUX_COMPAT_MALLOC
#define kmalloc(size, flags)	sys_malloc(size)
#define kzalloc(size, flags)	sys_zalloc(size)
#define vmalloc(size)		    sys_malloc(size)
#define kfree(ptr)		        sys_free(ptr)
#define vfree(ptr)		        sys_free(ptr)
#define GFP_KERNEL			    0
#define GFP_NOFS			    1
#endif /* !__LINUX_COMPAT_MALLOC */

#define ARCH_DMA_MINALIGN       8   /* 8-bytes */
#ifndef ALIGN
#define ALIGN(p, align)         (((unsigned long)(p) + (align - 1)) & ~(align - 1))
#endif

#define CONFIG_SYS_HZ           CLOCKS_PER_SEC
#define ENOTSUPP                ENOTSUP

#ifndef likely
#define likely(x)               (x)
#endif

#ifndef unlikely
#define unlikely(x)             (x)
#endif

#define cpu_to_le16(x)          htole16(x)
#define cpu_to_le32(x)          htole32(x)
#define cpu_to_le64(x)          htole64(x)
#define cpu_to_be16(x)          htobe16(x)
#define cpu_to_be32(x)          htobe32(x)
#define cpu_to_be64(x)          htobe64(x)

#define le16_to_cpu(x)          le16toh(x)
#define le32_to_cpu(x)          le32toh(x)
#define le64_to_cpu(x)          le64toh(x)
#define be16_to_cpu(x)          be16toh(x)
#define be32_to_cpu(x)          be32toh(x)
#define be64_to_cpu(x)          be64toh(x)

/*
 * user and kernel space copy
 */
#define copy_from_user(a, b, c)  \
        (lib_memcpy(a, b, c), 0)
#define copy_to_user(a, b, c)    \
        (lib_memcpy(a, b, c), 0)
/*
 * ..and if you can't take the strict
 * types, you can specify one yourself.
 *
 * Or not use min/max at all, of course.
 */
#define min_t(type, x, y) \
	({ type __x = (x); type __y = (y); __x < __y ? __x: __y; })
#define max_t(type, x, y) \
	({ type __x = (x); type __y = (y); __x > __y ? __x: __y; })

#ifndef BUG
#define BUG() do { \
	printf("BUG at %s:%d!\n", __FILE__, __LINE__); \
} while (0)

#define BUG_ON(condition) do { if (condition) BUG(); } while(0)
#endif /* BUG */

#define WARN_ON(x) if (x) {printf("WARNING in %s line %d\n" \
				  , __FILE__, __LINE__); }

#endif /* __LINUX_COMPAT_H */
