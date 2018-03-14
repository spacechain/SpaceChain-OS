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
** 文   件   名: ttinyVarLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 08 月 06 日
**
** 描        述: 一个超小型的 shell 系统的变量管理器内部文件.

** BUG:
2011.06.03  加入环境变量的保存和读取功能.
2012.10.19  __tshellVarGet() 直接返回变量的值的内存, 不再使用 static buffer.
            __tshellVarSet() 时如果有被 __tshellVarGet() 引用过内存, 则不释放老的内存!
2013.05.10  __tshellVarDefine() 中合法的变量名支持含有数字.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  应用级 API
*********************************************************************************************************/
#include "../SylixOS/api/Lw_Api_Kernel.h"
#include "../SylixOS/api/Lw_Api_System.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "../SylixOS/shell/hashLib/hashHorner.h"
#include "../SylixOS/shell/ttinyShell/ttinyShell.h"
#include "../SylixOS/shell/ttinyShell/ttinyShellLib.h"
#include "../SylixOS/shell/ttinyShell/ttinyString.h"
#include "ttinyVarLib.h"
/*********************************************************************************************************
  执行回调函数
*********************************************************************************************************/
extern  VOID   (*_G_pfuncTSVarHook)();
#define __TSHELL_RUN_HOOK(pcVarName)            \
        {                                       \
            if (_G_pfuncTSVarHook) {            \
                _G_pfuncTSVarHook(pcVarName);   \
            }                                   \
        }
