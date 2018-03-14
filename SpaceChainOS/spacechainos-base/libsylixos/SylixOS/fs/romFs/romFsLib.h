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
** 文   件   名: romFsLib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 06 月 27 日
**
** 描        述: rom 文件系统 sylixos 内部函数. 
*********************************************************************************************************/

#ifndef __ROMFSLIB_H
#define __ROMFSLIB_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0 && LW_CFG_ROMFS_EN > 0

/*********************************************************************************************************
  检测路径字串是否为根目录或者直接指向设备
*********************************************************************************************************/

#define __STR_IS_ROOT(pcName)           ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))

/*********************************************************************************************************
  类型
*********************************************************************************************************/
typedef struct {
    UINT32              ROMFSDNT_uiNext;                                /*  下一个文件地址              */
    UINT32              ROMFSDNT_uiSpec;                                /*  如果是目录则为目录内文件地址*/
    UINT32              ROMFSDNT_uiData;                                /*  数据地址                    */
    
    
    UINT32              ROMFSDNT_uiMe;                                  /*  自身                        */
    
    struct stat         ROMFSDNT_stat;                                  /*  文件 stat                   */
    CHAR                ROMFSDNT_cName[MAX_FILENAME_LENGTH];            /*  文件名                      */
} ROMFS_DIRENT;
typedef ROMFS_DIRENT   *PROMFS_DIRENT;

typedef struct {
    LW_DEV_HDR          ROMFS_devhdrHdr;                                /*  romfs 文件系统设备头        */
    PLW_BLK_DEV         ROMFS_pblkd;                                    /*  romfs 物理设备              */
    LW_OBJECT_HANDLE    ROMFS_hVolLock;                                 /*  卷操作锁                    */
    LW_LIST_LINE_HEADER ROMFS_plineFdNodeHeader;                        /*  fd_node 链表                */
    
    BOOL                ROMFS_bForceDelete;                             /*  是否允许强制卸载卷          */
    BOOL                ROMFS_bValid;
    
    uid_t               ROMFS_uid;                                      /*  用户 id                     */
    gid_t               ROMFS_gid;                                      /*  组   id                     */
    time_t              ROMFS_time;                                     /*  创建时间                    */
    
    ULONG               ROMFS_ulSectorSize;                             /*  扇区大小                    */
    ULONG               ROMFS_ulSectorNum;                              /*  扇区个数                    */
    
                                                                        /*  动态内存非配, 字对齐        */
    PCHAR               ROMFS_pcSector;                                 /*  扇区缓冲                    */
    ULONG               ROMFS_ulCurSector;                              /*  当前扇区号                  */
    
    UINT32              ROMFS_uiRootAddr;                               /*  第一个文件的位置            */
    UINT32              ROMFS_uiTotalSize;                              /*  文件系统大小                */
} ROM_VOLUME;
typedef ROM_VOLUME     *PROM_VOLUME;

typedef struct {
    ROMFS_DIRENT        ROMFIL_romfsdnt;
    PROM_VOLUME         ROMFIL_promfs;
    
    UINT32              ROMFIL_ulCookieDir;
    
    INT                 ROMFIL_iFileType;
    CHAR                ROMFIL_cName[1];                                /*  文件名                      */
} ROM_FILE;
typedef ROM_FILE       *PROM_FILE;
/*********************************************************************************************************
  内部函数
*********************************************************************************************************/
INT      __rfs_mount(PROM_VOLUME  promfs);
ssize_t  __rfs_pread(PROM_VOLUME  promfs, PROMFS_DIRENT promdnt, 
                     PCHAR  pcDest, size_t stSize, UINT32  uiOffset);
ssize_t  __rfs_readlink(PROM_VOLUME  promfs, PROMFS_DIRENT promdnt, PCHAR  pcDest, size_t stSize);
INT      __rfs_getfile(PROM_VOLUME  promfs, UINT32  uiAddr, PROMFS_DIRENT promdnt);
INT      __rfs_open(PROM_VOLUME  promfs, CPCHAR  pcName, 
                    PCHAR  *ppcTail, PCHAR  *ppcSymfile, PROMFS_DIRENT  promdnt);
INT      __rfs_path_build_link(PROM_VOLUME  promfs, PROMFS_DIRENT  promdnt, 
                           PCHAR        pcDest, size_t         stSize,
                           PCHAR        pcPrefix, PCHAR        pcTail);

#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_ROMFS_EN > 0         */
#endif                                                                  /*  __ROMFSLIB_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
