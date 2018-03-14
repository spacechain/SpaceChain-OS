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
** 文   件   名: mount.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 09 月 26 日
**
** 描        述: mount 挂载库, 这是 SylixOS 第二类块设备挂载方式.
                 详细见 blockIo.h (不推荐使用! 可能存在问题)
** BUG:
2012.03.10  加入对 NFS 的支持.
2012.03.12  API_Mount() 节点记录绝对路径, 删除节点也使用绝对路径.
2012.03.21  加入 API_MountShow() 函数.
2012.04.11  加入独立的 mount 锁, 当设备卸载失败时, umount 失败.
2012.06.27  第二类块设备支持直接读写数据接口.
            当物理设备不是块设备时, 可以使用 __LW_MOUNT_DEFAULT_SECSIZE 进行操作, 这样可以直接无缝挂载
            文件系统镜像文件, 例如: romfs 文件.
2012.08.16  使用 pread 与 pwrite 代替 lseek->read/write 操作.
2012.09.01  支持卸载非 mount 设备.
2012.12.07  将默认文件系统设置为 vfat.
2012.12.08  非 block 文件磁盘需要过滤 ioctl 命令.
2012.12.25  mount 和 umount 必须在内核空间执行, 以保证创建出来的文件描述符为内核文件描述符, 所遇进程在内核
            空间时均可以访问.
2013.04.02  加入 sys/mount.h 支持.
2013.06.25  logic 设备 BLKD_pvLink 不能为 NULL.
2014.05.24  加入对 ramfs 支持.
2015.08.26  将 mount 修正为 blk raw io 方式操作磁盘.
2016.06.13  修正 nfs 只读属性错误.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_BLKRAW_EN > 0) && (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_MOUNT_EN > 0)
#include "sys/mount.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define __LW_MOUNT_DEFAULT_FS       "vfat"                              /*  默认挂载文件系统格式        */
#define __LW_MOUNT_NFS_FS           "nfs"                               /*  nfs 挂载                    */
#define __LW_MOUNT_RAM_FS           "ramfs"                             /*  ram 挂载                    */
/*********************************************************************************************************
  挂载节点
*********************************************************************************************************/
typedef struct {
    LW_BLK_RAW              MN_blkraw;                                  /*  BLOCK RAW 设备              */
#define MN_blkd             MN_blkraw.BLKRAW_blkd
#define MN_iFd              MN_blkraw.BLKRAW_iFd

    BOOL                    MN_bNeedDelete;                             /*  是否需要删除 BLOCK RAW      */
    LW_LIST_LINE            MN_lineManage;                              /*  管理链表                    */
    CHAR                    MN_cVolName[1];                             /*  加载卷的名字                */
} LW_MOUNT_NODE;
typedef LW_MOUNT_NODE      *PLW_MOUNT_NODE;
/*********************************************************************************************************
  挂载点
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineMountDevHeader = LW_NULL;           /*  没有必要使用 hash 查询      */
static LW_OBJECT_HANDLE     _G_ulMountLock         = 0ul;

