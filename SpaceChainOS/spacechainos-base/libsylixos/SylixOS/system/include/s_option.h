/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: s_option.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 13 日
**
** 描        述: 这是系统选项宏定义。
**
** 注        意: <FIOTRUNC, FIOSEEK, FIOWHERE>
                 这三个命令的操作数位 off_t 指针, 即指向 64bit 数指针.
                 IO 系统最大支持 1TB 大小的文件, 但是受限于 FAT 系统 (4GB FILE MAX)
                 <FIONREAD>
                 仍然保持 LONG 型, 也就是说, 超过 2GB 的文件 FIONREAD 可能不准确.
                 同时 stdio 库的 fseek 第二个参数为 long, 最大支持 2GB 文件. (未来改进).
                 
** BUG:
2012.11.21  不再需要 O_TEMP 选项, 因为 SylixOS 已经支持 unlink 被打开的文件. 
2013.01.05  加入文件记录锁功能.
2014.07.21  加入电源管理功能.
2015.09.09  暂时废弃 FIODIRENTRY, FIOSCSICOMMAND, FIOINODETONAME
*********************************************************************************************************/

#ifndef  __S_OPTION_H
#define  __S_OPTION_H

/*********************************************************************************************************
  'old' standard ioctl cmd
*********************************************************************************************************/

#define  LW_OSIO(g, n)      n                                   /* 无参数 ioctl                         */
#define  LW_OSIOD(g, n, d)  n                                   /* 第三个参数为直接数值 ioctl           */

#define  LW_OSIOR(g, n, t)  n                                   /* 第三个参数为类型 t 指针的读 ioctl    */
#define  LW_OSIOW(g, n, t)  n                                   /* 第三个参数为类型 t 指针的写 ioctl    */
#define  LW_OSIOWR(g, n, t) n                                   /* 第三个参数为类型 t 指针的读写 ioctl  */

/*********************************************************************************************************
  ioctl parameter 注意凡是 LW_OSIOD(*, *, LONG) 或者 LW_OSIOD(*, *, ULONG) 第三个参数如果为立即数, 必须:
  ioctl(fd, *, LW_OSIOD_LARG(integer))
*********************************************************************************************************/

#define  LW_OSIOD_LARG(x)   (LONG)(x)
#define  LW_OSIOD_IARG(x)   (INT)(x)

/*********************************************************************************************************
  path buffer
*********************************************************************************************************/

#define  LW_PATHB           CHAR[PATH_MAX + 1]

/*********************************************************************************************************
  ioctl cmd
*********************************************************************************************************/

#define  FIONREAD           LW_OSIOR('f', 1, INT)               /* get num chars available to read      */
#define  FIOFLUSH           LW_OSIO( 'f', 2)                    /* flush any chars in buffers           */
#define  FIOOPTIONS         LW_OSIOD('f', 3, INT)               /* set options (FIOSETOPTIONS)          */
#define  FIOBAUDRATE        LW_OSIOD('f', 4, LONG)              /* set serial baud rate                 */

#ifdef __SYLIXOS_KERNEL
#define  FIODISKFORMAT      LW_OSIOD('f', 5, LONG)              /* format disk                          */
#define  FIODISKINIT        LW_OSIOD('f', 6, LONG)              /* initialize disk directory            */
#define  FIORENAME          LW_OSIOW('f', 10, LW_PATHB)         /* rename a directory entry             */
#define  FIOMOVE            FIORENAME
#endif

#define  FIOSEEK            LW_OSIOW('f', 7, off_t)             /* set current file char position       */
#define  FIOWHERE           LW_OSIOR('f', 8, off_t)             /* get current file char position       */
#define  FIOREADYCHANGE     LW_OSIO( 'f', 11)                   /* return TRUE if there has been ready  */

/*********************************************************************************************************
  media change on the device
*********************************************************************************************************/

