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
** 文   件   名: ttinyShellModemCmd.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 07 月 28 日
**
** 描        述: 系统 modem 相关命令定义.

** BUG:
2013.06.10  兼容 -fsigned-char .
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "../SylixOS/shell/ttinyShell/ttinyShell.h"
#include "../SylixOS/shell/ttinyShell/ttinyShellLib.h"
#include "../SylixOS/shell/ttinyVar/ttinyVarLib.h"
/*********************************************************************************************************
  XMODE 控制字
*********************************************************************************************************/
#define __LW_XMODEM_SOH             0x01
#define __LW_XMODEM_EOT             0x04
#define __LW_XMODEM_ACK             0x06
#define __LW_XMODEM_NAK             0x15
#define __LW_XMODEM_CAN             0x18
#define __LW_XMODEM_PAD             0x1A                                /*  数据补全填充字              */
/*********************************************************************************************************
  XMODE 控制字操作
*********************************************************************************************************/
#define __LW_XMODEM_SEND_CMD(c)     {                               \
                                        CHAR    cCmd = c;           \
                                        write(STD_OUT, &cCmd, 1);   \
                                    }
/*********************************************************************************************************
  XMODE 配置
*********************************************************************************************************/
#define __LW_XMODEM_DATA_LEN        128                                 /*  数据块大小                  */
#define __LW_XMODEM_PACKET_LEN      (__LW_XMODEM_DATA_LEN + 4)          /*  数据包大小                  */
#define __LW_XMODEM_TX_TIMEOUT      3                                   /*  以秒作为超时时间的单位      */
#define __LW_XMODEM_RX_TIMEOUT      3                                   /*  以秒作为超时时间的单位      */
/*********************************************************************************************************
** 函数名称: __tshellXmodemCleanup
** 功能描述: Xmodem 操作遇到 control-C 时的清除动作
** 输　入  : iFile     操作文件描述符
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tshellXmodemCleanup (INT  iFile)
{
    struct timeval  tmval = {2, 0};
           CHAR     cTemp[16];
    
    if (iFile >= 0) {
        close(iFile);
    }
    
    /*
     *  由于一些终端软件结束文件传输时, 会连续发送 __LW_XMODEM_CAN 0x18 命令, 
     *  所以会造成系统转换为 OPT_TERMINAL 模式时与 ctrl+x 命令冲突, 造成系统重启,
     *  所以这里需要等待 2s 的平静时间.
     */
    while (waitread(STD_IN, &tmval) == 1) {
        if (read(STD_IN, cTemp, sizeof(cTemp)) <= 0) {
            break;
        }
    }
    
    ioctl(STD_IN, FIOSETOPTIONS, OPT_TERMINAL);
    ioctl(STD_OUT, FIOSETOPTIONS, OPT_TERMINAL);
    
    ioctl(STD_IN, FIORFLUSH);                                           /*  清除读缓冲数据              */
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdXmodems
** 功能描述: 系统命令 "xmodems" (发送给 remote 一个文件)
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdXmodems (INT  iArgC, PCHAR  ppcArgV[])
{
    INT             i;
    INT             j;
    
    BOOL            bIsEot = LW_FALSE;
    BOOL            bStart = LW_FALSE;
    
    INT             iFile;
    INT             iRetVal;
    UCHAR           ucRead;
    UCHAR           ucTemp[__LW_XMODEM_PACKET_LEN] = {__LW_XMODEM_SOH};
    UCHAR           ucSeq = 1;
    UCHAR           ucChkSum;
    
    ssize_t         sstRecvNum;
    ssize_t         sstReadNum;
    
    fd_set          fdsetRead;
    struct timeval  timevalTO = {__LW_XMODEM_TX_TIMEOUT, 0};
    
    
    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    iFile = open(ppcArgV[1], O_RDONLY);
    if (iFile < 0) {
        fprintf(stderr, "can not open source file!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    API_ThreadCleanupPush(__tshellXmodemCleanup, (PVOID)iFile);         /*  加入清除函数                */
    
    ioctl(STD_IN, FIOSETOPTIONS, OPT_RAW);                              /*  将标准文件改为 raw 模式     */
    ioctl(STD_OUT, FIOSETOPTIONS, OPT_RAW);                             /*  将标准文件改为 raw 模式     */
    
    /*
     *  构建第一个数据包
     */
    sstReadNum = read(iFile, &ucTemp[3], __LW_XMODEM_DATA_LEN);
    if (sstReadNum <= 0) {
        __LW_XMODEM_SEND_CMD(__LW_XMODEM_EOT);                          /*  传输结束                    */
        API_ThreadCleanupPop(LW_TRUE);
        printf("0 bytes read from source file!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    for (j = (INT)sstReadNum; j < __LW_XMODEM_DATA_LEN; j++) {          /*  补全数据                    */
        ucTemp[j + 3] = __LW_XMODEM_PAD;
    }
    
    ucTemp[1] = (UCHAR)(ucSeq);                                         /*  序列号                      */
    ucTemp[2] = (UCHAR)(~ucSeq);
    
    ucChkSum = 0;
    for (j = 3; j < (__LW_XMODEM_DATA_LEN + 3); j++) {
        ucChkSum = (UCHAR)(ucChkSum + ucTemp[j]);                       /*  计算校验和                  */
    }
    ucTemp[__LW_XMODEM_DATA_LEN + 3] = ucChkSum;
    
    /*
     *  开始循环发送文件
     */
    FD_ZERO(&fdsetRead);
    /*
     *  超时次数默认为 20 次
     */
    for (i = 0; i < 20; i++) {
        FD_SET(STD_IN, &fdsetRead);
        iRetVal = select(1, &fdsetRead, LW_NULL, LW_NULL, &timevalTO);  /*  等待可读                    */
        if (iRetVal != 1) {
            if (bIsEot) {
                if (bStart) {
                    write(STD_OUT, ucTemp, __LW_XMODEM_PACKET_LEN);     /*  重发数据                    */
                }
            }
            continue;                                                   /*  接收超时                    */
        }
        i = 0;                                                          /*  重置超时次数                */
        
        sstRecvNum = read(STD_IN, &ucRead, 1);
        if (sstRecvNum <= 0) {
            API_ThreadCleanupPop(LW_TRUE);
            fprintf(stderr, "standard in device error!\n");
            return  (PX_ERROR);
        }
        
        if (ucRead == __LW_XMODEM_CAN) {                                /*  接收结束                    */
            break;
        
        } else if (ucRead == __LW_XMODEM_NAK) {                         /*  需要重发                    */
            write(STD_OUT, ucTemp, __LW_XMODEM_PACKET_LEN);
            bStart = LW_TRUE;
        
        } else if (ucRead == __LW_XMODEM_ACK) {
            ucSeq++;
            if (ucSeq > 255) {
                ucSeq = 1;
            }
            bStart = LW_TRUE;
            
            if (bIsEot) {
                break;                                                  /*  发送结束                    */
            
            } else {
                sstReadNum = read(iFile, &ucTemp[3], __LW_XMODEM_DATA_LEN);
                if (sstReadNum <= 0) {
                    bIsEot = LW_TRUE;
                    __LW_XMODEM_SEND_CMD(__LW_XMODEM_EOT);              /*  发送结束                    */
                
                } else {
                    /*
                     *  补全数据
                     */
                    for (j = (INT)sstReadNum; j < __LW_XMODEM_DATA_LEN; j++) {
                        ucTemp[j + 3] = __LW_XMODEM_PAD;
                    }
                    
                    ucTemp[1] = (UCHAR)(ucSeq);                         /*  序列号                      */
                    ucTemp[2] = (UCHAR)(~ucSeq);
                    
                    ucChkSum = 0;
                    for (j = 3; j < (__LW_XMODEM_DATA_LEN + 3); j++) {
                        ucChkSum = (UCHAR)(ucChkSum + ucTemp[j]);       /*  计算校验和                  */
                    }
                    ucTemp[__LW_XMODEM_DATA_LEN + 3] = ucChkSum;
                    
                    write(STD_OUT, ucTemp, __LW_XMODEM_PACKET_LEN);     /*  发送数据                    */
                }
            }
        }
    }

    API_ThreadCleanupPop(LW_TRUE);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdXmodemr
** 功能描述: 系统命令 "xmodemr" (从 remote 接收一个文件)
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFsCmdXmodemr (INT  iArgC, PCHAR  ppcArgV[])
{
    INT             i;
    INT             j;
    
    INT             iFile;
    INT             iRetVal;
    UCHAR           ucTemp[__LW_XMODEM_PACKET_LEN];
    UCHAR           ucSeq = 1;
    UCHAR           ucChkSum;
    
    ssize_t         sstRecvNum;
    ssize_t         sstWriteNum;
    size_t          stTotalNum = 0;
    
    fd_set          fdsetRead;
    struct timeval  timevalTO = {__LW_XMODEM_RX_TIMEOUT, 0};
    
    
    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    /*
     *  检测文件是否存在
     */
    iFile = open(ppcArgV[1], O_RDONLY);
    if (iFile >= 0) {
        close(iFile);                                                   /*  关闭文件                    */
        
__re_select:
        printf("destination file is exist, overwrite? (Y/N)\n");
        read(0, ucTemp, __LW_XMODEM_DATA_LEN);
        if ((ucTemp[0] == 'N') ||
            (ucTemp[0] == 'n')) {                                       /*  不覆盖                      */
            return  (ERROR_NONE);
        
        } else if ((ucTemp[0] != 'Y') &&
                   (ucTemp[0] != 'y')) {                                /*  选择错误                    */
            goto    __re_select;
        }
    }
    
    iFile = open(ppcArgV[1], (O_WRONLY | O_CREAT | O_TRUNC), 0666);     /*  创建文件                    */
    if (iFile < 0) {
        fprintf(stderr, "can not open destination file!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    API_ThreadCleanupPush(__tshellXmodemCleanup, (PVOID)iFile);         /*  加入清除函数                */
    
    ioctl(STD_IN, FIOSETOPTIONS, OPT_RAW);                              /*  将标准文件改为 raw 模式     */
    ioctl(STD_OUT, FIOSETOPTIONS, OPT_RAW);                             /*  将标准文件改为 raw 模式     */
    
    __LW_XMODEM_SEND_CMD(__LW_XMODEM_NAK);                              /*  启动接收                    */
    
    FD_ZERO(&fdsetRead);
    /*
     *  超时次数默认为 20 次
     */
    for (i = 0; i < 20; i++) {
        FD_SET(STD_IN, &fdsetRead);
        iRetVal = select(1, &fdsetRead, LW_NULL, LW_NULL, &timevalTO);  /*  等待可读                    */
        if (iRetVal != 1) {
            stTotalNum = 0;                                             /*  清除已接收的数据            */
            __LW_XMODEM_SEND_CMD(__LW_XMODEM_NAK);                      /*  重新请求接收数据包          */
            continue;                                                   /*  接收超时                    */
        }
        i = 0;                                                          /*  重置超时次数                */
    
        sstRecvNum = read(STD_IN, &ucTemp[stTotalNum], __LW_XMODEM_PACKET_LEN - stTotalNum);
        if (sstRecvNum <= 0) {
            API_ThreadCleanupPop(LW_TRUE);
            fprintf(stderr, "standard in device error!\n");
            return  (PX_ERROR);
        }
        if (ucTemp[0] == __LW_XMODEM_EOT) {                             /*  接收结束                    */
            __LW_XMODEM_SEND_CMD(__LW_XMODEM_ACK);
            break;
        }
        stTotalNum += (size_t)sstRecvNum;
        if (stTotalNum < __LW_XMODEM_PACKET_LEN) {                      /*  数据包不完整                */
            continue;
        } else {
            stTotalNum = 0;                                             /*  已经接到一个完整的数据包    */
        }
        
        /*
         *  开始判断数据包正确性
         */
        if (ucTemp[1] != ucSeq) {                                       /*  序列号是否错误              */
            if (ucTemp[1] == (ucSeq - 1)) {
                __LW_XMODEM_SEND_CMD(__LW_XMODEM_ACK);                  /*  需要下一包数据              */
            } else {
                __LW_XMODEM_SEND_CMD(__LW_XMODEM_CAN);                  /*  结束通信                    */
                API_ThreadCleanupPop(LW_TRUE);
                fprintf(stderr, "sequence number error!\n");
                return  (PX_ERROR);
            }
            continue;
        }
        
        if (~ucTemp[1] == ucTemp[2]) {                                  /*  序列号校验错误              */
            __LW_XMODEM_SEND_CMD(__LW_XMODEM_NAK);
            continue;
        }
        
        ucChkSum = 0;
        for (j = 3; j < (__LW_XMODEM_DATA_LEN + 3); j++) {
            ucChkSum = (UCHAR)(ucChkSum + ucTemp[j]);                   /*  计算校验和                  */
        }
        
        if (ucTemp[__LW_XMODEM_DATA_LEN + 3] != ucChkSum) {             /*  校验错误                    */
            __LW_XMODEM_SEND_CMD(__LW_XMODEM_NAK);
            continue;
        }
        
        /*
         *  将数据写入目标文件
         */
        sstWriteNum = write(iFile, &ucTemp[3], __LW_XMODEM_DATA_LEN);
        if (sstWriteNum != __LW_XMODEM_DATA_LEN) {
            INT     iErrNo = errno;
            __LW_XMODEM_SEND_CMD(__LW_XMODEM_CAN);                      /*  结束通信                    */
            API_ThreadCleanupPop(LW_TRUE);
            fprintf(stderr, "write file error %s!\n", lib_strerror(iErrNo));
            return  (PX_ERROR);
        }
        
        ucSeq++;
        __LW_XMODEM_SEND_CMD(__LW_XMODEM_ACK);                          /*  需要下一包数据              */
    }
    
    API_ThreadCleanupPop(LW_TRUE);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellModemCmdInit
** 功能描述: 初始化 modem 相关命令集
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __tshellModemCmdInit (VOID)
{
    API_TShellKeywordAdd("xmodems", __tshellFsCmdXmodems);
    API_TShellFormatAdd("xmodems", " file path");
    API_TShellHelpAdd("xmodems", "send a file use xmodem protocol.\n");
    
    API_TShellKeywordAdd("xmodemr", __tshellFsCmdXmodemr);
    API_TShellFormatAdd("xmodemr", " file path");
    API_TShellHelpAdd("xmodemr", "receive a file use xmodem protocol.\n");
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
