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
** 文   件   名: ttinyShellFsCmd.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 07 月 27 日
**
** 描        述: 系统内部命令定义 (文件系统相关).

** BUG
2008.11.11  __tshellFsCmdCp() 不能以目录文件作为源文件.
2008.12.04  mv 移动文件时, 不在同一个卷, 需要进行复制, 删除操作.
2009.02.25  touch 创建的新文件大小为零.
2009.02.21  修改了 ls 和 attrib 针对非当前目录的处理.
2009.03.10  cp 文件时需要同步文件时间.
2009.03.11  ls 和 attrib 命令的对文件的大小打印有误.
2009.03.14  加入对 df 指令的支持.
2009.03.21  将 attrib 命令改为 ll.
            cp 命令的缓冲区根据文件大小自适应.
2009.05.19  mv 要以只写方式打开文件.
2009.06.30  cp 命令需要以 O_TRUNC 打开文件.
2009.07.03  修正了 GCC 编译时的警告.
2009.07.04  加入 shfile 指令, 用来执行指定的 shell 命令集文件.
2009.07.08  mv 操作时当目标存在时, 需要提示.
2009.07.28  当 mv 失败时, 直接进行 cp 和 rm 联合操作.
2009.08.27  ls 和 ll 当遇到 stat() 错误时, 使用默认属性显示.
2009.09.01  修正了 ls ll 命令对参数目录结尾加入 / 字符时出现的错误.
2009.10.02  无法获取 stat 的文件默认为字符类型文件.
2009.11.18  更新 df 显示.
2009.12.14  __tshellFsCmdLl() 支持打印修改时间.
            简化 ls 命令的打印内容.
            加入 mount 操作.
2009.12.15  加入 symbol link 文件操作.
            修正拷贝文件成功判断的一处 bug.
2009.12.29  ll 命令文件属性第一个字符显示更加详尽.
2010.09.10  ls 命令当 d_type != DT_UNKNOWN 时, 不需要调用 stat() 函数.
2011.03.04  cat 命令支持中间打断.
2011.03.22  ls 命令链接文件紫色字体.
2011.03.27  加入 mkfifo 命令.
2011.05.16  __tshellFsCmdMv() 使用 rename() 替换 ioctl(... FIOMOVE ...) 方式. 
2011.06.03  加入 varload 与 varsave 功能.
2011.06.23  使用新的色彩转义.
2011.08.07  __tshellFsCmdLl() 支持显示链接文件目标的功能.
2011.11.17  cat 命令不能打印非 reg 文件.
2012.03.25  cd 和 ch 命令需要有 stat 判断.
2012.10.20  加入一个 tmpname 命令.
2012.10.25  cd 命令使用 chdir() 实现.
2013.01.22  ll 加入对文件所有者的显示, chmod 加入对通配符的支持.
2013.04.01  修正 GCC 4.7.3 引发的新 warning.
2013.06.24  加入对 vfat 卷标的支持.
2014.05.30  加入查看文件夹大小的命令.
2014.10.10  cp 命令将目标文件设置为与原始文件相同的 mode.
2014.10.29  ls 显示的单行宽度通过 TIOCGWINSZ 获取.
2014.12.08  ln 支持 -f 选项.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
#include "sys/ioctl.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0 && LW_CFG_MAX_VOLUMES > 0
#include "../SylixOS/shell/ttinyShell/ttinyShell.h"
#include "../SylixOS/shell/ttinyShell/ttinyShellLib.h"
#include "../SylixOS/shell/ttinyShell/ttinyShellSysCmd.h"
#include "../SylixOS/shell/ttinyVar/ttinyVarLib.h"
#include "../SylixOS/fs/include/fs_fs.h"
#if LW_CFG_POSIX_EN > 0
#include "fnmatch.h"
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
static INT      __tshellFsCmdCp(INT  iArgC, PCHAR  ppcArgV[]);
LW_API time_t   API_RootFsTime(time_t  *time);
/*********************************************************************************************************
** 函数名称: __tshellFsCmdCd
** 功能描述: 系统命令 "cd"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdCd (INT  iArgC, PCHAR  ppcArgV[])
{
    REGISTER INT    iError;
             CHAR   cPath[MAX_FILENAME_LENGTH];

    if (iArgC > 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (iArgC == 1) {
        if (API_TShellGetUserHome(getuid(), cPath, 
                                  MAX_FILENAME_LENGTH) == ERROR_NONE) {
            iError = cd(cPath);
        
        } else {
            iError = cd(PX_STR_ROOT);
        }
    
    } else {
        PCHAR   pcTail;
        
        if (ppcArgV[1][0] == '~') {
            pcTail = &ppcArgV[1][1];
            
            if (API_TShellGetUserHome(getuid(), cPath, 
                                      MAX_FILENAME_LENGTH) < ERROR_NONE) {
                fprintf(stderr, "can not get current user home directory!\n");
                return  (-ERROR_TSHELL_EUSER);
            }
            lib_strlcat(cPath, pcTail, MAX_FILENAME_LENGTH);
            iError = cd(cPath);
        
        } else {
            iError = cd(ppcArgV[1]);
        }
    }
    
    if (iError) {
        if (errno == EACCES) {
            fprintf(stderr, "insufficient permissions.\n");
        } else if (errno == ENOTDIR) {
            fprintf(stderr, "not a directory!\n");
        } else {
            fprintf(stderr, "cd: error: %s\n", lib_strerror(errno));
        }
        
        return  (iError);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdCh
** 功能描述: 系统命令 "ch"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdCh (INT  iArgC, PCHAR  ppcArgV[])
{
    REGISTER INT    iError;

    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    iError = chdir(ppcArgV[1]);
    if (iError) {
        if (errno == EACCES) {
            fprintf(stderr, "insufficient permissions.\n");
        } else if (errno == ENOTDIR) {
            fprintf(stderr, "not a directory!\n");
        } else {
            fprintf(stderr, "ch: error: %s\n", lib_strerror(errno));
        }
        return  (iError);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdPwd
** 功能描述: 系统命令 "pwd"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdPwd (INT  iArgC, PCHAR  ppcArgV[])
{
    CHAR   cFileName[MAX_FILENAME_LENGTH];
    
    ioDefPathGet(cFileName);
    lib_strcat(cFileName, "\n");
    printf(cFileName);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdTmpname
** 功能描述: 系统命令 "tmpname"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdTmpname (INT  iArgC, PCHAR  ppcArgV[])
{
    char cBuf[L_tmpnam];
    
    printf("can mktmp as name: %s\n", tmpnam(cBuf));
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdMkdir
** 功能描述: 系统命令 "mkdir"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdMkdir (INT  iArgC, PCHAR  ppcArgV[])
{
    REGISTER INT    iError;
    
    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    iError = mkdir(ppcArgV[1], DEFAULT_DIR_PERM);
    if (iError) {
        if (errno == EACCES) {
            fprintf(stderr, "insufficient permissions.\n");
        } else {
            fprintf(stderr, "can not make this directory, error: %s\n", lib_strerror(errno));
        }
    }
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdMkfifo
** 功能描述: 系统命令 "mkfifo"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdMkfifo (INT  iArgC, PCHAR  ppcArgV[])
{
    REGISTER INT    iError;
    
    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    iError = mkfifo(ppcArgV[1], 0);
    if (iError) {
        if (errno == EACCES) {
            fprintf(stderr, "insufficient permissions.\n");
        } else {
            fprintf(stderr, "can not make this fifo, error: %s\n", lib_strerror(errno));
        }
    }
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdRmdir
** 功能描述: 系统命令 "rmdir"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdRmdir (INT  iArgC, PCHAR  ppcArgV[])
{
    REGISTER INT    iError;
    
    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    iError = rmdir(ppcArgV[1]);
    if (iError) {
        if (API_GetLastError() == ENOENT) {
            fprintf(stderr, "directory is not exist!\n");
        } else {
            fprintf(stderr, "can not remove directory, error: %s\n", lib_strerror(errno));
        }
    }
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdRm
** 功能描述: 系统命令 "rm"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdRm (INT  iArgC, PCHAR  ppcArgV[])
{
    REGISTER INT    iError;
    
    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    iError = unlink(ppcArgV[1]);
    if (iError) {
        if (API_GetLastError() == ENOENT) {
            fprintf(stderr, "file is not exist!\n");
        } else {
            fprintf(stderr, "can not remove this file, error: %s\n", lib_strerror(errno));
        }
    }
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdMv
** 功能描述: 系统命令 "mv"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdMv (INT  iArgC, PCHAR  ppcArgV[])
{
    REGISTER INT    iError = PX_ERROR;
    REGISTER INT    iFd;
             CHAR   cTemp[16];
    
    if (iArgC != 3) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    iFd = open(ppcArgV[1], O_RDONLY, 0);
    if (iFd < 0) {
        fprintf(stderr, "%s error: %s\n", ppcArgV[1], lib_strerror(errno));
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    iError = access(ppcArgV[2], 0);                                     /*  检测目标文件是否存在        */
    if (iError != PX_ERROR) {
__re_select:
        printf("destination file is exist, overwrite? (Y/N)\n");
        read(0, cTemp, 16);
        if ((cTemp[0] == 'N') ||
            (cTemp[0] == 'n')) {                                        /*  不覆盖                      */
            goto    __error_handle;
        } else if ((cTemp[0] == 'Y') ||
                   (cTemp[0] == 'y')) {                                 /*  覆盖                        */
            if (unlink(ppcArgV[2]) != ERROR_NONE) {
                goto    __error_handle;
            }
        } else {
            goto    __re_select;                                        /*  选择错误                    */
        }
    }
    close(iFd);                                                         /*  关闭源文件                  */
    
    iError = rename(ppcArgV[1], ppcArgV[2]);
    if (iError < 0) {
        iError = __tshellFsCmdCp(3, ppcArgV);                           /*  需要进行拷贝删除            */
        if (iError >= 0) {
            __tshellFsCmdRm(2, ppcArgV);                                /*  删除源文件                  */
        }
    }
    return  (iError);
    
