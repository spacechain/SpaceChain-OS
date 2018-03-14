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
** 文   件   名: soundcard.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 07 月 05 日
**
** 描        述: 简易 OSS (open sound system) 的声卡操作.
** 注        意: 这里仅是一个设备驱动的接口规范, 所以不包含在 SylixOS.h 头文件中.
*********************************************************************************************************/

#ifndef __SOUNDCARD_H
#define __SOUNDCARD_H

#include "endian.h"
#include "sys/ioctl.h"

/*********************************************************************************************************
  ioctl command packet
*********************************************************************************************************/

#define SIOCPARM_MASK                           IOCPARM_MASK
#define SIOC_VOID                               IOC_VOID
#define SIOC_OUT                                IOC_OUT
#define SIOC_IN                                 IOC_IN
#define SIOC_INOUT                              IOC_INOUT
#define _SIO                                    _IO
#define _SIOR                                   _IOR
#define _SIOW                                   _IOW
#define _SIOWR                                  _IOWR

/*********************************************************************************************************
  MIXER 读写操作区分
*********************************************************************************************************/

#define MIXER_WRITE_FLAG                        0x80000000
#define MIXER_READ_FLAG                         0x40000000
#define MIXER_WRITE(a)                          (a | MIXER_WRITE_FLAG)
#define MIXER_READ(a)                           (a | MIXER_READ_FLAG)

/*********************************************************************************************************
  MIXER 操作参数
*********************************************************************************************************/

#define SOUND_MIXER_NRDEVICES                   25
#define SOUND_MIXER_VOLUME                      0                       /*  主音量调节                  */
#define SOUND_MIXER_BASS                        1                       /*  低音控制                    */
#define SOUND_MIXER_TREBLE                      2                       /*  高音控制                    */
#define SOUND_MIXER_SYNTH                       3                       /*  FM 合成器                   */
#define SOUND_MIXER_PCM                         4                       /*  主 D/A 转换器               */
#define SOUND_MIXER_SPEAKER                     5                       /*  PC 喇叭                     */
#define SOUND_MIXER_LINE                        6                       /*  音频线输入                  */
#define SOUND_MIXER_MIC                         7                       /*  麦克风输入                  */
#define SOUND_MIXER_CD                          8                       /*  CD 输入                     */
#define SOUND_MIXER_IMIX                        9                       /*  放音音量                    */
#define SOUND_MIXER_ALTPCM                      10                      /*  从 D/A 转换器               */
#define SOUND_MIXER_RECLEV                      11                      /*  录音音量                    */
#define SOUND_MIXER_IGAIN                       12                      /*  输入增益                    */
#define SOUND_MIXER_OGAIN                       13                      /*  输出增益                    */
#define SOUND_MIXER_LINE1                       14                      /*  声卡的第 1 输入             */
#define SOUND_MIXER_LINE2                       15                      /*  声卡的第 2 输入             */
#define SOUND_MIXER_LINE3                       16                      /*  声卡的第 3 输入             */
#define SOUND_MIXER_DIGITAL1                    17                      /*  Digital (input) 1           */
#define SOUND_MIXER_DIGITAL2                    18                      /*  Digital (input) 2           */
#define SOUND_MIXER_DIGITAL3                    19                      /*  Digital (input) 3           */
#define SOUND_MIXER_PHONEIN                     20                      /*  Phone input                 */
#define SOUND_MIXER_PHONEOUT                    21                      /*  Phone output                */
#define SOUND_MIXER_VIDEO                       22                      /*  Video/TV (audio) in         */
#define SOUND_MIXER_RADIO                       23                      /*  Radio in                    */
#define SOUND_MIXER_MONITOR                     24                      /*  Monitor (usually mic) volume*/

#define SOUND_ONOFF_MIN                         28
#define SOUND_ONOFF_MAX                         30
#define SOUND_MIXER_NONE                        31

#define SOUND_MIXER_RECSRC                      0xff
#define SOUND_MIXER_DEVMASK                     0xfe                    /*  支持设备位掩码              */
#define SOUND_MIXER_RECMASK                     0xfd                    /*  支持的录音通道掩码          */
#define SOUND_MIXER_STEREODEVS                  0xfb                    /*  支持立体声                  */
#define SOUND_MIXER_CAPS                        0xfc

