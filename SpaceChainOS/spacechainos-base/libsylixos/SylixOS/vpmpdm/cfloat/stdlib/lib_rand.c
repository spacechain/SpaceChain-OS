/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#define  __SYLIXOS_KERNEL /* need some kernel define */
#include <sys/cdefs.h>
#include <SylixOS.h>

#define MODULUS     2147483647L
#define FACTOR      16807L
#define DIVISOR     127773L
#define REMAINDER   2836L

static INT     last_val     = 1;
static LONG    last_val_dom = 1;

/*
 * lib_rand
 */
__weak_reference(lib_rand, rand);

INT lib_rand (VOID)
{
    INT64   llTmp = (INT64)((INT64)last_val * 1103515245);
    
    llTmp  += ((INT64)FACTOR ^ API_TimeGet());
    llTmp >>= 16;
    
    last_val = (INT)(llTmp % __ARCH_UINT_MAX);
    
    return  ((last_val >= 0) ? last_val : (0 - last_val));
}

/*
 * lib_rand_r
 */
__weak_reference(lib_rand_r, rand_r);

INT lib_rand_r (uint_t *puiSeed)
{
    INT64   llTmp = (INT64)((INT64)*puiSeed * 1103515245);
    
    llTmp  += ((INT64)FACTOR ^ *puiSeed);
    llTmp >>= 16;
    
    last_val = (INT)(llTmp % __ARCH_UINT_MAX);
    *puiSeed = (uint_t)last_val;
    
    return  ((last_val >= 0) ? last_val : (0 - last_val));
}

/*
 * lib_srand
 */
__weak_reference(lib_srand, srand);

VOID lib_srand (uint_t uiSeed)
{
    last_val = (INT)uiSeed;
}

/*
 * lib_srandom
 */
__weak_reference(lib_srandom, srandom);

VOID lib_srandom (uint_t uiSeed)
{
    last_val_dom = (LONG)uiSeed;
}

/*
 * lib_random
 */
__weak_reference(lib_random, random);

LONG lib_random (VOID)
{
    INT64   llTmp = (INT64)((INT64)last_val_dom * 1103515245);
    
    llTmp  += ((INT64)FACTOR ^ API_TimeGet());
    llTmp >>= 16;
    
    last_val_dom = (LONG)(llTmp % __ARCH_UINT_MAX);
    
    return  (LONG)((last_val_dom >= 0) ? last_val_dom : (0 - last_val_dom));
}

#define	RAND48_SEED_0	(0x330e)
#define	RAND48_SEED_1	(0xabcd)
#define	RAND48_SEED_2	(0x1234)
#define	RAND48_MULT_0	(0xe66d)
#define	RAND48_MULT_1	(0xdeec)
#define	RAND48_MULT_2	(0x0005)
#define	RAND48_ADD	    (0x000b)

static unsigned short __rand48_seed[3] = {
	RAND48_SEED_0,
	RAND48_SEED_1,
	RAND48_SEED_2
};
static unsigned short __rand48_mult[3] = {
	RAND48_MULT_0,
	RAND48_MULT_1,
	RAND48_MULT_2
};
static unsigned short __rand48_add = RAND48_ADD;

#define DBL_EXP_BIAS    (__ARCH_DOUBLE_EXP_NAN / 2)

/*
 * __dorand48
 */
static void __dorand48 (unsigned short xseed[3])
{
	unsigned long accu;
	unsigned short temp[2];

	if (!xseed) {
	    return;
	}

	accu = (unsigned long) __rand48_mult[0] * (unsigned long) xseed[0] +
	       (unsigned long) __rand48_add;
	temp[0] = (unsigned short) accu;	/* lower 16 bits */
	accu >>= sizeof(unsigned short) * 8;
	accu += (unsigned long) __rand48_mult[0] * (unsigned long) xseed[1] +
	        (unsigned long) __rand48_mult[1] * (unsigned long) xseed[0];
	temp[1] = (unsigned short) accu;	/* middle 16 bits */
	accu >>= sizeof(unsigned short) * 8;
	accu += __rand48_mult[0] * xseed[2] + __rand48_mult[1] * xseed[1] + __rand48_mult[2] * xseed[0];
	xseed[0] = temp[0];
	xseed[1] = temp[1];
	xseed[2] = (unsigned short) accu;
}

