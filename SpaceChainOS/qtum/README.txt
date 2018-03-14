
部署注意：
1. qtumd需要栈大小300多KB，建议设置栈
# shstack 500000

2. rename函数需要patch

libsylixos中libsylixos/SylixOS/system/ioLib/ioInterface.c

rename(...) 函数中
    if (_PathBuildLink(cFullFileName, MAX_FILENAME_LENGTH, 
                       LW_NULL, LW_NULL,
                       pdevhdrHdr->DEVHDR_pcName, cFullFileName) < ERROR_NONE) {
        close(iFd);
        return  (PX_ERROR);
    }
    
    unlink(cFullFileName); // 增加这行，如果新文件名已经存在，要删除

    iRet = ioctl(iFd, FIORENAME, (LONG)cFullFileName);
    
    close(iFd);
    
    if (iRet == ERROR_NONE) {
        MONITOR_EVT(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_MOVE_FROM, pcOldName);
        MONITOR_EVT(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_MOVE_TO,   pcNewName);
    }
    
    return  (iRet);
}


关闭挖矿

-staking=<true/false>
Enables or disables staking (enabled by default) 
-stakecache=<true/false>
Enables or disables the staking cache; significantly improves staking performance, but can use a lot of memory (enabled by default) 

qtum.conf中设置
staking=0
stakecache=0

另外，大部分命令可以从help的参数中找到，并设置到conf中。

