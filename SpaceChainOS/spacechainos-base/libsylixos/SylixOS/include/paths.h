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
** 文   件   名: paths.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 09 月 22 日
**
** 描        述: 操作系统标准路径表.
*********************************************************************************************************/

#ifndef __PATHS_H
#define __PATHS_H

#define _PATH_DEFPATH       "/usr/bin:/bin:/usr/pkg/sbin:/sbin:/usr/local/bin"
#define _PATH_STDPATH       "/usr/bin:/bin:/usr/sbin:/sbin:/usr/pkg/bin:/usr/pkg/sbin:" \
                            "/usr/local/bin:/usr/local/sbin"
                            
#define _PATH_LIBPATH       "/usr/lib:/lib:/usr/local/lib"
        
#define _PATH_FB            "/dev/fb"
#define _PATH_FB0           "/dev/fb0"

#define _PATH_BEEP          "/dev/beep"
#define _PATH_BEEP0         "/dev/beep0"

#define _PATH_CAN           "/dev/can"
#define _PATH_CAN0          "/dev/can0"

#define _PATH_INPUT         "/dev/input"
        
#define _PATH_AUDIO         "/dev/dsp"
#define _PATH_AUDIO0        "/dev/dsp0"

#define _PATH_AUDIOCTL      "/dev/audioctl"
#define _PATH_AUDIOCTL0     "/dev/audioctl0"

#define _PATH_MIXER         "/dev/mixer"
#define _PATH_MIXER0        "/dev/mixer0"

#define _PATH_SOUND         "/dev/sound"
#define _PATH_SOUND0        "/dev/sound0"

#define _PATH_CLOCK         "/dev/rtc"

#define _PATH_CONSOLE       "/dev/console"
#define _PATH_CONSTTY       "/dev/constty"

#define _PATH_CSMAPPER      "/usr/share/i18n/csmapper"
#define _PATH_ESDB          "/usr/share/i18n/esdb"
#define _PATH_I18NMODULE    "/usr/lib/i18n"
#define _PATH_ICONV         "/usr/share/i18n/iconv"
#define _PATH_LOCALE        "/usr/share/locale"

#define _PATH_DEVDB         "/var/run/dev.db"
#define _PATH_DEVNULL       "/dev/null"

#define _PATH_TTY           "/dev/tty"

#define _PATH_DEV           "/dev/"
#define _PATH_DEV_PTS       "/dev/pts/"
#define _PATH_EMUL_AOUT     "/emul/aout/"
#define _PATH_TMP           "/tmp/"
#define _PATH_VARDB         "/var/db/"
#define _PATH_VARRUN        "/var/run/"
#define _PATH_VARTMP        "/var/tmp/"

#endif                                                                  /*  __PATHS_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
