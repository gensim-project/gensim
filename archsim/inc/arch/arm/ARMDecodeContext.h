/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   ARMDecodeContext.h
 * Author: harry
 *
 * Created on 02 May 2016, 11:11
 */

#ifndef ARMDECODESEQUENCER_H
#define ARMDECODESEQUENCER_H

#include <unordered_map>

#include "gensim/gensim_decode_context.h"
#include "gensim/gensim_decode.h"
#include "blockjit/translation-context.h"
#include "abi/Address.h"


namespace gensim
{
	class BaseDecode;
	class DecodeContext;
	class Processor;
}

namespace archsim
{
	class ArchDescriptor;

	namespace arch
	{
		namespace arm
		{
			/*
			 *
			 * IT Blocks cause us a lot of issues in full system simulation. We want to
			 * avoid really tracking the ITSTATE bits because this would be very
			 * inefficient. We need to be especially careful with the following cases:
			 *  - IT block which spans a page boundary, where accessing the second page
			 *    causes an Instruction Abort
			 *  - IT block which spans two pages, and an interrupt occurs between
			 *    finishing the block on one page and starting the block on the next
			 *  - IT block which contains a load instruction, where the load instruction
			 *    causes a Data Abort and there are more predicated instructions after
			 *    the load.
			 *  - IT block which contains a software interrupt instruction and there
			 *    are more predicated instructions after the SWI
			 *
			 * The Decode Context class is designed to address this in a reasonably
			 * generic manner - the class internally keeps track of the ITSTATE field,
			 * and writes it back to the CPU after decoding each instruction. This
			 * ensures that page boundary issues are correctly handled.
			 *
			 * This
			 * could potentially be extended to handle (straightforward) branch delay
			 * effects by having the class track end-of-basic-block conditions. This
			 * could also potentially be extended to handle e.g. x86 instruction
			 * prefixes.
			 *
			 */

			class ARMDecodeContext : public gensim::DecodeContext
			{
			public:
				ARMDecodeContext(const archsim::ArchDescriptor &arch);
				virtual ~ARMDecodeContext();

				void Reset(archsim::core::thread::ThreadInstance* thread) override;
				void WriteBackState(archsim::core::thread::ThreadInstance* thread) override;

				/*
				 * DecodeSync is the main decode method exposed by the decode context.
				 * It should be called synchronously with the processor executing
				 * or translating instructions i.e., not for tracing.
				 */
				virtual uint32_t DecodeSync(archsim::MemoryInterface &interface, Address address, uint32_t mode, gensim::BaseDecode &target) override;

			private:
				uint8_t itstate_;
				const archsim::ArchDescriptor &arch_;
			};

		}
	}
}

#endif /* ARMDECODESEQUENCER_H */