/*********************************************************************************************************
 * The following unsupported macros are no longer functional.
 * Use SOUND_MIXER_PRIVATE# macros in future.
*********************************************************************************************************/

#define SOUND_MIXER_ENHANCE                     SOUND_MIXER_NONE
#define SOUND_MIXER_MUTE                        SOUND_MIXER_NONE
#define SOUND_MIXER_LOUD                        SOUND_MIXER_NONE

#define SOUND_DEVICE_LABELS     {"Vol  ", "Bass ", "Trebl", "Synth", "Pcm  ", "Spkr ", "Line ", \
                                 "Mic  ", "CD   ", "Mix  ", "Pcm2 ", "Rec  ", "IGain", "OGain", \
                                 "Line1", "Line2", "Line3", "Digital1", "Digital2", "Digital3", \
                                 "PhoneIn", "PhoneOut", "Video", "Radio", "Monitor"}

#define SOUND_DEVICE_NAMES      {"vol", "bass", "treble", "synth", "pcm", "speaker", "line", \
                                 "mic", "cd", "mix", "pcm2", "rec", "igain", "ogain", \
                                 "line1", "line2", "line3", "dig1", "dig2", "dig3", \
                                 "phin", "phout", "video", "radio", "monitor"}

/*********************************************************************************************************
  MIXER ioctl 读取操作 (/dev/mixer)
*********************************************************************************************************/

#define SOUND_MIXER_READ_VOLUME                 MIXER_READ(SOUND_MIXER_VOLUME)
#define SOUND_MIXER_READ_BASS                   MIXER_READ(SOUND_MIXER_BASS)
#define SOUND_MIXER_READ_TREBLE                 MIXER_READ(SOUND_MIXER_TREBLE)
#define SOUND_MIXER_READ_SYNTH                  MIXER_READ(SOUND_MIXER_SYNTH)
#define SOUND_MIXER_READ_PCM                    MIXER_READ(SOUND_MIXER_PCM)
#define SOUND_MIXER_READ_SPEAKER                MIXER_READ(SOUND_MIXER_SPEAKER)
#define SOUND_MIXER_READ_LINE                   MIXER_READ(SOUND_MIXER_LINE)
#define SOUND_MIXER_READ_MIC                    MIXER_READ(SOUND_MIXER_MIC)
#define SOUND_MIXER_READ_CD                     MIXER_READ(SOUND_MIXER_CD)
#define SOUND_MIXER_READ_IMIX                   MIXER_READ(SOUND_MIXER_IMIX)
#define SOUND_MIXER_READ_ALTPCM                 MIXER_READ(SOUND_MIXER_ALTPCM)
#define SOUND_MIXER_READ_RECLEV                 MIXER_READ(SOUND_MIXER_RECLEV)
#define SOUND_MIXER_READ_IGAIN                  MIXER_READ(SOUND_MIXER_IGAIN)
#define SOUND_MIXER_READ_OGAIN                  MIXER_READ(SOUND_MIXER_OGAIN)
#define SOUND_MIXER_READ_LINE1                  MIXER_READ(SOUND_MIXER_LINE1)
#define SOUND_MIXER_READ_LINE2                  MIXER_READ(SOUND_MIXER_LINE2)
#define SOUND_MIXER_READ_LINE3                  MIXER_READ(SOUND_MIXER_LINE3)

#define SOUND_MIXER_READ_RECSRC                 MIXER_READ(SOUND_MIXER_RECSRC)
#define SOUND_MIXER_READ_DEVMASK                MIXER_READ(SOUND_MIXER_DEVMASK)
#define SOUND_MIXER_READ_RECMASK                MIXER_READ(SOUND_MIXER_RECMASK)
#define SOUND_MIXER_READ_STEREODEVS             MIXER_READ(SOUND_MIXER_STEREODEVS)
#define SOUND_MIXER_READ_CAPS                   MIXER_READ(SOUND_MIXER_CAPS)

#define SOUND_MIXER_READ_MUTE                   MIXER_READ(SOUND_MIXER_MUTE)
#define SOUND_MIXER_READ_ENHANCE                MIXER_READ(SOUND_MIXER_ENHANCE)
#define SOUND_MIXER_READ_LOUD                   MIXER_READ(SOUND_MIXER_LOUD)

