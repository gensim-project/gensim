/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   blockcache-defines.h
 * Author: harry
 *
 * Created on 23 June 2016, 13:58
 */

#ifndef BLOCKCACHE_DEFINES_H
#define BLOCKCACHE_DEFINES_H

#define BLOCKCACHE_SIZE_BITS (10)
#define BLOCKCACHE_INSTRUCTION_SHIFT (1)

#define BLOCKCACHE_SIZE (1 << BLOCKCACHE_SIZE_BITS)
#define BLOCKCACHE_MASK (((1 << BLOCKCACHE_SIZE_BITS)-1) << BLOCKCACHE_INSTRUCTION_SHIFT)

#endif /* BLOCKCACHE_DEFINES_H */

