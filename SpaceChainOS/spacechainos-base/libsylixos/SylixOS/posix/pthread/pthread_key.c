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
** 文   件   名: pthread_key.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 30 日
**
** 描        述: pthread 私有数据兼容库.

** BUG:
2012.12.07  加入资源管理节点.
2013.05.01  If successful, the pthread_key_*() function shall store the newly created key value at *key 
            and shall return zero. Otherwise, an error number shall be returned to indicate the error.
2013.05.02  修正 destructor 调用的参数和时机.
2014.12.09  强制被杀死的进程不执行 desturtors.
2017.03.15  使用 hash 表, 提高查询速度.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_pthread.h"                                      /*  已包含操作系统头文件        */
#include "../include/posixLib.h"                                        /*  posix 内部公共库            */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
  进程相关
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  key HASH
*********************************************************************************************************/
#define __PX_KEY_THREAD_HASH_SIZE   16
#define __PX_KEY_THREAD_HASH_MASK   (__PX_KEY_THREAD_HASH_SIZE - 1)
#define __PX_KEY_THREAD_HASH(t)     ((INT)(t) & __PX_KEY_THREAD_HASH_MASK)
/*********************************************************************************************************
  key 私有数据类型
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            PKEYN_lineManage;                           /*  所有 key 键链表             */
    long                    PKEYN_lId;                                  /*  key id                      */
    void                  (*PKEYN_pfuncDestructor)(void *);             /*  destructor                  */
    LW_LIST_LINE_HEADER     PKEYN_plineKeyHeader[__PX_KEY_THREAD_HASH_SIZE];
                                                                        /*  所有线程私有数据指针        */
    LW_OBJECT_HANDLE        PKEYN_ulMutex;                              /*  互斥量                      */
    LW_RESOURCE_RAW         PKEYN_resraw;                               /*  资源管理节点                */
} __PX_KEY_NODE;

static LW_LIST_LINE_HEADER  _G_plineKeyHeader;                          /*  所有的 key 键链表           */

