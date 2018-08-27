/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   BasicJITExecutionEngine.h
 * Author: harry
 *
 * Created on 24 August 2018, 15:06
 */

#ifndef BASICJITEXECUTIONENGINE_H
#define BASICJITEXECUTIONENGINE_H

#include "ExecutionEngine.h"
#include "blockjit/BlockCache.h"
#include "blockjit/BlockProfile.h"

namespace archsim
{
	namespace core
	{
		namespace execution
		{

			class BasicJITExecutionEngine : public ExecutionEngine
			{
			public:
				BasicJITExecutionEngine();

				ExecutionResult Execute(ExecutionEngineThreadContext* thread) override;

				virtual ExecutionEngineThreadContext* GetNewContext(thread::ThreadInstance* thread) = 0;

				void FlushTxlns();
				void FlushTxlnsFeature();
				void FlushTxlnCache();
				void FlushAllTxlns();
				void InvalidateRegion(Address addr);

			protected:
				virtual bool translateBlock(thread::ThreadInstance *thread, archsim::Address block_pc, bool support_chaining, bool support_profiling) = 0;
				virtual bool lookupBlock(thread::ThreadInstance *thread, Address addr, captive::shared::block_txln_fn &);

				void checkFlushTxlns();
				void checkCodeSize();
				void registerTranslation(Address phys_addr, Address virt_addr, archsim::blockjit::BlockTranslation &txln);

				wulib::MemAllocator &GetMemAllocator()
				{
					return mem_allocator_;
				}

			private:
				template<typename PC_t> ExecutionResult ExecuteLoop(ExecutionEngineThreadContext *ctx, PC_t* pc_ptr);
				template<typename PC_t> void ExecuteInnerLoop(ExecutionEngineThreadContext *ctx, PC_t* pc_ptr);

				archsim::blockjit::BlockProfile phys_block_profile_;
				archsim::blockjit::BlockCache virt_block_cache_;
				wulib::SimpleZoneMemAllocator mem_allocator_;

				bool flush_txlns_;
				bool flush_all_txlns_;
				bool subscribed_;


			};

		}
	}
}

#endif /* BASICJITEXECUTIONENGINE_H */

