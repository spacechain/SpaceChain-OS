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
** 文   件   名: ttinyUserAdmin.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 05 月 09 日
**
** 描        述: shell 用户名管理.
**
** BUG:
2016.05.16  创建新用户自动创建用户目录.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "unistd.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_SHELL_EN > 0) && (LW_CFG_SHELL_USER_EN > 0)
#include "pwd.h"
#include "grp.h"
#include "limits.h"
#include "crypt.h"
#include "userdb.h"
#include "../ttinyShell/ttinyShellLib.h"
/*********************************************************************************************************
  文件单行缓冲
*********************************************************************************************************/
extern int  scanpw(FILE *fp, struct passwd *pwd, char *buffer, size_t bufsize);
extern int  scangr(FILE *fp, struct group *grp, char *buffer, size_t bufsize);
/*********************************************************************************************************
** 函数名称: __tshellUserShow
** 功能描述: 显示所有用户信息
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellUserShow (VOID)
{
    static PCHAR    pcUserInfo = "\n"\
    "     USER      ENABLE  UID   GID\n"
    "-------------- ------ ----- -----\n";

    struct passwd   pwd;
    FILE           *pfile;
    CHAR            cBuffer[MAX_FILENAME_LENGTH];
    
    pfile = fopen(_PATH_PASSWD, "r");
    if (pfile == LW_NULL) {
        fprintf(stderr, "can not open %s : %s.\n", _PATH_PASSWD, lib_strerror(errno));
        return;
    }
    
    printf(pcUserInfo);
    
    for(;;) {
        PCHAR   pcEnable;
    
        if (!scanpw(pfile, &pwd, cBuffer, sizeof(cBuffer))) {
            fclose(pfile);
            return;
        }
        
        if (lib_strcmp(pwd.pw_passwd, "x") == 0) {
            pcEnable = "yes";
        } else if (lib_strcmp(pwd.pw_passwd, "!!") == 0) {
            pcEnable = "nopass";
        } else {
            pcEnable = "no";
        }
        
        printf("%-14s %-6s %5d %5d\n", pwd.pw_name, pcEnable, pwd.pw_uid, pwd.pw_gid);
    }
    
    fclose(pfile);
}
/*********************************************************************************************************
** 函数名称: __tshellGroupShow
** 功能描述: 显示所有组信息
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellGroupShow (VOID)
{
    static PCHAR    pcGrpInfo = "\n"\
    "     GROUP      GID               USERs\n"
    "-------------- ----- --------------------------------\n";

    INT             i;
    struct group    grp;
    FILE           *pfile;
    CHAR            cBuffer[MAX_FILENAME_LENGTH];
    
    pfile = fopen(_PATH_GROUP, "r");
    if (pfile == LW_NULL) {
        fprintf(stderr, "can not open %s : %s.\n", _PATH_GROUP, lib_strerror(errno));
        return;
    }
    
    printf(pcGrpInfo);
    
    for(;;) {
        if (!scangr(pfile, &grp, cBuffer, sizeof(cBuffer))) {
            fclose(pfile);
            return;
        }
        
        printf("%-14s %5d ", grp.gr_name, grp.gr_gid);
        
        for (i = 0; grp.gr_mem[i] != LW_NULL; i++) {
            printf("%s,", grp.gr_mem[i]);
        }
        
        printf("\n");
    }
    
    fclose(pfile);
}
/*********************************************************************************************************
** 函数名称: __tshellUserGenpass
** 功能描述: 为一个用户生成密码
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellUserGenpass (VOID)
{
#if LW_CFG_SHELL_PASS_CRYPT_EN > 0
    INT             i;
    PCHAR           pcGet;
    CHAR            cPass[PASS_MAX];
    CHAR            cConfirm[PASS_MAX];
    CHAR            cSalt[14] = "$1$........$";
    CHAR            cBuffer[PASS_MAX];
    
    printf("\n---GENPASS---\n");
    
__re_input_pass:
    pcGet = getpass_r("input password:", cPass, PASS_MAX);
    if (pcGet == LW_NULL) {
        fprintf(stderr, "password input error.\n");
        goto    __re_input_pass;
    
    } else if (*pcGet == PX_EOS) {
        fprintf(stderr, "you must set a password.\n");
        goto    __re_input_pass;
    }
    
    
    pcGet = getpass_r("input confirm :", cConfirm, PASS_MAX);
    if (pcGet == LW_NULL) {
        fprintf(stderr, "confirm input error.\n");
        goto    __re_input_pass;
    }
    
    if (lib_strcmp(cPass, cConfirm)) {
        fprintf(stderr, "password confirm error.\n");
        goto    __re_input_pass;
    }
    
    for (i = 3; i < 12; i++) {
        INT iType = (lib_rand() % 3);
        
        switch (iType) {
        
        case 0:
            cSalt[i] = (CHAR)((lib_rand() % 10) + '0');
            break;
            
        case 1:
            cSalt[i] = (CHAR)((lib_rand() % 26) + 'A');
            break;
            
        case 2:
            cSalt[i] = (CHAR)((lib_rand() % 26) + 'a');
            break;
        }
    }
    
    crypt_safe(cPass, cSalt, cBuffer, sizeof(cBuffer));
    
    printf("%s\n", cBuffer);
#else
    printf("LW_CFG_SHELL_PASS_CRYPT_EN is 0, can not support genpass operate.\n");
#endif                                                                  /* LW_CFG_SHELL_PASS_CRYPT_EN   */
}
/*********************************************************************************************************
** 函数名称: __tshellUserCmdUser
** 功能描述: 系统命令 "user"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellUserCmdUser (INT  iArgC, PCHAR  ppcArgV[])
{
    if (geteuid() != 0) {
        fprintf(stderr, "Permission denied or can not access, you must use \"root\" user.\n");
        return  (-ERROR_TSHELL_EUSER);
    }

    if (iArgC < 2) {
        __tshellUserShow();
        return  (ERROR_NONE);
    }
    
    if (lib_strcmp(ppcArgV[1], "genpass") == 0) {
        __tshellUserGenpass();
        return  (ERROR_NONE);
    
    } else {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
}
/*********************************************************************************************************
** 函数名称: __tshellUserCmdGroup
** 功能描述: 系统命令 "group"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellUserCmdGroup (INT  iArgC, PCHAR  ppcArgV[])
{
    if (geteuid() != 0) {
        fprintf(stderr, "Permission denied or can not access, you must use \"root\" user.\n");
        return  (-ERROR_TSHELL_EUSER);
    }

    __tshellGroupShow();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellUserCmdUadd
** 功能描述: 系统命令 "uadd"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellUserCmdUadd (INT  iArgC, PCHAR  ppcArgV[])
{
    INT    iEn;
    uid_t  uid;
    gid_t  gid;
    
    if (geteuid() != 0) {
        fprintf(stderr, "Permission denied or can not access, you must use \"root\" user.\n");
        return  (-ERROR_TSHELL_EUSER);
    }
    
    if (iArgC != 8) {
__argument_error:
        fprintf(stderr, "argument error.\n");
        fprintf(stderr, "eg. uadd 'name' 'password' enable[0 / 1] uid gid comment homedir\n");
        return  (-1);
    }
    
    if (ppcArgV[3][0] == '0') {
        iEn = 0;
        
    } else {
        iEn = 1;
    }
    
    if (sscanf(ppcArgV[4], "%d", &uid) != 1) {
        goto    __argument_error;
    }
    
    if (sscanf(ppcArgV[5], "%d", &gid) != 1) {
        goto    __argument_error;
    }
    
    if (ppcArgV[7][0] != PX_ROOT) {
        fprintf(stderr, "home directory MUST use absolute path.\n");
        goto    __argument_error;
    }
    
    if (user_db_uadd(ppcArgV[1], ppcArgV[2], iEn, uid,  gid, 
                     ppcArgV[6], ppcArgV[7])) {
        if (errno == EEXIST) {
            fprintf(stderr, "User already exist.\n");
        
        } else {
            fprintf(stderr, "Can not create new user: %s\n", lib_strerror(errno));
        }
        return  (-1);
    
    } else {
        mkdir(ppcArgV[7], DEFAULT_DIR_PERM);
    }
    
    API_TShellFlushCache();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellUserCmdUdel
** 功能描述: 系统命令 "udel"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellUserCmdUdel (INT  iArgC, PCHAR  ppcArgV[])
{
    if (geteuid() != 0) {
        fprintf(stderr, "Permission denied or can not access, you must use \"root\" user.\n");
        return  (-ERROR_TSHELL_EUSER);
    }
    
    if (iArgC != 2) {
        fprintf(stderr, "argument error.\n");
        fprintf(stderr, "eg. udel 'name'\n");
        return  (-1);
    }
    
    if (user_db_udel(ppcArgV[1])) {
        if (errno == EINVAL) {
            fprintf(stderr, "User Invalidate.\n");
        
        } else {
            fprintf(stderr, "Can not delete user: %s\n", lib_strerror(errno));
        }
        return  (-1);
    }
    
    API_TShellFlushCache();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellUserCmdUmod
** 功能描述: 系统命令 "umod"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellUserCmdUmod (INT  iArgC, PCHAR  ppcArgV[])
{
    INT    iEn;
    
    if (geteuid() != 0) {
        fprintf(stderr, "Permission denied or can not access, you must use \"root\" user.\n");
        return  (-ERROR_TSHELL_EUSER);
    }
    
    if (iArgC != 5) {
        fprintf(stderr, "argument error.\n");
        fprintf(stderr, "eg. umod 'name' enable[0 / 1] comment homedir\n");
        return  (-1);
    }
    
    if (ppcArgV[2][0] == '0') {
        iEn = 0;
        
    } else {
        iEn = 1;
    }
    
    if (user_db_umod(ppcArgV[1], iEn, 
                     ppcArgV[3], ppcArgV[4])) {
        if (errno == EINVAL) {
            fprintf(stderr, "User Invalidate.\n");
        
        } else {
            fprintf(stderr, "Can not modify user: %s\n", lib_strerror(errno));
        }
        return  (-1);
    }
    
    API_TShellFlushCache();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellUserCmdGadd
** 功能描述: 系统命令 "gadd"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellUserCmdGadd (INT  iArgC, PCHAR  ppcArgV[])
{
    gid_t  gid;
    
    if (geteuid() != 0) {
        fprintf(stderr, "Permission denied or can not access, you must use \"root\" user.\n");
        return  (-ERROR_TSHELL_EUSER);
    }
    
    if (iArgC != 3) {
__argument_error:
        fprintf(stderr, "argument error.\n");
        fprintf(stderr, "eg. gadd 'group_name' gid\n");
        return  (-1);
    }
    
    if (sscanf(ppcArgV[2], "%d", &gid) != 1) {
        goto    __argument_error;
    }
    
    if (user_db_gadd(ppcArgV[1], gid)) {
        if (errno == EEXIST) {
            fprintf(stderr, "Group already exist.\n");
        
        } else {
            fprintf(stderr, "Can not create new group: %s\n", lib_strerror(errno));
        }
        return  (-1);
    }
    
    API_TShellFlushCache();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellUserCmdGdel
** 功能描述: 系统命令 "gdel"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellUserCmdGdel (INT  iArgC, PCHAR  ppcArgV[])
{
    if (geteuid() != 0) {
        fprintf(stderr, "Permission denied or can not access, you must use \"root\" user.\n");
        return  (-ERROR_TSHELL_EUSER);
    }
    
    if (iArgC != 2) {
        fprintf(stderr, "argument error.\n");
        fprintf(stderr, "eg. gdel 'group_name'\n");
        return  (-1);
    }
    
    if (user_db_gdel(ppcArgV[1])) {
        if (errno == EINVAL) {
            fprintf(stderr, "Group Invalidate.\n");
        
        } else if (errno == ENOTEMPTY) {
            fprintf(stderr, "Group not empty.\n");
        
        } else {
            fprintf(stderr, "Can not delete group: %s\n", lib_strerror(errno));
        }
        return  (-1);
    }
    
    API_TShellFlushCache();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellUserCmdPmod
** 功能描述: 系统命令 "pmod"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellUserCmdPmod (INT  iArgC, PCHAR  ppcArgV[])
{
    if (geteuid() != 0) {
        fprintf(stderr, "Permission denied or can not access, you must use \"root\" user.\n");
        return  (-ERROR_TSHELL_EUSER);
    }
    
    if (iArgC != 4) {
        fprintf(stderr, "argument error.\n");
        fprintf(stderr, "eg. pmod 'name' old_password new_password\n");
        return  (-1);
    }
    
    if (user_db_pmod(ppcArgV[1], ppcArgV[2], ppcArgV[3])) {
        if (errno == EINVAL) {
            fprintf(stderr, "User Invalidate.\n");
        
        } else {
            fprintf(stderr, "Can not modify password: %s\n", lib_strerror(errno));
        }
        return  (-1);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellUserCmdInit
** 功能描述: 初始化 tar 命令集
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __tshellUserCmdInit (VOID)
{
    API_TShellKeywordAdd("user", __tshellUserCmdUser);
    API_TShellFormatAdd("user", " [genpass]");
    API_TShellHelpAdd("user", "show user infomation, generate a password.\n"
                               "eg. user\n"
                               "    user genpass\n");
    
    API_TShellKeywordAdd("group", __tshellUserCmdGroup);
    API_TShellHelpAdd("group", "show group infomation.\n");
    
    API_TShellKeywordAdd("uadd", __tshellUserCmdUadd);
    API_TShellFormatAdd("uadd", " name password enable[0 / 1] uid gid comment homedir");
    API_TShellHelpAdd("uadd", "add a new user for this machine.\n"
                               "eg. uadd newuser passwd 1 10 20 a_new_user /home/newuser\n");
                               
    API_TShellKeywordAdd("udel", __tshellUserCmdUdel);
    API_TShellFormatAdd("udel", " name");
    API_TShellHelpAdd("udel", "delete a user in this machine.\n");
    
    API_TShellKeywordAdd("umod", __tshellUserCmdUmod);
    API_TShellFormatAdd("umod", " name enable[0 / 1] comment homedir");
    API_TShellHelpAdd("umod", "modify a user for this machine.\n"
                               "eg. umod user 0  user_comment /home/user\n");

    API_TShellKeywordAdd("gadd", __tshellUserCmdGadd);
    API_TShellFormatAdd("gadd", " group_name gid ");
    API_TShellHelpAdd("gadd", "add a new group for this machine.\n"
                               "eg. gadd newgroup 200\n");

    API_TShellKeywordAdd("gdel", __tshellUserCmdGdel);
    API_TShellFormatAdd("gdel", " group_name");
    API_TShellHelpAdd("gdel", "delete a group in this machine.\n");
                               
    API_TShellKeywordAdd("pmod", __tshellUserCmdPmod);
    API_TShellFormatAdd("pmod", " name old_password new_password");
    API_TShellHelpAdd("pmod", "modify a user password for this machine.\n"
                               "eg. pmod root root newpass\n");
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
                                                                        /*  LW_CFG_SHELL_USER_EN > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
