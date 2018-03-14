/*
 * YAFFS: Yet Another Flash File System. A NAND-flash specific file system.
 *
 *        Nor-flash interface
 * 
 * Copyright (c) 2006-2014 SylixOS Group.
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

#define  __SYLIXOS_KERNEL
#include "SylixOS.h"

#if LW_CFG_YAFFS_NOR_EN > 0

#include "yaffs_guts.h"
#include "yaffs_nor.h"
#include "yaffs_trace.h"

#define YNOR_PREMARKER          (0xf6)
#define YNOR_POSTMARKER         (0xf0)

#define FORMAT_OFFSET           ((addr_t)nordev->nor.chunk_size * nordev->nor.chunk_per_block)
#define FORMAT_VALUE            0x1234

/*
 * address transform
 */
static inline addr_t norFormat2Addr (struct yaffs_devnor *nordev, int  block)
{
    addr_t  addr = norBlock2Addr(nordev, block);
    
    addr += FORMAT_OFFSET;
    
    return  (addr);
}

static inline addr_t norChunk2Addr (struct yaffs_devnor *nordev, int  chunk, addr_t *spare_addr)
{
    addr_t  addr;
    int     block;
    int     chunk_in_block;
    
    block          = chunk / nordev->nor.chunk_per_block;
    chunk_in_block = chunk % nordev->nor.chunk_per_block;
    
    addr  = norBlock2Addr(nordev, block);
    addr += (addr_t)chunk_in_block * nordev->nor.chunk_size;
    
    if (spare_addr) {
        *spare_addr = addr + nordev->nor.bytes_per_chunk;
    }
    
    return  (addr);
}

/*
 * yaffs nor block format
 */
static void  norFormatBlock (struct yaffs_devnor *nordev, unsigned int block)
{
    addr_t  addr_format;
    u32     format = FORMAT_VALUE;

    if (nordev->nor.nor_erase_fn(nordev, block) != YAFFS_OK) {
        yaffs_trace(YAFFS_TRACE_ERROR, "Can not erase block %d\n", block);
        return;
    }
    
    addr_format = norFormat2Addr(nordev, block);
    
    if (nordev->nor.nor_write_fn(nordev, addr_format, &format, sizeof(u32)) != YAFFS_OK) {
        yaffs_trace(YAFFS_TRACE_ERROR, "Can not format block %d\n", block);
        return;
    }
}

static void norUnformatBlock (struct yaffs_devnor *nordev, unsigned int block)
{
    addr_t  addr_format;
    u32     format = 0;
    
    addr_format = norFormat2Addr(nordev, block);
    
    if (nordev->nor.nor_write_fn(nordev, addr_format, &format, sizeof(u32)) != YAFFS_OK) {
        yaffs_trace(YAFFS_TRACE_ERROR, "Can not unformat block %d\n", block);
        return;
    }
}

static int norIsFormatBlock (struct yaffs_devnor *nordev, unsigned int block)
{
    addr_t  addr_format;
    u32     format;
    
    addr_format = norFormat2Addr(nordev, block);
    
    if (nordev->nor.nor_read_fn(nordev, addr_format, &format, sizeof(u32)) != YAFFS_OK) {
        yaffs_trace(YAFFS_TRACE_ERROR, "Can not unformat block %d\n", block);
        return  (0);
    }
    
    return  (format == FORMAT_VALUE);
}

/*
 * yaffs nor drv
 */