#define  FIONWRITE          LW_OSIOR('f', 12, INT)              /* get num chars still to be written    */
#define  FIODISKCHANGE      LW_OSIO( 'f', 13)                   /* set a media change on the device     */
#define  FIOCANCEL          LW_OSIO( 'f', 14)                   /* cancel read or write on the device   */
#define  FIOSQUEEZE         LW_OSIO( 'f', 15)                   /* squeeze out empty holes in rt-11     */

/*********************************************************************************************************
  file system
*********************************************************************************************************/

#define  FIONBIO            LW_OSIOW('f', 16, INT)              /* set non-blocking I/O;                */
#define  FIONMSGS           LW_OSIOR('f', 17, INT)              /* return num msgs in pipe              */
#define  FIOGETNAME         LW_OSIOR('f', 18, LW_PATHB)         /* return file name in arg              */
#define  FIOGETOPTIONS      LW_OSIOR('f', 19, INT)              /* get options                          */
#define  FIOSETOPTIONS      FIOOPTIONS                          /* set options                          */
#define  FIOISATTY          LW_OSIOR('f', 20, BOOL)             /* is a tty                             */
#define  FIOSYNC            LW_OSIO( 'f', 21)                   /* sync to disk                         */

#ifdef __SYLIXOS_KERNEL
#define  FIOPROTOHOOK       LW_OSIOD('f', 22, FUNCPTR)          /* specify protocol hook routine        */
#define  FIOPROTOARG        LW_OSIOD('f', 23, PVOID)            /* specify protocol argument            */
#endif

#define  FIORBUFSET         LW_OSIOD('f', 24, LONG)             /* alter the size of read buffer        */
#define  FIOWBUFSET         LW_OSIOD('f', 25, LONG)             /* alter the size of write buffer       */
#define  FIORFLUSH          LW_OSIO( 'f', 26)                   /* flush any chars in read buffers      */
#define  FIOWFLUSH          LW_OSIO( 'f', 27)                   /* flush any chars in write buffers     */

#ifdef __SYLIXOS_KERNEL
#define  FIOSELECT          LW_OSIOW('f', 28, LW_SEL_WAKEUPNODE)/* wake up process in select on I/O     */
#define  FIOUNSELECT        LW_OSIOW('f', 29, LW_SEL_WAKEUPNODE)/* wake up process in select on I/O     */
#endif

#define  FIONFREE           LW_OSIOR('f', 30, INT)              /* get free byte count on device        */

#ifdef __SYLIXOS_KERNEL
#define  FIOTRIM            LW_OSIOW('f', 31, LW_BLK_RANGE)     /* ATA TRIM command                     */
#define  FIOSYNCMETA        LW_OSIOW('f', 32, LW_BLK_RANGE)     /* sync range sector to disk            */
#endif

#define  FIOLABELGET        LW_OSIOR('f', 33, LW_PATHB)         /* get volume label                     */
#define  FIOLABELSET        LW_OSIOW('f', 34, LW_PATHB)         /* set volume label                     */
#define  FIOATTRIBSET       LW_OSIOD('f', 35, LONG)             /* set file attribute                   */
#define  FIOCONTIG          LW_OSIOD('f', 36, UINT)             /* allocate contiguous space            */

#ifdef __SYLIXOS_KERNEL
#define  FIOREADDIR         LW_OSIOR('f', 37, DIR)              /* read a directory entry (POSIX)       */
#define  FIOFSTATGET        LW_OSIOR('f', 38, struct stat)      /* get file status info                 */
#define  FIOUNMOUNT         LW_OSIO( 'f', 39)                   /* unmount disk volume                  */
#endif