#define SOUND_MIXER_READ_RECSRC                 MIXER_READ(SOUND_MIXER_RECSRC)
#define SOUND_MIXER_READ_DEVMASK                MIXER_READ(SOUND_MIXER_DEVMASK)
#define SOUND_MIXER_READ_RECMASK                MIXER_READ(SOUND_MIXER_RECMASK)
#define SOUND_MIXER_READ_STEREODEVS             MIXER_READ(SOUND_MIXER_STEREODEVS)
#define SOUND_MIXER_READ_CAPS                   MIXER_READ(SOUND_MIXER_CAPS)

/*********************************************************************************************************
  MIXER ioctl 写入操作 (/dev/mixer)
*********************************************************************************************************/

#define SOUND_MIXER_WRITE_VOLUME                MIXER_WRITE(SOUND_MIXER_VOLUME)
#define SOUND_MIXER_WRITE_BASS                  MIXER_WRITE(SOUND_MIXER_BASS)
#define SOUND_MIXER_WRITE_TREBLE                MIXER_WRITE(SOUND_MIXER_TREBLE)
#define SOUND_MIXER_WRITE_SYNTH                 MIXER_WRITE(SOUND_MIXER_SYNTH)
#define SOUND_MIXER_WRITE_PCM                   MIXER_WRITE(SOUND_MIXER_PCM)
#define SOUND_MIXER_WRITE_SPEAKER               MIXER_WRITE(SOUND_MIXER_SPEAKER)
#define SOUND_MIXER_WRITE_LINE                  MIXER_WRITE(SOUND_MIXER_LINE)
#define SOUND_MIXER_WRITE_MIC                   MIXER_WRITE(SOUND_MIXER_MIC)
#define SOUND_MIXER_WRITE_CD                    MIXER_WRITE(SOUND_MIXER_CD)
#define SOUND_MIXER_WRITE_IMIX                  MIXER_WRITE(SOUND_MIXER_IMIX)
#define SOUND_MIXER_WRITE_ALTPCM                MIXER_WRITE(SOUND_MIXER_ALTPCM)
#define SOUND_MIXER_WRITE_RECLEV                MIXER_WRITE(SOUND_MIXER_RECLEV)
#define SOUND_MIXER_WRITE_IGAIN                 MIXER_WRITE(SOUND_MIXER_IGAIN)
#define SOUND_MIXER_WRITE_OGAIN                 MIXER_WRITE(SOUND_MIXER_OGAIN)
#define SOUND_MIXER_WRITE_LINE1                 MIXER_WRITE(SOUND_MIXER_LINE1)
#define SOUND_MIXER_WRITE_LINE2                 MIXER_WRITE(SOUND_MIXER_LINE2)
#define SOUND_MIXER_WRITE_LINE3                 MIXER_WRITE(SOUND_MIXER_LINE3)

#define SOUND_MIXER_WRITE_MUTE                  MIXER_WRITE(SOUND_MIXER_MUTE)
#define SOUND_MIXER_WRITE_ENHANCE               MIXER_WRITE(SOUND_MIXER_ENHANCE)
#define SOUND_MIXER_WRITE_LOUD                  MIXER_WRITE(SOUND_MIXER_LOUD)
#define SOUND_MIXER_WRITE_RECSRC                MIXER_WRITE(SOUND_MIXER_RECSRC)

/*********************************************************************************************************
  MIXER ioctl 获取 mixer 信息 (/dev/mixer)
  (SOUND_MIXER_DEVMASK 获取播放通道支持情况, SOUND_MIXER_RECMASK 获得录音通道支持情况)
*********************************************************************************************************/

