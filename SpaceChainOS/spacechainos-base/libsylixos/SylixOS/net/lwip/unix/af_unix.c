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
** 文   件   名: af_unix.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 12 月 18 日
**
** 描        述: AF_UNIX 支持

** BUG:
2012.12.27  UNIX 规定 len 长度不包含 sun_path 中的 '\0' 字符.
2012.12.28  加入传递文件描述符和传递进程证书的功能.
2012.12.29  发送时如果 flags 没有 MSG_NOSIGNAL 则遇到面向连接不可达的管道需要发送 SIGPIPE 信号.
2012.12.31  使用小内存缓冲区时优先从内存池中分配.
2013.01.16  unix_sendto2 如果没有错误发生, 则确保完全发送完毕再退出.
2013.01.20  为符合 unix 标准, connect 如果使用 NBIO 模式, 则如果服务器还没有相应则 errno 为 EINPROGRESS.
            根据 BSD socket 规范, 提交 select 激活时, 必须要更新 SO_ERROR 用以判断 NBIO 的正确情况.
2013.03.29  修正 unix_listen 返回值错误.
            修正 unix_get/setsockopt 缺少一个 break;
2013.04.01  修正 __unix_have_event 返回值错误.
2013.04.30  精确 connect 操作的行为.
2013.06.21  加入遍历所有 unix 控制块的功能.
2013.09.06  修正 ECONNREFUSED 与 ECONNRESET 的使用.
            [ECONNREFUSED]
            The target address was not listening for connections or refused the connection request.
            [ECONNRESET]
            Remote host reset the connection request.
            unix_accept() 在没有足够内存时, 需要拒绝所有等待的连接.
2013.11.17  支持 SOCK_SEQPACKET 类型连接.
2013.11.21  升级新的发送信号接口.
2014.10.16  __unixFind() 加入对 listen 状态 unix 套接字的搜索.
2015.01.01  修正 __unixFind() 对 DGRAM 类型判断错误.
2015.01.08  SOCK_DGRAM 没有一次接收完整包, 剩下的数据需要丢弃.
2016.12.14  accept connect 使用不同的阻塞信号量.
2017.08.01  今天中国人民解放军建军 90 周年纪念日, 祝愿祖国蒸蒸日上.
            AF_UNIX 支持多线程并行读写.
2017.08.31  shutdown 写后, 远程端有机会读出最后暂存的数据.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_UNIX_EN > 0
#include "limits.h"
#include "sys/socket.h"
#include "sys/un.h"
#include "af_unix.h"
#include "lwip/mem.h"
#include "af_unix_msg.h"
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
extern void  __unix_socket_event(AF_UNIX_T  *pafunix, LW_SEL_TYPE type, INT  iSoErr);
/*********************************************************************************************************
  宏配置
*********************************************************************************************************/
#define __AF_UNIX_ADDROFFSET        offsetof(struct sockaddr_un, sun_path)
#define __AF_UNIX_DEF_FLAG          0777
#define __AF_UNIX_DEF_BUFSIZE       (LW_CFG_KB_SIZE * 64)               /*  默认为 64K 接收缓冲         */
#define __AF_UNIX_DEF_BUFMAX        (LW_CFG_KB_SIZE * 256)              /*  默认为 256K 接收缓冲        */
#define __AF_UNIX_DEF_BUFMIN        (LW_CFG_KB_SIZE * 8)                /*  最小接收缓冲大小            */

#ifdef __SYLIXOS_LITE
#define __AF_UNIX_PIPE_BUF          (LW_CFG_KB_SIZE * 2)                /*  一次原子操作的数据大小      */
#define __AF_UNIX_PIPE_BUF_SHIFT    11                                  /*  1 << 11 == 2K               */
#else
#define __AF_UNIX_PIPE_BUF          (LW_CFG_KB_SIZE * 8)                /*  一次原子操作的数据大小      */
#define __AF_UNIX_PIPE_BUF_SHIFT    13                                  /*  1 << 13 == 8K               */
#endif

#define __AF_UNIX_PART_256          LW_CFG_AF_UNIX_256_POOLS            /*  256 字节内存池数量          */
#define __AF_UNIX_PART_512          LW_CFG_AF_UNIX_512_POOLS            /*  512 字节内存池数量          */
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_LIST_LINE_HEADER          _G_plineAfUnix;
static LW_OBJECT_HANDLE             _G_hAfUnixMutex;

#define __AF_UNIX_LOCK()            API_SemaphoreMPend(_G_hAfUnixMutex, LW_OPTION_WAIT_INFINITE)
#define __AF_UNIX_UNLOCK()          API_SemaphoreMPost(_G_hAfUnixMutex)

static LW_OBJECT_HANDLE             _G_hAfUnixPart256;
static LW_OBJECT_HANDLE             _G_hAfUnixPart512;

#define __AF_UNIX_PART_256_SIZE     (__AF_UNIX_PART_256 * (256 / sizeof(LW_STACK)))
#define __AF_UNIX_PART_512_SIZE     (__AF_UNIX_PART_512 * (512 / sizeof(LW_STACK)))

static LW_STACK                    *_G_pstkUnixPart256;
static LW_STACK                    *_G_pstkUnixPart512;
/*********************************************************************************************************
  宏操作
  
  connect 阻塞借助于 UNIX_hCanWrite
  accept  阻塞借助于 UNIX_hCanRead
*********************************************************************************************************/
#define __AF_UNIX_WCONN(pafunix)    API_SemaphoreBPend(pafunix->UNIX_hCanWrite, \
                                                       pafunix->UNIX_ulConnTimeout)
#define __AF_UNIX_SCONN(pafunix)    API_SemaphoreBPost(pafunix->UNIX_hCanWrite)
#define __AF_UNIX_CCONN(pafunix)    API_SemaphoreBClear(pafunix->UNIX_hCanWrite)
                                                       
#define __AF_UNIX_WACCE(pafunix)    API_SemaphoreBPend(pafunix->UNIX_hCanRead, \
                                                       LW_OPTION_WAIT_INFINITE)
#define __AF_UNIX_SACCE(pafunix)    API_SemaphoreBPost(pafunix->UNIX_hCanRead)
#define __AF_UNIX_CACCE(pafunix)    API_SemaphoreBClear(pafunix->UNIX_hCanRead)
                                                       
#define __AF_UNIX_WREAD(pafunix)    API_SemaphoreBPend(pafunix->UNIX_hCanRead, \
                                                       pafunix->UNIX_ulRecvTimeout)
#define __AF_UNIX_SREAD(pafunix)    API_SemaphoreBPost(pafunix->UNIX_hCanRead)
#define __AF_UNIX_CREAD(pafunix)    API_SemaphoreBClear(pafunix->UNIX_hCanRead)
                                                       
#define __AF_UNIX_WWRITE(pafunix)   API_SemaphoreBPend(pafunix->UNIX_hCanWrite, \
                                                       pafunix->UNIX_ulSendTimeout)
#define __AF_UNIX_SWRITE(pafunix)   API_SemaphoreBPost(pafunix->UNIX_hCanWrite)
#define __AF_UNIX_CWRITE(pafunix)   API_SemaphoreBClear(pafunix->UNIX_hCanWrite)
/*********************************************************************************************************
  等待判断
*********************************************************************************************************/
#define __AF_UNIX_IS_NBIO(pafunix, flags)   \
        ((pafunix->UNIX_iFlag & O_NONBLOCK) || (flags & MSG_DONTWAIT))
        
