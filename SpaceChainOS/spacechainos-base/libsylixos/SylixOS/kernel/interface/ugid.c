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
** 文   件   名: ugid.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 03 月 30 日
**
** 描        述: 设置/获取 uid  gid (实验性质, 主要为了兼容性, SylixOS 不提倡使用此权限管理)
**
** 注        意: real user ID:实际用户ID,指的是进程执行者是谁 
                 effective user ID:有效用户ID,指进程执行时对文件的访问权限 
                 saved set-user-ID:保存设置用户ID,作为effective user ID的副本,
                                   在执行exec调用时后能重新恢复原来的effectiv user ID. 
                                   
** BUG:
2012.04.18  加入 getlogin api.
2013.01.23  修正 setgid() 对保存 gid 设置的错误.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "limits.h"
#include "pwd.h"
/*********************************************************************************************************
** 函数名称: getgid
** 功能描述: get current gid
** 输　入  : NONE
** 输　出  : gid
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
gid_t getgid (void)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    return  (ptcbCur->TCB_gid);
}
/*********************************************************************************************************
** 函数名称: setgid
** 功能描述: set current gid
** 输　入  : gid
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int setgid (gid_t  gid)
{
    INT             i;
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    /*
     *  If the effective user ID of the process is the root user, the process's real, effective, 
     *  and saved group IDs are set to the value of the GID parameter. 
     */
    if (ptcbCur->TCB_euid == 0) {
        ptcbCur->TCB_gid  = gid;
        ptcbCur->TCB_egid = gid;
        ptcbCur->TCB_sgid = gid;
        return  (ERROR_NONE);
    }
    
    /*
     *  the process effective group ID is reset if the GID parameter is equal to either the current real
     *  or saved group IDs, or one of its supplementary group IDs. Supplementary group IDs of the 
     *  calling process are not changed.
     */
    if (ptcbCur->TCB_gid == gid) {
        ptcbCur->TCB_egid = gid;
        return  (ERROR_NONE);
    }
    
    if (ptcbCur->TCB_sgid == gid) {                                     /*  == save gid                 */
        ptcbCur->TCB_egid =  gid;
        return  (ERROR_NONE);
    }

    for (i = 0; i < NGROUPS_MAX; i++) {
        if (ptcbCur->TCB_suppgid[i] == gid) {
            ptcbCur->TCB_egid = gid;
            return  (ERROR_NONE);
        }
    }
    
    errno = EPERM;
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: getegid
** 功能描述: get current egid
** 输　入  : NONE
** 输　出  : egid
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
gid_t getegid (void)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    return  (ptcbCur->TCB_egid);
}
/*********************************************************************************************************
** 函数名称: setegid
** 功能描述: set current egid
** 输　入  : egid
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int setegid (gid_t  egid)
{
    INT             i;
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    /*
     *  The effective user ID of the process is the root user.
     */
    if (ptcbCur->TCB_euid == 0) {
        ptcbCur->TCB_egid =  egid;
        return  (ERROR_NONE);
    }

    /*
     *  The EGID parameter is equal to either the current real or saved group IDs.
     */
    if (ptcbCur->TCB_gid == egid) {
        ptcbCur->TCB_egid =  egid;
        return  (ERROR_NONE);
    }
    
    if (ptcbCur->TCB_sgid == egid) {                                    /*  == save gid                 */
        ptcbCur->TCB_egid =  egid;
        return  (ERROR_NONE);
    }
    
    /*
     *  The EGID parameter is equal to one of its supplementary group IDs.
     */
    for (i = 0; i < NGROUPS_MAX; i++) {
        if (ptcbCur->TCB_suppgid[i] == egid) {
            ptcbCur->TCB_egid =  egid;
            return  (ERROR_NONE);
        }
    }
    
    errno = EPERM;
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: getuid
** 功能描述: get current uid
** 输　入  : NONE
** 输　出  : uid
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
uid_t getuid (void)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    return  (ptcbCur->TCB_uid);
}
/*********************************************************************************************************
** 函数名称: getuid
** 功能描述: get current uid
** 输　入  : uid
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int setuid (uid_t  uid)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    /*
     *  If the effective user ID of the process is the root user, the process's real, effective, 
     *  and saved user IDs are set to the value of the UID parameter. 
     */
    if (ptcbCur->TCB_euid == 0) {                                       /*  root                        */
        ptcbCur->TCB_uid  = uid;
        ptcbCur->TCB_euid = uid;
        ptcbCur->TCB_suid = uid;
        return  (ERROR_NONE);
    }
    
    /*
     *  the process effective user ID is reset if the UID parameter specifies either the 
     *  current real or saved user IDs.
     */
    if (ptcbCur->TCB_uid == uid) {
        ptcbCur->TCB_euid = uid;
        return  (ERROR_NONE);
    }
    
    if (ptcbCur->TCB_suid == uid) {
        ptcbCur->TCB_euid  = uid;
        return  (ERROR_NONE);
    }
    
    errno = EPERM;
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: geteuid
** 功能描述: get current euid
** 输　入  : NONE
** 输　出  : euid
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
uid_t geteuid (void)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    return  (ptcbCur->TCB_euid);
}
/*********************************************************************************************************
** 函数名称: seteuid
** 功能描述: set current euid
** 输　入  : euid
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int seteuid (uid_t  euid)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    /*
     *  The process effective user ID is reset if the UID parameter is equal to either the 
     *  current real or saved user IDs or if the effective user ID of the process is the root user.
     */
    if (ptcbCur->TCB_euid == 0) {                                       /*  root                        */
        ptcbCur->TCB_euid = euid;
        return  (ERROR_NONE);
    }
    
    if (ptcbCur->TCB_uid == euid) {
        ptcbCur->TCB_euid = euid;
        return  (ERROR_NONE);
    }
    
    if (ptcbCur->TCB_suid == euid) {
        ptcbCur->TCB_euid  = euid;
        return  (ERROR_NONE);
    }
    
    errno = EPERM;
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: setgroups
** 功能描述: 设置当前进程用户的添加组ID，调用此函数必须具有root权限
** 输　入  : groupsun      数量
**           grlist        组 ID 数组
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int setgroups (int groupsun, const gid_t grlist[])
{
    INT             i;
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    if ((groupsun < 1) ||
        (groupsun > NGROUPS_MAX) || 
        (grlist == LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (geteuid() != 0) {
        errno = EPERM;
        return  (PX_ERROR);
    }
    
    __KERNEL_ENTER();
    for (i = 0; i < groupsun; i++) {
        ptcbCur->TCB_suppgid[i] = grlist[i];
    }
    ptcbCur->TCB_iNumSuppGid = groupsun;
    __KERNEL_EXIT();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: getgroups
** 功能描述: 这个函数可以当前进程用户ID所属的添加组ID，
** 输　入  : groupsize     缓冲数量
**           grlist        缓冲
** 输　出  : 返回值为实际存储到grlist的gid个数。。 groupsize为预读的gid最大数，grlist存储gid
** 全局变量: 
** 调用模块: 
** 注  意  : POSIX 规定, 参数大小必须可以容纳所有添加组信息.

                                           API 函数
*********************************************************************************************************/
LW_API 
int getgroups (int groupsize, gid_t grlist[])
{
    INT             i;
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    __KERNEL_ENTER();
    if ((groupsize == 0) || (grlist == LW_NULL)) {                      /*  只统计数量                  */
        i = (int)ptcbCur->TCB_iNumSuppGid;
    
    } else if (groupsize < ptcbCur->TCB_iNumSuppGid) {
        __KERNEL_EXIT();
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
        
    } else {
        for (i = 0; i < (int)ptcbCur->TCB_iNumSuppGid; i++) {
            grlist[i] = ptcbCur->TCB_suppgid[i];
        }
    }
    __KERNEL_EXIT();
    
    return  (i);
}
/*********************************************************************************************************
** 函数名称: getlogin_r
** 功能描述: 获得当前登陆用户名
** 输　入  : groupsun      数量
**           grlist        组 ID 数组
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int getlogin_r (char *name, size_t namesize)
{
    struct passwd *pw;
    struct passwd  pwBuf;
           CHAR    cBuf[MAX_FILENAME_LENGTH];
    
    if (namesize < LOGIN_NAME_MAX) {
        return  (ERANGE);
    }
    
    getpwuid_r(getuid(), &pwBuf, cBuf, sizeof(cBuf), &pw);
    if (pw) {
        if ((lib_strlen(pw->pw_name) + 1) > namesize) {
            return  (ERANGE);
        }
        lib_strlcpy(name, pw->pw_name, namesize);
    
    } else {
        lib_strlcpy(name, "", namesize);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: getlogin
** 功能描述: 获得当前登陆用户名
** 输　入  : groupsun      数量
**           grlist        组 ID 数组
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
char *getlogin (void)
{
    static char cNameBuffer[LOGIN_NAME_MAX];
    
    getlogin_r(cNameBuffer, LOGIN_NAME_MAX);
    
    return  (cNameBuffer);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
