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

#ifndef __YAFFS_NOR_H__
#define __YAFFS_NOR_H__

/*
 * nor device
 * 
 * read, write functions 'addr' 32bits align, 'len' 16bits align
 */
struct yaffs_devnor;
 
struct yaffs_nor {
    addr_t  nor_base;           /* nor-flash base address */
    
    int     block_size;         /* Eg. 32K, 64K, 128K... */
    int     chunk_size;         /* Now must 512 + 16 */
    
    int     bytes_per_chunk;    /* Now must 512 */
    int     spare_per_chunk;    /* Now must 16 */
    
    int     chunk_per_block;    /* Must block_size / chunk_size */
    int     block_per_chip;
    
    int   (*nor_erase_fn)(struct yaffs_devnor *nordev, int block_no);
    
    int   (*nor_read_fn)(struct yaffs_devnor *nordev, addr_t addr, void *data, size_t len);
    int   (*nor_write_fn)(struct yaffs_devnor *nordev, addr_t addr, const void *data, size_t len);

    int   (*nor_initialise_fn)(struct yaffs_devnor *nordev);
    int   (*nor_deinitialise_fn)(struct yaffs_devnor *nordev);
    
    void   *driver_context;
};

/*
 * yaffs device
 */
struct yaffs_devnor {
    struct yaffs_dev  dev;
    struct yaffs_nor  nor;
};

/*
 * address transform
 */
static inline addr_t norBlock2Addr (struct yaffs_devnor *nordev, int  block)
{
    addr_t  addr = nordev->nor.nor_base;
    
    addr += ((addr_t)block * nordev->nor.block_size);
    
    return  (addr);
}

/*
 * yaffs drv
 */
int ynorWriteChunk(struct yaffs_dev *dev,int chunk,
                   const u8 *data, int data_len,
                   const u8 *oob, int oob_len);
int ynorReadChunk(struct yaffs_dev *dev, int chunk,
                  u8 *data, int data_len,
                  u8 *oob, int oob_len,
                  enum yaffs_ecc_result *ecc_result);
int ynorEraseBlock(struct yaffs_dev *dev, int block);
int ynorInitialise(struct yaffs_dev *dev);
int ynorDeinitialise(struct yaffs_dev *dev);

/*
 * Add nor flash chip to yaffs eg.
 *
 * struct yaffs_dev *ynorInstall (const char *name)
 * {
 *     static char                 devname[64];
 *     static struct yaffs_devnor  nordev;
 *            struct yaffs_nor    *nor;
 *            struct yaffs_dev    *dev;
 *            struct yaffs_param  *param;
 *            struct yaffs_driver *drv;
 *
 *     nor = &nordev.nor;
 *     dev = &nordev.dev;
 * 
 *     param = &dev->param;
 *     drv   = &dev->drv;
 * 
 *     lib_bzero(&nordev, sizeof(struct yaffs_devnor));
 *     lib_strlcpy(devname, name, sizeof(devname));
 * 
 *     nor->nor_base        = NOR_BASE_ADDR;
 *     nor->block_size      = 64 * 1024;
 *     nor->chunk_size      = 512 + 16;
 *     nor->bytes_per_chunk = 512;
 *     nor->spare_per_chunk = 16;
 *     nor->chunk_per_block = nor->block_size / nor->chunk_size;
 *     nor->block_per_chip  = NOR_CHIP_SIZE / nor->block_size;
 * 
 *     nor->nor_erase_fn        = ...;
 *     nor->nor_read_fn         = ...;
 *     nor->nor_write_fn        = ...;
 *     nor->nor_initialise_fn   = ...;
 *     nor->nor_deinitialise_fn = ...;
 * 
 *     param->name                  = devname;
 *     param->total_bytes_per_chunk = nor->bytes_per_chunk;
 *     param->chunks_per_block      = nor->chunk_per_block;
 *     param->n_reserved_blocks     = 2;
 *     param->start_block           = 0;                        // Start block
 *     param->end_block             = nor->block_per_chip - 1;  // Last block
 *     param->use_nand_ecc          = 0;                        // use YAFFS's ECC
 *     param->disable_soft_del      = 1;
 *     param->n_caches              = 10;
 * 
 *     drv->drv_write_chunk_fn  = ynorWriteChunk;
 *     drv->drv_read_chunk_fn   = ynorReadChunk;
 *     drv->drv_erase_fn        = ynorEraseBlock;
 *     drv->drv_initialise_fn   = ynorInitialise;
 *     drv->drv_deinitialise_fn = ynorDeinitialise;
 * 
 *     dev->driver_context = &nordev;
 * 
 *     yaffs_add_device(dev);
 *     yaffs_mount(devname);
 * 
 *     return  (dev);
 * }
 */

#endif /* __YAFFS_NOR_H__ */
