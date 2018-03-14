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
** 文   件   名: ttinyShellTarCmd.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 02 月 26 日
**
** 描        述: 系统 tar 命令.
**
** BUG:
2014.07.23  加入统计信息打印.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_SHELL_EN > 0) && (LW_CFG_SHELL_TAR_EN > 0)
#include <stddef.h>
#include "stdio.h"
#include "tar.h"
#ifndef  __SYLIXOS_LITE
#include "zlib.h"
#endif                                                                  /*  __SYLIXOS_LITE              */
/*********************************************************************************************************
** 函数名称: __untarOctal2Long
** 功能描述: ascii (octal) to long
** 输　入  : octascii      ascii (octal)
**           len           strlen
** 输　出  : long number
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static unsigned long __untarOctal2Long (const char *octascii, size_t len)
{
    size_t        i;
    unsigned long num;

    num = 0;

    for (i = 0; i < len; i++) {
        if ((octascii[i] < '0') || (octascii[i] > '9')) {
            continue;
        }
        num  = num * 8 + ((unsigned long)(octascii[i] - '0'));
    }

    return   (num);
}
/*********************************************************************************************************
** 函数名称: __untarHeaderChksum
** 功能描述: calculate header chksum
** 输　入  : buf           header
** 输　出  : chksum
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static int __untarHeaderChksum (const char *buf)
{
    int  i, sum;

    sum = 0;
    for (i = 0; i < 512; i++) {
        if ((i >= 148) && (i < 156)) {
            sum += 0xff & ' ';
        } else {
            sum += 0xff & buf[i];
        }
    }

    return   (sum);
}
/*********************************************************************************************************
** 函数名称: __untarFile
** 功能描述: unpackage a .tar file.
** 输　入  : pcTarFile     tar file
**           pcDestPath    destination directory
** 输　出  : 0 or -1
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __untarFile (CPCHAR  pcTarFile, CPCHAR  pcDestPath)
{
    INT     iFdTar;
    
    char    *pcBuf;
    ssize_t  sstN;
    char     cFname[100];
    char     cLinkname[100];
    int            iSum;
    int            iHdrChksum;
    int            iRetVal;
    unsigned long  ulI;
    unsigned long  ulNblocks;
    unsigned long  ulSize;
    unsigned char  ucLinkflag;
    
    ULONG          ulTotalFile = 0ul;
    ULONG          ulTotalDir  = 0ul;
    
    iFdTar = open(pcTarFile, O_RDONLY);
    if (iFdTar < 0) {
        fprintf(stderr, "can not open: %s : %s\n", pcTarFile, lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    pcBuf = (char *)__SHEAP_ALLOC(512);
    if (pcBuf == LW_NULL) {
        close(iFdTar);
        fprintf(stderr, "system low memory.\n");
        return  (PX_ERROR);
    }
    
    iRetVal = ERROR_NONE;
    
    while (1) {
        char    cOutFile[PATH_MAX + 1];
        mode_t  mode;
        
        if ((sstN = read(iFdTar, pcBuf, 512)) != 512) {
            break;
        }
        
        if (lib_strncmp(&pcBuf[257], TMAGIC, 5)) {
            break;
        }
        
        lib_strlcpy(cFname, pcBuf, 100);

        ucLinkflag = pcBuf[156];
        ulSize     = __untarOctal2Long(&pcBuf[124], 12);
        
        iHdrChksum = (int)__untarOctal2Long(&pcBuf[148], 8);
        iSum       = __untarHeaderChksum(pcBuf);
        
        if (iSum != iHdrChksum) {
            fprintf(stderr, "tar file: %s chksum error.\n", pcTarFile);
            iRetVal = PX_ERROR;
            break;
        }
        
        mode = (int)__untarOctal2Long(&pcBuf[100], 8);
        
        if (pcDestPath) {
            if (lib_strcmp(pcDestPath, PX_STR_ROOT) == 0) {
                if (cFname[0] == PX_ROOT) {
                    lib_strlcpy(cOutFile, cFname, PATH_MAX + 1);
                } else {
                    snprintf(cOutFile, PATH_MAX + 1, "%s%s", pcDestPath, cFname);
                }
            } else {
                snprintf(cOutFile, PATH_MAX + 1, "%s/%s", pcDestPath, cFname);
            }
        } else {
            lib_strlcpy(cOutFile, cFname, PATH_MAX + 1);
        }
        
        if (ucLinkflag == SYMTYPE) {
            printf("unpackage %s <LNK> ...\n", cOutFile);
            lib_strlcpy(cLinkname, &pcBuf[157], 100);
            symlink(cLinkname, cOutFile);
            ulTotalFile++;
            
        } else if (ucLinkflag == REGTYPE) {
            INT     iFdOut;
            
            printf("unpackage %s size: %ld ...\n", cOutFile, ulSize);
            ulNblocks = (((ulSize) + 511) & ~511) / 512;
            if ((iFdOut = creat(cOutFile, mode)) < 0) {
                fprintf(stderr, "can not create: %s\n", cOutFile);
                lseek(iFdTar, (off_t)(ulNblocks * 512), SEEK_CUR);
                
            } else {
                for (ulI = 0; ulI < ulNblocks; ulI++) {
                    sstN = read(iFdTar, pcBuf, 512);
                    sstN = min(sstN, ulSize - (ulI * 512));
                    write(iFdOut, pcBuf, (size_t)sstN);
                }
                close(iFdOut);
                ulTotalFile++;
            }
            
        } else if (ucLinkflag == DIRTYPE) {
            printf("unpackage %s <DIR> ...\n", cOutFile);
            mkdir(cOutFile, mode);
            ulTotalDir++;
        }
    }
    
    __SHEAP_FREE(pcBuf);
    close(iFdTar);
    
    printf("unpackage total %lu files %lu directory.\n", ulTotalFile, ulTotalDir);

    return  (iRetVal);
}
/*********************************************************************************************************
** 函数名称: __untargzFile
** 功能描述: unpackage a .tar.gz or .tgz file.
** 输　入  : pcTarFile     tar.gz file
**           pcDestPath    destination directory
** 输　出  : 0 or -1
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#ifndef  __SYLIXOS_LITE

static INT  __untargzFile (CPCHAR  pcTargzFile, CPCHAR  pcDestPath)
{
    INT     iFdTar;
    gzFile  gzTar;
    
    char    *pcBuf;
    ssize_t  sstN;
    char     cFname[100];
    char     cLinkname[100];
    int            iSum;
    int            iHdrChksum;
    int            iRetVal;
    unsigned long  ulI;
    unsigned long  ulNblocks;
    unsigned long  ulSize;
    unsigned char  ucLinkflag;
    
    ULONG          ulTotalFile = 0ul;
    ULONG          ulTotalDir  = 0ul;
    
    iFdTar = open(pcTargzFile, O_RDONLY);
    if (iFdTar < 0) {
        fprintf(stderr, "can not open: %s : %s\n", pcTargzFile, lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    pcBuf = (char *)__SHEAP_ALLOC(512);
    if (pcBuf == LW_NULL) {
        close(iFdTar);
        fprintf(stderr, "system low memory.\n");
        return  (PX_ERROR);
    }
    
    gzTar = gzdopen(iFdTar, "rb");
    if (!gzTar) {
        __SHEAP_FREE(pcBuf);
        close(iFdTar);
        fprintf(stderr, "zlib can not open: %s\n", pcTargzFile);
        return  (PX_ERROR);
    }
    
    iRetVal = ERROR_NONE;
    
    while (1) {
        char    cOutFile[PATH_MAX + 1];
        mode_t  mode;
        
        if ((sstN = gzread(gzTar, pcBuf, 512)) != 512) {
            break;
        }
        
        if (lib_strncmp(&pcBuf[257], TMAGIC, 5)) {
            break;
        }
        
        lib_strlcpy(cFname, pcBuf, 100);

        ucLinkflag = pcBuf[156];
        ulSize     = __untarOctal2Long(&pcBuf[124], 12);
        
        iHdrChksum = (int)__untarOctal2Long(&pcBuf[148], 8);
        iSum       = __untarHeaderChksum(pcBuf);
        
        if (iSum != iHdrChksum) {
            fprintf(stderr, "tar file: %s chksum error.\n", pcTargzFile);
            iRetVal = PX_ERROR;
            break;
        }
        
        mode = (int)__untarOctal2Long(&pcBuf[100], 8);
        
        if (pcDestPath) {
            if (lib_strcmp(pcDestPath, PX_STR_ROOT) == 0) {
                if (cFname[0] == PX_ROOT) {
                    lib_strlcpy(cOutFile, cFname, PATH_MAX + 1);
                } else {
                    snprintf(cOutFile, PATH_MAX + 1, "%s%s", pcDestPath, cFname);
                }
            } else {
                snprintf(cOutFile, PATH_MAX + 1, "%s/%s", pcDestPath, cFname);
            }
        } else {
            lib_strlcpy(cOutFile, cFname, PATH_MAX + 1);
        }
        
        if (ucLinkflag == SYMTYPE) {
            printf("unpackage %s <LNK> ...\n", cOutFile);
            lib_strlcpy(cLinkname, &pcBuf[157], 100);
            symlink(cLinkname, cOutFile);
            ulTotalFile++;
            
        } else if (ucLinkflag == REGTYPE) {
            INT     iFdOut;
            
            printf("unpackage %s size: %ld ...\n", cOutFile, ulSize);
            ulNblocks = (((ulSize) + 511) & ~511) / 512;
            if ((iFdOut = creat(cOutFile, mode)) < 0) {
                fprintf(stderr, "can not create: %s\n", cOutFile);
                gzseek(gzTar, (ulNblocks * 512), SEEK_CUR);
                
            } else {
                for (ulI = 0; ulI < ulNblocks; ulI++) {
                    sstN = gzread(gzTar, pcBuf, 512);
                    sstN = min(sstN, ulSize - (ulI * 512));
                    write(iFdOut, pcBuf, (size_t)sstN);
                }
                close(iFdOut);
                ulTotalFile++;
            }
            
        } else if (ucLinkflag == DIRTYPE) {
            printf("unpackage %s <DIR> ...\n", cOutFile);
            mkdir(cOutFile, mode);
            ulTotalDir++;
        }
    }
    
    __SHEAP_FREE(pcBuf);
    gzclose(gzTar);
    
    printf("unpackage total %lu files %lu directory.\n", ulTotalFile, ulTotalDir);

    return  (iRetVal);
}

#endif                                                                  /*  __SYLIXOS_LITE              */
/*********************************************************************************************************
** 函数名称: __tshellFsCmdUntar
** 功能描述: 系统命令 "untar"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdUntar (INT  iArgC, PCHAR  ppcArgV[])
{
    INT         iError;
    PCHAR       pcDest = LW_NULL;

    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (iArgC == 3) {
        pcDest = ppcArgV[2];
    }
    
    if (lib_strstr(ppcArgV[1], ".tar.gz") ||
        lib_strstr(ppcArgV[1], ".tgz")) {
#ifndef  __SYLIXOS_LITE
        iError = __untargzFile(ppcArgV[1], pcDest);
#else
        fprintf(stderr, "can not support untar zip file!\n");
        return  (PX_ERROR);
#endif                                                                  /*  __SYLIXOS_LITE              */
    
    } else {
        iError = __untarFile(ppcArgV[1], pcDest);
    }
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __tshellTarCmdInit
** 功能描述: 初始化 tar 命令集
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __tshellTarCmdInit (VOID)
{
    API_TShellKeywordAdd("untar", __tshellFsCmdUntar);
    API_TShellFormatAdd("untar", " [.tar or .tar.gz file] [destination directory]");
    API_TShellHelpAdd("untar", "extract a .tar or .tar.gz file.\n"
                               "eg. untar example.tar /\n"
                               "    untar example.tar.gz\n"
                               "    untar tools.tar.gz /bin\n");
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
                                                                        /*  LW_CFG_SHELL_TAR_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
