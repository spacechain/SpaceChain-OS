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
** 文   件   名: vi_fix.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 02 月 10 日
**
** 描        述: vi 编辑器移植层
*********************************************************************************************************/
#include "vi_fix.h"
#include "poll.h"
#include "sys/ioctl.h"
/*********************************************************************************************************
** 函数名称: get_terminal_width_height
** 功能描述: 获得终端窗口的大小
** 输　入  : fd            终端
**           width         宽度
**           height        高度
** 输　出  : ERROR_NONE 表示没有错误, -1 表示错误,
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int  get_terminal_width_height (const int  fd, int  *width, int  *height)
{
#define FIX_BUSYBOX_BUG

#ifdef FIX_BUSYBOX_BUG
    struct winsize  ws;
    
    if (ioctl(fd, TIOCGWINSZ, &ws) == ERROR_NONE) {
        *width  = ws.ws_col;
        *height = ws.ws_row;
    
    } else {
#endif
        *width  = __VI_TERMINAL_WIDTH;
        *height = __VI_TERMINAL_HEIGHT;
        
#ifdef FIX_BUSYBOX_BUG
    }
#endif
    
    return  (0);
}
/*********************************************************************************************************
** 函数名称: last_char_is
** 功能描述: 判断字符串最后一个字符
** 输　入  : s             字符串
**           c             最后一个字符
** 输　出  : 如果与给定相同返回指针, 如果不同返回 NULL
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
char *last_char_is (const char *s, int c)
{
    if (s && *s) {
        size_t sz = lib_strlen(s) - 1;
        s += sz;
        if ((unsigned char)*s == c) {
            return  (char*)s;
        }
    }
    return  LW_NULL;
}
/*********************************************************************************************************
** 函数名称: lib_xzalloc
** 功能描述: 内存分配(清零)
** 输　入  : s             大小
** 输　出  : 开辟成功返回内存地址, 否则直接退出线程.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
void  *lib_xzalloc (size_t s)
{
    PVOID   pvMem = lib_xmalloc(s);
    
    if (pvMem) {
        lib_bzero(pvMem, s);
    }

    return  (pvMem);
}
/*********************************************************************************************************
** 函数名称: full_write
** 功能描述: 将所有缓冲区内的数据写入文件
** 输　入  : fd        文件
**           buf       缓冲区
**           len       长度
** 输　出  : 写入的长度
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t full_write (int fd, const void *buf, size_t len)
{
    ssize_t temp;
    ssize_t total = 0;
    
    while (len) {
        temp = write(fd, (const void *)buf, len);
        if (temp < 0) {
            return  (temp);
        }
        total += temp;
        buf    = ((const char *)buf) + temp;
        len   -= (size_t)temp;
    }

	return  (total);
}
/*********************************************************************************************************
** 函数名称: vi_safe_write
** 功能描述: 安全写入文件
** 输　入  : fd        文件
**           buf       缓冲区
**           len       长度
** 输　出  : 写入的长度
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t vi_safe_write (int fd, const void *buf, size_t len)
{
    ssize_t  n;
    
    do {
        n = write(fd, buf, len);
    } while ((n < 0) && (errno == EINTR));
    
    return  (n);
}
/*********************************************************************************************************
** 函数名称: vi_safe_read
** 功能描述: 安全读出文件
** 输　入  : fd        文件
**           buf       缓冲区
**           len       长度
** 输　出  : 写入的长度
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t vi_safe_read (int fd, void *buf, size_t len)
{
    ssize_t  n;
    
    do {
        n = read(fd, buf, len);
    } while ((n < 0) && (errno == EINTR));
    
    return  (n);
}
/*********************************************************************************************************
** 函数名称: vi_safe_poll
** 功能描述: safe input/output multiplexing
** 输　入  : fds           pecifies the file descriptors to be examined and the events of interest for 
                           each file descriptor.
**           nfds          The array's members are pollfd structures within which fd specifies
**           timeout       wait (timeout) milliseconds for an event to occur, on any of the selected file 
                           descriptors.
                           0:  poll() returns immediately
                           -1: poll() blocks until a requested event occurs.
** 输　出  : A positive value indicates the total number of file descriptors that have been selected.
             A value of 0 indicates that the call timed out and no file descriptors have been selected. 
             Upon failure, poll() returns -1 and sets errno
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int vi_safe_poll (struct pollfd fds[], nfds_t nfds, int timeout)
{
    int  ret;
    
    do {
        ret = poll(fds, nfds, timeout);
    } while ((ret < 0) && (errno == EINTR));
    
    return  (ret);
}
/*********************************************************************************************************
** 函数名称: bb_putchar
** 功能描述: 输出一个字符
** 输　入  : c     字符
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int  bb_putchar (int ch)
{
    return  (fputc(ch, stdout));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
