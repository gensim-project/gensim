/*
 * gensim/gensim_processor_ij.cpp
 */
#include "gensim/gensim_processor.h"
#include "abi/devices/MMU.h"
#include "ij/IJManager.h"
#include "system.h"

using namespace gensim;
using namespace archsim::ij;

bool Processor::RunIJ(bool verbose, uint32_t iterations)
{
	IJManager& ij = GetEmulationModel().GetSystem().GetIJManager();
	bool stepOK = true;

	state.iterations = iterations;
	if (verbose) exec_time.Start();

	archsim::abi::devices::MMU *mmu = (archsim::abi::devices::MMU *)peripherals.GetDeviceByName("mmu");

	while (stepOK && !halted && state.iterations--) {
		virt_addr_t virt_pc = read_pc();

		// Initialise the physical pc with a canary.
		phys_addr_t phys_pc = 0xf0f0f0f0;

		// If we've got an MMU, do a translation.
		if (mmu) {
			uint32_t tx_fault = mmu->Translate(this, virt_pc, phys_pc, MMUACCESSINFO(in_kernel_mode(), 0), 0);

			// If we faulted, execute in the interpreter, otherwise, we now have a physical pc.
			if (tx_fault) {
				assert(false);
			}
		} else {
			// If there is no MMU, assume a one-to-one mapping.
			phys_pc = virt_pc;
		}

		IJManager::ij_block_fn fn = ij.TranslateBlock(*this, phys_pc);
		if (!fn) {
			stepOK = false;
		} else {
			stepOK = fn(&state) == 0;
		}
	}

	if (verbose) exec_time.Stop();
	return stepOK;
}
