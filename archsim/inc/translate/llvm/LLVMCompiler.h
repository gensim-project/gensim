/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef _LLVM_COMPILER_H
#define _LLVM_COMPILER_H

#include "translate/llvm/LLVMTranslation.h"
#include "translate/llvm/LLVMMemoryManager.h"
#include "translate/TranslationWorkUnit.h"

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
	namespace translate
	{
		namespace translate_llvm
		{

			class LLVMCompiledModuleHandle;

			class LLVMCompiler
			{
			public:
				using LinkLayer = llvm::orc::RTDyldObjectLinkingLayer;
				using CompileLayer = llvm::orc::IRCompileLayer<LinkLayer, llvm::orc::SimpleCompiler>;

				LLVMCompiler();

				LLVMCompiledModuleHandle AddModule(llvm::Module *module);

				LLVMTranslation *GetTranslation(LLVMCompiledModuleHandle &module, TranslationWorkUnit &twu);

				void GC()
				{
					code_pool.GC();
				}
			private:
				void initJitSymbols();

				std::shared_ptr<archsim::translate::translate_llvm::LLVMMemoryManager> memory_manager_;
				std::unique_ptr<llvm::TargetMachine> target_machine_;
				LinkLayer linker_;
				CompileLayer compiler_;
				std::map<std::string, void *> jit_symbols_;

				util::PagePool code_pool;
			};

			class LLVMCompiledModuleHandle
			{
			public:
				using HandleT = LLVMCompiler::CompileLayer::ModuleHandleT;

				LLVMCompiledModuleHandle(HandleT handle) : handle_(handle) {}
				HandleT Get()
				{
					return handle_;
				}
			private:
				HandleT handle_;
			};
		}
	}
}

#endif
