#ifndef __LINUX_TYPES_H
#define __LINUX_TYPES_H

#include "sys/types.h"

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

typedef uint16_t  __le16;
typedef uint16_t  __be16;
typedef uint32_t  __le32;
typedef uint32_t  __be32;
typedef uint64_t  __le64;
typedef uint64_t  __be64;

#endif /* __LINUX_TYPES_H */
