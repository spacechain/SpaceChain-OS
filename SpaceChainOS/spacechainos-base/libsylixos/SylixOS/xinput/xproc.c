/**
 * @file
 * SylixOS multi-input device support kernel module.
 */

/*
 * Copyright (c) 2006-2013 SylixOS Group.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 4. This code has been or is applying for intellectual property protection 
 *    and can only be used with acoinfo software products.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Author: Han.hui <sylixos@gmail.com>
 *
 */

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include <SylixOS.h>

#include "xdev.h"

#define FILE_MAX_SZ     2048

/*
 *  proc file read functions
 */
static ssize_t  xproc_input_read(PLW_PROCFS_NODE  p_pfsn,
                                 PCHAR            pcBuffer,
                                 size_t           stMaxBytes,
                                 off_t            oft);
/*
 *  proc file function set
 */
static LW_PROCFS_NODE_OP    xproc_input_funcs = {
        xproc_input_read, NULL
};

/*
 *  static proc file node
 */
static LW_PROCFS_NODE   xinput_proc[] = {
        LW_PROCFS_INIT_NODE("xinput",
                            (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH),
                            &xproc_input_funcs, NULL, FILE_MAX_SZ),
};

/*
 *  proc file read functions
 */
static ssize_t  xproc_input_read (PLW_PROCFS_NODE  p_pfsn,
                                  PCHAR            pcBuffer,
                                  size_t           stMaxBytes,
                                  off_t            oft)
{
    static const CHAR     hdr[] = \
            "devices                 type status\n";
    PCHAR        pcFileBuffer;
    size_t       stRealSize;
    size_t       stCopeBytes;

    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }

    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {
        int          i;
        input_dev_t *input;

        stRealSize = bnprintf(pcFileBuffer, FILE_MAX_SZ, 0, hdr);

        input = xdev_kbd_array;
        for (i = 0; i < xdev_kbd_num; i++) {
            stRealSize = bnprintf(pcFileBuffer, FILE_MAX_SZ, stRealSize,
                                  "%-23s %-4s %s\n", input->dev, "kbd",
                                  (input->fd < 0) ? "close" : "open");
            input++;
        }

        input = xdev_mse_array;
        for (i = 0; i < xdev_mse_num; i++) {
            stRealSize = bnprintf(pcFileBuffer, FILE_MAX_SZ, stRealSize,
                                  "%-23s %-4s %s\n", input->dev, "mse",
                                  (input->fd < 0) ? "close" : "open");
            input++;
        }

        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    }

    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }

    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);

    return  ((ssize_t)stCopeBytes);
}

/*
 *  proc file init
 */
void xinput_proc_init (void)
{
    API_ProcFsMakeNode(&xinput_proc[0], "/");
}

/*
 *  proc file deinit
 */
void xinput_proc_deinit (void)
{
    if (API_ProcFsRemoveNode(&xinput_proc[0], NULL)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Serious error: /proc/xinput file busy now, "
                                           "the system will become unstable!\r\n");
    }
}

/*
 * end
 */
