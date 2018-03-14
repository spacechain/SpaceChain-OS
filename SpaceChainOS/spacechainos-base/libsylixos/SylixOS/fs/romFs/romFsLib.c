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
** 文   件   名: romFsLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 06 月 27 日
**
** 描        述: rom 文件系统 sylixos 内部函数. 
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "sys/endian.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0 && LW_CFG_ROMFS_EN > 0
#include "romFsLib.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define ROMFS_MAGIC_STR             "-rom1fs-"
#define ROMFS_MAGIC_0               ('-' << 24 | 'r' << 16 | 'o' << 8 | 'm')
#define ROMFS_MAGIC_1               ('1' << 24 | 'f' << 16 | 's' << 8 | '-')
#define ROMFS_BUFFER_SIZE           512

#define ROMFH_ALIGN                 16                                  /*  16 字节对齐                 */
#define ROMFH_ALIGN_SHIFT           4                                   /*  2 ^ 4 == 16                 */
/*********************************************************************************************************
  romfs 内文件类型
*********************************************************************************************************/
#define ROMFH_HRD                   0
#define ROMFH_DIR                   1
#define ROMFH_REG                   2
#define ROMFH_LNK                   3
#define ROMFH_BLK                   4
#define ROMFH_CHR                   5
#define ROMFH_SCK                   6
#define ROMFH_FIF                   7
#define ROMFH_EXEC                  8
/*********************************************************************************************************
  基本操作
*********************************************************************************************************/
#define __ROMFS_SECTOR_READ(promfs, sector) \
        (promfs)->ROMFS_pblkd->BLKD_pfuncBlkRd((promfs)->ROMFS_pblkd, \
                                               (promfs)->ROMFS_pcSector, \
                                               (sector), 1)
#define __ROMFS_SECTOR_BUFFER(promfs) \
        (promfs)->ROMFS_pcSector
        
#define __ROMFS_SECTOR_SIZE(promfs) \
        (UINT32)(promfs)->ROMFS_ulSectorSize
        
#define __ROMFS_SECTOR_SIZE_MASK(promfs) \
        (UINT32)((promfs)->ROMFS_ulSectorSize - 1)
/*********************************************************************************************************
  ROMFS 结构定义
*********************************************************************************************************/
typedef struct {
    UINT32      ROMFSSB_uiMagic0;
    UINT32      ROMFSSB_uiMagic1;
    UINT32      ROMFSSB_uiSize;
    UINT32      ROMFSSB_uiChecksum;
    CHAR        ROMFSSB_cName[1];
} ROMFS_SUPER_BLOCK;
typedef ROMFS_SUPER_BLOCK   *PROMFS_SUPER_BLOCK;