#define SOUND_MASK_VOLUME                       (1 << SOUND_MIXER_VOLUME)
#define SOUND_MASK_BASS                         (1 << SOUND_MIXER_BASS)
#define SOUND_MASK_TREBLE                       (1 << SOUND_MIXER_TREBLE)
#define SOUND_MASK_SYNTH                        (1 << SOUND_MIXER_SYNTH)
#define SOUND_MASK_PCM                          (1 << SOUND_MIXER_PCM)
#define SOUND_MASK_SPEAKER                      (1 << SOUND_MIXER_SPEAKER)
#define SOUND_MASK_LINE                         (1 << SOUND_MIXER_LINE)
#define SOUND_MASK_MIC                          (1 << SOUND_MIXER_MIC)
#define SOUND_MASK_CD                           (1 << SOUND_MIXER_CD)
#define SOUND_MASK_IMIX                         (1 << SOUND_MIXER_IMIX)
#define SOUND_MASK_ALTPCM                       (1 << SOUND_MIXER_ALTPCM)
#define SOUND_MASK_RECLEV                       (1 << SOUND_MIXER_RECLEV)
#define SOUND_MASK_IGAIN                        (1 << SOUND_MIXER_IGAIN)
#define SOUND_MASK_OGAIN                        (1 << SOUND_MIXER_OGAIN)
#define SOUND_MASK_LINE1                        (1 << SOUND_MIXER_LINE1)
#define SOUND_MASK_LINE2                        (1 << SOUND_MIXER_LINE2)
#define SOUND_MASK_LINE3                        (1 << SOUND_MIXER_LINE3)
#define SOUND_MASK_DIGITAL1                     (1 << SOUND_MIXER_DIGITAL1)
#define SOUND_MASK_DIGITAL2                     (1 << SOUND_MIXER_DIGITAL2)
#define SOUND_MASK_DIGITAL3                     (1 << SOUND_MIXER_DIGITAL3)
#define SOUND_MASK_PHONEIN                      (1 << SOUND_MIXER_PHONEIN)
#define SOUND_MASK_PHONEOUT                     (1 << SOUND_MIXER_PHONEOUT)
#define SOUND_MASK_RADIO                        (1 << SOUND_MIXER_RADIO)
#define SOUND_MASK_VIDEO                        (1 << SOUND_MIXER_VIDEO)
#define SOUND_MASK_MONITOR                      (1 << SOUND_MIXER_MONITOR)

/*********************************************************************************************************
  MIXER 信息 (/dev/mixer)
*********************************************************************************************************/

#define SOUND_MIXER_INFO                        300                     /*  获取 MIXER 信息             */
#define SOUND_OLD_MIXER_INFO                    301                     /*  获取 MIXER 信息 (old)       */

typedef struct mixer_info {
    char        id[16];
    char        name[32];
    int         modify_counter;
    int         fillers[10];
} mixer_info;

typedef struct _old_mixer_info {
    char        id[16];
    char        name[32];
} _old_mixer_info;

/*********************************************************************************************************
  DSP 读写操作
*********************************************************************************************************/

#define DSP_WRITE_FLAG                          MIXER_WRITE_FLAG
#define DSP_READ_FLAG                           MIXER_READ_FLAG
#define DSP_WRITE(a)                            (a | DSP_WRITE_FLAG)
#define DSP_READ(a)                             (a | DSP_READ_FLAG)

/*********************************************************************************************************
  DSP 基本 ioctl 操作 (/dev/dsp)
*********************************************************************************************************/

#define SNDCTL_DSP_RESET                        0
#define SNDCTL_DSP_SYNC                         1
#define SNDCTL_DSP_SPEED                        2                       /*  采样频率                    */
#define SNDCTL_DSP_STEREO                       3                       /*  是否设置为立体声            */
#define SNDCTL_DSP_GETBLKSIZE                   4
#define SNDCTL_DSP_SAMPLESIZE                   SNDCTL_DSP_SETFMT
#define SNDCTL_DSP_CHANNELS                     6
#define SOUND_PCM_WRITE_CHANNELS	            SNDCTL_DSP_CHANNELS
#define SOUND_PCM_WRITE_FILTER                  7
#define SNDCTL_DSP_POST                         8
#define SNDCTL_DSP_SUBDIVIDE                    9
#define SNDCTL_DSP_SETFRAGMENT                  10

/*********************************************************************************************************
  Buffer status queries
*********************************************************************************************************/

typedef struct audio_buf_info {
    int     fragments;	                                                /* of avail. frags (partly used */
                                                                        /* ones not counted)            */
    int     fragstotal;	                                                /* Total of fragments allocated */
    int     fragsize;	                                                /* Size of a fragment in bytes  */

    int     bytes;	                                                    /* Avail. space in bytes includ-*/
                                                                        /* es partly used fragments     */
		                                                                /* Note! 'bytes' could be more  */
		                                                                /* than fragments*fragsize      */
} audio_buf_info;