#define __AF_UNIX_TYPE(pafunix)     (pafunix->UNIX_iType)
/*********************************************************************************************************
** 函数名称: __unixBufAlloc
** 功能描述: 分配内存
** 输　入  : stLen                 长度
** 输　出  : 创建出来的信息节点
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PVOID  __unixBufAlloc (size_t  stLen)
{
    PVOID   pvMem;

    if (stLen <= 256) {
        pvMem = API_PartitionGet(_G_hAfUnixPart256);
        if (pvMem) {
            return  (pvMem);
        }
    }
    if (stLen <= 512) {
        pvMem = API_PartitionGet(_G_hAfUnixPart512);
        if (pvMem) {
            return  (pvMem);
        }
    }
    
    return  (mem_malloc(stLen));
}
/*********************************************************************************************************
** 函数名称: __unixBufFree
** 功能描述: 回收内存
** 输　入  : pvMem                 内存
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __unixBufFree (PVOID  pvMem)
{
    if (((PLW_STACK)pvMem >= &_G_pstkUnixPart256[0]) && 
        ((PLW_STACK)pvMem <  &_G_pstkUnixPart256[__AF_UNIX_PART_256_SIZE])) {
        API_PartitionPut(_G_hAfUnixPart256, pvMem);
    
    } else if (((PLW_STACK)pvMem >= &_G_pstkUnixPart512[0]) && 
               ((PLW_STACK)pvMem <  &_G_pstkUnixPart512[__AF_UNIX_PART_512_SIZE])) {
        API_PartitionPut(_G_hAfUnixPart512, pvMem);
    
    } else {
        mem_free(pvMem);
    }
}
/*********************************************************************************************************
** 函数名称: __unixCreateMsg
** 功能描述: 创建一个消息节点 (面向连接类型不需要包含地址信息, 可节省空间, 提高速度)
** 输　入  : pafunixSender         发送方
**           pafunixRecver         接收方
**           pvMsg                 消息
**           stLen                 消息长度
**           pvMsgEx               扩展消息
**           uiLenEx               扩展消息长度
** 输　出  : 创建出来的信息节点
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static AF_UNIX_N  *__unixCreateMsg (AF_UNIX_T  *pafunixSender, 
                                    AF_UNIX_T  *pafunixRecver,
                                    CPVOID  pvMsg, size_t  stLen,
                                    CPVOID  pvMsgEx, socklen_t  uiLenEx)
{
    AF_UNIX_N  *pafunixmsg;
    size_t      stPathLen;
    INT         iError;
    
    if (__AF_UNIX_TYPE(pafunixSender) == SOCK_DGRAM) {                 /*  SOCK_DGRAM 需要包含地址信息  */
        stPathLen = lib_strlen(pafunixSender->UNIX_cFile);
    } else {
        stPathLen = 0;
    }
    
    pafunixmsg = (AF_UNIX_N *)__unixBufAlloc(sizeof(AF_UNIX_N) + stPathLen + stLen);
    if (pafunixmsg == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    
    pafunixmsg->UNIM_pcMsg = (PCHAR)pafunixmsg + sizeof(AF_UNIX_N) + stPathLen;
    lib_memcpy(pafunixmsg->UNIM_pcMsg, pvMsg, stLen);
    
    if (stPathLen > 0) {
        lib_strcpy(pafunixmsg->UNIM_cPath, pafunixSender->UNIX_cFile);
    } else {
        pafunixmsg->UNIM_cPath[0] = PX_EOS;
    }
    
    pafunixmsg->UNIM_stLen    = stLen;
    pafunixmsg->UNIM_stOffset = 0;
    
    if (pvMsgEx && uiLenEx) {                                           /*  是否需要扩展消息            */
        AF_UNIX_NEX  *punie = (AF_UNIX_NEX *)__unixBufAlloc(sizeof(AF_UNIX_NEX) + uiLenEx);
        if (punie == LW_NULL) {
            __unixBufFree(pafunixmsg);
            _ErrorHandle(ENOMEM);
            return  (LW_NULL);
        }
        punie->UNIE_pcMsgEx     = (PCHAR)punie + sizeof(AF_UNIX_NEX);
        punie->UNIE_uiLenEx     = uiLenEx;
        punie->UNIE_pid         = __PROC_GET_PID_CUR();                 /*  记录发送方的 pid            */
        punie->UNIE_bValid      = LW_TRUE;                              /*  扩展信息有效可被接收        */
        punie->UNIE_bNeedUnProc = LW_TRUE;                              /*  需要 unproc 处理            */
        lib_memcpy(punie->UNIE_pcMsgEx, pvMsgEx, uiLenEx);              /*  保存扩展消息                */
        
        iError = __unix_smsg_proc(pafunixRecver,
                                  punie->UNIE_pcMsgEx, 
                                  punie->UNIE_uiLenEx,
                                  punie->UNIE_pid);                     /*  扩展信息发送前预处理        */
        if (iError < ERROR_NONE) {                                      /*  如果错误, 则 proc 内部已处理*/
            __unixBufFree(punie);                                       /*  相关的 errno                */
            __unixBufFree(pafunixmsg);
            return  (LW_NULL);
        }
        pafunixmsg->UNIM_punie = punie;
    } else {
        pafunixmsg->UNIM_punie = LW_NULL;
    }
    
    return  (pafunixmsg);
}
/*********************************************************************************************************
** 函数名称: __unixDeleteMsg
** 功能描述: 删除一个消息节点
** 输　入  : pafunixmsg            消息节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __unixDeleteMsg (AF_UNIX_N  *pafunixmsg)
{
    if (pafunixmsg->UNIM_punie) {
        AF_UNIX_NEX  *punie = pafunixmsg->UNIM_punie;
        if (punie->UNIE_bNeedUnProc) {                                  /*  是否需要 UnProc 处理        */
            __unix_smsg_unproc(punie->UNIE_pcMsgEx,
                               punie->UNIE_uiLenEx,
                               punie->UNIE_pid);                        /*  回复以前的状态              */
        }
        __unixBufFree(punie);                                           /*  释放扩展消息内存            */
    }
    __unixBufFree(pafunixmsg);
}
/*********************************************************************************************************
** 函数名称: __unixDeleteMsg
** 功能描述: 删除一个 unix 节点的所有消息节点
** 输　入  : pafunix               unix 节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __unixDeleteAllMsg (AF_UNIX_T  *pafunix)
{
    AF_UNIX_N       *pafunixmsg;
    AF_UNIX_Q       *pafunixq;
    
    pafunixq = &pafunix->UNIX_unixq;
    
    while (pafunixq->UNIQ_pmonoHeader) {
        pafunixmsg = (AF_UNIX_N *)pafunixq->UNIQ_pmonoHeader;
        _list_mono_allocate_seq(&pafunixq->UNIQ_pmonoHeader,
                                &pafunixq->UNIQ_pmonoTail);
        __unixDeleteMsg(pafunixmsg);
    }
    
    pafunixq->UNIQ_stTotal = 0;
}
/*********************************************************************************************************
** 函数名称: __unixCanWrite
** 功能描述: 判断是否可以向指定的接收节点发送数据
** 输　入  : pafunixRecver         接收方
** 输　出  : 是否可写
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  __unixCanWrite (AF_UNIX_T  *pafunixRecver)
{
    size_t      stFreeBuf;
    
    if (pafunixRecver->UNIX_iStatus == __AF_UNIX_STATUS_LISTEN) {       /*  CONNECT 状态下不可写        */
        return  (LW_FALSE);
    }
    
    if (pafunixRecver->UNIX_stMaxBufSize > 
        pafunixRecver->UNIX_unixq.UNIQ_stTotal) {
        stFreeBuf = pafunixRecver->UNIX_stMaxBufSize
                  - pafunixRecver->UNIX_unixq.UNIQ_stTotal;             /*  获得对方剩余缓冲大小        */
    } else {
        stFreeBuf = 0;
    }
    if (stFreeBuf < __AF_UNIX_PIPE_BUF) {                               /*  对方缓冲区满                */
        return  (LW_FALSE);
    
    } else {
        return  (LW_TRUE);                                              /*  可以发送                    */
    }
}
/*********************************************************************************************************
** 函数名称: __unixCanRead
** 功能描述: 当前节点是否可读
** 输　入  : pafunix           unix 节点
**           flags             MSG_PEEK or MSG_WAITALL 
**           stLen             如果是 MSG_WAITALL 需要判断长度.
** 输　出  : 是否可读
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  __unixCanRead (AF_UNIX_T  *pafunix, INT  flags, size_t  stLen)
{
    AF_UNIX_Q       *pafunixq;
    
    pafunixq = &pafunix->UNIX_unixq;
    
    if (pafunixq->UNIQ_pmonoHeader == LW_NULL) {                        /*  没有信息可接收              */
        return  (LW_FALSE);
    }
    
    if (flags & MSG_WAITALL) {
        if (pafunixq->UNIQ_stTotal < stLen) {                           /*  数据不够无法接收            */
            return  (LW_FALSE);
        }
    }
    
    return  (LW_TRUE);
}
/*********************************************************************************************************
** 函数名称: __unixCanAccept
** 功能描述: 查看请求连接的队列中是否有等待的节点
** 输　入  : pafunix               控制块
** 输　出  : 等待连接的节点
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  __unixCanAccept (AF_UNIX_T  *pafunix)
{
    if (pafunix->UNIX_pringConnect == LW_NULL) {
        return  (LW_FALSE);
    
    } else {
        return  (LW_TRUE);
    }
}
/*********************************************************************************************************
** 函数名称: __unixSendtoMsg
** 功能描述: 向一个 unix 节点发送一条信息
** 输　入  : pafunixSender         发送方
**           pafunixRecver         接收方
**           pvMsg                 消息
**           stLen                 消息长度
**           pvMsgEx               扩展消息
**           uiLenEx               扩展消息长度
**           flags                 目前未使用
** 输　出  : 发送的字节数
** 全局变量: 
** 调用模块: 
** 注  意  : 此函数最多发送 __AF_UNIX_PIPE_BUF 个字节.
*********************************************************************************************************/
static ssize_t  __unixSendtoMsg (AF_UNIX_T  *pafunixSender, AF_UNIX_T  *pafunixRecver, 
                                 CPVOID  pvMsg, size_t  stLen, 
                                 CPVOID  pvMsgEx, socklen_t  uiLenEx, 
                                 INT  flags)
{
    AF_UNIX_N  *pafunixmsg;
    AF_UNIX_Q  *pafunixq;
    
    if (stLen > __AF_UNIX_PIPE_BUF) {
        stLen = __AF_UNIX_PIPE_BUF;                                     /*  一次发送最多 PIPE_BUF       */
    }
    
    pafunixmsg = __unixCreateMsg(pafunixSender, pafunixRecver, 
                                 pvMsg, stLen, pvMsgEx, uiLenEx);
    if (pafunixmsg == LW_NULL) {                                        /*  这里已经保存了对应的 errno  */
        return  (PX_ERROR);
    }
    
    pafunixq = &pafunixRecver->UNIX_unixq;
    
    _list_mono_free_seq(&pafunixq->UNIQ_pmonoHeader,
                        &pafunixq->UNIQ_pmonoTail,
                        &pafunixmsg->UNIM_monoManage);                  /*  通过 mono free 插入链表最后 */
    
    pafunixq->UNIQ_stTotal += stLen;                                    /*  更新缓冲区中的总数据        */
    
    return  ((ssize_t)stLen);
}
/*********************************************************************************************************
** 函数名称: __unixRecvfromMsg
** 功能描述: 从一个 unix 节点接收一条信息
** 输　入  : pafunixRecver         接收方
**           pcMsg                 消息
**           stLen                 消息长度
**           pvMsgEx               扩展消息
**           puiLenEx              扩展消息长度 (参数需要保存信息缓冲区的大小, 返回值为接收扩展信息的长度)
**           flags                 MSG_PEEK (MSG_WAITALL 由外层 recvfrom2 支持) 
**           from                  远程地址保存
**           fromlen               远程地址长度
** 输　出  : 接收的字节数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __unixRecvfromMsg (AF_UNIX_T  *pafunixRecver, 
                                   PVOID pvMsg, size_t  stLen, 
                                   PVOID pvMsgEx, socklen_t  *puiLenEx, INT  flags,
                                   struct sockaddr_un *from, socklen_t *fromlen)
{
    AF_UNIX_NEX     *punie;
    AF_UNIX_N       *pafunixmsg;
    AF_UNIX_Q       *pafunixq;
    size_t           stMsgLen;
    size_t           stPathLen;
    size_t           stBufPathLen;
    
    pafunixq = &pafunixRecver->UNIX_unixq;
    
    if (pafunixq->UNIQ_pmonoHeader == LW_NULL) {                        /*  没有信息可接收              */
        return  (0);
    }
    
    pafunixmsg = (AF_UNIX_N *)pafunixq->UNIQ_pmonoHeader;               /*  获得一个需要接收的消息控制块*/
    
    /*
     *  保存信息发送方的地址.
     */
    if (from && fromlen && (*fromlen > __AF_UNIX_ADDROFFSET)) {         /*  是否需要获取地址信息        */
        stBufPathLen = (*fromlen - __AF_UNIX_ADDROFFSET);
        if (__AF_UNIX_TYPE(pafunixRecver) == SOCK_DGRAM) {              /*  非面向连接需要从每个包中获取*/
            stPathLen = lib_strlen(pafunixmsg->UNIM_cPath);
            if (stPathLen > stBufPathLen) {
                stPathLen = stBufPathLen;
            }
            lib_strncpy(from->sun_path, pafunixmsg->UNIM_cPath, stBufPathLen);
        
        } else {                                                        /*  面向连接需要从对等节点获取  */
            AF_UNIX_T  *pafunixPeer = pafunixRecver->UNIX_pafunxPeer;
            if (pafunixPeer) {
                stPathLen = lib_strlen(pafunixPeer->UNIX_cFile);
                if (stPathLen > stBufPathLen) {
                    stPathLen = stBufPathLen;
                }
                lib_strncpy(from->sun_path, pafunixPeer->UNIX_cFile, stBufPathLen);
            } else {                                                    /*  连接已经中断, 接收剩余数据  */
                stPathLen = 0;
                if (stBufPathLen > 0) {
                    from->sun_path[0] = PX_EOS;
                }
            }
        }
        
        from->sun_family = AF_UNIX;
        from->sun_len    = (uint8_t)(__AF_UNIX_ADDROFFSET + stPathLen); /*  不包括 \0 的总长度          */
        *fromlen = from->sun_len;
    }
    
    /*
     *  首先接收扩展信息
     */
    if (pvMsgEx && puiLenEx && *puiLenEx) {                             /*  需要接收扩展消息            */
        punie = pafunixmsg->UNIM_punie;
        if (punie && punie->UNIE_bValid) {                              /*  存在扩展数据并且有效        */
            if (punie->UNIE_bNeedUnProc) {
                __unix_rmsg_proc(punie->UNIE_pcMsgEx,
                                 punie->UNIE_uiLenEx,
                                 punie->UNIE_pid, flags);
                punie->UNIE_bNeedUnProc = LW_FALSE;                     /*  处理过了                    */
            }
            if (*puiLenEx > punie->UNIE_uiLenEx) {
                *puiLenEx = punie->UNIE_uiLenEx;
            }
            lib_memcpy(pvMsgEx, punie->UNIE_pcMsgEx, *puiLenEx);        /*  拷贝扩展信息                */
        } else {
            *puiLenEx = 0;                                              /*  没有外带数据                */
        }
    } else if (puiLenEx) {
        *puiLenEx = 0;
    }
    
    /*
     *  接收数据信息
     */
    stMsgLen = pafunixmsg->UNIM_stLen - pafunixmsg->UNIM_stOffset;      /*  计算消息节点内消息的长度    */
    if (stLen > stMsgLen) {                                             /*  可以获取全部信息            */
        stLen = stMsgLen;
    }
                                                                        /*  拷贝数据                    */
    lib_memcpy(pvMsg, &pafunixmsg->UNIM_pcMsg[pafunixmsg->UNIM_stOffset], stLen);
    
    if (flags & MSG_PEEK) {                                             /*  数据预读, 不删除数据        */
        return  ((ssize_t)stLen);
    }
    
    if ((stLen == stMsgLen) || 
        (pafunixRecver->UNIX_iType == SOCK_DGRAM)) {                    /*  已经读取完毕或者 DGRAM 型   */
        _list_mono_allocate_seq(&pafunixq->UNIQ_pmonoHeader,
                                &pafunixq->UNIQ_pmonoTail);             /*  将消息从队列中删除          */
        __unixDeleteMsg(pafunixmsg);                                    /*  释放消息节点                */
        
        pafunixq->UNIQ_stTotal -= stMsgLen;                             /*  减少整包字节数              */
        
    } else {
        if (pafunixmsg->UNIM_punie) {
            pafunixmsg->UNIM_punie->UNIE_bValid = LW_FALSE;             /*  扩展信息只能接收一次        */
        }
        
        pafunixmsg->UNIM_stOffset += stLen;                             /*  向前推移指针                */
        pafunixq->UNIQ_stTotal    -= stLen;                             /*  减少单次读取字节数          */
    }
    
    return  ((ssize_t)stLen);
}
/*********************************************************************************************************
** 函数名称: __unixUpdateReader
** 功能描述: 向一个 unix 节点发送可读
** 输　入  : pafunix               接收节点
**           iSoErr                错误类型
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __unixUpdateReader (AF_UNIX_T  *pafunix, INT  iSoErr)
{
    __AF_UNIX_SREAD(pafunix);
    
    __unix_socket_event(pafunix, SELREAD, iSoErr);                      /*  本地 select 可读            */
}
/*********************************************************************************************************
** 函数名称: __unixUpdateWriter
** 功能描述: 向一个 unix 节点发送可写
** 输　入  : pafunix                发送节点
**           iSoErr                 错误类型
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __unixUpdateWriter (AF_UNIX_T  *pafunix, INT  iSoErr)
{
    AF_UNIX_T  *pafunixPeer;

    if (pafunix->UNIX_unixq.UNIQ_stTotal > __AF_UNIX_PIPE_BUF) {        /*  保证写入原子性              */
        __AF_UNIX_SWRITE(pafunix);
        
        pafunixPeer = pafunix->UNIX_pafunxPeer;                         /*  如果存在远程对等方          */
        if (pafunixPeer) {
            __unix_socket_event(pafunixPeer, SELWRITE, iSoErr);         /*  远程对等方 select 可写      */
        }
    }
}
/*********************************************************************************************************
** 函数名称: __unixUpdateExcept
** 功能描述: 向一个 unix 节点发送异常
** 输　入  : pafunix               节点
**           iSoErr                错误类型
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __unixUpdateExcept (AF_UNIX_T  *pafunix, INT  iSoErr)
{
    AF_UNIX_T  *pafunixPeer;

    __AF_UNIX_SREAD(pafunix);
    
    __unix_socket_event(pafunix, SELREAD, iSoErr);                      /*  本地 select 可读            */
    
    __AF_UNIX_SWRITE(pafunix);
    
    pafunixPeer = pafunix->UNIX_pafunxPeer;                             /*  如果存在远程对等方          */
    if (pafunixPeer) {
        __unix_socket_event(pafunixPeer, SELWRITE, iSoErr);             /*  远程对等方 select 可写      */
    }
    
    __unix_socket_event(pafunix, SELEXCEPT, iSoErr);                    /*  本地 select 可异常          */
}
/*********************************************************************************************************
** 函数名称: __unixUpdateConnecter
** 功能描述: 激活请求连接的 unix 节点 (select 只要激活请求点的可写就可以了)
** 输　入  : pafunixConn         请求连接的节点
**           iSoErr              错误类型
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __unixUpdateConnecter (AF_UNIX_T  *pafunixConn, INT  iSoErr)
{
    __AF_UNIX_SCONN(pafunixConn);
    
    __unix_socket_event(pafunixConn, SELWRITE, iSoErr);                 /*  请求连接的节点 select 可写  */
}
/*********************************************************************************************************
** 函数名称: __unixUpdateAccept
** 功能描述: 激活 accept 节点, 表示有节点请求连接
** 输　入  : pafunixAcce         accept 节点
**           iSoErr              错误类型
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __unixUpdateAccept (AF_UNIX_T  *pafunixAcce, INT  iSoErr)
{
    __AF_UNIX_SACCE(pafunixAcce);
    
    __unix_socket_event(pafunixAcce, SELREAD, iSoErr);                  /*  accept 节点 select 可读     */
}
/*********************************************************************************************************
** 函数名称: __unixShutdownR
** 功能描述: 关闭本地读的功能
** 输　入  : pafunix               节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __unixShutdownR (AF_UNIX_T  *pafunix)
{
    AF_UNIX_T  *pafunixPeer = pafunix->UNIX_pafunxPeer;
    
    if ((pafunix->UNIX_iShutDFlag & __AF_UNIX_SHUTD_R) == 0) {
        pafunix->UNIX_iShutDFlag |= __AF_UNIX_SHUTD_R;                  /*  我不能再读                  */
        if (pafunixPeer) {
            pafunixPeer->UNIX_iShutDFlag |= __AF_UNIX_SHUTD_W;          /*  远程不能再写                */
            __unixUpdateWriter(pafunixPeer, ESHUTDOWN);                 /*  激活远程节点等待写          */
        }
        __unixDeleteAllMsg(pafunix);                                    /*  删除没有接收的信息          */
        __unixUpdateReader(pafunix, ENOTCONN);                          /*  激活我的读等待              */
    }
}
/*********************************************************************************************************
** 函数名称: __unixShutdownW
** 功能描述: 关闭本地写的功能
** 输　入  : pafunix               节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 这里并不清除读缓冲, 给对方读剩余数据的机会.
*********************************************************************************************************/
static VOID  __unixShutdownW (AF_UNIX_T  *pafunix)
{
    AF_UNIX_T  *pafunixPeer = pafunix->UNIX_pafunxPeer;
    
    if ((pafunix->UNIX_iShutDFlag & __AF_UNIX_SHUTD_W) == 0) {
        pafunix->UNIX_iShutDFlag |= __AF_UNIX_SHUTD_W;                  /*  我不能再写                  */
        if (pafunixPeer) {
            pafunixPeer->UNIX_iShutDFlag |= __AF_UNIX_SHUTD_R;          /*  远程不能再读                */
            __unixUpdateReader(pafunixPeer, ENOTCONN);                  /*  激活远程节点等待读          */
        }
        __unixUpdateWriter(pafunix, ESHUTDOWN);                         /*  激活我的等待写              */
    }
}
/*********************************************************************************************************
** 函数名称: __unixCreate
** 功能描述: 创建一个 af_unix 控制块
** 输　入  : iType                 SOCK_STREAM / SOCK_DGRAM / SOCK_SEQPACKET
** 输　出  : 控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static AF_UNIX_T  *__unixCreate (INT  iType)
{
    AF_UNIX_T   *pafunix;

    pafunix = (AF_UNIX_T *)__SHEAP_ALLOC(sizeof(AF_UNIX_T));
    if (pafunix == LW_NULL) {
        return  (LW_NULL);
    }
    lib_bzero(pafunix, sizeof(AF_UNIX_T));
    
    pafunix->UNIX_iFlag        = O_RDWR;
    pafunix->UNIX_iType        = iType;
    pafunix->UNIX_iStatus      = __AF_UNIX_STATUS_NONE;
    pafunix->UNIX_iShutDFlag   = 0;
    pafunix->UNIX_iBacklog     = 1;
    pafunix->UNIX_pafunxPeer   = LW_NULL;
    pafunix->UNIX_stMaxBufSize = __AF_UNIX_DEF_BUFSIZE;
    
    pafunix->UNIX_ulSendTimeout = LW_OPTION_WAIT_INFINITE;
    pafunix->UNIX_ulRecvTimeout = LW_OPTION_WAIT_INFINITE;
    pafunix->UNIX_ulConnTimeout = LW_OPTION_WAIT_INFINITE;
    
    pafunix->UNIX_hCanRead = API_SemaphoreBCreate("unix_rlock", LW_FALSE, 
                                                  LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pafunix->UNIX_hCanRead == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pafunix);
        return  (LW_NULL);
    }
    
    pafunix->UNIX_hCanWrite = API_SemaphoreBCreate("unix_wlock", LW_FALSE, 
                                                   LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pafunix->UNIX_hCanWrite == LW_OBJECT_HANDLE_INVALID) {
        API_SemaphoreBDelete(&pafunix->UNIX_hCanRead);
        __SHEAP_FREE(pafunix);
        return  (LW_NULL);
    }
    
    __AF_UNIX_LOCK();
    _List_Line_Add_Ahead(&pafunix->UNIX_lineManage, &_G_plineAfUnix);
    __AF_UNIX_UNLOCK();
    
    return  (pafunix);
}
/*********************************************************************************************************
** 函数名称: __unixDelete
** 功能描述: 删除一个 af_unix 控制块 (遍历所有控制块, 删除 UNIX_pafunxPeer)
** 输　入  : pafunix               控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __unixDelete (AF_UNIX_T  *pafunix)
{
    AF_UNIX_T       *pafunixTemp;
    PLW_LIST_LINE    plineTemp;
    
    __AF_UNIX_LOCK();
    __unixDeleteAllMsg(pafunix);                                        /*  删除所有未接收的信息        */
    for (plineTemp  = _G_plineAfUnix;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
    
        pafunixTemp = (AF_UNIX_T *)plineTemp;
        if (pafunixTemp->UNIX_pafunxPeer == pafunix) {
            pafunixTemp->UNIX_pafunxPeer = LW_NULL;
        }
    }
    _List_Line_Del(&pafunix->UNIX_lineManage, &_G_plineAfUnix);
    __AF_UNIX_UNLOCK();
    
    API_SemaphoreBDelete(&pafunix->UNIX_hCanRead);
    API_SemaphoreBDelete(&pafunix->UNIX_hCanWrite);
    
    __SHEAP_FREE(pafunix);
}
/*********************************************************************************************************
** 函数名称: __unixFind
** 功能描述: 查询一个节点
** 输　入  : pcPath                查询一个节点
**           iType                 类型
**           bListen               是否查询一个 listen 节点
** 输　出  : pafunix
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static AF_UNIX_T  *__unixFind (CPCHAR  pcPath, INT  iType, BOOL  bListen)
{
    AF_UNIX_T       *pafunixTemp;
    PLW_LIST_LINE    plineTemp;
    
    for (plineTemp  = _G_plineAfUnix;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
    
        pafunixTemp = (AF_UNIX_T *)plineTemp;
        if ((__AF_UNIX_TYPE(pafunixTemp) == iType) &&
            (lib_strcmp(pafunixTemp->UNIX_cFile, pcPath) == 0)) {
            if ((iType == SOCK_DGRAM) || !bListen ||
                (pafunixTemp->UNIX_iStatus == __AF_UNIX_STATUS_LISTEN)) {
                return  (pafunixTemp);
            }
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __unixConnect
** 功能描述: pafunix 请求连接到 pafunixAcce
** 输　入  : pafunix               控制块
**           pafunixAcce           控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __unixConnect (AF_UNIX_T  *pafunix, AF_UNIX_T  *pafunixAcce)
{
    pafunix->UNIX_iStatus = __AF_UNIX_STATUS_CONNECT;
    pafunix->UNIX_pafunxPeer = pafunixAcce;

    _List_Ring_Add_Last(&pafunix->UNIX_ringConnect,
                        &pafunixAcce->UNIX_pringConnect);
                        
    pafunixAcce->UNIX_iConnNum++;
}
/*********************************************************************************************************
** 函数名称: __unixUnconnect
** 功能描述: 从请求连接队列中退出来
** 输　入  : pafunixConn             请求连接的控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __unixUnconnect (AF_UNIX_T  *pafunixConn)
{
    AF_UNIX_T  *pafunixAcce = pafunixConn->UNIX_pafunxPeer;
    
    pafunixConn->UNIX_iStatus = __AF_UNIX_STATUS_NONE;
    
    if (pafunixAcce) {
        _List_Ring_Del(&pafunixConn->UNIX_ringConnect,
                       &pafunixAcce->UNIX_pringConnect);
                       
        pafunixAcce->UNIX_iConnNum--;
        
        pafunixConn->UNIX_pafunxPeer = LW_NULL;
    }
}
/*********************************************************************************************************
** 函数名称: __unixAccept
** 功能描述: 从请求连接的队列中取出一个
** 输　入  : pafunix               控制块
** 输　出  : 等待连接的节点
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static AF_UNIX_T  *__unixAccept (AF_UNIX_T  *pafunix)
{
    AF_UNIX_T      *pafunixConn;
    PLW_LIST_RING   pringConn;
    
    if (pafunix->UNIX_pringConnect == LW_NULL) {
        return  (LW_NULL);
    }
    
    pringConn = pafunix->UNIX_pringConnect;                             /*  从等待表头获取              */
    
    pafunixConn = _LIST_ENTRY(pringConn, AF_UNIX_T, UNIX_ringConnect);
    
    _List_Ring_Del(&pafunixConn->UNIX_ringConnect,
                   &pafunix->UNIX_pringConnect);                        /*  从等待连接表中删除          */
    
    pafunix->UNIX_iConnNum--;
    
    pafunixConn->UNIX_pafunxPeer = LW_NULL;
    
    return  (pafunixConn);
}
/*********************************************************************************************************
** 函数名称: __unixRefuseAll
** 功能描述: 请求连接的队列删除所有等待连接节点 (unix_close 时调用)
** 输　入  : pafunix               控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __unixRefuseAll (AF_UNIX_T  *pafunix)
{
    AF_UNIX_T      *pafunixConn;
    PLW_LIST_RING   pringConn;
    
    while (pafunix->UNIX_pringConnect) {
        pringConn = pafunix->UNIX_pringConnect;                         /*  从等待表头获取              */
        
        pafunixConn = _LIST_ENTRY(pringConn, AF_UNIX_T, UNIX_ringConnect);
        pafunixConn->UNIX_iStatus = __AF_UNIX_STATUS_NONE;
        pafunixConn->UNIX_pafunxPeer = LW_NULL;
        
        _List_Ring_Del(&pafunixConn->UNIX_ringConnect,
                       &pafunix->UNIX_pringConnect);                    /*  从等待连接表中删除          */
        
        __unixUpdateConnecter(pafunixConn, ECONNREFUSED);               /*  通知请求连接的线程          */
    }
    pafunix->UNIX_iConnNum = 0;
}
/*********************************************************************************************************
** 函数名称: __unixMsToTicks
** 功能描述: 毫秒转换为 ticks
** 输　入  : ulMs        毫秒
** 输　出  : tick
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ULONG  __unixMsToTicks (ULONG  ulMs)
{
    ULONG   ulTicks;
    
    if (ulMs == 0) {
        ulTicks = LW_OPTION_WAIT_INFINITE;
    } else {
        ulTicks = LW_MSECOND_TO_TICK_1(ulMs);
    }
    
    return  (ulTicks);
}
/*********************************************************************************************************
** 函数名称: __unixTvToTicks
** 功能描述: timeval 转换为 ticks
** 输　入  : ptv       时间
** 输　出  : tick
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ULONG  __unixTvToTicks (struct timeval  *ptv)
{
    ULONG   ulTicks;

    if ((ptv->tv_sec == 0) && (ptv->tv_usec == 0)) {
        return  (LW_OPTION_WAIT_INFINITE);
    }
    
    ulTicks = __timevalToTick(ptv);
    if (ulTicks == 0) {
        ulTicks = 1;
    }
    
    return  (ulTicks);
}
/*********************************************************************************************************
** 函数名称: __unixTicksToMs
** 功能描述: ticks 转换为毫秒
** 输　入  : ulTicks       tick
** 输　出  : 毫秒
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ULONG  __unixTicksToMs (ULONG  ulTicks)
{
    ULONG  ulMs;
    
    if (ulTicks == LW_OPTION_WAIT_INFINITE) {
        ulMs = 0;
    } else {
        ulMs = (ulTicks * 1000) / LW_TICK_HZ;
    }
    
    return  (ulMs);
}
/*********************************************************************************************************
** 函数名称: __unixTicksToTv
** 功能描述: ticks 转换为 timeval
** 输　入  : ulTicks       tick
** 输　出  : timeval
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID __unixTicksToTv (ULONG  ulTicks, struct timeval *ptv)
{
    if (ulTicks == LW_OPTION_WAIT_INFINITE) {
        ptv->tv_sec  = 0;
        ptv->tv_usec = 0;
    } else {
        __tickToTimeval(ulTicks, ptv);
    }
}
/*********************************************************************************************************
** 函数名称: __unixSignalNotify
** 功能描述: 需要断裂的管道需要发送 SIGPIPE 信号
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID __unixSignalNotify (INT  iFlag)
{
#if LW_CFG_SIGNAL_EN > 0
    sigevent_t      sigeventPipe;
    
    if ((iFlag & MSG_NOSIGNAL) == 0) {                                  /*  没有 MSG_NOSIGNAL 标志      */
        sigeventPipe.sigev_signo           = SIGPIPE;
        sigeventPipe.sigev_value.sival_ptr = LW_NULL;
        sigeventPipe.sigev_notify          = SIGEV_SIGNAL;
    
        _doSigEvent(API_ThreadIdSelf(), &sigeventPipe, SI_MESGQ);       /*  产生 SIGPIPE 信号           */
    }
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
}
/*********************************************************************************************************
** 函数名称: unix_init
** 功能描述: 初始化 unix 域协议
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  unix_init (VOID)
{
    _G_hAfUnixMutex = API_SemaphoreMCreate("afunix_lock", LW_PRIO_DEF_CEILING, 
                                           LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                           LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                           LW_NULL);
                                           
    _G_pstkUnixPart256 = (LW_STACK *)__SHEAP_ALLOC(__AF_UNIX_PART_256 * 256);
    _BugHandle(!_G_pstkUnixPart256, LW_TRUE, "AF_UNIX buffer create error!\r\n");
    
    _G_pstkUnixPart512 = (LW_STACK *)__SHEAP_ALLOC(__AF_UNIX_PART_512 * 512);
    _BugHandle(!_G_pstkUnixPart512, LW_TRUE, "AF_UNIX buffer create error!\r\n");
    
    _G_hAfUnixPart256 = API_PartitionCreate("unix_256", _G_pstkUnixPart256, __AF_UNIX_PART_256,
                                            256, LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_hAfUnixPart512 = API_PartitionCreate("unix_512", _G_pstkUnixPart512, __AF_UNIX_PART_512,
                                            512, LW_OPTION_OBJECT_GLOBAL, LW_NULL);
}
/*********************************************************************************************************
** 函数名称: unix_traversal
** 功能描述: 遍历所有 af_unix 控制块
** 输　入  : pfunc                遍历函数
**           pvArg[0 ~ 5]         遍历函数参数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  unix_traversal (VOIDFUNCPTR    pfunc, 
                      PVOID          pvArg0,
                      PVOID          pvArg1,
                      PVOID          pvArg2,
                      PVOID          pvArg3,
                      PVOID          pvArg4,
                      PVOID          pvArg5)
{
    AF_UNIX_T       *pafunixTemp;
    PLW_LIST_LINE    plineTemp;
    
    __AF_UNIX_LOCK();
    for (plineTemp  = _G_plineAfUnix;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pafunixTemp = (AF_UNIX_T *)plineTemp;
        pfunc(pafunixTemp, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
    }
    __AF_UNIX_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: unix_socket
** 功能描述: unix socket
** 输　入  : iDomain        域, 必须是 AF_UNIX
**           iType          SOCK_STREAM / SOCK_DGRAM / SOCK_SEQPACKET
**           iProtocol      协议
** 输　出  : unix socket
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
AF_UNIX_T  *unix_socket (INT  iDomain, INT  iType, INT  iProtocol)
{
    AF_UNIX_T   *pafunix;
    
    if (iDomain != AF_UNIX) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    if ((iType != SOCK_STREAM) && 
        (iType != SOCK_DGRAM)  &&
        (iType != SOCK_SEQPACKET)) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    pafunix = __unixCreate(iType);
    if (pafunix == LW_NULL) {
        _ErrorHandle(ENOMEM);
    }
    
    return  (pafunix);
}
/*********************************************************************************************************
** 函数名称: unix_bind
** 功能描述: bind
** 输　入  : pafunix   unix file
**           name      address
**           namelen   address len
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  unix_bind (AF_UNIX_T  *pafunix, const struct sockaddr *name, socklen_t namelen)
{
    struct sockaddr_un  *paddrun = (struct sockaddr_un *)name;
           INT           iFd;
           INT           iPathLen;
           INT           iSockType;
           AF_UNIX_T    *pafunixFind;
           CHAR          cPath[MAX_FILENAME_LENGTH];
    
    if (paddrun == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iPathLen = (namelen - __AF_UNIX_ADDROFFSET);
    if (iPathLen < 1) {                                                 /*  sockaddr 中没有路径         */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (iPathLen > PATH_MAX) {                                          /*  路径超长                    */
        _ErrorHandle(ENAMETOOLONG);
        return  (PX_ERROR);
    }
    
    lib_strncpy(cPath, paddrun->sun_path, iPathLen);
    cPath[iPathLen] = PX_EOS;
    
    iFd = open(cPath, O_CREAT | O_RDWR, __AF_UNIX_DEF_FLAG | S_IFSOCK); /*  创建 socket 文件            */
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    __AF_UNIX_LOCK();
    API_IosFdGetName(iFd, cPath, MAX_FILENAME_LENGTH);                  /*  获得完整路径                */
    iSockType = __AF_UNIX_TYPE(pafunix);
    pafunixFind = __unixFind(cPath, iSockType, 
                             (iSockType == SOCK_DGRAM) ?
                             LW_FALSE : LW_TRUE);
    if (pafunixFind) {
        __AF_UNIX_UNLOCK();
        close(iFd);
        _ErrorHandle(EADDRINUSE);                                       /*  不允许重复地址绑定          */
        return  (PX_ERROR);
    }
    lib_strcpy(pafunix->UNIX_cFile, cPath);
    __AF_UNIX_UNLOCK();
    
    close(iFd);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: unix_listen