typedef struct {
    UINT32      ROMFSN_uiNext;
    UINT32      ROMFSN_uiSpec;
    UINT32      ROMFSN_uiSize;
    UINT32      ROMFSN_uiChecksum;
    CHAR        ROMFSN_cName[1];
} ROMFS_NODE;
typedef ROMFS_NODE          *PROMFS_NODE;
/*********************************************************************************************************
** 函数名称: __rfs_ntohl
** 功能描述: 等同于 ntohl
** 输　入  : uiData        转换的值
** 输　出  : 转换结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT32 __rfs_ntohl (UINT32 uiData)
{
#if BYTE_ORDER == LITTLE_ENDIAN
    return  ((uiData & 0xff) << 24) |
            ((uiData & 0xff00) << 8) |
            ((uiData & 0xff0000UL) >> 8) |
            ((uiData & 0xff000000UL) >> 24);
#else
    return  (uiData);
#endif
}
/*********************************************************************************************************
** 函数名称: __rfs_checksum
** 功能描述: romfs 计算 checksum 
** 输　入  : pvData           数据地址
**           iSize            校验大小
** 输　出  : 计算结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT32 __rfs_checksum (PVOID pvData, INT iSize)
{
    UINT32  uiSum;
    UINT32 *puiData;

    uiSum   = 0; 
    puiData = (UINT32 *)pvData;
    
    iSize >>= 2;
    while (iSize > 0) {
        uiSum += __rfs_ntohl(*puiData);
        puiData++;
        iSize--;
    }
    
    return  (uiSum);
}
/*********************************************************************************************************
** 函数名称: __rfs_getsector
** 功能描述: romfs 将指定扇区数据装入缓冲
** 输　入  : promfs           文件系统
**           ulSectorNo       扇区号
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __rfs_getsector (PROM_VOLUME  promfs, ULONG  ulSectorNo)
{
    if (promfs->ROMFS_ulCurSector == ulSectorNo) {
        return  (ERROR_NONE);
    }
    
    if (ulSectorNo >= promfs->ROMFS_ulSectorNum) {
        return  (PX_ERROR);
    }
    
    if (__ROMFS_SECTOR_READ(promfs, ulSectorNo) < ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    promfs->ROMFS_ulCurSector = ulSectorNo;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __rfs_getaddr
** 功能描述: romfs 将 addr 指定地址的扇区装入缓冲
** 输　入  : promfs           文件系统
**           uiAddr           地址
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __rfs_getaddr (PROM_VOLUME  promfs, UINT32  uiAddr)
{
    ULONG  ulSectorNo = uiAddr / promfs->ROMFS_ulSectorSize;
    
    if (promfs->ROMFS_ulCurSector == ulSectorNo) {                      /*  快速判断                    */
        return  (ERROR_NONE);
    }

    return  (__rfs_getsector(promfs, ulSectorNo));
}
/*********************************************************************************************************
** 函数名称: __rfs_bufaddr
** 功能描述: romfs 获得指定 romfs 地址的数据缓冲地址
** 输　入  : promfs           文件系统
**           uiAddr           地址
** 输　出  : 数据缓冲地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PCHAR __rfs_bufaddr (PROM_VOLUME  promfs, UINT32  uiAddr)
{
    if (__rfs_getaddr(promfs, uiAddr)) {                                /*  获得指定地址扇区            */
        return  (LW_NULL);
    }

    return  (&__ROMFS_SECTOR_BUFFER(promfs)[uiAddr % promfs->ROMFS_ulSectorSize]);
}
/*********************************************************************************************************
** 函数名称: __rfs_memcpy
** 功能描述: romfs 从文件系统设备指定地址开始拷贝 uiSize 个数据
** 输　入  : promfs           文件系统
**           pcDest           拷贝目的
**           uiAddr           romfs 内部地址
**           uiSize           需要拷贝的大小
** 输　出  : 实际拷贝的数据数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t __rfs_memcpy (PROM_VOLUME  promfs, PCHAR  pcDest, UINT32  uiAddr, UINT32  uiSize)
{
    UINT        uiTimes;
    UINT        uiLefts;
    PCHAR       pcSrc;
    INT         i;
    ssize_t     sstNum = 0;
    
    if (uiAddr & __ROMFS_SECTOR_SIZE_MASK(promfs)) {
        UINT32  uiAddrSector = ROUND_UP(uiAddr, __ROMFS_SECTOR_SIZE(promfs));
        UINT32  uiLen        = uiAddrSector - uiAddr;
        
        uiLen = __MIN(uiLen, uiSize);
        pcSrc = __rfs_bufaddr(promfs, uiAddr);
        if (!pcSrc) {
            return  (sstNum);
        }
        lib_memcpy(pcDest, pcSrc, uiLen);
        
        pcDest += uiLen;
        sstNum += uiLen;
        uiAddr += uiLen;
        uiSize -= uiLen;
    }
    
    if (uiSize == 0) {
        return  (sstNum);                                               /*  拷贝结束                    */
    }
    
    uiTimes = uiSize / __ROMFS_SECTOR_SIZE(promfs);
    uiLefts = uiSize & __ROMFS_SECTOR_SIZE_MASK(promfs);
    
    for (i = 0; i < uiTimes; i++) {
        pcSrc = __rfs_bufaddr(promfs, uiAddr);
        if (!pcSrc) {
            return  (sstNum);
        }
        lib_memcpy(pcDest, pcSrc, (size_t)__ROMFS_SECTOR_SIZE(promfs));
        
        uiAddr += __ROMFS_SECTOR_SIZE(promfs);
        pcDest += __ROMFS_SECTOR_SIZE(promfs);
        sstNum += __ROMFS_SECTOR_SIZE(promfs);
    }
    
    if (uiLefts) {
        pcSrc = __rfs_bufaddr(promfs, uiAddr);
        if (!pcSrc) {
            return  (sstNum);
        }
        lib_memcpy(pcDest, pcSrc, uiLefts);
        
        sstNum += uiLefts;
    }
    
    return  (sstNum);
}
/*********************************************************************************************************
** 函数名称: __rfs_strncpy
** 功能描述: romfs 从文件系统设备指定地址开始拷贝字符串
** 输　入  : promfs           文件系统
**           pcDest           拷贝目的
**           uiAddr           romfs 内部地址
**           stMax            缓冲区最大长度
** 输　出  : 字符串长度
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t __rfs_strncpy (PROM_VOLUME  promfs, PCHAR  pcDest, UINT32  uiAddr, size_t  stMax)
{
    UINT        uiTimes;
    UINT        uiLefts;
    PCHAR       pcSrc;
    INT         i;
    ssize_t     sstNum = 0;

    if (uiAddr & __ROMFS_SECTOR_SIZE_MASK(promfs)) {
        UINT32  uiAddrSector = ROUND_UP(uiAddr, __ROMFS_SECTOR_SIZE(promfs));
        UINT32  uiLen        = uiAddrSector - uiAddr;
        
        uiLen = __MIN(uiLen, stMax);
        
        pcSrc = __rfs_bufaddr(promfs, uiAddr);
        if (!pcSrc) {
            return  (sstNum);
        }
        
        for (i = 0; i < uiLen; i++) {
            if (*pcSrc) {
                *pcDest++ = *pcSrc++;
                sstNum++;
            } else {
                *pcDest = *pcSrc;
                return  (sstNum);
            }
        }
        
        stMax  -= uiLen;
        uiAddr += uiLen;
    }
    
    if (stMax == 0) {
        return  (sstNum);
    }
    
    uiTimes = stMax / __ROMFS_SECTOR_SIZE(promfs);
    uiLefts = stMax & __ROMFS_SECTOR_SIZE_MASK(promfs);
    
    for (i = 0; i < uiTimes; i++) {
        pcSrc = __rfs_bufaddr(promfs, uiAddr);
        if (!pcSrc) {
            return  (sstNum);
        }
        
        for (i = 0; i < __ROMFS_SECTOR_SIZE(promfs); i++) {
            if (*pcSrc) {
                *pcDest++ = *pcSrc++;
                sstNum++;
            } else {
                *pcDest = *pcSrc;
                return  (sstNum);
            }
        }
        
        uiAddr += __ROMFS_SECTOR_SIZE(promfs);
    }
    
    if (uiLefts) {
        pcSrc = __rfs_bufaddr(promfs, uiAddr);
        if (!pcSrc) {
            return  (sstNum);
        }
        
        for (i = 0; i < uiLefts; i++) {
            if (*pcSrc) {
                *pcDest++ = *pcSrc++;
                sstNum++;
            } else {
                *pcDest = *pcSrc;
                return  (sstNum);
            }
        }
    }
    
    return  (sstNum);
}
/*********************************************************************************************************
** 函数名称: __rfs_mount
** 功能描述: romfs mount 操作
** 输　入  : promfs           文件系统
** 输　出  : 是否成功 mount
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  __rfs_mount (PROM_VOLUME  promfs)
{
    PROMFS_SUPER_BLOCK   prsb;
    size_t               stVolNameLen;
    UINT                 uiSize;
    
    if (__rfs_getsector(promfs, 0)) {                                   /*  获得 0 扇区                 */
        return  (PX_ERROR);
    }
    
    prsb = (PROMFS_SUPER_BLOCK)__ROMFS_SECTOR_BUFFER(promfs);           /*  promfs 缓冲区字对齐         */
    
    if ((prsb->ROMFSSB_uiMagic0 != __rfs_ntohl(ROMFS_MAGIC_0)) || 
        (prsb->ROMFSSB_uiMagic1 != __rfs_ntohl(ROMFS_MAGIC_1))) {       /*  查看魔数                    */
        return  (PX_ERROR);
    }
    
    uiSize = __rfs_ntohl(prsb->ROMFSSB_uiSize);
    if (__rfs_checksum(prsb, __MIN(uiSize, 512))) {                     /*  校验错误                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "checksum error.\r\n");
        return  (PX_ERROR);
    }
    
    stVolNameLen = lib_strnlen(prsb->ROMFSSB_cName, (size_t)(promfs->ROMFS_ulSectorSize - 16));
    if (stVolNameLen >= promfs->ROMFS_ulSectorSize) {                   /*  卷标名过长                  */
        return  (PX_ERROR);
    }
    
    if (ALIGNED(stVolNameLen, ROMFH_ALIGN)) {
        promfs->ROMFS_uiRootAddr = (UINT32)(stVolNameLen + 16) + 16;
    
    } else {
        promfs->ROMFS_uiRootAddr = (UINT32)ROUND_UP(stVolNameLen, ROMFH_ALIGN) + 16;
    }
    
    promfs->ROMFS_uiTotalSize = __rfs_ntohl(prsb->ROMFSSB_uiSize);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __rfs_getfile
