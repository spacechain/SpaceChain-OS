/**
 * @file
 * atomic library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#include "vxAtomicLib.h"

/*
 * atomic add
 */
atomicVal_t vxAtomicAdd (atomic_t *target, atomicVal_t value)
{
    return  (API_AtomicAdd(value, target));
}

/*
 * atomic and
 */
atomicVal_t vxAtomicAnd (atomic_t *target, atomicVal_t value)
{
    return  (API_AtomicAnd(value, target));
}

/*
 * atomic dec
 */
atomicVal_t vxAtomicDec (atomic_t *target)
{
    return  (API_AtomicDec(target));
}

/*
 * atomic inc
 */
atomicVal_t vxAtomicInc (atomic_t *target)
{
    return  (API_AtomicInc(target));
}

/*
 * atomic nand
 */
atomicVal_t vxAtomicNand (atomic_t *target, atomicVal_t value)
{
    return  (API_AtomicNand(value, target));
}

/*
 * atomic or
 */
atomicVal_t vxAtomicOr (atomic_t *target, atomicVal_t value)
{
    return  (API_AtomicOr(value, target));
}

/*
 * atomic sub
 */
atomicVal_t vxAtomicSub (atomic_t *target, atomicVal_t value)
{
    return  (API_AtomicSub(value, target));
}

/*
 * atomic xor
 */
atomicVal_t vxAtomicXor (atomic_t *target, atomicVal_t value)
{
    return  (API_AtomicXor(value, target));
}

/*
 * atomic clear
 */
atomicVal_t vxAtomicClear (atomic_t *target)
{
    API_AtomicSet(0, target);
    return  (0);
}

/*
 * atomic get
 */
atomicVal_t vxAtomicGet(atomic_t *target)
{
    return  (API_AtomicGet(target));
}

atomicVal_t vxAtomicSet(atomic_t *target, atomicVal_t value)
{
    API_AtomicSet(value, target);
    return  (0);
}

/*
 * end
 */

