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
** 文   件   名: sound.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 07 月 05 日
**
** 描        述: 简易 OSS (open sound system) 的声卡操作.
** 注        意: 这里仅是一个设备驱动的接口规范, 所以不包含在 SylixOS.h 头文件中.
*********************************************************************************************************/

#ifndef __SOUND_H
#define __SOUND_H

/*********************************************************************************************************
  注册与卸载设备范例:
  
int  register_sound_mixer(...)
{
    int    devno;
    
    devno = iosDrvInstall(...);
    
    return  (iosDevAdd(..., "/dev/mixer", devno));
}

int  register_sound_dsp(...)
{
    int    devno;
    
    devno = iosDrvInstall(...);
    
    return  (iosDevAdd(..., "/dev/dsp", devno));
}

int  unregister_sound_mixer (...)
{
    int         devno;
    PLW_DEV_HDR pdev;
    
    pdev = iosFindDev("/dev/mixer", ...);
    if (pdev) {
        iosDevDelete(pdev);
        return  (iosDrvRemove(pdev->DEVHDR_usDrvNum, ...));
    } else {
        return  (-1);
    }
}

int  unregister_sound_dsp (...)
{
    int         devno;
    PLW_DEV_HDR pdev;
    
    pdev = iosFindDev("/dev/dsp", ...);
    if (pdev) {
        iosDevDelete(pdev);
        return  (iosDrvRemove(pdev->DEVHDR_usDrvNum, ...));
    } else {
        return  (-1);
    }
}
*********************************************************************************************************/

#endif                                                                  /*  __SOUND_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
