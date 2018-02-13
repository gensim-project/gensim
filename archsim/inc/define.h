/*                      Confidential Information
 *           Limited Distribution to Authorized Persons Only
 *         Copyright (C) 2003-2004 The University of Edinburgh
 *                        All Rights Reserved
 *
 * =====================================================================
 *
 * Description:
 *
 *  This file contains macro definitions which define default variables,
 *  structure sizes, bit-masks etc.
 *
 * ===================================================================*/

#ifndef _define_h_
#define _define_h_

// =====================================================================
// HEADERS
// =====================================================================
#include <stdint.h>
#include <stddef.h>
#include <assert.h>


// TODO: SWITCH THESE TO CMAKE VARIABLES
#define CONFIG_NO_MEMORY_EVENTS
#define INSTRUCTION_WORD_TYPE uint32_t


// Prompts for gcc's block placement algorithms
#define LIKELY(x) __builtin_expect((x), 1)
#define UNLIKELY(x) __builtin_expect((x), 0)

// Useful macros for bit manipulation
#define UNSIGNED_BITS(v, u, l) (((uint32_t)(v) << (31 - (u))) >> (31 - (u) + (l)))
#define UNSIGNED_BITS_64(v, u, l) (((uint64_t)(v) << (63 - (u))) >> (63 - (u) + (l)))
#define SIGNED_BITS(v, u, l) (((int32_t)(v) << (31 - (u))) >> (31 - (u) + (l)))
#define BITSEL(v, b) (((v) >> (b)) & 1UL)

/* Pre-processor tricks to enable common definitions of data-structres to be
 * re-usable in --fast mode during code-generation of JIT code.
 * For this we need variadic macros and indirect quoting of macro arguments:
 * @see: http://en.wikipedia.org/wiki/C_preprocessor#Indirectly_quoting_macro_arguments
 * @see: http://en.wikipedia.org/wiki/Variadic_macro
 */
#define _QUOTEME(...) #__VA_ARGS__
#define QUOTEME(...) _QUOTEME(__VA_ARGS__)

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
	TypeName(const TypeName&) = delete; \
	void operator=(const TypeName&) = delete


/**
 * Archsim copyright string
 */
#define ARCHSIM_COPYRIGHT QUOTEME(\n \
\t\tArchsim: The Edinburgh High Speed (EHS) Simulator\n \
\t\t\tUniversity of Edinburgh (c) 2017\n \
\t\t\t\tRevision : SCM_REV\n \
\t\t\t\tConfiguration: CONFIGSTRING\n\n)

/*
 * Type-safe memory addressing
 */
typedef void *host_addr_t;
typedef uint32_t addr_t;
typedef addr_t addr_off_t;
typedef addr_t virt_addr_t;
typedef addr_t phys_addr_t;

#endif /* _define_h_ */
