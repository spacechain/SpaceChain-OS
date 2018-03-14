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
** 文   件   名: video.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2015 年 07 月 08 日
**
** 描        述: 标准视频采集驱动.
*********************************************************************************************************/

#ifndef __VIDEO_H
#define __VIDEO_H

#include "sys/types.h"
#include "sys/ioctl.h"

/*********************************************************************************************************
  video 接口说明:
  一个视频捕获接口, 可以有一个或者多个输入源, 例如很多视频采集卡可以有 4 路视频输入源, 每一个视频采集卡有
  很多视频采集与转换通道, 典型的视频采集卡有两个通道, 一个是用来显示的 RGB 通道, 另一个是用于数据压缩的
  YUV 格式通道. 每一个视频通道可以拥有一个或者多个内存缓冲 queue 构成 ping-pang 缓冲区, 视频采集卡循环的
  将视频数据放在连续的 ping-pang 缓冲区, 例如一个通道有 4 个 queue 缓冲区, 则视频采集卡会从 1 循环到 4 并
  不停的一帧一帧重复这个过程.
  
  实例代码:
  
  int  fd;
  int  i, j;
  void *pcapmem;
  
  video_dev_desc      dev;
  video_channel_desc  channel;
  video_format_desc   format;
  video_buf_cal       cal;
  video_buf_ctl       buf;
  video_cap_ctl       cap;
  
  fd = open("/dev/video0", O_RDWR);
  
  ioctl(fd, VIDIOC_DEVDESC, &dev);
  ...
  
  for (i = 0; i < dev.channels; i++) {
      channel.channel = i;
      ioctl(fd, VIDIOC_CHANDESC, &channel);
      ...
      for (j = 0; j < channel.formats; j++) {
          format.channel = i;
          format.index = j;
          ioctl(fd, VIDIOC_FORMATDESC, &format);
          ...
      }
  }
  
  channel.channel = 0;
  channel.xsize   = 640;
  channel.ysize   = 480;
  channel.x_off   = 0;
  channel.y_off   = 0;
  channel.queue   = 1;
  
  channel.source = 0;
  channel.format = VIDEO_PIXEL_FORMAT_RGBX_8888;
  channel.order  = VIDEO_LSB_CRCB;
  
  ioctl(fd, VIDIOC_SCHANCTL, &channel);
  ioctl(fd, VIDIOC_MAPCAL, &cal);
  
  buf.channel = 0;
  buf.mem     = NULL;
  buf.size    = cal.size;
  buf.mtype   = VIDEO_MEMORY_AUTO;
  
  ioctl(fd, VIDIOC_MAPPREPAIR, &buf);
  
  pcapmem = mmap(NULL, buf.size, PROT_READ, MAP_SHARED, fd, 0);
  
  如果使用 read() 调用, 则每次读取的数据都是最近有效的一帧数据.
  
  cap.channel = 0;
  cap.on      = 1;
  cap.flags   = 0;
  
  ioctl(fd, VIDEO_SCAPCTL, &cap);
  ...
  ...
  ...
  
  cap.on = 0;
  ioctl(fd, VIDEO_SCAPCTL, &cap);
  
  munmap(pcapmem, buf.size);
  
  close(fd);
*********************************************************************************************************/
/*********************************************************************************************************
  video ioctl command.
*********************************************************************************************************/

#define VIDIOC_DEVDESC          _IOR( 'v', 0, video_dev_desc)           /*  获得设备描述信息            */
#define VIDIOC_CHANDESC         _IOWR('v', 1, video_channel_desc)       /*  获得指定通道的信息          */
#define VIDIOC_FORMATDESC       _IOWR('v', 2, video_format_desc)        /*  获得指定通道支持的格式信息  */

/*********************************************************************************************************
  video channel control.
*********************************************************************************************************/

#define VIDIOC_GCHANCTL         _IOWR('v', 3, video_channel_ctl)        /*  获取通道控制字              */
#define VIDIOC_SCHANCTL         _IOW( 'v', 3, video_channel_ctl)        /*  设置通道控制字              */

/*********************************************************************************************************
  video prepair for mmap().
*********************************************************************************************************/

#define VIDIOC_MAPCAL           _IOWR('v', 4, video_buf_cal)            /*  读取内存需求情况            */
#define VIDIOC_MAPPREPAIR       _IOW( 'v', 4, video_buf_ctl)            /*  准备映射指定通道内存        */

/*********************************************************************************************************
  video capture status query
*********************************************************************************************************/

#define VIDIOC_CAPSTAT          _IOWR('v', 5, video_cap_stat)           /*  查询当前捕获信息            */

/*********************************************************************************************************
  video capture on/off.
*********************************************************************************************************/

#define VIDIOC_GCAPCTL          _IOWR('v', 10, video_cap_ctl)           /*  获得当前视频捕获状态        */
#define VIDIOC_SCAPCTL          _IOW( 'v', 10, video_cap_ctl)           /*  设置视频捕获状态 1:ON 0:OFF */