#define  FIONCONTIG         LW_OSIOR('f', 41, UINT)             /* get size of max contig area on dev   */
#define  FIOTRUNC           LW_OSIOW('f', 42, off_t)            /* truncate file to specified length    */
#define  FIOGETFL           LW_OSIOR('f', 43, INT)              /* get file mode, like fcntl(F_GETFL)   */
#define  FIOTIMESET         LW_OSIOW('f', 44, struct utimbuf)   /* change times on a file for utime()   */
#define  FIOFSTATFSGET      LW_OSIOR('f', 46, struct statfs)    /* get file status info 64bit           */
#define  FIOFSTATGET64      LW_OSIOR('f', 47, struct stat64)    /* move file, ala mv, (mv not rename)   */
#define  FIODATASYNC        LW_OSIO( 'f', 48)                   /* sync data to disk                    */
#define  FIOSETFL           LW_OSIOW('f', 49, INT)              /* set file mode, like fcntl(F_SETFL)   */

#define  FIOGETCC           LW_OSIOR('f', 50, CHAR[NCCS])       /* get tty ctl char table               */
#define  FIOSETCC           LW_OSIOW('f', 51, CHAR[NCCS])       /* set tty ctl char table               */

#ifdef __SYLIXOS_KERNEL
#define  FIOGETLK           LW_OSIOR('f', 52, struct flock)     /* get a lockf                          */
#define  FIOSETLK           LW_OSIOW('f', 53, struct flock)     /* set a lockf                          */
#define  FIOSETLKW          LW_OSIOW('f', 54, struct flock)     /* set a lockf (with blocking)          */
#endif

/*********************************************************************************************************
  SylixOS extern
*********************************************************************************************************/

#define  FIORTIMEOUT        LW_OSIOW('f', 60, struct timeval)   /* 设置读超时时间                       */
#define  FIOWTIMEOUT        LW_OSIOW('f', 61, struct timeval)   /* 设置写超时时间                       */

#ifdef __SYLIXOS_KERNEL
#define  FIOWAITABORT       LW_OSIOD('f', 62, INT)              /* 当前阻塞线程停止等待 read()/write()  */
#define  FIOABORTFUNC       LW_OSIOD('f', 63, FUNCPTR)          /* tty 接收到 control-C 时的动作设置    */
#define  FIOABORTARG        LW_OSIOD('f', 64, PVOID)            /* tty 接收到 control-C 时的参数设置    */

#define  FIOCHMOD           LW_OSIOD('f', 70, INT)              /* chmod                                */
#define  FIOCHOWN           LW_OSIOW('f', 71, LW_IO_USR)        /* chown                                */

#define  FIOGETOWN          LW_OSIOR('f', 75, pid_t)            /* F_GETOWN                             */
#define  FIOSETOWN          LW_OSIOD('f', 76, pid_t)            /* F_SETOWN                             */

#define  FIOGETSIG          LW_OSIOR('f', 77, INT)              /* F_GETSIG                             */
#define  FIOSETSIG          LW_OSIOD('f', 78, INT)              /* F_SETSIG                             */
#endif

#define  FIOFSTYPE          LW_OSIOR('f', 80, LW_PATHB)         /* get file system type sring           */

/*********************************************************************************************************
  hardware rtc driver
*********************************************************************************************************/

#define  FIOGETTIME         LW_OSIOR('f', 90, time_t)           /*  get hardware RTC time (arg:time_t *)*/
#define  FIOSETTIME         LW_OSIOW('f', 91, time_t)           /*  set hardware RTC time (arg:time_t *)*/

/*********************************************************************************************************
  force delete
*********************************************************************************************************/

#define  FIOGETFORCEDEL     LW_OSIOR('f', 92, BOOL)             /*  get force delete flag               */
#define  FIOSETFORCEDEL     LW_OSIOD('f', 93, BOOL)             /*  set force delete flag               */

/*********************************************************************************************************
  power manager
*********************************************************************************************************/

#define  FIOSETWATCHDOG     LW_OSIOD('f', 94, ULONG)            /*  set power manager watchdog (sec)    */
#define  FIOGETWATCHDOG     LW_OSIOR('f', 95, ULONG)            /*  get power manager watchdog (sec)    */
#define  FIOWATCHDOGOFF     LW_OSIO( 'f', 96)                   /*  turn-off power manager watchdog     */