/*********************************************************************************************************
  i/o space (arg: audio_buf_info)
*********************************************************************************************************/

#define SNDCTL_DSP_GETOSPACE	                12
#define SNDCTL_DSP_GETISPACE	                13

/*********************************************************************************************************
  SNDCTL_DSP_NONBLOCK is the same (but less powerful, since the
  action cannot be undone) of FIONBIO. The same can be achieved
  by opening the device with O_NDELAY
*********************************************************************************************************/

#define SNDCTL_DSP_NONBLOCK	                    14

/*********************************************************************************************************
  获取 CAPS 信息
*********************************************************************************************************/

#define SNDCTL_DSP_GETCAPS                      15

#define DSP_CAP_REVISION	                    0x000000ff              /* revision level (0 to 255)    */
#define DSP_CAP_DUPLEX		                    0x00000100              /* Full duplex record/playback  */
#define DSP_CAP_REALTIME	                    0x00000200              /* Real time capability         */
#define DSP_CAP_BATCH		                    0x00000400
    
/*********************************************************************************************************
  Device has some kind of internal buffers which may
  cause some delays and decrease precision of timing
*********************************************************************************************************/

#define DSP_CAP_COPROC		                    0x00000800

/*********************************************************************************************************
  Has a coprocessor, sometimes it's a DSP but usually not
*********************************************************************************************************/

#define DSP_CAP_TRIGGER		                    0x00001000              /* Supports SETTRIGGER          */
#define DSP_CAP_MMAP                            0x00002000              /* Supports mmap()              */

/*********************************************************************************************************
  trigger setting
*********************************************************************************************************/

#define SNDCTL_DSP_GETTRIGGER	                DSP_READ(16)
#define SNDCTL_DSP_SETTRIGGER	                DSP_WRITE(16)
#define PCM_ENABLE_INPUT	                    0x00000001
#define PCM_ENABLE_OUTPUT	                    0x00000002

/*********************************************************************************************************
  count_info
*********************************************************************************************************/

typedef struct count_info {
	int bytes;	                                                        /* Total # of bytes processed   */
	int blocks;	                                                        /* # of fragment transitions    */
	                                                                    /* since last time              */
	int ptr;	                                                        /* Current DMA pointer value    */
} count_info;

/*********************************************************************************************************
  buffmem_desc
*********************************************************************************************************/

typedef struct buffmem_desc {
    unsigned *buffer;
    int size;
} buffmem_desc;
        
#define SNDCTL_DSP_MAPINBUF                     _SIOR ('P', 19, buffmem_desc)
#define SNDCTL_DSP_MAPOUTBUF                    _SIOR ('P', 20, buffmem_desc)
#define SNDCTL_DSP_SETSYNCRO                    _SIO  ('P', 21)
#define SNDCTL_DSP_SETDUPLEX                    _SIO  ('P', 22)
#define SNDCTL_DSP_GETODELAY                    _SIOR ('P', 23, int)

/*********************************************************************************************************
  GETIPTR and GETISPACE are not that different... same for out.  (arg: count_info)
*********************************************************************************************************/

#define SNDCTL_DSP_GETIPTR	                    17
#define SNDCTL_DSP_GETOPTR	                    18

/*********************************************************************************************************
  DSP 采样格式 ioctl 操作 (/dev/dsp)
*********************************************************************************************************/

#define SNDCTL_DSP_GETFMTS                      11                      /*  获取支持的采样格式          */
#define SNDCTL_DSP_SETFMT                       5                       /*  设置一个采样格式            */

#define AFMT_QUERY                              0x00000000              /* Return current fmt           */
#define AFMT_MU_LAW                             0x00000001
#define AFMT_A_LAW                              0x00000002
#define AFMT_IMA_ADPCM                          0x00000004
#define AFMT_U8                                 0x00000008
#define AFMT_S16_LE                             0x00000010              /* Little endian signed 16      */
#define AFMT_S16_BE                             0x00000020              /* Big endian signed 16         */
#define AFMT_S8                                 0x00000040
#define AFMT_U16_LE                             0x00000080              /* Little endian U16            */
#define AFMT_U16_BE                             0x00000100              /* Big endian U16               */
#define AFMT_MPEG                               0x00000200              /* MPEG (2) audio               */
#define AFMT_AC3                                0x00000400              /* Dolby Digital AC3            */

