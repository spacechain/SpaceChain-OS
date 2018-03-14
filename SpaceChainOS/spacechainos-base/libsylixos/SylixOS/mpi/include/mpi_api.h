/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: mpi_api.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 04 月 10 日
**
** 描        述: 这是系统多处理器应用层接口定义。
*********************************************************************************************************/

#ifndef  __MPI_API_H
#define  __MPI_API_H

#if LW_CFG_MPI_EN > 0
/*********************************************************************************************************
  双端口内存管理
*********************************************************************************************************/

LW_API LW_OBJECT_HANDLE  API_PortCreate(PCHAR          pcName,
                                        PVOID          pvInternalStart,
                                        PVOID          pvExternalStart,
                                        size_t         stByteLenght,
                                        LW_OBJECT_ID  *pulId);
                                        
LW_API ULONG             API_PortDelete(LW_OBJECT_HANDLE   *pulId);

LW_API ULONG             API_PortExToIn(LW_OBJECT_HANDLE   ulId,
                                        PVOID              pvExternal,
                                        PVOID             *ppvInternal);
                                        
LW_API ULONG             API_PortInToEx(LW_OBJECT_HANDLE   ulId,
                                        PVOID              pvInternal,
                                        PVOID             *ppvExternal);
                                        
LW_API ULONG             API_PortGetName(LW_OBJECT_HANDLE  ulId, PCHAR  pcName);

#endif                                                                  /*  LW_CFG_MPI_EN               */
#endif                                                                  /*  __MP_API_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
