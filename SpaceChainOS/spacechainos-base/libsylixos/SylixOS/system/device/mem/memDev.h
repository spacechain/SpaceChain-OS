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
** 文   件   名: memdev.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 06 月 04 日
**
** 描        述: VxWorks 内存设备兼容接口.
*********************************************************************************************************/

#ifndef __MEMDEV_H
#define __MEMDEV_H

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_MEMDEV_EN > 0)

typedef struct mem_drv_direntry {                                       /*  兼容 VxWorks 结构           */
    char                    *name;                                      /*  名字                        */
    char                    *base;                                      /*  内存基地址                  */
    struct mem_drv_direntry *pDir;                                      /*  下一级节点                  */
    size_t                   length;                                    /*  内存长度                    */
} MEM_DRV_DIRENTRY;

/*********************************************************************************************************
  内核 API
*********************************************************************************************************/

LW_API INT  API_MemDrvInstall(void);
LW_API INT  API_MemDevCreate(char *name, char *base, size_t length);
LW_API INT  API_MemDevCreateDir(char *name, MEM_DRV_DIRENTRY *files, int numFiles);
LW_API INT  API_MemDevDelete(char *name);

#define memDrv              API_MemDrvInstall
#define memDevCreate        API_MemDevCreate
#define memDevCreateDir     API_MemDevCreateDir
#define memDevDelete        API_MemDevDelete

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_MEMDEV_EN > 0)      */
#endif                                                                  /*  __MEMDEV_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
