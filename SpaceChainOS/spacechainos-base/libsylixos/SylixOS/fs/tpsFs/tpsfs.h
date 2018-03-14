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
** 文   件   名: tpsfs.h
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: tpsfs API 实现

** BUG:
*********************************************************************************************************/

#ifndef __TSPSFS_H
#define __TSPSFS_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0

/*********************************************************************************************************
  目录结构
*********************************************************************************************************/

typedef struct {
    PTPS_INODE          DIR_pinode;                                     /* 当前目录                     */
    PTPS_ENTRY          DIR_pentry;                                     /* 当前entry                    */
    TPS_OFF_T           DIR_offPos;                                     /* 当前entry在目录中的偏移      */
} TPS_DIR;
typedef TPS_DIR        *PTPS_DIR;

/*********************************************************************************************************
  API
*********************************************************************************************************/
#ifdef __cplusplus 
extern "C" { 
#endif 
                                                                        /* 打开文件                     */
errno_t     tpsFsOpen(PTPS_SUPER_BLOCK psb, CPCHAR pcPath, INT iFlags,
                      INT iMode, PCHAR *ppcRemain, PTPS_INODE *ppinode);
                                                                        /* 关闭文件                     */
errno_t     tpsFsClose(PTPS_INODE pinode);
                                                                        /* 提交文件头属性修改           */
errno_t     tpsFsFlushHead(PTPS_INODE pinode);
                                                                        /* 删除entry                    */
errno_t     tpsFsRemove(PTPS_SUPER_BLOCK psb, CPCHAR pcPath);
                                                                        /* 移动entry                    */
errno_t     tpsFsMove(PTPS_SUPER_BLOCK psb, CPCHAR pcPathSrc, CPCHAR pcPathDst);
                                                                        /* 建立硬链接                   */
errno_t     tpsFsLink(PTPS_SUPER_BLOCK psb, CPCHAR pcPathSrc, CPCHAR pcPathDst);
                                                                        /* 读文件                       */
errno_t     tpsFsRead(PTPS_INODE pinode, PUCHAR pucBuff, TPS_OFF_T off,
                      TPS_SIZE_T szLen, TPS_SIZE_T *pszRet);
                                                                        /* 写文件                       */
errno_t     tpsFsWrite(PTPS_INODE pinode, PUCHAR pucBuff, TPS_OFF_T off,
                       TPS_SIZE_T szLen, TPS_SIZE_T *pszRet);
                                                                        /* 截断文件                     */
errno_t     tpsFsTrunc(PTPS_INODE pinode, TPS_SIZE_T szNewSize);
                                                                        /* 创建目录                     */
errno_t     tpsFsMkDir(PTPS_SUPER_BLOCK psb, CPCHAR pcPath, INT iFlags, INT iMode);
                                                                        /* 打开目录                     */
errno_t     tpsFsOpenDir(PTPS_SUPER_BLOCK psb, CPCHAR pcPath, PTPS_DIR *ppdir);
                                                                        /* 关闭目录                     */
errno_t     tpsFsCloseDir(PTPS_DIR pdir);
                                                                        /* 读取目录                     */
errno_t     tpsFsReadDir(PTPS_INODE pinodeDir, BOOL bInHash, TPS_OFF_T off, PTPS_ENTRY *ppentry);
                                                                        /* 同步文件                     */
errno_t     tpsFsSync(PTPS_INODE pinode);
                                                                        /* 同步整个文件系统分区         */
errno_t     tpsFsVolSync (PTPS_SUPER_BLOCK psb);
                                                                        /* tpsfs 获得文件 stat          */
VOID        tpsFsStat(PTPS_SUPER_BLOCK  psb, PTPS_INODE  pinode, struct stat *pstat);
                                                                        /* tpsfs 获得文件系统 statfs    */
VOID        tpsFsStatfs(PTPS_SUPER_BLOCK  psb, struct statfs *pstatfs);
                                                                        /* 获取文件大小                 */
TPS_SIZE_T  tpsFsGetSize(PTPS_INODE pinode);
                                                                        /* 获取文件模式                 */
errno_t     tpsFsGetmod(PTPS_INODE pinode);
                                                                        /* 修改文件模式                 */
errno_t     tpsFsChmod(PTPS_INODE pinode, INT iMode);
                                                                        /* 修改文件所有者               */
errno_t     tpsFsChown(PTPS_INODE pinode, uid_t uid, gid_t gid);
                                                                        /* 修改文件时间                 */
errno_t     tpsFsChtime(PTPS_INODE pinode, struct utimbuf  *utim);
                                                                        /* 回写脏inode节点              */
VOID        tpsFsFlushInodes(PTPS_SUPER_BLOCK psb);

#ifdef __cplusplus 
}
#endif 

#endif                                                                  /* LW_CFG_TPSFS_EN > 0          */
#endif                                                                  /* __TSPSFS_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
