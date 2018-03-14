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
** 文   件   名: rootFsMap.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 09 月 14 日
**
** 描        述: 根目录文件系统映射.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
  映射结构
*********************************************************************************************************/
#define LW_ROOTFS_ARRAY_SIZE(x)     (sizeof(x) / sizeof((x)[0]))
#define LW_ROOTFS_MAP_LEN           32

typedef struct {
    PCHAR   RFSMN_pcDir;
    CHAR    RFSMN_cMapDir[LW_ROOTFS_MAP_LEN];
} LW_ROOTFS_MAP_NODE;

static LW_ROOTFS_MAP_NODE   _G_rfsmapRoot   = { "/" , "/media/hdd1" };
static LW_ROOTFS_MAP_NODE   _G_rfsmapSubp[] = {
    { "/var" , "" },
    { "/usr" , "" },
    { "/tmp" , "" },
    { "/sbin", "" },
    { "/root", "" },
    { "/qt"  , "" },
    { "/lib" , "" },
    { "/home", "" },
    { "/etc" , "" },
    { "/boot", "/media/hdd0" },
    { "/bin" , "" },
    { "/apps", "" }
};
/*********************************************************************************************************
** 函数名称: API_RootFsMapInit
** 功能描述: 根文件系统映射初始化
** 输　入  : pcMap         映射关系
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_RootFsMapInit (CPCHAR  pcMap)
{
    static BOOL     bInit = LW_FALSE;
    INT             i;
    CHAR            cMap[MAX_FILENAME_LENGTH];
    PCHAR           pcDir, pcMdir, pcNext;
    
    if (bInit) {
        return  (PX_ERROR);
    }
    bInit = LW_TRUE;
    
    lib_strlcpy(cMap, pcMap, MAX_FILENAME_LENGTH);
    
    pcDir  = cMap;
    pcNext = cMap;
    
    while (*pcNext) {
        if (*pcNext == ',') {
            *pcNext =  PX_EOS;
            pcNext++;
__mapsave:
            if (pcMdir) {
                if (lib_strcmp(pcDir, _G_rfsmapRoot.RFSMN_pcDir) == 0) {
                    lib_strlcpy(_G_rfsmapRoot.RFSMN_cMapDir, pcMdir, LW_ROOTFS_MAP_LEN);
                    
                } else {
                    for (i = 0; i < LW_ROOTFS_ARRAY_SIZE(_G_rfsmapSubp); i++) {
                        if (lib_strcmp(pcDir, _G_rfsmapSubp[i].RFSMN_pcDir) == 0) {
                            lib_strlcpy(_G_rfsmapSubp[i].RFSMN_cMapDir, pcMdir, LW_ROOTFS_MAP_LEN);
                        }
                    }
                }
                pcMdir = LW_NULL;
                pcDir  = pcNext;
            }
            
        } else if (*pcNext == ':') {
            *pcNext =  PX_EOS;
            pcNext++;
            pcMdir = pcNext;
        
        } else {
            pcNext++;
            if (*pcNext == PX_EOS) {
                goto    __mapsave;
            }
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_RootFsMap
** 功能描述: 根文件系统映射
** 输　入  : pdevhdr                       设备头
**           pcName                        链接原始文件名
**           pcLinkDst                     链接目标文件名
**           stMaxSize                     缓冲大小
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_RootFsMap (ULONG  ulFlags)
{
    static BOOL     bInit = LW_FALSE;
    CHAR            cMap[MAX_FILENAME_LENGTH];
    LW_BLK_DEV      blkdevRam;
    INT             i;
    
    if (bInit) {
        return  (PX_ERROR);
    }
    bInit = LW_TRUE;
    
    if (lib_strcmp(_G_rfsmapRoot.RFSMN_cMapDir, "/dev/ram") == 0) {
        lib_bzero(&blkdevRam, sizeof(LW_BLK_DEV));
        blkdevRam.BLKD_pcName = "0";
        ramFsDevCreate("/dev/ram", &blkdevRam);
    }
    
    for (i = 0; i < LW_ROOTFS_ARRAY_SIZE(_G_rfsmapSubp); i++) {
        if (_G_rfsmapSubp[i].RFSMN_cMapDir[0]) {
            if (access(_G_rfsmapSubp[i].RFSMN_cMapDir, R_OK) < 0) {
                mkdir(_G_rfsmapSubp[i].RFSMN_cMapDir, DEFAULT_DIR_PERM);
            }
            symlink(_G_rfsmapSubp[i].RFSMN_cMapDir, _G_rfsmapSubp[i].RFSMN_pcDir);
        
        } else {
            snprintf(cMap, MAX_FILENAME_LENGTH, "%s%s", 
                     _G_rfsmapRoot.RFSMN_cMapDir, _G_rfsmapSubp[i].RFSMN_pcDir);
            if (access(cMap, R_OK) < 0) {
                mkdir(cMap, DEFAULT_DIR_PERM);
            }
            symlink(cMap, _G_rfsmapSubp[i].RFSMN_pcDir);
        }
    }
    
    if (access("/lib/modules", R_OK) < 0) {
        mkdir("/lib/modules", DEFAULT_DIR_PERM);
        if (access("/lib/modules/drivers", R_OK) < 0) {
            mkdir("/lib/modules/drivers", DEFAULT_DIR_PERM);
        }
    }
    
    if (access("/usr/lib", R_OK) < 0) {
        mkdir("/usr/lib", DEFAULT_DIR_PERM);
    }
    
    if (access("/var/tmp", R_OK) < 0) {
        symlink("/tmp", "/var/tmp");
    }
    
    if (access("/var/log", R_OK) < 0) {
        mkdir("/var/log", DEFAULT_DIR_PERM);
#if LW_CFG_CDUMP_EN > 0
        if (access("/var/log/cdump", R_OK) < 0) {
            mkdir("/var/log/cdump", DEFAULT_DIR_PERM);
        }
#endif
    }
    
    if (ulFlags & LW_ROOTFS_MAP_LOAD_VAR) {
        lib_system("varload");
    }
    
    if (ulFlags & LW_ROOTFS_MAP_SYNC_TZ) {
        lib_tzset();
    }
    
    if (ulFlags & LW_ROOTFS_MAP_SET_TIME) {
        rtcToRoot();
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