#define __PX_KEY_LOCK(pkeyn)        API_SemaphoreMPend(pkeyn->PKEYN_ulMutex, LW_OPTION_WAIT_INFINITE)
#define __PX_KEY_UNLOCK(pkeyn)      API_SemaphoreMPost(pkeyn->PKEYN_ulMutex)
/*********************************************************************************************************
  协程删除回调函数声明
*********************************************************************************************************/
static BOOL  _G_bKeyDelHookAdd = LW_FALSE;
static VOID  __pthreadDataDeleteByThread(LW_OBJECT_HANDLE  ulId, PVOID  pvRetVal, PLW_CLASS_TCB  ptcbDel);
/*********************************************************************************************************
** 函数名称: __pthreadKeyOnce
** 功能描述: 创建 POSIX 线程
** 输　入  : lId           键id
**           pvData        初始数据
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __pthreadKeyOnce (VOID)
{
    API_SystemHookAdd(__pthreadDataDeleteByThread, LW_OPTION_THREAD_DELETE_HOOK);
}
/*********************************************************************************************************
** 函数名称: __pthreadDataSet
** 功能描述: 设置指定 key 在当前线程内部数据节点. (无则创建)
** 输　入  : lId           键id
**           pvData        初始数据
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __pthreadDataSet (long  lId, const void  *pvData)
{
    REGISTER INT         iHash;
    __PX_KEY_NODE       *pkeyn = (__PX_KEY_NODE *)lId;
    __PX_KEY_DATA       *pkeyd;
    PLW_CLASS_TCB        ptcbCur;
    LW_OBJECT_HANDLE     ulMe;
    PLW_LIST_LINE        plineTemp;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    ulMe = ptcbCur->TCB_ulId;
    
    /*
     *  查找是否已经创建了相关私有数据
     */
    iHash = __PX_KEY_THREAD_HASH(ulMe);

    __PX_KEY_LOCK(pkeyn);                                               /*  锁住 key 键                 */
    for (plineTemp  = pkeyn->PKEYN_plineKeyHeader[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pkeyd = (__PX_KEY_DATA *)plineTemp;                             /*  链表是 KEY DATA 的第一个元素*/
        if (pkeyd->PKEYD_ulOwner == ulMe) {                             /*  找到对应当前线程的数据      */
            pkeyd->PKEYD_pvData = (void *)pvData;
            break;
        }
    }
    __PX_KEY_UNLOCK(pkeyn);                                             /*  解锁 key 键                 */

    if (plineTemp) {
        return  (ERROR_NONE);                                           /*  已经找到了对应的节点        */
    }
    
    /*
     *  如果没有找到, 则需要新建私有数据
     */
    pkeyd = (__PX_KEY_DATA  *)__SHEAP_ALLOC(sizeof(__PX_KEY_DATA));     /*  没有节点, 需要新建          */
    if (pkeyd == LW_NULL) {
        errno = ENOMEM;
        return  (ENOMEM);
    }
    pkeyd->PKEYD_lId     = lId;                                         /*  通过 id 反向查找 key        */
    pkeyd->PKEYD_pvData  = (void *)pvData;
    pkeyd->PKEYD_ulOwner = ulMe;                                        /*  记录线程 ID                 */
    
    __PX_KEY_LOCK(pkeyn);                                               /*  锁住 key 键                 */
    _List_Line_Add_Ahead(&pkeyd->PKEYD_lineManage, 
                         &pkeyn->PKEYN_plineKeyHeader[iHash]);          /*  加入对应 key 键链表         */
    __PX_KEY_UNLOCK(pkeyn);                                             /*  解锁 key 键                 */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pthreadDataGet
** 功能描述: 获取指定 key 在当前线程内部数据节点.
** 输　入  : lId           键id
**           ppvData       初始数据(返回)
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __pthreadDataGet (long  lId, void  **ppvData)
{
    REGISTER INT         iHash;
    __PX_KEY_NODE       *pkeyn = (__PX_KEY_NODE *)lId;
    __PX_KEY_DATA       *pkeyd;
    LW_OBJECT_HANDLE     ulMe  = API_ThreadIdSelf();
    PLW_LIST_LINE        plineTemp;
    
    if (ppvData == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *ppvData = LW_NULL;
    
    iHash = __PX_KEY_THREAD_HASH(ulMe);

    __PX_KEY_LOCK(pkeyn);                                               /*  锁住 key 键                 */
    for (plineTemp  = pkeyn->PKEYN_plineKeyHeader[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pkeyd = (__PX_KEY_DATA *)plineTemp;                             /*  链表是 KEY DATA 的第一个元素*/
        if (pkeyd->PKEYD_ulOwner == ulMe) {                             /*  找到对应当前线程的数据      */
            *ppvData = pkeyd->PKEYD_pvData;
            break;
        }
    }
    __PX_KEY_UNLOCK(pkeyn);                                             /*  解锁 key 键                 */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pthreadDataDeleteByKey
** 功能描述: 删除指定 key 键的所有数据节点.
** 输　入  : pkeyn        KEY NODE
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __pthreadDataDeleteByKey (__PX_KEY_NODE  *pkeyn)
{
    __PX_KEY_DATA       *pkeyd;
    INT                  i;
    
    /*
     *  key 键删除, 需要遍历 key 键的私有数据表, 删除所有私有数据
     */
    __PX_KEY_LOCK(pkeyn);                                               /*  锁住 key 键                 */
    for (i = 0; i < __PX_KEY_THREAD_HASH_SIZE; i++) {
        while (pkeyn->PKEYN_plineKeyHeader[i]) {
            pkeyd = (__PX_KEY_DATA *)pkeyn->PKEYN_plineKeyHeader[i];    /*  链表是 KEY DATA 的第一个元素*/

            _List_Line_Del(&pkeyd->PKEYD_lineManage,
                           &pkeyn->PKEYN_plineKeyHeader[i]);            /*  从链表中删除                */

            __SHEAP_FREE(pkeyd);                                        /*  释放线程私有数据内存        */
        }
    }
    __PX_KEY_UNLOCK(pkeyn);                                             /*  解锁 key 键                 */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pthreadDataDeleteByThread
** 功能描述: 删除所有与当前线程相关的内部数据节点.
** 输　入  : ulId      被删除任务句柄
**           pvRetVal  任务返回值
**           ptcbDel   被删除任务控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __pthreadDataDeleteByThread (LW_OBJECT_HANDLE  ulId, PVOID  pvRetVal, PLW_CLASS_TCB  ptcbDel)
{
    REGISTER INT         iHash;
    __PX_KEY_NODE       *pkeyn;
    __PX_KEY_DATA       *pkeyd;
    PLW_LIST_LINE        plineTempK;
    PLW_LIST_LINE        plineTempD;
    VOIDFUNCPTR          pfuncDestructor;
    
    PVOID                pvPrevValue;
    BOOL                 bCall = LW_TRUE;
    
#if LW_CFG_MODULELOADER_EN > 0
    LW_LD_VPROC         *pvprocDel;

    pvprocDel = __LW_VP_GET_TCB_PROC(ptcbDel);
    if (pvprocDel && pvprocDel->VP_bImmediatelyTerm) {                  /*  进程不需要执行 destructor   */
        bCall = LW_FALSE;
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */

    /*
     *  线程删除, 需要遍历所有 key 键的私有数据表, 删除与本线程相关的私有数据
     */
    iHash = __PX_KEY_THREAD_HASH(ulId);

__re_check:
    __PX_LOCK();                                                        /*  锁住 posix 库               */
    for (plineTempK  = _G_plineKeyHeader;
         plineTempK != LW_NULL;
         plineTempK  = _list_line_get_next(plineTempK)) {               /*  遍历所有 key 键             */
        
        pkeyn = (__PX_KEY_NODE *)plineTempK;
        
        plineTempD = pkeyn->PKEYN_plineKeyHeader[iHash];                /*  遍历 key 键内的所有节点     */
        while (plineTempD) {
            pkeyd = (__PX_KEY_DATA *)plineTempD;
            plineTempD  = _list_line_get_next(plineTempD);
            
            if (pkeyd->PKEYD_ulOwner == ulId) {                         /*  是否为当前线程数据节点      */
                if (pkeyn->PKEYN_pfuncDestructor &&
                    pkeyd->PKEYD_pvData) {                              /*  需要调用 destructor         */
                    pvPrevValue = pkeyd->PKEYD_pvData;
                    pkeyd->PKEYD_pvData = LW_NULL;                      /*  下次不再调用 destructor     */
                    pfuncDestructor = pkeyn->PKEYN_pfuncDestructor;
                    __PX_UNLOCK();                                      /*  解锁 posix 库               */
                    
                    if (pfuncDestructor && bCall) {                     /*  调用删除函数                */
                        LW_SOFUNC_PREPARE(pfuncDestructor);
                        pfuncDestructor(pvPrevValue);
                    }
                    goto    __re_check;                                 /*  重新检查                    */
                }
                
                _List_Line_Del(&pkeyd->PKEYD_lineManage,
                               &pkeyn->PKEYN_plineKeyHeader[iHash]);    /*  从链表中删除                */
                __SHEAP_FREE(pkeyd);                                    /*  释放线程私有数据内存        */
            }
        }
    }
    __PX_UNLOCK();                                                      /*  解锁 posix 库               */
}
/*********************************************************************************************************
** 函数名称: pthread_key_create
** 功能描述: 创建一个数据键.
** 输　入  : pkey          键 (返回)
**           fdestructor   删除函数
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : key 信号量设置为 LW_OPTION_OBJECT_GLOBAL 是因为 key 已经使用了原始资源进行回收.
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_key_create (pthread_key_t  *pkey, void (*fdestructor)(void *))
{
    INT              i;
    __PX_KEY_NODE   *pkeyn;
    
    if (pkey == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    API_ThreadOnce(&_G_bKeyDelHookAdd, __pthreadKeyOnce);               /*  安装线程删除回调            */
    
    pkeyn = (__PX_KEY_NODE *)__SHEAP_ALLOC(sizeof(__PX_KEY_NODE));      /*  创建节点内存                */
    if (pkeyn == LW_NULL) {
        errno = ENOMEM;
        return  (ENOMEM);
    }
    pkeyn->PKEYN_lId             = (long)pkeyn;
    pkeyn->PKEYN_pfuncDestructor = fdestructor;
    pkeyn->PKEYN_ulMutex         = API_SemaphoreMCreate("pxkey", LW_PRIO_DEF_CEILING, 
                                            LW_OPTION_WAIT_PRIORITY |
                                            LW_OPTION_INHERIT_PRIORITY |
                                            LW_OPTION_DELETE_SAFE |
                                            LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pkeyn->PKEYN_ulMutex == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pkeyn);
        errno = EAGAIN;
        return  (EAGAIN);
    }
    
    for (i = 0; i < __PX_KEY_THREAD_HASH_SIZE; i++) {
        pkeyn->PKEYN_plineKeyHeader[i] = LW_NULL;
    }

    __PX_LOCK();                                                        /*  锁住 posix 库               */
    _List_Line_Add_Ahead(&pkeyn->PKEYN_lineManage,
                         &_G_plineKeyHeader);                           /*  加入 key 键链表             */
    __PX_UNLOCK();                                                      /*  解锁 posix 库               */
    
    __resAddRawHook(&pkeyn->PKEYN_resraw, (VOIDFUNCPTR)pthread_key_delete, 
                    pkeyn, 0, 0, 0, 0, 0);                              /*  加入资源管理器              */
    
    *pkey = (pthread_key_t)pkeyn;                                       /*  将内存地址定义为 id         */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_key_delete
** 功能描述: 删除一个数据键. (注意, 删除函数不会调用创建时安装的析构函数)
** 输　入  : key          键
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_key_delete (pthread_key_t  key)
{
    __PX_KEY_NODE   *pkeyn = (__PX_KEY_NODE *)key;

    if (key == 0) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthreadDataDeleteByKey(pkeyn);                                    /*  删除所有与此 key 相关的数据 */
    
    __PX_LOCK();                                                        /*  锁住 posix 库               */
    _List_Line_Del(&pkeyn->PKEYN_lineManage,
                   &_G_plineKeyHeader);                                 /*  从 key 键链表中删除         */
    __PX_UNLOCK();                                                      /*  解锁 posix 库               */
    
    API_SemaphoreMDelete(&pkeyn->PKEYN_ulMutex);
    
    __resDelRawHook(&pkeyn->PKEYN_resraw);
    
    __SHEAP_FREE(pkeyn);                                                /*  释放 key                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_setspecific
** 功能描述: 设定一个数据键指定当前线程的私有数据.
** 输　入  : key          键
**           pvalue       值
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_setspecific (pthread_key_t  key, const void  *pvalue)
{
    if (key == 0) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (__pthreadDataSet(key, pvalue));
}
/*********************************************************************************************************
** 函数名称: pthread_getspecific
** 功能描述: 获取一个数据键指定当前线程的私有数据.
** 输　入  : key          键
**           pvalue       值
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
void *pthread_getspecific (pthread_key_t  key)
{
    void   *pvalue = LW_NULL;

    if (key == 0) {
        errno = EINVAL;
        return  (LW_NULL);
    }
    
    __pthreadDataGet(key, &pvalue);
    
    return  (pvalue);
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
