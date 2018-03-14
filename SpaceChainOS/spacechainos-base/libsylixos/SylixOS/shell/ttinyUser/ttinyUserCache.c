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
** 文   件   名: ttinyUserAuthen.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 05 月 21 日
**
** 描        述: shell 用户名组名的缓冲
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "shadow.h"
#include "pwd.h"
#include "grp.h"
#include "../ttinyShell/ttinyShellLib.h"
/*********************************************************************************************************
  用户/组缓冲
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            UC_lineManage;
    uid_t                   UC_uid;
    PCHAR                   UC_pcHome;
    CHAR                    UC_cName[1];
} __TSHELL_UCACHE;
typedef __TSHELL_UCACHE    *__PTSHELL_UCACHE;

typedef struct {
    LW_LIST_LINE            GC_lineManage;
    gid_t                   GC_gid;
    CHAR                    GC_cName[1];
} __TSHELL_GCACHE;
typedef __TSHELL_GCACHE    *__PTSHELL_GCACHE;
/*********************************************************************************************************
  链表
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineUsrCache;
static LW_LIST_LINE_HEADER  _G_plineGrpCache;
/*********************************************************************************************************
** 函数名称: __tshellGetUserName
** 功能描述: 获得一个用户名
** 输　入  : uid           用户 id
**           pcName        用户名
**           stNSize       用户名缓冲区大小
**           pcHome        用户主目录
**           stHSize       用户主目录缓冲区大小
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __tshellGetUserName (uid_t  uid, PCHAR  pcName, size_t  stNSize, PCHAR  pcHome, size_t  stHSize)
{
    PLW_LIST_LINE       plineTmp;
    __PTSHELL_UCACHE    puc;
    struct passwd       passwd;
    struct passwd      *ppasswd = LW_NULL;
    CHAR                cBuffer[MAX_FILENAME_LENGTH];
    
    __TTINY_SHELL_LOCK();                                               /*  互斥访问                    */
    for (plineTmp  = _G_plineUsrCache;
         plineTmp != LW_NULL;
         plineTmp  = _list_line_get_next(plineTmp)) {
        
        puc = _LIST_ENTRY(plineTmp, __TSHELL_UCACHE, UC_lineManage);
        if (puc->UC_uid == uid) {
            if (pcName && stNSize) {
                lib_strlcpy(pcName, puc->UC_cName, stNSize);
            }
            if (pcHome && stHSize) {
                lib_strlcpy(pcHome, puc->UC_pcHome, stHSize);
            }
            if (plineTmp != _G_plineUsrCache) {
                _List_Line_Del(&puc->UC_lineManage, 
                               &_G_plineUsrCache);
                _List_Line_Add_Ahead(&puc->UC_lineManage, 
                                     &_G_plineUsrCache);                /*  最近使用的放在表头          */
            }
            __TTINY_SHELL_UNLOCK();                                     /*  释放资源                    */
            return  (ERROR_NONE);
        }
    }
    __TTINY_SHELL_UNLOCK();                                             /*  释放资源                    */
    
    getpwuid_r(uid, &passwd, cBuffer, sizeof(cBuffer), &ppasswd);
    if (!ppasswd) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    puc = (__PTSHELL_UCACHE)__SHEAP_ALLOC(sizeof(__TSHELL_UCACHE) + 
                                          lib_strlen(ppasswd->pw_name));
    if (!puc) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    puc->UC_pcHome = (PCHAR)__SHEAP_ALLOC(lib_strlen(ppasswd->pw_dir) + 1);
    if (!puc->UC_pcHome) {
        __SHEAP_FREE(puc);
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
                                          
    puc->UC_uid = uid;
    lib_strcpy(puc->UC_cName,  ppasswd->pw_name);                       /*  这里可能会有重复添加, 没问题*/
    lib_strcpy(puc->UC_pcHome, ppasswd->pw_dir);
    
    if (pcName && stNSize) {
        lib_strlcpy(pcName, puc->UC_cName,  stNSize);
    }
    
    if (pcHome && stHSize) {
        lib_strlcpy(pcHome, puc->UC_pcHome, stHSize);
    }
    
    __TTINY_SHELL_LOCK();                                               /*  互斥访问                    */
    _List_Line_Add_Ahead(&puc->UC_lineManage, 
                         &_G_plineUsrCache);                            /*  最近使用的放在表头          */
    __TTINY_SHELL_UNLOCK();                                             /*  释放资源                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellGetGrpName
** 功能描述: 获得一个组名
** 输　入  : gid           组 id
**           pcName        组名
**           stSize        缓冲区大小
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __tshellGetGrpName (gid_t  gid, PCHAR  pcName, size_t  stSize)
{
    PLW_LIST_LINE       plineTmp;
    __PTSHELL_GCACHE    pgc;
    struct group        group;
    struct group       *pgroup = LW_NULL;
    CHAR                cBuffer[MAX_FILENAME_LENGTH];

    __TTINY_SHELL_LOCK();                                               /*  互斥访问                    */
    for (plineTmp  = _G_plineGrpCache;
         plineTmp != LW_NULL;
         plineTmp  = _list_line_get_next(plineTmp)) {
        
        pgc = _LIST_ENTRY(plineTmp, __TSHELL_GCACHE, GC_lineManage);
        if (pgc->GC_gid == gid) {
            if (pcName && stSize) {
                lib_strlcpy(pcName, pgc->GC_cName, stSize);
            }
            if (plineTmp != _G_plineGrpCache) {
                _List_Line_Del(&pgc->GC_lineManage, 
                               &_G_plineGrpCache);
                _List_Line_Add_Ahead(&pgc->GC_lineManage, 
                                     &_G_plineGrpCache);                /*  最近使用的放在表头          */
            }
            __TTINY_SHELL_UNLOCK();                                     /*  释放资源                    */
            return  (ERROR_NONE);
        }
    }
    __TTINY_SHELL_UNLOCK();                                             /*  释放资源                    */
    
    getgrgid_r(gid, &group, cBuffer, sizeof(cBuffer), &pgroup);
    if (!pgroup) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    pgc = (__PTSHELL_GCACHE)__SHEAP_ALLOC(sizeof(__TSHELL_GCACHE) + 
                                          lib_strlen(pgroup->gr_name));
    if (!pgc) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
                                          
    pgc->GC_gid = gid;
    lib_strcpy(pgc->GC_cName, pgroup->gr_name);                         /*  这里可能会有重复添加, 没问题*/
    
    if (pcName && stSize) {
        lib_strlcpy(pcName, pgc->GC_cName, stSize);
    }
    
    __TTINY_SHELL_LOCK();                                               /*  互斥访问                    */
    _List_Line_Add_Ahead(&pgc->GC_lineManage, 
                         &_G_plineGrpCache);                            /*  最近使用的放在表头          */
    __TTINY_SHELL_UNLOCK();                                             /*  释放资源                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellFlushCache
** 功能描述: 删除所有 shell 缓冲的用户与组信息
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __tshellFlushCache (VOID)
{
    __PTSHELL_UCACHE    puc;
    __PTSHELL_GCACHE    pgc;

    __TTINY_SHELL_LOCK();                                               /*  互斥访问                    */
    while (_G_plineUsrCache) {
        puc = _LIST_ENTRY(_G_plineUsrCache, __TSHELL_UCACHE, UC_lineManage);
        _List_Line_Del(&puc->UC_lineManage, &_G_plineUsrCache);
        __SHEAP_FREE(puc->UC_pcHome);
        __SHEAP_FREE(puc);
    }
    while (_G_plineGrpCache) {
        pgc = _LIST_ENTRY(_G_plineGrpCache, __TSHELL_GCACHE, GC_lineManage);
        _List_Line_Del(&pgc->GC_lineManage, &_G_plineGrpCache);
        __SHEAP_FREE(pgc);
    }
    __TTINY_SHELL_UNLOCK();                                             /*  释放资源                    */
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