/*
 * lib_lcong48
 */
__weak_reference(lib_lcong48, lcong48);

void lib_lcong48 (unsigned short p[7])
{
	if (!p) {
	    return;
	}

	__rand48_seed[0] = p[0];
	__rand48_seed[1] = p[1];
	__rand48_seed[2] = p[2];
	__rand48_mult[0] = p[3];
	__rand48_mult[1] = p[4];
	__rand48_mult[2] = p[5];
	__rand48_add     = p[6];
}

/*
 * lib_erand48
 */
__weak_reference(lib_erand48, erand48);

double lib_erand48 (unsigned short xseed[3])
{
	__CPU_DOUBLE u;

	if (!xseed) {
	    return  (0.0);
	}

	__dorand48(xseed);
	
	u.dblfield.sig = 0;
	u.dblfield.exp = DBL_EXP_BIAS; /* so we get [1,2) */
	u.dblfield.frach = ((unsigned int)xseed[2] << 4)
				     | ((unsigned int)xseed[1] >> 12);
	u.dblfield.fracl = (((unsigned int)xseed[1] & 0x0fff) << 20)
				     | ((unsigned int)xseed[0] << 4);
	return  (u.dbl - 1);
}

/*
 * lib_drand48
 */
__weak_reference(lib_drand48, drand48);

double lib_drand48 (void)
{
	return  lib_erand48(__rand48_seed);
}

/*
 * lib_lrand48
 */
__weak_reference(lib_lrand48, lrand48);

long lib_lrand48 (void)
{
	__dorand48(__rand48_seed);
	return  (long)((unsigned long) __rand48_seed[2] << 15) +
	              ((unsigned long) __rand48_seed[1] >> 1);
}

/*
 * lib_mrand48
 */
__weak_reference(lib_mrand48, mrand48);

long lib_mrand48 (void)
{
	__dorand48(__rand48_seed);
	return  ((long) __rand48_seed[2] << 16) + (long) __rand48_seed[1];
}

/*
 * lib_nrand48
 */
__weak_reference(lib_nrand48, nrand48);

long lib_nrand48 (unsigned short xseed[3])
{
	if (!xseed) {
	    return  (0);
	}

	__dorand48(xseed);
	return  (long)((unsigned long) xseed[2] << 15) +
	              ((unsigned long) xseed[1] >> 1);
}

/*
 * lib_jrand48
 */
__weak_reference(lib_jrand48, jrand48);

long lib_jrand48 (unsigned short xseed[3])
{
    if (!xseed) {
        return  (0);
    }

	__dorand48(xseed);
	return  ((long) xseed[2] << 16) + (long) xseed[1];
}

/*
 * lib_seed48
 */
__weak_reference(lib_seed48, seed48);

unsigned short *lib_seed48 (unsigned short xseed[3])
{
	static unsigned short sseed[3];

	if (!xseed) {
	    return  (LW_NULL);
	}

	sseed[0] = __rand48_seed[0];
	sseed[1] = __rand48_seed[1];
	sseed[2] = __rand48_seed[2];
	__rand48_seed[0] = xseed[0];
	__rand48_seed[1] = xseed[1];
	__rand48_seed[2] = xseed[2];
	__rand48_mult[0] = RAND48_MULT_0;
	__rand48_mult[1] = RAND48_MULT_1;
	__rand48_mult[2] = RAND48_MULT_2;
	__rand48_add = RAND48_ADD;
	return  (sseed);
}

/*
 * lib_srand48
 */
__weak_reference(lib_srand48, srand48);

void lib_srand48 (long seed)
{
	__rand48_seed[0] = RAND48_SEED_0;
	__rand48_seed[1] = (unsigned short) seed;
	__rand48_seed[2] = (unsigned short) ((unsigned long)seed >> 16);
	__rand48_mult[0] = RAND48_MULT_0;
	__rand48_mult[1] = RAND48_MULT_1;
	__rand48_mult[2] = RAND48_MULT_2;
	__rand48_add = RAND48_ADD;
}

/*
 * end
 */
