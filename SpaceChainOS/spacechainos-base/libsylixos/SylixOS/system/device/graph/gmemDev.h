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
** 文   件   名: gmemDev.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 03 月 28 日
**
** 描        述: 图形化设备模型. (由于图形操作需要足够快速度, 所以图形设备驱动将不穿越 IO 层)

** BUG:
2011.11.17  varinfo 中加入色彩位域信息.
2014.05.20  可以设置 varinfo.
*********************************************************************************************************/

#ifndef __GMEMDEV_H
#define __GMEMDEV_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_GRAPH_EN > 0

/*********************************************************************************************************
  位信息
*********************************************************************************************************/

typedef struct {
    UINT32             GMBF_uiOffset;                                   /* beginning of bitfield        */
    UINT32             GMBF_uiLength;                                   /* length of bitfield           */
    UINT32             GMBF_uiMsbRight;                                 /* !=0:Most significant bit is  */
                                                                        /* right                        */
} LW_GM_BITFIELD;

/*********************************************************************************************************
  显示信息
*********************************************************************************************************/

typedef struct {
    ULONG               GMVI_ulXRes;                                    /*  可视区域                    */
    ULONG               GMVI_ulYRes;
    
    ULONG               GMVI_ulXResVirtual;                             /*  虚拟区域                    */
    ULONG               GMVI_ulYResVirtual;
    
    ULONG               GMVI_ulXOffset;                                 /*  显示区域偏移                */
    ULONG               GMVI_ulYOffset;

    ULONG               GMVI_ulBitsPerPixel;                            /*  每个像素的数据位数          */
    ULONG               GMVI_ulBytesPerPixel;                           /*  每个像素的存储字节数        */
                                                                        /*  有些图形处理器 DMA 为了对齐 */
                                                                        /*  使用了填补无效字节          */
    ULONG               GMVI_ulGrayscale;                               /*  灰度等级                    */
    
    ULONG               GMVI_ulRedMask;                                 /*  红色掩码                    */
    ULONG               GMVI_ulGreenMask;                               /*  绿色掩码                    */
    ULONG               GMVI_ulBlueMask;                                /*  蓝色掩码                    */
    ULONG               GMVI_ulTransMask;                               /*  透明度掩码                  */
    
/*********************************************************************************************************
  0.9.9 227 后期加入
*********************************************************************************************************/
    LW_GM_BITFIELD      GMVI_gmbfRed;                                   /* bitfield in gmem (true color)*/
    LW_GM_BITFIELD      GMVI_gmbfGreen;
    LW_GM_BITFIELD      GMVI_gmbfBlue;
    LW_GM_BITFIELD      GMVI_gmbfTrans;
    
    BOOL                GMVI_bHardwareAccelerate;                       /*  是否使用硬件加速            */
    ULONG               GMVI_ulMode;                                    /*  显示模式                    */
    ULONG               GMVI_ulStatus;                                  /*  显示器状态                  */
} LW_GM_VARINFO;
typedef LW_GM_VARINFO  *PLW_GM_VARINFO;

typedef struct {
    PCHAR               GMSI_pcName;                                    /*  显示器名称                  */
    ULONG               GMSI_ulId;                                      /*  ID                          */
    size_t              GMSI_stMemSize;                                 /*  framebuffer 内存大小        */
    size_t              GMSI_stMemSizePerLine;                          /*  每一行的内存大小            */
    caddr_t             GMSI_pcMem;                                     /*  显示内存 (需要驱动程序映射) */
} LW_GM_SCRINFO;
typedef LW_GM_SCRINFO  *PLW_GM_SCRINFO;

/*********************************************************************************************************
  物理信息 (当 GMPHY_uiXmm 与 GMPHY_uiYmm 都为 0 时, GMPHY_uiDpi 必须有效)
*********************************************************************************************************/

typedef struct {
    UINT                GMPHY_uiXmm;                                    /*  横向毫米数                  */
    UINT                GMPHY_uiYmm;                                    /*  纵向毫米数                  */
    UINT                GMPHY_uiDpi;                                    /*  每英寸像素数                */
    ULONG               GMPHY_ulReserve[16];                            /*  保留                        */
} LW_GM_PHYINFO;
typedef LW_GM_PHYINFO  *PLW_GM_PHYINFO;