** 功能描述: listen
** 输　入  : pafunix   unix file
**           backlog   back log num
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  unix_listen (AF_UNIX_T  *pafunix, INT  backlog)
{
    if (backlog < 0) {
        backlog = 1;
    }
    
    if (__AF_UNIX_TYPE(pafunix) == SOCK_DGRAM) {                        /*  不能为非面向连接 socket     */
        _ErrorHandle(EOPNOTSUPP);
        return  (PX_ERROR);
    }
    
    if (pafunix->UNIX_iStatus != __AF_UNIX_STATUS_NONE) {               /*  必须从 NONE 模式开始 listen */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __AF_UNIX_LOCK();
    if (pafunix->UNIX_cFile[0] == PX_EOS) {
        __AF_UNIX_UNLOCK();
        _ErrorHandle(EDESTADDRREQ);                                     /*  没有绑定地址                */
        return  (PX_ERROR);
    }
    
    pafunix->UNIX_iBacklog = backlog;
    pafunix->UNIX_iStatus  = __AF_UNIX_STATUS_LISTEN;
    __AF_UNIX_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: unix_accept
** 功能描述: accept
** 输　入  : pafunix   unix file
**           addr      address
**           addrlen   address len
** 输　出  : unix socket
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
AF_UNIX_T  *unix_accept (AF_UNIX_T  *pafunix, struct sockaddr *addr, socklen_t *addrlen)
{
    struct sockaddr_un  *paddrun = (struct sockaddr_un *)addr;
    AF_UNIX_T           *pafunixConn;
    AF_UNIX_T           *pafunixNew;

    if (__AF_UNIX_TYPE(pafunix) == SOCK_DGRAM) {                        /*  不能为非面向连接 socket     */
        _ErrorHandle(EOPNOTSUPP);
        return  (LW_NULL);
    }
    
    if (pafunix->UNIX_iStatus != __AF_UNIX_STATUS_LISTEN) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    __AF_UNIX_LOCK();
    do {
        if (__unixCanAccept(pafunix)) {                                 /*  是否可以 accept             */
            __AF_UNIX_UNLOCK();
            pafunixNew = __unixCreate(__AF_UNIX_TYPE(pafunix));         /*  继承原来的类型              */
            if (pafunixNew == LW_NULL) {
                __AF_UNIX_LOCK();
                __unixRefuseAll(pafunix);                               /*  没有足够内存, 拒绝连接      */
                __AF_UNIX_UNLOCK();
                _ErrorHandle(ENOMEM);
                return  (LW_NULL);
            }
            __AF_UNIX_LOCK();
            
            pafunixConn = __unixAccept(pafunix);                        /*  获得一个正在等待的连接      */
            if (pafunixConn) {
                pafunixConn->UNIX_iStatus    = __AF_UNIX_STATUS_ESTAB;
                pafunixConn->UNIX_pafunxPeer = pafunixNew;
                pafunixNew->UNIX_iStatus     = __AF_UNIX_STATUS_ESTAB;
                pafunixNew->UNIX_pafunxPeer  = pafunixConn;
                pafunixNew->UNIX_iPassCred   = pafunix->UNIX_iPassCred; /*  继承 SO_PASSCRED 选项       */
                lib_strcpy(pafunixNew->UNIX_cFile, pafunix->UNIX_cFile);/*  继承 accept 地址            */
                
                if (paddrun && addrlen && (*addrlen > __AF_UNIX_ADDROFFSET)) {
                    size_t  stPathLen    = lib_strlen(pafunixConn->UNIX_cFile);
                    size_t  stBufPathLen = (*addrlen - __AF_UNIX_ADDROFFSET);
                    if (stPathLen > stBufPathLen) {
                        stPathLen = stBufPathLen;
                    }
                    lib_strncpy(paddrun->sun_path, pafunixConn->UNIX_cFile, 
                                stBufPathLen);                          /*  记录请求连接节点的信息      */
                    paddrun->sun_family = AF_UNIX;
                    paddrun->sun_len = (uint8_t)(__AF_UNIX_ADDROFFSET + stPathLen);
                    *addrlen = paddrun->sun_len;                        /*  长度不包含 \0               */
                }
                
                __unixUpdateConnecter(pafunixConn, ERROR_NONE);         /*  激活请求连接的线程          */
                
                __AF_UNIX_CCONN(pafunixConn);                           /*  清除信号量, 开始当做其他用途*/
                break;
            
            } else {                                                    /*  没有取得远程连接            */
                __AF_UNIX_UNLOCK();
                __unixDelete(pafunixNew);
                __AF_UNIX_LOCK();
            }
        }
        
        if (__AF_UNIX_IS_NBIO(pafunix, 0)) {                            /*  非阻塞 IO                   */
            __AF_UNIX_UNLOCK();
            _ErrorHandle(EWOULDBLOCK);                                  /*  需要重新读                  */
            return  (LW_NULL);
        }
        
        __AF_UNIX_UNLOCK();
        
        __AF_UNIX_WACCE(pafunix);                                       /*  等待请求连接                */
        
        __AF_UNIX_LOCK();
    } while (1);
    __AF_UNIX_UNLOCK();

    return  (pafunixNew);
}
/*********************************************************************************************************
** 函数名称: unix_connect
** 功能描述: connect
** 输　入  : pafunix   unix file
**           name      address
**           namelen   address len
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  unix_connect (AF_UNIX_T  *pafunix, const struct sockaddr *name, socklen_t namelen)
{
    struct sockaddr_un  *paddrun = (struct sockaddr_un *)name;
    AF_UNIX_T           *pafunixAcce;
    CHAR                 cPath[MAX_FILENAME_LENGTH];

    if (paddrun == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (namelen <= __AF_UNIX_ADDROFFSET) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    _PathGetFull(cPath, MAX_FILENAME_LENGTH, paddrun->sun_path);        /*  获得完整路径                */
    
    __AF_UNIX_LOCK();
    if (pafunix->UNIX_iStatus == __AF_UNIX_STATUS_CONNECT) {
        if (__AF_UNIX_IS_NBIO(pafunix, 0)) {
            __AF_UNIX_UNLOCK();
            _ErrorHandle(EINPROGRESS);                                  /*  正在执行 connect 操作       */
        } else {
            __AF_UNIX_UNLOCK();
            _ErrorHandle(EALREADY);
        }
        return  (PX_ERROR);
    }
    
    if (pafunix->UNIX_iStatus != __AF_UNIX_STATUS_NONE) {
        __AF_UNIX_UNLOCK();
        _ErrorHandle(EISCONN);                                          /*  已经有链接                  */
        return  (PX_ERROR);
    }
    
    pafunixAcce = __unixFind(cPath, __AF_UNIX_TYPE(pafunix), LW_TRUE);  /*  查询连接目的                */
    if (pafunixAcce == LW_NULL) {
        __AF_UNIX_UNLOCK();
        _ErrorHandle(ECONNRESET);                                       /*  没有目的 ECONNRESET         */
        return  (PX_ERROR);
    }
    
    if (__AF_UNIX_TYPE(pafunix) == SOCK_DGRAM) {                        /*  非面向连接类型              */
        pafunix->UNIX_pafunxPeer = pafunixAcce;                         /*  保存远程节点                */
        
    } else {                                                            /*  面向连接类型                */
        if ((pafunixAcce->UNIX_iStatus != __AF_UNIX_STATUS_LISTEN) ||   /*  对方没有在 listen 模式      */
            (pafunixAcce->UNIX_iConnNum > pafunixAcce->UNIX_iBacklog)) {/*  对方已经到最大连接数        */
            __AF_UNIX_UNLOCK();
            _ErrorHandle(ECONNREFUSED);                                 /*  无法连接目的 ECONNREFUSED   */
            return  (PX_ERROR);
        }
        
        __AF_UNIX_CCONN(pafunix);
         
        __unixConnect(pafunix, pafunixAcce);                            /*  连接远程节点                */
        
        __unixUpdateAccept(pafunixAcce, ERROR_NONE);                    /*  激活 accept                 */
        
        if (__AF_UNIX_IS_NBIO(pafunix, 0)) {                            /*  非阻塞 IO                   */
            __AF_UNIX_UNLOCK();
            _ErrorHandle(EINPROGRESS);                                  /*  需要用户等待连接结果        */
            return  (PX_ERROR);
        }
        __AF_UNIX_UNLOCK();
        
        __AF_UNIX_WCONN(pafunix);                                       /*  等待 accept 确认此消息      */
        
        __AF_UNIX_LOCK();
        if (pafunix->UNIX_iStatus == __AF_UNIX_STATUS_CONNECT) {
            __unixUnconnect(pafunix);                                   /*  解除连接                    */
            __AF_UNIX_UNLOCK();
            _ErrorHandle(ETIMEDOUT);                                    /*  connect 超时                */
            return  (PX_ERROR);
        
        } else if (pafunix->UNIX_iStatus == __AF_UNIX_STATUS_NONE) {
            __AF_UNIX_UNLOCK();
            _ErrorHandle(ECONNREFUSED);                                 /*  connect 被拒绝              */
            return  (PX_ERROR);
        }
    }
    __AF_UNIX_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: unix_connect2
** 功能描述: connect two unix socket
** 输　入  : pafunix0   unix file
**           pafunix1   unix file
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  unix_connect2 (AF_UNIX_T  *pafunix0, AF_UNIX_T  *pafunix1)
{
    __AF_UNIX_LOCK();
    pafunix0->UNIX_pafunxPeer = pafunix1;
    pafunix1->UNIX_pafunxPeer = pafunix0;
    
    if ((__AF_UNIX_TYPE(pafunix0) == SOCK_STREAM) ||
        (__AF_UNIX_TYPE(pafunix0) == SOCK_SEQPACKET)) {
        pafunix0->UNIX_iStatus = __AF_UNIX_STATUS_ESTAB;
        pafunix1->UNIX_iStatus = __AF_UNIX_STATUS_ESTAB;
    }
    __AF_UNIX_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: unix_recvfrom2
** 功能描述: recvfrom (带有扩展信息接收功能)
** 输　入  : pafunix   unix file
**           mem       buffer
**           len       buffer len
**           mem_ex    control msg
**           plen_ex   control msg len
**           pid       sender pid
**           flags     flag
**           from      packet from
**           fromlen   name len
** 输　出  : NUM
** 全局变量: 
** 调用模块: 
** 注  意  : 多个线程同时读同一个 socket 而且使用不同的 flag 会出现唤醒逻辑性错误, 目前并没有这样设计的
             应用.
*********************************************************************************************************/
static ssize_t  unix_recvfrom2 (AF_UNIX_T  *pafunix, 
                                void *mem, size_t len, 
                                void *mem_ex, socklen_t *plen_ex, int flags,
                                struct sockaddr *from, socklen_t *fromlen)
{
    ssize_t     sstTotal = 0;
    ULONG       ulError;
    BOOL        bNeedUpdateWriter;
    
    if (!mem || !len) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if ((flags & MSG_PEEK) && (flags & MSG_WAITALL)) {                  /*  目前这是不允许的            */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __AF_UNIX_LOCK();
    do {
        if ((__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) ||
            (__AF_UNIX_TYPE(pafunix) == SOCK_SEQPACKET)) {              /*  面向连接类型                */
            if (pafunix->UNIX_iStatus != __AF_UNIX_STATUS_ESTAB) {
                __AF_UNIX_UNLOCK();
                _ErrorHandle(ENOTCONN);                                 /*  没有连接                    */
                return  (PX_ERROR);
            }
        }
        
        if (__unixCanRead(pafunix, flags, len)) {                       /*  可以接收                    */
            PCHAR   pcRecvMem = (PCHAR)mem;                             /*  当前接收指针                */
            ssize_t sstReadNum;                                         /*  单次接收数据长度            */
            
            if (__unixCanWrite(pafunix)) {
                bNeedUpdateWriter = LW_FALSE;                           /*  本来就可以写不需要激活      */
            } else {
                bNeedUpdateWriter = LW_TRUE;                            /*  需要激活写任务              */
            }
            
__recv_more:
            sstReadNum = __unixRecvfromMsg(pafunix,
                                           (PVOID)pcRecvMem, 
                                           (size_t)(len - sstTotal), 
                                           mem_ex, plen_ex, flags, 
                                           (struct sockaddr_un *)from, 
                                           fromlen);                    /*  从缓冲区取一个消息          */
            pcRecvMem += sstReadNum;
            sstTotal  += sstReadNum;
            
            if (__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) {               /*  字符流类型                  */
                if (sstTotal < len) {                                   /*  还有空间                    */
                    if ((flags & MSG_WAITALL) ||                        /*  需要装满接收缓冲区          */
                        (pafunix->UNIX_unixq.UNIQ_pmonoHeader)) {       /*  还有数据可以接收            */
                        goto    __recv_more;
                    }
                }
            }
            break;                                                      /*  跳出接收循环                */
        
        } else {
#if LW_CFG_NET_UNIX_MULTI_EN > 0
            if (__unixCanRead(pafunix, 0, 0)) {
                __AF_UNIX_CREAD(pafunix);                               /*  可接收, 但清除掉等待信号量  */
            }
#endif
            
            if ((__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) ||
                (__AF_UNIX_TYPE(pafunix) == SOCK_SEQPACKET)) {          /*  面向连接类型                */
                if (pafunix->UNIX_iShutDFlag & __AF_UNIX_SHUTD_R) {     /*  已被关闭读                  */
                    __AF_UNIX_UNLOCK();
                    _ErrorHandle(ENOTCONN);                             /*  本地已经关闭                */
                    return  (sstTotal);
                }
                if (pafunix->UNIX_pafunxPeer == LW_NULL) {
                    __AF_UNIX_UNLOCK();
                    _ErrorHandle(ECONNRESET);                           /*  没有连接                    */
                    return  (PX_ERROR);
                }
            }
        }
        
        if (__AF_UNIX_IS_NBIO(pafunix, flags)) {                        /*  非阻塞 IO                   */
            __AF_UNIX_UNLOCK();
            _ErrorHandle(EWOULDBLOCK);                                  /*  需要重新读                  */
            return  (sstTotal);
        }
        
        __AF_UNIX_UNLOCK();
        ulError = __AF_UNIX_WREAD(pafunix);                             /*  等待数据                    */
        if (ulError) {
            _ErrorHandle(EWOULDBLOCK);                                  /*  等待超时                    */
            return  (sstTotal);
        }
        __AF_UNIX_LOCK();
    } while (1);
    
    if (bNeedUpdateWriter && (sstTotal > 0)) {
        __unixUpdateWriter(pafunix, ERROR_NONE);                        /*  update writer               */
    }

#if LW_CFG_NET_UNIX_MULTI_EN > 0
    if (__unixCanRead(pafunix, 0, 0)) {
        __AF_UNIX_SREAD(pafunix);                                       /*  other reader can read       */
    }
#endif
    __AF_UNIX_UNLOCK();
    
    return  (sstTotal);
}
/*********************************************************************************************************
** 函数名称: unix_recvmsg
** 功能描述: recvmsg
** 输　入  : pafunix   unix file
**           msg       消息
**           flags     flag
** 输　出  : NUM 数据长度
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  unix_recvmsg (AF_UNIX_T  *pafunix, struct msghdr *msg, int flags)
{
    ssize_t     sstRecvLen;

    if (!msg) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (msg->msg_iovlen == 1) {
        sstRecvLen = unix_recvfrom2(pafunix, msg->msg_iov->iov_base, msg->msg_iov->iov_len, 
                                    msg->msg_control, &msg->msg_controllen, flags, 
                                    (struct sockaddr *)msg->msg_name, &msg->msg_namelen);
    
    } else {
        struct iovec    liovec, *msg_iov;
        int             msg_iovlen;
        ssize_t         sstRecvCnt;
        unsigned int    i, totalsize;
        char           *lbuf;
        char           *temp;
        
        msg_iov    = msg->msg_iov;
        msg_iovlen = msg->msg_iovlen;
        
        for (i = 0, totalsize = 0; i < msg_iovlen; i++) {
            if ((msg_iov[i].iov_len == 0) || (msg_iov[i].iov_base == LW_NULL)) {
                _ErrorHandle(EINVAL);
                return  (PX_ERROR);
            }
            totalsize += (unsigned int)msg_iov[i].iov_len;
        }
        
        lbuf = (char *)__unixBufAlloc(totalsize);
        if (lbuf == LW_NULL) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        
        liovec.iov_base = (PVOID)lbuf;
        liovec.iov_len  = (size_t)totalsize;
        
        sstRecvLen = unix_recvfrom2(pafunix, liovec.iov_base, liovec.iov_len, 
                                    msg->msg_control, &msg->msg_controllen, flags, 
                                    (struct sockaddr *)msg->msg_name, &msg->msg_namelen);
        
        sstRecvCnt = sstRecvLen;
        temp       = lbuf;
        for (i = 0; sstRecvCnt > 0 && i < msg_iovlen; i++) {
            size_t   qty = (size_t)((sstRecvCnt > msg_iov[i].iov_len) ? msg_iov[i].iov_len : sstRecvCnt);
            lib_memcpy(msg_iov[i].iov_base, temp, qty);
            temp += qty;
            sstRecvCnt -= qty;
        }
        
        __unixBufFree(lbuf);
    }
    
    return  (sstRecvLen);
}
/*********************************************************************************************************
** 函数名称: unix_recvfrom
** 功能描述: recvfrom
** 输　入  : pafunix   unix file
**           mem       buffer
**           len       buffer len
**           flags     flag
**           from      packet from
**           fromlen   name len
** 输　出  : NUM
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  unix_recvfrom (AF_UNIX_T  *pafunix, void *mem, size_t len, int flags,
                        struct sockaddr *from, socklen_t *fromlen)
{
    return  (unix_recvfrom2(pafunix, mem, len, LW_NULL, LW_NULL, flags, from, fromlen));
}
/*********************************************************************************************************
** 函数名称: unix_recv
** 功能描述: recv
** 输　入  : pafunix   unix file
**           mem       buffer
**           len       buffer len
**           flags     flag
** 输　出  : NUM
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  unix_recv (AF_UNIX_T  *pafunix, void *mem, size_t len, int flags)
{
    return  (unix_recvfrom2(pafunix, mem, len, LW_NULL, LW_NULL, flags, LW_NULL, LW_NULL));
}
/*********************************************************************************************************
** 函数名称: unix_sendto2
** 功能描述: sendto (带有扩展信息接收功能)
** 输　入  : pafunix   unix file
**           data      send buffer
**           size      send len
**           data_ex   control msg
**           size_ex   control msg len
**           flags     flag
**           to        packet to
**           tolen     name len
** 输　出  : NUM
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  unix_sendto2 (AF_UNIX_T  *pafunix, const void *data, size_t size, 
                              const void *data_ex, socklen_t size_ex, int flags,
                              const struct sockaddr *to, socklen_t tolen)
{
    BOOL        bHaveTo  = LW_FALSE;
    ssize_t     sstTotal = 0;
    ssize_t     sstWriteNum;                                            /*  单次发送长度                */
    ULONG       ulError;
    
    CHAR        cPath[MAX_FILENAME_LENGTH];
    AF_UNIX_T  *pafunixRecver;
    
    INT         i       = 0;                                            /*  总发送次数                  */
    UINT        uiTimes = size >> __AF_UNIX_PIPE_BUF_SHIFT;             /*  循环次数                    */
    UINT        uiLeft  = size & (__AF_UNIX_PIPE_BUF - 1);              /*  最后一次数量                */
    
    CPCHAR      pcSendMem = (CPCHAR)data;                               /*  当前发送数据指针            */
    BOOL        bNeedUpdateReader = LW_FALSE;
    
    if (!data || !size) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__AF_UNIX_TYPE(pafunix) == SOCK_DGRAM) {                        /*  DGRAM                       */
        if (to && tolen && (tolen > __AF_UNIX_ADDROFFSET)) {            /*  是否有地址信息              */
            _PathGetFull(cPath, MAX_FILENAME_LENGTH, ((struct sockaddr_un *)to)->sun_path);
            bHaveTo = LW_TRUE;
        }
    }
    
    __AF_UNIX_LOCK();
    do {
        if ((__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) ||
            (__AF_UNIX_TYPE(pafunix) == SOCK_SEQPACKET)) {              /*  面向连接类型                */
            if (pafunix->UNIX_iShutDFlag & __AF_UNIX_SHUTD_W) {
                __AF_UNIX_UNLOCK();
                __unixSignalNotify(flags);
                _ErrorHandle(ESHUTDOWN);                                /*  本地已经关闭                */
                return  (sstTotal);
            }
            if (pafunix->UNIX_iStatus != __AF_UNIX_STATUS_ESTAB) {
                __AF_UNIX_UNLOCK();
                __unixSignalNotify(flags);
                _ErrorHandle(ENOTCONN);                                 /*  没有连接                    */
                return  (PX_ERROR);
            }
        }
        
        if (bHaveTo) {                                                  /*  是否有地址信息              */
            pafunixRecver = __unixFind(cPath, __AF_UNIX_TYPE(pafunix), LW_FALSE);
        
        } else {
            pafunixRecver = pafunix->UNIX_pafunxPeer;
        }
    
        if (pafunixRecver == LW_NULL) {
            if (__AF_UNIX_TYPE(pafunix) == SOCK_DGRAM) {
                __AF_UNIX_UNLOCK();
                _ErrorHandle(EHOSTUNREACH);                             /*  没有目的                    */
            } else {
                __AF_UNIX_UNLOCK();
                __unixSignalNotify(flags);
                _ErrorHandle(ECONNRESET);                               /*  连接已经中断                */
            }
            return  (sstTotal);
        }
        
__try_send:
        if (__unixCanWrite(pafunixRecver)) {                            /*  可以发送                    */
            if (i < uiTimes) {
                sstWriteNum = __unixSendtoMsg(pafunix, pafunixRecver, 
                                              pcSendMem, __AF_UNIX_PIPE_BUF, 
                                              data_ex, size_ex, flags);
                if (sstWriteNum <= 0) {
                    break;                                              /*  失败则内部已经设置了 errno  */
                }
                pcSendMem += __AF_UNIX_PIPE_BUF;
                sstTotal  += __AF_UNIX_PIPE_BUF;
                data_ex = LW_NULL;                                      /*  外带数据只发送一次          */
                size_ex = 0;
                
                i++;                                                    /*  发送次数++                  */
                bNeedUpdateReader = LW_TRUE;                            /*  需要通知读端                */
                
                if (sstTotal >= size) {                                 /*  没有需要发送的数据了        */
                    break;
                
                } else {
                    goto    __try_send;                                 /*  重新尝试发送数据            */
                }
                
            } else {
                sstWriteNum = __unixSendtoMsg(pafunix, pafunixRecver, 
                                              pcSendMem, uiLeft, 
                                              data_ex, size_ex, flags); /*  最后一次发送                */
                if (sstWriteNum > 0) {
                    sstTotal += uiLeft;
                    bNeedUpdateReader = LW_TRUE;                        /*  需要通知读端                */
                }
                break;                                                  /*  发送完毕, 如果最后一次失败  */
            }                                                           /*  这里也必须退出              */
        
        } 
#if LW_CFG_NET_UNIX_MULTI_EN > 0
          else {
            __AF_UNIX_CWRITE(pafunixRecver);                            /*  不可写, 清除掉可写信号量    */
        }
#endif
        
        if (bNeedUpdateReader) {
            bNeedUpdateReader = LW_FALSE;
            __unixUpdateReader(pafunixRecver, ERROR_NONE);              /*  update remote reader        */
        }
        
        if (__AF_UNIX_IS_NBIO(pafunix, flags)) {                        /*  非阻塞 IO                   */
            __AF_UNIX_UNLOCK();
            _ErrorHandle(EWOULDBLOCK);                                  /*  需要重新读                  */
            return  (sstTotal);
        }
    
        __AF_UNIX_UNLOCK();
        ulError = __AF_UNIX_WWRITE(pafunixRecver);                      /*  等待可写                    */
        if (ulError) {
            if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
                _ErrorHandle(EWOULDBLOCK);                              /*  等待超时                    */
            } else {
                _ErrorHandle(ECONNABORTED);
            }
            return  (sstTotal);
        }
        __AF_UNIX_LOCK();
    } while (1);
    
    if (bNeedUpdateReader) {
        __unixUpdateReader(pafunixRecver, ERROR_NONE);                  /*  update remote reader        */
    }
    
#if LW_CFG_NET_UNIX_MULTI_EN > 0
    if (__unixCanWrite(pafunixRecver)) {
        __AF_UNIX_SWRITE(pafunixRecver);                                /*  other writer can write      */
    }
#endif
    __AF_UNIX_UNLOCK();
    
    return  (sstTotal);
}
/*********************************************************************************************************
** 函数名称: unix_sendmsg
** 功能描述: sendmsg
** 输　入  : pafunix   unix file
**           msg       消息
**           flags     flag
** 输　出  : NUM 数据长度
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  unix_sendmsg (AF_UNIX_T  *pafunix, const struct msghdr *msg, int flags)
{
    ssize_t     sstSendLen;

    if (!msg) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (msg->msg_iovlen == 1) {
        sstSendLen = unix_sendto2(pafunix, msg->msg_iov->iov_base, msg->msg_iov->iov_len, 
                                  msg->msg_control, msg->msg_controllen, flags, 
                                  (const struct sockaddr *)msg->msg_name, msg->msg_namelen);
                                  
    } else {
        struct iovec    liovec,*msg_iov;
        int             msg_iovlen;
        unsigned int    i, totalsize;
        ssize_t         size;
        char           *lbuf;
        char           *temp;
        
        msg_iov    = msg->msg_iov;
        msg_iovlen = msg->msg_iovlen;
        
        for (i = 0, totalsize = 0; i < msg_iovlen; i++) {
            if ((msg_iov[i].iov_len == 0) || (msg_iov[i].iov_base == LW_NULL)) {
                _ErrorHandle(EINVAL);
                return  (PX_ERROR);
            }
            totalsize += (unsigned int)msg_iov[i].iov_len;
        }
        
        lbuf = (char *)__unixBufAlloc(totalsize);
        if (lbuf == LW_NULL) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        
        liovec.iov_base = (PVOID)lbuf;
        liovec.iov_len  = (size_t)totalsize;
        
        size = totalsize;
        
        temp = lbuf;
        for (i = 0; size > 0 && i < msg_iovlen; i++) {
            int     qty = msg_iov[i].iov_len;
            lib_memcpy(temp, msg_iov[i].iov_base, qty);
            temp += qty;
            size -= qty;
        }
        
        sstSendLen = unix_sendto2(pafunix, liovec.iov_base, liovec.iov_len, 
                                  msg->msg_control, msg->msg_controllen, flags, 
                                  (const struct sockaddr *)msg->msg_name, msg->msg_namelen);
                           
        __unixBufFree(lbuf);
    }
    
    return  (sstSendLen);
}
/*********************************************************************************************************
** 函数名称: unix_sendto
** 功能描述: sendto
** 输　入  : pafunix   unix file
**           data      send buffer
**           size      send len
**           flags     flag
**           to        packet to
**           tolen     name len
** 输　出  : NUM
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  unix_sendto (AF_UNIX_T  *pafunix, const void *data, size_t size, int flags,
                      const struct sockaddr *to, socklen_t tolen)
{
    return  (unix_sendto2(pafunix, data, size, LW_NULL, 0, flags, to, tolen));
}
/*********************************************************************************************************
** 函数名称: unix_send
** 功能描述: send
** 输　入  : pafunix   unix file
**           data      send buffer
**           size      send len
**           flags     flag
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  unix_send (AF_UNIX_T  *pafunix, const void *data, size_t size, int flags)
{
    return  (unix_sendto2(pafunix, data, size, LW_NULL, 0, flags, LW_NULL, 0));
}
/*********************************************************************************************************
** 函数名称: unix_close
** 功能描述: close 
** 输　入  : pafunix   unix file
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  unix_close (AF_UNIX_T  *pafunix)
{
    AF_UNIX_T  *pafunixPeer;

    __AF_UNIX_LOCK();
    if ((__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) ||
        (__AF_UNIX_TYPE(pafunix) == SOCK_SEQPACKET)) {                  /*  面向连接类型                */
        if (pafunix->UNIX_iStatus == __AF_UNIX_STATUS_CONNECT) {        /*  正在连接                    */
            __unixUnconnect(pafunix);                                   /*  解除连接                    */
        } else if (pafunix->UNIX_iStatus == __AF_UNIX_STATUS_LISTEN) {  /*  等待其他 unix 连接          */
            __unixRefuseAll(pafunix);                                   /*  拒绝所有连接请求            */
        }
        pafunixPeer = pafunix->UNIX_pafunxPeer;
        if (pafunixPeer) {
            /*
             *  如果存在连接对等节点, 这里将激活对方, 解除连接关系, 并不改变对方的状态,
             *  并不删除对方没有接收完全的剩余数据, 如果还有剩余数据, 对方可以继续接收, 
             *  但对方一旦收完, 再次接收就会收到 ECONNABORTED 错误.
             */
            pafunixPeer->UNIX_pafunxPeer = LW_NULL;                     /*  解除对方的连接关系          */
            __unixUpdateExcept(pafunixPeer, ECONNRESET);                /*  如果可能激活对方退出等待    */
        }
        pafunix->UNIX_iStatus    = __AF_UNIX_STATUS_NONE;
        pafunix->UNIX_pafunxPeer = LW_NULL;                             /*  解除本地连接关系            */
        __unixUpdateExcept(pafunix, ENOTCONN);
    }
    __AF_UNIX_UNLOCK();
    
    __unixDelete(pafunix);                                              /*  删除同时会清除接收缓存      */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: unix_shutdown
