/* 
 * File:   LLVMRegionJITExecutionEngine.h
 * Author: harry
 *
 * Created on 24 May 2018, 13:43
 */

#ifndef LLVMREGIONJITEXECUTIONENGINE_H
#define LLVMREGIONJITEXECUTIONENGINE_H

#include "core/execution/ExecutionEngine.h"

namespace archsim {
	namespace core {
		namespace execution {
			class LLVMRegionJITExecutionEngine : public ExecutionEngine {
			public:
				LLVMRegionJITExecutionEngine(Interpreter *interp, BlockJitTranslate *translate);
				
				ExecutionResult Execute(ExecutionEngineThreadContext* thread) override;
				ExecutionEngineThreadContext* GetNewContext(thread::ThreadInstance* thread) override;

			private:
				Interpreter *interpreter_;
				BlockJitTranslate *translator_;
				
			};
		}
	}
}

#endif /* LLVMREGIONJITEXECUTIONENGINE_H */

