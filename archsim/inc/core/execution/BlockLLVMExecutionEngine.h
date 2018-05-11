/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BlockLLVMExecutionEngine.h
 * Author: harry
 *
 * Created on 11 May 2018, 16:32
 */

#ifndef BLOCKLLVMEXECUTIONENGINE_H
#define BLOCKLLVMEXECUTIONENGINE_H

#include "core/execution/ExecutionEngine.h"

namespace archsim {
	namespace core {
		namespace execution {
			class BlockLLVMExecutionEngine : public ExecutionEngine {
			public:
				virtual ~BlockLLVMExecutionEngine()
				{

				}
				
			private:
				ExecutionResult Execute(ExecutionEngineThreadContext* thread) override;
				ExecutionEngineThreadContext* GetNewContext(thread::ThreadInstance* thread) override;
				
			};
		}
	}
}

#endif /* BLOCKLLVMEXECUTIONENGINE_H */

