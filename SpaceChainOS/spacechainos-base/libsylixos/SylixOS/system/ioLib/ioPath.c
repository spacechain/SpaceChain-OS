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
** 文   件   名: ioPath.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 16 日
**
** 描        述: 系统 IO 内部功能函数库，(PATH)

** BUG
2007.06.04  Line214 行 ppcArray++; 应该是每次 while(...) 进行一次。
2007.09.12  加入裁剪支持。
2008.03.27  修改了代码的格式.
2008.10.07  解决了 _PathCondense() 函数的一个的 BUG.
2008.10.09  当 _PathCondense() 化简为无实际意义的分割符时, 就忽略.
2008.11.21  加入 _PathGetDef() 支持线程私有 io 当前路径.
2009.12.14  当系统使用分级目录管理时, 使用 _PathCondense() 将进行包含设备名的全路径压缩.
2009.12.17  更新 _PathCondense() 函数, 不再需要巨大的堆栈支持.
2010.10.23  去掉了目前已经很多不需要的功能, 并整理代码.
2011.02.17  修正注释.
2011.08.07  升级 _PathBuildLink() 函数, 使之更易用.
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2012.10.25  _PathGetDef() 获得相对路径的优先级是: 进程(高)->当前任务(中)->全局(低)
2012.12.17  加入 _PathGetFull() 功能.
2013.01.09  _PathGetDef() 使用 _IosEnvGetDef() 获取当前 IO 环境.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
  PATH
*********************************************************************************************************/
static PCHAR _PathSlashRindex(CPCHAR  pcString);
static PCHAR _Strcatlim(PCHAR   pcStr1,
                        CPCHAR  pcStr2,
                        size_t  stLimit);
