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
#include "translate/profile/RegionProfile.h"
#include "module/Module.h"

namespace archsim {
	namespace core {
		namespace execution {
			class LLVMRegionJITExecutionEngineContext : public ExecutionEngineThreadContext {
			public:
				LLVMRegionJITExecutionEngineContext(archsim::core::execution::ExecutionEngine *engine, archsim::core::thread::ThreadInstance *thread);
				
				translate::AsynchronousTranslationManager TxlnMgr;
			};
			
			class LLVMRegionJITExecutionEngine : public ExecutionEngine {
			public:
				friend class LLVMRegionJITExecutionEngineContext;
				
				LLVMRegionJITExecutionEngine(interpret::Interpreter *interp, gensim::blockjit::BaseBlockJITTranslate *translate);
				~LLVMRegionJITExecutionEngine();
				
				ExecutionResult Execute(ExecutionEngineThreadContext* thread) override;
				ExecutionEngineThreadContext* GetNewContext(thread::ThreadInstance* thread) override;

				ExecutionResult EpochInterpret();
				ExecutionResult EpochNative();
				
				static ExecutionEngine *Factory(const archsim::module::ModuleInfo *module, const std::string &cpu_prefix);
				
			private:
				interpret::Interpreter *interpreter_;
				gensim::blockjit::BaseBlockJITTranslate *translator_;
			};
		}
	}
}

#endif /* LLVMREGIONJITEXECUTIONENGINE_H */

