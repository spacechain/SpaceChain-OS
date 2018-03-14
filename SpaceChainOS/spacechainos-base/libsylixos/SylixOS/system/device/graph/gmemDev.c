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
** 文   件   名: gmemDev.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 03 月 28 日
**
** 描        述: 图形化设备模型. (由于图形操作需要足够快速度, 所以图形设备驱动将不穿越 IO 层)

** BUG:
2009.12.14  在 file_operations 中将符号链接函数指针清零.
2010.01.20  加入驱动程序信息.
2010.08.12  加入 FRAMEBUFFER 环境变量.
2010.09.11  创建设备时, 指定设备类型.
2011.06.03  环境变量支持保存功能, 所以创建 gmem 时不再需要设置 FRAMEBUFFER 环境变量.
2011.08.02  加入对 mmap 的支持.
2011.11.09  加入 GMFO_pfuncIoctl 接口, 以支持驱动程序的更多功能.
2014.07.05  加入 LW_GM_GET_PHYINFO 可以获取显示物理特征.
2014.09.29  允许驱动程序指定内存映射属性.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_GRAPH_EN > 0
/*********************************************************************************************************
  内部函数声明
*********************************************************************************************************/
static LONG  __gmemOpen(PLW_GM_DEVICE   pgmdev, INT  iFlag, INT  iMode);
static INT   __gmemClose(PLW_GM_DEVICE  pgmdev);
static INT   __gmemIoctl(PLW_GM_DEVICE  pgmdev, INT  iCommand, LONG  lArg);
static INT   __gmemMmap(PLW_GM_DEVICE  pgmdev, PLW_DEV_MMAP_AREA  pdmap);
/*********************************************************************************************************
  图形显示 IO 设备
*********************************************************************************************************/
struct file_operations  _G_foGMemDrv = {
    THIS_MODULE,
    __gmemOpen,
    LW_NULL,
    __gmemOpen,
    __gmemClose,
    LW_NULL,
    LW_NULL,
    LW_NULL,
    LW_NULL,
    __gmemIoctl,
    LW_NULL,
    LW_NULL,
    LW_NULL,
    LW_NULL,
    LW_NULL,
    LW_NULL,
    LW_NULL,
    __gmemMmap
};
/*********************************************************************************************************
** 函数名称: __gmemOpen
** 功能描述: 打开一个图形显示设备
** 输　入  : pgmdev        图形显示设备
**           iFlag         
**           iMode
** 输　出  : 图形显示设备
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  __gmemOpen (PLW_GM_DEVICE  pgmdev, INT  iFlag, INT  iMode)
{
    REGISTER INT    iError;
    
    if (LW_DEV_INC_USE_COUNT((PLW_DEV_HDR)pgmdev) == 1) {
        iError = pgmdev->GMDEV_gmfileop->GMFO_pfuncOpen(pgmdev, iFlag, iMode);
        if (iError < 0) {
            LW_DEV_DEC_USE_COUNT((PLW_DEV_HDR)pgmdev);
            return  (PX_ERROR);                                         /*  打开失败                    */
        }
    }
    
    return  ((LONG)pgmdev);
}
/*********************************************************************************************************
** 函数名称: __gmemClose
** 功能描述: 关闭一个图形显示设备
** 输　入  : pgmdev        图形显示设备
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __gmemClose (PLW_GM_DEVICE  pgmdev)
{
    if (LW_DEV_DEC_USE_COUNT((PLW_DEV_HDR)pgmdev) == 0) {
        return  (pgmdev->GMDEV_gmfileop->GMFO_pfuncClose(pgmdev));
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __gmemIoctl
** 功能描述: 控制一个图形显示设备
** 输　入  : pgmdev        图形显示设备
**           iCommand      命令
**           lArg          参数
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __gmemIoctl (PLW_GM_DEVICE  pgmdev, INT  iCommand, LONG  lArg)
{
LW_API time_t  API_RootFsTime(time_t  *time);

    REGISTER INT            iError = PX_ERROR;
             LW_GM_SCRINFO  scrinfo;
             struct stat   *pstat;

    switch (iCommand) {
    
    case FIOFSTATGET:
        pstat = (struct stat *)lArg;
        if (pgmdev->GMDEV_gmfileop->GMFO_pfuncGetScrInfo((LONG)pgmdev, &scrinfo) >= ERROR_NONE) {
            pstat->st_dev     = (dev_t)pgmdev;
            pstat->st_ino     = (ino_t)0;                               /*  相当于唯一节点              */
            pstat->st_mode    = 0666 | S_IFCHR;                         /*  默认属性                    */
            pstat->st_nlink   = 1;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 1;
            pstat->st_size    = scrinfo.GMSI_stMemSize;
            pstat->st_blksize = LW_CFG_VMM_PAGE_SIZE;
            pstat->st_blocks  = (blkcnt_t)(scrinfo.GMSI_stMemSize >> LW_CFG_VMM_PAGE_SHIFT);
            pstat->st_atime   = API_RootFsTime(LW_NULL);                /*  默认使用 root fs 基准时间   */
            pstat->st_mtime   = API_RootFsTime(LW_NULL);
            pstat->st_ctime   = API_RootFsTime(LW_NULL);
            
            iError = ERROR_NONE;
        }
        break;

    case LW_GM_GET_VARINFO:
        iError = pgmdev->GMDEV_gmfileop->GMFO_pfuncGetVarInfo((LONG)pgmdev, (PLW_GM_VARINFO)lArg);
        break;
        
    case LW_GM_SET_VARINFO:
        if (pgmdev->GMDEV_gmfileop->GMFO_pfuncSetVarInfo) {
            iError = pgmdev->GMDEV_gmfileop->GMFO_pfuncSetVarInfo((LONG)pgmdev, 
                                                                  (const PLW_GM_VARINFO)lArg);
        } else {
            _ErrorHandle(ENOSYS);
        }
        break;
        
    case LW_GM_GET_SCRINFO:
        iError = pgmdev->GMDEV_gmfileop->GMFO_pfuncGetScrInfo((LONG)pgmdev, (PLW_GM_SCRINFO)lArg);
        break;
        
    case LW_GM_GET_PHYINFO:
        if (pgmdev->GMDEV_gmfileop->GMFO_pfuncGetPhyInfo) {
            iError = pgmdev->GMDEV_gmfileop->GMFO_pfuncGetPhyInfo((LONG)pgmdev, (PLW_GM_PHYINFO)lArg);
        } else {
            _ErrorHandle(ENOSYS);
        }
        break;

    case LW_GM_GET_MODE:
        if (pgmdev->GMDEV_gmfileop->GMFO_pfuncGetMode) {
            iError = pgmdev->GMDEV_gmfileop->GMFO_pfuncGetMode((LONG)pgmdev, (ULONG *)lArg);
        } else {
            _ErrorHandle(ENOSYS);
        }
        break;

    case LW_GM_SET_MODE:
        if (pgmdev->GMDEV_gmfileop->GMFO_pfuncSetMode) {
            iError = pgmdev->GMDEV_gmfileop->GMFO_pfuncSetMode((LONG)pgmdev, (ULONG)lArg);
        } else {
            _ErrorHandle(ENOSYS);
        }
        break;
    
    default:
        if (pgmdev->GMDEV_gmfileop->GMFO_pfuncIoctl) {
            iError = pgmdev->GMDEV_gmfileop->GMFO_pfuncIoctl((LONG)pgmdev, iCommand, lArg);
        } else {
            _ErrorHandle(ENOSYS);
        }
    }
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __gmemMmap
** 功能描述: 将 fb 内存映射到虚拟内存空间
** 输　入  : pgmdev        图形显示设备
**           pdmap         虚拟空间信息
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT   __gmemMmap (PLW_GM_DEVICE  pgmdev, PLW_DEV_MMAP_AREA  pdmap)
{
#if LW_CFG_VMM_EN > 0
    LW_GM_SCRINFO   scrinfo;
    addr_t          ulPhysical;

    if (!pdmap) {
        return  (PX_ERROR);
    }
    
    if (pgmdev->GMDEV_gmfileop->GMFO_pfuncGetScrInfo((LONG)pgmdev, &scrinfo) < ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    ulPhysical  = (addr_t)(scrinfo.GMSI_pcMem);
    ulPhysical += (addr_t)(pdmap->DMAP_offPages << LW_CFG_VMM_PAGE_SHIFT);
    
    if (API_VmmRemapArea(pdmap->DMAP_pvAddr, (PVOID)ulPhysical, 
                         pdmap->DMAP_stLen, pgmdev->GMDEV_ulMapFlags, 
                         LW_NULL, LW_NULL)) {                           /*  将物理显存映射到虚拟内存    */
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
#else
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_VMM_EN               */
}
/*********************************************************************************************************
** 函数名称: API_GMemDevAdd
** 功能描述: 创建一个图形显示设备 (此设备会自动安装驱动程序)
** 输　入  : cpcName           图形设备名称
**           pgmdev            图形设备信息
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT   API_GMemDevAdd (CPCHAR  cpcName, PLW_GM_DEVICE  pgmdev)
{
    static   INT    iGMemDrvNum = PX_ERROR;
    
    if (iGMemDrvNum <= 0) {
        iGMemDrvNum  = iosDrvInstallEx(&_G_foGMemDrv);                  /*  安装驱动                    */
        if (iGMemDrvNum > 0) {
            DRIVER_LICENSE(iGMemDrvNum,     "GPL->Ver 2.0");
            DRIVER_AUTHOR(iGMemDrvNum,      "Han.hui");
            DRIVER_DESCRIPTION(iGMemDrvNum, "graph frame buffer driver.");
        }
    }
    
    if (!pgmdev || !cpcName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!pgmdev->GMDEV_gmfileop->GMFO_pfuncOpen       ||
        !pgmdev->GMDEV_gmfileop->GMFO_pfuncClose      ||
        !pgmdev->GMDEV_gmfileop->GMFO_pfuncGetVarInfo ||
        !pgmdev->GMDEV_gmfileop->GMFO_pfuncGetScrInfo) {
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    if (pgmdev->GMDEV_ulMapFlags == 0ul) {
#if LW_CFG_CACHE_EN > 0
        if (API_CacheGetMode(DATA_CACHE) & (CACHE_WRITETHROUGH | CACHE_SNOOP_ENABLE)) {
            pgmdev->GMDEV_ulMapFlags = LW_VMM_FLAG_RDWR;
        } else 
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
        {
            pgmdev->GMDEV_ulMapFlags = LW_VMM_FLAG_DMA;
        }
    }
    
    if (iosDevAddEx((PLW_DEV_HDR)pgmdev, cpcName, iGMemDrvNum, DT_CHR) != 
        ERROR_NONE) {                                                   /*  创建设备驱动                */
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_GMemGet2D
** 功能描述: 通过已经打开的图形设备获得 2D 加速绘图设备
** 输　入  : iFd           打开的文件描述符
** 输　出  : 2D 设备控制块
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PLW_GM_DEVICE  API_GMemGet2D (INT  iFd)
{
    REGISTER INT                iError;
             LW_GM_VARINFO      gmvi;
             PLW_GM_DEVICE      gmdev;
             
    iError = ioctl(iFd, LW_GM_GET_VARINFO, (LONG)&gmvi);                /*  获得显示设备信息            */
    if (iError < 0) {
        return  (LW_NULL);
    }
    
    if (gmvi.GMVI_bHardwareAccelerate) {                                /*  检测设备是否支持 2D 加速    */
        gmdev = (PLW_GM_DEVICE)API_IosFdValue(iFd);                     /*  获得设备控制块              */
        if (gmdev != (PLW_GM_DEVICE)PX_ERROR) {
            return  (gmdev);
        
        } else {
            _ErrorHandle(ENOSYS);
            return  (LW_NULL);
        }
    }
    
    _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                             /*  驱动程序不支持              */
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_GMemSetPalette
** 功能描述: 设置图形设备的调色板
** 输　入  : iFd           打开的文件描述符
**           uiStart       起始
**           uiLen         长度
**           pulRed        红色表
**           pulGreen      绿色表
**           pulBlue       蓝色表
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GMemSetPalette (INT      iFd,
                         UINT     uiStart,
                         UINT     uiLen,
                         ULONG   *pulRed,
                         ULONG   *pulGreen,
                         ULONG   *pulBlue)
{
    REGISTER INT                iError;
             PLW_GM_DEVICE      gmdev;
             
    gmdev = (PLW_GM_DEVICE)API_IosFdValue(iFd);                         /*  获得设备控制块              */
    if (gmdev != (PLW_GM_DEVICE)PX_ERROR) {
        iError = gmdev->GMDEV_gmfileop->GMFO_pfuncSetPalette((LONG)gmdev,
                                                             uiStart,
                                                             uiLen,
                                                             pulRed,
                                                             pulGreen,
                                                             pulBlue);
        return  (iError);
    
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                         /*  驱动程序不支持              */
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_GMemGetPalette
** 功能描述: 获取图形设备的调色板
** 输　入  : iFd           打开的文件描述符
**           uiStart       起始
**           uiLen         长度
**           pulRed        红色表
**           pulGreen      绿色表
**           pulBlue       蓝色表
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GMemGetPalette (INT      iFd,
                         UINT     uiStart,
                         UINT     uiLen,
                         ULONG   *pulRed,
                         ULONG   *pulGreen,
                         ULONG   *pulBlue)
{
    REGISTER INT                iError;
             PLW_GM_DEVICE      gmdev;
             
    gmdev = (PLW_GM_DEVICE)API_IosFdValue(iFd);                         /*  获得设备控制块              */
    if (gmdev != (PLW_GM_DEVICE)PX_ERROR) {
        iError = gmdev->GMDEV_gmfileop->GMFO_pfuncGetPalette((LONG)gmdev,
                                                             uiStart,
                                                             uiLen,
                                                             pulRed,
                                                             pulGreen,
                                                             pulBlue);
        return  (iError);
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                         /*  驱动程序不支持              */
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_GMemSetPixel
** 功能描述: 绘制一个像素点
** 输　入  : gmdev         图形设备
**           iX, iY        坐标
**           ulColor       色彩
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GMemSetPixel (PLW_GM_DEVICE    gmdev,
                       INT              iX, 
                       INT              iY, 
                       ULONG            ulColor)
{
    /*
     *  这里不进行指针有效性判断.
     */
    return  (gmdev->GMDEV_gmfileop->GMFO_pfuncSetPixel((LONG)gmdev, iX, iY, ulColor));
}
/*********************************************************************************************************
** 函数名称: API_GMemGetPixel
** 功能描述: 获取一个像素点
** 输　入  : gmdev         图形设备
**           iX, iY        坐标
**           pulColor      色彩
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GMemGetPixel (PLW_GM_DEVICE    gmdev,
                       INT              iX, 
                       INT              iY, 
                       ULONG           *pulColor)
{
    /*
     *  这里不进行指针有效性判断.
     */
    return  (gmdev->GMDEV_gmfileop->GMFO_pfuncGetPixel((LONG)gmdev, iX, iY, pulColor));
}
/*********************************************************************************************************
** 函数名称: API_GMemSetColor
** 功能描述: 设置当前绘图色彩
** 输　入  : gmdev         图形设备
**           ulColor       色彩
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GMemSetColor (PLW_GM_DEVICE    gmdev,
                       ULONG            ulColor)
{
    /*
     *  这里不进行指针有效性判断.
     */
    return  (gmdev->GMDEV_gmfileop->GMFO_pfuncSetColor((LONG)gmdev, ulColor));
}
/*********************************************************************************************************
** 函数名称: API_GMemSetAlpha
** 功能描述: 设置当前绘图透明度
** 输　入  : gmdev         图形设备
**           ulAlpha       透明度
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GMemSetAlpha (PLW_GM_DEVICE    gmdev,
                       ULONG            ulAlpha)
{
    /*
     *  这里不进行指针有效性判断.
     */
    return  (gmdev->GMDEV_gmfileop->GMFO_pfuncSetAlpha((LONG)gmdev, ulAlpha));
}
/*********************************************************************************************************
** 函数名称: API_GMemDrawHLine
** 功能描述: 绘制水平线
** 输　入  : gmdev         图形设备
**           iX0           起始 X 坐标
**           iY            Y 坐标
**           iX1           终值 X 坐标
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GMemDrawHLine (PLW_GM_DEVICE   gmdev,
                        INT             iX0,
                        INT             iY,
                        INT             iX1)
{
    /*
     *  这里不进行指针有效性判断.
     */
    return  (gmdev->GMDEV_gmfileop->GMFO_pfuncDrawHLine((LONG)gmdev, iX0, iY, iX1));
}
/*********************************************************************************************************
** 函数名称: API_GMemDrawVLine
** 功能描述: 绘制垂直线
** 输　入  : gmdev         图形设备
**           iX            X 坐标
**           iY0           起始 Y 坐标
**           iY1           终值 Y 坐标
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GMemDrawVLine (PLW_GM_DEVICE   gmdev,
                        INT             iX,
                        INT             iY0,
                        INT             iY1)
{
    /*
     *  这里不进行指针有效性判断.
     */
    return  (gmdev->GMDEV_gmfileop->GMFO_pfuncDrawVLine((LONG)gmdev, iX, iY0, iY1));
}
/*********************************************************************************************************
** 函数名称: API_GMemFillRect
** 功能描述: 填充区域
** 输　入  : gmdev         图形设备
**           iX0           起始 X 坐标
**           iY0           起始 Y 坐标
**           iX1           终值 X 坐标
**           iY1           终值 Y 坐标
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_GMemFillRect (PLW_GM_DEVICE   gmdev,
                       INT             iX0,
                       INT             iY0,
                       INT             iX1,
                       INT             iY1)
{
    /*
     *  这里不进行指针有效性判断.
     */
    return  (gmdev->GMDEV_gmfileop->GMFO_pfuncFillRect((LONG)gmdev, iX0, iY0, iX1, iY1));
}

#endif                                                                  /*  LW_CFG_GRAPH_EN             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