/*********************************************************************************************************
  标准屏幕命令 ioctl
*********************************************************************************************************/
#define LW_GM_GET_VARINFO   LW_OSIOR('g', 200, LW_GM_VARINFO)           /*  获得显示规格                */
#define LW_GM_SET_VARINFO   LW_OSIOW('g', 201, LW_GM_VARINFO)           /*  设置显示规格                */
#define LW_GM_GET_SCRINFO   LW_OSIOR('g', 202, LW_GM_SCRINFO)           /*  获得显示属性                */
#define LW_GM_GET_PHYINFO   LW_OSIOR('g', 203, LW_GM_PHYINFO)           /*  获得显示物理特征            */

/*********************************************************************************************************
  显示模式
*********************************************************************************************************/
#define LW_GM_MODE_PALETTE  0x80000000                                  /*  调色板模式掩码              */

#define LW_GM_GET_MODE      LW_OSIOR('g', 204, ULONG)                   /*  获取显示模式                */
#define LW_GM_SET_MODE      LW_OSIOD('g', 205, ULONG)                   /*  设置显示模式                */

/*********************************************************************************************************
  图形设备 file_operations
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct gmem_file_operations {
    FUNCPTR             GMFO_pfuncOpen;                                 /*  打开显示                    */
    FUNCPTR             GMFO_pfuncClose;                                /*  关闭显示                    */
    
    FUNCPTR             GMFO_pfuncIoctl;                                /*  设备控制                    */
    
    INT               (*GMFO_pfuncGetVarInfo)(LONG            lDev, 
                                              PLW_GM_VARINFO  pgmvi);   /*  获得 VARINFO                */
    INT               (*GMFO_pfuncSetVarInfo)(LONG                  lDev, 
                                              const PLW_GM_VARINFO  pgmvi);
                                                                        /*  设置 VARINFO                */
    INT               (*GMFO_pfuncGetScrInfo)(LONG            lDev, 
                                              PLW_GM_SCRINFO  pgmsi);   /*  获得 SCRINFO                */
    INT               (*GMFO_pfuncGetPhyInfo)(LONG            lDev, 
                                              PLW_GM_PHYINFO  pgmphy);  /*  获得 PHYINFO                */
    INT               (*GMFO_pfuncGetMode)(LONG     lDev, 
                                           ULONG   *pulMode);           /*  获取显示模式                */
    INT               (*GMFO_pfuncSetMode)(LONG     lDev, 
                                           ULONG    ulMode);            /*  设置显示模式                */
                                           
    INT               (*GMFO_pfuncSetPalette)(LONG     lDev,
                                              UINT     uiStart,
                                              UINT     uiLen,
                                              ULONG   *pulRed,
                                              ULONG   *pulGreen,
                                              ULONG   *pulBlue);        /*  设置调色板                  */
    INT               (*GMFO_pfuncGetPalette)(LONG     lDev,
                                              UINT     uiStart,
                                              UINT     uiLen,
                                              ULONG   *pulRed,
                                              ULONG   *pulGreen,
                                              ULONG   *pulBlue);        /*  获取调色板                  */
    /*
     *  2D 加速 (仅当 GMVI_bHardwareAccelerate 为 LW_TRUE 时有效)
     */
    INT               (*GMFO_pfuncSetPixel)(LONG     lDev, 
                                            INT      iX, 
                                            INT      iY, 
                                            ULONG    ulColor);          /*  绘制一个像素                */
    INT               (*GMFO_pfuncGetPixel)(LONG     lDev, 
                                            INT      iX, 
                                            INT      iY, 
                                            ULONG   *pulColor);         /*  获取一个像素                */
    INT               (*GMFO_pfuncSetColor)(LONG     lDev, 
                                            ULONG    ulColor);          /*  设置当前绘图前景色          */
    INT               (*GMFO_pfuncSetAlpha)(LONG     lDev, 
                                            ULONG    ulAlpha);          /*  设置当前绘图透明度          */
    INT               (*GMFO_pfuncDrawHLine)(LONG    lDev, 
                                             INT     iX0,
                                             INT     iY,
                                             INT     IX1);              /*  绘制一条水平线              */
    INT               (*GMFO_pfuncDrawVLine)(LONG    lDev, 
                                             INT     iX,
                                             INT     iY0,
                                             INT     IY1);              /*  绘制一条垂直线              */
    INT               (*GMFO_pfuncFillRect)(LONG    lDev, 
                                            INT     iX0,
                                            INT     iY0,
                                            INT     iX1,
                                            INT     iY1);               /*  填充区域                    */
} LW_GM_FILEOPERATIONS;
typedef LW_GM_FILEOPERATIONS    *PLW_GM_FILEOPERATIONS;

