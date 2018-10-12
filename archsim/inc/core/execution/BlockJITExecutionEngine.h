/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


/*
 * File:   BlockJITExecutionEngine.h
 * Author: harry
 *
 * Created on 25 April 2018, 15:17
 */

#ifndef BLOCKJITEXECUTIONENGINE_H
#define BLOCKJITEXECUTIONENGINE_H

#include "core/execution/BasicJITExecutionEngine.h"

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
			class BlockJITExecutionEngine : public BasicJITExecutionEngine
			{
			public:

				BlockJITExecutionEngine(gensim::blockjit::BaseBlockJITTranslate *translator);


				gensim::blockjit::BaseBlockJITTranslate *GetTranslator()
				{
					return translator_;
				}

				ExecutionEngineThreadContext* GetNewContext(thread::ThreadInstance* thread) override;

				virtual bool translateBlock(thread::ThreadInstance *thread, archsim::Address block_pc, bool support_chaining, bool support_profiling);


				gensim::DecodeContext *decode_context_;
				gensim::DecodeTranslateContext *decode_translation_context_;


				gensim::blockjit::BaseBlockJITTranslate *translator_;

				static ExecutionEngine *Factory(const archsim::module::ModuleInfo *module, const std::string &cpu_prefix);
			};
		}
	}
}

#endif /* BLOCKJITEXECUTIONENGINE_H */

