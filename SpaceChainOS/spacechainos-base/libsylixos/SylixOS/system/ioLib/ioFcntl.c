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
** 文   件   名: ioFcntl.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 01 月 14 日
**
** 描        述: fcntl (2013.01.05 从 lib 文件夹移至此处)

** BUG:
2011.02.26  F_SETFL 选项仅对 O_NONBLOCK 选项有效.
2011.05.24  修正 FIONBIO 参数的问题.
2012.12.21  支持 FD_CLOEXEC
2013.01.05  支持文件记录锁.
2013.11.18  加入对 F_DUPFD_CLOEXEC 与 F_DUP2FD_CLOEXEC 支持.
*********************************************************************************************************/
#define  __SYLIXOS_STDARG
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
** 函数名称: fcntl
** 功能描述: 文件控制
** 输　入  : iFd           文件描述符
**           iCmd          命令
**           ...
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  fcntl (INT  iFd, INT  iCmd, ...)
{
    REGISTER PLW_FD_ENTRY  pfdentry;
             va_list       varlist;
             INT           iFd2;
             INT           iFlag;
             INT           iSigno;
             pid_t         pidOwn;
             INT           iRetVal = ERROR_NONE;
             INT           iNbioStat;
             struct flock *pfl;
             
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry == LW_NULL) {
        errno = EBADF;
        return  (PX_ERROR);
    }
    
    switch (iCmd) {
    
    case F_DUPFD:                                                       /*  dup fd                      */
        va_start(varlist, iCmd);
        iFd2 = va_arg(varlist, INT);
        iRetVal = dupminfd(iFd, iFd2);
        va_end(varlist);
        break;
        
    case F_DUPFD_CLOEXEC:                                               /* dup() fildes with cloexec    */
        va_start(varlist, iCmd);
        iFd2 = va_arg(varlist, INT);
        iRetVal = dupminfd(iFd, iFd2);
        if (iRetVal >= 0) {
            API_IosFdSetCloExec(iRetVal, FD_CLOEXEC);
        }
        va_end(varlist);
        break;
        
    case F_DUP2FD:                                                      /*  dup2 fd                     */
        va_start(varlist, iCmd);
        iFd2 = va_arg(varlist, INT);
        iRetVal = dup2(iFd, iFd2);
        va_end(varlist);
        break;
        
    case F_DUP2FD_CLOEXEC:                                              /* dup2() fildes with cloexec   */
        va_start(varlist, iCmd);
        iFd2 = va_arg(varlist, INT);
        iRetVal = dup2(iFd, iFd2);
        if (iRetVal >= 0) {
            API_IosFdSetCloExec(iRetVal, FD_CLOEXEC);
        }
        va_end(varlist);
        break;
        
    case F_GETFD:                                                       /*  Get fildes flags            */
        iRetVal = 0;
        API_IosFdGetCloExec(iFd, &iRetVal);
        break;
        
    case F_SETFD:                                                       /*  set fildes flags            */
        va_start(varlist, iCmd);
        iFlag = va_arg(varlist, INT);
        iRetVal = API_IosFdSetCloExec(iFd, iFlag);
        va_end(varlist);
        break;
        
    case F_GETFL:                                                       /* Get file flags               */
        if (ioctl(iFd, FIOGETFL, (void *)&iRetVal) < 0) {
            iRetVal = pfdentry->FDENTRY_iFlag;
        }
        break;
        
    case F_SETFL:                                                       /* Set file flags               */
        va_start(varlist, iCmd);
        iFlag = va_arg(varlist, INT);
        if (iFlag & O_NONBLOCK) {                                       /* O_NONBLOCK I/O               */
            iNbioStat = 1;
            iRetVal = ioctl(iFd, FIONBIO, &iNbioStat);
            if (iRetVal < ERROR_NONE) {
                iRetVal = ioctl(iFd, FIOSETFL, (void *)O_NONBLOCK);
            }
            if (iRetVal >= ERROR_NONE) {
                pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
            }
        } else {
            iNbioStat = 0;
            iRetVal = ioctl(iFd, FIONBIO, &iNbioStat);
            if (iRetVal < ERROR_NONE) {
                iRetVal = ioctl(iFd, FIOSETFL, (void *)(pfdentry->FDENTRY_iFlag & ~O_NONBLOCK));
            }
            if (iRetVal >= ERROR_NONE) {
                pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
            }
        }
        va_end(varlist);
        break;
        
    case F_GETOWN:                                                      /* Get owner - for ASYNC        */
        iRetVal = ioctl(iFd, FIOGETOWN, &pidOwn);
        if (iRetVal >= ERROR_NONE) {
            iRetVal = (INT)pidOwn;
        }
        break;
        
    case F_SETOWN:                                                      /* Set owner - for ASYNC        */
        va_start(varlist, iCmd);
        pidOwn = va_arg(varlist, pid_t);
        iRetVal = ioctl(iFd, FIOSETOWN, (void *)pidOwn);
        va_end(varlist);
        break;
        
    case F_GETLK:                                                       /* Get record-locking           */
        va_start(varlist, iCmd);
        pfl = va_arg(varlist, struct flock *);
        iRetVal = ioctl(iFd, FIOGETLK, pfl);
        va_end(varlist);
        break;
    
    case F_SETLK:                                                       /* Set or Clear a record-lock   */
        va_start(varlist, iCmd);
        pfl = va_arg(varlist, struct flock *);
        iRetVal = ioctl(iFd, FIOSETLK, pfl);
        va_end(varlist);
        break;
        
    case F_SETLKW:                                                      /* F_SETLK with blocking        */
        va_start(varlist, iCmd);
        pfl = va_arg(varlist, struct flock *);
        iRetVal = ioctl(iFd, FIOSETLKW, pfl);
        va_end(varlist);
        break;
    
    case F_GETSIG:                                                      /* Get device notify signo      */
        iRetVal = ioctl(iFd, FIOGETSIG, &iSigno);
        if (iRetVal >= ERROR_NONE) {
            iRetVal = iSigno;
        }
        break;
        
    case F_SETSIG:                                                      /* Set device notify signo      */
        va_start(varlist, iCmd);
        iSigno = va_arg(varlist, INT);
        iRetVal = ioctl(iFd, FIOSETSIG, (void *)iSigno);
        va_end(varlist);
        break;
    
    default:                                                            /*  其他命令系统目前不支持      */
        errno = ENOSYS;
        return  (PX_ERROR);
    }
    
    return  (iRetVal);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