int ynorWriteChunk (struct yaffs_dev *dev,int chunk,
                    const u8 *data, int data_len,
                    const u8 *oob, int oob_len)
{
    struct yaffs_devnor *nordev = (struct yaffs_devnor *)dev;
    struct yaffs_spare  *spare = (struct yaffs_spare *)oob;
    struct yaffs_spare   tmpspare;
    
    addr_t  data_addr;
    addr_t  spare_addr;
    
    data_addr = norChunk2Addr(nordev, chunk, &spare_addr);
    
    /* We should only be getting called for one of 3 reasons:
     * Writing a chunk: data and spare will not be NULL
     * Writing a deletion marker: data will be NULL, spare not NULL
     * Writing a bad block marker: data will be NULL, spare not NULL
     */
    if (data && oob) {
        if (spare->page_status != 0xff) {
            BUG();
        }
        
        /* Write a pre-marker */
        lib_memset(&tmpspare, 0xff, sizeof(struct yaffs_spare));
        tmpspare.page_status = YNOR_PREMARKER;
        nordev->nor.nor_write_fn(nordev, spare_addr, &tmpspare, sizeof(struct yaffs_spare));

        /* Write the data */
        nordev->nor.nor_write_fn(nordev, data_addr, data, data_len);

        /* Write the real tags, but override the premarker */
        lib_memcpy(&tmpspare, spare, sizeof(struct yaffs_spare));
        tmpspare.page_status = YNOR_PREMARKER;
        nordev->nor.nor_write_fn(nordev, spare_addr, &tmpspare, sizeof(struct yaffs_spare));

        /* Write a post-marker */
        tmpspare.page_status = YNOR_POSTMARKER;
        nordev->nor.nor_write_fn(nordev, spare_addr, &tmpspare, sizeof(struct yaffs_spare));
    
    } else if (spare) {
        /* This has to be a read-modify-write operation to handle NOR-ness */
        lib_memcpy(&tmpspare, spare, sizeof(struct yaffs_spare));
        nordev->nor.nor_write_fn(nordev, spare_addr, &tmpspare, sizeof(struct yaffs_spare));
    
    } else {
        BUG();
    }
    
    return  (YAFFS_OK);
}

int ynorReadChunk (struct yaffs_dev *dev, int chunk,
                   u8 *data, int data_len,
                   u8 *oob, int oob_len,
                   enum yaffs_ecc_result *ecc_result)
{
    struct yaffs_devnor *nordev = (struct yaffs_devnor *)dev;
    struct yaffs_spare  *spare = (struct yaffs_spare *)oob;
    
    addr_t  data_addr;
    addr_t  spare_addr;
    
    data_addr = norChunk2Addr(nordev, chunk, &spare_addr);
    
    if (data) {
        nordev->nor.nor_read_fn(nordev, data_addr, data, data_len);
    }
    
    if (oob) {
        nordev->nor.nor_read_fn(nordev, spare_addr, spare, sizeof(struct yaffs_spare));

        /* If the page status is YNOR_POSTMARKER then it was written properly
         * so change that to 0xFF so that the rest of yaffs is happy.
         */
        if (spare->page_status == YNOR_POSTMARKER) {
            spare->page_status = 0xff;

        } else if ((spare->page_status != 0xff) &&
                   ((spare->page_status | YNOR_PREMARKER) != 0xff)) {
            spare->page_status = YNOR_PREMARKER;
        }
    }

    if (ecc_result) {
        *ecc_result = YAFFS_ECC_RESULT_NO_ERROR;
    }
    
    return  (YAFFS_OK);
}

int ynorEraseBlock (struct yaffs_dev *dev, int block)
{
    struct yaffs_devnor *nordev = (struct yaffs_devnor *)dev;
    
    if ((block < dev->param.start_block) || (block > dev->param.end_block)) {
        yaffs_trace(YAFFS_TRACE_ALWAYS,
                    "Attempt to erase non-existant block %d\n",
                    block);
        return  (YAFFS_FAIL);

    } else {
        norUnformatBlock(nordev, block);
        norFormatBlock(nordev, block);
        return  (YAFFS_OK);
    }
}

int ynorInitialise (struct yaffs_dev *dev)
{
    int  i;
    struct yaffs_devnor *nordev = (struct yaffs_devnor *)dev;

    nordev->nor.nor_initialise_fn(nordev);

    /* Go through the blocks formatting them if they are not formatted */
    for (i = dev->param.start_block; i <= dev->param.end_block; i++) {
        if (!norIsFormatBlock(nordev, i)) {
            norFormatBlock(nordev, i);
        }
    }

    return  (YAFFS_OK);
}

int ynorDeinitialise (struct yaffs_dev *dev)
{
    struct yaffs_devnor *nordev = (struct yaffs_devnor *)dev;
    
    nordev->nor.nor_deinitialise_fn(nordev);
    
    return  (YAFFS_OK);
}

#endif /* LW_CFG_YAFFS_NOR_EN > 0 */
/*
 * end
 */
