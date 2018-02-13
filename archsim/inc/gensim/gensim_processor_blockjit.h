/*
 * gensim_processor_blockjit.h
 *
 *  Created on: 24 Aug 2015
 *      Author: harry
 */

#ifndef INC_GENSIM_GENSIM_PROCESSOR_BLOCKJIT_H_
#define INC_GENSIM_GENSIM_PROCESSOR_BLOCKJIT_H_

#include "translate/profile/ProfileManager.h"
#include "gensim/gensim_processor.h"
#include "blockjit/BlockJitTranslate.h"
#include "blockjit/BlockCache.h"
#include "blockjit/BlockProfile.h"
#include "util/Cache.h"
#include "util/MemAllocator.h"

#define BLOCKJIT_USE_BAD_JIT

#ifndef BLOCKJIT_USE_BAD_JIT
#ifndef BLOCKJIT_USE_NEW_JIT
#error BLOCKJIT_USE_BAD_JIT or BLOCKJIT_USE_NEW_JIT must be defined!
#endif
#endif

namespace archsim
{
	namespace blockjit
	{
		class BlockTranslation;
	}
}

namespace gensim
{

	class BlockJitProcessor : public Processor
	{
	public:
		BlockJitProcessor(const std::string &arch_name, int _core_id, archsim::util::PubSubContext*);
		virtual ~BlockJitProcessor();

		virtual bool RunInterp(uint32_t steps) override;

#ifdef BLOCKJIT_USE_NEW_JIT
		virtual bool RunJIT(bool verbose, uint32_t steps) override;
#endif

		virtual bool Initialise(archsim::abi::EmulationModel& emulation_model, archsim::abi::memory::MemoryModel& memory_model) override;

		void InvalidatePage(archsim::PhysicalAddress pagebase);

		void FlushTxlnCache();
		void FlushTxlnsFeature();
		void FlushTxlns();
		void FlushAllTxlns();

		bool GetTranslatedBlock(archsim::PhysicalAddress, const archsim::ProcessorFeatureSet &features, archsim::blockjit::BlockTranslation &);

		gensim::DecodeTranslateContext *GetDecodeTranslateContext()
		{
			return decode_translate_context_;
		}

		virtual bool step_block_fast() override;
		virtual bool step_block_trace() override;

		uint32_t read_pc_fast()
		{
			return *_pc_ptr;
		}

	protected:
		virtual gensim::blockjit::BaseBlockJITTranslate *CreateBlockJITTranslate() const = 0;

		bool translate_block(archsim::VirtualAddress block_pc, bool support_chaining, bool support_profiling);

		captive::shared::block_txln_fn lookup_virt_block_txln(archsim::VirtualAddress address, bool side_effects);

		void InitPcPtr();
	private:
		uint32_t *_pc_ptr;
		bool _flush_txlns;
		bool _flush_all_txlns;
		wulib::SimpleZoneMemAllocator _block_allocator;

		gensim::DecodeTranslateContext *decode_translate_context_;
		gensim::DecodeContext *decode_context_;

		archsim::blockjit::BlockProfile _phys_block_profile;
		archsim::blockjit::BlockCache _virt_block_cache;
		gensim::blockjit::BaseBlockJITTranslate *_translator;
	};

}


#endif /* INC_GENSIM_GENSIM_PROCESSOR_BLOCKJIT_H_ */
