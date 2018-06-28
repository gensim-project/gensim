/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   define.h
 * Author: s0803652
 *
 * Created on 28 September 2011, 14:13
 */

#ifndef _DEFINE_H
#define _DEFINE_H

#include <stdexcept>
#include "util/Exception.h"

#ifdef UNSIGNED_BITS
#undef UNSIGNED_BITS
#endif

#ifdef SIGNED_BITS
#undef SIGNED_BITS
#endif

#ifdef BITSEL
#undef BITSEL
#endif

/* Always have these. */
#define UNSIGNED_BITS(v, u, l) (((uint32_t)(v) << (31 - (u)) >> (31 - (u) + (l))))
#define SIGNED_BITS(v, u, l) (((int32_t)(v) << (31 - (u)) >> (31 - (u) + (l))))
#define BITSEL(v, b) (((v) >> b) & 1UL)

// UNIMPLEMENTED: this behaviour is expected to work, but has not been implemented yet
#define UNIMPLEMENTED throw gensim::NotImplementedException(__FILE__, __LINE__)

// UNEXPECTED: an unexpected condition has occurred
#define UNEXPECTED throw gensim::NotExpectedException(__FILE__, __LINE__)

// UNREACHABLE: a provably unobtainable situation has occurred
#define UNREACHABLE __builtin_unreachable()

#define STRINGIFY(x) #x

#ifdef NDEBUG
#define GASSERT(x)
#else
#define GASSERT(x) do { if(!(x)) throw gensim::AssertionFailedException(__FILE__, __LINE__, STRINGIFY(x)); } while(0)
#endif

#endif /* _DEFINE_H */
