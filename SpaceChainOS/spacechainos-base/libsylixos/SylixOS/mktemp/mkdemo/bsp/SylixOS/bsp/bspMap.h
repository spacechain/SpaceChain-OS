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
** 文   件   名: bspMap.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 12 月 23 日
**
** 描        述: C 程序入口部分. 物理分页空间与全局映射关系表定义.
*********************************************************************************************************/

#ifndef __BSPMAP_H
#define __BSPMAP_H

/*********************************************************************************************************
   内存分配关系图

    +-----------------------+--------------------------------+
    |       通用内存区      |          VMM 管理区            |
    |         CACHE         |                                |
    +-----------------------+--------------------------------+

*********************************************************************************************************/
/*********************************************************************************************************
  physical memory
*********************************************************************************************************/
#ifdef  __BSPINIT_MAIN_FILE

LW_MMU_PHYSICAL_DESC    _G_physicalDesc[] = {
    {                                                                   /*  中断向量表                  */
        BSP_CFG_RAM_BASE,
        0,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_VECTOR
    },

    {                                                                   /*  内核代码段                  */
        BSP_CFG_RAM_BASE,
        BSP_CFG_RAM_BASE,
        BSP_CFG_TEXT_SIZE,
        LW_PHYSICAL_MEM_TEXT
    },

    {                                                                   /*  内核数据段                  */
        BSP_CFG_RAM_BASE + BSP_CFG_TEXT_SIZE,
        BSP_CFG_RAM_BASE + BSP_CFG_TEXT_SIZE,
        BSP_CFG_DATA_SIZE,
        LW_PHYSICAL_MEM_DATA
    },

    {                                                                   /*  DMA 缓冲区                  */
        BSP_CFG_RAM_BASE + BSP_CFG_TEXT_SIZE + BSP_CFG_DATA_SIZE,
        BSP_CFG_RAM_BASE + BSP_CFG_TEXT_SIZE + BSP_CFG_DATA_SIZE,
        BSP_CFG_DMA_SIZE,
        LW_PHYSICAL_MEM_DMA
    },

    {                                                                   /*  APP 通用内存                */
        BSP_CFG_RAM_BASE + BSP_CFG_TEXT_SIZE + BSP_CFG_DATA_SIZE + BSP_CFG_DMA_SIZE,
        BSP_CFG_RAM_BASE + BSP_CFG_TEXT_SIZE + BSP_CFG_DATA_SIZE + BSP_CFG_DMA_SIZE,
        BSP_CFG_APP_SIZE,
        LW_PHYSICAL_MEM_APP
    },

    /*
     * TODO: 加入启动设备的寄存器映射, 参考代码如下:
     */
#if 0
    {
        0x20000000,
        0x20000000,
        LW_CFG_VMM_PAGE_SIZE,
        LW_PHYSICAL_MEM_BOOTSFR
    },
#endif

    {                                                                   /*  结束                        */
        0,
        0,
        0,
        0
    }
};
/*********************************************************************************************************
  virtual memory
*********************************************************************************************************/
LW_MMU_VIRTUAL_DESC    _G_virtualDesc[] = {
	/*
	 * TODO: 加入虚拟地址空间的定义, 参考代码如下:
	 */
#if 0
    {                                                                   /*  应用程序虚拟空间            */
        0x60000000,
        ((size_t)2 * LW_CFG_GB_SIZE),
        LW_VIRTUAL_MEM_APP
    },

    {
        0xe0000000,                                                     /*  ioremap 空间                */
        (256 * LW_CFG_MB_SIZE),
        LW_VIRTUAL_MEM_DEV
    },
#endif

    {                                                                   /*  结束                        */
        0,
        0,
        0
    }
};

#endif                                                                  /*  __BSPINIT_MAIN_FILE         */
#endif                                                                  /*  __BSPMAP_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