__error_handle:
    close(iFd);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdCat
** 功能描述: 系统命令 "cat"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdCat (INT  iArgC, PCHAR  ppcArgV[])
{
             BOOL           bLastLf = LW_TRUE;
    REGISTER INT            iError;
    REGISTER ssize_t        sstNum;
    REGISTER INT            iFd;
             CHAR           cBuffer[MAX_FILENAME_LENGTH];
             struct stat    statFile;
             
    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    iFd = open(ppcArgV[1], O_RDONLY, 0);
    if (iFd < 0) {
        if (errno == EACCES) {
            fprintf(stderr, "insufficient permissions.\n");
        } else {
            fprintf(stderr, "file error %s!\n", lib_strerror(errno));
        }
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    iError = fstat(iFd, &statFile);
    if (iError < 0) {
        fprintf(stderr, "file stat error!\n");
        close(iFd);
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (S_ISDIR(statFile.st_mode)) {
        fprintf(stderr, "file read error: Is a directory!\n");
        close(iFd);
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    API_ThreadCleanupPush((VOIDFUNCPTR)close, (PVOID)iFd);
    
    do {
        sstNum = read(iFd, cBuffer, MAX_FILENAME_LENGTH);
        if (sstNum > 0) {
            write(1, cBuffer, (size_t)sstNum);
            bLastLf = (cBuffer[sstNum - 1] == '\n') ? LW_TRUE : LW_FALSE;
        }
    } while (sstNum > 0);
    
    API_ThreadCleanupPop(LW_TRUE);
    
    if (!bLastLf) {
        printf("\n");
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __checkIsTime
** 功能描述: 检查字串是否为时间
** 输　入  : pcStr         字符串
**           time          时间
** 输　出  : 0  表示转换正确
**           -1 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __checkIsTime (PCHAR  pcStr, time_t  *time)
{
    /*
     *  这里目前不处理时间
     */
    if (lib_strlen(pcStr) == 10) {
    }
    
    if (lib_strlen(pcStr) == 8) {
    }
    
    return  (-1);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdTouch
** 功能描述: 系统命令 "touch"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdTouch (INT  iArgC, PCHAR  ppcArgV[])
{
    REGISTER INT        iAflag = 0;
    REGISTER INT        iMflag = 0;
    REGISTER INT        iCflag = 0;
    
    REGISTER INT        i;
    REGISTER INT        iFileNameIndex = -1;
    
    REGISTER INT        iOc;
             time_t     timeNew;
             
    REGISTER INT        iFd;
    
    while ((iOc = getopt(iArgC, ppcArgV, "amc")) != -1) {
        switch (iOc) {
        case 'a':
            iAflag = 1;                                                 /*  只改变访问时间              */
            break;
        case 'm':
            iMflag = 1;                                                 /*  只改变修改时间              */
            break;
        case 'c':
            iCflag = 1;                                                 /*  若文件不存在，不创建它      */
            break;
        }
    }
    getopt_free();
    
    for (i = 1; i < iArgC; i++) {
        if (*ppcArgV[i] != '-') {
            if (__checkIsTime(ppcArgV[i], &timeNew)) {
                iFileNameIndex = i;
            }
        }
    }
    
    if (iFileNameIndex == -1) {
        fprintf(stderr, "file error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    (VOID)iAflag;                                                       /*  暂时不使用                  */
    (VOID)iMflag;
    
    if (iCflag == 0) {
        iFd = open(ppcArgV[iFileNameIndex], O_WRONLY | O_CREAT | O_TRUNC, DEFAULT_FILE_PERM);
        if (iFd < 0) {
            if (errno == EACCES) {
                fprintf(stderr, "insufficient permissions.\n");
            } else {
                fprintf(stderr, "can not create file! error: %s\n", lib_strerror(errno));
            }
            return  (-1);
        }
        close(iFd);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __fillWhite
** 功能描述: 补充若干个空格
** 输　入  : stLen      空格的长度
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __fillWhite (size_t  stLen)
{
    CHAR    cFmt[16];
    
    sprintf(cFmt, "%%-%zds", stLen);
    printf(cFmt, "");                                                   /*  补充空格                    */
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdLs
** 功能描述: 系统命令 "ls"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdLs (INT  iArgC, PCHAR  ppcArgV[])
{
#define __TSHELL_BYTES_PERLINE          80                              /*  单行 80 字符                */
#define __TSHELL_BYTES_PERFILE          16                              /*  每个文件名显示格长度        */

             CHAR            cDirName[MAX_FILENAME_LENGTH] = ".";
             size_t          stDirLen = 1;
    
    REGISTER DIR            *pdir;
    REGISTER struct dirent  *pdirent;
             struct stat     statGet;
             struct winsize  winsz;
             
             INT             iError;
             
             size_t          stTotalLen = 0;
             size_t          stPrintLen;
             size_t          stPad;
             
    if ((iArgC > 1) && (lib_strcmp(ppcArgV[1], "-l") == 0)) {
        printf("you can use 'll' command.\n");
        return  (ERROR_NONE);
    }
    
    if ((iArgC == 2) && lib_strcmp(ppcArgV[1], ".")) {                  /*  指定目录                    */
        lib_strcpy(cDirName, ppcArgV[1]);
        stDirLen = lib_strlen(cDirName);
        pdir     = opendir(cDirName);
        if (stDirLen > 0) {
            if (cDirName[stDirLen - 1] != PX_DIVIDER) {                 /*  参数目录不是以 / 结尾       */
                cDirName[stDirLen++] = PX_DIVIDER;                      /*  添加一个 /                  */
            }
        }
    } else {
        pdir = opendir(cDirName);                                       /*  当前目录                    */
    }
    
    if (!pdir) {
        if (errno == EACCES) {
            fprintf(stderr, "insufficient permissions.\n");
        } else {
            fprintf(stderr, "can not open dir! %s\n", lib_strerror(errno));
        }
        return  (-1);
    }
    
    if (ioctl(STD_OUT, TIOCGWINSZ, &winsz)) {                           /*  获得窗口信息                */
        winsz.ws_col = __TSHELL_BYTES_PERLINE;
    } else {
        winsz.ws_col = (unsigned short)ROUND_DOWN(winsz.ws_col, __TSHELL_BYTES_PERFILE);
    }
    
    do {
        pdirent = readdir(pdir);
        if (!pdirent) {
            break;
        
        } else {
            if ((pdirent->d_type == DT_UNKNOWN) ||                      /*  无法获取文件类型            */
                (pdirent->d_type == DT_REG)) {                          /*  需要获取文件信息            */
                if ((stDirLen > 1) || 
                    ((stDirLen == 1) && (cDirName[0] == PX_ROOT))) {
                    lib_strcpy(&cDirName[stDirLen], pdirent->d_name);   /*  链接指定目录                */
                    iError = stat(cDirName, &statGet);
                } else {
                    iError = stat(pdirent->d_name, &statGet);           /*  使用当前目录                */
                }
                if (iError < 0) {                                       /*  设备文件默认使用下面的属性  */
                    statGet.st_mode = S_IRUSR | S_IFREG;                /*  默认属性                    */
                }
            } else {
                statGet.st_mode = DTTOIF(pdirent->d_type);
            }
            
            API_TShellColorStart(pdirent->d_name, "", statGet.st_mode, STD_OUT);
            stPrintLen = printf("%-15s ", pdirent->d_name);             /*  打印文件名                  */
            if (stPrintLen > __TSHELL_BYTES_PERFILE) {
                stPad = ROUND_UP(stPrintLen, __TSHELL_BYTES_PERFILE)
                      - stPrintLen;                                     /*  计算填充数量                */
                __fillWhite(stPad);
            } else {
                stPad = 0;
            }
            stTotalLen += stPrintLen + stPad;
            API_TShellColorEnd(STD_OUT);
            
            if (stTotalLen >= winsz.ws_col) {
                printf("\n");                                           /*  换行                        */
                stTotalLen = 0;
            }
        }
    } while (1);
    
    if (stTotalLen) {
        printf("\n");                                                   /*  结束本行                    */
    }
    
    closedir(pdir);                                                     /*  关闭文件夹                  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdCmp
** 功能描述: 系统命令 "cmp"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0:  文件相同
**           1:  文件不同
**           -1: 出现错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdCmp (INT  iArgC, PCHAR  ppcArgV[])
{
             INT        iError = ERROR_NONE;
    REGISTER INT        iFdSrc;
    REGISTER INT        iFdDst = PX_ERROR;
    
    REGISTER ssize_t    sstRdSrcNum;
    REGISTER ssize_t    sstRdDstNum;
            
             CHAR       cTempSrc[128];
             CHAR       cTempDst[128];
             
             PCHAR      pcBufferSrc = LW_NULL;
             PCHAR      pcBufferDst = LW_NULL;
             
             size_t     stBuffer;
             
    struct   stat       statFile;
             off_t      oftSize;

    if (iArgC != 3) {
        fprintf(stderr, "parameter error!\n");
        return  (-1);
    }
    if (lib_strcmp(ppcArgV[1], ppcArgV[2]) == 0) {                      /*  文件相同                    */
        printf("file same!\n");
        return  (0);
    }
    
    iFdSrc = open(ppcArgV[1], O_RDONLY);
    if (iFdSrc < 0) {
        fprintf(stderr, "can not open %s!\n", ppcArgV[1]);
        return  (-1);
    }
    iFdDst = open(ppcArgV[2], O_RDONLY);
    if (iFdDst < 0) {
        close(iFdSrc);
        fprintf(stderr, "can not open %s!\n", ppcArgV[2]);
        return  (-1);
    }
    
    iError = fstat(iFdSrc, &statFile);                                  /*  首先比较文件大小            */
    if (iError != ERROR_NONE) {
        fprintf(stderr, "%s get stat error!\n", ppcArgV[1]);
        goto    __error_handle;
    }
    oftSize = statFile.st_size;
    iError = fstat(iFdDst, &statFile);
    if (iError != ERROR_NONE) {
        fprintf(stderr, "%s get stat error!\n", ppcArgV[2]);
        goto    __error_handle;
    }
    if (oftSize != statFile.st_size) {                                  /*  文件大小不同                */
        printf("file not same!\n");
        goto    __error_handle;
    }
    
    pcBufferSrc = (PCHAR)__SHEAP_ALLOC((16 * LW_CFG_KB_SIZE));          /*  开辟比较内存                */
    pcBufferDst = (PCHAR)__SHEAP_ALLOC((16 * LW_CFG_KB_SIZE));
    if (!pcBufferSrc || !pcBufferDst) {
        if (pcBufferSrc) {
            __SHEAP_FREE(pcBufferSrc);                                  /*  必须两个同时申请成功        */
        }
        if (pcBufferDst) {
            __SHEAP_FREE(pcBufferDst);
        }
        pcBufferSrc = cTempSrc;
        pcBufferDst = cTempDst;
        stBuffer    = sizeof(cTempSrc);
    } else {
        stBuffer    = (16 * LW_CFG_KB_SIZE);
    }
    
    for (;;) {                                                          /*  开始比较文件                */
        sstRdSrcNum = read(iFdSrc, pcBufferSrc, stBuffer);
        sstRdDstNum = read(iFdDst, pcBufferDst, stBuffer);
        if (sstRdSrcNum != sstRdDstNum) {
            fprintf(stderr, "file read error!\n");
            goto    __error_handle;
        }
        if (sstRdSrcNum <= 0) {                                         /*  文件读取完毕                */
            break;
        }
        if (lib_memcmp(pcBufferSrc, pcBufferDst, (UINT)sstRdSrcNum) != 0) {
            printf("file not same!\n");
            goto    __error_handle;
        }
    }
    close(iFdSrc);
    close(iFdDst);                                                      /*  关闭文件                    */
    if (stBuffer == (16 * LW_CFG_KB_SIZE)) {
        __SHEAP_FREE(pcBufferSrc);
        __SHEAP_FREE(pcBufferDst);                                      /*  释放缓冲区                  */
    }
    printf("file same!\n");
    
    return  (ERROR_NONE);
    
__error_handle:
    close(iFdSrc);
    close(iFdDst);
    
    if (pcBufferSrc && (pcBufferSrc != cTempSrc)) {
        __SHEAP_FREE(pcBufferSrc);
    }
    if (pcBufferDst && (pcBufferDst != cTempDst)) {
        __SHEAP_FREE(pcBufferDst);
    }
    
    return  (-1);
}
/*********************************************************************************************************
** 函数名称: __buildDstFileName
** 功能描述: 创建目标文件名
** 输　入  : pcSrc         源文件
**           pcDstDir      目标目录
**           pcBuffer      输出缓冲
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID __buildDstFileName (CPCHAR  pcSrc, CPCHAR  pcDstDir, PCHAR  pcBuffer)
{
    size_t  stDstDirLen   = lib_strlen(pcDstDir);
    PCHAR   pcSrcFileName = lib_rindex(pcSrc, PX_DIVIDER);

    if (pcSrcFileName == LW_NULL) {
        pcSrcFileName =  (PCHAR)pcSrc;
    } else {
        pcSrcFileName++;
    }

    lib_strlcpy(pcBuffer, pcDstDir, MAX_FILENAME_LENGTH - 1);

    if (pcBuffer[stDstDirLen - 1] != PX_DIVIDER) {
        pcBuffer[stDstDirLen]      = PX_DIVIDER;
        pcBuffer[stDstDirLen + 1]  = PX_EOS;
    }

    lib_strlcat(pcBuffer, pcSrcFileName, MAX_FILENAME_LENGTH);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdCp
** 功能描述: 系统命令 "cp"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdCp (INT  iArgC, PCHAR  ppcArgV[])
{
#define __LW_CP_BUF_SZ  (128 * LW_CFG_KB_SIZE)

             INT        iError = PX_ERROR;
    REGISTER INT        iFdSrc;
    REGISTER INT        iFdDst = PX_ERROR;
    
             CHAR       cDstFile[MAX_FILENAME_LENGTH] = "\0";
             PCHAR      pcDest;
             PCHAR      pcSrc;
             BOOL       bForce = LW_FALSE;

    REGISTER ssize_t    sstRdNum;
    REGISTER ssize_t    sstWrNum;
            
             CHAR       cTemp[128];
             PCHAR      pcBuffer = cTemp;
             size_t     stBuffer = sizeof(cTemp);
             size_t     stOptim;
             
    struct   stat       statFile;
    struct   stat       statDst;
    
             time_t     timeStart;
             time_t     timeEnd;
             time_t     timeDiff;

    if (iArgC == 3) {
        pcDest = ppcArgV[2];
        pcSrc  = ppcArgV[1];
    
    } else if (iArgC == 4) {
        if (ppcArgV[1][0] != '-') {
            fprintf(stderr, "option error!\n");
            return  (-1);
        }
        if (lib_strchr(ppcArgV[1], 'f')) {                              /*  强行复制                    */
            bForce = LW_TRUE;
        }
        pcDest = ppcArgV[3];
        pcSrc  = ppcArgV[2];
        
    } else {
        fprintf(stderr, "parameter error!\n");
        return  (-1);
    }
    
    if (lib_strcmp(pcSrc, pcDest) == 0) {                               /*  文件重复                    */
        fprintf(stderr, "parameter error!\n");
        return  (-1);
    }
    
    iFdSrc = open(pcSrc, O_RDONLY, 0);                                  /*  打开源文件                  */
    if (iFdSrc < 0) {
        fprintf(stderr, "can not open source file!\n");
        return  (-1);
    }
    
    iError = fstat(iFdSrc, &statFile);                                  /*  获得源文件属性              */
    if (iError < 0) {
        goto    __error_handle;
    }
    if (S_ISDIR(statFile.st_mode)) {                                    /*  不能复制目录文件            */
        fprintf(stderr, "can not copy directory!\n");
        errno  = EISDIR;
        iError = PX_ERROR;
        goto    __error_handle;
    }
    
    iError = stat(pcDest, &statDst);
    if (iError == ERROR_NONE) {
        if (S_ISDIR(statDst.st_mode)) {
            __buildDstFileName(pcSrc, pcDest, cDstFile);                /*  生成目标文件路径            */
        }
    }
    if (cDstFile[0] == PX_EOS) {
        lib_strlcpy(cDstFile, pcDest, MAX_FILENAME_LENGTH);
    }

    if (!bForce) {
        iError = access(cDstFile, 0);                                   /*  检测目标文件是否存在        */
        if (iError == ERROR_NONE) {
__re_select:
            printf("destination file is exist, overwrite? (Y/N)\n");
            read(0, cTemp, 128);
            if ((cTemp[0] == 'N') || (cTemp[0] == 'n')) {               /*  不覆盖                      */
                iError = PX_ERROR;
                goto    __error_handle;
            
            } else if ((cTemp[0] != 'Y') && (cTemp[0] != 'y')) {        /*  选择错误                    */
                goto    __re_select;
            
            } else {                                                    /*  选择覆盖                    */
                unlink(cDstFile);                                       /*  删除目标文件                */
            }
        }
    } else {
        iError = access(cDstFile, 0);                                   /*  检测目标文件是否存在        */
        if (iError == ERROR_NONE) {
            unlink(cDstFile);                                           /*  删除目标文件                */
        }
    }
                                                                        /*  创建目标文件                */
    iFdDst = open(cDstFile, (O_WRONLY | O_CREAT | O_TRUNC), DEFFILEMODE);
    if (iFdDst < 0) {
        close(iFdSrc);
        fprintf(stderr, "can not open destination file!\n");
        return  (-1);
    }
    
    stOptim = (UINT)__MIN(__LW_CP_BUF_SZ, statFile.st_size);            /*  计算缓冲区                  */
    if (stOptim > 128) {
        pcBuffer = (PCHAR)__SHEAP_ALLOC(stOptim);                       /*  分配缓冲区                  */
        if (pcBuffer == LW_NULL) {
            pcBuffer =  cTemp;                                          /*  使用局部变量缓冲            */
        
        } else {
            stBuffer =  stOptim;
        }
    }
    
    lib_time(&timeStart);                                               /*  记录起始时间                */
    
    for (;;) {                                                          /*  开始拷贝文件                */
        sstRdNum = read(iFdSrc, pcBuffer, stBuffer);
        if (sstRdNum > 0) {
            sstWrNum = write(iFdDst, pcBuffer, (size_t)sstRdNum);
            if (sstWrNum != sstRdNum) {                                 /*  写入文件错误                */
                fprintf(stderr, "can not write destination file! error: %s\n", lib_strerror(errno));
                iError = PX_ERROR;
                break;
            }
        } else if (sstRdNum == 0) {                                     /*  拷贝完毕                    */
            iError = ERROR_NONE;
            break;
        
        } else {
            iError = PX_ERROR;                                          /*  读取数据错误                */
            break;
        }
    }
    
    if (iError == ERROR_NONE) {
        lib_time(&timeEnd);                                             /*  记录结束时间                */
        timeDiff = timeEnd - timeStart;
        
        printf("copy complete. size:%lld(Bytes) time:%lld(s) speed:%lld(Bps)\n", 
               (INT64)statFile.st_size,
               (INT64)timeDiff,
               (INT64)(statFile.st_size / (timeDiff ? timeDiff : 1)));
    }
        
    if (pcBuffer != cTemp) {
        __SHEAP_FREE(pcBuffer);                                         /*  释放缓冲                    */
    }
    
__error_handle:
    close(iFdSrc);
    if (iFdDst >= 0) {
        fchmod(iFdDst, statFile.st_mode);                               /*  设置为与源文件相同的 mode   */
        close(iFdDst);
    }
    
    if (iError == ERROR_NONE) {                                         /*  拷贝完成                    */
        struct utimbuf  utimbDst;
        
        utimbDst.actime  = statFile.st_atime;                           /*  修改复制后文件的时间信息    */
        utimbDst.modtime = statFile.st_mtime;
        
        utime(cDstFile, &utimbDst);                                     /*  设置文件时间                */
    }
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __tshellFsShowMode
** 功能描述: 显示文件 mode 选项
** 输　入  : pstat     文件选项
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID __tshellFsShowMode (struct stat  *pstat)
{
    CHAR    cBuffer[11];
    
    if (S_ISLNK(pstat->st_mode)) {
        cBuffer[0] = 'l';
    
    } else if (S_ISDIR(pstat->st_mode)) {
        cBuffer[0] = 'd';
    
    } else if (S_ISCHR(pstat->st_mode)) {
        cBuffer[0] = 'c';
    
    } else if (S_ISBLK(pstat->st_mode)) {
        cBuffer[0] = 'b';
    
    } else if (S_ISSOCK(pstat->st_mode)) {
        cBuffer[0] = 's';
    
    } else if (S_ISFIFO(pstat->st_mode)) {
        cBuffer[0] = 'f';
    
    } else {
        cBuffer[0] = '-';
    }
    
    if (pstat->st_mode & S_IRUSR) {
        cBuffer[1] = 'r';
    } else {
        cBuffer[1] = '-';
    }
    
    if (pstat->st_mode & S_IWUSR) {
        cBuffer[2] = 'w';
    } else {
        cBuffer[2] = '-';
    }
    
    if (pstat->st_mode & S_IXUSR) {
        if (pstat->st_mode & S_ISUID) {
            cBuffer[3] = 's';
        } else {
            cBuffer[3] = 'x';
        }
    } else {
        if (pstat->st_mode & S_ISUID) {
            cBuffer[3] = 'S';
        } else {
            cBuffer[3] = '-';
        }
    }
    
    if (pstat->st_mode & S_IRGRP) {
        cBuffer[4] = 'r';
    } else {
        cBuffer[4] = '-';
    }
    
    if (pstat->st_mode & S_IWGRP) {
        cBuffer[5] = 'w';
    } else {
        cBuffer[5] = '-';
    }
    
    if (pstat->st_mode & S_IXGRP) {
        if (pstat->st_mode & S_ISGID) {
            cBuffer[6] = 's';
        } else {
            cBuffer[6] = 'x';
        }
    } else {
        if (pstat->st_mode & S_ISGID) {
            cBuffer[6] = 'S';
        } else {
            cBuffer[6] = '-';
        }
    }
    
    if (pstat->st_mode & S_IROTH) {
        cBuffer[7] = 'r';
    } else {
        cBuffer[7] = '-';
    }
    
    if (pstat->st_mode & S_IWOTH) {
        cBuffer[8] = 'w';
    } else {
        cBuffer[8] = '-';
    }
    
    if (pstat->st_mode & S_IXOTH) {
        cBuffer[9] = 'x';
    } else {
        cBuffer[9] = '-';
    }
    cBuffer[10] = PX_EOS;
    
    printf(cBuffer);
}
/*********************************************************************************************************
** 函数名称: __tshellFsShowFile
** 功能描述: 显示文件详细信息
** 输　入  : pcFileName 文件名
**           pcStat     文件名
**           pstat      文件选项
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID __tshellFsShowFile (CPCHAR  pcFileName, PCHAR  pcStat, struct stat  *pstat)
{
    PCHAR           pcN;
    struct tm       tmBuf;
    CHAR            cTimeBuf[32];                                       /*  bigger than sizeof(ASCBUF)  */
    CHAR            cBuffer[MAX_FILENAME_LENGTH];
    
    __tshellFsShowMode(pstat);                                          /*  显示文件属性                */
            
    if (API_TShellGetUserName(pstat->st_uid,
                              cBuffer, sizeof(cBuffer))) {              /*  获得文件所属用户名          */
        printf(" %-8d", pstat->st_uid);
    } else {
        printf(" %-8s", cBuffer);
    }
    
    if (API_TShellGetGrpName(pstat->st_gid,
                             cBuffer, sizeof(cBuffer))) {               /*  获得文件所属用户名          */
        printf(" %-8d", pstat->st_gid);
    } else {
        printf(" %-8s", cBuffer);
    }
    
    lib_localtime_r(&pstat->st_mtime, &tmBuf);                          /*  转换为 tm 格式              */
    lib_asctime_r(&tmBuf, cTimeBuf);                                    /*  生成字符串                  */
    pcN = lib_index(cTimeBuf, '\n');
    if (pcN) {
        *pcN = PX_EOS;
    }
    
    printf(" %s", cTimeBuf);                                            /*  打印修改时间                */
    
    if (S_ISDIR(pstat->st_mode)) {
        API_TShellColorStart(pcFileName, "", pstat->st_mode, STD_OUT);
        printf("           %s/\n", pcFileName);
        API_TShellColorEnd(STD_OUT);
                              
    } else if (S_ISLNK(pstat->st_mode)) {                               /*  链接文件                    */
        CHAR            cDstName[MAX_FILENAME_LENGTH] = "<unknown>";
        struct stat     statDst;
        ssize_t         sstLen;
        
        statDst.st_mode = 0;
        sstLen = readlink(pcStat, cDstName, MAX_FILENAME_LENGTH);
        if (sstLen >= 0) {
            cDstName[sstLen] = PX_EOS;                                  /*  加入结束符                  */
        }
        stat(cDstName, &statDst);
        API_TShellColorStart(pcFileName, cDstName, pstat->st_mode, STD_OUT);
        printf("           %s -> ", pcFileName);
        API_TShellColorStart(cDstName, "", statDst.st_mode, STD_OUT);
        printf("%s\n", cDstName);
        API_TShellColorEnd(STD_OUT);
    
    } else {
        if (pstat->st_size > (10 * (UINT64)LW_CFG_GB_SIZE)) {
            printf(" %6zdGB, ", (size_t)(pstat->st_size / LW_CFG_GB_SIZE));
        } else if (pstat->st_size > (10 * LW_CFG_MB_SIZE)) {
            printf(" %6zdMB, ", (size_t)(pstat->st_size / LW_CFG_MB_SIZE));
        } else if (pstat->st_size > (10 * LW_CFG_KB_SIZE)) {
            printf(" %6zdKB, ", (size_t)(pstat->st_size / LW_CFG_KB_SIZE));
        } else {
            printf(" %6zd B, ", (size_t)(pstat->st_size));
        }
        
        API_TShellColorStart(pcFileName, "", pstat->st_mode, STD_OUT);
        printf("%s\n", pcFileName);
        API_TShellColorEnd(STD_OUT);
    }
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdLl
** 功能描述: 系统命令 "ll"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdLl (INT  iArgC, PCHAR  ppcArgV[])
{
             PCHAR           pcStat;
             CHAR            cDirName[MAX_FILENAME_LENGTH] = ".";
             size_t          stDirLen = 1;
             
    REGISTER DIR            *pdir;
    REGISTER struct dirent  *pdirent;
             struct stat     statGet;
             
             INT             iError;
             INT             iItemNum = 0;
    
    if ((iArgC == 2) && lib_strcmp(ppcArgV[1], ".")) {                  /*  指定目录                    */
        lib_strcpy(cDirName, ppcArgV[1]);
        if (stat(cDirName, &statGet) == ERROR_NONE) {
            if (!S_ISDIR(statGet.st_mode)) {                            /*  不是目录                    */
                PCHAR   pcFile;
                _PathLastName(cDirName, &pcFile);
                __tshellFsShowFile(pcFile, pcFile, &statGet);
                iItemNum = 1;
                goto    __display_over;
            }
        }
        
        stDirLen = lib_strlen(cDirName);
        pdir     = opendir(cDirName);
        if (stDirLen > 0) {
            if (cDirName[stDirLen - 1] != PX_DIVIDER) {                 /*  参数目录不是以 / 结尾       */
                cDirName[stDirLen++] = PX_DIVIDER;                      /*  添加一个 /                  */
            }
        }
    } else {
        pdir = opendir(cDirName);                                       /*  当前目录                    */
    }
    
    if (!pdir) {
        if (errno == EACCES) {
            fprintf(stderr, "insufficient permissions.\n");
        } else {
            fprintf(stderr, "can not open dir %s!\n", lib_strerror(errno));
        }
        return  (-1);
    }
    
    do {
        pdirent = readdir(pdir);
        if (!pdirent) {
            break;
        
        } else {
            if ((stDirLen > 1) || 
                ((stDirLen == 1) && (cDirName[0] == PX_ROOT))) {
                lib_strcpy(&cDirName[stDirLen], pdirent->d_name);       /*  链接指定目录                */
                pcStat = cDirName;
                iError = lstat(cDirName, &statGet);
            
            } else {
                pcStat = pdirent->d_name;
                iError = lstat(pdirent->d_name, &statGet);              /*  使用当前目录                */
            }
            if (iError < 0) {                                           /*  设备文件默认使用下面的属性  */
                statGet.st_dev     = 0;
                statGet.st_ino     = 0;
                statGet.st_mode    = 0666 | S_IFCHR;                    /*  默认属性                    */
                statGet.st_nlink   = 0;
                statGet.st_uid     = 0;
                statGet.st_gid     = 0;
                statGet.st_rdev    = 1;
                statGet.st_size    = 0;
                statGet.st_blksize = 0;
                statGet.st_blocks  = 0;
                statGet.st_atime   = API_RootFsTime(LW_NULL);           /*  默认使用 root fs 基准时间   */
                statGet.st_mtime   = API_RootFsTime(LW_NULL);
                statGet.st_ctime   = API_RootFsTime(LW_NULL);
            }
            
            __tshellFsShowFile(pdirent->d_name, pcStat, &statGet);
            
            iItemNum++;
        }
    } while (1);
    
    closedir(pdir);
    
__display_over:
    printf("      total items: %d\n", iItemNum);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __getDsize
** 功能描述: 递归获取文件夹的文件数量和大小
** 输　入  : pcDirName     目录名
**           stDirLen      当前目录名长度
**           pulFileCnt    文件数量
**           poftSize      总大小
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __getDsize (PCHAR  pcDirName, size_t  stDirLen, ULONG  *pulFileCnt, off_t  *poftSize)
{
    DIR            *pdir;
    struct dirent  *pdirent;
    struct stat     statGet;
    
    pdir = opendir(pcDirName);
    if (pdir) {
        do {
            pdirent = readdir(pdir);
            if (pdirent) {
                bnprintf(pcDirName, MAX_FILENAME_LENGTH, stDirLen, "/%s", pdirent->d_name);
                if (lstat(pcDirName, &statGet) == ERROR_NONE) {
                    if (S_ISDIR(statGet.st_mode)) {
                        __getDsize(pcDirName, lib_strlen(pcDirName), pulFileCnt, poftSize);
                    
                    } else {
                        (*pulFileCnt) += 1;
                        (*poftSize)   += statGet.st_size;
                    }
                }
                pcDirName[stDirLen] = PX_EOS;
            }
        } while (pdirent);
        closedir(pdir);
    }
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdDsize
** 功能描述: 系统命令 "dsize"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdDsize (INT  iArgC, PCHAR  ppcArgV[])
{
    ULONG       ulFiles = 0ul;
    off_t       oftSize = 0;
    struct stat statGet;
    CHAR        cDirName[MAX_FILENAME_LENGTH];
    
    if (iArgC < 2) {
        fprintf(stderr, "df arguments error, (dsize directory)\n");
        return  (-1);
    }
    
    if (stat(ppcArgV[1], &statGet)) {
        fprintf(stderr, "can not get %s stat().\n", ppcArgV[1]);
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (S_ISDIR(statGet.st_mode)) {
        lib_strlcpy(cDirName, ppcArgV[1], MAX_FILENAME_LENGTH);
        printf("scanning...\n");
        __getDsize(cDirName, lib_strlen(cDirName), &ulFiles, &oftSize);
        printf("total file %lu size %llu bytes.\n", ulFiles, oftSize);
        
    } else {
        printf("total file 1 size %llu bytes.\n", statGet.st_size);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdDf
** 功能描述: 系统命令 "df"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdDf (INT  iArgC, PCHAR  ppcArgV[])
{
    static PCHAR   pcVolumeInfoHdr = \
                    "    VOLUME       TOTAL     FREE    USED RO            FS TYPE\n"
                    "-------------- --------- --------- ---- -- ---------------------------------\n";

    struct statfs       statfsGet;
           
           /*
            *  磁盘信息
            */
           UINT64       ullFree;
           UINT64       ullTotal;
           
           /*
            *  显示用变量
            */
           ULONG        ulTotalDisp;
           ULONG        ulTotalPoint;
           ULONG        ulFreeDisp;
           ULONG        ulFreePoint;
           
           /*
            *  显示单位的比值
            */
           ULONG        ulTotalDiv;
           ULONG        ulFreeDiv;
           
           /*
            *  显示单位
            */
           PCHAR        pcTotalUnit = "";
           PCHAR        pcFreeUnit  = "";
           
           /*
            *  文件系统类型
            */
           PCHAR        pcFsType = "unknown";
           
           /*
            *  使用百分比
            */
           INT          iUseagePercent;
           
           /*
            *  是否为只读
            */
           PCHAR        pcRo = "n";

    if (iArgC != 2) {
        fprintf(stderr, "df arguments error, (df volname)\n");
        return  (-1);
    }
    
    if (statfs(ppcArgV[1], &statfsGet) < 0) {
        if (errno == EACCES) {
            fprintf(stderr, "insufficient permissions.\n");
        } else {
            perror("volume state error");
        }
        return  (-1);
    }
    
    ullFree  = ((UINT64)statfsGet.f_bfree  * statfsGet.f_bsize);
    ullTotal = ((UINT64)statfsGet.f_blocks * statfsGet.f_bsize);
    
    if (ullTotal > (2ul * LW_CFG_GB_SIZE)) {
        ulTotalDiv  = LW_CFG_GB_SIZE;
        pcTotalUnit = "GB";
    } else if (ullTotal > (2ul * LW_CFG_MB_SIZE)) {
        ulTotalDiv  = LW_CFG_MB_SIZE;
        pcTotalUnit = "MB";
    } else {
        ulTotalDiv  = LW_CFG_KB_SIZE;
        pcTotalUnit = "KB";
    }
    
    if (ullFree > (2ul * LW_CFG_GB_SIZE)) {
        ulFreeDiv  = LW_CFG_GB_SIZE;
        pcFreeUnit = "GB";
    } else if (ullFree > (2ul * LW_CFG_MB_SIZE)) {
        ulFreeDiv  = LW_CFG_MB_SIZE;
        pcFreeUnit = "MB";
    } else {
        ulFreeDiv  = LW_CFG_KB_SIZE;
        pcFreeUnit = "KB";
    }
    
    ulTotalDisp  = (ULONG)(ullTotal / ulTotalDiv);
    if (ullTotal % ulTotalDiv) {
        ulTotalPoint = (ULONG)(ullTotal / (UINT64)(ulTotalDiv / 100));
        ulTotalPoint = (ulTotalPoint % 100);
    } else {
        ulTotalPoint = 0;
    }
    
    ulFreeDisp   = (ULONG)(ullFree / ulFreeDiv);
    if (ullFree % ulFreeDiv) {
        ulFreePoint = (ULONG)(ullFree / (UINT64)(ulFreeDiv / 100));
        ulFreePoint = (ulFreePoint % 100);
    } else {
        ulFreePoint = 0;
    }
    
    {
        INT  iFd = open(ppcArgV[1], O_RDONLY, 0);
        if (iFd >= 0) {
            ioctl(iFd, FIOFSTYPE, (LONG)&pcFsType);
            close(iFd);
        }
    }
    
    printf(pcVolumeInfoHdr);
    
    if (statfsGet.f_flag & ST_RDONLY) {
        pcRo = "y";
    }
    
    if (ullFree > ullTotal) {
        printf("%-14s %4lu.%02lu%-2s   unknown ---%% %-2s %s\n", ppcArgV[1], 
               ulTotalDisp, ulTotalPoint, 
               pcTotalUnit, pcRo, pcFsType);

    } else if (ullTotal == 0) {
        printf("%-14s %4lu.%02lu%-2s %4lu.%02lu%-2s %3d%% %-2s %s\n", ppcArgV[1],
               0ul, 0ul, "KB", 0ul, 0ul, "KB",
               100, pcRo, pcFsType);
    
    } else {
        UINT64      ullUseBlocks = (UINT64)(statfsGet.f_blocks - statfsGet.f_bfree);
    
        ullUseBlocks *= 100;
        iUseagePercent = (INT)(ullUseBlocks / statfsGet.f_blocks);
        printf("%-14s %4lu.%02lu%-2s %4lu.%02lu%-2s %3d%% %-2s %s\n", ppcArgV[1], 
               ulTotalDisp, ulTotalPoint, pcTotalUnit, 
               ulFreeDisp,  ulFreePoint,  pcFreeUnit, 
               iUseagePercent, pcRo, pcFsType);
    }
           
    return  (0);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdChmod
** 功能描述: 系统命令 "chmod"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdChmod (INT  iArgC, PCHAR  ppcArgV[])
{
    REGISTER size_t     stLen;
             INT        i;
             INT        iNewMode = DEFAULT_FILE_PERM;

    if (iArgC != 3) {
        fprintf(stderr, "chmod arguments error, (chmod newmode filename)\n");
        return  (-1);
    }
    
    stLen = lib_strnlen(ppcArgV[1], 5);
    if (stLen > 4) {
        fprintf(stderr, "newmode error, (eg. 777 ... octal)\n");
        return  (-1);
    }
    
    for (i = 0; i < stLen; i++) {                                       /*  必须是 8进制的              */
        if ((ppcArgV[1][i] > '7') || (ppcArgV[1][i] < '0')) {
            fprintf(stderr, "newmode error, (eg. 777 ... octal)\n");
            return  (-1);
        }
    }
    
    sscanf(ppcArgV[1], "%o", &iNewMode);
    
    if (lib_strchr(ppcArgV[2], '*') ||
        lib_strchr(ppcArgV[2], '?')) {                                  /*  包含 shell 通配符           */

#if LW_CFG_POSIX_EN > 0
        DIR             *pdir;
        struct dirent   *pdirent;
        CHAR             cName[MAX_FILENAME_LENGTH];
        size_t           stDirLen;
        PCHAR            pcTail;
        INT              iRet;
        
        pcTail = lib_strrchr(ppcArgV[2], PX_DIVIDER);
        if (pcTail) {
            stDirLen = pcTail - ppcArgV[2];
            lib_memcpy(cName, ppcArgV[2], stDirLen);
            cName[stDirLen] = PX_EOS;
            pcTail++;                                                   /*  指向文件名部分              */
        
        } else {
            stDirLen = 1;
            lib_strcpy(cName, ".");                                     /*  当前目录                    */
            pcTail = ppcArgV[2];                                        /*  指向文件名部分              */
        }
        
        pdir = opendir(cName);
        if (!pdir) {
            fprintf(stderr, "can not open dir %s error: %s\n", cName, lib_strerror(errno));
            return  (-1);
        }
        
        pdirent = readdir(pdir);
        while (pdirent) {
            iRet = fnmatch(pcTail, pdirent->d_name, FNM_PATHNAME);
            if (iRet == ERROR_NONE) {
                bnprintf(cName, MAX_FILENAME_LENGTH, stDirLen, "/%s", pdirent->d_name);
                i = chmod(cName, iNewMode);                             /*  转换文件模式                */
                if (i < 0) {
                    if (errno == EACCES) {
                        fprintf(stderr, "%s insufficient permissions.\n", 
                                pdirent->d_name);
                    } else {
                        fprintf(stderr, "%s can not change file mode, error: %s\n", 
                                pdirent->d_name, lib_strerror(errno));
                    }
                }
            }
            pdirent = readdir(pdir);
        }
        closedir(pdir);
#else
        printf("sylixos do not have fnmatch().\n");
        return  (-1);
#endif
    } else {
        i = chmod(ppcArgV[2], iNewMode);                                /*  转换文件模式                */
        if (i < 0) {
            if (errno == EACCES) {
                fprintf(stderr, "insufficient permissions.\n");
            } else {
                perror("can not change file mode error");
            }
            return  (-1);
        }
    }
    
    return  (0);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdMkfs
** 功能描述: 系统命令 "mkfs"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdMkfs (INT  iArgC, PCHAR  ppcArgV[])
{
    REGISTER INT    iError;
    
    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    printf("now format media, please wait...\n");
    iError = mkfs(ppcArgV[1]);
    if (iError) {
        if (errno == EACCES) {
            fprintf(stderr, "insufficient permissions.\n");
        } else {
            fprintf(stderr, "can not format media : error: %s\n", lib_strerror(errno));
        }
        return  (-errno);
    }
    printf("disk format ok.\n");
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdShfile
** 功能描述: 系统命令 "shfile"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdShfile (INT  iArgC, PCHAR  ppcArgV[])
{
    FILE  *fileShell;
    PCHAR  pcCmd       = LW_NULL;
    PCHAR  pcCmdBuffer = LW_NULL;                                       /*  命令缓冲                    */
    
    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    fileShell = fopen(ppcArgV[1], "r");                                 /*  打开 shell 文件             */
    if (fileShell == LW_NULL) {
        if (errno == EACCES) {
            fprintf(stderr, "insufficient permissions.\n");
        } else {
            fprintf(stderr, "can not open %s: %s\n", ppcArgV[1], lib_strerror(errno));
        }
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    pcCmdBuffer = (PCHAR)__SHEAP_ALLOC(LW_CFG_SHELL_MAX_COMMANDLEN + 1);/*  开辟命令缓冲                */
    if (pcCmdBuffer == LW_NULL) {
        fclose(fileShell);
        fprintf(stderr, "system low memory.\n");
        return  (PX_ERROR);
    }
    
    do {
        pcCmd = fgets(pcCmdBuffer, LW_CFG_SHELL_MAX_COMMANDLEN, 
                      fileShell);                                       /*  获得一条指令                */
        if (pcCmd) {
            API_TShellExec(pcCmd);                                      /*  执行指定命令                */
        }
    } while (pcCmd);
    
    __SHEAP_FREE(pcCmdBuffer);                                          /*  释放缓存                    */
    fclose(fileShell);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdMount
** 功能描述: 系统命令 "mount"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_MOUNT_EN > 0

static INT  __tshellFsCmdMount (INT  iArgC, PCHAR  ppcArgV[])
{
    PCHAR       pcType   = LW_NULL;
    PCHAR       pcDev    = LW_NULL;
    PCHAR       pcFs     = LW_NULL;
    PCHAR       pcOption = LW_NULL;

    INT         iC;
    INT         iOptInd;
    
    if (iArgC < 3) {
        fprintf(stderr, "option error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    while ((iC = getopt(iArgC, ppcArgV, "t:o:")) != EOF) {
        switch (iC) {
        
        case 't':
            pcType = optarg;
            break;
            
        case 'o':
            pcOption = optarg;
            break;
        }
    }
    
    iOptInd = optind;
    
    getopt_free();
    
    if (iOptInd > (iArgC - 2)) {
        fprintf(stderr, "option error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    pcDev = ppcArgV[iOptInd];
    pcFs  = ppcArgV[iOptInd + 1];
    
    if (API_MountEx(pcDev, pcFs, pcType, pcOption) != ERROR_NONE) {
        fprintf(stderr, "mount error, error: %s\n", lib_strerror(errno));
        return  (PX_ERROR);
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdUmount
** 功能描述: 系统命令 "umount"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdUmount (INT  iArgC, PCHAR  ppcArgV[])
{
    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    if (API_Unmount(ppcArgV[1]) == ERROR_NONE) {
        return  (ERROR_NONE);
    } else {
        fprintf(stderr, "umount error, error: %s\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdRemount
** 功能描述: 系统命令 "remount"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_OEMDISK_EN > 0

static INT  __tshellFsCmdRemount (INT  iArgC, PCHAR  ppcArgV[])
{
    INT             iBlkFd;
    PLW_OEMDISK_CB  poemd = LW_NULL;

    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    iBlkFd = open(ppcArgV[1], O_RDONLY);
    if (iBlkFd < 0) {
        fprintf(stderr, "can not open %s error: %s!\n", ppcArgV[1], lib_strerror(errno));
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (ioctl(iBlkFd, LW_BLKD_CTRL_OEMDISK, &poemd) || !poemd) {
        close(iBlkFd);
        fprintf(stderr, "can not get AUTO-Mount information!\n");
        return  (PX_ERROR);
    }
    close(iBlkFd);
    
    if (API_OemDiskRemount(poemd)) {
        fprintf(stderr, "Remount fail: %s!\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_OEMDISK_EN > 0       */
/*********************************************************************************************************
** 函数名称: __tshellFsCmdShowmount
** 功能描述: 系统命令 "showmount"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdShowmount (INT  iArgC, PCHAR  ppcArgV[])
{
#if LW_CFG_OEMDISK_EN > 0
    API_OemDiskMountShow();
    printf("\n");
#endif                                                                  /*  LW_CFG_OEMDISK_EN > 0       */

#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_YAFFS_EN > 0)
    API_YaffsDevMountShow();
    printf("\n");
#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_YAFFS_EN > 0)       */
    API_MountShow();
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MOUNT_EN > 0         */
/*********************************************************************************************************
** 函数名称: __tshellFsCmdLn
** 功能描述: 系统命令 "ln"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdLn (INT  iArgC, PCHAR  ppcArgV[])
{
    if (iArgC == 3) {
        if (ppcArgV[1][0] == '-') {
            fprintf(stderr, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        if (symlink(ppcArgV[1], ppcArgV[2]) != ERROR_NONE) {
            fprintf(stderr, "symlink error: %s\n", lib_strerror(errno));
        }
    } else if (iArgC == 4) {
        if (ppcArgV[1][0] != '-') {
            fprintf(stderr, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        if (!lib_strchr(ppcArgV[1], 's')) {
            fprintf(stderr, "must use -s to create a symbol link.\n");
            return  (-ERROR_TSHELL_EPARAM);
            
        } else {
            if (lib_strchr(ppcArgV[1], 'f')) {
                struct stat  statGet;
                if (lstat(ppcArgV[3], &statGet) == ERROR_NONE) {
                    if (S_ISLNK(statGet.st_mode)) {
                        unlink(ppcArgV[3]);
                    }
                }
            }
            if (symlink(ppcArgV[2], ppcArgV[3]) != ERROR_NONE) {
                fprintf(stderr, "symlink error: %s\n", lib_strerror(errno));
            }
        }
    } else {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdDosfslabel
** 功能描述: 系统命令 "dosfslabel"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_FATFS_EN > 0

static INT  __tshellFsCmdDosfslabel (INT  iArgC, PCHAR  ppcArgV[])
{
    INT  iFd;
    INT  iError;
    CHAR cLabel[MAX_FILENAME_LENGTH];
    
    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    if (iArgC == 2) {                                                   /*  获取卷标                    */
        iFd = open(ppcArgV[1], O_RDONLY);
    } else {
        iFd = open(ppcArgV[1], O_RDWR);                                 /*  设置卷标                    */
    }
    
    if (iFd < 0) {
        fprintf(stderr, "can not open device, error: %s\n", lib_strerror(errno));
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (iArgC == 2) {
        iError = ioctl(iFd, FIOLABELGET, cLabel);
        if (iError < ERROR_NONE) {
            fprintf(stderr, "can not get label, error: %s\n", lib_strerror(errno));
        } else {
            printf("%s\n", cLabel);
        }
    } else {
        iError = ioctl(iFd, FIOLABELSET, ppcArgV[2]);
        if (iError < ERROR_NONE) {
            fprintf(stderr, "can not set label, error: %s\n", lib_strerror(errno));
        }
    }
    
    close(iFd);
    
    return  (iError);
}

#endif                                                                  /*  LW_CFG_FATFS_EN > 0         */
/*********************************************************************************************************
** 函数名称: __tshellFsCmdFdisk
** 功能描述: 系统命令 "fdisk"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_OEMDISK_EN > 0

static INT  __tshellFsCmdFdisk (INT  iArgC, PCHAR  ppcArgV[])
{
    LW_OEMFDISK_PART  fdpInfo[4];
    UINT              uiNPart;
    size_t            stAlign;
    struct stat       statGet;
    PCHAR             pcBlkFile;
    INT               i, iPct, iTotalPct = 0;
    CHAR              cActive;
    INT               iCnt, iType;

    if (iArgC < 2) {
        fprintf(stderr, "too few arguments!\n");
        return  (-ERROR_TSHELL_EPARAM);

    } else if (iArgC == 2) {
        return  (oemFdiskShow(ppcArgV[1]));

    } else {
        if (lib_strcmp(ppcArgV[1], "-f")) {
            fprintf(stderr, "you must use '-f' to make disk partition!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }

        pcBlkFile = ppcArgV[2];
    }

    if (stat(pcBlkFile, &statGet)) {
        fprintf(stderr, "can not get block device status: %s.\n", lib_strerror(errno));
        return  (-ERROR_TSHELL_EPARAM);
    }

    if (!S_ISBLK(statGet.st_mode)) {
        fprintf(stderr, "%s is not a block device.\n", pcBlkFile);
        return  (-ERROR_TSHELL_EPARAM);
    }

    printf("block device %s total size: %llu (MB)\n", pcBlkFile, (statGet.st_size >> 20));

__input_num:
    printf("please input how many partition(s) you want to make (1 ~ 4) : ");
    fflush(stdout);
    fpurge(stdin);
    if (scanf("%d", &uiNPart) != 1) {
        goto    __input_num;
    }

    if ((uiNPart < 1) || (uiNPart > 4)) {
        printf("the number must be 1 ~ 4\n");
        goto    __input_num;
    }

__input_align:
    printf("please input how many bytes align (4K 8K ...) : ");
    fflush(stdout);
    fpurge(stdin);
    if (scanf("%zu", &stAlign) != 1) {
        goto    __input_align;
    }

    if ((stAlign < 4096) || (stAlign & (stAlign - 1))) {
        printf("the number must be 4096 at least and must the n-th power of 2\n");
        goto    __input_align;
    }

    for (i = 0; i < uiNPart; i++) {
__input_size:
        printf("please input the partition %d size percentage(%%) 0 means all left space : ", i);
        fflush(stdout);
        fpurge(stdin);
        if (scanf("%d", &iPct) != 1) {
            goto    __input_size;
        }

        if ((iPct < 0) || (iPct > 100)) {
            printf("the partition size percentage(%%) must be 1 ~ 100\n");
            goto    __input_size;
        }

        if (iPct == 0) {
            iTotalPct  = 100;
        } else {
            iTotalPct += iPct;
        }

        if (iTotalPct > 100) {
            printf("the partition size percentage seriously error (bigger than 100%%)!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }

        fdpInfo[i].FDP_ucSzPct = (UINT8)iPct;

__input_active:
        printf("is this partition active(y/n) : ");
        fflush(stdout);
        fpurge(stdin);
        do {
            cActive = (CHAR)getchar();
        } while ((cActive == '\r') || (cActive == '\n'));
        if ((cActive != 'y') &&
            (cActive != 'Y') &&
            (cActive != 'n') &&
            (cActive != 'N')) {
            printf("please use y or n\n");
            goto    __input_active;
        }

        if ((cActive == 'y') || (cActive == 'Y')) {
            fdpInfo[i].FDP_bIsActive = LW_TRUE;
        } else {
            fdpInfo[i].FDP_bIsActive = LW_FALSE;
        }

__input_type:
        printf("please input the file system type\n");
        printf("1: FAT   2: TPSFS   3: LINUX   4: RESERVED\n");
        fpurge(stdin);
        if ((scanf("%d", &iType) != 1) || ((iType < 1) || (iType > 4))) {
            printf("please use 1 2 3 or 4\n");
            goto    __input_type;
        }

        switch (iType) {

        case 1:
            fdpInfo[i].FDP_ucPartType = LW_DISK_PART_TYPE_WIN95_FAT32;
            break;

        case 2:
            fdpInfo[i].FDP_ucPartType = LW_DISK_PART_TYPE_TPS;
            break;

        case 3:
            fdpInfo[i].FDP_ucPartType = LW_DISK_PART_TYPE_NATIVE_LINUX;
            break;
        
        case 4:
            fdpInfo[i].FDP_ucPartType = LW_DISK_PART_TYPE_RESERVED;
            break;
            
        default:
            fdpInfo[i].FDP_ucPartType = LW_DISK_PART_TYPE_TPS;
            break;
        }
    }

    printf("making partition...\n");
    iCnt = oemFdisk(pcBlkFile, fdpInfo, uiNPart, stAlign);
    if (iCnt <= ERROR_NONE) {
        fprintf(stderr, "make partition error: %s\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    oemFdiskShow(pcBlkFile);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdMkGrub
** 功能描述: 系统命令 "mkgrub"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#ifdef LW_CFG_CPU_ARCH_X86

static INT  __tshellFsCmdMkGrub (INT  iArgC, PCHAR  ppcArgV[])
{
    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (oemGrub(ppcArgV[1]) < ERROR_NONE) {
        fprintf(stderr, "make grub boot program error: %s\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    printf("make grub boot program ok.\n");
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_CPU_ARCH_X86         */
#endif                                                                  /*  LW_CFG_OEMDISK_EN > 0       */
/*********************************************************************************************************
** 函数名称: __tshellFsCmdInit
** 功能描述: 初始化文件系统命令集
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __tshellFsCmdInit (VOID)
{
    API_TShellKeywordAdd("cd", __tshellFsCmdCd);
    API_TShellFormatAdd("cd", " path");
    API_TShellHelpAdd("cd", "set current working path\n");
    
    API_TShellKeywordAdd("ch", __tshellFsCmdCh);
    API_TShellFormatAdd("ch", " dir");
    API_TShellHelpAdd("ch", "change dir\n");
    
    API_TShellKeywordAdd("pwd", __tshellFsCmdPwd);
    API_TShellFormatAdd("pwd", " ");
    API_TShellHelpAdd("pwd", "print working directory\n");
    
    API_TShellKeywordAdd("df", __tshellFsCmdDf);
    API_TShellFormatAdd("df", " volume name");
    API_TShellHelpAdd("df", "display volume useage information\n");
    
    API_TShellKeywordAdd("tmpname", __tshellFsCmdTmpname);
    API_TShellHelpAdd("tmpname", "calculate a temp file name.\n");
    
    API_TShellKeywordAdd("mkdir", __tshellFsCmdMkdir);
    API_TShellFormatAdd("mkdir", " directory");
    API_TShellHelpAdd("mkdir", "make a new directory\n");
    
    API_TShellKeywordAdd("mkfifo", __tshellFsCmdMkfifo);
    API_TShellFormatAdd("mkfifo", " [fifo name]");
    API_TShellHelpAdd("mkfifo", "make a new fifo\n");
    
    API_TShellKeywordAdd("rmdir", __tshellFsCmdRmdir);
    API_TShellFormatAdd("rmdir", " directory");
    API_TShellHelpAdd("rmdir", "remove a directory\n");
    
    API_TShellKeywordAdd("rm", __tshellFsCmdRm);
    API_TShellFormatAdd("rm", " file name");
    API_TShellHelpAdd("rm", "remove a file\n");
    
    API_TShellKeywordAdd("mv", __tshellFsCmdMv);
    API_TShellFormatAdd("mv", " SRC file name, DST file name");
    API_TShellHelpAdd("mv", "move a file\n");
    
    API_TShellKeywordAdd("cat", __tshellFsCmdCat);
    API_TShellFormatAdd("cat", " file name");
    API_TShellHelpAdd("cat", "display file\n");
    
    API_TShellKeywordAdd("cp", __tshellFsCmdCp);
    API_TShellFormatAdd("cp", " src file name dst file name");
    API_TShellHelpAdd("cp", "copy file\n");
    
    API_TShellKeywordAdd("cmp", __tshellFsCmdCmp);
    API_TShellFormatAdd("cmp", " [file one] [file two]");
    API_TShellHelpAdd("cmp", "compare two file\n");
    
    API_TShellKeywordAdd("touch", __tshellFsCmdTouch);
    API_TShellFormatAdd("touch", " [-amc] file name");
    API_TShellHelpAdd("touch", "touch a file\n");
    
    API_TShellKeywordAdd("ls", __tshellFsCmdLs);
    API_TShellFormatAdd("ls", " [path name]");
    API_TShellHelpAdd("ls", "list file(s)\n");
    
    API_TShellKeywordAdd("ll", __tshellFsCmdLl);
    API_TShellFormatAdd("ll", " [path name]");
    API_TShellHelpAdd("ll", "get file(s) attrib\n");
    
    API_TShellKeywordAdd("dsize", __tshellFsCmdDsize);
    API_TShellFormatAdd("dsize", " [path name]");
    API_TShellHelpAdd("dsize", "get directory size\n");
    
    API_TShellKeywordAdd("chmod", __tshellFsCmdChmod);
    API_TShellFormatAdd("chmod", " newmode filename");
    API_TShellHelpAdd("chmod", "change file attrib\n"
                               "eg. chmod 777 file\n");
    
    API_TShellKeywordAdd("mkfs", __tshellFsCmdMkfs);
    API_TShellFormatAdd("mkfs", " media name");
    API_TShellHelpAdd("mkfs", "make a fs format in a disk(format disk).\n");
    
    API_TShellKeywordAdd("shfile", __tshellFsCmdShfile);
    API_TShellFormatAdd("shfile", " shell file");
    API_TShellHelpAdd("shfile", "execute a shell file.\n");
    
#if LW_CFG_MOUNT_EN > 0
    API_TShellKeywordAdd("mount", __tshellFsCmdMount);
    API_TShellFormatAdd("mount", " [-t fstype] [-o option] [blk dev] [mount path]");
    API_TShellHelpAdd("mount",  "mount a volume.\n"
                                "eg. mount /dev/blk/sata0 /mnt/hdd0\n"
                                "    mount -t vfat /dev/blk/sata0 /mnt/hdd0\n"
                                "    mount -t tpsfs /dev/blk/sata0 /mnt/hdd0\n"
                                "    mount -t romfs /dev/blk/rom0 /mnt/rom0\n"
                                "    mount -t romfs /root/romfile /mnt/rom1\n"
                                "    mount -t ramfs 100000 /mnt/ram\n"
                                "    mount -t nfs -o ro 192.168.0.123:/nfstest /mnt/nfs\n"
                                "-o \n"
                                "ro (read-only file system)\n"
                                "rw (read-write file system)\n");
                                
    API_TShellKeywordAdd("umount", __tshellFsCmdUmount);
    API_TShellFormatAdd("umount", " [mount path]");
    API_TShellHelpAdd("umount",  "unmount a volume.\n"
                                 "eg. mount /mnt/usb\n");
                                 
#if LW_CFG_OEMDISK_EN > 0
    API_TShellKeywordAdd("remount", __tshellFsCmdRemount);
    API_TShellFormatAdd("remount", " [/dev/blk/*]");
    API_TShellHelpAdd("remount",  "auto remount a block device.\n"
                                 "eg. remount /dev/blk/hdd-0\n");
#endif                                                                  /*  LW_CFG_OEMDISK_EN > 0       */

    API_TShellKeywordAdd("showmount", __tshellFsCmdShowmount);
    API_TShellHelpAdd("showmount",  "show all mount point.\n");
#endif                                                                  /*  LW_CFG_MOUNT_EN > 0         */

    API_TShellKeywordAdd("ln", __tshellFsCmdLn);
    API_TShellFormatAdd("ln", " [-s | -f] [actualpath] [sympath]");
    API_TShellHelpAdd("ln",   "create a symbol link file (must use -s).\n"
                              "eg. ln -s /tmp/dir /dir\n");

#if LW_CFG_FATFS_EN > 0
    API_TShellKeywordAdd("dosfslabel", __tshellFsCmdDosfslabel);
    API_TShellFormatAdd("dosfslabel", " [[vol newlabel] [vol]]");
    API_TShellHelpAdd("dosfslabel",   "get or set volumn label.\n"
                                      "eg. dosfslabel /usb/ms0\n"
                                      "    dosfslabel /usb/ms1 newlabel\n");
#endif                                                                  /*  LW_CFG_FATFS_EN > 0         */

#if LW_CFG_OEMDISK_EN > 0
    API_TShellKeywordAdd("fdisk", __tshellFsCmdFdisk);
    API_TShellFormatAdd("fdisk", " [-f] [block I/O device]");
    API_TShellHelpAdd("fdisk",   "show or make disk partition table\n"
                                 "eg. fdisk /dev/blk/udisk0\n"
                                 "    fdisk -f /dev/blk/sata0\n");

#ifdef LW_CFG_CPU_ARCH_X86
    API_TShellKeywordAdd("mkgrub", __tshellFsCmdMkGrub);
    API_TShellFormatAdd("mkgrub", " [block I/O device]");
    API_TShellHelpAdd("mkgrub",   "make disk grub boot program\n");
#endif                                                                  /*  LW_CFG_CPU_ARCH_X86         */
#endif                                                                  /*  LW_CFG_OEMDISK_EN > 0       */
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
                                                                        /*  LW_CFG_MAX_VOLUMES > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
