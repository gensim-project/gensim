/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


/*
 * File:   BlockJITExecutionEngine.h
 * Author: harry
 *
 * Created on 25 April 2018, 15:17
 */

#ifndef BLOCKJITEXECUTIONENGINE_H
#define BLOCKJITEXECUTIONENGINE_H

#include "core/execution/ExecutionEngine.h"

#include "util/MemAllocator.h"
#include "abi/Address.h"
#include "gensim/gensim_decode_context.h"
#include "blockjit/BlockJitTranslate.h"
#include "blockjit/BlockProfile.h"
#include "blockjit/BlockCache.h"
#include "module/ModuleManager.h"

namespace archsim
{
	namespace core
	{
		namespace execution
		{
			class BlockJITExecutionEngine : public ExecutionEngine
			{
			public:

				BlockJITExecutionEngine(gensim::blockjit::BaseBlockJITTranslate *translator);

				void FlushTxlns();
				void FlushTxlnsFeature();
				void FlushTxlnCache();
				void FlushAllTxlns();
				void InvalidateRegion(Address addr);

				gensim::blockjit::BaseBlockJITTranslate *GetTranslator()
				{
					return translator_;
				}

				ExecutionEngineThreadContext* GetNewContext(thread::ThreadInstance* thread) override;
				ExecutionResult Execute(ExecutionEngineThreadContext* thread) override;

				template<typename PC_t> ExecutionResult ExecuteLoop(ExecutionEngineThreadContext *ctx, PC_t* pc_ptr);
				template<typename PC_t> void ExecuteInnerLoop(ExecutionEngineThreadContext *ctx, PC_t* pc_ptr);

				void checkFlushTxlns();
				virtual bool translateBlock(thread::ThreadInstance *thread, archsim::Address block_pc, bool support_chaining, bool support_profiling);

				wulib::SimpleZoneMemAllocator mem_allocator_;

				gensim::DecodeContext *decode_context_;
				gensim::DecodeTranslateContext *decode_translation_context_;

				archsim::blockjit::BlockProfile phys_block_profile_;
				archsim::blockjit::BlockCache virt_block_cache_;
				gensim::blockjit::BaseBlockJITTranslate *translator_;

				bool flush_txlns_;
				bool flush_all_txlns_;
				bool subscribed_;

				static ExecutionEngine *Factory(const archsim::module::ModuleInfo *module, const std::string &cpu_prefix);
			};
		}
	}
}

#endif /* BLOCKJITEXECUTIONENGINE_H */

