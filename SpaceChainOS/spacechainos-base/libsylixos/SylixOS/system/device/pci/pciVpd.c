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
** 文   件   名: pciVpd.c
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2015 年 09 月 17 日
**
** 描        述: PCI 总线 VPD (Vital Product Data).
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "pciLib.h"
#include "pciBus.h"
#include "pciDev.h"
#include "pciShow.h"
/*********************************************************************************************************
  vpd format
*********************************************************************************************************/
enum pci_vpd_format {
    F_BINARY,
    F_TEXT,
    F_RESVD,
    F_RDWR
};
/*********************************************************************************************************
  vpd item
*********************************************************************************************************/
static const struct pci_vpd_item {
    UINT8           id1, id2;
    UINT8           format;
    const PCHAR     name;
} _GpviVpdItems[] = {
    { 'C','P', F_BINARY,  "Extended capability" },
    { 'E','C', F_TEXT,    "Engineering changes" },
    { 'M','N', F_BINARY,  "Manufacture ID" },
    { 'P','N', F_TEXT,    "Part number" },
    { 'R','V', F_RESVD,   "Reserved" },
    { 'R','W', F_RDWR,    "Read-write area" },
    { 'S','N', F_TEXT,    "Serial number" },
    { 'Y','A', F_TEXT,    "Asset tag" },
    { 'V', 0 , F_TEXT,    "Vendor specific" },
    { 'Y', 0 , F_TEXT,    "System specific" },
    {  0,  0 , F_BINARY,  "Unknown" }
};
/*********************************************************************************************************
** 函数名称: __pciVpdStringPrint
** 功能描述: VPD 字符串打印.
** 输　入  : pucBuf      缓冲区
**           usLen       缓冲区大小
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciVpdStringPrint (const UINT8 *pucBuf, UINT16 usLen)
{
    while (usLen--) {
        UINT8   ucC = *pucBuf++;
        
        if (ucC == '\\') {
            printf("\\\\");
        
        } else if (!ucC && !usLen) {
            /*
             *  Cards with null-terminated strings have been observed
             */
        } else if ((ucC < 32) || (ucC == 127)) {
            printf("\\x%02x", ucC);
        
        } else {
            putchar(ucC);
        }
    }
}
/*********************************************************************************************************
** 函数名称: __pciVpdBinaryPrint
** 功能描述: VPD 二进制打印.
** 输　入  : buf         缓冲区
**           usLen       缓冲区大小
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciVpdBinaryPrint (const UINT8 *pucBuf, UINT16 usLen)
{
    REGISTER INT  i;

    for (i = 0; i < usLen; i++) {
        if (i) {
            putchar(' ');
        }
        printf("%02x", pucBuf[i]);
    }
}
/*********************************************************************************************************
** 函数名称: __pciVpdRead
** 功能描述: 读取 VPD 信息.
** 输　入  : iBus        总线号
**           iSlot       插槽
**           iFunc       功能
**           iPos        数据地址
**           pucBuf      结果缓冲区
**           iLen        结果缓冲区大小
**           pucCsum     检验和
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciVpdRead (INT    iBus,
                          INT    iSlot,
                          INT    iFunc,
                          INT    iPos,
                          UINT8 *pucBuf,
                          INT    iLen,
                          UINT8 *pucCsum)
{
    if (!API_PciVpdRead(iBus, iSlot, iFunc, iPos, pucBuf, iLen)) {
        return  (0);
    }

    while (iLen--) {
        *pucCsum += *pucBuf++;
    }

    return  (-1);
}
/*********************************************************************************************************
** 函数名称: API_PciCapVpdShow
** 功能描述: 打印扩展能力 VPD (Vital Product Data) 等.
** 输　入  : iBus        总线号
**           iSlot       插槽
**           iFunc       功能
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciCapVpdShow (INT iBus, INT iSlot, INT iFunc)
{

    UINT16      usResAddr = 0, usResLen, usPartPos, usPartLen;
    UINT8       ucBuf[256];
    UINT8       ucTag;
    UINT8       ucCsum = 0;

    printf("Vital Product Data\n");
    if (_G_iPciVerbose < 2) {
        return;
    }

    while (usResAddr <= PCI_VPD_ADDR_MASK) {
        if (!__pciVpdRead(iBus, iSlot, iFunc, usResAddr, &ucTag, 1, &ucCsum)) {
            break;
        }
        if (ucTag & 0x80) {
            if (usResAddr > PCI_VPD_ADDR_MASK + 1 - 3) {
                break;
            }
            if (!__pciVpdRead(iBus, iSlot, iFunc, usResAddr + 1, ucBuf, 2, &ucCsum)) {
                break;
            }
            usResLen = ucBuf[0] + (ucBuf[1] << 8);
            usResAddr += 3;
            
        } else {
            usResLen = ucTag & 7;
            ucTag >>= 3;
            usResAddr += 1;
        }
        if (usResLen > PCI_VPD_ADDR_MASK + 1 - usResAddr) {
            break;
        }
        usPartPos = 0;

        switch (ucTag) {
        
        case 0x0f:
            printf("\t\tEnd\n");
            return;

        case 0x82:
            printf("\t\tProduct Name: ");
            while (usPartPos < usResLen) {
                usPartLen = usResLen - usPartPos;
                if (usPartLen > sizeof(ucBuf)) {
                    usPartLen = sizeof(ucBuf);
                }
                if (!__pciVpdRead(iBus, iSlot, iFunc, usResAddr + usPartPos, ucBuf, usPartLen, &ucCsum)) {
                    break;
                }
                __pciVpdStringPrint(ucBuf, usPartLen);
                usPartPos += usPartLen;
            }
            printf("\n");
            break;

        case 0x90:
        case 0x91:
            printf("\t\t%s fields:\n",
                   (ucTag == 0x90) ? "Read-only" : "Read/write");

            while (usPartPos + 3 <= usResLen) {
                UINT16                     usReadLen;
                const struct pci_vpd_item *pviItem;
                UINT8                      ucId1, ucId2;

                if (!__pciVpdRead(iBus, iSlot, iFunc, usResAddr + usPartPos, ucBuf, 3, &ucCsum)) {
                    break;
                }
                usPartPos += 3;
                ucId1 = ucBuf[0];
                ucId2 = ucBuf[1];
                usPartLen = ucBuf[2];
                if (usPartLen > usResLen - usPartPos) {
                    break;
                }
                /* Is this item known? */
                for (pviItem = _GpviVpdItems;
                     (pviItem->id1 && pviItem->id1 != ucId1) ||
                     (pviItem->id2 && pviItem->id2 != ucId2);
                     pviItem++) {
                    ;
                }
                /*  Only read the first byte of the RV field because the
                 *  remaining bytes are not included in the checksum.
                 */
                usReadLen = (pviItem->format == F_RESVD) ? 1 : usPartLen;
                if (!__pciVpdRead(iBus, iSlot, iFunc, usResAddr + usPartPos, ucBuf, usReadLen, &ucCsum)) {
                    break;
                }
                printf("\t\t\t[%c%c] %s: ", ucId1, ucId2, pviItem->name);

                switch (pviItem->format) {
                
                case F_TEXT:
                    __pciVpdStringPrint(ucBuf, usPartLen);
                    printf("\n");
                    break;

                case F_BINARY:
                    __pciVpdBinaryPrint(ucBuf, usPartLen);
                    printf("\n");
                    break;

                case F_RESVD:
                    printf("checksum %s, %d byte(s) reserved\n", ucCsum ? "bad" : "good", usPartLen - 1);
                    break;

                case F_RDWR:
                    printf("%d byte(s) free\n", usPartLen);
                    break;
                }

                usPartPos += usPartLen;
            }
            break;

        default:
            printf("\t\tUnknown %s resource type %02x, will not decode more.\n",
                   (ucTag & 0x80) ? "large" : "small", ucTag & ~0x80);
            return;
        }

        usResAddr += usResLen;
    }

    if (usResAddr == 0) {
        printf("\t\tNot readable\n");
    
    } else {
        printf("\t\tNo end tag found\n");
    }
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