/*********************************************************************************************************
** 函数名称: _PathCondense
** 功能描述: 压缩一个文件或者目录名
** 输　入  : 
**           pcPathName                       名字
** 输　出  : VOID
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#define __LW_PATH_IS_DIVIDER(c) \
        (c == '/' || c == '\\')

#define __LW_PATH_PUTBACK(pcPtr, pcRoot)    \
        while (pcPtr != pcRoot) {   \
            pcPtr--;    \
            if (__LW_PATH_IS_DIVIDER(*pcPtr)) { \
                break;  \
            }   \
        }
        
VOID  _PathCondense (PCHAR  pcPathName)
{
    PCHAR    pcTail;                                                    /*  压缩起始点                  */
    PCHAR    pcNode;                                                    /*  当前节点                    */
    BOOL     bCheakDot = LW_TRUE;                                       /*  是否检查 . 和 ..            */
    BOOL     bNotInc   = LW_FALSE;                                      /*  是否向前推移了保存指针      */
    
    PCHAR    pcTemp;                                                    /*  搜索点                      */
    INT      iDotNum = 0;                                               /*  连续点的个数                */
    
    if ((pcPathName == LW_NULL) || (*pcPathName == PX_EOS)) {           /*  无效目录                    */
        return;
    }
    
    if (pcPathName[0] == PX_ROOT) {                                     /*  从根目录开始                */
        /*
         *  搜索起始为根目录.
         */
        pcTail  = pcPathName;
    } else {
        iosDevFind(pcPathName, &pcTail);                                /*  从设备尾部开始压缩          */
    }
    pcNode  = pcTail;
    pcTemp  = pcTail;
    
    /*
     *  开始搜索, 同时将所有的 '\' 转换成 '/'! (有些文件系统分析必须使用 / 作为分割)
     */
    while (*pcTemp != PX_EOS) {                                         /*  遍历路径                    */
    
        if (__LW_PATH_IS_DIVIDER(*pcTemp)) {                            /*  分隔符                      */
            *pcTemp = PX_DIVIDER;                                       /*  转换分隔符                  */

            if (iDotNum > 1) {                                          /*  双点, 将忽略上一级目录      */
                /*
                 *  XXX 起始字符必须为 / 否则这里的后退会多出一个字符!
                 */
                __LW_PATH_PUTBACK(pcNode, pcTail);                      /*  退回到上一级目录            */
            } else if (iDotNum == 1) {                                  /*  单点                        */
                /*
                 *  什么也不处理, 直接忽略此目录.
                 */
            } else {
                /*
                 *  这里直接拷贝分隔符但不改变保存指针.
                 */
                *pcNode = *pcTemp;                                      /*  直接拷贝                    */
                bNotInc = LW_TRUE;
            }

            /*
             *  发现分隔符时应该开始检测 . 和 ..
             */
            iDotNum   = 0;                                              /*  重新开始检测 . 和 ..        */
            bCheakDot = LW_TRUE;
        
        } else if ((*pcTemp == '.') && bCheakDot) {                     /*  点                          */
            iDotNum++;

        } else {                                                        /*  普通字符                    */
            /*
             *  如果由于分隔符导致当前的保存指针没有向前推进, 这里需要处理.
             */
            if (bNotInc) {
                bNotInc = LW_FALSE;
                pcNode++;                                               /*  需要向前推移                */
            }
            
            /*
             *  如果字符前发现了错误判断的 . 这里需要处理.
             */
            if (bCheakDot) {
                INT         i;
                for (i = 0; i < iDotNum; i++) {
                     *pcNode++ = '.';                                   /*  将之前的 . 都补上           */
                }
                
                bCheakDot = LW_FALSE;                                   /*  不检测. 和 ..               */
                iDotNum   = 0;
            }

            *pcNode++ = *pcTemp;                                        /*  拷贝这个字符                */
        }

        pcTemp++;                                                       /*  下一个字符                  */

    } while (*pcTemp != 0);

    /*
     *  如果以 .. 结束, 需要做处理, (将结束字符当作分隔符的处理过程)
     */
    if (bCheakDot && (iDotNum > 1)) {                                   /*  最后有双点, 将忽略上一级目录*/
        __LW_PATH_PUTBACK(pcNode, pcTail);                              /*  退回到上一级目录            */
    }

    if (bNotInc) {                                                      /*  结束时如果指向的是 /        */
        pcNode++;                                                       /*  需要推移一个字节            */
    }
    *pcNode = PX_EOS;                                                   /*  结束                        */

    /*
     *  如果最后一个字节为多余的分隔符, 将它去掉
     */
    if (((pcNode - pcTail) > 1) && 
        __LW_PATH_IS_DIVIDER(*(pcNode - 1))) {
        *(pcNode - 1) = PX_EOS;
    }
}
/*********************************************************************************************************
** 函数名称: _PathCat
** 功能描述: 连接目录和文件名
** 输　入  : 
**           pcDirName                     目录名
**           pcFileName                    文件名
**           pcResult                      结果
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  _PathCat (CPCHAR  pcDirName,
                 CPCHAR  pcFileName,
                 PCHAR   pcResult)
{
    PCHAR    pcTail;
    
    if ((pcFileName == LW_NULL) || (pcFileName[0] == PX_EOS)) {
	    lib_strcpy(pcResult, pcDirName);
	    return  (ERROR_NONE);
	}
	
	if ((pcDirName == LW_NULL) || (pcDirName[0] == PX_EOS)) {
	    lib_strcpy(pcResult, pcFileName);
	    return  (ERROR_NONE);
	}
	
	if (lib_strnlen(pcFileName, MAX_FILENAME_LENGTH) >=
        MAX_FILENAME_LENGTH) {                                          /*  名字太长                    */
	    return  (ENAMETOOLONG);
	}
	
	iosDevFind(pcFileName, &pcTail);
	if (pcTail != pcFileName) {                                         /*  pcFileName 为设备名         */
	    lib_strcpy(pcResult, pcFileName);                               /*  直接返回设备名              */
	    return  (ERROR_NONE);
	}
	
	*pcResult = PX_EOS;
	
#if LW_CFG_PATH_VXWORKS > 0                                             /*  不进行分级目录管理          */
	/* 
	 *  非 '/' 起始的设备名.
     */
    if (pcDirName[0] != PX_ROOT) {
        iosDevFind(pcDirName, &pcTail);
        if (pcTail != pcDirName) {
            lib_strncat(pcResult, pcDirName, pcTail - pcDirName);
	        pcDirName = pcTail;
	    }
	}
#endif                                                                  /*  LW_CFG_PATH_VXWORKS > 0     */
	
	/* 
	 *  if filename is relative path, prepend directory if any 
	 */
	if (lib_index("/\\~$", pcFileName[0]) == LW_NULL) {
	    if (pcDirName[0] != PX_EOS) {
	        _Strcatlim(pcResult, pcDirName, PATH_MAX);
	        if (pcDirName[lib_strlen(pcDirName) - 1] != PX_DIVIDER) {
	            _Strcatlim(pcResult, PX_STR_DIVIDER, PATH_MAX);
	        }
	    }
	}
	
	/* 
	 *  concatenate filename 
	 */
	_Strcatlim(pcResult, pcFileName, PATH_MAX);
	
	return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _Strcatlim
