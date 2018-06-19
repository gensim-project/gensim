/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

// =====================================================================
//
// Description:
//
// Processor micro-architecture model is implemented in classes derived
// from ProcessorPipeline.
//
// =====================================================================

#ifndef __GENSIM_PIPELINE__
#define __GENSIM_PIPELINE__

#include <string>
#include "api/types.h"
#include "define.h"

// =====================================================================
// MACROS
// =====================================================================

// Pipeline invariant macro
//
#define CHECK_PIPELINE_INVARIANT(_CPU, _STAGE_CUR, _STAGE_NEXT)         \
	if (_CPU.state.pl[_STAGE_CUR] < _CPU.state.pl[_STAGE_NEXT])     \
	{                                                               \
		_CPU.state.pl[_STAGE_CUR] = _CPU.state.pl[_STAGE_NEXT]; \
	}

// --------------------------------------------------------------------------
// FORWARD DECLARATIONS
//

class SimOptions;
class IsaOptions;

namespace gensim
{
	class BaseDecode;
	class Processor;

// Interface class for processor pipeline
//
	class ProcessorPipelineInterface
	{
	protected:
		// Constructor MUST be protected and empty!
		//
		ProcessorPipelineInterface()
		{
			/* EMPTY */
		}

	public:
		/**
		 * Destructor of interface must be declared AND virtual so all implementations
		 * of this interface can be destroyed correctly.
		 */
		virtual ~ProcessorPipelineInterface()
		{
			/* EMPTY */
		}

		// Abstract method.
		// This method is called by the decoder to allow instruction timing
		// behaviour to be pre-computed and cached with the decoded instruction
		// for later use by the performance model.
		//
		virtual bool precompute_pipeline_model(BaseDecode& inst, const IsaOptions& isa_opts) = 0;

		// Abstract method.
		// This method is called in interpretive mode to update the microarchitectural
		// performance model.
		//
		virtual bool update_pipeline(gensim::Processor& _cpu, uint32 insn_pc, uint32 next_pc) = 0;

		/*
		// ---------------------------------------------------------------------------

		// Abstract methods called by the JIT at the beginning and end of a translation unit
		//
		virtual std::string jit_emit_translation_unit_begin(archsim::sys::cpu::ProcessorCounterContext& cnt_ctx,
								    const SimOptions&              opts,
								    const IsaOptions&              isa_opts) = 0;
		virtual std::string jit_emit_translation_unit_end  (archsim::sys::cpu::ProcessorCounterContext& cnt_ctx,
								    const SimOptions&              opts,
								    const IsaOptions&              isa_opts) = 0;

		// Abstract methods called by the JIT at the beginning, and at the end, of a block
		//
		virtual std::string jit_emit_block_begin(archsim::sys::cpu::ProcessorCounterContext& cnt_ctx,
							 const SimOptions&              opts,
							 const IsaOptions&              isa_opts) = 0;
		virtual std::string jit_emit_block_end  (archsim::sys::cpu::ProcessorCounterContext& cnt_ctx,
							 const SimOptions&              opts,
							 const IsaOptions&              isa_opts) = 0;


		// Abstract methods called by the JIT at the beginning, and at the end, of each instruction
		//
		virtual std::string jit_emit_instr_begin(const BaseDecode& inst,
							 uint32 pc,
							 archsim::sys::cpu::ProcessorCounterContext& cnt_ctx,
							 const SimOptions&              opts) = 0;
		virtual std::string jit_emit_instr_end  (const BaseDecode& inst,
							 uint32 pc,
							 archsim::sys::cpu::ProcessorCounterContext& cnt_ctx,
							 const SimOptions&              opts) = 0;


		// Abstract methods called by the JIT in case of taken or not-taken branch
		//
		virtual std::string jit_emit_instr_branch_taken     (const BaseDecode& inst,
								     uint32 pc) = 0;
		virtual std::string jit_emit_instr_branch_not_taken (const BaseDecode& inst,
								     uint32 pc) = 0;

		// Abstract method called by the JIT to update the pipeline timing model,
		// after the sub-methods for instruction begin, instruciton fetch and memory access
		// have been called.
		//
		virtual std::string jit_emit_instr_pipeline_update (const BaseDecode& inst,
								    const char *src1,
								    const char *src2,
								    const char *dsc1,
								    const char *dsc2
								   ) = 0;
		*/
	};
}
// -----------------------------------------------------------------------------

#endif  // __GENSIM_PIPELINE__