#define __LW_MOUNT_LOCK()   API_SemaphoreMPend(_G_ulMountLock, LW_OPTION_WAIT_INFINITE)
#define __LW_MOUNT_UNLOCK() API_SemaphoreMPost(_G_ulMountLock)
/*********************************************************************************************************
** 函数名称: __mount
** 功能描述: 挂载一个分区(内部函数)
** 输　入  : pcDevName         块设备名   例如: /dev/sda1
**           pcVolName         挂载目标   例如: /mnt/usb (不能使用相对路径, 否则无法卸载)
**           pcFileSystem      文件系统格式 "vfat" "iso9660" "ntfs" "nfs" "romfs" "ramfs" ... 
                               NULL 表示使用默认文件系统
**           pcOption          选项, 当前支持 ro 或者 rw
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __mount (CPCHAR  pcDevName, CPCHAR  pcVolName, CPCHAR  pcFileSystem, CPCHAR  pcOption)
{
#define __LW_MOUNT_OPT_RO   "ro"
#define __LW_MOUNT_OPT_RW   "rw"

    REGISTER PCHAR      pcFs;
    PLW_MOUNT_NODE      pmnDev;
             FUNCPTR    pfuncFsCreate;
             BOOL       bRdOnly = LW_FALSE;
             BOOL       bNeedDelete;
             CHAR       cVolNameBuffer[MAX_FILENAME_LENGTH];
             size_t     stLen;

    if (!pcDevName || !pcVolName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pcOption) {                                                     /*  文件系统挂载选项            */
        if (lib_strcasecmp(__LW_MOUNT_OPT_RO, pcOption) == 0) {
            bRdOnly = LW_TRUE;
        
        } else if (lib_strcasecmp(__LW_MOUNT_OPT_RW, pcOption) == 0) {
            bRdOnly = LW_FALSE;
        }
    }
    
    pcFs = (!pcFileSystem) ? __LW_MOUNT_DEFAULT_FS : (PCHAR)pcFileSystem;
    pfuncFsCreate = __fsCreateFuncGet(pcFs, LW_NULL, 0);                /*  文件系统创建函数            */
    if (pfuncFsCreate == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DRIVER);                               /*  没有文件系统驱动            */
        return  (PX_ERROR);
    }
    
    if ((lib_strcmp(pcFs, __LW_MOUNT_NFS_FS) == 0) ||
        (lib_strcmp(pcFs, __LW_MOUNT_RAM_FS) == 0)) {                   /*  NFS 或者 RAM FS             */
        bNeedDelete = LW_FALSE;                                         /*  不需要操作 BLK RAW 设备     */

    } else {
        bNeedDelete = LW_TRUE;
    }
    
    _PathGetFull(cVolNameBuffer, MAX_FILENAME_LENGTH, pcVolName);
    pcVolName = cVolNameBuffer;                                         /*  使用绝对路径                */
    
    stLen  = lib_strlen(pcVolName);
    pmnDev = (PLW_MOUNT_NODE)__SHEAP_ALLOC(sizeof(LW_MOUNT_NODE) + stLen);
    if (pmnDev == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pmnDev, sizeof(LW_MOUNT_NODE));
    lib_strcpy(pmnDev->MN_cVolName, pcVolName);                         /*  保存卷挂载名                */
    pmnDev->MN_bNeedDelete = bNeedDelete;
    
    if (bNeedDelete) {
        if (API_BlkRawCreate(pcDevName, bRdOnly, 
                             LW_TRUE, &pmnDev->MN_blkraw) < ERROR_NONE) {
            __SHEAP_FREE(pmnDev);
            return  (PX_ERROR);
        }
    
    } else {
        pmnDev->MN_blkd.BLKD_pcName = (PCHAR)__SHEAP_ALLOC(lib_strlen(pcDevName) + 1);
        if (pmnDev->MN_blkd.BLKD_pcName == LW_NULL) {
            __SHEAP_FREE(pmnDev);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        lib_strcpy(pmnDev->MN_blkd.BLKD_pcName, pcDevName);             /*  记录设备名 (nfs ram 使用)   */
        
        pmnDev->MN_blkd.BLKD_iFlag = (bRdOnly) ? O_RDONLY : O_RDWR;
    }
    
    if (pfuncFsCreate(pcVolName, &pmnDev->MN_blkd) < 0) {               /*  挂载文件系统                */
        if (bNeedDelete) {
            API_BlkRawDelete(&pmnDev->MN_blkraw);
        
        } else {
            __SHEAP_FREE(pmnDev->MN_blkd.BLKD_pcName);
        }
        
        __SHEAP_FREE(pmnDev);                                           /*  释放控制块                  */
        return  (PX_ERROR);
    }
    
    __LW_MOUNT_LOCK();
    _List_Line_Add_Ahead(&pmnDev->MN_lineManage,
                         &_G_plineMountDevHeader);                      /*  挂入链表                    */
    __LW_MOUNT_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __unmount
** 功能描述: 卸载一个分区(内部函数)
** 输　入  : pcVolName         挂载目标   例如: /mnt/usb
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __unmount (CPCHAR  pcVolName)
{
    INT             iError;
    PLW_MOUNT_NODE  pmnDev;
    PLW_LIST_LINE   plineTemp;
    CHAR            cVolNameBuffer[MAX_FILENAME_LENGTH];
    
    _PathGetFull(cVolNameBuffer, MAX_FILENAME_LENGTH, pcVolName);
    
    pcVolName = cVolNameBuffer;                                         /*  使用绝对路径                */
    
    __LW_MOUNT_LOCK();
    for (plineTemp  = _G_plineMountDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pmnDev = _LIST_ENTRY(plineTemp, LW_MOUNT_NODE, MN_lineManage);
        if (lib_strcmp(pmnDev->MN_cVolName, pcVolName) == 0) {
            break;
        }
    }
    
    if (plineTemp == LW_NULL) {                                         /*  没有找到                    */
        INT iError = PX_ERROR;
        
        if (API_IosDevMatchFull(pcVolName)) {                           /*  如果是设备, 这里就卸载设备  */
            iError = unlink(pcVolName);
            __LW_MOUNT_UNLOCK();
            return  (iError);
        }
        
        __LW_MOUNT_UNLOCK();
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    
    } else {
        iError = unlink(pmnDev->MN_cVolName);                           /*  卸载卷                      */
        if (iError < 0) {
            if (errno != ENOENT) {                                      /*  如果不是被卸载过了          */
                __LW_MOUNT_UNLOCK();
                return  (PX_ERROR);                                     /*  卸载失败                    */
            }
        }
        
        if (pmnDev->MN_bNeedDelete) {
            API_BlkRawDelete(&pmnDev->MN_blkraw);
        
        } else {
            __SHEAP_FREE(pmnDev->MN_blkd.BLKD_pcName);
        }

        _List_Line_Del(&pmnDev->MN_lineManage,
                       &_G_plineMountDevHeader);                        /*  退出挂载链表                */
    }
    __LW_MOUNT_UNLOCK();
    
    __SHEAP_FREE(pmnDev);                                               /*  释放控制块                  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_MountInit
** 功能描述: 初始化 mount 库.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID  API_MountInit (VOID)
{
    if (_G_ulMountLock == 0) {
        _G_ulMountLock =  API_SemaphoreMCreate("mount_lock", LW_PRIO_DEF_CEILING, 
                            LW_OPTION_WAIT_PRIORITY | LW_OPTION_INHERIT_PRIORITY |
                            LW_OPTION_DELETE_SAFE | LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: API_MountEx
** 功能描述: 挂载一个分区
** 输　入  : pcDevName         块设备名   例如: /dev/sda1
**           pcVolName         挂载目标   例如: /mnt/usb (不能使用相对路径, 否则无法卸载)
**           pcFileSystem      文件系统格式 "vfat" "iso9660" "ntfs" "nfs" "romfs" "ramfs" ... 
                               NULL 表示使用默认文件系统
**           pcOption          选项, 当前支持 ro 或者 rw
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_MountEx (CPCHAR  pcDevName, CPCHAR  pcVolName, CPCHAR  pcFileSystem, CPCHAR  pcOption)
{
    INT     iRet;
    
    __KERNEL_SPACE_ENTER();
    iRet = __mount(pcDevName, pcVolName, pcFileSystem, pcOption);
    __KERNEL_SPACE_EXIT();
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_Mount
** 功能描述: 挂载一个分区
** 输　入  : pcDevName         块设备名   例如: /dev/sda1
**           pcVolName         挂载目标   例如: /mnt/usb (不能使用相对路径, 否则无法卸载)
**           pcFileSystem      文件系统格式 "vfat" "iso9660" "ntfs" "nfs" "romfs" "ramfs" .. 
                               NULL 表示使用默认文件系统
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_Mount (CPCHAR  pcDevName, CPCHAR  pcVolName, CPCHAR  pcFileSystem)
{
    INT     iRet;
    
    __KERNEL_SPACE_ENTER();
    iRet = __mount(pcDevName, pcVolName, pcFileSystem, LW_NULL);
    __KERNEL_SPACE_EXIT();
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_Unmount
** 功能描述: 卸载一个分区
** 输　入  : pcVolName         挂载目标   例如: /mnt/usb
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_Unmount (CPCHAR  pcVolName)
{
    INT     iRet;
    
    __KERNEL_SPACE_ENTER();
    iRet = __unmount(pcVolName);
    __KERNEL_SPACE_EXIT();
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_MountShow
** 功能描述: 显示当前挂载的信息
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID  API_MountShow (VOID)
{
    PCHAR           pcMountInfoHdr = "       VOLUME                    BLK NAME\n"
                                     "-------------------- --------------------------------\n";
    PLW_MOUNT_NODE  pmnDev;
    PLW_LIST_LINE   plineTemp;
    
    CHAR            cBlkNameBuffer[MAX_FILENAME_LENGTH];
    PCHAR           pcBlkName;

    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return;
    }
    
    printf("Mount point show >>\n");
    printf(pcMountInfoHdr);                                             /*  打印欢迎信息                */
    
    __LW_MOUNT_LOCK();
    for (plineTemp  = _G_plineMountDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pmnDev = _LIST_ENTRY(plineTemp, LW_MOUNT_NODE, MN_lineManage);
        
        if (pmnDev->MN_blkd.BLKD_pcName) {
            pcBlkName = pmnDev->MN_blkd.BLKD_pcName;
        
        } else {
            INT     iRet;
            
            __KERNEL_SPACE_ENTER();                                     /*  此文件描述符属于内核        */
            iRet = API_IosFdGetName(pmnDev->MN_iFd, cBlkNameBuffer, MAX_FILENAME_LENGTH);
            __KERNEL_SPACE_EXIT();
            
            if (iRet < ERROR_NONE) {
                pcBlkName = "<unknown>";
            } else {
                pcBlkName = cBlkNameBuffer;
            }
        }
        printf("%-20s %-32s\n", pmnDev->MN_cVolName, pcBlkName);
    }
    __LW_MOUNT_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: mount
** 功能描述: linux 兼容 mount.
** 输　入  : pcDevName     设备名
**           pcVolName     挂载目标
**           pcFileSystem  文件系统
**           ulFlag        挂载参数
**           pvData        附加信息(未使用)
** 输　出  : 挂载结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  mount (CPCHAR  pcDevName, CPCHAR  pcVolName, CPCHAR  pcFileSystem, 
            ULONG   ulFlag, CPVOID pvData)
{
    PCHAR   pcOption = "rw";

    if (ulFlag & MS_RDONLY) {
        pcOption = "ro";
    }
    
    (VOID)pvData;
    
    return  (API_MountEx(pcDevName, pcVolName, pcFileSystem, pcOption));
}
/*********************************************************************************************************
** 函数名称: umount
** 功能描述: linux 兼容 umount.
** 输　入  : pcVolName     挂载节点
** 输　出  : 解除挂载结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  umount (CPCHAR  pcVolName)
{
    return  (API_Unmount(pcVolName));
}
/*********************************************************************************************************
** 函数名称: umount2
** 功能描述: linux 兼容 umount2.
** 输　入  : pcVolName     挂载节点
**           iFlag         MNT_FORCE 表示立即解除
** 输　出  : 解除挂载结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  umount2 (CPCHAR  pcVolName, INT iFlag)
{
    INT iFd;

    if (iFlag & MNT_FORCE) {
        iFd = open(pcVolName, O_RDONLY);
        if (iFd >= 0) {
            ioctl(iFd, FIOSETFORCEDEL, LW_TRUE);                        /*  允许立即卸载设备            */
            close(iFd);
        }
    }
    
    return  (API_Unmount(pcVolName));
}

#endif                                                                  /*  LW_CFG_BLKRAW_EN > 0        */
                                                                        /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_MOUNT_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
