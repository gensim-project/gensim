/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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

