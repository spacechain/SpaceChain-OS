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
** 文   件   名: yaffs_sylixosapi.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 12 月 07 日
**
** 描        述: yaffs api 函数接口.
*********************************************************************************************************/

#ifndef __YAFFS_SYLIXOSAPI_H
#define __YAFFS_SYLIXOSAPI_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_YAFFS_EN > 0)

/*********************************************************************************************************
  API 函数
*********************************************************************************************************/

LW_API INT      API_YaffsDrvInstall(VOID);
LW_API INT      API_YaffsDevCreate(PCHAR   pcName);
LW_API INT      API_YaffsDevDelete(PCHAR   pcName);
LW_API VOID     API_YaffsDevMountShow(VOID);

#define yaffsDrv            API_YaffsDrvInstall
#define yaffsDevCreate      API_YaffsDevCreate
#define yaffsDevDelete      API_YaffsDevDelete
#define yaffsDevMountShow   API_YaffsDevMountShow

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_YAFFS_EN > 0)       */
#endif                                                                  /*  __YAFFS_SYLIXOSAPI_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