/*********************************************************************************************************
  video image effect.
*********************************************************************************************************/

#define VIDIOC_GEFFCTL          _IOR('v', 12, video_effect_info)        /*  获得图像特效信息            */
#define VIDIOC_SEFFCTL          _IOW('v', 12, video_effect_ctl)         /*  设置设置图像特效            */

/*********************************************************************************************************
  video memory format.
*********************************************************************************************************/

typedef enum {
    VIDEO_MEMORY_AUTO = 0,                                              /*  自动分配帧缓冲              */
    VIDEO_MEMORY_USER = 1                                               /*  用户分配帧缓冲              */
} video_mem_t;

/*********************************************************************************************************
  video CrCb order.
*********************************************************************************************************/

typedef enum {
    VIDEO_LSB_CRCB = 0,                                                 /*  低位在前                    */
    VIDEO_MSB_CRCB = 1                                                  /*  高位在前                    */
} video_order_t;

/*********************************************************************************************************
  video pixel format.
*********************************************************************************************************/

typedef enum {
    VIDEO_PIXEL_FORMAT_RESERVE = 0,
    /*
     *  RGB
     */
    VIDEO_PIXEL_FORMAT_RGBA_8888 = 1,
    VIDEO_PIXEL_FORMAT_RGBX_8888 = 2,
    VIDEO_PIXEL_FORMAT_RGB_888   = 3,
    VIDEO_PIXEL_FORMAT_RGB_565   = 4,
    VIDEO_PIXEL_FORMAT_BGRA_8888 = 5,
    VIDEO_PIXEL_FORMAT_RGBA_5551 = 6,
    VIDEO_PIXEL_FORMAT_RGBA_4444 = 7,
    /*
     *  0x8 ~ 0xF range reserve
     */
    VIDEO_PIXEL_FORMAT_YCbCr_422_SP = 0x10,                             /*  NV16                        */
    VIDEO_PIXEL_FORMAT_YCrCb_420_SP = 0x11,                             /*  NV21                        */
    VIDEO_PIXEL_FORMAT_YCbCr_422_P  = 0x12,                             /*  IYUV                        */
    VIDEO_PIXEL_FORMAT_YCbCr_420_P  = 0x13,                             /*  YUV9                        */
    VIDEO_PIXEL_FORMAT_YCbCr_422_I  = 0x14,                             /*  YUY2                        */
    /*
     *  0x15 reserve
     */
    VIDEO_PIXEL_FORMAT_CbYCrY_422_I = 0x16,
    /*
     *  0x17 0x18 ~ 0x1F range reserve
     */
    VIDEO_PIXEL_FORMAT_YCbCr_420_SP_TILED = 0x20,                       /*  NV12 tiled                  */
    VIDEO_PIXEL_FORMAT_YCbCr_420_SP       = 0x21,                       /*  NV12                        */
    VIDEO_PIXEL_FORMAT_YCrCb_420_SP_TILED = 0x22,                       /*  NV21 tiled                  */
    VIDEO_PIXEL_FORMAT_YCrCb_422_SP       = 0x23,                       /*  NV61                        */
    VIDEO_PIXEL_FORMAT_YCrCb_422_P        = 0x24                        /*  YV12                        */
} video_pixel_format;

/*********************************************************************************************************
  video format desc.
*********************************************************************************************************/

typedef struct video_format_desc {
    UINT32  channel;                                                    /*  指定的视频采集通道          */
    UINT32  index;                                                      /*  指定的序列编号              */
    
    CHAR    description[32];                                            /*  说明                        */
    UINT32  format;                                                     /*  帧格式 video_pixel_format   */
    UINT32  order;                                                      /*  MSB or LSB video_order_t    */
    
    UINT32  reserve[8];
} video_format_desc;

/*********************************************************************************************************
  video channel desc.
*********************************************************************************************************/

typedef struct video_channel_desc {
    UINT32  channel;                                                    /*  指定的视频采集通道          */
    
    CHAR    description[32];                                            /*  说明                        */
    UINT32  xsize_max;                                                  /*  最大尺寸                    */
    UINT32  ysize_max;
    UINT32  queue_max;                                                  /*  最大支持存储序列数          */
    
    UINT32  formats;                                                    /*  支持的视频采集格式个数      */
    UINT32  capabilities;                                               /*  具有的能力                  */
#define VIDEO_CHAN_ONESHOT      1                                       /*  仅采集一帧                  */
    
    UINT32  reserve[8];
} video_channel_desc;

/*********************************************************************************************************
  video device desc.
*********************************************************************************************************/

typedef struct video_dev_desc {
    CHAR    driver[32];
    CHAR    card[32];
    CHAR    bus[32];
    
    UINT32  version;                                                    /*  video 驱动版本              */
#define VIDEO_DRV_VERSION       1
    
    UINT32  capabilities;                                               /*  具有的能力                  */
#define VIDEO_CAP_CAPTURE       1                                       /*  视屏捕捉能力                */
#define VIDEO_CAP_READWRITE     2                                       /*  read/write 系统调用支持     */

    UINT32  sources;                                                    /*  视频源个数                  */
    UINT32  channels;                                                   /*  总采集通道数                */

    UINT32  reserve[8];
} video_dev_desc;

