/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/* =====================================================================
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
#include <cstdint>
#include <stddef.h>
#include <assert.h>

#include <stdexcept>


// TODO: SWITCH THESE TO CMAKE VARIABLES
#define CONFIG_NO_MEMORY_EVENTS
#define INSTRUCTION_WORD_TYPE uint32_t

#define UNIMPLEMENTED do { throw std::logic_error("Not implemented " __FILE__ ":" + std::to_string(__LINE__)); } while(0)
#define ASSERT(x) do { if(!(x)) { throw std::logic_error("Assertion failed: " #x ", " __FILE__ ":" + std::to_string(__LINE__)); } } while(0)
#define UNEXPECTED do {throw std::logic_error("Unexpected: " __FILE__", " + std::to_string(__LINE__)); } while(0)

// Prompts for gcc's block placement algorithms
#define LIKELY(x) __builtin_expect((x), 1)
#define UNLIKELY(x) __builtin_expect((x), 0)

// Useful macros for bit manipulation
#define UNSIGNED_BITS(v, u, l) (((uint32_t)(v) << (31 - (u))) >> (31 - (u) + (l)))
#define UNSIGNED_BITS_64(v, u, l) (((uint64_t)(v) << (63 - (u))) >> (63 - (u) + (l)))
#define SIGNED_BITS(v, u, l) (((int32_t)(v) << (31 - (u))) >> (31 - (u) + (l)))
#define BITSEL(v, b) (((v) >> (b)) & 1UL)
#define BIT_LSB(i) (1 << (i))
#define BIT_32_MSB(i) (0x80000000 >> (i))

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

using host_addr_t = void*;
using uint128_t = __uint128_t;
using int128_t = __int128_t;

#include "cmake-config.h"

#endif /* _define_h_ */
