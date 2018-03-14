
#ifndef __LINUX_VERSION_H
#define __LINUX_VERSION_H

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c)   (((a) << 16) + ((b) << 8) + (c))
#endif

#define LINUX_VERSION_CODE      KERNEL_VERSION(3, 4, 0)

#endif  /* __LINUX_VERSION_H */