** 功能描述: romfs 获得指定地址对应的文件
** 输　入  : promfs           文件系统
**           uiAddr           设备地址
**           promdnt          文件信息
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  __rfs_getfile (PROM_VOLUME  promfs, UINT32  uiAddr, PROMFS_DIRENT promdnt)
{
    PROMFS_NODE     prnode;
    UINT32          uiAddrName;
    UINT32          uiAddrData;
    
    UINT32          uiNext;
    UINT32          uiSpec;
    UINT32          uiSize;
    UINT            uiTemp;
    ssize_t         sstNameLen;
    
    prnode = (PROMFS_NODE)__rfs_bufaddr(promfs, uiAddr);                /*  promfs 缓冲区字对齐         */
    if (prnode == LW_NULL) {
        return  (PX_ERROR);
    }
    
    promdnt->ROMFSDNT_uiMe = uiAddr;                                    /*  记录自己的位置              */
    
    uiNext = __rfs_ntohl(prnode->ROMFSN_uiNext);
    uiSpec = __rfs_ntohl(prnode->ROMFSN_uiSpec);
    uiSize = __rfs_ntohl(prnode->ROMFSN_uiSize);
    
    uiAddrName = uiAddr + 16;                                           /*  文件名起始地址              */
    sstNameLen = __rfs_strncpy(promfs, promdnt->ROMFSDNT_cName, uiAddrName, MAX_FILENAME_LENGTH);
    if (sstNameLen <= 0) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    if (sstNameLen >= MAX_FILENAME_LENGTH) {
        _ErrorHandle(ENAMETOOLONG);
        return  (PX_ERROR);
    }
    
    if (ALIGNED(sstNameLen, ROMFH_ALIGN)) {
        uiAddrData = (UINT32)((uiAddrName + sstNameLen) + 16);
        
    } else {
        uiAddrData = (UINT32)ROUND_UP((uiAddrName + sstNameLen), ROMFH_ALIGN);
    }
    
    uiTemp = uiNext & 0xF;
    promdnt->ROMFSDNT_stat.st_mode = 0;
    if (uiTemp & ROMFH_EXEC) {
        uiTemp &= (~ROMFH_EXEC);
        promdnt->ROMFSDNT_stat.st_mode = S_IXUSR | S_IXGRP | S_IXOTH;
    }
    
    switch (uiTemp) {
    
    case ROMFH_HRD:
        promdnt->ROMFSDNT_stat.st_mode |= S_IFLNK;
        break;
        
    case ROMFH_DIR:
        promdnt->ROMFSDNT_stat.st_mode |= S_IFDIR;
        break;
        
    case ROMFH_REG:
        promdnt->ROMFSDNT_stat.st_mode |= S_IFREG;
        break;
        
    case ROMFH_LNK:
        promdnt->ROMFSDNT_stat.st_mode |= S_IFLNK;
        break;
        
    case ROMFH_BLK:
        promdnt->ROMFSDNT_stat.st_mode |= S_IFBLK;
        break;
        
    case ROMFH_CHR:
        promdnt->ROMFSDNT_stat.st_mode |= S_IFCHR;
        break;
        
    case ROMFH_SCK:
        promdnt->ROMFSDNT_stat.st_mode |= S_IFSOCK;
        break;
        
    case ROMFH_FIF:
        promdnt->ROMFSDNT_stat.st_mode |= S_IFIFO;
        break;
        
    default:
        promdnt->ROMFSDNT_stat.st_mode |= S_IFCHR;
        break;
    }
    
    promdnt->ROMFSDNT_stat.st_dev     = (dev_t)promfs;
    promdnt->ROMFSDNT_stat.st_ino     = (ino_t)promdnt->ROMFSDNT_uiMe;
    promdnt->ROMFSDNT_stat.st_mode   |= S_IRUSR | S_IRGRP | S_IROTH;
    promdnt->ROMFSDNT_stat.st_nlink   = 1;
    promdnt->ROMFSDNT_stat.st_uid     = promfs->ROMFS_uid;
    promdnt->ROMFSDNT_stat.st_gid     = promfs->ROMFS_gid;
    promdnt->ROMFSDNT_stat.st_rdev    = 1;
    promdnt->ROMFSDNT_stat.st_size    = (off_t)uiSize;
    promdnt->ROMFSDNT_stat.st_blksize = uiSize;
    promdnt->ROMFSDNT_stat.st_blocks  = 1;
    promdnt->ROMFSDNT_stat.st_atime   = promfs->ROMFS_time;
    promdnt->ROMFSDNT_stat.st_mtime   = promfs->ROMFS_time;
    promdnt->ROMFSDNT_stat.st_ctime   = promfs->ROMFS_time;
    
    promdnt->ROMFSDNT_uiNext = uiNext & (~0xF);
    promdnt->ROMFSDNT_uiSpec = uiSpec;
    promdnt->ROMFSDNT_uiData = uiAddrData;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __rfs_lookup
** 功能描述: romfs 在指定 uiAddr 文件夹下, 获取指定文件名对应的文件
** 输　入  : promfs           文件系统
**           uiAddr           设备地址
**           pcName           文件名
**           promdnt          文件信息
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  __rfs_lookup (PROM_VOLUME  promfs, UINT32  uiAddr, PCHAR  pcName, PROMFS_DIRENT promdnt)
{
    do {
        if (uiAddr == 0) {
            return  (PX_ERROR);
        }
        
        if (__rfs_getfile(promfs, uiAddr, promdnt)) {
            return  (PX_ERROR);
        }
        
        if (lib_strcmp(pcName, promdnt->ROMFSDNT_cName) == 0) {
            break;
        }
        
        uiAddr = promdnt->ROMFSDNT_uiNext;
    } while (1);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __rfs_pread
** 功能描述: romfs 读取一个文件
** 输　入  : promfs           文件系统
**           promdnt          文件
**           pcDest           目标缓冲
**           stSize           读取字节数
**           uiOffset         起始偏移量
** 输　出  : 读取结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ssize_t  __rfs_pread (PROM_VOLUME  promfs, PROMFS_DIRENT promdnt, 
                      PCHAR  pcDest, size_t stSize, UINT32  uiOffset)
{
    UINT32      uiReadSize = (UINT32)__MIN((promdnt->ROMFSDNT_stat.st_size - uiOffset), stSize); 
    
    if (uiOffset >= promdnt->ROMFSDNT_stat.st_size) {
        return  (0);
    }
    
    return  (__rfs_memcpy(promfs, pcDest, 
                          promdnt->ROMFSDNT_uiData + uiOffset, 
                          uiReadSize));
}
/*********************************************************************************************************
** 函数名称: __rfs_readlink
** 功能描述: romfs 读取一个连接文件
** 输　入  : promfs           文件系统
**           promdnt          文件
**           pcDest           目标缓冲
**           stSize           读取字节数
**           offt             起始偏移量
** 输　出  : 读取结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ssize_t  __rfs_readlink (PROM_VOLUME  promfs, PROMFS_DIRENT promdnt, PCHAR  pcDest, size_t stSize)
{
    UINT32      uiReadSize = stSize;
    
    return  (__rfs_strncpy(promfs, pcDest, promdnt->ROMFSDNT_uiData, uiReadSize));
}
/*********************************************************************************************************
** 函数名称: __rfs_open
** 功能描述: romfs 打开一个文件
** 输　入  : promfs           文件系统
**           pcName           文件名
**           ppcTail          如果存在连接文件, 指向连接文件后的路径
**           ppcSymfile       连接文件
**           promdnt          文件结构(获取)
** 输　出  : 打开结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  __rfs_open (PROM_VOLUME  promfs, CPCHAR  pcName, 
                 PCHAR  *ppcTail, PCHAR  *ppcSymfile, PROMFS_DIRENT  promdnt)
{
    CHAR        cNameBuffer[PATH_MAX + 1];
    PCHAR       pcNameStart;
    PCHAR       pcPtr = cNameBuffer;
    
    UINT32      uiAddr = promfs->ROMFS_uiRootAddr;
    
    if (__STR_IS_ROOT(pcName)) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    lib_strlcpy(cNameBuffer, pcName, PATH_MAX + 1);
    if (*pcPtr) {
        pcPtr++;
    }
    
    while (*pcPtr) {
        pcNameStart = pcPtr;
        
        while (*pcPtr && *pcPtr != PX_DIVIDER) {
            pcPtr++;
        }
        if (*pcPtr != 0) {
            *pcPtr++ = 0;
        }
        
        if (__rfs_lookup(promfs, uiAddr, pcNameStart, promdnt)) {
            _ErrorHandle(ENOENT);
            return  (PX_ERROR);
        }
        
        if (S_ISLNK(promdnt->ROMFSDNT_stat.st_mode)) {
            *ppcTail    = (char *)(pcName + (pcPtr - cNameBuffer));
            *ppcSymfile = (char *)(pcName + (pcNameStart - cNameBuffer));/* point to symlink file name  */
            return  (ERROR_NONE);
        
        } else if (!S_ISDIR(promdnt->ROMFSDNT_stat.st_mode)) {
            break;
        }
        
        if ((promdnt->ROMFSDNT_uiSpec == uiAddr) || 
            (promdnt->ROMFSDNT_uiSpec == 0)) {                          /*  下级目录没有文件存在了      */
            break;
        }
        uiAddr = promdnt->ROMFSDNT_uiSpec;
    }
    
    if (*pcPtr) {                                                       /*  没有查找到目标              */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __rfs_path_build_link
** 功能描述: 根据链接文件内容生成连接目标
** 输　入  : promfs           文件系统
**           promdnt          文件结构
**           pcDest           输出缓冲
**           stSize           输出缓冲大小
**           pcPrefix         前缀
**           pcTail           后缀
** 输　出  : < 0 错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  __rfs_path_build_link (PROM_VOLUME  promfs, PROMFS_DIRENT  promdnt, 
                            PCHAR        pcDest, size_t         stSize,
                            PCHAR        pcPrefix, PCHAR        pcTail)
{
    CHAR        cLink[PATH_MAX + 1];
    
    if (__rfs_readlink(promfs, promdnt, cLink, PATH_MAX + 1) > 0) {
        return  (_PathBuildLink(pcDest, stSize, promfs->ROMFS_devhdrHdr.DEVHDR_pcName, pcPrefix, 
                                cLink, pcTail));
    } else {
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_ROMFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