** 功能描述: shutdown
** 输　入  : pafunix   unix file
**           how       SHUT_RD  SHUT_WR  SHUT_RDWR
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  unix_shutdown (AF_UNIX_T  *pafunix, int how)
{
    if ((how != SHUT_RD) && (how != SHUT_WR) && (how != SHUT_RDWR)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __AF_UNIX_LOCK();
    if ((__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) ||
        (__AF_UNIX_TYPE(pafunix) == SOCK_SEQPACKET)) {                  /*  面向连接类型                */
        if ((pafunix->UNIX_iStatus == __AF_UNIX_STATUS_NONE)   ||
            (pafunix->UNIX_iStatus == __AF_UNIX_STATUS_LISTEN) ||
            (pafunix->UNIX_iStatus == __AF_UNIX_STATUS_CONNECT)) {      /*  对于这三种状态不起作用      */
            __AF_UNIX_UNLOCK();
            return  (ERROR_NONE);
        }
        
        if (how == SHUT_RD) {                                           /*  关闭本地读                  */
            __unixShutdownR(pafunix);
            
        } else if (how == SHUT_WR) {                                    /*  关闭本地写                  */
            __unixShutdownW(pafunix);
            
        } else {                                                        /*  关闭本地读写                */
            __unixShutdownR(pafunix);
            __unixShutdownW(pafunix);
        }
    } else {
        __AF_UNIX_UNLOCK();
        _ErrorHandle(EOPNOTSUPP);
        return  (PX_ERROR);
    }
    __AF_UNIX_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: unix_getsockname
** 功能描述: getsockname
** 输　入  : pafunix   unix file
**           name      address
**           namelen   address len
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  unix_getsockname (AF_UNIX_T  *pafunix, struct sockaddr *name, socklen_t *namelen)
{
    struct sockaddr_un  *paddrun = (struct sockaddr_un *)name;
    size_t               stPathLen;
    size_t               stBufPathLen;

    if (paddrun && namelen && (*namelen > __AF_UNIX_ADDROFFSET)) {
        __AF_UNIX_LOCK();
        stPathLen    = lib_strlen(pafunix->UNIX_cFile);
        stBufPathLen = (*namelen - __AF_UNIX_ADDROFFSET);
        if (stPathLen > stBufPathLen) {
            stPathLen = stBufPathLen;
        }
        lib_strncpy(paddrun->sun_path, pafunix->UNIX_cFile, stBufPathLen);
        __AF_UNIX_UNLOCK();
        paddrun->sun_family = AF_UNIX;
        paddrun->sun_len = (uint8_t)(__AF_UNIX_ADDROFFSET + stPathLen); /*  不包括 \0 的总长度          */
        *namelen = paddrun->sun_len;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: unix_getpeername
** 功能描述: getpeername
** 输　入  : pafunix   unix file
**           name      address
**           namelen   address len
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  unix_getpeername (AF_UNIX_T  *pafunix, struct sockaddr *name, socklen_t *namelen)
{
    struct sockaddr_un  *paddrun = (struct sockaddr_un *)name;
    AF_UNIX_T           *pafunixPeer;
    size_t               stPathLen;
    size_t               stBufPathLen;
    
    if (paddrun && namelen && (*namelen > __AF_UNIX_ADDROFFSET)) {
        __AF_UNIX_LOCK();
        stBufPathLen = (*namelen - __AF_UNIX_ADDROFFSET);
        pafunixPeer = pafunix->UNIX_pafunxPeer;
        if (pafunixPeer) {
            stPathLen = lib_strlen(pafunixPeer->UNIX_cFile);
            if (stPathLen > stBufPathLen) {
                stPathLen = stBufPathLen;
            }
            lib_strncpy(paddrun->sun_path, pafunixPeer->UNIX_cFile, stBufPathLen);
        } else {
            stPathLen = 0;
            if (stBufPathLen > 0) {
                paddrun->sun_path[0] = PX_EOS;
            }
        }
        __AF_UNIX_UNLOCK();
        paddrun->sun_family = AF_UNIX;
        paddrun->sun_len = (uint8_t)(__AF_UNIX_ADDROFFSET + stPathLen); /*  不包括 \0 的总长度          */
        *namelen = paddrun->sun_len;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: unix_setsockopt
** 功能描述: setsockopt
** 输　入  : pafunix   unix file
**           level     level
**           optname   option
**           optval    option value
**           optlen    option value len
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  unix_setsockopt (AF_UNIX_T  *pafunix, int level, int optname, const void *optval, socklen_t optlen)
{
    INT     iRet = PX_ERROR;
    
    if (!optval || optlen < sizeof(INT)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __AF_UNIX_LOCK();
    switch (level) {
    
    case SOL_SOCKET:
        switch (optname) {
        
        case SO_LINGER:
            if (optlen < sizeof(struct linger)) {
                _ErrorHandle(EINVAL);
            } else {
                pafunix->UNIX_linger = *(struct linger *)optval;
                iRet = ERROR_NONE;
            }
            break;
        
        case SO_RCVBUF:
            pafunix->UNIX_stMaxBufSize = *(INT *)optval;
            if (pafunix->UNIX_stMaxBufSize < __AF_UNIX_DEF_BUFMIN) {
                pafunix->UNIX_stMaxBufSize = __AF_UNIX_DEF_BUFMIN;
            } else if (pafunix->UNIX_stMaxBufSize > __AF_UNIX_DEF_BUFMAX) {
                pafunix->UNIX_stMaxBufSize = __AF_UNIX_DEF_BUFMAX;
            }
            iRet = ERROR_NONE;
            break;
            
        case SO_SNDTIMEO:
            if (optlen == sizeof(struct timeval)) {
                pafunix->UNIX_ulSendTimeout = __unixTvToTicks((struct timeval *)optval);
            } else {
                pafunix->UNIX_ulSendTimeout = __unixMsToTicks(*(INT *)optval);
            }
            iRet = ERROR_NONE;
            break;
            
        case SO_RCVTIMEO:
            if (optlen == sizeof(struct timeval)) {
                pafunix->UNIX_ulRecvTimeout = __unixTvToTicks((struct timeval *)optval);
            } else {
                pafunix->UNIX_ulRecvTimeout = __unixMsToTicks(*(INT *)optval);
            }
            iRet = ERROR_NONE;
            break;
        
        case SO_CONTIMEO:
            if (optlen == sizeof(struct timeval)) {
                pafunix->UNIX_ulConnTimeout = __unixTvToTicks((struct timeval *)optval);
            } else {
                pafunix->UNIX_ulConnTimeout = __unixMsToTicks(*(INT *)optval);
            }
            iRet = ERROR_NONE;
            break;
        
        case SO_DONTLINGER:
            if (*(INT *)optval) {
                pafunix->UNIX_linger.l_onoff = 0;
            } else {
                pafunix->UNIX_linger.l_onoff = 1;
            }
            iRet = ERROR_NONE;
            break;
            
        case SO_PASSCRED:
            pafunix->UNIX_iPassCred = *(INT *)optval;
            iRet = ERROR_NONE;
            break;
            
        default:
            _ErrorHandle(ENOSYS);
            break;
        }
        break;
        
    default:
        _ErrorHandle(ENOPROTOOPT);
        break;
    }
    __AF_UNIX_UNLOCK();
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: unix_getsockopt
** 功能描述: getsockopt
** 输　入  : pafunix   unix file
**           level     level
**           optname   option
**           optval    option value
**           optlen    option value len
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  unix_getsockopt (AF_UNIX_T  *pafunix, int level, int optname, void *optval, socklen_t *optlen)
{
    INT     iRet = PX_ERROR;
    
    if (!optval || *optlen < sizeof(INT)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __AF_UNIX_LOCK();
    switch (level) {
    
    case SOL_SOCKET:
        switch (optname) {
        
        case SO_ACCEPTCONN:
            if ((__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) ||
                (__AF_UNIX_TYPE(pafunix) == SOCK_SEQPACKET)) {
                if (pafunix->UNIX_iStatus == __AF_UNIX_STATUS_LISTEN) {
                    *(INT *)optval = 1;
                } else {
                    *(INT *)optval = 0;
                }
                iRet = ERROR_NONE;
            } else {
                _ErrorHandle(ENOTSUP);
            }
            break;
        
        case SO_LINGER:
            if (*optlen < sizeof(struct linger)) {
                _ErrorHandle(EINVAL);
            } else {
                *(struct linger *)optval = pafunix->UNIX_linger;
                iRet = ERROR_NONE;
            }
            break;
        
        case SO_RCVBUF:
            *(INT *)optval = pafunix->UNIX_stMaxBufSize;
            iRet = ERROR_NONE;
            break;
            
        case SO_SNDTIMEO:
            if (*optlen == sizeof(struct timeval)) {
                __unixTicksToTv(pafunix->UNIX_ulSendTimeout, (struct timeval *)optval);
            } else {
                *(INT *)optval = (INT)__unixTicksToMs(pafunix->UNIX_ulSendTimeout);
            }
            iRet = ERROR_NONE;
            break;
        
        case SO_RCVTIMEO:
            if (*optlen == sizeof(struct timeval)) {
                __unixTicksToTv(pafunix->UNIX_ulRecvTimeout, (struct timeval *)optval);
            } else {
                *(INT *)optval = (INT)__unixTicksToMs(pafunix->UNIX_ulRecvTimeout);
            }
            iRet = ERROR_NONE;
            break;
        
        case SO_TYPE:
            *(INT *)optval = (INT)__AF_UNIX_TYPE(pafunix);
            iRet = ERROR_NONE;
            break;
        
        case SO_CONTIMEO:
            if (*optlen == sizeof(struct timeval)) {
                __unixTicksToTv(pafunix->UNIX_ulConnTimeout, (struct timeval *)optval);
            } else {
                *(INT *)optval = (INT)__unixTicksToMs(pafunix->UNIX_ulConnTimeout);
            }
            iRet = ERROR_NONE;
            break;
            
        case SO_PASSCRED:
            *(INT *)optval = pafunix->UNIX_iPassCred;
            iRet = ERROR_NONE;
            break;
        
        default:
            _ErrorHandle(ENOSYS);
            break;
        }
        break;
        
    default:
        _ErrorHandle(ENOPROTOOPT);
        break;
    }
    __AF_UNIX_UNLOCK();
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: unix_ioctl
** 功能描述: ioctl
** 输　入  : pafunix   unix file
**           iCmd      命令
**           pvArg     参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  unix_ioctl (AF_UNIX_T  *pafunix, INT  iCmd, PVOID  pvArg)
{
    INT     iRet = ERROR_NONE;
    
    switch (iCmd) {
    
    case FIOGETFL:
        if (pvArg) {
            *(INT *)pvArg = pafunix->UNIX_iFlag;
        }
        break;
        
    case FIOSETFL:
        if ((INT)pvArg & O_NONBLOCK) {
            pafunix->UNIX_iFlag |= O_NONBLOCK;
        } else {
            pafunix->UNIX_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    case FIONREAD:
        if (pvArg) {
            *(INT *)pvArg = (INT)pafunix->UNIX_unixq.UNIQ_stTotal;
        }
        break;
        
    case FIONBIO:
        if (pvArg && *(INT *)pvArg) {
            pafunix->UNIX_iFlag |= O_NONBLOCK;
        } else {
            pafunix->UNIX_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    default:
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
        break;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __unix_have_event
** 功能描述: 检测对应的控制块是否有指定的事件
** 输　入  : pafunix   unix file
**           type      事件类型
**           piSoErr   如果等待的事件有效则更新 SO_ERROR
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
** 说  明  :

SELREAD:
    1>.套接口有数据可读
    2>.该连接的读这一半关闭 (也就是接收了FIN的TCP连接). 对这样的套接口进行读操作将不阻塞并返回0
    3>.该套接口是一个侦听套接口且已完成的连接数不为0.
    4>.其上有一个套接口错误待处理，对这样的套接口的读操作将不阻塞并返回-1, 并设置errno.
       可以通过设置 SO_ERROR 选项调用 getsockopt 函数获得.
       
SELWRITE:
    1>.套接口有可用于写的空间.
    2>.该连接的写这一半关闭，对这样的套接口进行写操作将产生SIGPIPE信号.
    3>.该套接口使用非阻塞的方式connect建立连接，并且连接已经异步建立，或则connect已经以失败告终.
    4>.其上有一个套接口错误待处理.
    
SELEXCEPT
    1>.面向连接的套接口没有连接.
*********************************************************************************************************/
int __unix_have_event (AF_UNIX_T *pafunix, int type, int  *piSoErr)
{
    INT     iEvent = 0;

    switch (type) {

    case SELREAD:                                                       /*  是否可读                    */
        __AF_UNIX_LOCK();
        if ((__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) ||
            (__AF_UNIX_TYPE(pafunix) == SOCK_SEQPACKET)) {
            switch (pafunix->UNIX_iStatus) {
            
            case __AF_UNIX_STATUS_NONE:
                *piSoErr = ENOTCONN;                                    /*  没有连接                    */
                iEvent   = 1;
                break;
                
            case __AF_UNIX_STATUS_LISTEN:
                if (__unixCanAccept(pafunix)) {                         /*  可以 accept                 */
                    *piSoErr = ERROR_NONE;
                    iEvent   = 1;
                }
                break;
                
            case __AF_UNIX_STATUS_CONNECT:
                *piSoErr = ENOTCONN;                                    /*  正在连接过程中              */
                iEvent   = 1;
                break;
                
            case __AF_UNIX_STATUS_ESTAB:
                if (pafunix->UNIX_pafunxPeer == LW_NULL) {
                    *piSoErr = ECONNRESET;                              /*  连接已经中断                */
                    iEvent   = 1;
                
                } else if (__unixCanRead(pafunix, 0, 0)) {              /*  可读                        */
                    *piSoErr = ERROR_NONE;
                    iEvent   = 1;
                
                } else if (pafunix->UNIX_iShutDFlag & __AF_UNIX_SHUTD_R) {
                    *piSoErr = ENOTCONN;                                /*  读已经被停止了              */
                    iEvent   = 1;
                }
                break;
                
            default:
                *piSoErr = ENOTCONN;                                    /*  不会运行到这里              */
                iEvent   = 1;
                break;
            }
        } else {
            if (__unixCanRead(pafunix, 0, 0)) {                         /*  可读                        */
                *piSoErr = ERROR_NONE;
                iEvent   = 1;
            }
        }
        __AF_UNIX_UNLOCK();
        break;

    case SELWRITE:                                                      /*  是否可写                    */
        __AF_UNIX_LOCK();                                               /*  防止 peer 被删除            */
        if ((__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) ||
            (__AF_UNIX_TYPE(pafunix) == SOCK_SEQPACKET)) {
            switch (pafunix->UNIX_iStatus) {
            
            case __AF_UNIX_STATUS_NONE:
            case __AF_UNIX_STATUS_LISTEN:
                *piSoErr = ENOTCONN;                                    /*  没有连接                    */
                iEvent   = 1;
                break;
                
            case __AF_UNIX_STATUS_CONNECT:
                break;                                                  /*  需要等待                    */
            
            case __AF_UNIX_STATUS_ESTAB:
                if (pafunix->UNIX_pafunxPeer == LW_NULL) {
                    *piSoErr = ECONNRESET;                              /*  连接已经中断                */
                    iEvent   = 1;
                
                } else if (pafunix->UNIX_iShutDFlag & __AF_UNIX_SHUTD_W) {
                    *piSoErr = ESHUTDOWN;                               /*  写已经被停止了              */
                    iEvent   = 1;
                
                } else if (__unixCanWrite(pafunix->UNIX_pafunxPeer)) {  /*  检查目标是否可写            */
                    *piSoErr = ERROR_NONE;
                    iEvent   = 1;
                }
                break;
                
            default:
                *piSoErr = ENOTCONN;                                    /*  不会运行到这里              */
                iEvent   = 1;
                break;
            }
        } else {
            if (pafunix->UNIX_pafunxPeer) {                             /*  如果存在远程端              */
                if (__unixCanWrite(pafunix->UNIX_pafunxPeer)) {         /*  检查目标是否可写            */
                    *piSoErr = ERROR_NONE;
                    iEvent   = 1;
                }
            
            } else {
                *piSoErr = ERROR_NONE;
                iEvent   = 1;                                           /*  没有目标的 DGRAM            */
            }
        }
        __AF_UNIX_UNLOCK();
        break;
        
    case SELEXCEPT:                                                     /*  是否异常                    */
        __AF_UNIX_LOCK();
        if ((__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) ||
            (__AF_UNIX_TYPE(pafunix) == SOCK_SEQPACKET)) {
            if (pafunix->UNIX_iStatus == __AF_UNIX_STATUS_ESTAB) {      /*  estab 状态但无连接          */
                if (pafunix->UNIX_pafunxPeer == LW_NULL) {
                    iEvent = 1;                                         /*  不设置 SO_ERROR 继承之前的值*/
                }
            }
        }
        __AF_UNIX_UNLOCK();
        break;
    }
    
    return  (iEvent);
}

#endif                                                                  /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_NET_UNIX_EN > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
