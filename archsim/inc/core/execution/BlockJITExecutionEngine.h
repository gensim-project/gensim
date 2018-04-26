/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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

namespace archsim {
	namespace core {
		namespace execution {
			class BlockJITExecutionEngine : public BasicExecutionEngine
			{
			public:
				
				BlockJITExecutionEngine(gensim::blockjit::BaseBlockJITTranslate *translator);
				
				ExecutionResult ArchStepBlock(thread::ThreadInstance* thread) override;
				ExecutionResult ArchStepSingle(thread::ThreadInstance* thread) override;

			private:
				void checkFlushTxlns();
				bool translateBlock(thread::ThreadInstance *thread, archsim::Address block_pc, bool support_chaining, bool support_profiling);
				
				wulib::SimpleZoneMemAllocator mem_allocator_;
				
				gensim::DecodeContext *decode_context_;
				gensim::DecodeTranslateContext *decode_translation_context_;
				
				archsim::blockjit::BlockProfile phys_block_profile_;
				archsim::blockjit::BlockCache virt_block_cache_;
				gensim::blockjit::BaseBlockJITTranslate *translator_;
			};
		}
	}
}

#endif /* BLOCKJITEXECUTIONENGINE_H */

