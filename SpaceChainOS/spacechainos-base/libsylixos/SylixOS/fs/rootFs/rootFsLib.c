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
** 文   件   名: rootFsLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 03 日
**
** 描        述: root 文件系统内部库 (在安装所有设备之前, 必须安装 rootFs 驱动和 root 设备, 
                                      它定义所有设备的挂接点).
                                      
** BUG:
2010.08.10  修正内存不足时的处理 BUG.
2010.09.10  修正创建连接节点时的 mode 设置 BUG.
2011.03.22  API_RootFsMakeNode() 符号链接文件应该具有 S_IFLNK 属性.
2011.05.16  创建节点时, 如果父系节点为符号链接, 则返回 errno == ERROR_IOS_FILE_SYMLINK.
2011.05.19  修正 dev match 时对根文件系统目录内文件判断错误问题.
2011.06.11  make node 加入 opt 选项, 加入对节点时间继承 rootfs 时间的支持.
2011.08.11  __rootFsDevMatch() 如果都匹配失败, 如果文件名是从根符号开始的, 则返回根设备.
2012.09.25  支持 LW_ROOTFS_NODE_TYPE_SOCK 类型文件.
2012.11.09  __rootFsReadNode() 返回有效字节长度.
2012.12.20  修正 API_RootFsMakeNode() 建立 socket 文件错误.
2013.03.16  加入对 reg 文件的支持.
            API_RootFsMakeNode 加入 mode 参数.
