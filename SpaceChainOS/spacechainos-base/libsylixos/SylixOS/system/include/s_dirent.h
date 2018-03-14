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
** 文   件   名: s_dirent.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 09 月 19 日
**
** 描        述: POSIX dirent.h 头文件.

** BUG:
2010.09.10  struct dirent 中加入 d_type 字段.
*********************************************************************************************************/

#ifndef __S_DIRENT_H
#define __S_DIRENT_H

/*********************************************************************************************************
  相关结构定义 (短文件名对其他文件系统不起效)
*********************************************************************************************************/

struct dirent {
    char	            d_name[NAME_MAX + 1];                           /*  文件名                      */
    unsigned char       d_type;                                         /*  文件类型 (可能为 DT_UNKNOWN)*/
    char                d_shortname[13];                                /*  fat 短文件名 (可能不存在)   */
    PVOID              *d_resv;                                         /*  保留                        */
};

struct dirent64 {                                                       /*  same as `struct dirent'     */
    char	            d_name[NAME_MAX + 1];                           /*  文件名                      */
    unsigned char       d_type;                                         /*  文件类型 (可能为 DT_UNKNOWN)*/
    char                d_shortname[13];                                /*  fat 短文件名 (可能不存在)   */
    PVOID              *d_resv;                                         /*  保留                        */
};

typedef struct {
    int                 dir_fd;                                         /*  文件描述符                  */
    LONG                dir_pos;                                        /*  位置                        */
    struct dirent       dir_dirent;                                     /*  获得的选项                  */
    LW_OBJECT_HANDLE    dir_lock;                                       /*  readdir_r 需要此锁          */
    LW_RESOURCE_RAW     dir_resraw;                                     /*  进程原始资源记录            */
    
#ifdef __SYLIXOS_KERNEL
#define DIR_RESV_DATA_PV0(pdir)   ((pdir)->dir_resraw.RESRAW_pvArg[5])  /*  文件系统加速搜索用          */
#define DIR_RESV_DATA_PV1(pdir)   ((pdir)->dir_resraw.RESRAW_pvArg[4])
#endif                                                                  /*  __SYLIXOS_KERNEL            */
} DIR;

/*********************************************************************************************************
  d_type 
  
  注意: 并不是每一种文件系统(或设备)都支持 d_type 字段, 所以当 d_type == DT_UNKNOWN 时, 必须调用 stat() 
        系列函数来获取文件真实的类型.
*********************************************************************************************************/

#define DT_UNKNOWN      0
#define DT_FIFO         1
#define DT_CHR          2
#define DT_DIR          4
#define DT_BLK          6
#define DT_REG          8
#define DT_LNK          10
#define DT_SOCK         12
#define DT_WHT          14

#define IFTODT(mode)    (unsigned char)(((mode) & 0xF000) >> 12)        /*  d_type to st_mode           */
#define DTTOIF(dtype)   (mode_t)((dtype) << 12)                         /*  st_mode to d_type           */

/*********************************************************************************************************
  POSIX API
*********************************************************************************************************/

LW_API INT              mkdir(CPCHAR  dirname, mode_t  mode);
LW_API INT              rmdir(CPCHAR  pathname); 

LW_API INT              dirfd(DIR  *pdir);
LW_API DIR             *opendir(CPCHAR   pathname);  
LW_API INT              closedir(DIR    *dir);
LW_API struct dirent   *readdir(DIR     *dir);
LW_API struct dirent64 *readdir64(DIR   *dir);
LW_API INT              readdir_r(DIR             *pdir, 
                                  struct dirent   *pdirentEntry,
                                  struct dirent  **ppdirentResult);
LW_API INT              readdir64_r(DIR             *pdir, 
                                    struct dirent64   *pdirent64Entry,
                                    struct dirent64  **ppdirent64Result);
LW_API INT              rewinddir(DIR   *dir);  

#endif                                                                  /*  __S_DIRENT_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