#if __BYTE_ORDER == __BIG_ENDIAN
#define AFMT_S16_NE                             AFMT_S16_BE
#define AFMT_U16_NE                             AFMT_U16_BE
#else
#define AFMT_S16_NE                             AFMT_S16_LE
#define AFMT_U16_NE                             AFMT_U16_LE
#endif                                                                  /* __BYTE_ORDER == __BIG_ENDIAN */

/*********************************************************************************************************
  DSP 其他 ioctl 操作 (/dev/dsp)
*********************************************************************************************************/

#define SNDCTL_DSP_NONBLOCK                     14
#define SNDCTL_DSP_GETCAPS                      15

#define SOUND_PCM_READ_RATE                     (0x40000000 | 2)
#define SOUND_PCM_READ_CHANNELS                 (0x40000000 | 6)
#define SOUND_PCM_READ_BITS                     (0x40000000 | 5)
#define SOUND_PCM_READ_FILTER                   (0x40000000 | 7)

#define SOUND_PCM_WRITE_BITS                    SNDCTL_DSP_SETFMT
#define SOUND_PCM_WRITE_RATE                    SNDCTL_DSP_SPEED
#define SOUND_PCM_POST                          SNDCTL_DSP_POST
#define SOUND_PCM_RESET                         SNDCTL_DSP_RESET
#define SOUND_PCM_SYNC                          SNDCTL_DSP_SYNC
#define SOUND_PCM_SUBDIVIDE                     SNDCTL_DSP_SUBDIVIDE
#define SOUND_PCM_SETFRAGMENT                   SNDCTL_DSP_SETFRAGMENT
#define SOUND_PCM_GETFMTS                       SNDCTL_DSP_GETFMTS
#define SOUND_PCM_SETFMT                        SNDCTL_DSP_SETFMT
#define SOUND_PCM_NONBLOCK                      SNDCTL_DSP_NONBLOCK
#define SOUND_PCM_GETCAPS                       SNDCTL_DSP_GETCAPS

/*********************************************************************************************************
  ioctl calls to be used in communication with coprocessors and DSP chips.
*********************************************************************************************************/

typedef struct copr_buffer {
    int command;                                                        /* Set to 0 if not used         */
    int flags;
#define CPF_NONE           0x0000
#define CPF_FIRST          0x0001                                       /* First block                  */
#define CPF_LAST           0x0002                                       /* Last block                   */
    int len;
    int offs;                                                           /* If required by the device (0 if not used) */

    unsigned char data[4000];                                           /* NOTE! 4000 is not 4k         */
} copr_buffer;

typedef struct copr_debug_buf {
    int command;                                                        /* Used internally. Set to 0    */
    int parm1;
    int parm2;
    int flags;    
    int len;                                                            /* Length of data in bytes      */
} copr_debug_buf;

typedef struct copr_msg {
    int len;
    unsigned char data[4000];
} copr_msg;

#define SNDCTL_COPR_RESET               _SIO  ('C',  0)
#define SNDCTL_COPR_LOAD                _SIOWR('C',  1, copr_buffer)
#define SNDCTL_COPR_RDATA               _SIOWR('C',  2, copr_debug_buf)
#define SNDCTL_COPR_RCODE               _SIOWR('C',  3, copr_debug_buf)
#define SNDCTL_COPR_WDATA               _SIOW ('C',  4, copr_debug_buf)
#define SNDCTL_COPR_WCODE               _SIOW ('C',  5, copr_debug_buf)
#define SNDCTL_COPR_RUN                 _SIOWR('C',  6, copr_debug_buf)
#define SNDCTL_COPR_HALT                _SIOWR('C',  7, copr_debug_buf)
#define SNDCTL_COPR_SENDMSG             _SIOWR('C',  8, copr_msg)
#define SNDCTL_COPR_RCVMSG              _SIOR ('C',  9, copr_msg)

/*********************************************************************************************************
  ioctl timer
*********************************************************************************************************/

