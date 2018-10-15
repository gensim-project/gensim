/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   LLVMRegionJITExecutionEngine.h
 * Author: harry
 *
 * Created on 24 May 2018, 13:43
 */

#ifndef LLVMREGIONJITEXECUTIONENGINE_H
#define LLVMREGIONJITEXECUTIONENGINE_H

#include "core/execution/ExecutionEngine.h"
#include "interpret/Interpreter.h"
#include "blockjit/BlockJitTranslate.h"
#include "translate/AsynchronousTranslationManager.h"
#include "translate/SynchronousTranslationManager.h"
#include "translate/profile/RegionProfile.h"
#include "module/Module.h"
#include "gensim/gensim_translate.h"

namespace archsim
{
	namespace core
	{
		namespace execution
		{
			class LLVMRegionJITCache
			{
			public:
				struct RegionJITCacheEntry {
					Address PageBase;
					captive::shared::block_txln_fn Translation;
				};

				static const uint32_t kCacheSize = 1024;
				RegionJITCacheEntry Cache[kCacheSize];

				LLVMRegionJITCache()
				{
					Invalidate();
				}

				void Invalidate()
				{
					for(auto &i : Cache) {
						i.PageBase = Address(1);
						i.Translation = nullptr;
					}
				}

				RegionJITCacheEntry &GetEntry(Address addr)
				{
					return Cache[addr.GetPageIndex() % kCacheSize];
				}

				captive::shared::block_txln_fn GetTranslation(Address addr)
				{
					auto &entry = GetEntry(addr);
					if(entry.PageBase == addr.PageBase()) {
						return entry.Translation;
					}
					return nullptr;
				}

				void InsertEntry(Address addr, captive::shared::block_txln_fn txln)
				{
					auto &entry = GetEntry(addr);
					entry.PageBase = addr.PageBase();
					entry.Translation = txln;
				}
			};

			class LLVMRegionJITExecutionEngineContext : public InterpreterExecutionEngineThreadContext
			{
			public:
				LLVMRegionJITExecutionEngineContext(archsim::core::execution::ExecutionEngine *engine, archsim::core::thread::ThreadInstance *thread);

				translate::AsynchronousTranslationManager TxlnMgr;
				LLVMRegionJITCache PageCache;

				Address PrevRegionBase;
				Region *CurrentRegion;

				int Iterations;
			};

			class LLVMRegionJITExecutionEngine : public ExecutionEngine
			{
			public:
				friend class LLVMRegionJITExecutionEngineContext;

				LLVMRegionJITExecutionEngine(interpret::Interpreter *interp, gensim::BaseLLVMTranslate *translate);
				~LLVMRegionJITExecutionEngine();

				ExecutionResult Execute(ExecutionEngineThreadContext* thread) override;
				ExecutionEngineThreadContext* GetNewContext(thread::ThreadInstance* thread) override;

				ExecutionResult EpochInterpret(LLVMRegionJITExecutionEngineContext *ctx, archsim::translate::profile::Region &region);
				ExecutionResult EpochNative();

				static ExecutionEngine *Factory(const archsim::module::ModuleInfo *module, const std::string &cpu_prefix);

			private:
				interpret::Interpreter *interpreter_;
				gensim::BaseLLVMTranslate *translator_;
			};
		}
	}
}

#endif /* LLVMREGIONJITEXECUTIONENGINE_H */

