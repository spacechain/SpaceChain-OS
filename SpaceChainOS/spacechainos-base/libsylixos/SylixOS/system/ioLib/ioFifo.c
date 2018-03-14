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
** 文   件   名: ioFifo.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 03 月 27 日
**
** 描        述: 系统 FIFO 操作库.

** BUG:
2012.08.25  pipe() 中 uiPipCnt++; 需要有保护.
2012.09.25  mkfifo() 和 pipe() 需要有 O_EXCL 参数.
2012.11.21  已经支持 unlink 延迟, 不再需要 O_TEMP.
2013.01.17  加入对 pipe2 的支持.
2013.05.08  匿名 pipe 存放在 /dev/pipe 目录下.
2013.08.14  pipe2 使用 atomic lock 操作.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
  pipe 默认路径名
*********************************************************************************************************/
#define __LW_PIPE_PATH      "/dev/pipe"
/*********************************************************************************************************
** 函数名称: mkfifo
** 功能描述: 创建一个新的 FIFO
** 输　入  : pcFifoName    目录名
**           mode          方式 (目前未使用)
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  mkfifo (CPCHAR  pcFifoName, mode_t  mode)
{
    REGISTER INT    iFd;
    
    (VOID)mode;
    
    iFd = open(pcFifoName, O_RDWR | O_CREAT | O_EXCL, S_IFIFO | DEFAULT_DIR_PERM);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    close(iFd);
    
    return (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pipe2
** 功能描述: 创建一个 "匿名" 管道
** 输　入  : iFd           返回的文件描述符数组 0:读 1:写
**           iFlag         方式  0 / O_NONBLOCK / O_CLOEXEC
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  pipe2 (INT  iFd[2], INT  iFlag)
{
    static UINT     uiPipeIndex = 0;
    
    INTREG          iregInterLevel;
    CHAR            cName[64];
    INT             iFifo;
    INT             iFdR, iFdW;
    UINT            uiPipe;
    
    if (!iFd) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iFlag &= (O_NONBLOCK | O_CLOEXEC);                                  /*  只允许这两个标志            */
    
    do {
        __LW_ATOMIC_LOCK(iregInterLevel);
        uiPipe = uiPipeIndex;
        uiPipeIndex++;
        __LW_ATOMIC_UNLOCK(iregInterLevel);
    
        snprintf(cName, sizeof(cName), "%s/%d", __LW_PIPE_PATH, uiPipe);
        
        iFifo = open(cName, O_RDWR | O_CREAT | O_EXCL, S_IFIFO | DEFAULT_DIR_PERM);
        if (iFifo < 0) {
            if (errno != ERROR_IO_FILE_EXIST) {                         /*  不是重复设备的其他错误      */
                return  (PX_ERROR);
            }
        
        } else {
            close(iFifo);
            break;                                                      /*  创建 FIFO 成功              */
        }
    } while (1);
    
    iFdR = open(cName, iFlag | O_RDONLY);
    if (iFdR >= 0) {
        ioctl(iFdR, FIOUNMOUNT);                                        /*  最后一次关闭时删除设备      */
        iFdW = open(cName, iFlag | O_WRONLY);
        if (iFdW >= 0) {
            iFd[0] = iFdR;
            iFd[1] = iFdW;
            return  (ERROR_NONE);
        }
        close(iFdR);                                                    /*  无法创建写端, 关闭读端      */
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: pipe
** 功能描述: 创建一个 "匿名" 管道
** 输　入  : iFd           返回的文件描述符数组 0:读 1:写
** 输　出  : ERROR_NONE    没有错误
**           其他值表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  pipe (INT  iFd[2])
{
    return  (pipe2(iFd, 0));
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