#define SNDCTL_TMR_TIMEBASE             _SIOWR('T', 1, int)
#define SNDCTL_TMR_START                _SIO  ('T', 2)
#define SNDCTL_TMR_STOP                 _SIO  ('T', 3)
#define SNDCTL_TMR_CONTINUE             _SIO  ('T', 4)
#define SNDCTL_TMR_TEMPO                _SIOWR('T', 5, int)
#define SNDCTL_TMR_SOURCE               _SIOWR('T', 6, int)
#  define TMR_INTERNAL             0x00000001
#  define TMR_EXTERNAL             0x00000002
#          define TMR_MODE_MIDI    0x00000010
#          define TMR_MODE_FSK     0x00000020
#          define TMR_MODE_CLS     0x00000040
#          define TMR_MODE_SMPTE   0x00000080
#define SNDCTL_TMR_METRONOME            _SIOW ('T', 7, int)
#define SNDCTL_TMR_SELECT               _SIOW ('T', 8, int)

/*********************************************************************************************************
  Volume mode decides how volumes are used
*********************************************************************************************************/

#define VOL_METHOD_ADAGIO         1
#define VOL_METHOD_LINEAR         2

/*********************************************************************************************************
  Note! SEQ_WAIT, SEQ_MIDIPUTC and SEQ_ECHO are used also as
        input events.
*********************************************************************************************************/
/*********************************************************************************************************
  Event codes 0xf0 to 0xfc are reserved for future extensions.
*********************************************************************************************************/

#define SEQ_FULLSIZE              0xfd                                  /* Long events                  */

/*********************************************************************************************************
 *      SEQ_FULLSIZE events are used for loading patches/samples to the
 *      synthesizer devices. These events are passed directly to the driver
 *      of the associated synthesizer device. There is no limit to the size
 *      of the extended events. These events are not queued but executed
 *      immediately when the write() is called (execution can take several
 *      seconds of time). 
 *
 *      When a SEQ_FULLSIZE message is written to the device, it must
 *      be written using exactly one write() call. Other events cannot
 *      be mixed to the same write.
 *      
 *      For FM synths (YM3812/OPL3) use struct sbi_instrument and write it to the 
 *      /dev/sequencer. Don't write other data together with the instrument structure
 *      Set the key field of the structure to FM_PATCH. The device field is used to
 *      route the patch to the corresponding device.
 *
 *      For wave table use struct patch_info. Initialize the key field
 *      to WAVE_PATCH.
*********************************************************************************************************/

#define SEQ_PRIVATE                0xfe                     /* Low level HW dependent events (8 bytes)  */
#define SEQ_EXTENDED               0xff                     /* Extended events (8 bytes) OBSOLETE       */

/*********************************************************************************************************
 * Record for FM patches
*********************************************************************************************************/

typedef unsigned char sbi_instr_data[32];

struct sbi_instrument {
    unsigned short key;                                     /* FM_PATCH or OPL3_PATCH                   */
#define FM_PATCH   _PATCHKEY(0x01)
#define OPL3_PATCH _PATCHKEY(0x03)
    short          device;                                  /* Synth# (0-4)                             */
    int            channel;                                 /* Program# to be initialized               */
    sbi_instr_data operators;                               /* Register settings for operator cells (.SBI format)      */
};

struct synth_info {                                         /* Read only                                */
    char   name[30];
    int    device;                                          /* 0-N. INITIALIZE BEFORE CALLING           */
    int    synth_type;
#define SYNTH_TYPE_FM               0
#define SYNTH_TYPE_SAMPLE           1
#define SYNTH_TYPE_MIDI             2                       /* Midi interface                           */

    int    synth_subtype;
#define FM_TYPE_ADLIB               0x00
#define FM_TYPE_OPL3                0x01
#define MIDI_TYPE_MPU401            0x401

#define SAMPLE_TYPE_BASIC           0x10
#define SAMPLE_TYPE_GUS             SAMPLE_TYPE_BASIC
#define SAMPLE_TYPE_WAVEFRONT       0x11

