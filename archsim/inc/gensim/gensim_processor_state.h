/*
 * gensim_processor_state.h
 *
 *  Created on: 7 Jan 2014
 *      Author: harry
 */

#ifndef GENSIM_PROCESSOR_STATE_H_
#define GENSIM_PROCESSOR_STATE_H_

/* ***
 * WARNING: This file MUST be safe for compilation in both a C and a C++ context
 */

#include "define.h"

/**
 * Some constants for dealing with asynchronous actions on the processor. These are written into state.pending_actions and should be flags
 */

#define PENDINGACTION_INTERRUPT 0x00000001
#define PENDINGACTION_SIGNAL    0x00000002
#define PENDINGACTION_ABORT     0x00000004

typedef uint64_t blockjit_spill_area_t[16];

#ifndef __cplusplus

//Emit some Igorified typedefs if we're building the precomp module because we haven't got them from anywhere else
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t sint8;
typedef int16_t sint16;
typedef int32_t sint32;
typedef int64_t sint64;

typedef void *cpuContext;

#endif

#ifdef __cplusplus
extern "C" {
#endif

struct cpuState;
typedef uint32_t (*jit_region_fn)(struct cpuState*);
typedef jit_region_fn* jit_region_fn_table;
typedef jit_region_fn_table jit_region_fn_table_ptr;

struct mmap_ret_val {
	uint32_t retcode;
	void *addr;
};

typedef struct mmap_ret_val (*mmap_fn_t)(struct cpuState*, uint32_t addr);
typedef mmap_fn_t *mmap_table_t;

struct smm_cache_entry {
	uint32_t virt_tag;
	uint32_t flags;
	void *ptr;
};

#define STATE_ENTRY(name, type) type name;

struct cpuState {
#include "gensim_processor_state_fields.h"
};

#ifdef __cplusplus
}
#endif

#undef STATE_ENTRY

#ifdef __cplusplus

#define STATE_ENTRY(name, type) CpuState_##name,
namespace gensim
{
	namespace CpuStateEntries
	{
		enum CpuStateEntry {
#include "gensim_processor_state_fields.h"
		};
	}
}

#undef STATE_ENTRY

#define STATE_ENTRY(name, type) CpuState_##name = offsetof(cpuState, name),
namespace gensim
{
	namespace CpuStateOffsets
	{
		enum CpuStateEntry {
#include "gensim_processor_state_fields.h"
		};
	}
}

#undef STATE_ENTRY

#endif

#endif /* GENSIM_PROCESSOR_STATE_H_ */
