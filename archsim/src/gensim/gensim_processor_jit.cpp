/*
 * gensim/gensim_processor_jit.cpp
 */
#include "system.h"

#include "gensim/gensim_processor.h"
#include "abi/devices/MMU.h"
#include "ij/IJManager.h"
#include "translate/TranslationManager.h"
#include "translate/profile/Region.h"
#include "translate/profile/Block.h"
#include "translate/Translation.h"

#include "util/LogContext.h"

#include <libtrace/TraceSource.h>

UseLogContext(LogTranslate);

using namespace gensim;

using namespace archsim::translate;
using namespace archsim::translate::profile;

bool Processor::RunJIT(bool verbose, uint32_t iterations)
{
	TranslationManager& tmgr = GetEmulationModel().GetSystem().GetTranslationManager();

	uint32_t current_profiling_interval = 0;
	bool stepOK = true;


	// Record simulation start time, if in verbose mode.
	if (verbose) exec_time.Start();

	CreateSafepoint();
	if(GetTraceManager() != NULL && GetTraceManager()->IsPacketOpen()) GetTraceManager()->Trace_End_Insn();

	// Initialise iteration count (block iterations are decremented in native mode)
	state.iterations = iterations;

	// Initialise our current execution mode
	cur_exec_mode = kExecModeInterpretive;

	tmgr.RunGC();

	archsim::abi::devices::MMU *mmu = (archsim::abi::devices::MMU *)peripherals.GetDeviceByName("mmu");
	while (stepOK && !halted && state.iterations) {
		virt_addr_t virt_pc = (virt_addr_t)read_pc();

		if (GetEmulationModel().GetSystem().HasBreakpoints()) {
			if (GetEmulationModel().GetSystem().ShouldBreak(virt_pc)) {
				if (!HasTraceManager()) {
					InitialiseTracing();
					StartTracing();
				}

				fprintf(stderr, "*** HIT USER BREAKPOINT AT: %08x\n", virt_pc);
				GetEmulationModel().GetSystem().SetSimulationMode(System::SingleStep);
				return true;
			}
		}

		// Initialise the physical pc with a canary.
		phys_addr_t phys_pc = 0xf0f0f0f0;

		// If we've got an MMU, do a translation.
		if (mmu) {
			uint32_t tx_fault = mmu->Translate(this, virt_pc, phys_pc, MMUACCESSINFO(in_kernel_mode(), 0, 1));

			// If we faulted, execute in the interpreter, otherwise, we now have a physical pc.
			if (tx_fault) {
				assert(state.jit_status != TXLN_RESULT_MUST_INTERPRET);
				stepOK = JitInterpBlock(virt_pc, verbose);
				continue;
			}
		} else {
			// If there is no MMU, assume a one-to-one mapping.
			phys_pc = virt_pc;
		}

		tmgr.RegisterCompletedTranslations();
		Region *rgn_ptr;
		if(tmgr.RegionIsDirty(phys_pc)) {
			stepOK = JitInterpBlock(virt_pc, verbose);

			if (stepOK && (++current_profiling_interval == archsim::options::JitProfilingInterval.GetValue())) {
				// Reset the profiling interval.
				current_profiling_interval = 0;

				// Run a profiling cycle, tracing the time it takes if verbose is enabled.
				//if (verbose) trace_time.Start();
				tmgr.Profile(*this);
				//if (verbose) trace_time.Stop();
			}
		} else {
			// Retrieve the region descriptor, containing the block we are about to execute.
			Region& rgn = tmgr.GetRegion(phys_pc);

			assert(rgn.IsValid());

			// Attempt to retrieve a translation for this block.
			if (state.jit_status != TXLN_RESULT_MUST_INTERPRET && rgn.txln && rgn.txln->ContainsBlock(RegionArch::PageOffsetOf(virt_pc))) {

				rgn.virtual_images.insert(profile::RegionArch::PageBaseOf(virt_pc));

				if (!archsim::options::JitDisableBranchOpt)
					rgn.txln->Install((jit_region_fn_table)tmgr.GetRegionTxlnCacheEntry(phys_pc));

				// The region provided us with a translation for this block, so execute it.
				cur_exec_mode = kExecModeNative;
				//if (verbose) native_time.Start();
				state.jit_status = rgn.txln->Execute(*this);
				//if (verbose) native_time.Stop();
				cur_exec_mode = kExecModeInterpretive;

				if(state.jit_status == TXLN_RESULT_MUST_INTERPRET) _skip_verify = true;

				// Reset the tracing infrastructure.
				tmgr.ResetTrace();

				continue;
			}

			if (state.jit_status != TXLN_RESULT_MUST_INTERPRET) {
				// If we haven't got a translation, trace this block.
				rgn.TraceBlock(*this, virt_pc);
			}

			// Now execute it in the interpreter.
			stepOK = JitInterpBlock(virt_pc, verbose);

			// If the region is still valid (it may have been invalidated by the last interpretive execution), then...
			if (rgn.IsValid()) {
				// Increment profiling counters, and if we hit the threshold, perform a profiling cycle.
				// Also, don't trace if we've just come out of the jit for a special reason.
				if (stepOK && state.jit_status != TXLN_RESULT_MUST_INTERPRET && (++current_profiling_interval == archsim::options::JitProfilingInterval.GetValue())) {
					// Reset the profiling interval.
					current_profiling_interval = 0;

					// Run a profiling cycle, tracing the time it takes if verbose is enabled.
					//if (verbose) trace_time.Start();
					tmgr.Profile(*this);
					//if (verbose) trace_time.Stop();
				}
			}
		}

		// Decrement the iterations counter.
		--state.iterations;
		state.jit_status = 0;
	}

	// Record and update simulation end time, if in verbose mode.
	if (verbose) exec_time.Stop();

	return stepOK;
}

bool Processor::JitInterpBlock(virt_addr_t pc, bool verbose)
{
	bool ret;

	//if (verbose) interp_time.Start();

	if (archsim::options::JitUseIJ) {
		return false;
	} else {
		if (UNLIKELY(IsTracingEnabled())) {
			ret = step_block_trace();
		} else {
			ret = step_block_fast();
		}
	}

	//if (verbose) interp_time.Stop();

	return ret;
}