/*********************************************************************************************************
  video channel control.
*********************************************************************************************************/

typedef struct video_channel_ctl {
    UINT32  channel;                                                    /*  视频通道号                  */
    
    UINT32  xsize;                                                      /*  采集输出的尺寸              */
    UINT32  ysize;
    
    UINT32  x_off;                                                      /*  相对 size_max 采集起始偏移量*/
    UINT32  y_off;
    
    UINT32  x_cut;                                                      /*  相对 size_max 采集结束偏移量*/
    UINT32  y_cut;
    
    UINT32  queue;                                                      /*  采集序列数                  */
    
    UINT32  source;                                                     /*  指定的视频输入源            */
    UINT32  format;                                                     /*  帧格式 video_pixel_format   */
    UINT32  order;                                                      /*  MSB or LSB video_order_t    */
    
    UINT32  reserve[8];
} video_channel_ctl;

/*********************************************************************************************************
  video buffer calculate. (设置完 channel ctl 后由驱动计算内存需求信息)
*********************************************************************************************************/

typedef struct video_buf_cal {
    UINT32  channel;                                                    /*  视频通道号                  */
    
    size_t  align;                                                      /*  最小内存对齐要求            */
    size_t  size;                                                       /*  该通道缓冲内存总大小        */
    size_t  size_per_fq;                                                /*  队列中每一帧图像内存大小    */
    size_t  size_per_line;                                              /*  一帧图像中每一行内存大小    */

    UINT32  reserve[8];
} video_buf_cal;

/*********************************************************************************************************
  video buffer control. (如果指定为自动分配 mem size 均不关心)
*********************************************************************************************************/

typedef struct video_buf_ctl {
    UINT32  channel;                                                    /*  视频通道号                  */
    
    PVOID   mem;                                                        /*  帧缓冲区 (物理内存地址)     */
    size_t  size;                                                       /*  缓冲区大小                  */
    UINT32  mtype;                                                      /*  帧缓存类型 video_mem_t      */
    
    UINT32  reserve[8];
} video_buf_ctl;

/*********************************************************************************************************
  video capture control.
*********************************************************************************************************/

typedef struct video_cap_ctl {
    UINT32  channel;                                                    /*  视频通道号                  */
#define VIDEO_CAP_ALLCHANNEL    0xffffffff

    UINT32  on;                                                         /*  on / off                    */
    UINT32  flags;
#define VIDEO_CAP_ONESHOT       1                                       /*  仅采集一帧                  */
    
    UINT32  reserve[8];
} video_cap_ctl;

/*********************************************************************************************************
  video capture status query.
*********************************************************************************************************/

typedef struct video_cap_stat {
    UINT32  channel;                                                    /*  视频通道号                  */
    
    UINT32  on;                                                         /*  on / off                    */
    UINT32  qindex_vaild;                                               /*  最近一帧有效画面的队列号    */
    UINT32  qindex_cur;                                                 /*  正在采集的队列号            */
#define VIDEO_CAP_QINVAL        0xffffffff
    
    UINT32  reserve[8];
} video_cap_stat;

/*********************************************************************************************************
  video image effect control.
*********************************************************************************************************/

typedef enum {
    VIDEO_EFFECT_BRIGHTNESS         = 0,
    VIDEO_EFFECT_CONTRAST           = 1,
    VIDEO_EFFECT_SATURATION         = 2,
    VIDEO_EFFECT_HUE                = 3,
    VIDEO_EFFECT_BLACK_LEVEL        = 4,
    VIDEO_EFFECT_AUTO_WHITE_BALANCE = 5,
    VIDEO_EFFECT_DO_WHITE_BALANCE   = 6,
    VIDEO_EFFECT_RED_BALANCE        = 7,
    VIDEO_EFFECT_BLUE_BALANCE       = 8,
    VIDEO_EFFECT_GAMMA              = 9,
    VIDEO_EFFECT_WHITENESS          = 10,
    VIDEO_EFFECT_EXPOSURE           = 11,
    VIDEO_EFFECT_AUTOGAIN           = 12,
    VIDEO_EFFECT_GAIN               = 13,
    VIDEO_EFFECT_HFLIP              = 14,
    VIDEO_EFFECT_VFLIP              = 15,
} video_effect_id;

/*********************************************************************************************************
  video image effect control.
*********************************************************************************************************/

typedef struct video_effect_info {
    UINT32  effect_id;

    INT32   is_support;
    INT32   min_val;
    INT32   max_val;
    INT32   curr_val;

    UINT32  reserve[8];
} video_effect_info;

typedef struct video_effect_ctl {
    UINT32  effect_id;
    INT32   val;

    UINT32  reserve[8];
} video_effect_ctl;

#endif                                                                  /*  __VIDEO_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