/*********************************************************************************************************
  64 bit extern
*********************************************************************************************************/

#define  FIONFREE64         LW_OSIOR('f', (FIONFREE + 100), off_t)
#define  FIONREAD64         LW_OSIOR('f', (FIONREAD + 100), off_t)

/*********************************************************************************************************
  自定义功能
*********************************************************************************************************/

#define  FIOUSRFUNC         0x2000

/*********************************************************************************************************
  ioctl option values
*********************************************************************************************************/

#define  OPT_ECHO                           0x01                /* echo input                           */
#define  OPT_CRMOD                          0x02                /* lf -> crlf                           */
#define  OPT_TANDEM                         0x04                /* ^S/^Q flow control protocol          */
#define  OPT_7_BIT                          0x08                /* strip parity bit from 8 bit input    */
#define  OPT_MON_TRAP                       0x10                /* enable trap to monitor               */
#define  OPT_ABORT                          0x20                /* enable shell restart                 */
#define  OPT_LINE                           0x40                /* enable basic line protocol           */

#define  OPT_RAW                            0                   /* raw mode                             */

#if LW_CFG_SIO_TERMINAL_NOT_7_BIT > 0
#define  OPT_TERMINAL                       (OPT_ECHO     | OPT_CRMOD | OPT_TANDEM | \
			                                 OPT_MON_TRAP | OPT_ABORT | OPT_LINE)
#else
#define  OPT_TERMINAL                       (OPT_ECHO     | OPT_CRMOD | OPT_TANDEM | \
			                                 OPT_MON_TRAP | OPT_7_BIT | OPT_ABORT  | OPT_LINE)
#endif                                                          /*  LW_CFG_SIO_TERMINAL_NOT_7_BIT > 0   */

#define  CONTIG_MAX                         -1                  /* "count" for FIOCONTIG if requesting  */
                                                                /*  maximum contiguous space on dev     */
/*********************************************************************************************************
  abort flag
*********************************************************************************************************/

#define  OPT_RABORT                         0x01                /*  read() 阻塞退出                     */
#define  OPT_WABORT                         0x02                /*  write() 阻塞退出                    */

/*********************************************************************************************************
  miscellaneous
*********************************************************************************************************/

#define  FOLLOW_LINK_FILE                   -2                  /* this file is a symbol link file      */
#define  FOLLOW_LINK_TAIL                   -3                  /* file in symbol link file (have tail) */

#define  DEFAULT_SYMLINK_PERM               0000777             /* default symbol link permissions      */
                                                                /* unix style rwxrwxrwx                 */
#define  DEFAULT_FILE_PERM                  0000644             /* default file permissions             */
                                                                /* unix style rw-r--r--                 */
#define  DEFAULT_DIR_PERM                   0000754             /* default directory permissions        */
                                                                /* unix style rwxr-xr--                 */

/*********************************************************************************************************
  file types -- NOTE:  these values are specified in the NFS protocol spec 
*********************************************************************************************************/

#define  FSTAT_DIR                          0040000             /* directory                            */
#define  FSTAT_CHR                          0020000             /* character special file               */
#define  FSTAT_BLK                          0060000             /* block special file                   */
#define  FSTAT_REG                          0100000             /* regular file                         */
#define  FSTAT_LNK                          0120000             /* symbolic link file                   */
#define  FSTAT_NON                          0140000             /* named socket                         */

/*********************************************************************************************************
  lseek offset option
*********************************************************************************************************/

#ifndef  SEEK_SET
#define  SEEK_SET	                        0                   /* set file offset to offset            */
#endif

#ifndef  SEEK_CUR
#define  SEEK_CUR                           1                   /* set file offset to current plus oft  */
#endif

#ifndef  SEEK_END
#define  SEEK_END	                        2                   /* set file offset to EOF plus offset   */
#endif

#endif                                                          /*  __S_OPTION_H                        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
