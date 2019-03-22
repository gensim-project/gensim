/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   BlockLLVMExecutionEngine.h
 * Author: harry
 *
 * Created on 27 August 2018, 14:09
 */

#ifndef BLOCKLLVMEXECUTIONENGINE_H
#define BLOCKLLVMEXECUTIONENGINE_H

#include "core/execution/BasicJITExecutionEngine.h"
#include "core/execution/BlockToLLVMExecutionEngine.h"
#include "module/Module.h"
#include "translate/llvm/LLVMCompiler.h"

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
			class BlockLLVMExecutionEngine : public BasicJITExecutionEngine
			{
			public:

				BlockLLVMExecutionEngine(gensim::BaseLLVMTranslate *translator);

				ExecutionEngineThreadContext* GetNewContext(thread::ThreadInstance* thread) override;

				static ExecutionEngine *Factory(const archsim::module::ModuleInfo *module, const std::string &cpu_prefix);

				llvm::LLVMContext &GetContext()
				{
					return *llvm_ctx_.getContext();
				}

			protected:
				bool translateBlock(thread::ThreadInstance* thread, archsim::Address block_pc, bool support_chaining, bool support_profiling) override;

			private:

				llvm::FunctionType *getFunctionType();

				llvm::Function *translateToFunction(archsim::core::thread::ThreadInstance *thread, Address phys_pc, const std::string fn_name, std::unique_ptr<llvm::Module> &llvm_module);

				gensim::BaseLLVMTranslate *translator_;

				llvm::orc::ThreadSafeContext llvm_ctx_;
				archsim::translate::translate_llvm::LLVMCompiler compiler_;

			};
		}
	}
}

#endif /* BLOCKLLVMEXECUTIONENGINE_H */

