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
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/LambdaResolver.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>

namespace archsim {
	namespace core {
		namespace execution {
			class BlockJITLLVMMemoryManager : public llvm::RTDyldMemoryManager {
			public:
				BlockJITLLVMMemoryManager(wulib::MemAllocator &allocator);
				
				uint8_t* allocateCodeSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, llvm::StringRef SectionName) override;
				uint8_t* allocateDataSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, llvm::StringRef SectionName, bool IsReadOnly) override;
				
				bool finalizeMemory(std::string* ErrMsg) override;
			private:
				wulib::MemAllocator &allocator_;
			};
			
			class BlockLLVMExecutionEngine : public BlockJITExecutionEngine {
			public:
                            
				BlockLLVMExecutionEngine(gensim::blockjit::BaseBlockJITTranslate *translator);
                            
				virtual ~BlockLLVMExecutionEngine()
				{

				}
				
			private:
				bool translateBlock(thread::ThreadInstance* thread, archsim::Address block_pc, bool support_chaining, bool support_profiling) override;	
				bool buildBlockJITIR(thread::ThreadInstance *thread, archsim::Address block_pc, captive::arch::jit::TranslationContext &ctx, archsim::blockjit::BlockTranslation &txln);
				
				llvm::LLVMContext llvm_ctx_;
				
				std::unique_ptr<llvm::TargetMachine> target_machine_;
				llvm::orc::RTDyldObjectLinkingLayer linker_;
				llvm::orc::IRCompileLayer<decltype(linker_), llvm::orc::SimpleCompiler> compiler_;
			};
		}
	}
}

#endif /* BLOCKLLVMEXECUTIONENGINE_H */