/*********************************************************************************************************
  图形设备控制块
*********************************************************************************************************/

typedef struct {
    LW_DEV_HDR                  GMDEV_devhdrHdr;                        /*  IO 设备头                   */
    PLW_GM_FILEOPERATIONS       GMDEV_gmfileop;                         /*  设备操作函数集              */
    ULONG                       GMDEV_ulMapFlags;                       /*  内存映射选项                */
    PVOID                       GMDEV_pvReserved[8];                    /*  保留配置字                  */
    /*
     * ... (设备相关信息, 此结构体作为所有绘图函数的第一个参数)
     */
} LW_GM_DEVICE;
typedef LW_GM_DEVICE           *PLW_GM_DEVICE;

/*********************************************************************************************************
  显示模式
*********************************************************************************************************/


LW_API INT              API_GMemDevAdd(CPCHAR  cpcName, PLW_GM_DEVICE  pgmdev);
LW_API PLW_GM_DEVICE    API_GMemGet2D(INT  iFd);
LW_API INT              API_GMemSetPalette(INT      iFd,
                                           UINT     uiStart,
                                           UINT     uiLen,
                                           ULONG   *pulRed,
                                           ULONG   *pulGreen,
                                           ULONG   *pulBlue);
LW_API INT              API_GMemGetPalette(INT      iFd,
                                           UINT     uiStart,
                                           UINT     uiLen,
                                           ULONG   *pulRed,
                                           ULONG   *pulGreen,
                                           ULONG   *pulBlue);
LW_API INT              API_GMemSetPixel(PLW_GM_DEVICE    gmdev,
                                         INT              iX, 
                                         INT              iY, 
                                         ULONG            ulColor);
LW_API INT              API_GMemGetPixel(PLW_GM_DEVICE    gmdev,
                                         INT              iX, 
                                         INT              iY, 
                                         ULONG           *pulColor);
LW_API INT              API_GMemSetColor(PLW_GM_DEVICE    gmdev,
                                         ULONG            ulColor);
LW_API INT              API_GMemSetAlpha(PLW_GM_DEVICE    gmdev,
                                         ULONG            ulAlpha);
LW_API INT              API_GMemDrawHLine(PLW_GM_DEVICE   gmdev,
                                          INT             iX0,
                                          INT             iY,
                                          INT             iX1);
LW_API INT              API_GMemDrawVLine(PLW_GM_DEVICE   gmdev,
                                          INT             iX,
                                          INT             iY0,
                                          INT             iY1);
LW_API INT              API_GMemFillRect(PLW_GM_DEVICE   gmdev,
                                         INT             iX0,
                                         INT             iY0,
                                         INT             iX1,
                                         INT             iY1);
                         
#define gmemDevAdd                      API_GMemDevAdd
#define gmemGet2d                       API_GMemGet2D
#define gmemSetPalette                  API_GMemSetPalette
#define gmemGetPalette                  API_GMemGetPalette
#define gmemSetPixel                    API_GMemSetPixel
#define gmemGetPixel                    API_GMemGetPixel
#define gmemSetColor                    API_GMemSetColor
#define gmemSetAlpha                    API_GMemSetAlpha
#define gmemDrawHLine                   API_GMemDrawHLine
#define gmemDrawVLine                   API_GMemDrawVLine
#define gmemFillRect                    API_GMemFillRect

#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  LW_CFG_GRAPH_EN             */
#endif                                                                  /*  __GMEMDEV_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
