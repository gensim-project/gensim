/*
 * gensim_processor_state.h
 *
 *  Created on: 7 Jan 2014
 *      Author: harry
 */

#ifndef STATE_ENTRY
#error "gensim_processor_state_fields.h must be included in a context where STATE_ENTRY is defined!"
#endif

// Opaque pointer back to processor object.
STATE_ENTRY(cpu_ctx, void *)

// Pointer to arch-specific state structure.
STATE_ENTRY(gensim_state, struct gensim_state *)

// Current mode index of the operating ISA.
STATE_ENTRY(isa_mode, uint8_t)

// Number of iterations remaining before something happens.
STATE_ENTRY(iterations, uint32_t)

STATE_ENTRY(chained_target, uint64_t)
STATE_ENTRY(chained_fallthrough, uint64_t)
STATE_ENTRY(chained_failed, uint64_t)
STATE_ENTRY(chained_cantchain, uint64_t)

// Bit-mask of actions that are pending.
STATE_ENTRY(pending_actions, uint32_t)

// Pointer to the table of region entry points for inter-region chaining.
STATE_ENTRY(jit_entry_table, jit_region_fn_table_ptr)

// Value indicating whether we are in a privileged mode
STATE_ENTRY(in_kern_mode, uint8_t)

// Last return value from JIT
STATE_ENTRY(jit_status, uint32_t)

// System memory model cache pointers
STATE_ENTRY(smm_read_cache, void*)
STATE_ENTRY(smm_write_cache, void*)

STATE_ENTRY(smm_read_user_cache, void*)
STATE_ENTRY(smm_write_user_cache, void*)

STATE_ENTRY(mem_generation, uint32_t)

STATE_ENTRY(blockjit_spill_area, blockjit_spill_area_t)
