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
** 文   件   名: s_stat.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 09 月 19 日
**
** 描        述: POSIX stat.h 头文件.

** BUG:
2012.09.22  statfs 加入 f_flag 标志.
*********************************************************************************************************/

#ifndef __S_STAT_H
#define __S_STAT_H

/*********************************************************************************************************
  相关结构定义 (注意 st_atime, st_mtime, st_ctime 为 UTC 时间)
*********************************************************************************************************/

struct stat {
    dev_t         st_dev;                                               /* device                       */
    ino_t         st_ino;                                               /* inode                        */
    mode_t        st_mode;                                              /* protection                   */
    nlink_t       st_nlink;                                             /* number of hard links         */
    uid_t         st_uid;                                               /* user ID of owner             */
    gid_t         st_gid;                                               /* group ID of owner            */
    dev_t         st_rdev;                                              /* device type (if inode device)*/
    off_t         st_size;                                              /* total size, in bytes         */
    time_t        st_atime;                                             /* time of last access          */
    time_t        st_mtime;                                             /* time of last modification    */
    time_t        st_ctime;                                             /* time of last create          */
    blksize_t     st_blksize;                                           /* blocksize for filesystem I/O */
    blkcnt_t      st_blocks;                                            /* number of blocks allocated   */
    
    void         *st_resv1;
    void         *st_resv2;
    void         *st_resv3;
};

struct stat64 {
    dev_t         st_dev;
    ino64_t       st_ino;
    mode_t        st_mode;
    nlink_t       st_nlink;
    uid_t         st_uid;
    gid_t         st_gid;
    dev_t         st_rdev;
    off64_t       st_size;
    time_t        st_atime;
    time_t        st_mtime;
    time_t        st_ctime;
    blksize_t     st_blksize;
    blkcnt64_t    st_blocks;
    
    void         *st_resv1;
    void         *st_resv2;
    void         *st_resv3;
}; 

typedef struct fsid { 
    int32_t       val[2]; 
} fsid_t;                                                               /* filesystem id type           */

#define ST_RDONLY   0x1
#define ST_NOSUID   0x2

struct statfs {
    long          f_type;                                               /* type of info, zero for now   */
    long          f_bsize;                                              /* fundamental file system block*/
                                                                        /* size                         */
    long          f_blocks;                                             /* total blocks in file system  */
    long          f_bfree;                                              /* free block in fs             */
    long          f_bavail;                                             /* free blocks avail to         */
                                                                        /* non-superuser                */
    long          f_files;                                              /* total file nodes in file     */
                                                                        /* system                       */
    long          f_ffree;                                              /* free file nodes in fs        */
    fsid_t        f_fsid;                                               /* file system id               */
    long          f_flag;                                               /* Bit mask of f_flag values.   */
    long          f_namelen;                                            /* max file name len            */
    long          f_spare[7];                                           /* spare for later              */
};

/*********************************************************************************************************
  File mode (st_mode) bit masks
*********************************************************************************************************/

#define S_IFMT      0xf000                                              /*  file type field             */
#define S_IFIFO     0x1000                                              /*  fifo                        */
#define S_IFCHR     0x2000                                              /*  character special           */
#define S_IFDIR     0x4000                                              /*  directory                   */
#define S_IFBLK     0x6000                                              /*  block special               */
#define S_IFREG     0x8000                                              /*  regular                     */
#define S_IFLNK     0xa000                                              /*  symbolic link               */
#define S_IFSOCK    0xc000                                              /*  socket                      */

#define S_ISUID     0x0800                                              /* set user id on execution     */
#define S_ISGID     0x0400                                              /* set group id on execution    */
#define S_ISVTX     0x0200                                              /* set on a directory means     */
                                                                        /* that a file in that directory*/
                                                                        /* can be renamed or deleted    */
                                                                        /* only by the owner of the file*/

#define S_IRUSR     0x0100                                              /* read permission, owner       */
#define S_IWUSR     0x0080                                              /* write permission, owner      */
#define S_IXUSR     0x0040                                              /* execute/search permission,   */
                                                                        /* owner                        */
#define S_IRWXU     0x01c0                                              /* read/write/execute permission*/
                                                                        /* owner                        */

#define S_IRGRP     0x0020                                              /* read permission, group       */
#define S_IWGRP     0x0010                                              /* write permission, group      */
#define S_IXGRP     0x0008                                              /* execute/search permission,   */
                                                                        /* group                        */
#define S_IRWXG     0x0038                                              /* read/write/execute permission*/
                                                                        /* , group                      */

#define S_IROTH     0x0004                                              /* read permission, other       */
#define S_IWOTH     0x0002                                              /* write permission, other      */
#define S_IXOTH     0x0001                                              /* execute/search permission,   */
                                                                        /* other                        */
#define S_IRWXO     0x0007                                              /* read/write/execute permission*/
                                                                        /* other                        */

#define S_IREAD     S_IRUSR
#define S_IWRITE    S_IWUSR
#define S_IEXEC     S_IXUSR

/*********************************************************************************************************
  判断相应的位
*********************************************************************************************************/

#define S_ISDIR(mode)           ((mode & S_IFMT) == S_IFDIR)            /* directory                    */
#define S_ISCHR(mode)           ((mode & S_IFMT) == S_IFCHR)            /* character special            */
#define S_ISBLK(mode)           ((mode & S_IFMT) == S_IFBLK)            /* block special                */
#define S_ISREG(mode)           ((mode & S_IFMT) == S_IFREG)            /* regular file                 */
#define S_ISLNK(mode)           ((mode & S_IFMT) == S_IFLNK)            /* symbolic link                */
#define S_ISFIFO(mode)          ((mode & S_IFMT) == S_IFIFO)            /* fifo special                 */
#define S_ISSOCK(mode)          ((mode & S_IFMT) == S_IFSOCK)           /* sock special                 */

/*********************************************************************************************************
  access() 函数
*********************************************************************************************************/

#undef  R_OK
#define R_OK            4                                               /*  read                        */
#undef  W_OK
#define W_OK            2                                               /*  write                       */
#undef  X_OK
#define X_OK            1                                               /*  executable                  */
#undef  F_OK
#define F_OK            0                                               /*  exist                       */

/*********************************************************************************************************
  POSIX API
*********************************************************************************************************/

LW_API INT       access(CPCHAR  pcName, INT  iMode);

LW_API INT       fstat(INT  iFd, struct stat *pstat);
LW_API INT       stat(CPCHAR pcName, struct stat *pstat);
LW_API INT       lstat(CPCHAR  pcName, struct stat *pstat);

LW_API INT       fstat64(INT  iFd, struct stat64 *pstat64);
LW_API INT       stat64(CPCHAR pcName, struct stat64 *pstat64);
LW_API INT       lstat64(CPCHAR  pcName, struct stat64 *pstat64);

LW_API INT       fstatfs(INT  iFd, struct statfs *pstatfs);
LW_API INT       statfs(CPCHAR pcName, struct statfs *pstatfs);

LW_API INT       ftruncate(INT  iFd, off_t  oftLength);
LW_API INT       truncate(CPCHAR  pcName, off_t  oftLength);

LW_API INT       ftruncate64(INT  iFd, off64_t  oftLength);
LW_API INT       truncate64(CPCHAR  pcName, off64_t  oftLength);

LW_API VOID      sync(VOID);
LW_API INT       fsync(INT  iFd);

LW_API INT       fchmod(INT  iFd, INT  iMode);
LW_API INT       chmod(CPCHAR  pcName, INT  iMode);

#endif                                                                  /*  __S_STAT_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
