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
			class LLVMRegionJITExecutionEngine : public ExecutionEngine {
			public:
				LLVMRegionJITExecutionEngine(interpret::Interpreter *interp, gensim::blockjit::BaseBlockJITTranslate *translate);
				~LLVMRegionJITExecutionEngine();
				
				ExecutionResult Execute(ExecutionEngineThreadContext* thread) override;
				ExecutionEngineThreadContext* GetNewContext(thread::ThreadInstance* thread) override;

				static ExecutionEngine *Factory(const archsim::module::ModuleInfo *module, const std::string &cpu_prefix);
				
			private:
				interpret::Interpreter *interpreter_;
				gensim::blockjit::BaseBlockJITTranslate *translator_;
				
				translate::profile::RegionProfile regions_;
			};
		}
	}
}

#endif /* LLVMREGIONJITEXECUTIONENGINE_H */

