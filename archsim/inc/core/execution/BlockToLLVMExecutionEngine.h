/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


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
#include "translate/adapt/BlockJITToLLVM.h"
#include "BlockJITLLVMMemoryManager.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/LambdaResolver.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>

namespace archsim
{
	namespace core
	{
		namespace execution
		{

			class BlockToLLVMExecutionEngine : public BlockJITExecutionEngine
			{
			public:

				BlockToLLVMExecutionEngine(gensim::blockjit::BaseBlockJITTranslate *translator);
				virtual ~BlockToLLVMExecutionEngine()
				{

				}

				static ExecutionEngine *Factory(const archsim::module::ModuleInfo *module, const std::string &cpu_prefix);

			private:
				bool translateBlock(thread::ThreadInstance* thread, archsim::Address block_pc, bool support_chaining, bool support_profiling) override;
				bool buildBlockJITIR(thread::ThreadInstance *thread, archsim::Address block_pc, captive::arch::jit::TranslationContext &ctx, archsim::blockjit::BlockTranslation &txln);

				llvm::LLVMContext llvm_ctx_;
				archsim::translate::adapt::BlockJITToLLVMAdaptor adaptor_;

				std::shared_ptr<BlockJITLLVMMemoryManager> memory_manager_;
				std::unique_ptr<llvm::TargetMachine> target_machine_;
				llvm::orc::RTDyldObjectLinkingLayer linker_;
				llvm::orc::IRCompileLayer<decltype(linker_), llvm::orc::SimpleCompiler> compiler_;
			};
		}
	}
}

#endif /* BLOCKLLVMEXECUTIONENGINE_H */

