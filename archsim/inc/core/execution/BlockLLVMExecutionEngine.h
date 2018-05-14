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
#include "BlockJITExecutionEngine.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

namespace archsim {
	namespace core {
		namespace execution {
			class BlockLLVMExecutionEngine : public BlockJITExecutionEngine {
			public:
                            
				BlockLLVMExecutionEngine(gensim::blockjit::BaseBlockJITTranslate *translator);
                            
				virtual ~BlockLLVMExecutionEngine()
				{

				}
				
			private:
				bool translateBlock(thread::ThreadInstance* thread, archsim::Address block_pc, bool support_chaining, bool support_profiling) override;	
				
				llvm::LLVMContext llvm_ctx_;
				llvm::ExecutionEngine *engine_;
			};
		}
	}
}

#endif /* BLOCKLLVMEXECUTIONENGINE_H */