/*********************************************************************************************************
  内部链表头
*********************************************************************************************************/
static PLW_LIST_LINE    _G_plineTSVarHeader = LW_NULL;                  /*  统一链表头                  */
static PLW_LIST_LINE    _G_plineTSVarHeaderHashTbl[LW_CFG_SHELL_VAR_HASH_SIZE];
                                                                        /*  hash 散列表                 */
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
INT  __tshellExec(CPCHAR  pcCommandExec, VOIDFUNCPTR  pfuncHook);
/*********************************************************************************************************
** 函数名称: __tshellVarAdd
** 功能描述: 向 ttiny shell 系统添加一个变量
** 输　入  : pcVarName     变量名
**           pcVarValue    变量的值
**           stNameStrLen  名字长度
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __tshellVarAdd (CPCHAR       pcVarName, CPCHAR       pcVarValue, size_t  stNameStrLen)
{
    REGISTER __PTSHELL_VAR        pskvNode         = LW_NULL;           /*  变量节点                    */
    REGISTER PLW_LIST_LINE       *pplineHashHeader = LW_NULL;
    REGISTER INT                  iHashVal;
    REGISTER size_t               stValueStrLen;
    
    /*
     *  分配内存
     */
    pskvNode = (__PTSHELL_VAR)__SHEAP_ALLOC(sizeof(__TSHELL_VAR));
    if (!pskvNode) {                                                    /*  分配失败                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (ERROR_SYSTEM_LOW_MEMORY);                              /*  缺少内存                    */
    }
    
    pskvNode->SV_pcVarName = (PCHAR)__SHEAP_ALLOC(stNameStrLen + 1);
    if (!pskvNode->SV_pcVarName) {
        __SHEAP_FREE(pskvNode);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (ERROR_SYSTEM_LOW_MEMORY);                              /*  缺少内存                    */
    }
    
    lib_strcpy(pskvNode->SV_pcVarName, pcVarName);                      /*  保存变量名                  */
    
    pskvNode->SV_ulRefCnt = 0ul;                                        /*  没有外部引用此变量缓冲      */
    
    if (pcVarValue) {
        stValueStrLen = lib_strlen(pcVarValue);
        
        pskvNode->SV_pcVarValue = (PCHAR)__SHEAP_ALLOC(stValueStrLen + 1);
        if (!pskvNode->SV_pcVarValue) {
            __SHEAP_FREE(pskvNode->SV_pcVarName);
            __SHEAP_FREE(pskvNode);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (ERROR_SYSTEM_LOW_MEMORY);                          /*  缺少内存                    */
        }
        
        lib_strcpy(pskvNode->SV_pcVarValue, pcVarValue);
    
    } else {
        pskvNode->SV_pcVarValue = (PCHAR)__SHEAP_ALLOC(1);
        if (!pskvNode->SV_pcVarValue) {
            __SHEAP_FREE(pskvNode->SV_pcVarName);
            __SHEAP_FREE(pskvNode);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (ERROR_SYSTEM_LOW_MEMORY);                          /*  缺少内存                    */
        }
        
        lib_strcpy(pskvNode->SV_pcVarValue, "\0");
    }
    
    iHashVal = __hashHorner(pcVarName, LW_CFG_SHELL_VAR_HASH_SIZE);     /*  确定一阶散列的位置          */

    __TTINY_SHELL_LOCK();                                               /*  互斥访问                    */
    /*
     *  插入管理链表
     */
    _List_Line_Add_Ahead(&pskvNode->SV_lineManage, 
                         &_G_plineTSVarHeader);                         /*  插入管理表头                */
    
    /*
     *  插入哈希表
     */
    pplineHashHeader = &_G_plineTSVarHeaderHashTbl[iHashVal];
    
    _List_Line_Add_Ahead(&pskvNode->SV_lineHash, 
                         pplineHashHeader);                             /*  插入相应的表                */
    
    __TTINY_SHELL_UNLOCK();                                             /*  释放资源                    */
    
    __TSHELL_RUN_HOOK(pcVarName);                                       /*  调用改写回调                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellVarDelete
** 功能描述: 从 ttiny shell 系统中删除一个变量
** 输　入  : pskvNode      变量节点
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __tshellVarDelete (__PTSHELL_VAR  pskvNode)
{
    REGISTER PLW_LIST_LINE       *pplineHashHeader = LW_NULL;
    REGISTER INT                  iHashVal;
    
    __TTINY_SHELL_LOCK();                                               /*  互斥访问                    */
    /*
     *  从管理链表中删除
     */
    _List_Line_Del(&pskvNode->SV_lineManage, 
                   &_G_plineTSVarHeader);                               /*  插入管理表头                */
    
    /*
     *  从哈希表中删除
     */
    iHashVal = __hashHorner(pskvNode->SV_pcVarName, 
                            LW_CFG_SHELL_VAR_HASH_SIZE);                /*  确定一阶散列的位置          */
     
    pplineHashHeader = &_G_plineTSVarHeaderHashTbl[iHashVal];
    
    _List_Line_Del(&pskvNode->SV_lineHash, 
                   pplineHashHeader);                                   /*  删除                        */

    __TTINY_SHELL_UNLOCK();                                             /*  释放资源                    */
    
    __TSHELL_RUN_HOOK(pskvNode->SV_pcVarName);                          /*  调用回调                    */
    
    if (pskvNode->SV_ulRefCnt == 0) {                                   /*  有被直接引用过内存          */
        __SHEAP_FREE(pskvNode->SV_pcVarValue);
    }
    __SHEAP_FREE(pskvNode->SV_pcVarName);
    __SHEAP_FREE(pskvNode);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellVarDeleteByName
** 功能描述: 从 ttiny shell 系统中删除一个变量 (参数为名字)
** 输　入  : pcVarName     变量名
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __tshellVarDeleteByName (CPCHAR  pcVarName)
{
    REGISTER PLW_LIST_LINE        plineHash;
    REGISTER PLW_LIST_LINE       *pplineHashHeader = LW_NULL;
    REGISTER __PTSHELL_VAR        pskvNode = LW_NULL;                   /*  变量节点                    */
    REGISTER INT                  iHashVal;

    iHashVal = __hashHorner(pcVarName, LW_CFG_SHELL_VAR_HASH_SIZE);     /*  确定一阶散列的位置          */
    
    __TTINY_SHELL_LOCK();                                               /*  互斥访问                    */
    plineHash = _G_plineTSVarHeaderHashTbl[iHashVal];
    for (;
         plineHash != LW_NULL;
         plineHash  = _list_line_get_next(plineHash)) {
    
        pskvNode = _LIST_ENTRY(plineHash, __TSHELL_VAR,
                               SV_lineHash);
    
        if (lib_strcmp(pcVarName, pskvNode->SV_pcVarName) == 0) {       /*  变量名相同                  */
            break;
        }
    }
    
    if (plineHash == LW_NULL) {
        __TTINY_SHELL_UNLOCK();                                         /*  释放资源                    */
        _ErrorHandle(ERROR_TSHELL_EVAR);
        return  (ERROR_TSHELL_EVAR);                                    /*  没有找到变量                */
    }
    
    /*
     *  从管理链表中删除
     */
    _List_Line_Del(&pskvNode->SV_lineManage, 
                   &_G_plineTSVarHeader);                               /*  插入管理表头                */
                   
    pplineHashHeader = &_G_plineTSVarHeaderHashTbl[iHashVal];
    
    _List_Line_Del(&pskvNode->SV_lineHash, 
                   pplineHashHeader);                                   /*  删除                        */
    
    __TTINY_SHELL_UNLOCK();                                             /*  释放资源                    */
    
    __TSHELL_RUN_HOOK(pskvNode->SV_pcVarName);                          /*  调用回调                    */
    
    if (pskvNode->SV_ulRefCnt == 0) {                                   /*  有被直接引用过内存          */
        __SHEAP_FREE(pskvNode->SV_pcVarValue);
    }
    __SHEAP_FREE(pskvNode->SV_pcVarName);
    __SHEAP_FREE(pskvNode);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellVarFind
** 功能描述: 在 ttiny shell 系统查找一个变量.
** 输　入  : pcVarName     变量名
**           ppskvNode     关键字节点双指针
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __tshellVarFind (char  *pcVarName, __PTSHELL_VAR   *ppskvNode)
{
    REGISTER PLW_LIST_LINE        plineHash;
    REGISTER __PTSHELL_VAR        pskvNode = LW_NULL;                   /*  变量节点                    */
    REGISTER INT                  iHashVal;

    iHashVal = __hashHorner(pcVarName, LW_CFG_SHELL_VAR_HASH_SIZE);     /*  确定一阶散列的位置          */
    
    __TTINY_SHELL_LOCK();                                               /*  互斥访问                    */
    plineHash = _G_plineTSVarHeaderHashTbl[iHashVal];
    for (;
         plineHash != LW_NULL;
         plineHash  = _list_line_get_next(plineHash)) {
    
        pskvNode = _LIST_ENTRY(plineHash, __TSHELL_VAR,
                               SV_lineHash);
    
        if (lib_strcmp(pcVarName, pskvNode->SV_pcVarName) == 0) {       /*  变量名相同                  */
            break;
        }
    }
    
    if (plineHash == LW_NULL) {
        __TTINY_SHELL_UNLOCK();                                         /*  释放资源                    */
        _ErrorHandle(ERROR_TSHELL_EVAR);
        return  (ERROR_TSHELL_EVAR);                                    /*  没有找到变量                */
    }
    
    *ppskvNode = pskvNode;
    
    __TTINY_SHELL_UNLOCK();                                             /*  释放资源                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellVarList
** 功能描述: 获得链表中的所有变量控制块地址
** 输　入  : pskvNodeStart   起始节点地址, NULL 表示从头开始
**           ppskvNode[]     节点列表
**           iMaxCounter     列表中可以存放的最大节点数量
** 输　出  : 真实获取的节点数量
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __tshellVarList (__PTSHELL_VAR   pskvNodeStart,
                        __PTSHELL_VAR   ppskvNode[],
                        INT             iMaxCounter)
{
    REGISTER INT                  i = 0;
    
    REGISTER PLW_LIST_LINE        plineNode;
    REGISTER __PTSHELL_VAR        pskvNode = pskvNodeStart;             /*  变量节点                    */
    
    __TTINY_SHELL_LOCK();                                               /*  互斥访问                    */
    if (pskvNode == LW_NULL) {
        plineNode = _G_plineTSVarHeader;
    } else {
        plineNode = _list_line_get_next(&pskvNode->SV_lineManage);
    }
    
    for (;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode)) {                 /*  开始搜索                    */
        
        pskvNode = _LIST_ENTRY(plineNode, __TSHELL_VAR,
                               SV_lineManage);
                               
        ppskvNode[i++] = pskvNode;
        
        if (i >= iMaxCounter) {                                         /*  已经保存满了                */
            break;
        }
    }
    __TTINY_SHELL_UNLOCK();                                             /*  释放资源                    */
    
    return  ((ULONG)i);
}
/*********************************************************************************************************
** 函数名称: __tshellVarNum
** 功能描述: 获得 shell 变量个数
** 输　入  : NONE
** 输　出  : 变量个数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __tshellVarNum (VOID)
{
    REGISTER PLW_LIST_LINE        plineNode;
             INT                  iNum = 0;
    
    __TTINY_SHELL_LOCK();                                               /*  互斥访问                    */
    for (plineNode  = _G_plineTSVarHeader;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode)) {                 /*  开始搜索                    */
        iNum++;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  释放资源                    */
    
    return  (iNum);
}
/*********************************************************************************************************
** 函数名称: __tshellVarDup
** 功能描述: dup shell 变量
** 输　入  : pfuncStrdup       内存分配函数
**           ppcEvn            dup 目标
**           ulMax             最大个数
** 输　出  : dup 个数
** 全局变量: 
** 调用模块: 
** 注  意  : Han.hui 对当前获得 shell 环境变量快照的方法并不是很满意, 因为 shell 中回调了用户进程的函数.
*********************************************************************************************************/
INT   __tshellVarDup (PVOID (*pfuncMalloc)(size_t stSize), PCHAR  ppcEvn[], ULONG  ulMax)
{
             INT            iDupNum = 0;
    REGISTER PLW_LIST_LINE  plineNode;
    REGISTER __PTSHELL_VAR  pskvNode;
             PCHAR          pcLine;
    
    __TTINY_SHELL_LOCK();                                               /*  互斥访问                    */
    for (plineNode  = _G_plineTSVarHeader;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode)) {                 /*  开始搜索                    */
        
        size_t  stLen;
        size_t  stNameLen;
        size_t  stValueLen;
        
        pskvNode = _LIST_ENTRY(plineNode, __TSHELL_VAR,
                               SV_lineManage);
        if (iDupNum < ulMax) {
            stNameLen  = lib_strlen(pskvNode->SV_pcVarName);
            stValueLen = lib_strlen(pskvNode->SV_pcVarValue);
            stLen      = stNameLen + stValueLen + 2;
            
            LW_SOFUNC_PREPARE(pfuncMalloc);
            pcLine = (PCHAR)pfuncMalloc(stLen);
            if (pcLine == LW_NULL) {
                break;
            }
            
            lib_strcpy(pcLine, pskvNode->SV_pcVarName);
            lib_strcpy(&pcLine[stNameLen], "=");
            lib_strcpy(&pcLine[stNameLen + 1], pskvNode->SV_pcVarValue);
            
            ppcEvn[iDupNum] = pcLine;
            
            iDupNum++;
        
        } else {
            break;
        }
    }
    __TTINY_SHELL_UNLOCK();                                             /*  释放资源                    */
    
    return  (iDupNum);
}
/*********************************************************************************************************
** 函数名称: __tshellVarSave
** 功能描述: 将所有的环境变量保存到文件.
** 输　入  : pcFileName     需要保存的文件名 (有则更新, 无则创建, 一般为 "/etc/profile")
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __tshellVarSave (CPCHAR  pcFileName)
{
             FILE                *pfile;
    REGISTER PLW_LIST_LINE        plineNode;
    REGISTER __PTSHELL_VAR        pskvNode;                             /*  变量节点                    */
    
    pfile = fopen(pcFileName, "w");
    if (pfile == LW_NULL) {
        return  (PX_ERROR);
    }
    
    fprintf(pfile, "#sylixos envionment variables profile.\n");
    __TTINY_SHELL_LOCK();                                               /*  互斥访问                    */
    for (plineNode  = _G_plineTSVarHeader;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode)) {                 /*  开始遍历                    */
         
        pskvNode = _LIST_ENTRY(plineNode, __TSHELL_VAR,
                               SV_lineManage);
                               
        if ((lib_strcmp(pskvNode->SV_pcVarName, "SYSTEM")    == 0) ||
            (lib_strcmp(pskvNode->SV_pcVarName, "VERSION")   == 0) ||
            (lib_strcmp(pskvNode->SV_pcVarName, "LICENSE")   == 0) ||
            (lib_strcmp(pskvNode->SV_pcVarName, "TMPDIR")    == 0) ||
            (lib_strcmp(pskvNode->SV_pcVarName, "KERN_FLOAT") == 0)) {  /*  这些变量不允许保存          */
            continue;
        }

        /*
         *  TODO:这里没有判断变量的值中含有 " 的情况.
         */
        fprintf(pfile, "%s=\"%s\"\n", pskvNode->SV_pcVarName, pskvNode->SV_pcVarValue);
    }
    __TTINY_SHELL_UNLOCK();                                             /*  释放资源                    */
    
    fclose(pfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellVarLoad
** 功能描述: 从 profile 文件中读取所有环境变量的值 (一般为 "/etc/profile")
** 输　入  : pcFileName     profile 文件名
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __tshellVarLoad (CPCHAR  pcFileName)
{
    CHAR                 cLineBuffer[LW_CFG_SHELL_MAX_COMMANDLEN + 1];
    PCHAR                pcCmd;
    FILE                *pfile;
             
    pfile = fopen(pcFileName, "r");
    if (pfile == LW_NULL) {
        return  (PX_ERROR);
    }
    
    do {
        pcCmd = fgets(cLineBuffer, LW_CFG_SHELL_MAX_COMMANDLEN, 
                      pfile);                                           /*  获得一条指令                */
        if (pcCmd) {
            __tshellExec(pcCmd, LW_NULL);                               /*  变量赋值                    */
        }
    } while (pcCmd);
    
    fclose(pfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellVarGetRt
** 功能描述: 获得一个变量的值
** 输　入  : pcVarName       变量名
**           pcVarValue      变量的值双指针
** 输　出  : 变量值的长度
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __tshellVarGetRt (CPCHAR       pcVarName, 
                       PCHAR        pcVarValue,
                       INT          iMaxLen)
{
    REGISTER PLW_LIST_LINE        plineHash;
    REGISTER __PTSHELL_VAR        pskvNode = LW_NULL;                   /*  变量节点                    */
    REGISTER INT                  iHashVal;

    iHashVal = __hashHorner(pcVarName, LW_CFG_SHELL_VAR_HASH_SIZE);     /*  确定一阶散列的位置          */
    
    __TTINY_SHELL_LOCK();                                               /*  互斥访问                    */
    plineHash = _G_plineTSVarHeaderHashTbl[iHashVal];
    for (;
         plineHash != LW_NULL;
         plineHash  = _list_line_get_next(plineHash)) {
    
        pskvNode = _LIST_ENTRY(plineHash, __TSHELL_VAR,
                               SV_lineHash);
    
        if (lib_strcmp(pcVarName, pskvNode->SV_pcVarName) == 0) {       /*  变量名相同                  */
            break;
        }
    }
    
    if (plineHash == LW_NULL) {
        __TTINY_SHELL_UNLOCK();                                         /*  释放资源                    */
        _ErrorHandle(ERROR_TSHELL_EVAR);
        return  (PX_ERROR);                                             /*  没有找到变量                */
    }
    
    lib_strlcpy(pcVarValue, pskvNode->SV_pcVarValue, iMaxLen);
    
    __TTINY_SHELL_UNLOCK();                                             /*  释放资源                    */

    return  ((INT)lib_strlen(pcVarValue));
}
/*********************************************************************************************************
** 函数名称: __tshellVarGet
** 功能描述: 获得一个变量的值
** 输　入  : pcVarName       变量名
**           ppcVarValue     变量的值双指针
** 输　出  : 错误代码
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __tshellVarGet (CPCHAR  pcVarName, PCHAR  *ppcVarValue)
{
    REGISTER PLW_LIST_LINE        plineHash;
    REGISTER __PTSHELL_VAR        pskvNode = LW_NULL;                   /*  变量节点                    */
    REGISTER INT                  iHashVal;

    iHashVal = __hashHorner(pcVarName, LW_CFG_SHELL_VAR_HASH_SIZE);     /*  确定一阶散列的位置          */
    
    __TTINY_SHELL_LOCK();                                               /*  互斥访问                    */
    plineHash = _G_plineTSVarHeaderHashTbl[iHashVal];
    for (;
         plineHash != LW_NULL;
         plineHash  = _list_line_get_next(plineHash)) {
    
        pskvNode = _LIST_ENTRY(plineHash, __TSHELL_VAR,
                               SV_lineHash);
    
        if (lib_strcmp(pcVarName, pskvNode->SV_pcVarName) == 0) {       /*  变量名相同                  */
            break;
        }
    }
    
    if (plineHash == LW_NULL) {
        __TTINY_SHELL_UNLOCK();                                         /*  释放资源                    */
        _ErrorHandle(ERROR_TSHELL_EVAR);
        return  (ERROR_TSHELL_EVAR);                                    /*  没有找到变量                */
    }
    
    if (ppcVarValue) {
        *ppcVarValue = pskvNode->SV_pcVarValue;
        if (pskvNode->SV_ulRefCnt != __ARCH_ULONG_MAX) {
            pskvNode->SV_ulRefCnt++;                                    /*  引用计数                    */
        }
    }
    __TTINY_SHELL_UNLOCK();                                             /*  释放资源                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellVarSet
** 功能描述: 设置一个变量的值
** 输　入  : pcVarName       变量名
**           pcVarValue      变量的值
** 输　出  : 错误代码
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __tshellVarSet (CPCHAR       pcVarName, CPCHAR       pcVarValue, INT  iIsOverwrite)
{
    REGISTER PLW_LIST_LINE        plineHash;
    REGISTER __PTSHELL_VAR        pskvNode = LW_NULL;                   /*  变量节点                    */
    REGISTER INT                  iHashVal;
    REGISTER size_t               stValueStrLen;
    REGISTER PCHAR                pcNewBuffer;

    iHashVal = __hashHorner(pcVarName, LW_CFG_SHELL_VAR_HASH_SIZE);     /*  确定一阶散列的位置          */
    
    __TTINY_SHELL_LOCK();                                               /*  互斥访问                    */
    plineHash = _G_plineTSVarHeaderHashTbl[iHashVal];
    for (;
         plineHash != LW_NULL;
         plineHash  = _list_line_get_next(plineHash)) {
    
        pskvNode = _LIST_ENTRY(plineHash, __TSHELL_VAR,
                               SV_lineHash);
    
        if (lib_strcmp(pcVarName, pskvNode->SV_pcVarName) == 0) {       /*  变量名相同                  */
            break;
        }
    }
    
    if (plineHash == LW_NULL) {
        __TTINY_SHELL_UNLOCK();                                         /*  释放资源                    */
        _ErrorHandle(ERROR_TSHELL_EVAR);
        return  (ERROR_TSHELL_EVAR);                                    /*  没有找到变量                */
    }
    
    if ((iIsOverwrite == 0) && 
        (lib_strlen(pskvNode->SV_pcVarValue) > 0)) {                    /*  不能被改写                  */
        __TTINY_SHELL_UNLOCK();                                         /*  释放资源                    */
        _ErrorHandle(ERROR_TSHELL_CANNT_OVERWRITE);
        return  (ERROR_TSHELL_CANNT_OVERWRITE);
    }
    
    stValueStrLen = lib_strlen(pcVarValue);
    
    pcNewBuffer = (PCHAR)__SHEAP_ALLOC(stValueStrLen + 1);              /*  开辟新的缓冲区              */
    if (!pcNewBuffer) {
        __TTINY_SHELL_UNLOCK();                                         /*  释放资源                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (ERROR_SYSTEM_LOW_MEMORY);                              /*  缺少内存                    */
    }
    lib_strcpy(pcNewBuffer, pcVarValue);
    
    if (pskvNode->SV_ulRefCnt == 0) {                                   /*  有被直接引用过内存          */
        __SHEAP_FREE(pskvNode->SV_pcVarValue);
    }
    pskvNode->SV_pcVarValue = pcNewBuffer;                              /*  设置新的变量值连接          */
    
    __TTINY_SHELL_UNLOCK();                                             /*  释放资源                    */

    __TSHELL_RUN_HOOK(pcVarName);                                       /*  调用改写回调                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellVarDefine
** 功能描述: 通过向 ttiny shell 系统添加一个变量,(此函数仅被 API_TShellExec 调用, 缓冲区是可写的)
** 输　入  : pcCmd         shell 指令
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __tshellVarDefine (PCHAR  pcCmd)
{
    REGISTER PCHAR      pcTemp  = pcCmd;
    REGISTER PCHAR      pcTemp2 = pcCmd;
    
    REGISTER PCHAR      pcVarName  = (PCHAR)pcCmd;
    REGISTER PCHAR      pcVarValue = LW_NULL;
             PCHAR      pcTokenNext;
             
    REGISTER ULONG      ulError;
    
    for (; *pcTemp != PX_EOS; pcTemp++) {                               /*  扫描赋值符号                */
        if (*pcTemp == '=') {
            break;
        }
    }
    
    if ((*pcTemp == PX_EOS) || (pcTemp == pcCmd)) {                     /*  非变量赋值或变量名错误      */
        _ErrorHandle(ERROR_TSHELL_CMDNOTFUND);
        return  (ERROR_TSHELL_CMDNOTFUND);
    }
    
    do {
        if (((*pcTemp2 >= 'a') && (*pcTemp2 <= 'z')) ||
            ((*pcTemp2 >= 'A') && (*pcTemp2 <= 'Z')) ||
            ((*pcTemp2 >= '0') && (*pcTemp2 <= '9')) ||
            ((*pcTemp2 == '_'))) {                                      /*  合法变量名字符              */
            pcTemp2++;
        } else {
            break;
        }
    } while (pcTemp2 != pcTemp);                                        /*  开始检查变量名是否合法      */
    
    if (pcTemp2 != pcTemp) {                                            /*  变量名含有非法字符          */
        _ErrorHandle(ERROR_TSHELL_EVAR);
        return  (ERROR_TSHELL_EVAR);
    }
    
    *pcTemp    = PX_EOS;                                                /*  变量名结束                  */
    pcVarValue = pcTemp + 1;                                            /*  获得变量赋值字符串          */
    
    if ((*pcVarValue == ' ') ||
        (*pcVarValue == '\t')) {                                        /*  必须紧跟变量赋值字串        */
        _ErrorHandle(ERROR_TSHELL_EKEYWORD);
        return  (ERROR_TSHELL_EKEYWORD);
    }
    
    __tshellStrGetToken(pcVarValue, &pcTokenNext);                      /*  获得赋值字串                */
    __tshellStrDelTransChar(pcVarValue, pcVarValue);                    /*  删除转移字符                */
    
    ulError = __tshellVarSet(pcVarName, pcVarValue, 1);                 /*  设置变量的值                */
    if (ulError == ERROR_TSHELL_EVAR) {
        ulError = __tshellVarAdd(pcVarName, pcVarValue, 
                                 lib_strlen(pcVarName));                /*  创建新变量                  */
    }
    
    return  (ulError);
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