2013.08.13  加入对文件数量的记录.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PATH_VXWORKS == 0)
#include "rootFsLib.h"
#include "rootFs.h"
/*********************************************************************************************************
  rootfs 全局变量
*********************************************************************************************************/
LW_ROOTFS_ROOT      _G_rfsrRoot;                                        /*  rootFs 根                   */
/*********************************************************************************************************
** 函数名称: __rootFsFindNode
** 功能描述: rootfs 查找一个节点
** 输　入  : pcName            节点名 (必须从根符号 / 开始)
**           pprfsnFather      当无法找到节点时最为接近的一个, 
                               当找到节点时返回找到节点的父系节点. 
                               LW_NULL 表示找到的为根
**           pbRoot            节点名是否指向根节点.
**           pbLast            当匹配失败时, 是否是最后一级文件匹配失败
                               例如: /a/b/c 目录a和b都匹配成功, 仅剩c没有成功则 pbLast 为 LW_TRUE.
**           ppcTail           名字尾部, 当搜索到设备或者链接时, tail 指向名字尾.
** 输　出  : 节点, LW_NULL 表示根节点
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_ROOTFS_NODE  __rootFsFindNode (CPCHAR            pcName, 
                                   PLW_ROOTFS_NODE  *pprfsnFather, 
                                   BOOL             *pbRoot,
                                   BOOL             *pbLast,
                                   PCHAR            *ppcTail)
{
    /*
     *  所有调用此函数的调用者均要保证此函数被 LW_ROOTFS_LOCK() 保护, 
     *  此函数内部使用的路径缓冲为全局变量.
     */
    static CHAR         pcTempName[MAX_FILENAME_LENGTH];
    PCHAR               pcNext;
    PCHAR               pcNode;
    
    PLW_ROOTFS_NODE     prfsn;
    PLW_ROOTFS_NODE     prfsnTemp;
    
    PLW_LIST_LINE       plineTemp;
    PLW_LIST_LINE       plineHeader;                                    /*  当前目录头                  */
    
    if (pprfsnFather == LW_NULL) {
        pprfsnFather = &prfsnTemp;                                      /*  临时变量                    */
    }
    *pprfsnFather = LW_NULL;
    
    if (*pcName == PX_ROOT) {                                           /*  忽略根符号                  */
        lib_strlcpy(pcTempName, (pcName + 1), PATH_MAX);
    
    } else {
        if (pbRoot) {
            *pbRoot = LW_FALSE;                                         /*  pcName 不为根               */
        }
        if (pbLast) {
            *pbLast = LW_FALSE;
        }
        return  (LW_NULL);
    }
    
    /*
     *  判断文件名是否为根
     */
    if (pcTempName[0] == PX_EOS) {
        if (pbRoot) {
            *pbRoot = LW_TRUE;                                          /*  pcName 为根                 */
        }
        if (pbLast) {
            *pbLast = LW_FALSE;
        }
        return  (LW_NULL);
    
    } else {
        if (pbRoot) {
            *pbRoot = LW_FALSE;                                         /*  pcName 不为根               */
        }
    }
    
    pcNext      = pcTempName;
    plineHeader = _G_rfsrRoot.RFSR_plineSon;                            /*  从根目录开始搜索            */
    
    /*
     *  这里使用循环 (避免递归!)
     */
    do {
        pcNode = pcNext;
        pcNext = lib_index(pcNode, PX_DIVIDER);                         /*  移动到下级目录              */
        if (pcNext) {                                                   /*  是否可以进入下一层          */
            *pcNext = PX_EOS;
            pcNext++;                                                   /*  下一层的指针                */
        }
        
        for (plineTemp  = plineHeader;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            prfsn = _LIST_ENTRY(plineTemp, LW_ROOTFS_NODE, RFSN_lineBrother);
            if (prfsn->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_DEV) {     /*  此节点为设备                */
                /*
                 *  仅比较设备名部分
                 */
                PCHAR       pcDevName = lib_rindex(prfsn->RFSN_rfsnv.RFSNV_pdevhdr->DEVHDR_pcName, 
                                                   PX_DIVIDER);
                if (pcDevName == LW_NULL) {
                    pcDevName =  prfsn->RFSN_rfsnv.RFSNV_pdevhdr->DEVHDR_pcName;
                } else {
                    pcDevName++;                                        /*  过滤 /                      */
                }
                
                if (lib_strcmp(pcDevName, pcNode) == 0) {               /*  比较设备名                  */
                    goto    __find_ok;                                  /*  找到设备                    */
                }
            
            } else if (prfsn->RFSN_iNodeType == 
                       LW_ROOTFS_NODE_TYPE_LNK) {                       /*  此节点为链接                */
                if (lib_strcmp(prfsn->RFSN_rfsnv.RFSNV_pcName,
                               pcNode) == 0) {
                    goto    __find_ok;                                  /*  找到链接                    */
                }
            
            } else if (prfsn->RFSN_iNodeType == 
                       LW_ROOTFS_NODE_TYPE_SOCK) {                      /*  此节点为 socket 文件        */
                if (lib_strcmp(prfsn->RFSN_rfsnv.RFSNV_pcName,
                               pcNode) == 0) {
                    if (pcNext) {                                       /*  还存在下级, 这里必须为目录  */
                        goto    __find_error;                           /*  不是目录直接错误            */
                    }
                    break;
                }
            } else {                                                    /*  此节点为目录                */
                if (lib_strcmp(prfsn->RFSN_rfsnv.RFSNV_pcName,
                               pcNode) == 0) {
                    break;
                }
            }
        }
        if (plineTemp == LW_NULL) {                                     /*  无法继续搜索                */
            goto    __find_error;
        }
        
        *pprfsnFather = prfsn;                                          /*  从当前节点开始搜索          */
        plineHeader   = prfsn->RFSN_plineSon;                           /*  从第一个儿子开始            */
        
    } while (pcNext);                                                   /*  不存在下级目录              */
    
__find_ok:
    *pprfsnFather = prfsn->RFSN_prfsnFather;                            /*  父系节点                    */
    /*
     *  计算 tail 的位置.
     */
    if (ppcTail) {
        if (pcNext) {
            INT   iTail = pcNext - pcTempName;
            *ppcTail = (PCHAR)pcName + iTail;                           /*  指向没有被处理的 / 字符     */
        } else {
            *ppcTail = (PCHAR)pcName + lib_strlen(pcName);              /*  指向最末尾                  */
        }
    }
    return  (prfsn);
    
