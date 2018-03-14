/*
 * Copyright © 2011 Ivan Djelic <ivan.djelic@parrot.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This file is the header for the NAND BCH ECC implementation.
 */

#ifndef __LINUX_MTD_NAND_BCH_H
#define __LINUX_MTD_NAND_BCH_H

struct mtd_info;
struct nand_bch_control;

#if defined(CONFIG_NAND_ECC_BCH)

static inline int mtd_nand_has_bch(void) { return 1; }

/*
 * Calculate BCH ecc code
 */
int nand_bch_calculate_ecc(struct mtd_info *mtd, const u_char *dat,
			   u_char *ecc_code);

/*
 * Detect and correct bit errors
 */
int nand_bch_correct_data(struct mtd_info *mtd, u_char *dat, u_char *read_ecc,
			  u_char *calc_ecc);
/*
 * Initialize BCH encoder/decoder
 */
struct nand_bch_control *
nand_bch_init(struct mtd_info *mtd, unsigned int eccsize,
	      unsigned int eccbytes, struct nand_ecclayout **ecclayout);
/*
 * Release BCH encoder/decoder resources
 */
void nand_bch_free(struct nand_bch_control *nbc);

#else /* !CONFIG_NAND_ECC_BCH */

static LW_INLINE int mtd_nand_has_bch(void) { return 0; }

static LW_INLINE int
nand_bch_calculate_ecc(struct mtd_info *mtd, const u_char *dat,
		       u_char *ecc_code)
{
	return -1;
}

static LW_INLINE int
nand_bch_correct_data(struct mtd_info *mtd, unsigned char *buf,
		      unsigned char *read_ecc, unsigned char *calc_ecc)
{
	return -1;
}

static LW_INLINE struct nand_bch_control *
nand_bch_init(struct mtd_info *mtd, unsigned int eccsize,
	      unsigned int eccbytes, struct nand_ecclayout **ecclayout)
{
	return NULL;
}

static LW_INLINE void nand_bch_free(struct nand_bch_control *nbc) {}

#endif /* CONFIG_NAND_ECC_BCH */

#endif /* __LINUX_MTD_NAND_BCH_H */