** 功能描述: 有限制的将一个字符串的内容添加如另一个字符串.
** 输　入  : pcStr1         原始字符串
**           pcStr2         添加字符串
**           stLimit        限制数量
** 输　出  : 新字符串头.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PCHAR  _Strcatlim (PCHAR   pcStr1,
                          CPCHAR  pcStr2,
                          size_t  stLimit)
{
    REGISTER size_t stN1;
    REGISTER size_t stN2;
    
    stN1 = lib_strlen(pcStr1);
    if (stN1 < stLimit) {
        stN2 = lib_strlen(pcStr2);
        stN2 = __MIN(stN2, (stLimit - stN1));
        
        /*  bcopy(pcStr2, &pcStr1 [iN1], iN2);
         *  void  bcopy(const void *src, void *dest, int n);
         *  void *memcpy(void *dest, void *src, unsigned int count);
         */
        lib_memcpy(&pcStr1[stN1], pcStr2, stN2);
        pcStr1[stN1 + stN2] = PX_EOS;
    }
    
    return  (pcStr1);
}
/*********************************************************************************************************
** 函数名称: _PathLastNamePtr
** 功能描述: 返回路径中的最后一个名字指针
** 输　入  : 
**           pcPathName                    路径名
** 输　出  : 指针
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PCHAR  _PathLastNamePtr (CPCHAR  pcPathName)
{
    REGISTER PCHAR    pcP;
	
    pcP = _PathSlashRindex(pcPathName);
    
    if (pcP == LW_NULL) {
        return  ((PCHAR)pcPathName);
    } else {
        return  ((PCHAR)(pcP + 1));
    }
}
/*********************************************************************************************************
** 函数名称: _PathLastName
** 功能描述: 返回路径中的最后一个名字
** 输　入  : 
**           pcPathName                    路径名
** 输　出  : VOID
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _PathLastName (CPCHAR  pcPathName, PCHAR  *ppcLastName)
{
    *ppcLastName = _PathLastNamePtr(pcPathName);
}
/*********************************************************************************************************
** 函数名称: _PathSlashRindex
** 功能描述: 查找字符串中的最后 '/' 或 '\' 指针
** 输　入  : 
**           pcString                      字符串
** 输　出  : VOID
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PCHAR  _PathSlashRindex (CPCHAR  pcString)
{
    REGISTER PCHAR    pcForward;
    REGISTER PCHAR    pcBack;
    
    REGISTER PCHAR    pcLast;
    
    pcForward = lib_rindex(pcString, '/');
    pcBack    = lib_rindex(pcString, '\\');
    
    pcLast    = __MAX(pcForward, pcBack);
    
    return  (pcLast);
}
/*********************************************************************************************************
** 函数名称: _PathGetDef
** 功能描述: 获得当前路径指针
** 输　入  : NONE
** 输　出  : 当前路径指针
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PCHAR  _PathGetDef (VOID)
{
    PLW_IO_ENV      pioe = _IosEnvGetDef();
    
    return  (pioe->IOE_cDefPath);
}
/*********************************************************************************************************
** 函数名称: _PathGetFull
** 功能描述: 获得文件完整路径名
** 输　入  : pcFullPath            完整文件名结果
**           stMaxSize             缓冲区大小
**           pcFileName            文件名
** 输　出  : 如果成功则返回完整文件名
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PCHAR  _PathGetFull (PCHAR  pcFullPath, size_t  stMaxSize, CPCHAR  pcFileName)
{
    if (stMaxSize < 2) {
        return  (LW_NULL);
    }

    if (pcFileName[0] != PX_ROOT) {
        size_t  stCpLen = lib_strlcpy(pcFullPath, _PathGetDef(), stMaxSize - 1);
        if ((stCpLen > 1) && (pcFullPath[stCpLen - 1] != PX_DIVIDER)) {
            pcFullPath[stCpLen] = PX_DIVIDER;
            stCpLen++;
        }
        lib_strlcpy(&pcFullPath[stCpLen], pcFileName, stMaxSize - stCpLen);
    
    } else {
        lib_strlcpy(pcFullPath, pcFileName, stMaxSize);
    }
    
    _PathCondense(pcFullPath);
    
    return  (pcFullPath);
}
/*********************************************************************************************************
** 函数名称: _PathBuildLink
** 功能描述: 将两个目录链接为一个目录 (用于符号链接文件)
** 输　入  : pcBuildName   输出缓冲
**           stMaxSize     最大长度
**           pcDevName     设备名          例如: /yaffs2
**           pcPrefix      目录前缀              /n1/dir1
**           pcLinkDst     链接目标              linkdst or /linkdst
**           pcTail        名字尾部              /subdir
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _PathBuildLink (PCHAR      pcBuildName, 
                     size_t     stMaxSize, 
                     CPCHAR     pcDevName,
                     CPCHAR     pcPrefix,
                     CPCHAR     pcLinkDst,
                     CPCHAR     pcTail)
{
    size_t  stLenDevName = 0;
    size_t  stLenPrefix = 0;
    size_t  stLenTail = 0;
    size_t  stLenLink;
    size_t  stLenTotal;
    
    PCHAR   pcBuffer;
    
    /*
     *  因为 pcTail 可能在 pcBuffer 中, 所以不能直接使用 snprintf(..., "%s%s", ...) 链接目录.
     */
    if ((pcBuildName == LW_NULL) ||
        (stMaxSize   == 0) || 
        (pcLinkDst   == LW_NULL)) {                                     /*  参数检查                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pcDevName) {
        stLenDevName = lib_strlen(pcDevName);                           /*  确定 dev name 长度          */
    }
    if (pcPrefix) {
        stLenPrefix = lib_strlen(pcPrefix);                             /*  确定 前缀 长度              */
    }
    if (pcTail) {
        stLenTail = lib_strlen(pcTail);                                 /*  确定 tail 长度              */
    }
    stLenLink = lib_strlen(pcLinkDst);
    
    if ((stLenDevName + stLenLink + stLenPrefix) == 0) {                /*  直接输出 pcLinkDst          */
        lib_strlcpy(pcBuildName, pcLinkDst, stMaxSize);
        return  (ERROR_NONE);
    }
    
    stLenTotal = stLenDevName + stLenPrefix + stLenTail + stLenLink + 4;
    pcBuffer   = (PCHAR)__SHEAP_ALLOC(stLenTotal);
    if (pcBuffer == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    if (pcDevName == LW_NULL) {
        pcDevName =  "";
    }
    if (pcPrefix == LW_NULL) {
        pcPrefix =  "";
    }
    if (pcTail == LW_NULL) {
        pcTail =  "";
    }
    
    if (*pcLinkDst == PX_ROOT) {                                        /*  绝对地址不写设备名和前缀    */
        if ((*pcTail != PX_EOS) && (*pcTail != PX_DIVIDER)) {
            snprintf(pcBuffer, stLenTotal, 
                     "%s"PX_STR_DIVIDER"%s", pcLinkDst, pcTail);
        } else {
            snprintf(pcBuffer, stLenTotal, 
                     "%s%s", pcLinkDst, pcTail);
        }
        
    } else {
        if ((*pcTail != PX_EOS) && (*pcTail != PX_DIVIDER)) {
            snprintf(pcBuffer, stLenTotal, 
                     "%s%s"PX_STR_DIVIDER"%s"PX_STR_DIVIDER"%s", 
                     pcDevName, pcPrefix, pcLinkDst, pcTail);
        } else {
            snprintf(pcBuffer, stLenTotal, 
                     "%s%s"PX_STR_DIVIDER"%s%s", 
                     pcDevName, pcPrefix, pcLinkDst, pcTail);
        }
    }
    
    lib_strlcpy(pcBuildName, pcBuffer, stMaxSize);                      /*  拷贝结果                    */
    
    __SHEAP_FREE(pcBuffer);
    
    _PathCondense(pcBuildName);                                         /*  压缩目录                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _PathMoveCheck
** 功能描述: 将一个节点(目录或文件)移动(或重命名)到另一个目录, 检查是否合法.
** 输　入  : pcSrc         源文件或目录
**           pcDest        目标文件或目录
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  _PathMoveCheck (CPCHAR  pcSrc, CPCHAR  pcDest)
{
    CPCHAR  pcPos;

    if (*pcSrc == PX_DIVIDER) {
        pcSrc++;
    }

    if (*pcDest == PX_DIVIDER) {
        pcDest++;
    }

    if (lib_strstr(pcDest, pcSrc) == pcDest) {
        pcPos = pcDest + lib_strlen(pcSrc);
        if (*pcPos == PX_DIVIDER) {
            return  (PX_ERROR);                                         /*  不允许将父目录移至子目录    */
        }
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