    int    perc_mode;                                       /* No longer supported                      */
    int    nr_voices;
    int    nr_drums;                                        /* Obsolete field                           */
    int    instr_bank_size;
    unsigned int  capabilities;  
#define SYNTH_CAP_PERCMODE          0x00000001              /* No longer used                           */
#define SYNTH_CAP_OPL3              0x00000002              /* Set if OPL3 supported                    */
#define SYNTH_CAP_INPUT             0x00000004              /* Input (MIDI) device                      */
    int    dummies[19];                                     /* Reserve space                            */
};

struct sound_timer_info {
    char name[32];
    int caps;
};

#define MIDI_CAP_MPU401            1                        /* MPU-401 intelligent mode                 */

struct midi_info {
    char           name[30];
    int            device;                                  /* 0-N. INITIALIZE BEFORE CALLING           */
    unsigned int  capabilities;                             /* To be defined later                      */
    int            dev_type;
    int            dummies[18];                             /* Reserve space                            */
};

/*********************************************************************************************************
 * ioctl commands for the /dev/midi##
*********************************************************************************************************/

typedef struct {
    unsigned char cmd;
    char nr_args, nr_returns;
    unsigned char data[30];
} mpu_command_rec;

#define SNDCTL_MIDI_PRETIME         _SIOWR('m', 0, int)
#define SNDCTL_MIDI_MPUMODE         _SIOWR('m', 1, int)
#define SNDCTL_MIDI_MPUCMD          _SIOWR('m', 2, mpu_command_rec)

/*********************************************************************************************************
 * ioctl commands for the seq
*********************************************************************************************************/

typedef struct synth_control {
        int devno;                                                      /* Synthesizer #                */
        char data[4000];                                                /* Device spesific command/data record */
}synth_control;

typedef struct remove_sample {
        int devno;                                                      /* Synthesizer #                */
        int bankno;                                                     /* MIDI bank # (0=General MIDI) */
        int instrno;                                                    /* MIDI instrument number       */
} remove_sample;

typedef struct seq_event_rec {
        unsigned char arr[8];
} seq_event_rec;

#define SNDCTL_SEQ_RESET                    _SIO  ('Q', 0)
#define SNDCTL_SEQ_SYNC                     _SIO  ('Q', 1)
#define SNDCTL_SYNTH_INFO                   _SIOWR('Q', 2, struct synth_info)
#define SNDCTL_SEQ_CTRLRATE                 _SIOWR('Q', 3, int)         /* Set/get timer resolution (HZ)*/
#define SNDCTL_SEQ_GETOUTCOUNT              _SIOR ('Q', 4, int)
#define SNDCTL_SEQ_GETINCOUNT               _SIOR ('Q', 5, int)
#define SNDCTL_SEQ_PERCMODE                 _SIOW ('Q', 6, int)
#define SNDCTL_FM_LOAD_INSTR                _SIOW ('Q', 7, struct sbi_instrument)
                                                                        /* Obsolete. Don't use!!!!!!    */
#define SNDCTL_SEQ_TESTMIDI                 _SIOW ('Q', 8, int)
#define SNDCTL_SEQ_RESETSAMPLES             _SIOW ('Q', 9, int)
#define SNDCTL_SEQ_NRSYNTHS                 _SIOR ('Q',10, int)
#define SNDCTL_SEQ_NRMIDIS                  _SIOR ('Q',11, int)
#define SNDCTL_MIDI_INFO                    _SIOWR('Q',12, struct midi_info)
#define SNDCTL_SEQ_THRESHOLD                _SIOW ('Q',13, int)
#define SNDCTL_SYNTH_MEMAVL                 _SIOWR('Q',14, int)         /* in=dev#, out=memsize         */
#define SNDCTL_FM_4OP_ENABLE                _SIOW ('Q',15, int)         /* in=dev#                      */
#define SNDCTL_SEQ_PANIC                    _SIO  ('Q',17)
#define SNDCTL_SEQ_OUTOFBAND                _SIOW ('Q',18, struct seq_event_rec)
#define SNDCTL_SEQ_GETTIME                  _SIOR ('Q',19, int)
#define SNDCTL_SYNTH_ID                     _SIOWR('Q',20, struct synth_info)
#define SNDCTL_SYNTH_CONTROL                _SIOWR('Q',21, struct synth_control)
#define SNDCTL_SYNTH_REMOVESAMPLE           _SIOWR('Q',22, struct remove_sample)

#endif                                                                  /*  __SOUNDCARD_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