__find_error:
    if (pbLast) {
        if (pcNext == LW_NULL) {                                        /*  最后一级查找失败            */
            *pbLast = LW_TRUE;
        } else {
            *pbLast = LW_FALSE;
        }
    }
    return  (LW_NULL);                                                  /*  无法找到节点                */
}
/*********************************************************************************************************
** 函数名称: __rootFsDevMatch
** 功能描述: rootfs 匹配一个设备 (设备名必须从 / 符号开始)
** 输　入  : pcName            设备名
** 输　出  : 设备节点
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_DEV_HDR  __rootFsDevMatch (CPCHAR  pcName)
{
    PLW_ROOTFS_NODE    prfsnFather;
    PLW_ROOTFS_NODE    prfsn;
    BOOL               bIsRoot;
    PLW_DEV_HDR        pdevhdrFind = LW_NULL;
    
    if ((pcName == LW_NULL) || (pcName[0] != PX_ROOT)) {                /*  检查设备名有效性            */
        return  (LW_NULL);
    }
    
    /*
     *  根设备特殊处理
     */
    if (lib_strcmp(pcName, PX_STR_ROOT) == 0) {                         /*  寻找 root 设备              */
        if (_G_devhdrRoot.DEVHDR_usDrvNum) {
            return  (&_G_devhdrRoot);                                   /*  如果安装了根设备            */
        } else {
            return  (LW_NULL);                                          /*  没有安装根设备              */
        }
    }
    
    /*
     *  pcName 参数必须是从根目录符号开始的设备名
     */
    __LW_ROOTFS_LOCK();                                                 /*  锁定 rootfs                 */
    prfsn = __rootFsFindNode(pcName, &prfsnFather, &bIsRoot, LW_NULL, LW_NULL);
                                                                        /*  查询设备                    */
    if (prfsn) {
        if (prfsn->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_DEV) {         /*  查找到设备                  */
            pdevhdrFind = prfsn->RFSN_rfsnv.RFSNV_pdevhdr;
        } else {
            pdevhdrFind = &_G_devhdrRoot;                               /*  查找到根设备下的目录或链接  */
        }
    
    } else if (prfsnFather) {                                           /*  没有找到设备, 但找到父系节点*/
        if (prfsnFather->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_DEV) {   /*  有父系节点存在, 且为设备的  */
            /*
             *  判断名字是否带有 tail. 例如设备为: /dev/temp 但 pcName 为 /dev/temp/aaa/bbb/...
             */
            pdevhdrFind = prfsnFather->RFSN_rfsnv.RFSNV_pdevhdr;        /*  使用父系设备                */
        
        } else if (prfsnFather->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_DIR) {
            pdevhdrFind = &_G_devhdrRoot;                               /*  如果为目录, 则为根文件系统  */
        }
    
    } else if (pcName[0] == PX_ROOT) {                                  /*  从根目录开始                */
        if (_G_devhdrRoot.DEVHDR_usDrvNum) {
            pdevhdrFind = &_G_devhdrRoot;
        }
    }
    __LW_ROOTFS_UNLOCK();                                               /*  解锁 rootfs                 */
    
    return  (pdevhdrFind);
}
/*********************************************************************************************************
** 函数名称: __rootFsReadNode
** 功能描述: rootfs 读取一个节点, (节点只能是链接节点) 
** 输　入  : pcName        节点名
**           pcBuffer      内容缓冲
**           stSize        缓冲大小
** 输　出  : 读取的大小
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  __rootFsReadNode (CPCHAR  pcName, PCHAR  pcBuffer, size_t  stSize)
{
    PLW_ROOTFS_NODE    prfsnFather = LW_NULL;
    PLW_ROOTFS_NODE    prfsn;
    BOOL               bIsRoot = LW_FALSE;
    CHAR               cFullPathName[MAX_FILENAME_LENGTH];
    
    ssize_t            sstRet  = PX_ERROR;
    
    if ((pcName == LW_NULL)   || 
        (pcBuffer == LW_NULL) || 
        (stSize == 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    /*
     *  根设备特殊处理
     */
    if ((pcName[0] == PX_EOS) || 
        (lib_strcmp(pcName, PX_STR_ROOT) == 0)) {                       /*  不能读取根设备              */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    if (_PathCat(_PathGetDef(), pcName, cFullPathName) != ERROR_NONE) { /*  获得从根目录开始的路径      */
        _ErrorHandle(ENAMETOOLONG);
        return  (PX_ERROR);
    }
    
    __LW_ROOTFS_LOCK();                                                 /*  锁定 rootfs                 */
    prfsn = __rootFsFindNode(cFullPathName, &prfsnFather, 
                             &bIsRoot, LW_NULL, LW_NULL);               /*  查询设备                    */
    if (prfsn) {
        if (prfsn->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_LNK) {
            size_t  stLen = lib_strlen(prfsn->RFSN_pcLink);
            lib_strncpy(pcBuffer, prfsn->RFSN_pcLink, stSize);          /*  拷贝链接内容                */
            if (stLen > stSize) {
                stLen = stSize;                                         /*  计算有效字节数              */
            }
            sstRet = (ssize_t)stLen;
            
        } else {
            _ErrorHandle(ENOENT);
        }
    }
    __LW_ROOTFS_UNLOCK();                                               /*  解锁 rootfs                 */
    
    return  (sstRet);
}
/*********************************************************************************************************
** 函数名称: API_RootFsMakeNode
** 功能描述: rootfs 创建一个节点
** 输　入  : pcName        全名 (从根开始)
**           iNodeType     节点类型     为设备时 pvValue 为 PLW_DEV_HDR 指针
                                        为目录时 pvValue 为 LW_NULL 指针
                                        为链接时 pvValue 为 链接路径指针
             iNodeOpt      节点选项
             iMode         mode_t
**           pvValue       参数
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_RootFsMakeNode (CPCHAR  pcName, INT  iNodeType, INT  iNodeOpt, INT  iMode, PVOID  pvValue)
{
    PLW_ROOTFS_NODE    prfsnFather = LW_FALSE;
    PLW_ROOTFS_NODE    prfsn;
    BOOL               bIsRoot = LW_FALSE;
    BOOL               bLast = LW_FALSE;
    CHAR               cFullPathName[MAX_FILENAME_LENGTH];
    PCHAR              pcTail = LW_NULL;
    
    size_t             stAllocSize;
    INT                iError = PX_ERROR;
    PLW_ROOTFS_NODE    prfsnNew;

    iMode &= ~S_IFMT;                                                   /*  去掉类型信息                */
    
    if ((pcName == LW_NULL) || (*pcName != PX_ROOT)) {                  /*  路径名                      */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (iNodeType > LW_ROOTFS_NODE_TYPE_REG) {                          /*  检查节点类型                */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if ((iNodeType != LW_ROOTFS_NODE_TYPE_DIR) &&
        (iNodeType != LW_ROOTFS_NODE_TYPE_SOCK) &&
        (iNodeType != LW_ROOTFS_NODE_TYPE_REG) &&
        (pvValue == LW_NULL)) {                                         /*  需要第三个参数              */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    /*
     *  根设备特殊处理
     */
    if (lib_strcmp(pcName, PX_STR_ROOT) == 0) {                         /*  添加根设备忽略              */
        return  (ERROR_NONE);
    }
    
    if (_PathCat(_PathGetDef(), pcName, cFullPathName) != ERROR_NONE) { /*  获得从根目录开始的路径      */
        _ErrorHandle(ENAMETOOLONG);
        return  (PX_ERROR);
    }
    
    /*
     *  创建各种类型节点内存
     */
    if (iNodeType == LW_ROOTFS_NODE_TYPE_DIR) {                         /*  目录项                      */
        /*
         *  目录节点类型
         */
        PCHAR   pcDirName = lib_rindex(cFullPathName, PX_DIVIDER);      /*  获得文件名, 一定不为 NULL   */
        size_t  stLen;
        
        pcDirName++;                                                    /*  指向文件名的第一个节点      */
        stLen = lib_strlen(pcDirName);
        if (stLen == 0) {
            _ErrorHandle(EINVAL);                                       /*  目录文件名错误              */
            return  (PX_ERROR);
        }
        /*
         *  避免内存分片浪费, 这里分配一整片内存.
         */
        stLen++;                                                        /*  预留 \0 空间                */
        stAllocSize = sizeof(LW_ROOTFS_NODE) + stLen;
        prfsnNew = (PLW_ROOTFS_NODE)__SHEAP_ALLOC(stAllocSize);         /*  分配节点内存                */
        if (prfsnNew) {
            prfsnNew->RFSN_rfsnv.RFSNV_pcName = (PCHAR)prfsnNew + sizeof(LW_ROOTFS_NODE);
            lib_strcpy(prfsnNew->RFSN_rfsnv.RFSNV_pcName, pcDirName);
            
            prfsnNew->RFSN_pcLink = LW_NULL;                            /*  非链接文件                  */
            prfsnNew->RFSN_mode   = (iMode | S_IFDIR);
        }
    } else if (iNodeType == LW_ROOTFS_NODE_TYPE_DEV) {                  /*  设备项                      */
        /*
         *  设备节点类型
         */
        stAllocSize = sizeof(LW_ROOTFS_NODE);
        prfsnNew = (PLW_ROOTFS_NODE)__SHEAP_ALLOC(sizeof(LW_ROOTFS_NODE));
                                                                        /*  分配节点内存                */
        if (prfsnNew) {
            prfsnNew->RFSN_rfsnv.RFSNV_pdevhdr = (PLW_DEV_HDR)pvValue;  /*  保存设备头                  */
        
            prfsnNew->RFSN_pcLink = LW_NULL;                            /*  非链接文件                  */
            prfsnNew->RFSN_mode   = iMode;
        }
        
    } else if ((iNodeType == LW_ROOTFS_NODE_TYPE_SOCK) ||
               (iNodeType == LW_ROOTFS_NODE_TYPE_REG)) {                /*  socket or reg file                 */
        /*
         *  AF_UNIX 文件
         */
        PCHAR   pcSockName = lib_rindex(cFullPathName, PX_DIVIDER);     /*  获得文件名, 一定不为 NULL   */
        size_t  stLen;
        
        if (pcSockName == LW_NULL) {
            pcSockName = cFullPathName;
        } else {
            pcSockName++;                                               /*  指向文件名的第一个节点      */
        }
        
        stLen = lib_strlen(pcSockName);
        if (stLen == 0) {
            _ErrorHandle(EINVAL);                                       /*  socket 文件名错误           */
            return  (PX_ERROR);
        }
        /*
         *  避免内存分片浪费, 这里分配一整片内存.
         */
        stLen++;                                                        /*  预留 \0 空间                */
        stAllocSize = sizeof(LW_ROOTFS_NODE) + stLen;
        prfsnNew = (PLW_ROOTFS_NODE)__SHEAP_ALLOC(stAllocSize);         /*  分配节点内存                */
        if (prfsnNew) {
            prfsnNew->RFSN_rfsnv.RFSNV_pcName = (PCHAR)prfsnNew + sizeof(LW_ROOTFS_NODE);
            lib_strcpy(prfsnNew->RFSN_rfsnv.RFSNV_pcName, pcSockName);
            
            prfsnNew->RFSN_pcLink = LW_NULL;                            /*  非链接文件                  */
            if (iNodeType == LW_ROOTFS_NODE_TYPE_SOCK) {
                prfsnNew->RFSN_mode = (iMode | S_IFSOCK);               /*  socket 默认为 0777          */
            } else {
                prfsnNew->RFSN_mode = (iMode | S_IFREG);
            }
        }
    } else {                                                            /*  链接文件                    */
        /*
         *  链接节点类型
         */
        PCHAR   pcDirName = lib_rindex(cFullPathName, PX_DIVIDER);      /*  获得文件名, 一定不为 NULL   */
        size_t  stLen;
        size_t  stLinkLen = lib_strlen((PCHAR)pvValue);
        
        pcDirName++;                                                    /*  指向文件名的第一个节点      */
        stLen = lib_strlen(pcDirName);
        if ((stLen == 0) || (stLinkLen == 0)) {
            _ErrorHandle(EINVAL);                                       /*  文件名错误                  */
            return  (PX_ERROR);
        }
        /*
         *  避免内存分片浪费, 这里分配一整片内存.
         */
        stAllocSize = sizeof(LW_ROOTFS_NODE) + stLen;
        prfsnNew = (PLW_ROOTFS_NODE)__SHEAP_ALLOC(stAllocSize);         /*  分配节点内存                */
        if (prfsnNew) {
            prfsnNew->RFSN_rfsnv.RFSNV_pcName = (PCHAR)prfsnNew + sizeof(LW_ROOTFS_NODE);
            lib_strcpy(prfsnNew->RFSN_rfsnv.RFSNV_pcName, pcDirName);
        
            stLinkLen++;                                                /*  预留 \0 的位置              */
            prfsnNew->RFSN_pcLink = (PCHAR)__SHEAP_ALLOC(stLinkLen);
            if (prfsnNew->RFSN_pcLink) {
                lib_strcpy(prfsnNew->RFSN_pcLink, (PCHAR)pvValue);      /*  保存链接内容                */
                stAllocSize += stLinkLen;
            
            } else {
                __SHEAP_FREE(prfsnNew);                                 /*  缺少内存                    */
                prfsnNew = LW_NULL;
            }
        }
        
        /*
         *  将链接文件属性置为与连接对象一致.
         */
        if (prfsnNew) {
            struct stat   statBuf;
            
            if (stat((PCHAR)pvValue, &statBuf) < ERROR_NONE) {
                prfsnNew->RFSN_mode  = (iMode | S_IFLNK);
            } else {
                prfsnNew->RFSN_mode  = (statBuf.st_mode & ~S_IFMT);     /*  过滤 file type 信息         */
                prfsnNew->RFSN_mode |= S_IFLNK;
            }
        }
    }
    
    if (prfsnNew == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    prfsnNew->RFSN_plineSon    = LW_NULL;                               /*  没有任何儿子                */
    prfsnNew->RFSN_iOpenNum    = 0;
    prfsnNew->RFSN_iNodeType   = iNodeType;
    prfsnNew->RFSN_stAllocSize = stAllocSize;
    
    prfsnNew->RFSN_uid = getuid();
    prfsnNew->RFSN_gid = getgid();
    
    if (iNodeOpt & LW_ROOTFS_NODE_OPT_ROOTFS_TIME) {
        prfsnNew->RFSN_time = (time_t)(PX_ERROR);                       /*  继承 rootfs 时间            */
    } else {
        prfsnNew->RFSN_time = lib_time(LW_NULL);                        /*  以 UTC 时间作为时间基准     */
    }
    
    __LW_ROOTFS_LOCK();                                                 /*  锁定 rootfs                 */
    prfsn = __rootFsFindNode(cFullPathName, &prfsnFather, 
                             &bIsRoot, &bLast, &pcTail);                /*  查询设备                    */
    if (prfsn) {
        if ((pcTail && *pcTail == PX_ROOT) &&
            (prfsn->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_LNK)) {
            _ErrorHandle(ERROR_IOS_FILE_SYMLINK);                       /*  父节点为 symlink 类型       */
        } else {
            _ErrorHandle(ERROR_IOS_DUPLICATE_DEVICE_NAME);              /*  设备重名                    */
        }
    } else {
        if (prfsnFather && prfsnFather->RFSN_iNodeType) {               /*  父系节点不为目录            */
            /*
             *  此节点的父亲必须为一个目录!
             */
            _ErrorHandle(ENOENT);
        
        } else if (bLast == LW_FALSE) {                                 /*  缺少中间目录项              */
            _ErrorHandle(ENOENT);                                       /*  XXX errno ?                 */
        
        } else {
            if (prfsnFather) {
                prfsnNew->RFSN_prfsnFather = prfsnFather;
                _List_Line_Add_Ahead(&prfsnNew->RFSN_lineBrother,
                                     &prfsnFather->RFSN_plineSon);      /*  链入指定的链表              */
            } else {
                prfsnNew->RFSN_prfsnFather = LW_NULL;
                _List_Line_Add_Ahead(&prfsnNew->RFSN_lineBrother,
                                     &_G_rfsrRoot.RFSR_plineSon);       /*  插入根节点                  */
            }
            iError = ERROR_NONE;
        }
    }
    __LW_ROOTFS_UNLOCK();                                               /*  解锁 rootfs                 */
    
    if (iError) {
        if (prfsnNew->RFSN_pcLink) {
            __SHEAP_FREE(prfsnNew->RFSN_pcLink);                        /*  释放链接文件内容            */
        }
        __SHEAP_FREE(prfsnNew);                                         /*  释放内存                    */
    } else {
        _G_rfsrRoot.RFSR_stMemUsed += stAllocSize;                      /*  更新内存使用量              */
        _G_rfsrRoot.RFSR_ulFiles   += 1;                                /*  增加一个文件                */
    }
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_RootFsRemoveNode
** 功能描述: rootfs 删除一个节点, 
** 输　入  : pcName        节点名
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_RootFsRemoveNode (CPCHAR  pcName)
{
    PLW_ROOTFS_NODE    prfsnFather = LW_NULL;
    PLW_ROOTFS_NODE    prfsn;
    BOOL               bIsRoot = LW_FALSE;
    CHAR               cFullPathName[MAX_FILENAME_LENGTH];
    
    INT                iError = PX_ERROR;
    
    if (pcName == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    /*
     *  根设备特殊处理
     */
    if ((pcName[0] == PX_EOS) || 
        (lib_strcmp(pcName, PX_STR_ROOT) == 0)) {                       /*  不能移除根设备              */
        return  (ERROR_NONE);
    }
    
    if (_PathCat(_PathGetDef(), pcName, cFullPathName) != ERROR_NONE) { /*  获得从根目录开始的路径      */
        _ErrorHandle(ENAMETOOLONG);
        return  (PX_ERROR);
    }
    
    __LW_ROOTFS_LOCK();                                                 /*  锁定 rootfs                 */
    prfsn = __rootFsFindNode(cFullPathName, &prfsnFather, 
                             &bIsRoot, LW_NULL, LW_NULL);               /*  查询设备                    */
    if (prfsn) {
        if (prfsn->RFSN_plineSon) {
            _ErrorHandle(ENOTEMPTY);                                    /*  不为空                      */
        } else if (prfsn->RFSN_iOpenNum) {
            _ErrorHandle(EBUSY);                                        /*  节点没有关闭                */
        } else {
            if (prfsnFather == LW_NULL) {
                _List_Line_Del(&prfsn->RFSN_lineBrother,
                               &_G_rfsrRoot.RFSR_plineSon);             /*  从根节点卸载                */
            } else {
                _List_Line_Del(&prfsn->RFSN_lineBrother,
                               &prfsnFather->RFSN_plineSon);            /*  从父节点卸载                */
            }
            iError = ERROR_NONE;
        }
    
    } else {
        _ErrorHandle(ENOENT);                                           /*  没有对应的节点              */
    }
    __LW_ROOTFS_UNLOCK();                                               /*  解锁 rootfs                 */
    
    if (iError == ERROR_NONE) {
        _G_rfsrRoot.RFSR_stMemUsed -= prfsn->RFSN_stAllocSize;
        _G_rfsrRoot.RFSR_ulFiles   -= 1;                                /*  减少一个文件                */
        if (prfsn->RFSN_pcLink) {
            __SHEAP_FREE(prfsn->RFSN_pcLink);                           /*  释放链接文件内容            */
        }
        __SHEAP_FREE(prfsn);                                            /*  释放内存                    */
    }
    
    return  (iError);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_PATH_VXWORKS == 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
