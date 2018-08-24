/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifdef __DEFINE_INTRINSIC_ENUM

#define ENUM_ENTRY(e) SSAIntrinsic_##e,

enum IntrinsicType {
#include "genC/ssa/statement/SSAIntrinsicTypes.h"
};

#undef ENUM_ENTRY

#endif

#ifdef __DEFINE_INTRINSIC_STRINGS

#define ENUM_ENTRY(e) #e,

static const char *IntrinsicNames[] = {
#include "genC/ssa/statement/SSAIntrinsicTypes.h"
};

#undef ENUM_ENTRY

#endif
